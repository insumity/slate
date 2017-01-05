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

#define SLEEPING_TIME_IN_MICROSECONDS 5000

#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10

typedef struct {
  void (*init)();
  void (*new_process)(pid_t pid, int policy);
  int (*get_hwc)(pid_t pid, int* node);
  void (*release_hwc)(int hwc, pid_t pid);
} heuristic_t;
heuristic_t h;


char* heuristic;
mctop_t* topo;

typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;

typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
  int policy;
} communication_slot;

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

// hwcs_per_id stores (pid, hwcs) meaning thread or process with pid is using hwcs
list* policy_per_process, *hwcs_per_pid, *fd_counters_per_pid, *tids_per_pid;


volatile bool* used_hwcs;
list* used_cores; // contains (core, true) if one of the hwcs of the core is used
list* used_sockets; // contains (socket, true) if one of the hwcs of one core of the socket is used
list* running_pids;
volatile pid_t* hwcs_used_by_pid; 


int get_hwc_and_schedule(heuristic_t h, pid_t pid, bool schedule, int* node);
void release_hwc(heuristic_t h, pid_t pid, int hwc);

size_t total_hwcs;
typedef struct {
  communication_slot* slots;
  pin_data** pin;
} check_slots_args;

void* check_slots(void* dt) {
  check_slots_args* args = (check_slots_args *) dt;
  communication_slot* slots = args->slots;
  pin_data** pin = args->pin;

  while (true) {
    usleep(SLEEPING_TIME_IN_MICROSECONDS);
    for (int i = 0; i < NUM_SLOTS; ++i) {
      acquire_lock(i, slots);
      communication_slot* slot = slots + i;

      if (slot->used == START_PTHREADS) {
	printf(" a thread just started\n");
	pid_t pid = slot->pid;
	pid_t* pt_pid = malloc(sizeof(pid_t));
	*pt_pid = pid;

	int times = 0;
      back: ;
	void* policy = list_get_value(policy_per_process, (void *) pt_pid, compare_pids);
      if (policy == NULL) {
	  usleep(5000); // 5ms
	  times++;
	  if (times == 5) {
	    fprintf(stderr, "Couldn't find policy of specific process with pid: %ld\n", *(long int*) (pt_pid));
	    exit(-1);
	  }
	  goto back;
	  }

	  int pol = *((int *) policy);

	/* int cnt = 0; */
	/* pin_data pd = pin[pol][cnt]; */
	/* int core = pd.core; */
	/* int node = pd.node; */
	/* cnt++; */
	/* while (used_hwcs[core]) { */
	/*   if (cnt == total_hwcs) { */
	/*     perror("We ran out of cores\n"); */
	/*     return NULL; */
	/*   } */

	/*   pd = pin[pol][cnt]; */
	/*   core = pd.core; */
	/*   node = pd.node; */
	/*   cnt++; */
	/* } */

	int node;
	int core = get_hwc_and_schedule(h, pid, false, &node);

	//int core = get_hwc(pin, pol, pid, &node, topo, total_hwcs);

	slot->node = node;
	slot->core = core;
	slot->policy = pol;

	pid_t* pt_tid = malloc(sizeof(pid_t));
	*pt_tid = slot->tid;

	int* pt_core = malloc(sizeof(int));
	*pt_core = core;

	list_add(tids_per_pid, (void *) pt_pid, (void *) pt_tid);
	list_add(hwcs_per_pid, (void *) pt_tid, (void *) pt_core);

	hw_counters_fd* cnt2 = malloc(sizeof(hw_counters_fd));
	//cnt2->ll_cache_write_accesses = open_perf(*pt_tid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, WRITE, ACCESS));
	cnt2->instructions = open_perf(*pt_tid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
	cnt2->cycles = open_perf(*pt_tid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES);
	cnt2->ll_cache_read_accesses = open_perf(*pt_tid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, ACCESS));
	cnt2->ll_cache_read_misses = open_perf(*pt_tid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, MISS));

	printf("about to add counters to the list.\n");
	list_add(fd_counters_per_pid, (void*) pt_tid, (void*) cnt2);
	printf("added counters to the list ...\n\n");
	
	start_perf_reading(cnt2->instructions);
	start_perf_reading(cnt2->cycles);

	//start_perf_reading(cnt2->l1i_cache_read_accesses);
	//start_perf_reading(cnt2->l1d_cache_read_accesses);
	start_perf_reading(cnt2->ll_cache_read_accesses);
	//	start_perf_reading(cnt2->l1i_cache_read_misses);
	start_perf_reading(cnt2->ll_cache_read_misses);
	
	slot->used = SCHEDULER;
      }
      else if (slot->used == END_PTHREADS) {
	// find the hwc this thread was using ...
	int* hwc = ((int *) list_get_value(hwcs_per_pid, (void *) &(slot->tid), compare_pids));
	if (hwc == NULL) {
	  fprintf(stderr, "Couldn't find the hwc this process is using: %ld\n", (long int) (slot->tid));
	  exit(-1);
	}

	release_hwc(h, slot->tid, *hwc);	

	slot->used = NONE;
      }

      release_lock(i, slots);
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

//void reschedule_on_exit(pin_data** pin);

FILE* results_fp;
volatile int processes_finished = 0;
void* wait_for_process_async(void* pro)
{
  process* p = (process *) pro;
  struct timespec *start = p->start;
  printf("Wait for process async from process: %p\n", p);
  pid_t* pid = p->pid;
  int status;

  waitpid(*pid, &status, 0);

  int hwc = *((int *) list_get_value(hwcs_per_pid, (void *) pid, compare_pids));

  release_hwc(h, *pid, hwc);

  
  list_remove(running_pids, pid, compare_pids);
  
  // FIXME
  if (strcmp(heuristic, "NONE") != 0) {
    pin_data** pin = initialize_pin_data(topo);

    //reschedule_on_exit(pin);
  }
  

  struct timespec *finish = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);

  pid_t* current_id = pid;
  
  void* fd_counters = list_get_value(fd_counters_per_pid, (void *) current_id, compare_pids);
  hw_counters_fd* dt = (hw_counters_fd*) fd_counters;

  long long int instructions = read_perf_counter(dt->instructions);
  long long int cycles = read_perf_counter(dt->cycles);
  long long ll_cache_read_accesses = read_perf_counter(dt->ll_cache_read_accesses);
  long long ll_cache_read_misses = read_perf_counter(dt->ll_cache_read_misses);

 back:
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

  /*  printf("@#$#@$OLAOLApid: %ld = instructions: %lld ... cycles: %lld, %lf\n", *((pid_t *) pid), instructions, cycles,
	 ((double) instructions) / cycles );
  printf("@#$@#$@#LLC_read_accesses: %lld, LLC_read_misses: %lld, %lf\n", ll_cache_read_accesses, ll_cache_read_misses,
  1 - ((double) ll_cache_read_misses / ll_cache_read_accesses)); */

  
  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;
  fprintf(results_fp, "%d\t%ld\t%lf\t%s\t%s\t" \
	  "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", p->num_id, (long int) *pid, elapsed, p->policy, p->program,
	  instructions, cycles, (((double) instructions) / cycles), ll_cache_read_accesses, ll_cache_read_misses,
	  1 - ((double) ll_cache_read_misses / (ll_cache_read_misses + ll_cache_read_accesses)));
	  
  processes_finished++;
  return NULL;
}

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
    slot->policy = MCTOP_ALLOC_NONE;
  }

  return slots;
}

