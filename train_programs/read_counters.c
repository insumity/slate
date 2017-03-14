#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <numaif.h>
#include <papi.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "read_counters.h"

volatile core_data all_cores[96];

static void read_counters_no_lock(int cpu);

core_data get_counters(int cpu) {
  core_data dt = all_cores[cpu];
  core_data result;
  pthread_mutex_lock(&(dt.lock));
  read_counters_no_lock(cpu);
  result = all_cores[cpu];
  pthread_mutex_unlock(&(dt.lock));
  return result;
}

static void read_counters_no_lock(int cpu) {

  char filename[100] = {'\0'};
  strcpy(filename, "/tmp/cpu");
  char id[100];
  sprintf(id, "%d", cpu);
  strcat(filename, id);
  int fd = open(filename, O_RDWR, 0777);
  
  core_data* data = mmap(0, sizeof(core_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("couldn't memory map in no_lock\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < 9; ++i) {
    all_cores[cpu].values[i] = data->values[i];
  }
  close(fd);
}

static void* read_counters_fn(void* dt) {
  int cpu = *(int *) dt;

  char filename[100] = {'\0'};
  strcpy(filename, "/tmp/cpu");
  char id[100];
  sprintf(id, "%d", cpu);
  strcat(filename, id);
  int fd = open(filename, O_RDWR, 0777);
  
  core_data* data = mmap(0, sizeof(core_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("couldn't memory map\n");
    exit(EXIT_FAILURE);
  }

  pthread_mutex_lock(&(data->lock));
  for (int i = 0; i < 9; ++i) {
    all_cores[cpu].values[i] = data->values[i];
  }
  pthread_mutex_unlock(&(data->lock));
  close(fd);

  return NULL;
}

void start_reading() {

  pthread_t th[96];
  for (int i = 0; i < 96; ++i) {
    int* cpu_i = malloc(sizeof(int));
    *cpu_i = i;
    // pthread_create(&th[i], NULL, read_counters_fn, cpu_i);
    read_counters_fn(cpu_i);
  }

  return;
}

/* int main(int argc, char* argv[]) { */
/*   start_reading(); */
/*   sleep(1); */
/*   while (true) { */
/*     printf("%lld\n", get_counters(0).values[1]); */
/*     sleep(1); */
/*   } */
/*   return 0; */
/* } */
