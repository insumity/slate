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


#define SHARED_MEM_ID "/tmp/scheduler"
#define NUM_SLOTS 10

typedef enum {NONE, SCHEDULER, PTHREADS} used_by;
typedef struct {
  int core, node;
  sem_t lock;
  pid_t tid;
  used_by used;
} communication_slot;


int* get_children_thread_ids(pid_t pid, int* number_of_threads) {
  //pstree -p 31796 | grep -P -o '\([0-9]+\)'
  FILE *fp;
  char path[1035];

  char command[500];
  command[0] = '\0';
  strcat(command, "pstree -p ");

  char str_pid[10];
  sprintf(str_pid, "%ld", (long) pid);
  strcat(command, str_pid);
  strcat(command, " | grep -P -o '\\([0-9]+\\)'");

  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }
  
  int cnt = 0;
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
        printf("[[[%s\n)))", path);
	cnt++;
  }
  *number_of_threads = cnt;
  cnt = 0;
  int* thread_ids = malloc(sizeof(int) * *number_of_threads);

  fp = popen(command, "r");
  while (fgets(path, sizeof(path) -1, fp) != NULL) {
    char id[10];
    strncpy(id, path + 1, strlen(path) - 3);
    id[strlen(path) - 3] = '\0';
    //printf("The id is: [[%s]]\\n", id);
    thread_ids[cnt++] = atol(id);
  }

  *number_of_threads = cnt;
  
  pclose(fp);

  return thread_ids;
}

int* get_thread_ids(pid_t pid, int *number_of_threads) {

    char str_pid[100];
    str_pid[0] = '\0';
    sprintf(str_pid, "%d", pid);
    
    char directory[200];
    directory[0] = '\0';
    strcat(directory, "/proc/");
    strcat(directory, str_pid);
    strcat(directory, "/task/");

    
    DIR *d = opendir(directory);
    struct dirent *dir;

    int num_threads = 0;

    if (d)  {
        while ((dir = readdir(d)) != NULL)  {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            num_threads++;
        }

        int* result = malloc(sizeof(int) * num_threads);
        int thread_cnt = 0;

        d = opendir(directory);
        if (d == NULL) {
            perror("couldn't open the directory!");
            return NULL;
        }

        while ((dir = readdir(d)) != NULL)  {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            result[thread_cnt] = atoi(dir->d_name);
            thread_cnt++;
        }
        *number_of_threads = thread_cnt;
        
        closedir(d);

        return result;
    }
    else {
        perror("couldn't open the directory!");
        return NULL;
    }

    assert(0);
}

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

