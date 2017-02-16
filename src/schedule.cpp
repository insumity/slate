#define _GNU_SOURCE
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
#include <numaif.h>
#include <linux/hw_breakpoint.h>

#include "ticket.h"
#include "slate_utils.h"
#include "list.h"
#include "read_counters.h"

#include "heuristic.h"
#include "heuristicX.h"
#include "heuristic_MCTOP.h"
#include "heuristic_greedy.h"
#include "heuristic_split.h"
#include "heuristic_rr_lat.h"

#include <map>
#include <vector>

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

  void (*reschedule)(pid_t pid, int new_policy);
} heuristic_t;
heuristic_t h;

double fo(long long ll_cache_read_accesses, long long ll_cache_read_misses,
	  long long retired_local_dram, long long retired_remote_dram) {
  double LLC = ll_cache_read_misses / ((double) ll_cache_read_accesses);
  double local = retired_local_dram / (retired_local_dram + (double) retired_remote_dram);

  return LLC * local * 2.2 + LLC * (1.0 - local) * 6.0;
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

list *tids_per_pid;

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

	list_add(tids_per_pid, (void *) pt_pid, (void *) pt_tid);
	
	slot->used = SCHEDULER;
      }
      else if (slot->used == END_PTHREADS) {
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
  sem_wait(h.get_lock());

  int hwc = h.get_hwc(pid, tid, node);

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
} execute_process_args;


