#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>

#include "heuristic.h"
#include "list.h"
#include "slate_utils.h"



typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* running_pids;
  list* rr_pids; // have to intersect with running to see what's happening
  
  list* policy_per_pid;
  list* hwcs_per_pid;
  list* tids_per_pid;
  
  list* used_sockets;
  list* used_cores;
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state state;

void HSPLIT_init(pin_data** pin, mctop_t* topo) {
  state.pin = pin;
  state.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  state.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  state.policy_per_pid = create_list();
  state.hwcs_per_pid = create_list();
  state.tids_per_pid = create_list();
  
  state.used_sockets = create_list();
  state.used_cores = create_list();

  state.running_pids = create_list();
  state.rr_pids = create_list();

  state.used_hwcs = malloc(sizeof(bool) * state.total_hwcs);
  for (int i = 0; i < state.total_hwcs; ++i) {
    state.used_hwcs[i] = false;
  }
  

  for (int i = 0; i < state.total_hwcs; ++i) {
    hwc_gs_t* core = mctop_hwcid_get_core(state.topo, i);
    void* core_vd = (void*) core;
    if (list_get_value(state.used_cores, core_vd, compare_voids) == NULL) {
      list_add(state.used_cores, core_vd, create_list());
    }
  }

  for (int i = 0; i < state.total_hwcs; ++i) {
    socket_t* socket = mctop_hwcid_get_socket(state.topo, i);
    void* socket_vd = (void*) socket;
    if (list_get_value(state.used_sockets, socket_vd, compare_voids) == NULL) {
      list_add(state.used_sockets, socket_vd, create_list());
    }
  }

  state.lock = malloc(sizeof(sem_t));
  if (sem_init(state.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* HSPLIT_get_lock() {
  return state.lock;
}



void reschedule(pid_t pid, void* sockets[], int num)
{
  int elems;
  void** elements = list_get_all_values(state.tids_per_pid, &pid, compare_pids, &elems);

  for (int i = 0; i < elems; ++i) {
    pid_t tid = *((pid_t *) elements[i]);

    int previous_hwc = *((int *) list_get_value(state.hwcs_per_pid, &tid, compare_pids));
    void* socket = (void*) mctop_hwcid_get_socket(state.topo, previous_hwc);

    bool is_in_desired_socket = false;
    for (int j = 0; j < num; ++j) {
      if (socket == sockets[j]) {
	is_in_desired_socket = true;
	printf("Already at the correct socket: %d\n", previous_hwc);
	goto next_iter;
      }
    }

    int new_hwc;
    int policy = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
    if (!is_in_desired_socket) {
      
      state.used_hwcs[previous_hwc] = false;

      // go through all the hwcs
      int cnt = 0;
      while (cnt < state.total_hwcs) {
	pin_data pd = state.pin[policy][cnt];
	new_hwc = pd.core;
	void* new_socket = (void*) mctop_hwcid_get_socket(state.topo, new_hwc);

	for (int j = 0; j < num; ++j) {
	  if (new_socket == sockets[j] && !state.used_hwcs[new_hwc]) {
	    goto endy;
	  }
	}

	cnt++;
      }
    }

  endy: ;
    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = pid;
  
    state.used_hwcs[new_hwc] = true;

    int*n = malloc(sizeof(int));
    *n = new_hwc;

    pid_t* tid_pt = malloc(sizeof(pid_t));
    *tid_pt = tid;

    printf("\tRescheduling thread: %ld from %d to %d\n", tid, previous_hwc, new_hwc);
    pin(tid, new_hwc, -1);


    list_remove(state.hwcs_per_pid, &tid, compare_pids);
    list_add(state.hwcs_per_pid, tid_pt, n);
    
    HSPLIT_release_hwc_no_lock(tid, previous_hwc);


    //    HSPLIT_get_hwc(tid);

    state.used_hwcs[previous_hwc] = false;
  next_iter: ;
  }

}

void HSPLIT_new_process(pid_t pid, int policy) {
  sem_wait(state.lock);

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = malloc(sizeof(int));
    *tmp = -1;
    list_add(state.running_pids, pid_pt, NULL);
    list_add(state.hwcs_per_pid, pid_pt, tmp);
    list_add(state.policy_per_pid, pid_pt, policy_pt);
    sem_post(state.lock);
    return;
  }

  if (policy == MCTOP_ALLOC_BW_ROUND_ROBIN_CORES) {
    void* socket1 = (void*) mctop_hwcid_get_socket(state.topo, 0);
    void* socket2 = (void*) mctop_hwcid_get_socket(state.topo, 1);
    void* socket3 = (void*) mctop_hwcid_get_socket(state.topo, 2);
    void* socket4 = (void*) mctop_hwcid_get_socket(state.topo, 3);
    printf("all the SOCKETS: %p %p %p %p\n", socket1, socket2, socket3, socket4);

    int rrs = list_elements(state.rr_pids);
    
    if  (rrs == 1) {
      pid_t* first = (pid_t *) list_get_nth(state.rr_pids, 0);
      void* sockets[2] = {socket1, socket2};      

      // reschedule first process to first 2 sockets
      printf("mpika edo alla pote den ...");
      reschedule(*first, sockets, 2);
      printf("VIKGA MORI\n");
    }
    else if  (rrs == 2) {
      pid_t* first = (pid_t *) list_get_nth(state.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(state.rr_pids, 1);

      void* sockets1[1] = {socket1};
      void* sockets2[1] = {socket2}; 

      // reschedule first two processes to first 2 sockets
      reschedule(*first, sockets1, 1);
      reschedule(*second, sockets2, 1);
    }
    else if  (rrs == 3) {
      pid_t* first = (pid_t *) list_get_nth(state.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(state.rr_pids, 1);
      pid_t* third = (pid_t *) list_get_nth(state.rr_pids, 2);

      void* sockets1[1] = {socket1};
      void* sockets2[1] = {socket2}; 
      void* sockets3[1] = {socket3}; 

      // reschedule first three processes to first 3 sockets
      reschedule(*first, sockets1, 1);
      reschedule(*second, sockets2, 1);
      reschedule(*third, sockets3, 1);
    }

    printf("kai goustaro ...\n");
    list_add(state.rr_pids, pid_pt, NULL);
    printf("RESCHEDULED DONE\n");
  }


  //  int* tmp = malloc(sizeof(int));
  //*tmp = -2;
  list_add(state.running_pids, pid_pt, NULL);
  //  list_add(state.hwcs_per_pid, pid_pt, tmp);
  list_add(state.policy_per_pid, pid_pt, policy_pt);
  printf("Both added\n");
  sem_post(state.lock);
}

void HSPLIT_process_exit(pid_t pid) {
  printf(" A Proecess just exited, let's kill it.\n");
  list_remove(state.running_pids, &pid, compare_pids);

  void* policy_pt;
  do {
    policy_pt = list_get_value(state.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  if (policy == MCTOP_ALLOC_BW_ROUND_ROBIN_CORES) {
    list_remove(state.rr_pids, &pid, compare_pids);
  }
  //list_remove(state.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}

int HSPLIT_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

  /* void* tmp_pt = list_get_value(state.hwcs_per_pid, &pid, compare_pids); */
  /* if (tmp_pt != NULL && *(int *) tmp_pt == -2) { */
  /*   list_remove(state.hwcs_per_pid, &pid, compare_pids); */
  /* } */

  
  void* policy_pt;
  do {
    policy_pt = list_get_value(state.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  if (policy == MCTOP_ALLOC_NONE) {
    *ret_node = -1;

    int* tmp = malloc(sizeof(int));
    *tmp = -1;

    pid_t* tid_pt = malloc(sizeof(pid_t));
    *tid_pt = tid;

    
    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = pid;

    if (pid != *tid_pt) {
      list_add(state.tids_per_pid, pid_pt, tid_pt);
    }

	
    list_add(state.hwcs_per_pid, tid_pt, tmp);

    return -1;
  }

  int hwc;
  int cnt = 0;
  // go through all the sockets first and see if there is one with no running process
  while (cnt < state.total_hwcs) {
    pin_data pd = state.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    void* socket = (void*) mctop_hwcid_get_socket(state.topo, hwc);
    list* lst = (list*) list_get_value(state.used_sockets, socket, compare_voids);

    if (lst == NULL) {
      goto end;
    }
    else {
      int num_elements;
      list_get_all_values(lst, &pid, compare_pids, &num_elements);
      int all_elements = list_elements(lst);
      if (num_elements == all_elements && !state.used_hwcs[hwc]) {
	goto end;
      }
    }

    cnt++;
  }

  // go through all the cores next and see if there is one with no running process
  cnt = 0;

  while (cnt < state.total_hwcs) {
    pin_data pd = state.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    void* core = (void*) mctop_hwcid_get_core(state.topo, hwc);
    list* lst = (list*) list_get_value(state.used_cores, core, compare_voids);
    if (lst == NULL) {
      goto end;
    }
    else {
      int num_elements;
      list_get_all_values(lst, &pid, compare_pids, &num_elements);
      int all_elements = list_elements(lst);
      if (num_elements == all_elements && !state.used_hwcs[hwc]) {
	goto end;
      }
    }

    cnt++;
  }

  // go through all the hwcs
  cnt = 0;
  while (cnt < state.total_hwcs) {
    pin_data pd = state.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    if (!state.used_hwcs[hwc]) {
      goto end;
    }

    cnt++;
  }

  // just return first hwc TODO... go random instead of always [0]
  cnt = 0;
  pin_data pd = state.pin[policy][cnt];
  hwc = pd.core;
  int node = pd.node;
  *ret_node = node;


  // update global state
 end: ;
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;
  void* socket_vd = (void*) mctop_hwcid_get_socket(state.topo, hwc);
  list* lst = list_get_value(state.used_sockets, socket_vd, compare_voids);
  if (lst == NULL) {
    perror("There is a missing socket from used_sockets");
    exit(1);
  }
  list_add(lst, pid_pt, NULL);

  void* core_vd = (void*) mctop_hwcid_get_core(state.topo, hwc);
  lst = list_get_value(state.used_cores, core_vd, compare_voids);
  if (lst == NULL) {
    perror("There is a missing core from used_cores");
    exit(1);
  }
  list_add(lst, pid_pt, NULL);

  state.used_hwcs[hwc] = true;

  int*n = malloc(sizeof(int));
  *n = hwc;

  pid_t* tid_pt = malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(state.hwcs_per_pid, tid_pt, n);


  if (pid != tid) {
    list_add(state.tids_per_pid, pid_pt, tid_pt);
  }

  return hwc;
}

void HSPLIT_release_hwc(pid_t pid) {
  sem_wait(state.lock);
  int hwc = *((int *) list_get_value(state.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
    sem_post(state.lock);
    return;
  }
  
  void* socket_vd = (void*) mctop_hwcid_get_socket(state.topo, hwc);
  list* lst = list_get_value(state.used_sockets, socket_vd, compare_voids);
  if (lst == NULL) {
    exit(1);
  }

  list_remove(lst, &pid, compare_pids);

  void* core_vd = (void*) mctop_hwcid_get_core(state.topo, hwc);
  lst = list_get_value(state.used_cores, core_vd, compare_voids);
  if (lst == NULL) {
    perror("h_release_hwc There is a missing core from used_cores");
    exit(1);
  }
  list_remove(lst, &pid, compare_pids);
	
  state.used_hwcs[hwc] = false;
  
  list_remove(state.hwcs_per_pid, &pid, compare_pids);

  sem_post(state.lock);
}

void HSPLIT_release_hwc_no_lock(pid_t pid, int hwc) {
  void* socket_vd = (void*) mctop_hwcid_get_socket(state.topo, hwc);
  list* lst = list_get_value(state.used_sockets, socket_vd, compare_voids);
  if (lst == NULL) {
    exit(1);
  }

  list_remove(lst, &pid, compare_pids);

  void* core_vd = (void*) mctop_hwcid_get_core(state.topo, hwc);
  lst = list_get_value(state.used_cores, core_vd, compare_voids);
  if (lst == NULL) {
    perror("h_release_hwc There is a missing core from used_cores");
    exit(1);
  }
  list_remove(lst, &pid, compare_pids);
	
  state.used_hwcs[hwc] = false;
  
  list_remove(state.hwcs_per_pid, &pid, compare_pids);
}

