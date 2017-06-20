/* Like loc4 but only with readers: 
Microbenchmark where half of the threads are RANDOMLY reading and half of the others are RANDOMLY writing in a shared array that exists in the memory node of only one socket. */

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdatomic.h>
#include <numa.h>
#include <time.h>

#define ATOMIC_INTS_PER_CACHE_LINE 16
#define MAX_NUM_THREADS 48
#define KB 1024
#define MB (1024L * KB)

volatile uint64_t *shared_memory;

int number_of_threads, pin, pol;
#define MAX_NUMBER_OF_THREADS 50
volatile long long int loops_per_thread[MAX_NUMBER_OF_THREADS];

#define RANDOM_NUMBERS 250000

typedef struct {
  int index;
  long long* random_numbers;
} thread_dt;

void *reader(void *v)
{
  thread_dt* dt = (thread_dt *) v;
  int index_thread = dt->index;
  long long int loops = 0;
  long offset = 1L << 14;

  long sum = 0;

  long long* random_numbers = dt->random_numbers;

  while (true) {
    long long index = random_numbers[loops % RANDOM_NUMBERS];
    sum += shared_memory[index];
    loops++;

    if (loops & offset == offset) {
      loops_per_thread[index_thread]++;
    }

  }
  printf("%ld\n", sum);

  return NULL;
}

void sig_handler(int signo)
{
  long long sum = 0;
  if (signo == SIGINT) {
    for (int i = 0; i < number_of_threads; ++i) {
      sum += loops_per_thread[i];
    }
    printf("received SIGINT: %lld\n", sum);
  }

  char filename[500];
  sprintf(filename, "total_loops_threads_%d_policy_%d_pin_%d", number_of_threads, pol, pin);
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    perror("couldn't open the file\n");
  }
  fprintf(fp, "Loops per thread\n%.2lf\n", sum / (double) number_of_threads);

  if (fclose(fp) != 0) {
    perror("couldn't close the file");
  }

  exit(1);
}

int main(int argc, char* argv[])
{
  shared_memory = NULL;
  shared_memory = (volatile uint64_t *) numa_alloc_onnode(16L * MB * KB, 0);

  printf("SHARED MEMORY : %p\n", shared_memory);
  
  memset(shared_memory, 0, 16L * MB * KB);
  

  srand(time(NULL));

  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    perror("couldn't set up signal");
  }

  int oc;

  pin = 0;
  while ((oc = getopt(argc, argv, "t:s:p")) != -1) {
    switch (oc) {
    case 't':
      number_of_threads = atoi(optarg);
      break;
    case 's':
      printf("P input: (%s)\n", optarg);
      pol = atoi(optarg);
      break;
    case 'p':
      pin = 1;
      break;
    default:
      printf("Use t, s, or p as option!\n");
    }
  }


  pthread_t threads[number_of_threads];

  int cores_loc_hwcs[20] = {0, 48, 4, 52, 8, 56, 12, 60, 16, 64, 20, 68, 24, 72, 28, 76, 32, 80, 36, 84};

  
  int cores_loc_cores[48] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44,
			     1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45,
			     2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46,
			     3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47};
  
  int cores_rr[48];
  for (int i = 0; i < 48; ++i) {
    cores_rr[i] = i;
  }
  
  int cores[48];

  for (int i = 0; i < 48; ++i) {
    if (pol == 0) {
      cores[i] = cores_loc_cores[i];
    }
    else {
      cores[i] = cores_rr[i];
    }
  }


  if (pin) {
    printf("I'm about to pin!\n");
    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(cores[0], &st);
    sched_setaffinity(0,  sizeof(cpu_set_t), &st);
  }


  printf("The number of threads is: %d\n", number_of_threads);
  printf("Used cores are:\n");

  int cnt = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    thread_dt* pa = malloc(sizeof(thread_dt));
    pa->index = i;
    pa->random_numbers = malloc(sizeof(long long) * RANDOM_NUMBERS);
    for (int i = 0; i < RANDOM_NUMBERS; ++i) {
      pa->random_numbers[i] = rand() % (16L * MB * KB);
    }

    if (pthread_create(&threads[i], NULL, reader, pa)) {
      fprintf(stderr, "Error creating thread\n");
      return EXIT_FAILURE;
    }

    if (pin) {
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(cores[i], &st);
      printf("%d ", cores[i]);
      if (pthread_setaffinity_np(threads[i], sizeof(st), &st)) {
	perror("Something went wrong while setting the affinity!\n");
	return EXIT_FAILURE;
      }
    }
    
  }
  printf("\n");

  for (int i = 0; i < number_of_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}

