#include <unistd.h>
#include <mctop.h>
#include <stdbool.h>
#include <semaphore.h>
#include <numaif.h>
#include <pthread.h>

#include "../include/heuristic_greedy.h"
#include "../include/list.h"

#include <vector>

// // dummy migration for testing purposes
// int migrate_pages(pid_t p, int i, long* x, long* y) {
//   return 0;
// }

typedef struct {
  pid_t pid;
  int maxnode;
  unsigned long* old_nodes;
  unsigned long* new_nodes;
} migrate_pages_dt;

void* migrate_pages_fn(void* dt) {
  migrate_pages_dt data = *((migrate_pages_dt*) dt);

  if (migrate_pages(data.pid, data.maxnode,
		      data.old_nodes,
		      data.new_nodes) == -1) {
      perror("couldn't migrate memory\n");
  }
  else {
    printf("memory migrated!\n");
  }

  return NULL;
}

typedef struct {
  pin_data** pin;
  mctop_t* topo;
  int total_hwcs;

  list* policy_per_pid;
  list* hwcs_per_pid;
  list* tids_per_pid;
  
  bool* used_hwcs;

  sem_t* lock;
} H_global_state;

H_global_state stateGREEDY;

int HGREEDY_release_hwc_extra(pid_t pid, pid_t tid) {
  sem_wait(stateGREEDY.lock);
  int *hwc_pt = ((int *) list_get_value(stateGREEDY.hwcs_per_pid, &tid, compare_pids));
  
  if (*hwc_pt == -1) {
    sem_post(stateGREEDY.lock);
    return -1;
  }

  int hwc = *hwc_pt;


  stateGREEDY.used_hwcs[hwc] = false;
  
  list_remove(stateGREEDY.hwcs_per_pid, &tid, compare_pids);
  int res = list_remove(stateGREEDY.tids_per_pid, &pid, compare_pids);
  if (res == 1) {
    printf("REMoved %lld\n", (long int) pid);
  }
  else {
    printf("Couldn't remove:  %lld\n", (long int) pid);
  }

  sem_post(stateGREEDY.lock);
  return hwc;
}

std::vector<int> HGREEDY_reschedule(pid_t pid, int new_policy) {
  printf("\n\n\n\n\nABOUT TO RESCHEDULE\n\n\n\n\n\n\n\n");
  int elems = 0;
  void** values = list_get_all_values(stateGREEDY.tids_per_pid, &pid, compare_pids, &elems);

  std::vector<int> result;

  printf("Elements: %d\n", elems);
  for (int i = 0; i < elems; ++i) {
    printf("%d: TID: %ld\n", i, *(pid_t *) values[i]);
    int hwc = HGREEDY_release_hwc_extra(pid, *((pid_t *) values[i]));
    //HGREEDY_release_hwc(pid);
  }
  printf("i managed to relase all the hwcs ...\n");
  int hwc = HGREEDY_release_hwc_extra(pid, pid);

  list_remove(stateGREEDY.policy_per_pid, &pid, compare_pids);


  if (new_policy == MCTOP_ALLOC_MIN_LAT_HWCS) {
    unsigned long old_nodes = 0xFF;
    unsigned long new_nodes = 0x01;
    /*if (migrate_pages(pid, 30,
		      &old_nodes,
		      &new_nodes) == -1) {
      perror("couldn't migrate memory\n");
    }
    else {
      printf(">>> memory migrated!\n");
      }*/ /*FIXME*/
  }
  
  HGREEDY_new_process(pid, new_policy);
  int node;
  int core = HGREEDY_get_hwc(pid, pid, &node);
  pin(pid, core, node);
  result.push_back(core);

  for (int i = 0; i < elems; ++i) {
    pid_t tid = *((pid_t *) values[i]);
    int core = HGREEDY_get_hwc(pid, tid, &node);
    pin(tid, core, node);
    result.push_back(core);
  }

  // FIXME
  if (new_policy != MCTOP_ALLOC_BW_ROUND_ROBIN_CORES) {

    // NOTE: migrating for pid, migrates the memory of its threads as well
    // Check experiment I executed (/localhome/kantonia/migrate_pages)
    // pthread_t migrate_thread[elems];
    // migrate_pages_dt data[elems];
    // unsigned long old_nodes = 0xFF;
    // unsigned long new_nodes = 0x1;

    // for (int i = 0; i < elems; ++i) {

    //   pid_t tid = *((pid_t *) values[i]);

      
    //   data[i].pid = tid;
    //   data[i].maxnode = 30;
    //   data[i].old_nodes = &old_nodes;
    //   data[i].new_nodes = &new_nodes;

    //   pthread_create(&migrate_thread[i], NULL, migrate_pages_fn, &data[i]);
    // }

    // for (int i = 0; i < elems; ++i) {
    //   pthread_join(migrate_thread[i], NULL);
    // }

  }

  return result;
}

