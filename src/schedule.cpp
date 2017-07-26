#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <mctop.h>
#include <mctop_alloc.h>
#include <sys/wait.h>
#include <time.h>
#include <numaif.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>
#include <numaif.h>
#include <linux/hw_breakpoint.h>

#include "ticket.h"
#include "slate_utils.h"
#include "../include/list.h"
#include "../include/read_counters.h" // FIXME: wouldn't compile otherwise. I've no idea why!
#include "../include/read_context_switches.h" // FIXME: wouldn't compile otherwise. I've no idea why!
#include "read_memory_bandwidth.h"
#include "get_tids.h"

#include "heuristic.h"
#include "../include/heuristicX.h"
#include "heuristic_MCTOP.h"
#include "heuristic_greedy.h"
#include "../include/heuristic_split.h"
#include "heuristic_rr_lat.h"
#include "python_classify.h"

#include <iostream>
#include <map>
#include <vector>
#include <string>

#include <memory>
#include <set>
#include <ctime>

#define COUNTERS_NUMBER 10

volatile classifier_data* classify_data; // communication with python classifier (scikitlearn)

int initial_policy = 1; // WITH what policy should we start the scheduler, 0 for MIN_LAT_CORES, 1 for BW_ROUND_ROBIN_CORES, can be changed through command line

// returns the old value if new_value - old_value < 00
long long get_difference(long long new_value, long long old_value) {
  return (new_value - old_value >= 0)? new_value - old_value: old_value;
}

long long* subtract(long long* ar1, long long* ar2, int length) {

  long long* result = (long long *) malloc(sizeof(long long) * length);

  for (int i = 0 ; i < length; ++i) {
    result[i] = get_difference(ar1[i], ar2[i]);
  }

  return result;
}


void print_perf_counters(bool is_rr, int result, long long values[], long long context_switches, double sockets_bw[], int number_of_threads) {

  if (is_rr) {
    std::cerr << "0, 1, ";
  }
  else {
    std::cerr << "1, 0, ";
  }

  for (int i = 0; i < COUNTERS_NUMBER; ++i) {
    std::cerr << values[i] << ", ";
  }
  std::cerr << context_switches << ", ";
  for (int i = 0; i < 4; ++i) {
    std::cerr << sockets_bw[i] << ", ";
  }

  std::cerr << number_of_threads << ", ";

  std::cerr << result << std::endl;
}

// returns the current values of the read performance counters
long long* read_perf_counters(std::vector<int> hwcs) {


  // Events being read by start_counter in this order
  // MEM_LOAD_UOPS_RETIRED:L3_HIT
  // MEM_LOAD_UOPS_RETIRED:L3_MISS
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:LOCAL_DRAM
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM
  // MEM_LOAD_UOPS_RETIRED:L2_MISS
  // UOPS_RETIRED
  // UNHALTED_CORE_CYCLES
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_FWD
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_HITM
  // INSTRUCTIONS_RETIRED

  long long* results = (long long *) malloc(sizeof(long long) * COUNTERS_NUMBER);
  long long L3_HIT = 0; 
  long long L3_MISS = 0;
  long long LOCAL_DRAM = 0;
  long long REMOTE_DRAM = 0;
  long long L2_MISS = 0;
  long long UOPS_RETIRED = 0;
  long long UNHALTED_CORE_CYCLES = 0;
  long long REMOTE_FWD = 0;
  long long REMOTE_HITM = 0;
  long long INSTRUCTIONS_RETIRED = 0;
  

  for (std::vector<int>::iterator it = hwcs.begin(); it != hwcs.end(); it++) {
    int cpu = *it;

    core_data cd = get_counters(cpu);
    L3_HIT += cd.values[0];
    L3_MISS += cd.values[1];
    LOCAL_DRAM += cd.values[2];
    REMOTE_DRAM += cd.values[3];
    L2_MISS += cd.values[4];
    UOPS_RETIRED += cd.values[5];
    UNHALTED_CORE_CYCLES += cd.values[6];
    REMOTE_FWD += cd.values[7];
    REMOTE_HITM += cd.values[8];
    INSTRUCTIONS_RETIRED += cd.values[9];

  }

  results[0] = L3_HIT;
  results[1] = L3_MISS;
  results[2] = LOCAL_DRAM;
  results[3] = REMOTE_DRAM;
  results[4] = L2_MISS;
  results[5] = UOPS_RETIRED;
  results[6] = UNHALTED_CORE_CYCLES;
  results[7] = REMOTE_FWD;
  results[8] = REMOTE_HITM;
  results[9] = INSTRUCTIONS_RETIRED;
  
  return results;
}


