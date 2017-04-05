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

const int KB = 1024;
const long long int MB = 1024 * 1024;
const long long int GB = 1024 * 1024 * 1024;

typedef struct {
  int repetions;
  int numa_node;
} inc_dt;

void *inc(void *index_pt)
{
  sleep(3);
  inc_dt* dt = (inc_dt *) index_pt;
  long long size = 2 * GB;
  int repetions = dt->repetions;
  int numa_node = dt->numa_node;
  
  char *y = numa_alloc_onnode(sizeof(char) * size, numa_node);
  bzero(y, sizeof(char) * size);

  char sum = 0;

  const long long int n_warmup = size >> 2;
  for (long long int i = 0; i < n_warmup; i++) {
    sum = y[i];
  }
  
  for (long long int k = 0; k < repetions; ++k) {
    for (long long int j = 0; j < size; ++j) {
      sum = y[j];
    }
  }

  for (long long int k = 0; k < repetions; ++k) {
    for (long long int j = 0; j < size; ++j) {
      y[j] = 0xFF;
    }
  }

  for (long long int i = 0; i < n_warmup; i++) {
    sum = y[i];
  }
  
  printf("%c\n", sum);

  return NULL;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));

  int oc;

  int number_of_threads, repetitions, pin, pol;
  pin = 0;
  while ((oc = getopt(argc, argv, "t:r:s:p")) != -1) {
    switch (oc) {
    case 't':
      number_of_threads = atoi(optarg);
      break;
    case 'r':
      repetitions = atoi(optarg);
      break;
    case 's':
      printf("P input: (%s)\n", optarg);
      pol = atoi(optarg);
      break;
    case 'p':
      pin = 1;
      break;
    default:
      printf("Use t, r, s, or p as options!\n");
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
    for (int i = 0; i < 1; ++i) {
      CPU_SET(cores[i], &st);
    }

    sched_setaffinity(0,  sizeof(cpu_set_t), &st);
  }

  printf("The number of threads is: %d\n", number_of_threads);
  printf("Used cores are:\n");
  int cnt = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    inc_dt* dt = malloc(sizeof(inc_dt));
    dt->repetions = repetitions;

    if (pthread_create(&threads[i], NULL, inc, dt)) {
      fprintf(stderr, "Error creating thread\n");
      return EXIT_FAILURE;
    }

    if (pin) {
      fprintf(stderr, "I'm about to pin!\n");
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(cores[i], &st);
      dt->numa_node= cores[i] % 4;
      printf("%d ", cores[i]);
      fprintf(stderr, "Core: %d going to node: %d\n", cores[i], dt->numa_node);
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

