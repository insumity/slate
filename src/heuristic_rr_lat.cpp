#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>

#include "heuristic.h"
#include "../include/list.h"

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* lat_sockets; // sockets used by lat processes
  list* running_pids;
  list* lat_pids; // processes pids running with a min_lat policy
  list* tids_per_pid;
  
  list* policy_per_pid;
  list* hwcs_per_pid;
  
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state stateRRLAT;

void HRRLAT_init(pin_data** pin, mctop_t* topo) {
  stateRRLAT.pin = pin;
  stateRRLAT.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  stateRRLAT.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  stateRRLAT.lat_sockets = create_list();
  stateRRLAT.running_pids = create_list();
  stateRRLAT.lat_pids = create_list();
  stateRRLAT.tids_per_pid = create_list();
  
  stateRRLAT.policy_per_pid = create_list();
  stateRRLAT.hwcs_per_pid = create_list();

  stateRRLAT.used_hwcs = malloc(sizeof(bool) * stateRRLAT.total_hwcs);
  for (int i = 0; i < stateRRLAT.total_hwcs; ++i) {
    stateRRLAT.used_hwcs[i] = false;
  }
  
  stateRRLAT.lock = malloc(sizeof(sem_t));
  if (sem_init(stateRRLAT.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

void* find_free_lat_socket(pid_t pid) {

  for (int i = 0; i < 4; ++i) {
    void* s = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, i);

    if (list_get_value(stateRRLAT.lat_sockets, s, compare_voids) == NULL) {
      return s;
    }
    else {
      void* foo = list_get_value(stateRRLAT.lat_sockets, s, compare_voids);
      if (compare_pids(foo, &pid)) {
	return s;
      }
    }
  }

  return NULL;
}


sem_t* HRRLAT_get_lock() {
  return stateRRLAT.lock;
}

void HRRLAT_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  list_add(stateRRLAT.running_pids, pid_pt, NULL);
  
  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = malloc(sizeof(int));
    *tmp = -1;
    list_add(stateRRLAT.hwcs_per_pid, pid_pt, tmp);
    list_add(stateRRLAT.policy_per_pid, pid_pt, policy_pt);
    return;
  }

  if (policy == MCTOP_ALLOC_MIN_LAT_HWCS) {
    list_add(stateRRLAT.lat_pids, pid_pt, NULL);
  }

  int* tmp = malloc(sizeof(int));
  *tmp = -2;
  list_add(stateRRLAT.hwcs_per_pid, pid_pt, tmp);
  list_add(stateRRLAT.policy_per_pid, pid_pt, policy_pt);
}

void HRRLAT_process_exit(pid_t pid) {
  list_remove(stateRRLAT.running_pids, &pid, compare_pids);

  int policy = *((int *) list_get_value(stateRRLAT.policy_per_pid, &pid, compare_pids));
  if (policy == MCTOP_ALLOC_MIN_LAT_HWCS) {
    list_remove(stateRRLAT.lat_pids, &pid, compare_pids);
  }
    
  list_remove(stateRRLAT.running_pids, &pid, compare_pids);
  //list_remove(stateRRLAT.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}

void clean_socket(void* toSockets[], int num)
{
  list* l = stateRRLAT.running_pids;
  sem_wait(&(l->lock));
  struct node *tmp = l->head;
  while (tmp != NULL) {
    pid_t pid = *((pid_t*) tmp->key);

    int pol = *((int *) list_get_value(stateRRLAT.policy_per_pid, &pid, compare_pids));
    if (pol == MCTOP_ALLOC_MIN_LAT_HWCS) {
      tmp = tmp->next;
      continue;
    }
    
    int num_tids;
    void** tids = list_get_all_values(stateRRLAT.tids_per_pid, &pid, compare_pids, &num_tids);

    for (int i = 0; i < num_tids; ++i) {
      pid_t tid = *((pid_t *) tids[i]);
      // get the hwc of the tid
      int* hwc_pt = (int *) list_get_value(stateRRLAT.hwcs_per_pid, &tid, compare_pids);
      if (hwc_pt == NULL) {
	perror("Read hwc of thread is NULL. Heh?\n");
	exit(EXIT_FAILURE);
      }

      int previous_hwc = *hwc_pt;
      void* socket = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, previous_hwc);
      bool is_in = false;
      for (int j = 0; j < num; ++j) {
	if (toSockets[j] == socket) {
	  is_in = true;
	  break;
	}
      }

      if (is_in) {
	int* pol_pt = (int *) list_get_value(stateRRLAT.policy_per_pid, &pid, compare_pids);
	if (pol_pt == NULL) {
	  perror("policy is NULL.");
	  exit(EXIT_FAILURE);
	}
	int pol = *pol_pt;

	// go through the sockets shit and repin
	// go through all the hwcs
	int cnt = 0;
	int new_hwc;
	while (cnt < stateRRLAT.total_hwcs) {
	  pin_data pd = stateRRLAT.pin[pol][cnt];
	  new_hwc = pd.core;
	  //int node = pd.node;
	  
	  void* socket = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, new_hwc);
	  bool is_in = false;
	  for (int j = 0; j < num; ++j) {
	    if (toSockets[j] == socket) {
	      is_in = true;
	      break;
	    }
	  }

	  if (!stateRRLAT.used_hwcs[new_hwc] && !is_in) {
	    goto end;
	  }

	  cnt++;
	}

	// update global state
      end: ;

	// clean sate
	stateRRLAT.used_hwcs[previous_hwc] = false;
	list_remove(stateRRLAT.hwcs_per_pid, &tid, compare_pids);

	// also migrate memory
	/*void* previous_s = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, previous_hwc);
	void* new_s = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, new_hwc);

	if (previous_s != new_s) {
	  void* socketsy[4] = { (void*) mctop_hwcid_get_socket(stateRRLAT.topo, 0),
				(void*) mctop_hwcid_get_socket(stateRRLAT.topo, 1),
				(void*) mctop_hwcid_get_socket(stateRRLAT.topo, 2),
				(void*) mctop_hwcid_get_socket(stateRRLAT.topo, 3)};

	  int old_node = 0, new_node;
	  for (int i = 0; i < 4; ++i) {
	    if (previous_s == socketsy[i]) {
	      old_node = i;
	    }
	    if (new_s == socketsy[i]) {
	      new_node = i;
	    }
	  }
	  

	  int old = 1 << old_node;
	  int new = 1 << new_node;
	  if (migrate_pages(tid, 30, &old, &new) == -1) {
	    perror("Couldn't migrate pages");
	    exit(EXIT_FAILURE);
	  }
	  else {
	    printf("Successfully migrated pages!\n");
	  }
	  
	}
	*/
	
	// FIXME ... remove above. Just to check the code.

	
	pin(tid, new_hwc, -1);
	stateRRLAT.used_hwcs[new_hwc] = true;

	int*n = malloc(sizeof(int));
	*n = new_hwc;

	pid_t* tid_pt = malloc(sizeof(pid_t));
	*tid_pt = tid;

	list_add(stateRRLAT.hwcs_per_pid, tid_pt, n);

	if (pid != tid) {
	  pid_t* pid_pt = malloc(sizeof(pid_t));
	  *pid_pt = pid;

	  pid_t* tid_pt = malloc(sizeof(pid_t));
	  *tid_pt = pid;

	  list_add(stateRRLAT.tids_per_pid, pid_pt, tid_pt);
	}
      }
      
    }
    
    
    tmp = tmp->next;
  }
  sem_post(&(l->lock));
}

