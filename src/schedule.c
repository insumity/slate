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

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include "ticket.h"
#include "slate_utils.h"
#include "list.h"

#include "heuristic.h"
#include "heuristicX.h"
#include "heuristic_MCTOP.h"
#include "heuristic_greedy.h"
#include "heuristic_split.h"


#define SLEEPING_TIME_IN_MICROSECONDS 5000
#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10


typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;
typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
} communication_slot;

typedef struct {
  void (*init)();
  sem_t* (*get_lock)();
  
  void (*new_process)(pid_t pid, int policy);
  void (*process_exit)(pid_t pid);

  int (*get_hwc)(pid_t pid, pid_t tid, int* node);
  void (*release_hwc)(pid_t pid);
} heuristic_t;
heuristic_t h;

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

list *fd_counters_per_pid, *tids_per_pid;

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
	pid_t* pt_pid = malloc(sizeof(pid_t));
	*pt_pid = slot->pid;

	pid_t* pt_tid = malloc(sizeof(pid_t));
	*pt_tid = slot->tid;

	int node = -1;
	int core = get_hwc_and_schedule(h, pid, *pt_tid, false, &node);
	slot->node = node;
	slot->core = core;

	printf("The core is: %d and the node as well: %d\n", core, node);

	int* pt_core = malloc(sizeof(int));
	*pt_core = core;

	list_add(tids_per_pid, (void *) pt_pid, (void *) pt_tid);

	/* Was slowing things down ...
	   hw_counters_fd* cnt2 = malloc(sizeof(hw_counters_fd));
	   cnt2->instructions = open_perf(*pt_tid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
	   cnt2->cycles = open_perf(*pt_tid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES);
	   cnt2->ll_cache_read_accesses = open_perf(*pt_tid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, ACCESS));
	   cnt2->ll_cache_read_misses = open_perf(*pt_tid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, MISS));

	   list_add(fd_counters_per_pid, (void*) pt_tid, (void*) cnt2);
	
	   start_perf_reading(cnt2->instructions);
	   start_perf_reading(cnt2->cycles);
	   start_perf_reading(cnt2->ll_cache_read_accesses);
	   start_perf_reading(cnt2->ll_cache_read_misses); */
	
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
/*void* wait_for_process_async(void* pro)
{
  process* p = (process *) pro;
  struct timespec *start = p->start;
  pid_t* pid = p->pid;
  int status;


  waitpid(*pid, &status, 0);
  h.process_exit(*pid);
  release_hwc(h, *pid);

  struct timespec *finish = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);

  pid_t* current_id = pid;
  
  void* fd_counters = list_get_value(fd_counters_per_pid, (void *) current_id, compare_pids);
  hw_counters_fd* dt = (hw_counters_fd*) fd_counters;

  long long int instructions = read_perf_counter(dt->instructions);
  long long int cycles = read_perf_counter(dt->cycles);
  long long ll_cache_read_accesses = read_perf_counter(dt->ll_cache_read_accesses);
  long long ll_cache_read_misses = read_perf_counter(dt->ll_cache_read_misses);

 back: ;
  current_id = (pid_t *) list_get_value(tids_per_pid, (void *) current_id, compare_pids);
  if (current_id != NULL) {
    fd_counters = list_get_value(fd_counters_per_pid, (void *) current_id, compare_pids);
    if (fd_counters != NULL) {
      dt = (hw_counters_fd*) fd_counters;
      instructions += read_perf_counter(dt->instructions);
      cycles += read_perf_counter(dt->cycles);

      ll_cache_read_accesses += read_perf_counter(dt->ll_cache_read_accesses);
      ll_cache_read_misses += read_perf_counter(dt->ll_cache_read_misses);
      list_remove(tids_per_pid, (void *) pid, compare_pids);
      current_id = pid;
      goto back;
    }
  }

  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;
  fprintf(results_fp, "%d\t%ld\t%lf\t%s\t%s\t" \
	  "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", p->num_id, (long int) *pid, elapsed, p->policy, p->program,
	  instructions, cycles, (((double) instructions) / cycles), ll_cache_read_accesses, ll_cache_read_misses,
	  1 - ((double) ll_cache_read_misses / (ll_cache_read_misses + ll_cache_read_accesses)));
	  
  return NULL;
}*/

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
  communication_slot *slots = mmap(NULL, sizeof(communication_slot) * NUM_SLOTS, 
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

  if (schedule) {
    if (hwc == -1) {
      fprintf(stderr, "Trying to schedule to wrong process. Policy is probably MCTOP_ALLOC_NONE\n");
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

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;

  int pol = args->policy;
  int* pt_pol = malloc(sizeof(int));
  *pt_pol = pol;

  h.new_process(pid, pol);

  int core, node;
  core = node = -1;
  if (pol != MCTOP_ALLOC_NONE) {
    core = get_hwc_and_schedule(h, pid, pid, true, &node);
  }
  else {
    ;
  }

  int *pt_core = malloc(sizeof(int));
  *pt_core = core;

  process*p = args->p;
  pthread_t* pt = malloc(sizeof(pthread_t));
  p->pid = pid_pt;

  //pthread_create(pt, NULL, wait_for_process_async, p);
  

  hw_counters_fd* cnt2 = malloc(sizeof(hw_counters_fd));
  int leader = cnt2->instructions = open_perf(pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, -1);
  cnt2->cycles = open_perf(pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES, leader);
  cnt2->ll_cache_read_accesses = open_perf(pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, ACCESS), leader);
  cnt2->ll_cache_read_misses = open_perf(pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, MISS), leader);

  list_add(fd_counters_per_pid, (void*) pid_pt, (void*) cnt2);
  start_perf_reading(cnt2->instructions);
  start_perf_reading(cnt2->cycles);
  start_perf_reading(cnt2->ll_cache_read_accesses);
  start_perf_reading(cnt2->ll_cache_read_misses);

  printf("Added %lld to list with processes\n", (long long) (*pid_pt));

  {
    struct timespec *start = p->start;
    pid_t* pid = p->pid;
    int status;

    printf("BLOCK .. waiting for process to FINISH\n");
    waitpid(*pid, &status, 0);
    printf(" A process just finished!!!!!\n");


    h.process_exit(*pid);
    release_hwc(h, *pid);

    struct timespec *finish = malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_MONOTONIC, finish);

    pid_t* current_id = pid;
  
    void* fd_counters = list_get_value(fd_counters_per_pid, (void *) current_id, compare_pids);
    hw_counters_fd* dt = (hw_counters_fd*) fd_counters;

    long long int instructions = read_perf_counter(dt->instructions);
    long long int cycles = read_perf_counter(dt->cycles);
    long long ll_cache_read_accesses = read_perf_counter(dt->ll_cache_read_accesses);
    long long ll_cache_read_misses = read_perf_counter(dt->ll_cache_read_misses);

    /*
      back: ;
      current_id = (pid_t *) list_get_value(tids_per_pid, (void *) current_id, compare_pids);
      if (current_id != NULL) {
      fd_counters = list_get_value(fd_counters_per_pid, (void *) current_id, compare_pids);
      if (fd_counters != NULL) {
      dt = (hw_counters_fd*) fd_counters;
      instructions += read_perf_counter(dt->instructions);
      cycles += read_perf_counter(dt->cycles);

      ll_cache_read_accesses += read_perf_counter(dt->ll_cache_read_accesses);
      ll_cache_read_misses += read_perf_counter(dt->ll_cache_read_misses);
      list_remove(tids_per_pid, (void *) pid, compare_pids);
      current_id = pid;
      goto back;
      }
      }
    */

    double elapsed = (finish->tv_sec - start->tv_sec);
    elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;
    fprintf(results_fp, "%d\t%ld\t%lf\t%s\t%s\t" \
	    "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", p->num_id, (long int) *pid, elapsed, p->policy, p->program,
	    instructions, cycles, (((double) instructions) / cycles), ll_cache_read_accesses, ll_cache_read_misses,
	    1 - ((double) ll_cache_read_misses / (ll_cache_read_misses + ll_cache_read_accesses)));
	  
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  srand(time(NULL));
  char* heuristic = malloc(100);
  
  if (argc != 4) {
    perror("Call schedule like this: ./schedule input_file output_file_for_results heuristic\n");
    return EXIT_FAILURE;
  }

  results_fp = fopen(argv[2], "w");
  strcpy(heuristic, argv[3]);
  
  fd_counters_per_pid = create_list();
  tids_per_pid = create_list();

  mctop_t* topo = mctop_load(NULL);
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
  else {
    fprintf(stderr, "heuristic can only be \"NORMAL\" or \"X\"\n");
    return EXIT_FAILURE;
  } 
   
  // create a thread to check for new slots
  pthread_t check_slots_thread;
  check_slots_args* args = malloc(sizeof(check_slots_args));
  args->slots = slots;
  args->pin = pin;
  pthread_create(&check_slots_thread, NULL, check_slots, args);

  char line[300];
  FILE* fp = fopen(argv[1], "r");
  pthread_t threads[100];
  int programs = 0;
  while (fgets(line, 300, fp)) {
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

    mctop_alloc_policy pol = get_policy(policy);

    process* p = malloc(sizeof(process));
    p->pid = malloc(sizeof(pid_t));
    p->start = malloc(sizeof(struct timespec));
    p->policy = malloc(sizeof(char) * 100);
    p->policy[0] = '\0';
    strcpy(p->policy, policy);
    p->program = malloc(sizeof(char) * 300);
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

    execute_process_args* args = malloc(sizeof(execute_process_args));
    args->p = p;
    args->start_time_ms = start_time_ms;
    args->program = program;
    args->policy = pol;

    struct timeval tm;
    gettimeofday(&tm, NULL);
    printf("time: %lu:%lu\n", tm.tv_sec, tm.tv_usec);
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