using namespace std;

#define SLEEPING_TIME_IN_MICROSECONDS 5000
#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10

bool PERF_COUNTERS_ENABLED;
std::map< pid_t, std::vector<int> > cores_per_pid;
// TODO remove

typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;
typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
} communication_slot;

typedef struct {
  void (*init)(pin_data** pin, mctop_t* topo);
  sem_t* (*get_lock)();
  
  void (*new_process)(pid_t pid, int policy);
  void (*process_exit)(pid_t pid);

  int (*get_hwc)(pid_t pid, pid_t tid, int* node);
  void (*release_hwc)(pid_t pid);

  std::vector<int> (*reschedule)(pid_t pid, int new_policy);
} heuristic_t;
heuristic_t h;

heuristic_t new_heuristic(void (*init)(pin_data**, mctop_t*), sem_t* (*get_lock)(), void (*new_process)(pid_t, int), void (*process_exit)(pid_t),
			  int (*get_hwc)(pid_t, pid_t, int*), void (*release_hwc)(pid_t), std::vector<int> (*reschedule)(pid_t, int)) {
  heuristic_t tmp;
  tmp.init = init;
  tmp.get_lock = get_lock;
  tmp.new_process = new_process;
  tmp.process_exit = process_exit;
  tmp.get_hwc = get_hwc;
  tmp.release_hwc = release_hwc;
  tmp.reschedule = reschedule;
  return tmp;
}

void copy_heuristic(heuristic_t* to, heuristic_t from) {
  to->init = from.init;
  to->get_lock = from.get_lock;
  to->new_process = from.new_process;
  to->process_exit = from.process_exit;
  to->get_hwc = from.get_hwc;
  to->release_hwc = from.release_hwc;
  to->reschedule = from.reschedule;
  printf("After copy and to init is %p and %p\n", to->init, from.init);
}



void initialize_lock(int i, communication_slot* slots)
{
  ticketlock_t* locks = init_ticketlocks(1);
  slots[i].lock = locks[0];
}

void acquire_lock(int i, communication_slot* slots)
{
  ticketlock_t *lock = &(slots[i].lock);
  ticket_acquire(lock);
}

void release_lock(int i, communication_slot* slots)
{
  ticketlock_t *lock = &(slots[i].lock);
  ticket_release(lock);
}

std::map< pid_t, std::vector<pid_t> > tids_per_pid;

volatile pid_t* hwcs_used_by_pid; 

int get_hwc_and_schedule(heuristic_t h, pid_t pid, pid_t tid, bool schedule, int* node);
void release_hwc(heuristic_t h, pid_t pid);

typedef struct {
  communication_slot* slots;
  pin_data** pin;
} check_slots_args;

