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
  int SIZE = 500 * 1024 * 1024;

  int* foo = malloc(sizeof(int) * SIZE);

  long sum = 0;
  for (int k = 0; k < 10; ++k) {
    for (int j = 1; j < SIZE; ++j) {
      sum += j * 324 + 134 + foo[j - 1];
      foo[j] = sum * 3;
    }
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

