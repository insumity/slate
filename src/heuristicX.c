#define _GNU_SOURCE
#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <sched.h>
#include "heuristic.h"
#include "list.h"

#define CONCAT(a, b) a##b

// WHEN A PROCESS / THREAD leaves it removes the tids from tids_per_pid
// as well as hwcs_per_pid ... so that's the reason reschedule is not finding anythin
typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* running_pids;
  list* hwcs_per_pid;
  list* tids_per_pid;
  
  list* policy_per_pid;
  list* used_sockets;
  list* used_cores;
  bool* used_hwcs;

  sem_t* lock;
} HX_global_state;

HX_global_state state;

void HX_init(pin_data** pin, mctop_t* topo) {
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

sem_t* HX_get_lock() {
  return state.lock;
}

void HX_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;
    
  list_add(state.policy_per_pid, pid_pt, policy_pt);
  list_add(state.running_pids, pid_pt, NULL);
}

void reschedule_on_exit(pid_t pid);

void HX_process_exit(pid_t pid) {
  list_remove(state.running_pids, &pid, compare_pids);
  //list_remove(state.hwcs_per_pid, &pid, compare_pids);

  reschedule_on_exit(pid);
}


int HX_get_hwc(pid_t pid, pid_t tid, int* ret_node) {
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

  int hwc;
  int cnt = 0;

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

  pid_t* parent_pid = malloc(sizeof(pid_t));
  *parent_pid = getppid();

  if (pid != tid) {
    list_add(state.tids_per_pid, pid_pt, tid_pt);
  }

  return hwc;
}

void pin(pid_t pid, int core, int node)
{
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(core, &set);

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) != 0) {
    perror("sched affinity inside pin didn't work. Coudld be that process has " \
	   "already terminated.");
    exit(1);
  }
}

void HX_release_hwc_no_lock(pid_t pid) {

  int hwc = *((int *) list_get_value(state.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
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
}

void reschedule_for_process(pid_t pid) {
  int previous_core = *(int *) list_get_value(state.hwcs_per_pid, &pid, compare_pids);
  HX_release_hwc_no_lock(pid);
  
  int node;
  int core = HX_get_hwc(pid, pid, &node);

  pin(pid, core, node);
  
  int num_elements;
  void** tids = list_get_all_values(state.tids_per_pid, &pid, compare_pids, &num_elements);

  for (int i = 0; i < num_elements; ++i) {
    pid_t tid = *((pid_t *) tids[i]);
    previous_core = *(int *) list_get_value(state.hwcs_per_pid, &tid, compare_pids);
    HX_release_hwc_no_lock(tid);

    core = HX_get_hwc(pid, tid, &node);

    pin(tid, core, node);
    printf("Rescheduling from %d to %d\n", previous_core, core);
  }
} 

void reschedule_on_exit(pid_t in_pid) {
  sem_wait(state.lock);
  struct node* tmp = (state.running_pids)->head;

  while (tmp != NULL) {
    pid_t* pid = (pid_t *) tmp->key;

    if (*pid != in_pid) {
      reschedule_for_process(*pid);
      break; // just reschedule one process
    }

    tmp = tmp->next;
  }

  sem_post(state.lock);
}

void HX_release_hwc(pid_t pid) {
  sem_wait(state.lock);
  int* hwc_pt = (int *) list_get_value(state.hwcs_per_pid, &pid, compare_pids);
  if (hwc_pt == NULL) {
    perror("(pid, hwc) was already removed from hwcs_per_pid");
    exit(1);
  }
  int hwc = *hwc_pt;

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