void* execute_process(void* dt) {
  execute_process_args* args = (execute_process_args*) dt;
  
  usleep(args->start_time_ms * 1000);

  char**program = args->program;
  clock_gettime(CLOCK_MONOTONIC, (args->p)->start);

  pid_t pid = fork();

  if (pid == 0) {
    //char* envp[1] = {NULL};
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
    pol = MCTOP_ALLOC_MIN_LAT_HWCS;
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
    usleep(2000 * 1000);

    long long LLC_TCA = 0;
    long long LLC_TCM = 0;
    long long LOCAL_DRAM = 0;
    long long REMOTE_DRAM = 0;

    std::vector<int>::iterator it;
    std::vector<int> v = cores_per_pid.find(pid)->second;
    for (it = v.begin(); it != v.end(); it++) {
      int cpu = *it;
      printf("MPIKA\n\n\n edo me cpu\n\n: cpu: %d\n", cpu);
      core_data cd = get_counters(cpu);
      LLC_TCA += cd.values[0];
      LLC_TCM += cd.values[1];
      LOCAL_DRAM += cd.values[2];
      REMOTE_DRAM += cd.values[3];
      printf("%lld, %lld, %lld, %lld\n", LLC_TCA, LLC_TCM, LOCAL_DRAM, REMOTE_DRAM);
    }

    usleep(1200 * 1000); // wait for 1.2s

  
    long long LLC_TCA2 = 0;
    long long LLC_TCM2 = 0;
    long long LOCAL_DRAM2 = 0;
    long long REMOTE_DRAM2 = 0;
    
    for (it = v.begin(); it != v.end(); it++) {
      int cpu = *it;

      core_data cd = get_counters(cpu);
      LLC_TCA2 += cd.values[0];
      LLC_TCM2 += cd.values[1];
      LOCAL_DRAM2 += cd.values[2];
      REMOTE_DRAM2 += cd.values[3];
      printf("%lld, %lld, %lld, %lld\n", LLC_TCA2, LLC_TCM2, LOCAL_DRAM2, REMOTE_DRAM2);
    }

    printf("====\n\n%lld, %lld, %lld, %lld\n", LLC_TCA, LLC_TCM, LOCAL_DRAM, REMOTE_DRAM);

    LLC_TCA2 -= LLC_TCA;
    LLC_TCM2 -= LLC_TCM;
    LOCAL_DRAM2 -= LOCAL_DRAM;
    REMOTE_DRAM2 -= REMOTE_DRAM;
    printf("====\n\n%lld, %lld, %lld, %lld\n", LLC_TCA2, LLC_TCM2, LOCAL_DRAM2, REMOTE_DRAM2);


    double res1 = fo(LLC_TCA2, LLC_TCM2, LOCAL_DRAM2, REMOTE_DRAM2);

    // change policy
    int new_policy = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
    h.reschedule(pid, new_policy);
    printf("I just rescheduled to ROUND_ROBIN_CORES!!!\n");


    LLC_TCA = 0;
    LLC_TCM = 0;
    LOCAL_DRAM = 0;
    REMOTE_DRAM = 0;
    for (it = v.begin(); it != v.end(); it++) {
      int cpu = *it;
      core_data cd = get_counters(cpu);
      LLC_TCA += cd.values[0];
      LLC_TCM += cd.values[1];
      LOCAL_DRAM += cd.values[2];
      REMOTE_DRAM += cd.values[3];
    }
    usleep(1200 * 1000); // wait for 500ms

  
    LLC_TCA2 = 0;
    LLC_TCM2 = 0;
    LOCAL_DRAM2 = 0;
    REMOTE_DRAM2 = 0;
    for (it = v.begin(); it != v.end(); it++) {
      int cpu = *it;
      core_data cd = get_counters(cpu);
      LLC_TCA2 += cd.values[0];
      LLC_TCM2 += cd.values[1];
      LOCAL_DRAM2 += cd.values[2];
      REMOTE_DRAM2 += cd.values[3];
    }
  
    LLC_TCA2 -= LLC_TCA;
    LLC_TCM2 -= LLC_TCM;
    LOCAL_DRAM2 -= LOCAL_DRAM;
    REMOTE_DRAM2 -= REMOTE_DRAM;

    double res2 = fo(LLC_TCA2, LLC_TCM2, LOCAL_DRAM2, REMOTE_DRAM2);

    printf("RES1 is: %lf and RES2: %lf\n", res1, res2);
    if (res2 <= res1) {
      // good!
    }
    else {
      printf("\n\n\nMPIKA EDO mori ametaoniti kai ekana ksana reschedule gia MCTOP_ALLOC_BWRR_!!!\n\n\n");
      h.reschedule(pid, MCTOP_ALLOC_BW_ROUND_ROBIN_CORES);
      // got back to RR CORE
    }
  }
  
  printf("Added %lld to list with processes\n", (long long) (*pid_pt));
  {
    struct timespec *start = p->start;
    pid_t* pid = p->pid;
    int status;

    printf("BLOCK .. waiting for process to FINIShH\n");
    waitpid(*pid, &status, 0);
    printf(" A process just finished!!!!!\n");

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
  
  srand(time(NULL));
  char* heuristic = (char *) malloc(100);
  
  if (argc != 4 && argc != 5) {
    perror("Call schedule like this: ./schedule input_file output_file_for_results heuristic\n");
    return EXIT_FAILURE;
  }


  results_fp = fopen(argv[2], "w");
  strcpy(heuristic, argv[3]);
  
  tids_per_pid = create_list();

  mctop_t* topo = mctop_load(NULL);
  //  mctop_print(topo);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }
  
  communication_slot* slots = initialize_slots();
  pin_data** pin = initialize_pin_data(topo);
  if (strcmp(heuristic, "NORMAL") == 0) {
    h.init = H_init;
    h.get_lock = H_get_lock;
    h.new_process = H_new_process;
    h.process_exit = H_process_exit;
    h.get_hwc = H_get_hwc;
    h.release_hwc = H_release_hwc;
    h.reschedule = H_reschedule; // FIXME put this in the other ones
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "X") == 0) {
    h.init = HX_init;
    h.get_lock = HX_get_lock;
    h.new_process = HX_new_process;
    h.process_exit = HX_process_exit;
    h.get_hwc = HX_get_hwc;
    h.release_hwc = HX_release_hwc;
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "MCTOP") == 0) {
    h.init = HMCTOP_init;
    h.get_lock = HMCTOP_get_lock;
    h.new_process = HMCTOP_new_process;
    h.process_exit = HMCTOP_process_exit;
    h.get_hwc = HMCTOP_get_hwc;
    h.release_hwc = HMCTOP_release_hwc;
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "GREEDY") == 0) {
    h.init = HGREEDY_init;
    h.get_lock = HGREEDY_get_lock;
    h.new_process = HGREEDY_new_process;
    h.process_exit = HGREEDY_process_exit;
    h.get_hwc = HGREEDY_get_hwc;
    h.release_hwc = HGREEDY_release_hwc;
    h.reschedule = HGREEDY_reschedule; // FIXME put this in the other ones
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "SPLIT") == 0) {
    h.init = HSPLIT_init;
    h.get_lock = HSPLIT_get_lock;
    h.new_process = HSPLIT_new_process;
    h.process_exit = HSPLIT_process_exit;
    h.get_hwc = HSPLIT_get_hwc;
    h.release_hwc = HSPLIT_release_hwc;
    h.init(pin, topo);
  }
  else if (strcmp(heuristic, "RRLAT") == 0) {
    h.init = HRRLAT_init;
    h.get_lock = HRRLAT_get_lock;
    h.new_process = HRRLAT_new_process;
    h.process_exit = HRRLAT_process_exit;
    h.get_hwc = HRRLAT_get_hwc;
    h.release_hwc = HRRLAT_release_hwc;
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

    process* p = (process* )malloc(sizeof(process));
    p->pid = (pid_t*)malloc(sizeof(pid_t));
    p->start = (struct timespec * )malloc(sizeof(struct timespec));
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

    execute_process_args* args = (execute_process_args * )malloc(sizeof(execute_process_args));
    args->p = p;
    args->start_time_ms = start_time_ms;
    args->program = program;
    args->policy = pol;

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
  
  return 0;
}

