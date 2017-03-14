#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <numa.h>

void *inc(void *index_pt)
{
  int SIZE = 2.65 * 1024 * 1024;
  int SIZE2 = 1.65* 1024 * 1024;
  int *y, *y2;
  //y2 = numa_alloc_onnode(sizeof(int) * SIZE2, *((int *) index_pt));
  y2 = malloc(sizeof(int) * SIZE2);
  y = numa_alloc_onnode(sizeof(int) * SIZE, *((int *) index_pt));

  for (int k = 0; k < 100; ++k) {
    for (int j = 1; j < SIZE; ++j) {
      y[j] = 34 * 124 * j + y[j] + y[j - 1] + (j > 2? y[j - 2]: 0);
    }

    for (int j = 0; j < SIZE2; ++j) {
      y2[j] = 29 + 17 * 23 * j + y2[j] + (j > 2? y2[j - 2]: 0);
    }
  }

  int index = *((int *) index_pt);
  int loops = 0, times = 0;

  long sum = 0;
  for (int k = 0; k < 500; ++k) {
    for (int j = 5; j < SIZE; ++j) {
      sum += y[j] + y[j - 1] + y[j - 2] + y[j - 3] + y[j - 4] + y[j - 5];
    }
    for (int j = 3; j < SIZE2; ++j) {
      sum += y2[j] + y2[j - 1] + y2[j - 2] + y2[j - 3];
    }
    sum %= 100;
  }

  printf("%ld\n", sum);

  return NULL;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));
  int number_of_threads = atoi(argv[1]);

  bool pin = false;
  if (argc >= 3) {
    pin = true;
  }

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
    int* pa = malloc(sizeof(int));

    *pa = (i + 1) % 4;
    //*pa = i % 4;
    
    if (pthread_create(&threads[i], NULL, inc, pa)) {
      fprintf(stderr, "Error creating thread\n");
      return EXIT_FAILURE;
    }

    if (pin) {
      printf("I'mm about to pin!\n");
      cpu_set_t st;
      CPU_ZERO(&st);
      CPU_SET(cores[i], &st);
      printf("Core: %d going to node: %d\n", cores[i], *pa);
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

