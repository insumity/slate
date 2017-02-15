#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <mctop.h>
#include <mctop_alloc.h>
#include <sys/wait.h>
#include <time.h>
#include <numaif.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <numaif.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include "counters.h"

int main(int argc, char* argv[]) {

  uint64_t UNHALTED_CORE_CYCLES = 0x003C;
  uint64_t INSTRUCTIONS_RETIRED = 0x00C0;
  uint64_t CACHE_LLC_REFERENCES = 0x4F2E;
  uint64_t CACHE_LLC_MISSES = 0x412E; 
  uint64_t RETIRED_LOCAL_DRAM = 0x01D3; // MEM_LOAD_UOPS_LLC_MISS_RETIRED.LOCAL_DRAM
  uint64_t RETIRED_REMOTE_DRAM = 0x10D3; // MEM_LOAD_UOPS_LLC_MISS_RETIRED.REMOTE_DRAM

  uint64_t events[4] = {RETIRED_LOCAL_DRAM, RETIRED_REMOTE_DRAM, CACHE_LLC_REFERENCES, CACHE_LLC_MISSES};
  //uint64_t events[4] = {UNHALTED_CORE_CYCLES, INSTRUCTIONS_RETIRED, RETIRED_LOCAL_DRAM, RETIRED_REMOTE_DRAM};

  int COUNTERS = 4;
 
  hw_counters* counters = create_counters(5, COUNTERS, events);

  start_counters(counters);

  pid_t pid = fork();
  char* program[] = {"/bin/sleep", "10", NULL};
  if (pid == 0) {
    execvp(program[0], program);
  }

  int status;
  waitpid(pid, &status, 0);

  long long results[COUNTERS];
  stop_and_read_counters(counters, results);
  for (int i = 0; i < COUNTERS; ++i) {
    printf("%lld\n", results[i]);
  }

  free_counters(counters);
  return 0;
}