void* check_slots(void* dt) {
  check_slots_args* args = (check_slots_args *) dt;
  communication_slot* slots = args->slots;

  int sleeping = 20;
  while (true) {
    usleep(sleeping * 1000);
    int j = rand() % NUM_SLOTS;
    bool first_time = true;
    for (int i = j; first_time || i != j; ) {

      acquire_lock(i, slots);
      communication_slot* slot = slots + i;

      if (slot->used == START_PTHREADS) {
	pid_t pid = slot->pid;
	pid_t* pt_pid = (pid_t*) malloc(sizeof(pid_t));
	*pt_pid = slot->pid;

	pid_t* pt_tid = (pid_t*) malloc(sizeof(pid_t));
	*pt_tid = slot->tid;

	int node = -1;
	int core = get_hwc_and_schedule(h, pid, *pt_tid, false, &node);
	slot->node = node;
	slot->core = core;

	printf("The core is: %d and the node as well: %d\n", core, node);

	int* pt_core = (pid_t*) malloc(sizeof(int));
	*pt_core = core;

	std::map< pid_t, std::vector<pid_t> >::iterator it = tids_per_pid.find(pid);
	if (it == tids_per_pid.end()) {
	  tids_per_pid.insert(std::pair<pid_t, std::vector<pid_t> >(pid, std::vector<pid_t>()));
	}
	it = tids_per_pid.find(pid);
	it->second.push_back(*pt_tid);
	
	slot->used = SCHEDULER;
      }
      else if (slot->used == END_PTHREADS) {
	fprintf(stderr, "About to release: %d\n", slot->tid);
	release_hwc(h, slot->tid);
	slot->used = NONE;
      }
      
      release_lock(i, slots);
      first_time = false;
      i = (i + 1) % NUM_SLOTS;
    }
  }
}

typedef struct {
  int num_id; // the numerical value in front of every file
  pid_t* pid;
  struct timespec *start;
  char* policy;
  char* program;
} process;

FILE* results_fp;

communication_slot* initialize_slots() {
  int fd = open(SLOTS_FILE_NAME, O_CREAT | O_RDWR, 0777);
  if (fd == -1) {
    fprintf(stderr, "Couldnt' open file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    exit(-1);
  }

  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "Couldn't ftruncate file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    exit(-1);
  }
  communication_slot *slots = (communication_slot*) mmap(NULL, sizeof(communication_slot) * NUM_SLOTS, 
				  PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (slots == MAP_FAILED) {
    fprintf(stderr, "main mmap failed for file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    exit(-1);
  }

  for (int j = 0; j < NUM_SLOTS; ++j) {
    initialize_lock(j, slots);
    
    communication_slot* slot = &slots[j];
    slot->node = -1;
    slot->core = -1;
    slot->tid = 0;
    slot->pid = 0;
    slot->used = NONE;
  }

  return slots;
}

// returns chosen node as well
int get_hwc_and_schedule(heuristic_t h, pid_t pid, pid_t tid, bool schedule, int* node) {

  printf("\n Get hwc wtih tid: %lld\n", (long) tid);
  sem_wait(h.get_lock());

  int hwc = h.get_hwc(pid, tid, node);
  printf("Got ... HWC: %d\n", hwc);

  
  std::map< pid_t, std::vector<int> >::iterator it = cores_per_pid.find(pid);
  if (it == cores_per_pid.end()) {
    cores_per_pid.insert(std::pair<pid_t, std::vector<int> >(pid, std::vector<int>()));

  }
  it = cores_per_pid.find(pid);
  it->second.push_back(hwc);

  if (schedule) {
    if (hwc == -1) {
      fprintf(stderr, "Trying to schedule on hwc of -1. Policy is probably MCTOP_ALLOC_NONE\n");
      exit(-1);
    }
    
    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(hwc, &st);
    
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &st)) {
	perror("sched_setaffinitpy!\n");
     }
    // TODO ... we don't use mempolicy anymore
    //unsigned long num = 1 << pin[pol][cnt].node;
    //if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
    //perror("mempolicy didn't work\n");
  }

  sem_post(h.get_lock());
  return hwc;
}

