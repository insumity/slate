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
int pin_cnt[MCTOP_ALLOC_NUM];

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
    mctop_alloc_t* alloc = mctop_alloc_create(topo, total_hwcs, num_nodes, pol);

    pin_cnt[i] = 0;
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


list* policy_per_process;

int compare_pids(void* p1, void* p2) {
  return *((unsigned int *) p1) == *((unsigned int *) p2);
}

void* check_slots(void* dt) {

  printf("I'm checking the slots thread\n");
  communication_slot* slots = dt;

  while (true) {
    for (int i = 0; i < NUM_SLOTS; ++i) {
      acquire_lock(i, slots);

      int fd;
      communication_slot* slot = slots + i;

      if (slot->used == START_PTHREADS) {
	printf("A ptherad stared\n, awesome time to be alive\n");
	pid_t pid = slot->pid;

	printf("process pid is: %ld\n", pid);
	pid_t* pt_pid = malloc(sizeof(pid_t));
	*pt_pid = pid;
	void* policy = list_get_value(policy_per_process, (void *) pt_pid, compare_pids);
	// TODO ... check result

	int pol = *((int *) policy);
	printf("=========policy is : %d\n", pol);
	
	pin_data pd = pin[pol][pin_cnt[pol]];
	int core = pd.core;
	int node = pd.node;

	pin_cnt[pol]++;
	slot->node = node;
	slot->core = core;
	slot->used = SCHEDULER;
	printf("GOT A NEW SLOT with node: %d and core: %d\n", slot->node, slot->core);
      }
      else if (slot->used == END_PTHREADS) {
	printf("The thread has finished. FFFFF\n");
	slot->used = NONE;
      }

      release_lock(i, slots);
    }
  }
}



int main(int argc, char* argv[]) {

  policy_per_process = create_list();

  int fd = open(SLOTS_FILE_NAME, O_CREAT | O_RDWR, 0777);
  if (fd == -1) {
    fprintf(stderr, "Couldnt' open file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    return NULL;
  }

  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "Couldn't ftruncate file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
    return NULL;
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


  printf("Provide it with the following input: \"POLICY FULL_PATH_PROGRAM\"\n");

  mctop_t* topo = mctop_load("lpd48core.mct");
  if (topo)   {
    mctop_print(topo);
  }

  initialize_pin_data(topo);

  // create a thread to check for new slots
  pthread_t check_slots_threads;
  pthread_create(&check_slots_threads, NULL, check_slots, slots);

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores = mctop_get_num_cores(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  size_t total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;
  bool used_hwc[total_hwcs];
  memset(used_hwc, false, total_hwcs);
  // printf("TOTAL number of hardware contexts: %zu\n", num_hwc_per_processor);

  // printf("%zu %zu %zu %zu %zu\n", num_nodes, num_cores, num_cores_per_socket, num_hwc_per_socket, num_hwc_per_core);
  const uint8_t NUMBER_OF_POLICIES = 11;



  while (1) {
    char policy[100];
    int program_cnt = 0;
    char line[300], tmp_line[300];

    fgets(line, 300, stdin);
    line[strlen(line) - 1 ] = '\0'; // remove new line character
    //	printf("line to be written: [[%s]]\n", line);
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

    pid_t pid;
    if (c == 'Y') {
      pid = fork();
    }

    if (pid == 0) {
      char* envp[1] = {NULL};
      printf("About to run execve\n");
      execve(program[0], program, envp);
      perror("execve didn't work!\n"); 
    }


    mctop_alloc_policy pol = get_policy(policy);
    /* TODO not sure about the third parameter */

    pid_t *pt_pid = malloc(sizeof(pid_t));
    *pt_pid = pid;

    int* pt_pol = malloc(sizeof(int));
    *pt_pol = pol;
    printf("Added with policy: %d\n", *pt_pol);
    list_add(policy_per_process, pt_pid, pt_pol);

    /*
      int number_of_threads = 0;
      //get_thread_ids(pid, &number_of_threads);
      int* thread_pids = get_children_thread_ids(pid, &number_of_threads);
      if (number_of_threads != 6) {

      }

      printf("Number of threads: %d\n", number_of_threads);
    */

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

    //pthread_t* pt = malloc(sizeof(pthread_t));
    //pthread_create(pt, NULL, wait_for_process, fdt);
  }
	
  mctop_free(topo);
  
  return 0;
}