void print_counters(void* key, void *data)
{
  hw_counters_fd* dt = (hw_counters_fd*) data;
  long long int instructions = read_perf_counter(dt->instructions);
  long long int cycles = read_perf_counter(dt->cycles);

  //long long int cache_accesses = read_perf_counter(dt->cache_accesses);
  //loang long int cache_misses = read_perf_counter(dt->cache_misses);

  if (0) {
    reset_perf_counter(dt->instructions);
    reset_perf_counter(dt->cycles);
    //reset_perf_counter(dt->cache_accesses);
    //reset_perf_counter(dt->cache_misses);
  }

  long long ll_cache_read_accesses = read_perf_counter(dt->ll_cache_read_accesses);
  long long ll_cache_read_misses = read_perf_counter(dt->ll_cache_read_misses);
 
  printf("pid: %ld = instructions: %lld ... cycles: %lld, %lf\n", (long) (*((pid_t *) key)), instructions, cycles,
	 ((double) instructions) / cycles );
  printf("LLC_read_accesses: %lld, LLC_read_misses: %lld, %lf\n", ll_cache_read_accesses, ll_cache_read_misses,
	 1 - ((double) ll_cache_read_misses / ll_cache_read_accesses));


  //  printf("pid: %ld = cache accceses: %lld ... misses: %lld, %lf\n", *((pid_t *) key), cache_accesses, cache_misses,
  //	 ((double) cache_misses) / cache_accesses );

}

