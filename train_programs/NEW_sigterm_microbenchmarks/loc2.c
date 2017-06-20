/* Microbenchmark where half of the threads are reading and half of the others are writing in a shared array that fits in LLC. */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

#define ATOMIC_INTS_PER_CACHE_LINE 16
#define MAX_NUM_THREADS 48
#define KB 1024
#define MB (1024 * KB)

atomic_int shared_memory[16 * (256 * KB)];


int number_of_threads, pin, pol;
#define MAX_NUMBER_OF_THREADS 50

#define LONG_LONGS_PER_CACHE_LINE 8
long long int loops_per_thread[LONG_LONGS_PER_CACHE_LINE * MAX_NUMBER_OF_THREADS];


void *writer(void *v)
{
  int index = *((int *) v);
  long long int loops = 0;
  long offset = 1L << 14;

  while (true) {
    for (int i = 0; i < 256 * KB; ++i) {
      atomic_fetch_add(&shared_memory[i * ATOMIC_INTS_PER_CACHE_LINE], 1);
    }
    loops++;

    if (loops & offset == offset) {
      loops_per_thread[LONG_LONGS_PER_CACHE_LINE * index]++;
    }
  }

  return NULL;
}

void sig_handler(int signo)
{
  long long sum = 0;
  if (signo == SIGINT) {
    for (int i = 0; i < number_of_threads; ++i) {
      sum += loops_per_thread[LONG_LONGS_PER_CACHE_LINE * i];
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


void *reader(void *index_pt)
{
  long sum = 0;
  int loops = 0;
  while (true) {
    for (int i = 0; i < 256 * KB; ++i) {
      sum += shared_memory[i * ATOMIC_INTS_PER_CACHE_LINE];
    }
    loops++;
  }
  printf("%ld\n", sum);
}

int main(int argc, char* argv[])
{
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
    int* pa = malloc(sizeof(int));
    *pa = i;

    // splot the readers and the writers in such a way that each socket has both readers and writers
    int mod8 = i % 8;
    if (mod8 == 0 || mod8 == 3 || mod8 == 5 || mod8 == 6) {
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

  for (int i = 0; i < number_of_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}