std::vector<int> reschedule(heuristic_t h, pid_t pid, int new_policy) {
  printf("\n\n\n-----------\n\nreschedule()\n\n---------");
  for (std::vector<int>::iterator it = cores_per_pid[pid].begin() ; it != cores_per_pid[pid].end(); ++it) {
    printf("%d\n", *it);
  }

  cores_per_pid.clear();
  std::vector<int> tmp = h.reschedule(pid, new_policy);
  for (std::vector<int>::iterator it = tmp.begin() ; it != tmp.end(); ++it) {
    cores_per_pid[pid].push_back(*it);
  }
  printf("---AFTER ---\n");
  for (std::vector<int>::iterator it = cores_per_pid[pid].begin() ; it != cores_per_pid[pid].end(); ++it) {
    printf("%d\n", *it);
  }

  printf("-----\n");
  return tmp;
}

void release_hwc(heuristic_t h, pid_t pid) {
  h.release_hwc(pid);
}


// we need first to sleep (based on start_time_ms) and then start the timing of the process
// so, we cannot just fork in the main function
typedef struct {
  process* p;
  char** program;
  int start_time_ms;
  int policy;

  FILE* memory_bandwidth_file;
} execute_process_args;

double divide(long long a, long long b) {
  if (b == 0) {
    return 0; // so we don't get NaN
  }
  return a / ((double) b);
}

// including the threads of the application
long long read_all_context_switches(bool isVoluntary, pid_t pid) {
  long long context_switches = 0;

  std::vector<pid_t>::iterator it;
  printf("The number of elements is :%d\n", tids_per_pid.size());
  printf("The number of elements is for !!! :%d\n", tids_per_pid.find(pid)->second.size());

  std::vector<pid_t> v = tids_per_pid.find(pid)->second;
  
  for (it = v.begin(); it != v.end(); it++) {
    context_switches += read_context_switches(isVoluntary, *it);
  }
  context_switches += read_context_switches(isVoluntary, pid);


  return context_switches;
}
  

// long long* read_and_print_perf_counters(pid_t pid, long long old_values[], bool print_to_cerr, long long new_values[], int number_of_threads) {
//   long long* results = (long long *) malloc(sizeof(long long) * 10);
//   long long LLC_TCA = 0;
//   long long LLC_TCM = 0;
//   long long LOCAL_DRAM = 0;
//   long long REMOTE_DRAM = 0;
//   long long ALL_LOADS = 0;
//   long long UOPS_RETIRED = 0;
//   long long inter_socket_activity1 = 0; // MEM_LOAD_UOPS_LLC_MISS_..._REMOTE_FWD
//   long long inter_socket_activity2 = 0; // MEM_LOAD_UOPS_LLC_MISS_..._REMOTE_HITM
//   long long context_switches = 0;
//   long long unhalted_cycles = 0;

  
//   std::vector<int>::iterator it;
//   std::vector<int> v = cores_per_pid.find(pid)->second;
//   for (it = v.begin(); it != v.end(); it++) {
//     int cpu = *it;
//     core_data cd = get_counters(cpu);
//     LLC_TCA += cd.values[0];
//     LLC_TCM += cd.values[1];
//     LOCAL_DRAM += cd.values[2];
//     REMOTE_DRAM += cd.values[3];
//     ALL_LOADS += cd.values[4];
//     UOPS_RETIRED += cd.values[5];
//     unhalted_cycles += cd.values[6];
//     inter_socket_activity1 += cd.values[7];
//     inter_socket_activity2 += cd.values[8];
//   }
//   context_switches = read_all_context_switches(true, pid);

//   results[0] = LLC_TCA;
//   results[1] = LLC_TCM;
//   results[2] = LOCAL_DRAM;
//   results[3] = REMOTE_DRAM;
//   results[4] = ALL_LOADS;
//   results[5] = UOPS_RETIRED;
//   results[6] = unhalted_cycles;
//   results[7] = inter_socket_activity1;
//   results[8] = inter_socket_activity2;
//   results[9] = context_switches;


