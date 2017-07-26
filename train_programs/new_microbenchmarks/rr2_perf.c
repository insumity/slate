
/* Microbenchmark to overuse the LLC. Each thread spins 5MB ... meaning you'll start getting a lot of LLC misses at around 6 threads in a socket,
   hence a RR approach would be beneficial.  */

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

#define MEMORY_PER_THREAD (5 * MB)

#define ATOMIC_INTS_PER_CACHE_LINE 16
#define MAX_NUMBER_OF_THREADS 48

const int KB = 1024;
const long long int MB = 1024 * 1024;
const long long int GB = 1024 * 1024 * 1024;

volatile int stop_thread[MAX_NUMBER_OF_THREADS * ATOMIC_INTS_PER_CACHE_LINE];


typedef struct {
  int repetions;
  int numa_node;
  
  int index;
} inc_dt;

void *inc(void *index_pt)
{
  long long int loops = 0;

  sleep(3);
  inc_dt* dt = (inc_dt *) index_pt;
  long long size = MEMORY_PER_THREAD;
  int repetions = 1000 * 1000 * 500;
  int numa_node = dt->numa_node;
  int index = dt->index;

  
  char *y = numa_alloc_onnode(size, numa_node);
  bzero(y, size);

  char sum = 0;

  /* const long long int n_warmup = size >> 2; */
  /* for (long long int i = 0; i < n_warmup; i++) { */
  /*   sum = y[i]; */
  /* } */

  for (long long int k = 0; k < repetions; ++k) {
    for (long long int j = 0; j < size / 64; ++j) {
      sum = y[j * 64];
      //y[j] = 0XFF;
    }
    loops++;
    if (loops % 10000 == 0 && stop_thread[index * ATOMIC_INTS_PER_CACHE_LINE] == 1) {
      long long int* loops_pt = malloc(sizeof(long long int));
      *loops_pt = loops;
      return loops_pt;
    }

  }

  fprintf(stderr, "Never reaches this point!\n");

  /* for (long long int k = 0; k < repetions; ++k) { */
  /*   for (long long int j = 0; j < size; ++j) { */
  /*     y[j] = 0xFF; */
  /*   } */
  /* } */

  /* for (long long int i = 0; i < n_warmup; i++) { */
  /*   sum = y[i]; */
  /* } */
  
  printf("%c\n", sum);

  return NULL;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));

  int oc;

  int number_of_threads, pin, pol, execution_time_in_sec;
  pin = 0;
  while ((oc = getopt(argc, argv, "t:s:e:p")) != -1) {
    switch (oc) {
    case 't':
      number_of_threads = atoi(optarg);
      break;
    case 's':
      printf("P input: (%s)\n", optarg);
      pol = atoi(optarg);
      break;
    case 'e':
      execution_time_in_sec = atoi(optarg);
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
    CPU_SET(cores[0], &st);
    sched_setaffinity(0,  sizeof(cpu_set_t), &st);
  }

  printf("The number of threads is: %d\n", number_of_threads);
  printf("Used cores are:\n");
  int cnt = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    inc_dt* dt = malloc(sizeof(inc_dt));

    if (pthread_create(&threads[i], NULL, inc, dt)) {
      fprintf(stderr, "Error creating thread\n");
      return EXIT_FAILURE;
    }

    if (pin) {
      fprintf(stderr, "I'm about to pin!\n");
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(cores[i], &st);
      dt->numa_node = cores[i] % 4;
      dt->index = i;
      printf("%d ", cores[i]);
      fprintf(stderr, "Core: %d going to node: %d\n", cores[i], dt->numa_node);
      if (pthread_setaffinity_np(threads[i], sizeof(st), &st)) {
	perror("Something went wrong while setting the affinity!\n");
	return EXIT_FAILURE;
      }
    }
  }
  printf("\n");


  sleep(execution_time_in_sec);
  for (int i = 0; i < number_of_threads; ++i) {
    stop_thread[i * ATOMIC_INTS_PER_CACHE_LINE] = 1;
  }

  long long total_loops = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    void** retval = malloc(sizeof(void *));
    *retval = malloc(sizeof(long long int));
    pthread_join(threads[i], retval);
    total_loops += *((long long int *) *retval);
  }

  //fprintf(stderr, "Almost there ...\n");

  char filename[100];
  sprintf(filename, "total_loops_threads_%d_policy_%d_pin_%d_execution_time_%d", number_of_threads, pol, pin, execution_time_in_sec);
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    perror("couldn't open the file\n");
  }
  fprintf(fp, "Loops per thread\n%.2lf\n", total_loops / (double) number_of_threads);
  fclose(fp);

  return 0;
}

