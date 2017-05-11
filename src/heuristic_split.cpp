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

  list* running_pids;
  list* rr_pids; // have to intersect with running to see what's happening
  list* tids_per_pid;
  
  list* policy_per_pid;
  list* hwcs_per_pid;
  
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state stateSPLIT;

void HSPLIT_init(pin_data** pin, mctop_t* topo) {
  stateSPLIT.pin = pin;
  stateSPLIT.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  stateSPLIT.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  
  stateSPLIT.policy_per_pid = create_list();
  stateSPLIT.hwcs_per_pid = create_list();

  stateSPLIT.tids_per_pid = create_list();
  stateSPLIT.running_pids = create_list();
  stateSPLIT.rr_pids = create_list();

  
  stateSPLIT.used_hwcs = malloc(sizeof(bool) * stateSPLIT.total_hwcs);
  for (int i = 0; i < stateSPLIT.total_hwcs; ++i) {
    stateSPLIT.used_hwcs[i] = false;
  }
  
  stateSPLIT.lock = malloc(sizeof(sem_t));
  if (sem_init(stateSPLIT.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* HSPLIT_get_lock() {
  return stateSPLIT.lock;
}


void reschedule(pid_t pid, void* sockets[], int num)
{
  int elems;
  void** elements = list_get_all_values(stateSPLIT.tids_per_pid, &pid, compare_pids, &elems);

  for (int i = 0; i < elems; ++i) {
    pid_t tid = *((pid_t *) elements[i]);

    int previous_hwc = *((int *) list_get_value(stateSPLIT.hwcs_per_pid, &tid, compare_pids));
    void* socket = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, previous_hwc);

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
      
      stateSPLIT.used_hwcs[previous_hwc] = false;

      // go through all the hwcs
      int cnt = 0;
      while (cnt < stateSPLIT.total_hwcs) {
	pin_data pd = stateSPLIT.pin[policy][cnt];
	new_hwc = pd.core;
	void* new_socket = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, new_hwc);

	for (int j = 0; j < num; ++j) {
	  if (new_socket == sockets[j] && !stateSPLIT.used_hwcs[new_hwc]) {
	    goto endy;
	  }
	}

	cnt++;
      }
    }

  endy: ;
    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = pid;
  
    stateSPLIT.used_hwcs[new_hwc] = true;

    int*n = malloc(sizeof(int));
    *n = new_hwc;

    pid_t* tid_pt = malloc(sizeof(pid_t));
    *tid_pt = tid;

    printf("\tRescheduling thread: %ld from %d to %d\n", tid, previous_hwc, new_hwc);
    pin(tid, new_hwc, -1);


    list_remove(stateSPLIT.hwcs_per_pid, &tid, compare_pids);
    list_add(stateSPLIT.hwcs_per_pid, tid_pt, n);
    
    //    HSPLIT_get_hwc(tid);

    stateSPLIT.used_hwcs[previous_hwc] = false;
  next_iter: ;
  }

}

void HSPLIT_new_process(pid_t pid, int policy) {
  sem_wait(stateSPLIT.lock);

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = malloc(sizeof(int));
    *tmp = -1;
    list_add(stateSPLIT.running_pids, pid_pt, NULL);
    list_add(stateSPLIT.hwcs_per_pid, pid_pt, tmp);
    list_add(stateSPLIT.policy_per_pid, pid_pt, policy_pt);
    sem_post(stateSPLIT.lock);
    return;
  }

  if (policy == MCTOP_ALLOC_BW_ROUND_ROBIN_CORES) {
    void* socket1 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 0);
    void* socket2 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 1);
    void* socket3 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 2);
    void* socket4 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 3);
    printf("all the SOCKETS: %p %p %p %p\n", socket1, socket2, socket3, socket4);

    int rrs = list_elements(stateSPLIT.rr_pids);
    
    if  (rrs == 1) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      void* sockets[2] = {socket1, socket2};      

      // reschedule first process to first 2 sockets
      printf("mpika edo alla pote den ...");
      reschedule(*first, sockets, 2);
      printf("VIKGA MORI\n");
    }
    else if  (rrs == 2) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 1);

      void* sockets1[1] = {socket1};
      void* sockets2[1] = {socket2}; 

      // reschedule first two processes to first 2 sockets
      reschedule(*first, sockets1, 1);
      reschedule(*second, sockets2, 1);
    }
    else if  (rrs == 3) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 1);
      pid_t* third = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 2);

      void* sockets1[1] = {socket1};
      void* sockets2[1] = {socket2}; 
      void* sockets3[1] = {socket3}; 

      // reschedule first three processes to first 3 sockets
      reschedule(*first, sockets1, 1);
      reschedule(*second, sockets2, 1);
      reschedule(*third, sockets3, 1);
    }

    printf("kai goustaro ...\n");
    list_add(stateSPLIT.rr_pids, pid_pt, NULL);
    printf("RESCHEDULED DONE\n");
  }


  //  int* tmp = malloc(sizeof(int));
  //*tmp = -2;
  list_add(stateSPLIT.running_pids, pid_pt, NULL);
  //  list_add(stateSPLIT.hwcs_per_pid, pid_pt, tmp);
  list_add(stateSPLIT.policy_per_pid, pid_pt, policy_pt);
  printf("Both added\n");
  sem_post(stateSPLIT.lock);
}


