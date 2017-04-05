#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

volatile int x[100];
int time_before_exit;

void *inc(void *index_pt)
{
  int index = *((int *) index_pt);
  struct timespec start, finish;
  double elapsed;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int loops = 0, times = 0;
  while (true) {
    x[loops % 20]++;
    loops++;
    if (loops % 100000000 == 0) {
      times++;
      loops = 0;
      clock_gettime(CLOCK_MONOTONIC, &finish);
      elapsed = (finish.tv_sec - start.tv_sec);

      /* if (elapsed >= time_before_exit) { */
      /* 	printf("elapsed: %lf\n", elapsed); */
      /* 	return NULL; */
      /* } */
      if (times == 2) {
	return NULL;
      }
    }
  }
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
  int cores[33] = {0, 48, 4, 52, 8, 56, 12, 60, 16, 64, 20, 68, 24, 72, 28, 76, 32, 80, 36, 84};
#elif LOC_CORES
  int cores[33] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76};
#else
  int cores[33];
  for (int i = 0; i < 33; ++i) {
    cores[i] = i;
  }
#endif

  if (pin) {
    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(cores[0], &st);
    sched_setaffinity(0,  sizeof(cpu_set_t), &st);
  }

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
      printf("Core: %d\n", cores[i]);
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