//   LLC_TCA -= old_values[0];
//   LLC_TCM -= old_values[1];
//   LOCAL_DRAM -= old_values[2];
//   REMOTE_DRAM -= old_values[3];
//   ALL_LOADS -= old_values[4];
//   UOPS_RETIRED -= old_values[5];
//   unhalted_cycles -= old_values[6];
//   inter_socket_activity1 -= old_values[7];
//   inter_socket_activity2 -= old_values[8];
//   context_switches -= old_values[9];


//   // std::string features[10] = {"retired_uops", "loads_retired_ups", "LLC_misses", "LLC_references", 
//   // 			"local_accesses", "remote_accesses", "inter_socket1", "inter_socket2", "context_switches", "number_of_threads"};
  

//   new_values[0] = UOPS_RETIRED;
//   new_values[1] = ALL_LOADS;
//   new_values[2] = LLC_TCM;
//   new_values[3] = LLC_TCA;
//   new_values[4] = LOCAL_DRAM;
//   new_values[5] = REMOTE_DRAM;
//   new_values[6] = inter_socket_activity1;
//   new_values[7] = inter_socket_activity2;
//   new_values[8] = context_switches;
//   new_values[9] = number_of_threads;

//   return results;
// }


double clean(double x) {
  if (x > 1.0) {
    return 1.0;
  }
  else if (x < 0.0) {
    return 0.0;
  }
  return x;
}

typedef struct {
  pid_t pid;
  FILE* memory_bandwidth_file;
} slate_bg_dt;

