#define _GNU_SOURCE
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

pthread_mutex_t glock;

typedef struct {
  long long values[8];
  pthread_mutex_t lock;
} core_data;


void* foo(void* dt) {
  int cpu = *((int *) dt);
    
  char filename[100] = {'\0'};
  strcpy(filename, "/tmp/cpu");
  char id[100];
  sprintf(id, "%d", cpu);
  strcat(filename, id);
  printf("The filename is: %s\n", filename);
  int fd = open(filename, O_RDWR, 0777);
  
  core_data* data = mmap(0, sizeof(core_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    fprintf(stderr, "foo barisios\n");
  }

  while (true) {
    pthread_mutex_lock(&(glock));

    printf("%d\n", cpu);
    pthread_mutex_lock(&(data->lock));
    for (int i = 0; i < 8; ++i) {
      printf("%d: %lld\n", i, data->values[i]);
    }
    pthread_mutex_unlock(&(data->lock));
    pthread_mutex_unlock(&(glock));


    sleep(2);
  }
}

int main(int argc, char* argv[]) {

  int THREADS = atoi(argv[1]);
  pthread_t threads[THREADS];

  pthread_mutex_init(&glock, NULL);
  for (int i = 0; i < THREADS; ++i) {
    int* n = malloc(sizeof(int));
    *n = i;
    pthread_create(&threads[i], NULL, foo, n);
  }


  for (int i = 0; i < THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }




  return 0;
}

