#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>

#include "heuristic.h"
#include "list.h"

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* policy_per_pid;
  list* hwcs_per_pid;
  
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state state;

void HGREEDY_reschedule(pid_t pid, int new_policy) {}

void HGREEDY_init(pin_data** pin, mctop_t* topo) {
  printf("PAME OLOI MAZI GREEDY\n\n\n\tGREDY\n\n\t\tGREE\n");
  state.pin = pin;
  state.topo = topo;

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  state.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  state.policy_per_pid = create_list();
  state.hwcs_per_pid = create_list();
  
  state.used_hwcs = malloc(sizeof(bool) * state.total_hwcs);
  for (int i = 0; i < state.total_hwcs; ++i) {
    state.used_hwcs[i] = false;
  }
  
  state.lock = malloc(sizeof(sem_t));
  if (sem_init(state.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* HGREEDY_get_lock() {
  return state.lock;
}

void HGREEDY_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = malloc(sizeof(int));
    *tmp = -1;
    list_add(state.hwcs_per_pid, pid_pt, tmp);
    list_add(state.policy_per_pid, pid_pt, policy_pt);
    return;
  }

  int* tmp = malloc(sizeof(int));
  *tmp = -2;
  list_add(state.hwcs_per_pid, pid_pt, tmp);
  list_add(state.policy_per_pid, pid_pt, policy_pt);
}

void HGREEDY_process_exit(pid_t pid) {
  //list_remove(state.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}


int HGREEDY_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

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
    
    list_add(state.hwcs_per_pid, tid_pt, tmp);

    return -1;
  }

  int valid[48];

  int cnty = -1, i = 0;
  for (int j = 0; j < 12; ++j) {
    int hwc1 = i;
    cnty++;
    valid[cnty] = hwc1;

    int hwc2 = i + 48;
    cnty++;
    valid[cnty] = hwc2;

    int hwc3 = i + 1;
    cnty++;
    valid[cnty] = hwc3;

    int hwc4 = i + 49;
    cnty++;
    valid[cnty] = hwc4;

    i += 4;    
  }

  /*for (int k = 0; k < 48; ++k) {
    printf("CORESSS\n\n\tSSS: %d\n", valid[k]);
    }*/
  
  // go through all the hwcs
  int cnt = 0;
  int hwc;
  while (cnt < state.total_hwcs) {
    pin_data pd = state.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    if (!state.used_hwcs[hwc]) {
      for (int k = 0; k < 48; ++k) {
	if (valid[k] == hwc) { // FIXME remove me
	  goto end;
	}
      }
    }

    cnt++;
  }


  // update global state
 end: ;
  state.used_hwcs[hwc] = true;

  int*n = malloc(sizeof(int));
  *n = hwc;

  pid_t* tid_pt = malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(state.hwcs_per_pid, tid_pt, n);
  
  return hwc;
}

void HGREEDY_release_hwc(pid_t pid) {
  sem_wait(state.lock);
  int hwc = *((int *) list_get_value(state.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
    sem_post(state.lock);
    return;
  }
  
  state.used_hwcs[hwc] = false;
  
  list_remove(state.hwcs_per_pid, &pid, compare_pids);

  sem_post(state.lock);
}