volatile int times = 0, RRtimes = 0;
void* slate_bg(void* dt) {
  slate_bg_dt* slate_dt = (slate_bg_dt *) dt;
  
  pid_t pid = slate_dt->pid;
  FILE* fp = slate_dt->memory_bandwidth_file;

  long long prev_context_switches = 0;
  long long *prev_counters, *cur_counters;
  prev_counters = (long long *) malloc(sizeof(long long) * COUNTERS_NUMBER);
  bzero(prev_counters, sizeof(long long) * COUNTERS_NUMBER);

  int number_of_threads;
  int* tids = get_thread_ids(pid, &number_of_threads);

  std::vector<int> hwcs;

  std::map< pid_t, std::vector<int> >::iterator it = cores_per_pid.find(pid);
  it = cores_per_pid.find(pid);

  std::vector<int> cores_used = it->second;
  for (std::vector<int>::iterator it2 = cores_used.begin(); it2 != cores_used.end(); ++it2) {
    hwcs.push_back(*it2);
  }


  int policy_before = initial_policy;
  int status;
  while (true) {

    // re-check if new threads were spawned by the process
    tids = get_thread_ids(pid, &number_of_threads);

    hwcs.clear();

    std::map< pid_t, std::vector<int> >::iterator it = cores_per_pid.find(pid);
    it = cores_per_pid.find(pid);

    cores_used = it->second;
    for (std::vector<int>::iterator it2 = cores_used.begin(); it2 != cores_used.end(); ++it2) {
      hwcs.push_back(*it2);
    }
    //


    cur_counters = read_perf_counters(hwcs);
    sleep(1);

    pid_t w = waitpid(pid, &status, WNOHANG);
    if (w == pid) {
      cerr << "What's this?" << endl;
      break;
    }
    
    double sockets_bw[4];
    for (int i = 0; i < 4; ++i) {
      sockets_bw[i] = read_memory_bandwidth(i, fp);
      rewind(fp);
    }

    long cur_context_switches = 0;
    for (int i = 0; i < number_of_threads; ++i) {
      long long result = read_context_switches(true, tids[i]);
      if (result == -1) {
	printf("cannot read any more context switches. probably files are missing!");
	int status;
	while (waitpid(pid, &status, WNOHANG) != 0) {
	  usleep(100);
	  return 0;
	}
	
      }
      cur_context_switches += result;
    }


    std::cout << "Used @ hwcs: " << std::endl;
    for (int i = 0; i < hwcs.size(); ++i) {
      std::cout << hwcs[i] << " ";
    }
    std::cout << endl;
    
    prev_counters = read_perf_counters(hwcs);

    long long* actual_values = subtract(prev_counters, cur_counters, COUNTERS_NUMBER);

    int final_result = -1;
    print_perf_counters(policy_before == 1, final_result, actual_values, cur_context_switches - prev_context_switches, sockets_bw, number_of_threads); // FIXME +-1 for the thread

    w = waitpid(pid, &status, WNOHANG);
    if (w == pid) {
      cerr << "What's this?" << endl;
      break;
    }
    else {
    }


    int loc = policy_before == 0? 1: 0;
    int rr = 1 - loc;

    classifier_data input =  create_data(loc, rr, actual_values[0], actual_values[1], actual_values[2], actual_values[3],
					 actual_values[4], actual_values[5], actual_values[6], actual_values[7], actual_values[8],
					 actual_values[9], 
					 cur_context_switches - prev_context_switches,
					 sockets_bw[0], sockets_bw[1], sockets_bw[2], sockets_bw[3], number_of_threads);


    free(actual_values);
    prev_context_switches = cur_context_switches;
    
    int classification_result = classify(classify_data, input);
    if (policy_before == 1 && classification_result == 0) {
      int new_policy = MCTOP_ALLOC_MIN_LAT_CORES; // FIXME
      policy_before = 0;
      std::vector<int> new_cores = reschedule(h, pid, new_policy);
      hwcs.clear();
      for (int i = 0; i < new_cores.size(); ++i) {
	hwcs.push_back(new_cores[i]);
      }
      
      fprintf(stderr, "Moved to LOC\n");
    }
    else if (policy_before == 0 && classification_result == 1) {
      int new_policy = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
      policy_before = 1;
      hwcs.clear();
      std::vector<int> new_cores = reschedule(h, pid, new_policy);
      for (int i = 0; i < new_cores.size(); ++i) {
	hwcs.push_back(new_cores[i]);
      }

      fprintf(stderr, "Moved to RR\n");
    }
    else if (classification_result == 2 && policy_before == 0) {
      int new_policy = MCTOP_ALLOC_MIN_LAT_CORES;
      policy_before = 0;
      hwcs.clear();
      std::vector<int> new_cores = reschedule(h, pid, new_policy);
      for (int i = 0; i < new_cores.size(); ++i) {
	hwcs.push_back(new_cores[i]);
      }

      fprintf(stderr, "Moved to RR\n");
    }
    else {
      // stay with current policy
    }
      
    
    times++;
  }

}

