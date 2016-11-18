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

#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10

typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;

typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
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

typedef struct {
  pid_t child;

  int* used_hwc;
  int length;

  bool* used_hwc_to_change;
} forked_data;

void* wait_for_process(void* dt) {
  forked_data fdt = *((forked_data *) dt);

  pid_t pid = fdt.child;
  printf("I'm here with pid: %ld\n", pid);

  int return_status = -1;
  waitpid(pid, &return_status, 0);
  if (return_status == 0) {
    printf("process finished fine!\n");
  }
  
  for (int i = 0; i < fdt.length; ++i) {
    printf("Used one: %d\n", (fdt.used_hwc)[i]);
    (fdt.used_hwc_to_change)[(fdt.used_hwc)[i]] = false;
  }

  return NULL;
}

typedef struct {
  int core;
  int node;
} pin_data;

pin_data pin[MCTOP_ALLOC_NUM][48];
//int pin_cnt[MCTOP_ALLOC_NUM];

pin_data create_pin_data(int core, int node) {
  pin_data tmp;
  tmp.core = core;
  tmp.node = node;
  return tmp;
}

void initialize_pin_data(mctop_t* topo) {

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  size_t total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;

  for (int i = 0; i < MCTOP_ALLOC_NUM; ++i) {

    /* this policy does not contain any hardware contexts */
    if (i == MCTOP_ALLOC_NONE) {
      continue;
    }

    mctop_alloc_policy pol = i;
    // TODO ... n_config should be different depending on the policy
    mctop_alloc_t* alloc = mctop_alloc_create(topo, total_hwcs, num_nodes, pol);

    //pin_cnt[i] = 0;
    for (uint hwc_i = 0; hwc_i < alloc->n_hwcs; hwc_i++)
      {
	uint hwc = alloc->hwcs[hwc_i];
	pin[i][hwc_i] = create_pin_data(hwc, mctop_hwcid_get_local_node(topo, hwc));
      }
  }
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
void* check_slots(void* dt) {

  communication_slot* slots = dt;

  while (true) {
    usleep(5000);
    for (int i = 0; i < NUM_SLOTS; ++i) {
      acquire_lock(i, slots);

      communication_slot* slot = slots + i;

      if (slot->used == START_PTHREADS) {
	pid_t pid = slot->pid;

	pid_t* pt_pid = malloc(sizeof(pid_t));
	*pt_pid = pid;
	void* policy = list_get_value(policy_per_process, (void *) pt_pid, compare_pids);
	// TODO ... check result

	int pol = *((int *) policy);

	int cnt = 0;
	pin_data pd = pin[pol][cnt];
	int core = pd.core;
	int node = pd.node;
	//pin_cnt[pol]++;
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
	used_hwcs[core] = true;

	pid_t* pt_tid = malloc(sizeof(pid_t));
	*pt_tid = slot->tid;

	int* pt_core = malloc(sizeof(int));
	*pt_core = core;
	list_add(hwcs_per_pid, (void *) pt_tid, (void *) pt_core);

	slot->used = SCHEDULER;
	printf("GOT A NEW SLOT with node: %d and core: %d for thread with pid %d\n", slot->node, slot->core, slot->tid);
      }
      else if (slot->used == END_PTHREADS) {
	printf("The thread with pid %d and ppid %d has finished.", slot->tid, slot->pid);
	// find the hwc this thread was using ...
	int hwc = *((int *) list_get_value(hwcs_per_pid, (void *) &slot->tid, compare_pids));
	printf("Hwc that was used is: %d\n", hwc);
	used_hwcs[hwc] = false;

	slot->used = NONE;
      }

      release_lock(i, slots);
    }
  }
}


void* wait_for_process_async(void* dt)
{
  pid_t* pid = (pid_t *) dt;
  int status;
  waitpid(*pid, &status, 0);

  int hwc = *((int *) list_get_value(hwcs_per_pid, (void *) pid, compare_pids));
  used_hwcs[hwc] = false;

  return NULL;
}


int main(int argc, char* argv[]) {

  policy_per_process = create_list();
  hwcs_per_pid = create_list();

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


  /* initialize semaphores */
  for (int j = 0; j < NUM_SLOTS; ++j) {

    initialize_lock(j, slots);
    
    communication_slot* slot = &slots[j];
    slot->node = -1;
    slot->core = -1;
    slot->tid = 0;
    slot->pid = 0;
    slot->used = NONE;
  }

  mctop_t* topo = mctop_load("lpd48core.mct");
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }

  initialize_pin_data(topo);

  printf("Provide it with the following input: \"POLICY FULL_PATH_PROGRAM\"\n");

  // create a thread to check for new slots
  pthread_t check_slots_threads;
  pthread_create(&check_slots_threads, NULL, check_slots, slots);

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;
  used_hwcs = malloc(sizeof(bool) * total_hwcs);
  memset(used_hwcs, false, total_hwcs);

  while (1) {
    char policy[100];
    int program_cnt = 0;
    char line[300], tmp_line[300];

    fgets(line, 300, stdin);
    line[strlen(line) - 1 ] = '\0'; // remove new line character
    strcpy(tmp_line, line);
    int first_time = 1;
    char* word = strtok(line, " ");
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

    printf("verify the forking with Y: \n");
    char c = getchar();
    getchar(); // read new line


    /*
    unsigned long num = pin[pin_cnt].node;
    cpu_set_t foobarisios;
    CPU_ZERO(&foobarisios);
    CPU_SET(pin[pin_cnt].core, &foobarisios);
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &foobarisios) == -1) {
      perror("setaffinity didn't work");
    }
    if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
      perror("mempolicy didn't work\n");
    }
    ///pin_cnt[
    */


    /* TODO not sure about the third parameter */
    mctop_alloc_policy pol = get_policy(policy);
    cpu_set_t st;
    CPU_ZERO(&st);
    int cnt = 0;
    while (used_hwcs[pin[pol][cnt].core]) {
      cnt++;
    }
    int process_core = pin[pol][cnt].core;
    used_hwcs[pin[pol][cnt].core] = true;
    CPU_SET(pin[pol][cnt].core, &st);

    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &st)) {
      perror("sched_setaffinity!\n");
    }
      
    long num = 1 << pin[pol][cnt].node;
    if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
      perror("mempolicy didn't work\n");
    }


    pid_t pid;
    if (c == 'Y') {
      pid = fork();      
    }

    if (pid == 0) {
      char* envp[1] = {NULL};
      printf("About to run execve.\n");
      execve(program[0], program, envp);
      perror("execve didn't work!\n"); 
    }
    pid_t* pt_pid = malloc(sizeof(pid_t));
    *pt_pid = pid;
    printf("Added process: %ld\n", (long) *pt_pid);
    int *pt_core = malloc(sizeof(int));
    *pt_core = process_core;
    list_add(hwcs_per_pid, (void *) pt_pid, (void *) pt_core);
    pthread_t* pt = malloc(sizeof(pthread_t));
    pthread_create(pt, NULL, wait_for_process_async, pt_pid);



    int* pt_pol = malloc(sizeof(int));
    *pt_pol = pol;
    printf("Added with policy: %d\n", *pt_pol);
    list_add(policy_per_process, pt_pid, pt_pol);

    /*
      forked_data* fdt = malloc(sizeof(forked_data));
      fdt->child = pid;
      fdt->length = 6; // TODO 
      fdt->used_hwc = malloc(sizeof(int) * fdt->length);
      fdt->used_hwc_to_change = used_hwc;

      int k = 0;
      cpu_set_t set;

      CPU_ZERO(&set);
      for (uint hwc_i = 0; hwc_i < alloc->n_hwcs; hwc_i++)
      {
      uint hwc = alloc->hwcs[hwc_i];
      if (used_hwc[hwc]) {
      continue;
      }

      CPU_SET(hwc, &set);
      used_hwc[hwc] = true;
      fdt->used_hwc[k++] = hwc;


      printf("hwc : %d\n", hwc);
      if (k == number_of_threads) {
      break;
      }
      }

      for (long i = 0; i < 48; i++) {
      if (CPU_ISSET(i, &set)) {
      printf("Core %d is in cputset\n", i);
      }
      }
    */
    // clean used hardware contexts when process finishes

    //        printf("Where those threads should be pinned!: (cpu set %d) and (count set %d)\n", set, CPU_COUNT(&set));
    /*
      for (int i = 0; i < number_of_threads; ++i) {
      printf("%ld is the thread: %d\n", thread_pids[i], i);
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(alloc->hwcs[i], &st);
      if (sched_setaffinity(thread_pids[i], sizeof(cpu_set_t), &st)) {
      perror("sched_setaffinity!\n");
      return -1;
      }
      }*/

    //sched_setaffinity(pid, sizeof(cpu_set_t), &set);

  }
	
  mctop_free(topo);
  
  return 0;
}

