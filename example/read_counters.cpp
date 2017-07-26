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


core_data all_cores[96];

core_data get_counters(int cpu) {
  core_data dt = all_cores[cpu];
  core_data result;
  pthread_mutex_lock(&(dt.lock));
  result = all_cores[cpu];
  pthread_mutex_unlock(&(dt.lock));
  return result;
}

static void* read_counters_fn(void* dt) {

  while (true) {
    for (int cpu = 0; cpu < 96; ++cpu) {
      char filename[100] = {'\0'};
      strcpy(filename, "/tmp/cpu");
      char id[100];
      sprintf(id, "%d", cpu);
      strcat(filename, id);
      int fd = open(filename, O_RDWR, 0777);
  
      core_data* data = (core_data*) mmap(0, sizeof(core_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (data == MAP_FAILED) {
	perror("cannot memory map\n");
	exit(1);
      }

      pthread_mutex_lock(&(data->lock));
      for (int i = 0; i < 9; ++i) {
	all_cores[cpu].values[i] = data->values[i];
      }
      pthread_mutex_unlock(&(data->lock));
      usleep(10 * 1000); // wait 10ms
      close(fd);
    }
  }

  return NULL;
}

void start_reading() {

  pthread_t th;
  pthread_create(&th, NULL, read_counters_fn, NULL);

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