void HGREEDY_init(pin_data** pin, mctop_t* topo) {
  printf("From inside greedy: ... %p\n");
  stateGREEDY.pin = pin;
  stateGREEDY.topo = topo;



  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);
  stateGREEDY.total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  stateGREEDY.policy_per_pid = create_list();
  stateGREEDY.hwcs_per_pid = create_list();
  stateGREEDY.tids_per_pid = create_list();

  
  stateGREEDY.used_hwcs = (bool* ) malloc(sizeof(bool) * stateGREEDY.total_hwcs);
  for (int i = 0; i < stateGREEDY.total_hwcs; ++i) {
    stateGREEDY.used_hwcs[i] = false;
  }
  
  stateGREEDY.lock = (sem_t *) malloc(sizeof(sem_t));
  if (sem_init(stateGREEDY.lock, 0, 1)) {
    perror("couldn't create lock for state\n");
  }
}

sem_t* HGREEDY_get_lock() {
  return stateGREEDY.lock;
}

void HGREEDY_new_process(pid_t pid, int policy) {
  pid_t* pid_pt = (pid_t *) malloc(sizeof(pid_t));
  *pid_pt = pid;

  int* policy_pt = (int *) malloc(sizeof(int));
  *policy_pt = policy;

  if (policy == MCTOP_ALLOC_NONE) {
    int* tmp = (int *) malloc(sizeof(int));
    *tmp = -1;
    list_add(stateGREEDY.hwcs_per_pid, pid_pt, tmp);
    list_add(stateGREEDY.policy_per_pid, pid_pt, policy_pt);
    return;
  }

  int* tmp = (int *) malloc(sizeof(int));
  *tmp = -2;
  list_add(stateGREEDY.hwcs_per_pid, pid_pt, tmp);
  list_add(stateGREEDY.policy_per_pid, pid_pt, policy_pt);
}

void HGREEDY_process_exit(pid_t pid) {
  //list_remove(stateGREEDY.hwcs_per_pid, &pid, compare_pids); // already done by leave_process
}


int HGREEDY_get_hwc(pid_t pid, pid_t tid, int* ret_node) {

  /* void* tmp_pt = list_get_value(stateGREEDY.hwcs_per_pid, &pid, compare_pids); */
  /* if (tmp_pt != NULL && *(int *) tmp_pt == -2) { */
  /*   list_remove(stateGREEDY.hwcs_per_pid, &pid, compare_pids); */
  /* } */
  
  void* policy_pt;
  do {
    policy_pt = list_get_value(stateGREEDY.policy_per_pid, &pid, compare_pids);
    usleep(5000);
  } while (policy_pt == NULL);
  int policy = *((int *) policy_pt);

  if (policy == MCTOP_ALLOC_NONE) {
    *ret_node = -1;

    int* tmp = (int *) malloc(sizeof(int));
    *tmp = -1;

    pid_t* pid_pt = (pid_t *) malloc(sizeof(pid_t));
    *pid_pt = pid;

    pid_t* tid_pt = (pid_t *) malloc(sizeof(pid_t));
    *tid_pt = tid;

    if (pid != tid) {
      list_add(stateGREEDY.tids_per_pid, pid_pt, tid_pt);
    }
    
    list_add(stateGREEDY.hwcs_per_pid, tid_pt, tmp);

    return -1;
  }

  // go through all the hwcs
  int cnt = 0;
  int hwc;
  while (cnt < stateGREEDY.total_hwcs) {
    pin_data pd = stateGREEDY.pin[policy][cnt];
    hwc = pd.core;
    int node = pd.node;
    *ret_node = node;

    if (!stateGREEDY.used_hwcs[hwc]) {
      goto end;

    }

    cnt++;
  }


  // update global state
 end: ;
  stateGREEDY.used_hwcs[hwc] = true;

  int*n = (int *) malloc(sizeof(int));
  *n = hwc;

  pid_t* pid_pt = (pid_t *) malloc(sizeof(pid_t));
  *pid_pt = pid;

  pid_t* tid_pt = (pid_t* )malloc(sizeof(pid_t));
  *tid_pt = tid;

  list_add(stateGREEDY.hwcs_per_pid, tid_pt, n);

  if (pid != tid) {
    list_add(stateGREEDY.tids_per_pid, pid_pt, tid_pt);
  }
  
  return hwc;
}

void HGREEDY_release_hwc(pid_t pid) {
  sem_wait(stateGREEDY.lock);
  int hwc = *((int *) list_get_value(stateGREEDY.hwcs_per_pid, &pid, compare_pids));
  
  if (hwc == -1) {
    sem_post(stateGREEDY.lock);
    return;
  }
  
  stateGREEDY.used_hwcs[hwc] = false;
  
  list_remove(stateGREEDY.hwcs_per_pid, &pid, compare_pids);
  int res = list_remove(stateGREEDY.tids_per_pid, &pid, compare_pids);
  if (res == 1) {
    printf("REMoved %lld\n", (long int) pid);
  }
  else {
    printf("Couldn't remove:  %lld\n", (long int) pid);
  }

  sem_post(stateGREEDY.lock);
}

