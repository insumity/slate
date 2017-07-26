/* Microbenchmark where half of the threads are reading and half of the others are writing in a shared array that fits in LLC. */


#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

#define ATOMIC_INTS_PER_CACHE_LINE 16
#define MAX_NUM_THREADS 48
#define KB 1024
#define MB (1024 * KB)

atomic_int shared_memory[16 * MB];

volatile int stop_thread[MAX_NUM_THREADS * ATOMIC_INTS_PER_CACHE_LINE];


void *writer(void *v)
{
  long long int loops = 0;
  int index = *((int *) v);

  while (stop_thread[index * ATOMIC_INTS_PER_CACHE_LINE] == 0) {
    for (int i = 0; i < MB; ++i) {
      atomic_fetch_add(&shared_memory[i * ATOMIC_INTS_PER_CACHE_LINE], 1);
    }
    loops++;
  }
  
  long long int* loops_pt = malloc(sizeof(long long int));
  *loops_pt = loops;
  return loops_pt;
}

void *reader(void *v)
{
  long sum = 0;
  long long int loops = 0;
  int index = *((int *) v);

  while (stop_thread[index * ATOMIC_INTS_PER_CACHE_LINE] == 0) {
    for (int i = 0; i < MB; ++i) {
      sum += shared_memory[i * ATOMIC_INTS_PER_CACHE_LINE];
    }
    loops++;
  }
  printf("%ld\n", sum);

  long long int* loops_pt = malloc(sizeof(long long int));
  *loops_pt = loops;
  return loops_pt;
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
      printf("Use t, s, e, and p as option!\n");
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
    int* pa = malloc(sizeof(int));
    *pa = i;

    
    if (i % 2 == 0) {
      if (pthread_create(&threads[i], NULL, writer, pa)) {
	fprintf(stderr, "Error creating thread\n");
	return EXIT_FAILURE;
      }
    }
    else {
      if (pthread_create(&threads[i], NULL, reader, pa)) {
	fprintf(stderr, "Error creating thread\n");
	return EXIT_FAILURE;
      }
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

