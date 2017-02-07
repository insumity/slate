#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <numaif.h>
#include <numa.h>

#define SIZE (1024 * 1024)

typedef struct {
  int id;
  char* memory;
} do_stuff_data;

char* shared_memory;
  
void* do_stuff(void* args) {
  do_stuff_data* dt = (do_stuff_data *) args;
  int id = dt->id;
  char* memory = dt->memory;
  cpu_set_t st;
  CPU_ZERO(&st);
  CPU_SET(id, &st);
  if (sched_setaffinity(0, sizeof(cpu_set_t),  &st) != 0) {
    printf("couldn't set it ...\n");
  }
  long long sum = 0;

  for (int k = 0; k < 1; ++k) {
    for (int i = 0; i < SIZE; ++i) {
      memory[i] = (char) (rand() % 789 + memory[SIZE - i - 1]);
      sum += memory[i] + ((i > 1024 * 512)? memory[i - 1024 * 512]: 0);
      shared_memory[i] = (sum * 2 + 993) % 23;
      sum = sum + (i > 0? shared_memory[i - 1]: 0);

      if (i % 100 == 0) {
	usleep(1);
      }
    }
    usleep(2);
  }
  
  long* result = malloc(sizeof(long));
  *result = sum;
  return result;
}

int main(int argc, char* argv[]) {
  srand(time(NULL));

  shared_memory = numa_alloc_onnode(sizeof(volatile char) * SIZE, 0);

  int number_of_threads = atoi(argv[1]);
  pthread_t* threads = malloc(sizeof(pthread_t) * number_of_threads);

  for (int i = 0; i < number_of_threads; ++i) {
    do_stuff_data* dt = malloc(sizeof(do_stuff_data));
    dt->id = i;
    dt->memory = numa_alloc_onnode(sizeof(volatile char) * SIZE, i % 4);
    pthread_create(&threads[i], NULL, do_stuff, dt);
  }

  int i;
  long sum = 0;
  for (i = 0; i < number_of_threads; ++i) {
    void* result;
    pthread_join(threads[i], &result);
    sum += *((long *) result);
  }

  return (int) sum;
}