void* check_counters(void* data)
{
  sleep(2);

  while (true) {
    //    list_traverse(fd_counters_per_pid, print_counters);
    sleep(2);
  }
}

void unschedule(pid_t pid, int core)
{
  void* socket_vd = (void*) mctop_hwcid_get_socket(topo, core);
  list* lst = list_get_value(used_sockets, socket_vd, compare_voids);
  if (lst == NULL) {
    perror("wait_for_process_async There is a missing socket from used_sockets");
    exit(1);
  }
  list_remove(lst, &pid, compare_pids);

  void* core_vd = (void*) mctop_hwcid_get_core(topo, core);
  lst = list_get_value(used_cores, core_vd, compare_voids);
  if (lst == NULL) {
    perror("END_PTHREADS There is a missing core from used_cores");
    exit(1);
  }
  list_remove(lst, &pid, compare_pids);
  used_hwcs[core] = false;
}

// returns chosen node as well
sem_t global_lock;
int get_hwc_and_schedule(heuristic_t h, pid_t pid, bool schedule, int* node) {
  sem_wait(&global_lock);

  int hwc = h.get_hwc(pid, node);

  if (schedule) {
    // TODO schedule thread ... NOW mitch!!!
    //    shced_set_affinity();
    //set_mem_affinity();
  }

  sem_post(&global_lock);
  return hwc;
}


void release_hwc(heuristic_t h, pid_t pid, int hwc) {
  sem_wait(&global_lock);
  h.release_hwc(hwc, pid);
  sem_post(&global_lock);
}

void schedule(pid_t pid, int core, int node)
{
  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;
    
  void* socket_vd = (void*) mctop_hwcid_get_socket(topo, core);
  list* lst = list_get_value(used_sockets, socket_vd, compare_voids);
  if (lst == NULL) {
    perror("There is a missing socket from used_sockets");
    exit(1);
  }
  list_add(lst, pid_pt, NULL);

  void* core_vd = (void*) mctop_hwcid_get_core(topo, core);
  lst = list_get_value(used_cores, core_vd, compare_voids);
  if (lst == NULL) {
    perror("There is a missing core from used_cores");
    exit(1);
  }
  list_add(lst, pid_pt, NULL);

  used_hwcs[core] = true;

  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(core, &set);

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) != 0) {
    perror("sched affinity inside schedule didn't work");
    exit(1);
  }

  int* pt_core = malloc(sizeof(int));
  *pt_core = core;

  list_add(hwcs_per_pid, (void *) pid_pt, (void *) pt_core);
}


/*void reschedule_for_process(pin_data** pin, int policy, pid_t pid) {
  int node;
  int core = get_hwc(pin, policy, pid, &node, topo, total_hwcs);

  int previous_core = *(int *) list_get_value(hwcs_per_pid, &pid, compare_pids);
  unschedule(pid, previous_core);
  schedule(pid, core, node);
  

  int num_elements;
  void** tids = list_get_all_values(tids_per_pid, &pid, compare_pids, &num_elements);

  for (int i = 0; i < num_elements; ++i) {
    pid_t tid = *((pid_t *) tids[i]);
    core = get_hwc(pin, policy, tid, &node, topo, total_hwcs);
    previous_core = *(int *) list_get_value(hwcs_per_pid, &tid, compare_pids);
    unschedule(tid, previous_core);
    schedule(tid, core, node);
  }
  } */