void HSPLIT_process_exit(pid_t pid) {
    list_remove(stateSPLIT.running_pids, &pid, compare_pids);

  void* policy_pt;
  do {
    policy_pt = list_get_value(stateSPLIT.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  if (policy == MCTOP_ALLOC_BW_ROUND_ROBIN_CORES) {
    list_remove(stateSPLIT.rr_pids, &pid, compare_pids);
    void* socket1 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 0);
    void* socket2 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 1);
    void* socket3 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 2);
    void* socket4 = (void*) mctop_hwcid_get_socket(stateSPLIT.topo, 3);
    printf("all the SOCKETS: %p %p %p %p\n", socket1, socket2, socket3, socket4);

    int rrs = list_elements(stateSPLIT.rr_pids);
    if (rrs = 0) {
    }
    else if  (rrs == 1) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      void* sockets[4] = {socket1, socket2, socket3, socket4};
      // reschedule first process to first 2 sockets
      printf("mpika edo alla pote den ...");
      reschedule(*first, sockets, 4);
      printf("VIKGA MORI\n");
    }
    else if  (rrs == 2) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 1);

      void* sockets1[2] = {socket1, socket2};
      void* sockets2[2] = {socket3, socket4}; 

      // reschedule first two processes to first 2 sockets
      reschedule(*first, sockets1, 2);
      reschedule(*second, sockets2, 2);
    }
    else if  (rrs == 3) {
      pid_t* first = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 0);
      pid_t* second = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 1);
      pid_t* third = (pid_t *) list_get_nth(stateSPLIT.rr_pids, 2);

      void* sockets1[2] = {socket1, socket4};
      void* sockets2[1] = {socket2}; 
      void* sockets3[1] = {socket3}; 

      // reschedule first three processes to first 3 sockets
      reschedule(*first, sockets1, 2);
      reschedule(*second, sockets2, 1);
      reschedule(*third, sockets3, 1);
    }

    printf("RESCHEDULED \t\t\n\t\tWHEN REM\n\tREMONVING DONE\t\n\tDONE\n");
  }

}


int HSPLIT_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

  /* void* tmp_pt = list_get_value(stateSPLIT.hwcs_per_pid, &pid, compare_pids); */
  /* if (tmp_pt != NULL && *(int *) tmp_pt == -2) { */
  /*   list_remove(stateSPLIT.hwcs_per_pid, &pid, compare_pids); */
  /* } */
  
  void* policy_pt;
  do {
    policy_pt = list_get_value(stateSPLIT.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  if (policy == MCTOP_ALLOC_NONE) {
    *ret_node = -1;

    int* tmp = malloc(sizeof(int));
    *tmp = -1;

    pid_t* pid_pt = malloc(sizeof(pid_t));
    *pid_pt = tid;

    pid_t* tid_pt = malloc(sizeof(pid_t));
    *tid_pt = tid;
    if (pid != *tid_pt) {
      list_add(stateSPLIT.tids_per_pid, pid_pt, tid_pt);
    }

    
    list_add(stateSPLIT.hwcs_per_pid, tid_pt, tmp);

    return -1;
  }

  
  // go through all the hwcs
  int cnt = 0;
  int hwc;
  while (cnt < stateSPLIT.total_hwcs) {
    pin_data pd = stateSPLIT.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    if (!stateSPLIT.used_hwcs[hwc]) {
      goto end;
    }

    cnt++;
  }


  // update global state
 end: ;
  stateSPLIT.used_hwcs[hwc] = true;

  int*n = malloc(sizeof(int));
  *n = hwc;

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  
  pid_t* tid_pt = malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(stateSPLIT.hwcs_per_pid, tid_pt, n);

  if (pid != tid) {
    list_add(stateSPLIT.tids_per_pid, pid_pt, tid_pt);
  }

  return hwc;
}

void HSPLIT_release_hwc(pid_t pid) {
  sem_wait(stateSPLIT.lock);
  int hwc = *((int *) list_get_value(stateSPLIT.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
    sem_post(stateSPLIT.lock);
    return;
  }
  
  stateSPLIT.used_hwcs[hwc] = false;
  
  list_remove(stateSPLIT.hwcs_per_pid, &pid, compare_pids);

  sem_post(stateSPLIT.lock);
}