void* execute_process(void* dt) {
  execute_process_args* args = (execute_process_args*) dt;
  usleep(args->start_time_ms * 1000);

  char**program = args->program;
  clock_gettime(CLOCK_MONOTONIC, (args->p)->start);

  pid_t pid = fork();

  if (pid == 0) {
    //char* envp[1] = {NULL};

    printf("-----------------------------------\n");
    int i = 0;
    while (program[i] != NULL) {
      printf("(%s)\n", program[i]);
      i++;
    }
    printf("-----------------------------------\n");

    execv(program[0], program);
    printf("== %s\n", program[0]);
    perror("execve didn't work");
    exit(1);
  }

  pid_t* pid_pt = (pid_t *)malloc(sizeof(pid_t));
  *pid_pt = pid;
  
  int pol = args->policy;
  int* pt_pol = (int *) malloc(sizeof(int));
  *pt_pol = pol;

  bool was_slate = false;
  if (pol == MCTOP_ALLOC_SLATE) {

    if (initial_policy == 0) {
      pol = MCTOP_ALLOC_MIN_LAT_CORES;
    }
    else if (initial_policy == 1) {
      pol = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
    }
    else {
      fprintf(stderr, "NOT supported yet. What kind of initial policy is this?\n");
    }
    
    was_slate = true;
  }

  h.new_process(pid, pol);

  int core, node;
  core = node = -1;
  if (pol != MCTOP_ALLOC_NONE) {
    core = get_hwc_and_schedule(h, pid, pid, true, &node);
  }
  else {
    ;
  }

  int *pt_core = (int*) malloc(sizeof(int));
  *pt_core = core;

  process*p = args->p;
  pthread_t* pt = (pthread_t *) malloc(sizeof(pthread_t));
  p->pid = pid_pt;

  if (was_slate) {
    slate_bg_dt* dt = malloc(sizeof(slate_bg_dt));
    dt->pid = pid;
    dt->memory_bandwidth_file = args->memory_bandwidth_file;
    
    sleep(1); // threads might take some time to be created ... canneal needs 3 seconds, 2 for blackscholes, for dedup, 60 for raytrace, 4 for wrmem
    pthread_t slate_th;
    pthread_create(&slate_th, NULL, slate_bg, dt);
  }
  
  printf("Added %lld to list with processes\n", (long long) (*pid_pt));
  {
    struct timespec *start = p->start;
    pid_t* pid = p->pid;
    int status;

    printf("Waiting for the process to terminate ...\n");
    waitpid(*pid, &status, 0);
    printf("The process just terminated. Did it exit? %d %d\n", WIFSIGNALED(status), WTERMSIG(status));

    h.process_exit(*pid);
    release_hwc(h, *pid);

    struct timespec *finish = (struct timespec *) malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_MONOTONIC, finish);

    pid_t* current_id = pid;

    
    double elapsed = (finish->tv_sec - start->tv_sec);
    elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;
    //printf("RETIRED DRAM: %lld\n", retired_remote_dram);
    fprintf(results_fp, "%d\t%ld\t%lf\t%s\t%s\t"			\
	    "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", p->num_id, (long int) *pid, elapsed, p->policy, p->program,
	    //instructions, cycles, (((double) instructions) / cycles),
	    0, 1,
	    ((double) 0 / 2),
	    0, 2, 3 / ((double) 4 + 3)
	    );
	  
  }

  return NULL;
}

