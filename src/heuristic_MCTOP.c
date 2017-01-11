#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>

#include "heuristic_MCTOP.h"
#include "list.h"

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* counter_per_pid;
  list* policy_per_pid;
  list* hwcs_per_pid;
  
  list* used_sockets;
  list* used_cores;
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state state;

void HMCTOP_init(pin_data** pin, mctop_t* topo) {
  state.pin = pin;
  state.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  state.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  state.counter_per_pid = create_list();
  state.policy_per_pid = create_list();

  state.lock = malloc(sizeof(sem_t));
  if (sem_init(state.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* HMCTOP_get_lock() {
  return state.lock;
}

void HMCTOP_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    list_add(state.policy_per_pid, pid_pt, policy_pt);
    return;
  }

  list_add(state.policy_per_pid, pid_pt, policy_pt);

  int* zero = malloc(sizeof(int));
  *zero = 0;
  list_add(state.counter_per_pid, pid_pt, zero);
}

void HMCTOP_process_exit(pid_t pid) {
  //list_remove(state.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}


int HMCTOP_get_hwc(pid_t pid, pid_t tid, int* ret_node) {
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;
  
  void* policy_pt;
  do {
    policy_pt = list_get_value(state.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);


  if (policy == MCTOP_ALLOC_NONE) {
    *ret_node = -1;
    return -1;
  }

  int cnt = *((int *) list_get_value(state.counter_per_pid, &pid, compare_pids));

  int core = state.pin[policy][cnt].core;
  list_remove(state.counter_per_pid, &pid, compare_pids);

  int* plus_one = malloc(sizeof(int));
  *plus_one = cnt + 1;
  list_add(state.counter_per_pid, pid_pt, plus_one);
  *ret_node = 0;
  return core;
}

void HMCTOP_release_hwc(pid_t pid) {
  sem_wait(state.lock);

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* cnt = ((int *) list_get_value(state.counter_per_pid, &pid, compare_pids));
  if (cnt != NULL) {

    list_remove(state.counter_per_pid, &pid, compare_pids);

    int* minus_one = malloc(sizeof(int));
    *minus_one = cnt - 1;
    list_add(state.counter_per_pid, pid_pt, minus_one);
  }  
  sem_post(state.lock);
}