int migrate_memory(int pid, int to) {
  long _all = (1 << 8) - 1;
  long _to = 1 << to;
  return migrate_pages(pid, 30, &_all, &_to);
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

pin_data pin[48];
int pin_cnt;

pin_data create_pin_data(int core, int node) {
  pin_data tmp;
  tmp.core = core;
  tmp.node = node;
  return tmp;
}
void* check_slots(void* dt) {

  printf("I'm checking the slots thread\n");
  communication_slot* slots = dt;

  while (true) {
    usleep(500);
    for (int i = 0; i < NUM_SLOTS; ++i) {
      communication_slot* slot = slots + i;
      sem_wait(&(slot->lock));

      if (slot->used == PTHREADS) {
	pin_data pd = pin[pin_cnt];
	int core = pd.core;
	int node = pd.node;
	pin_cnt++;
	slot->node = node;
	slot->core = core;
	slot->used = SCHEDULER;
	printf("GOT A NEW SLOT with node: %d and core: %d\n", slot->node, slot->core);
      }
      sem_post(&(slot->lock));
    }
  }
}

int main(int argc, char* argv[]) {
  int fd;
  communication_slot *addr;
  pin_cnt = 1;
  pin[0] = create_pin_data(0, 0);
  pin[1] = create_pin_data(6, 1);
  pin[2] = create_pin_data(12, 2);
  pin[3] = create_pin_data(18, 3);
  pin[4] = create_pin_data(24, 4);
  pin[5] = create_pin_data(30, 5);

  if (argc != 1) {
    shm_unlink(SHARED_MEM_ID);
    return EXIT_SUCCESS;
  }

  fd = open(SHARED_MEM_ID, O_RDWR | O_CREAT, 0777);
  if (fd == -1) {
    fprintf(stderr, "Open failed : %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "ftruncate : %s\n", strerror(errno));
    return EXIT_FAILURE;

  }

  addr = mmap(0, sizeof(communication_slot) * NUM_SLOTS, PROT_READ | PROT_WRITE,
              MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    fprintf(stderr, "mmap failed:%s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  /* initialize semaphores */
  for (int j = 0; j < NUM_SLOTS; ++j) {
    printf("Addr: %p, Addr + j: %p\n", addr, addr + j);
    communication_slot* a = addr + j;
    a->node = 0;
    a->core = 0;
    a->used = NONE;
    int ret = sem_init(&(a->lock), 1, 1);
    int v;
    sem_getvalue(&(a->lock), &v);
    printf("slot: %p with value: %d\n", &(a->lock), v);
    if (ret == 1) {
      perror("Semaphore failed to initialize");
    }
  }

  printf("Map addr is %6.6X\n", addr);
  printf("Press break to stop.\n");

  // create a thread to check for new slots
  pthread_t check_slots_threads;
  pthread_create(&check_slots_threads, NULL, check_slots, addr);


    printf("Provide it with the following input: \"POLICY FULL_PATH_PROGRAM\"\n");

    mctop_t* topo = mctop_load("lpd48core.mct");
    if (topo)   {
      mctop_print(topo);
    }

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
    //list threads[NUMBER_OF_POLICIES];

    int j;
    for (j = 0; j < NUMBER_OF_POLICIES; ++j) {
       // threads[j] = create_list(); 
    }

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

	/*	cpu_set_t try;
	CPU_ZERO(&try);
	CPU_SET(0, &try);
	CPU_SET(6, &try);
	CPU_SET(12, &try);
	CPU_SET(18, &try);
	CPU_SET(24, &try);
	CPU_SET(30, &try);

	printf("how many cpus do we have: %d\n", CPU_COUNT(&try));

	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &try) == -1) {
	  printf("ERROR\n");
	}
	else {
	  printf("Sched affinity works!\n");
	}


	struct bitmask *bm = numa_get_membind();

	printf("BITMASK: %ld, %ld\n", bm->size, *(bm->maskp));
	struct bitmask f;
	f.size = 6;
	long num = 63;
	f.maskp = &num;
	numa_set_membind(&f);

	bm = numa_get_membind();

	printf("BITMASK: %ld, %ld\n", bm->size, *(bm->maskp));
	*/
	unsigned long num = 0;
	cpu_set_t foobarisios;
	CPU_ZERO(&foobarisios);
	CPU_SET(0, &foobarisios);
	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &foobarisios) == -1) {
	  perror("setaffinity didn't work");
	}
	if (set_mempolicy(MPOL_PREFERRED, &num, 31) != 0) {
	  perror("mempolicy didn't work\n");
	}

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

	//printf("PID IS: %d\n", pid);


	/*
        int number_of_threads = 0;
        //get_thread_ids(pid, &number_of_threads);
	int* thread_pids = get_children_thread_ids(pid, &number_of_threads);
	if (number_of_threads != 6) {

	}

	printf("Number of threads: %d\n", number_of_threads);
	*/
        mctop_alloc_policy pol = get_policy(policy);
        /* TODO not sure about the third parameter */
        mctop_alloc_t* alloc = mctop_alloc_create(topo, total_hwcs, num_nodes, pol);
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

	/*	for (int k = 0; k < 1; ++k) {
	  sleep(2);
	  for (int i = 0; i < number_of_threads; ++i) {
	    if (migrate_memory(thread_pids[i], alloc->hwcs[i] / 6) != 0) {
	      perror("baby baby ooo .. couldn't migrate memory\n");
	    }
	    usleep(100);
	  }
	  }*/
	//sched_setaffinity(pid, sizeof(cpu_set_t), &set);

	//pthread_t* pt = malloc(sizeof(pthread_t));
	//pthread_create(pt, NULL, wait_for_process, fdt);
    }
	
    mctop_free(topo);

    return 0;
}

