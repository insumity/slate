#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <numaif.h>
#include <numa.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE (1024 * 1024 * 1000)

typedef struct {
  int id;
  char* memory;
} do_stuff_data;
  
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

  char f[1] = { (char) id + 'a' };
  char filename[200];
  filename[0] = '\0';
  strcat(filename, "/tmp/foo");
  strcat(filename, f);
  
  int fd = open(filename, O_RDWR);
  char buf[1];
  buf[0] = 'A';
  write(fd, buf, 1);
  fsync(fd);
  close(fd);
  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 1;


  for (int i = 0; i < 1024 * 512 * 1000; ++i) {

  //  for (int i = 0; i < 1024 * 512 * 300; ++i) {
    int k = 234234 % (SIZE / 10);
    int y = k * 4 % (SIZE / 10);
    memory[y] = (char) (memory[k] + 2);
    int* f = malloc(sizeof(int) * 10);
    for (int j = 0; j < 10; ++j) {
      f[j] = 0;
    }
    /* if(i % 1866 == 0 && nanosleep(&tim , &tim2) < 0 ) */
    /*   { */
    /* 	printf("Nano sleep system call failed \n"); */
    /* 	return -1; */
    /*   } */
    sum += memory[k];
  }

  
  long* result = malloc(sizeof(long));
  *result = sum;
  return result;
}


int main(int argc, char* argv[]) {
  srand(time(NULL));

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