/*void reschedule_on_exit(pin_data** pin) {

  struct node* tmp = running_pids->head;

  while (tmp != NULL) {
    pid_t* pid = (pid_t *) tmp->key;

    int pol = *(int *) list_get_value(policy_per_process, pid, compare_pids);
    reschedule_for_process(pin, pol, *pid);

    tmp = tmp->next;
  }
  }*/

/*
void reschedule(int policy, int number_of_threads, pid_t pid, void** tids, pin_data** pin,
		int* used_threads)
{
  int i;
  // first schedule the process
  int process_core = pin[policy][0].core;
  int j = 1;
  //printf("The chosen process_Cores is :%d @#$#@$#@\n", process_core);
  while (used_hwcs[process_core]) {
    process_core = pin[policy][j].core;
    //printf("???The chosen process_Cores is :%d @#$#@$#@\n", process_core);

    ++j;
  }
  used_hwcs[process_core] = true;
  used_threads[0] = process_core; // save which used_hwcs were set to true so we set themback to false
  
  cpu_set_t st;
  CPU_ZERO(&st);
  CPU_SET(process_core, &st);

  //printf("pid: %lld to %d\n", pid, process_core);
  sched_setaffinity(pid, sizeof(cpu_set_t), &st);

  //printf("The number of threads is: %d\n", number_of_threads);
  for (i = 0; i < number_of_threads; ++i) {
    int process_core = pin[policy][i + 1].core;
    int j = 1;
    while (used_hwcs[process_core]) {
      process_core = pin[policy][j].core;
      ++j;
    }
    used_hwcs[process_core] = true;
    used_threads[i + 1] = process_core;

    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(process_core, &st);
    //printf("pid: %lld to %d\n", *((pid_t *) tids[i]), process_core);

    if (sched_setaffinity(*((pid_t *) tids[i]), sizeof(cpu_set_t), &st)) {
      perror("sched_setaffinity");
      return;
     }
  }
  } */

void print_pid(void *dt)
{
  printf("%ld ", (long)(*((pid_t *) dt)));
}

void print_stuff(void* key, void* data) 
{
  printf("%p: and something that is :%p\n", key, data);
}

