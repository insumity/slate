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
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>

void* foo_fn(void *x) {
  long long sum = 0;

  for (int j = 0; j < 5000; ++j) {
    for (int i = 0; i < 1000000; ++i) {
      long x = syscall(SYS_gettid);
      sum += x;
      if (i % 1000 == 0) {
	int*x = malloc(sizeof(4));
	*x = 0;
      }
    }
  }
  
  printf("%lld\n", sum);
  return NULL;
}

int main(int argc, char* argv[]) {

  pthread_t foo[5];

  for (int i = 0; i < 5; ++i) {
    pthread_create(&foo[i], NULL, &foo_fn, NULL);
  }

  for (int i = 0; i < 5; ++i) {
    pthread_join(foo[i], NULL);
  }

  return 0;
}

