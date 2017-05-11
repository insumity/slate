#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>

#include <vector>

#include "heuristic.h"
#include "../include/list.h"

#define CONCAT(a, b) a##b

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* policy_per_pid;
  list* hwcs_per_pid;
  
  list* used_sockets;
  list* used_cores;
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state state;

std::vector<int> H_reschedule(pid_t pid, int new_policy) {
  return std::vector<int>();
}

void H_init(pin_data** pin, mctop_t* topo) {
  state.pin = pin;
  state.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  state.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  state.policy_per_pid = create_list();
  state.hwcs_per_pid = create_list();
  
  state.used_sockets = create_list();
  state.used_cores = create_list();

  state.used_hwcs = (bool*) malloc(sizeof(bool) * state.total_hwcs);
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

  state.lock = (sem_t*) malloc(sizeof(sem_t));
  if (sem_init(state.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* H_get_lock() {
  return state.lock;
}

void H_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = (pid_t *) malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = (int*)malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = (int*)malloc(sizeof(int));
    *tmp = -1;
    list_add(state.hwcs_per_pid, pid_pt, tmp);
    list_add(state.policy_per_pid, pid_pt, policy_pt);
    return;
  }

  int* tmp = (int*)malloc(sizeof(int));
  *tmp = -2;
  list_add(state.hwcs_per_pid, pid_pt, tmp);
  list_add(state.policy_per_pid, pid_pt, policy_pt);
}

void H_process_exit(pid_t pid) {
  //list_remove(state.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}


int H_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

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

    int* tmp = (int*)malloc(sizeof(int));
    *tmp = -1;

    pid_t* tid_pt = (pid_t *)malloc(sizeof(pid_t));
    *tid_pt = tid;
    
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
  pid_t* pid_pt = (pid_t*)malloc(sizeof(pid_t));
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

  int*n = (int*)malloc(sizeof(int));
  *n = hwc;

  pid_t* tid_pt = (pid_t*)malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(state.hwcs_per_pid, tid_pt, n);

  
  return hwc;
}

void H_release_hwc(pid_t pid) {
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

