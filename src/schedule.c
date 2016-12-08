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

#include "ticket.h"
#include "list.h"

#define SLEEPING_TIME_IN_MICROSECONDS 5000

#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10

typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;

typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
  int policy;
} communication_slot;

mctop_alloc_policy get_policy(char* policy) {
  mctop_alloc_policy pol;
  if (strcmp(policy, "MCTOP_ALLOC_NONE") == 0) {
    pol = MCTOP_ALLOC_NONE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_SEQUENTIAL") == 0) {
    pol = MCTOP_ALLOC_SEQUENTIAL;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS") == 0) {
    pol = MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_CORES") == 0) {
    pol = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_BOUND") == 0) {
    pol = MCTOP_ALLOC_BW_BOUND;
  }
  else {
    perror("Not recognized policy\n");
    return -1;
  }

  return pol;
}

/* typedef struct { */
/*   pid_t child; */

/*   int* used_hwc; */
/*   int length; */

/*   bool* used_hwc_to_change; */
/* } forked_data; */

/* void* wait_for_process(void* dt) { */
/*   forked_data fdt = *((forked_data *) dt); */

/*   pid_t pid = fdt.child; */
/*   printf("I'm here with pid: %ld\n", pid); */

/*   int return_status = -1; */
/*   waitpid(pid, &return_status, 0); */
/*   if (return_status == 0) { */
/*     printf("process finished fine!\n"); */
/*   } */
  
/*   for (int i = 0; i < fdt.length; ++i) { */
/*     printf("Used one: %d\n", (fdt.used_hwc)[i]); */
/*     (fdt.used_hwc_to_change)[(fdt.used_hwc)[i]] = false; */
/*   } */

/*   return NULL; */
/* } */

typedef struct {
  int core;
  int node;
} pin_data;

//int pin_cnt[MCTOP_ALLOC_NUM];

pin_data create_pin_data(int core, int node) {
  pin_data tmp;
  tmp.core = core;
  tmp.node = node;
  return tmp;
}

pin_data** initialize_pin_data(mctop_t* topo) {

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  size_t total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;
  pin_data** pin = malloc(sizeof(pin_data*) * MCTOP_ALLOC_NUM);
  for (int i = 0; i < MCTOP_ALLOC_NUM; ++i) {
    pin[i] = malloc(sizeof(pin_data) * total_hwcs);
  }

  for (int i = 0; i < MCTOP_ALLOC_NUM; ++i) {

    mctop_alloc_policy pol = i;
    /* this policy does not contain any hardware contexts */
    if (i == MCTOP_ALLOC_NONE) {
      pol = i + 1; // the hardware contexts are always skipped actually
      // only done so we can measure cost of communication between slate and glibc
    }


    int n_config = -1;
    // Depending on the policy, n_config (3rd parameter of mctop_alloc_create) has different semantics.
    if (pol == MCTOP_ALLOC_MIN_LAT_HWCS || pol == MCTOP_ALLOC_MIN_LAT_CORES_HWCS || pol == MCTOP_ALLOC_MIN_LAT_CORES) {
      n_config = num_hwc_per_socket;
    }
    else {
      n_config = num_nodes;
    }
    assert(n_config != -1);

    mctop_alloc_t* alloc = mctop_alloc_create(topo, total_hwcs, n_config, pol);
    for (uint hwc_i = 0; hwc_i < alloc->n_hwcs; hwc_i++)
      {
	uint hwc = alloc->hwcs[hwc_i];
	mctop_hwcid_get_local_node(topo, hwc);
	pin[i][hwc_i] = create_pin_data(hwc, mctop_hwcid_get_local_node(topo, hwc));
      }
    mctop_alloc_free(alloc);
  }

  return pin;
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

// hwcs_per_id stores (pid, hwcs) meaning thread or process with pid is using hwcs
list* policy_per_process, *hwcs_per_pid;
bool* used_hwcs;

int compare_pids(void* p1, void* p2) {
  return *((unsigned int *) p1) == *((unsigned int *) p2);
}

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
	pid_t pid = slot->pid;

	pid_t* pt_pid = malloc(sizeof(pid_t));
	*pt_pid = pid;
	void* policy = list_get_value(policy_per_process, (void *) pt_pid, compare_pids);
	if (policy == NULL) {
	  fprintf(stderr, "Couldn't find policy of specific process with pid: %ld\n", (long int*) (pt_pid));
	  exit(-1);
	}

	int pol = *((int *) policy);

	int cnt = 0;
	pin_data pd = pin[pol][cnt];
	int core = pd.core;
	int node = pd.node;
	cnt++;
	while (used_hwcs[core]) {
	  if (cnt == total_hwcs) {
	    perror("We ran out of cores\n");
	    return NULL;
	  }

	  pd = pin[pol][cnt];
	  core = pd.core;
	  node = pd.node;
	  cnt++;
	}

	slot->node = node;
	slot->core = core;
	slot->policy = pol;
	used_hwcs[core] = true;

	pid_t* pt_tid = malloc(sizeof(pid_t));
	*pt_tid = slot->tid;

	int* pt_core = malloc(sizeof(int));
	*pt_core = core;
	list_add(hwcs_per_pid, (void *) pt_tid, (void *) pt_core);

	slot->used = SCHEDULER;
      }
      else if (slot->used == END_PTHREADS) {
	// find the hwc this thread was using ...
	int* hwc = ((int *) list_get_value(hwcs_per_pid, (void *) &slot->tid, compare_pids));
	if (hwc == NULL) {
	  fprintf(stderr, "Couldn't find the hwc this process is using: %ld\n", (slot->tid));
	  exit(-1);
	}
	
	used_hwcs[*hwc] = false;

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

FILE* results_fp;
volatile int processes_finished = 0;
void* wait_for_process_async(void* dt)
{
  process* p = (process *) dt;
  struct timespec *start = p->start;
  pid_t* pid = p->pid;
  int status;
  waitpid(*pid, &status, 0);

  int hwc = *((int *) list_get_value(hwcs_per_pid, (void *) pid, compare_pids));
  used_hwcs[hwc] = false;

  struct timespec *finish = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);
  
  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;
  fprintf(results_fp, "%d\t%ld\t%lf\t%s\t%s\n", p->num_id, *pid, elapsed, p->policy, p->program);
  processes_finished++;
  return NULL;
}

