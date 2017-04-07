#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

#define ATOMIC_INTS_PER_CACHE_LINE 16

int number_of_counters = 1;
atomic_int counters[1 * ATOMIC_INTS_PER_CACHE_LINE];

int every_loop = 1;

void *inc(void *v)
{
  int loops = 0;
  while (true) {
    if (loops % every_loop == 0) {
      atomic_fetch_add(&counters[0], 1);
    }
    loops++;
  }
}

int main(int argc, char* argv[])
{
  srand(time(NULL));

  int oc;

  int number_of_threads, pin, pol;
  pin = 0;
  while ((oc = getopt(argc, argv, "t:s:l:p")) != -1) {
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
    case 'l':
      every_loop = atoi(optarg);
      printf("Every loop (l) is now: %d\n", every_loop);
      break;
    default:
      printf("Use t, s, or p as option!\n");
    }
  }

  pthread_t threads[number_of_threads];
  //int cores_loc_hwcs[20] = {0, 48, 4, 52, 8, 56, 12, 60, 16, 64, 20, 68, 24, 72, 28, 76, 32, 80, 36, 84};

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
    if (pthread_create(&threads[i], NULL, inc, pa)) {
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