/*void* find_best_policy(void* data, pin_data** pin)
{
  pid_t* pid = (pid_t*) data;
  hw_counters_fd* dt = list_get_value(fd_counters_per_pid, pid, compare_pids);

  int number_of_threads;
  void** tids = list_get_all_values(tids_per_pid, pid, compare_pids, &number_of_threads);

  int SLEEP_IN_BETWEEN_IN_MS = 230;

  int used_threads[40] = {0};
  int best_policy;
  int k;
  for (k = 0; k < 1; ++k) {

    double best_IPC = 0.0;
    int i;
    for (i = 1; i < 11; ++i) {
      if (i > 1) {
	int l;
	for (l = 0; l < number_of_threads + 1; ++l) {
	  used_hwcs[used_threads[l]] = false;
	}
      }

      long long int instructions = read_perf_counter(dt->instructions);
      long long int cycles = read_perf_counter(dt->cycles);


      // add hw conters of the process threads
      int j;
      for (j = 0; j < number_of_threads; ++j) {
	hw_counters_fd* cnts = list_get_value(fd_counters_per_pid, tids[j], compare_pids);
	if (cnts != NULL) {
	  instructions += read_perf_counter(cnts->instructions);
	  cycles += read_perf_counter(cnts->cycles);
	}
      }
      //printf("instructions: %lld, cycles: %lld\n", instructions, cycles);

      usleep(SLEEP_IN_BETWEEN_IN_MS * 1000); 
      long long int new_instructions = read_perf_counter(dt->instructions);
      long long int new_cycles = read_perf_counter(dt->cycles);

      for (j = 0; j < number_of_threads; ++j) {
	hw_counters_fd* cnts = list_get_value(fd_counters_per_pid, tids[j], compare_pids);
	if (cnts != NULL) {
	  new_instructions += read_perf_counter(cnts->instructions);
	  new_cycles += read_perf_counter(cnts->cycles);
	}
      }

      //printf("new_instructions: %lld, new_cycles: %lld\n", new_instructions, new_cycles);

      new_instructions -= instructions;
      new_cycles -= cycles;
    
      double IPC = new_instructions / ((double) new_cycles);
      if (IPC > best_IPC) {
	best_IPC = IPC;
	best_policy = i;
      }
      printf("Policy: %d, IPC: %lf\n", i, IPC);

      //reschedule(i, number_of_threads, *pid, tids, pin, used_threads);
    }
    printf("%d: Best policy is: %d\n", k, best_policy);
    //usleep(731 * 1000);
  }

  
  if (best_policy != 10) { //if it is the last one, no reason to reschedule
    int l;
    for (l = 0; l < number_of_threads + 1; ++l) {
      used_hwcs[used_threads[l]] = false;
    }

    //    reschedule(best_policy, number_of_threads, *pid, tids, pin, used_threads);
  }

  return NULL;
  }*/

// we need first to sleep (based on start_time_ms) and then start the timing of the process
// so, we cannot just fork in the main function
typedef struct {
  process* p;
  char** program;
  int start_time_ms;
} execute_process_args;

void* execute_process(void* dt) {
  execute_process_args* args = (execute_process_args*) dt;
  
  usleep(args->start_time_ms * 1000);

  char**program = args->program;
  printf("I'm here with the given p being: %p\n", (args->p));
  clock_gettime(CLOCK_MONOTONIC, (args->p)->start);
  pid_t pid = fork();

  if (pid == 0) {
    char* envp[1] = {NULL};
    //    time_t seconds  = time(NULL);
    //printf("About to start executing program with %d: %ld\n", result.num_id, seconds);
    execve(program[0], program, envp);
    printf("== %s\n", program[0]);
    perror("execve didn't work");
    exit(1);
  }

  pid_t* pid_pt = malloc(sizeof(pid_t));
  *pid_pt = pid;
  pthread_exit(pid_pt);
}