int main(int argc, char* argv[]) {


  PERF_COUNTERS_ENABLED = false;
  if (argc == 5) {
    if (strcmp(argv[4], "true") == 0) {
      PERF_COUNTERS_ENABLED = true;
      start_reading();
    }
  }
  else if (argc == 6) {
    initial_policy = atoi(argv[5]);
    if (!(initial_policy == 0 || initial_policy == 1)) {
      perror("Initial policy can onlyl be 0 or 1 ... \n");
      return EXIT_FAILURE;
    }
  }
  
  srand(time(NULL));
  char* heuristic = (char *) malloc(100);
  
  if (argc != 4 && argc != 5 && argc != 6) {
    perror("Call schedule like this: ./schedule input_file output_file_for_results heuristic COUNTERS_OR_NOT? initial_policy\n");
    return EXIT_FAILURE;
  }

  results_fp = fopen(argv[2], "w");
  strcpy(heuristic, argv[3]);
  

  mctop_t* topo = mctop_load(NULL);
  //  mctop_print(topo);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }


  classify_data = init_classifier_data();
  communication_slot* slots = initialize_slots();
  pin_data** pin = initialize_pin_data(topo);

  heuristic_t init_h = new_heuristic(H_init, H_get_lock, H_new_process, H_process_exit, H_get_hwc, H_release_hwc, H_reschedule);
  heuristic_t X_h = new_heuristic(HX_init, HX_get_lock, HX_new_process, HX_process_exit, HX_get_hwc, HX_release_hwc, NULL);
  heuristic_t MCTOP_h = new_heuristic(HMCTOP_init, HMCTOP_get_lock, HMCTOP_new_process, HMCTOP_process_exit, HMCTOP_get_hwc, HMCTOP_release_hwc, NULL);
  heuristic_t GREEDY_h = new_heuristic(HGREEDY_init, HGREEDY_get_lock, HGREEDY_new_process, HGREEDY_process_exit, HGREEDY_get_hwc, HGREEDY_release_hwc, HGREEDY_reschedule);
  heuristic_t SPLIT_h = new_heuristic(HSPLIT_init, HSPLIT_get_lock, HSPLIT_new_process, HSPLIT_process_exit, HSPLIT_get_hwc, HSPLIT_release_hwc, NULL);
  heuristic_t RRLAT_h = new_heuristic(HRRLAT_init, HRRLAT_get_lock, HRRLAT_new_process, HRRLAT_process_exit, HRRLAT_get_hwc, HRRLAT_release_hwc, NULL);

  
  if (strcmp(heuristic, "NORMAL") == 0) {
    copy_heuristic(&h, init_h);
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "X") == 0) {
    copy_heuristic(&h, X_h);
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "MCTOP") == 0) {
    copy_heuristic(&h, MCTOP_h);
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "GREEDY") == 0) {
    copy_heuristic(&h, GREEDY_h);
    printf("%p\n", h.init);
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "SPLIT") == 0) {
    copy_heuristic(&h, SPLIT_h);
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "RRLAT") == 0) {
    copy_heuristic(&h, RRLAT_h);
    h.init(pin, topo);
  }
  else {
    fprintf(stderr, "heuristic can only be \"NORMAL\" or \"X\", or \"GREEDY\", \"SPLIT\", or \"RRLAT\"\n");
    return EXIT_FAILURE;
  } 
   
  // create a thread to check for new slots
  pthread_t check_slots_thread;
  check_slots_args* args = (check_slots_args *) malloc(sizeof(check_slots_args));
  args->slots = slots;
  args->pin = pin;
  pthread_create(&check_slots_thread, NULL, check_slots, args);

  char line[1000];
  FILE* fp = fopen(argv[1], "r");
  pthread_t threads[100];
  int programs = 0;
  while (fgets(line, 1000, fp)) {
    if (line[0] == '#') {
      continue; // the line is commented
    }

    //processes++;

    read_line_output result = read_line(line);
    int num_id = result.num_id;
    int start_time_ms = result.start_time_ms;
    char* policy = result.policy;
    char** program = result.program;
    line[0] = '\0';

    mctop_alloc_policy pol;

    if (strcmp(policy, "MCTOP_ALLOC_SLATE") == 0) {
      pol = (mctop_alloc_policy) MCTOP_ALLOC_SLATE;
    }
    else {
      pol = get_policy(policy);
    }

    process* p = (process* ) malloc(sizeof(process));
    p->pid = (pid_t*) malloc(sizeof(pid_t));
    p->start = (struct timespec * ) malloc(sizeof(struct timespec));
    p->policy = (char *) malloc(sizeof(char) * 100);
    p->policy[0] = '\0';
    strcpy(p->policy, policy);
    p->program = (char *) malloc(sizeof(char) * 400);
    p->program[0] = '\0';
    p->num_id = num_id;

    int z = 0;
    while (program[z] != NULL) {
      strcat(p->program, program[z]);

      if (program[z + 1] != NULL) {
	strcat(p->program, " ");
      }
      z++;
    }

    execute_process_args* args = (execute_process_args * ) malloc(sizeof(execute_process_args));
    args->p = p;
    args->start_time_ms = start_time_ms;
    args->program = program;
    args->policy = pol;
    args->memory_bandwidth_file = start_reading_memory_bandwidth();

    struct timeval tm;
    gettimeofday(&tm, NULL);
    pthread_create(&threads[programs], NULL, execute_process, args);
    programs++;
  }

  int i;
  for (i = 0; i < programs; ++i) {
    void* result;
    pthread_join(threads[i], &result);
  }

  mctop_free(topo);

  close_memory_bandwidth();
  
  return 0;
}

