#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <numa.h>
#include <getopt.h>
#include <strings.h>

typedef struct {
  long long size;
  int repetions;
  int numa_node;
} inc_dt;

void *inc(void *index_pt)
{
  sleep(3);
  inc_dt* dt = (inc_dt *) index_pt;
  long long size = dt->size;
  int repetions = dt->repetions;
  int numa_node = dt->numa_node;
  
  int *y = numa_alloc_onnode(sizeof(int) * size, numa_node);
  bzero(y, sizeof(int) * size);

  long sum = 0;

  const size_t n_warmup = size >> 2;
  for (size_t i = 0; i < n_warmup; i++) {
    sum = y[i];
  }
  
  for (int k = 0; k < repetions; ++k) {
    for (int j = 0; j < size; ++j) {
      sum = y[j];
    }
  }

  for (int k = 0; k < repetions; ++k) {
    for (int j = 0; j < size; ++j) {
      y[j] = 0xAAAAFFFF;
    }
  }

  for (size_t i = 0; i < n_warmup; i++) {
    sum = y[i];
  }
  
  printf("%ld\n", sum);

  return NULL;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));

  int oc;             /* option character */

  int number_of_threads, memory, repetitions, pin;
  pin = 0;
  while ((oc = getopt(argc, argv, "t:m:r:p")) != -1) {
    switch (oc) {
    case 't':
      number_of_threads = atoi(optarg);
      break;
    case 'm':
      memory = atol(optarg);
      break;
    case 'r':
      repetitions = atoi(optarg);
      break;
    case 'p':
      pin = 1;
      break;
    default:
      printf("you smoke heavy!\nDon't you?\n");
    }
  }

  printf("number of threads: %d, memory: %lld, repetions : %d and pin: %d\n", number_of_threads, memory, repetitions, pin);
  
  pthread_t threads[number_of_threads];

#ifdef LOC_HWCS
  int cores[20] = {0, 48, 4, 52, 8, 56, 12, 60, 16, 64, 20, 68, 24, 72, 28, 76, 32, 80, 36, 84};
#elif LOC_CORES
  int cores[20] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76};
#else
  int cores[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
#endif

  if (pin) {
    printf("I'm about to pin!\n");
    cpu_set_t st;
    CPU_ZERO(&st);
    for (int i = 0; i < 1; ++i) {
      CPU_SET(cores[i], &st);
    }

    sched_setaffinity(0,  sizeof(cpu_set_t), &st);
  }

  printf("The number of threads is: %d\n", number_of_threads);
  int cnt = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    inc_dt* dt = malloc(sizeof(inc_dt));
    dt->size = memory;
    dt->repetions = repetitions;
    dt->numa_node= i % 4;

    
    if (pthread_create(&threads[i], NULL, inc, dt)) {
      fprintf(stderr, "Error creating thread\n");
      return EXIT_FAILURE;
    }

    if (pin) {
      printf("I'mm about to pin!\n");
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(cores[i], &st);
      printf("Core: %d going to node: %d\n", cores[i], dt->numa_node);
      if (pthread_setaffinity_np(threads[i], sizeof(st), &st)) {
	perror("Something went wrong while setting the affinity!\n");
	return EXIT_FAILURE;
      }
    }
    
  }

  for (int i = 0; i < number_of_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}