// Given "line" that is of a format "POLICY program parameters" returns the program
// as a 2D char array and the policy
char** read_line(char* line, char* policy, int* num_id)
{
    int program_cnt = 0;
    char tmp_line[300];

    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1 ] = '\0'; // remove new line character
    }
    strcpy(tmp_line, line);
    int first_time = 1;
    char* word = strtok(line, " ");
    *num_id = atoi(word);
    printf("The num id is: %d\n", *num_id);
    word = strtok(NULL, " ");
    while (word != NULL)  {
      if (first_time) {
	first_time = 0;
	strcpy(policy, word);
      }
      else {
	program_cnt++;
      }
      word = strtok(NULL, " ");
    }

    char** program = malloc(sizeof(char *) * (program_cnt + 1));
    first_time = 1;
    program_cnt = 0;
    word = strtok(tmp_line, " ");
    strtok(NULL, " "); // skip the number
    while (word != NULL)  {

      if (first_time) {
	first_time = 0;
      }
      else {
	program[program_cnt] = malloc(sizeof(char) * 200);
	strcpy(program[program_cnt], word);
	program_cnt++;
      }
      word = strtok(NULL, " ");
    }

    program[program_cnt] = NULL;

    return program;
}

communication_slot* initialize_slots() {
  int fd = open(SLOTS_FILE_NAME, O_CREAT | O_RDWR, 0777);
  if (fd == -1) {
    fprintf(stderr, "Couldnt' open file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    return EXIT_FAILURE;
  }

  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "Couldn't ftruncate file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    return EXIT_FAILURE;
  }
  communication_slot *slots = mmap(NULL, sizeof(communication_slot) * NUM_SLOTS, 
				  PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (slots == MAP_FAILED) {
    fprintf(stderr, "main mmap failed for file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
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

int main(int argc, char* argv[]) {

  if (argc != 3) {
    perror("Call schedule like this: ./schedule input_file output_file_for_results\n");
    return -1;
  }

  results_fp = fopen(argv[2], "w");
  policy_per_process = create_list();
  hwcs_per_pid = create_list();


  mctop_t* topo = mctop_load(NULL);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }

  communication_slot* slots = initialize_slots();
  pin_data** pin = initialize_pin_data(topo);

  // create a thread to check for new slots
  pthread_t check_slots_thread;
  check_slots_args* args = malloc(sizeof(check_slots_args));
  args->slots = slots;
  args->pin = pin;
  pthread_create(&check_slots_thread, NULL, check_slots, args);

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;
  used_hwcs = malloc(sizeof(bool) * total_hwcs);
  memset(used_hwcs, false, total_hwcs);

  char line[300];
  FILE* fp = fopen(argv[1], "r");
  volatile int processes = 0;
  while (fgets(line, 300, fp)) {
    if (line[0] == '#') {
      continue; // the line is commented
    }

    processes++;
    char policy[100];
    int num_id;
    char** program = read_line(line, policy, &num_id);
    line[0] = '\0';

    mctop_alloc_policy pol = get_policy(policy);
    cpu_set_t st;
    CPU_ZERO(&st);
    int cnt = 0;
    /* FIXME: can run out ... */
    while (used_hwcs[pin[pol][cnt].core]) {
      cnt++;
    }
    int process_core = pin[pol][cnt].core;
    used_hwcs[pin[pol][cnt].core] = true;
    CPU_SET(pin[pol][cnt].core, &st);

    if (pol != MCTOP_ALLOC_NONE) {
      if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &st)) {
	perror("sched_setaffinity!\n");
     }
      
      unsigned long num = 1 << pin[pol][cnt].node;
      if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
	perror("mempolicy didn't work\n");
      }
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

    clock_gettime(CLOCK_MONOTONIC, p->start);
    pid_t pid = fork();      

    if (pid == 0) {
      char* envp[1] = {NULL};
      execve(program[0], program, envp);
      perror("execve didn't work!\n"); 
    }

    pid_t* pt_pid = malloc(sizeof(pid_t));
    *pt_pid = pid;
    int *pt_core = malloc(sizeof(int));
    *pt_core = process_core;
    list_add(hwcs_per_pid, (void *) pt_pid, (void *) pt_core);

    pthread_t* pt = malloc(sizeof(pthread_t));
    p->pid = pt_pid;
    pthread_create(pt, NULL, wait_for_process_async, p);

    int* pt_pol = malloc(sizeof(int));
    *pt_pol = pol;
    list_add(policy_per_process, pt_pid, pt_pol);
  }


  while (processes != processes_finished) {
    sleep(1);
  }

  mctop_free(topo);
  
  return 0;
}