int main(int argc, char* argv[]) {

  heuristic = malloc(100);
  
  if (argc != 4) {
    perror("Call schedule like this: ./schedule input_file output_file_for_results heuristic\n");
    return EXIT_FAILURE;
  }

  if (sem_init(&(global_lock), 0, 1)) {
    perror("couldn't create lock for get_hwc");
    return EXIT_FAILURE;
  }
  results_fp = fopen(argv[2], "w");

  strcpy(heuristic, argv[3]);
  
  hwcs_per_pid = create_list();
  fd_counters_per_pid = create_list();
  tids_per_pid = create_list();
  running_pids = create_list();

  policy_per_process = create_list();

  topo = mctop_load(NULL);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }
  
  // mctop_print(topo);
  size_t num_nodes = mctop_get_num_nodes(topo);
  //size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  //size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;


  communication_slot* slots = initialize_slots();
  pin_data** pin = initialize_pin_data(topo);

  
  h.init = H_init;
  h.new_process = H_new_process;
  h.get_hwc = H_get_hwc;
  h.release_hwc = H_release_hwc;
  h.init(pin, topo);


  // create a thread to check for new slots
  pthread_t check_slots_thread;
  check_slots_args* args = malloc(sizeof(check_slots_args));
  args->slots = slots;
  args->pin = pin;
  pthread_create(&check_slots_thread, NULL, check_slots, args);


  pthread_t check_counters_thread;
  pthread_create(&check_counters_thread, NULL, check_counters, args);

  used_hwcs = malloc(sizeof(bool) * total_hwcs);
  for (int i = 0; i < total_hwcs; ++i) {
    used_hwcs[i] = false;
  }

  char line[300];
  FILE* fp = fopen(argv[1], "r");
  volatile int processes = 0;
  while (fgets(line, 300, fp)) {
    if (line[0] == '#') {
      continue; // the line is commented
    }

    processes++;

    read_line_output result = read_line(line);
    int num_id = result.num_id;
    int start_time_ms = result.start_time_ms;
    char* policy = result.policy;
    char** program = result.program;
    line[0] = '\0';

    mctop_alloc_policy pol;
    if (strcmp(policy, "MCTOP_ALLOC_SLATE") == 0) {
      pol = 1; // go sequential for now ...
    }
    else {
      pol = get_policy(policy);
    }


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


    execute_process_args args;
    args.p = p;
    args.start_time_ms = start_time_ms;
    args.program = program;
    pthread_t execute_process_thread;
    pthread_create(&execute_process_thread, NULL, execute_process, &args);

    void* resulty;
    if (pthread_join(execute_process_thread, &resulty) != 0) {
      perror("SHIT hit the fan\n");
      exit(-1);
    }

    pid_t pid = *((pid_t *) resulty);
    pid_t* pt_pid = malloc(sizeof(pid_t));
    *pt_pid = pid;

    list_add(running_pids, pt_pid, NULL);
    
    int* pt_pol = malloc(sizeof(int));
    *pt_pol = pol;
    list_add(policy_per_process, pt_pid, pt_pol);
    printf("Added policy in the list: %d\n", *pt_pol);


    h.new_process(pid, pol);
    int node;
    int core = get_hwc_and_schedule(h, pid, false, &node);

    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(core, &st);
    

    if (pol != MCTOP_ALLOC_NONE && strcmp(policy, "MCTOP_ALLOC_SLATE") != 0) {
      if (sched_setaffinity(pid, sizeof(cpu_set_t), &st)) {
	perror("sched_setaffinity!\n");
     }

      // TODO ... we don't use mempolicy anymore
      //unsigned long num = 1 << pin[pol][cnt].node;
      //if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
      //perror("mempolicy didn't work\n");
      //}
    }

    
    int *pt_core = malloc(sizeof(int));
    *pt_core = core;
    list_add(hwcs_per_pid, (void *) pt_pid, (void *) pt_core);

    pthread_t* pt = malloc(sizeof(pthread_t));
    p->pid = pt_pid;
    pthread_create(pt, NULL, wait_for_process_async, p);


    hw_counters_fd* cnt2 = malloc(sizeof(hw_counters_fd));
    cnt2->instructions = open_perf(*pt_pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    cnt2->cycles = open_perf(*pt_pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES);
    cnt2->ll_cache_read_accesses = open_perf(*pt_pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, ACCESS));
    // cnt2->l1d_cache_read_accesses = open_perf(*pt_pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(L1D, READ, ACCESS));
    cnt2->ll_cache_read_misses = open_perf(*pt_pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, MISS));
    //cnt2->l1i_cache_read_misses = open_perf(*pt_pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(L1I, READ, MISS));


    list_add(fd_counters_per_pid, (void*) pt_pid, (void*) cnt2);
    start_perf_reading(cnt2->instructions);
    start_perf_reading(cnt2->cycles);
    //start_perf_reading(cnt2->l1d_cache_read_accesses);
    start_perf_reading(cnt2->ll_cache_read_accesses);
    //start_perf_reading(cnt2->l1i_cache_read_misses);
    start_perf_reading(cnt2->ll_cache_read_misses);

    if (strcmp(policy, "MCTOP_ALLOC_SLATE") == 0)  {
      usleep(200 * 1000); // wait 200ms
      //find_best_policy(pt_pid, pin);
    }

    printf("Added %lld to list with processes\n", (long long) (*pt_pid));
  }


  while (processes != processes_finished) {
    sleep(1);
  }

  mctop_free(topo);
  
  return 0;
}

