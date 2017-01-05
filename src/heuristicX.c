#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include "heuristic.h"
#include "list.h"

#define CONCAT(a, b) a##b

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* policy_per_pid;
  
  list* used_sockets;
  list* used_cores;
  bool* used_hwcs;
  
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
  state.used_sockets = create_list();
  state.used_cores = create_list();

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

}

void HX_new_process(pid_t pid, int policy) {

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = malloc(sizeof(int));
  *policy_pt = policy;
    
  list_add(state.policy_per_pid, pid_pt, policy_pt);
}

int HX_get_hwc(pid_t pid, int* ret_node) {

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

  int hwc;
  int cnt = 0;

  // go through all the hwcs

  return 5;
  
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
  return hwc;
}

void HX_release_hwc(int hwc, pid_t pid) {

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
}