int get_unused_hwc_at_socket(int pol, void* socket) {
  // go through all the hwcs
  int cnt = 0;
  int hwc;
  while (cnt < stateRRLAT.total_hwcs) {
    pin_data pd = stateRRLAT.pin[pol][cnt];
    hwc = pd.core;
    int node = pd.node;

    void* s = (void*) mctop_hwcid_get_socket(stateRRLAT.topo, hwc);
    bool is_in = false;
    if (s == socket) {
      is_in = true;
    }

    if (!stateRRLAT.used_hwcs[hwc] && is_in) {
      return hwc;

    }

    cnt++;
  }

  assert("Never say never!\n");
  return -1;
}

int HRRLAT_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

  /* void* tmp_pt = list_get_value(stateRRLAT.hwcs_per_pid, &pid, compare_pids); */
  /* if (tmp_pt != NULL && *(int *) tmp_pt == -2) { */
  /*   list_remove(stateRRLAT.hwcs_per_pid, &pid, compare_pids); */
  /* } */
  
  void* policy_pt;
  do {
    policy_pt = list_get_value(stateRRLAT.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  int hwc = -1;
  if (policy == MCTOP_ALLOC_NONE) {
    *ret_node = -1;

    int* tmp = malloc(sizeof(int));
    *tmp = -1;

    pid_t* tid_pt = malloc(sizeof(pid_t));
    *tid_pt = tid;
    
    list_add(stateRRLAT.hwcs_per_pid, tid_pt, tmp);
    return -1;
  }
  else if (policy == MCTOP_ALLOC_MIN_LAT_HWCS) {
    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = pid;
    

    int pol = policy;
    int num = list_elements(stateRRLAT.lat_pids);
    if (num <= 4) {
      void* s = find_free_lat_socket(pid);
      void* sockets[] = { s };
      clean_socket(sockets, 1);
      hwc = get_unused_hwc_at_socket(pol, s);
      printf("RECEIVED HWCS: %d\n", hwc);

      if (list_get_value(stateRRLAT.lat_sockets, s, compare_voids) == NULL) {
	list_add(stateRRLAT.lat_sockets, s, pid_pt);
      }
    }
    else {
      // schedule wherever ...
      ;
    }

  }


  if (hwc == -1) {
  // go through all the hwcs
    int cnt = 0;
    while (cnt < stateRRLAT.total_hwcs) {
      pin_data pd = stateRRLAT.pin[policy][cnt];
      hwc = pd.core;
      int node = pd.node;
      *ret_node = node;

      if (!stateRRLAT.used_hwcs[hwc]) {
	goto end;

      }

      cnt++;
    }
  }

  // update global state
 end: ;
  stateRRLAT.used_hwcs[hwc] = true;

  int*n = malloc(sizeof(int));
  *n = hwc;

  pid_t* tid_pt = malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(stateRRLAT.hwcs_per_pid, tid_pt, n);

  if (pid != tid) {
    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = pid;

    list_add(stateRRLAT.tids_per_pid, pid_pt, tid_pt);
  }
  
  return hwc;
}

void HRRLAT_release_hwc(pid_t pid) {
  sem_wait(stateRRLAT.lock);
  int hwc = *((int *) list_get_value(stateRRLAT.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
    sem_post(stateRRLAT.lock);
    return;
  }
  
  stateRRLAT.used_hwcs[hwc] = false;
  list_remove(stateRRLAT.hwcs_per_pid, &pid, compare_pids);
  list_remove_all_data(stateRRLAT.lat_sockets, &pid, compare_pids);

  sem_post(stateRRLAT.lock);
}

