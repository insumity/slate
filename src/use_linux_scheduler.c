#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <stdint.h>

#include "slate_utils.h"

typedef struct {
  int program;
  int time;
  hw_counters* cnts;
} calculate_data;

typedef struct {
  char **program;
  char* id;
  FILE* fp;
  int start_time_ms;
} thread_args;

void* execute_process(void* arg)
{
  printf("I'm running by : %ld\n", syscall(SYS_gettid));
  thread_args* a = (thread_args *) arg;
  char **program = a->program;
  char* id = a->id;
  FILE* fp = a->fp;
  struct timespec start, end;

  usleep(a->start_time_ms * 1000); // important to sleep before getting start time
  clock_gettime(CLOCK_MONOTONIC, &start);

  pid_t pid = fork();
  if (pid == 0) {
    execv(program[0], program); // SPPEND TWO HOURS ON this, if you use execve instead with envp = {NULL} it doesn't work
    perror("Couldn't run execv\n");
  }

  
  uint64_t INSTRUCTIONS = 0x00C0;
  uint64_t CYCLES = 0x013C;
  uint64_t CACHE_LLC_HITS = 0x4F2E;
  uint64_t CACHE_LLC_MISSES = 0x412E;
  uint64_t events[4] = {INSTRUCTIONS, CYCLES, CACHE_LLC_HITS, CACHE_LLC_MISSES};
  hw_counters* counters = create_counters(pid, 4, events);
  start_counters(*counters);
  


  calculate_data* cd = malloc(sizeof(calculate_data));
  cd->program = atoi(a->id);
  cd->time = 100;
  cd->cnts = counters;
  //pthread_t foo;
  //pthread_create(&foo, NULL, calculate_IPC_every, cd);

  int status;
  waitpid(pid, &status, 0);

  clock_gettime(CLOCK_MONOTONIC, &end);

  long long results[4];
  read_counters(*counters, results);
  long long int instructions = results[0];
  long long int cycles = results[1];
  long long ll_cache_read_accesses = results[2];
  long long ll_cache_read_misses = results[3];

  double *elapsed = malloc(sizeof(double));
  *elapsed = (end.tv_sec - start.tv_sec);
  *elapsed += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
  fprintf(fp, "%s\t%ld\t%lf\tprogram\t" \
	  "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n",
	  id, (long) syscall(SYS_gettid), *elapsed,
	  instructions, cycles, (((double) instructions) / cycles), ll_cache_read_accesses, ll_cache_read_misses,
	  1 - ((double) ll_cache_read_misses / (ll_cache_read_misses + ll_cache_read_accesses)));
	  

  return elapsed;
}


int main(int argc, char* argv[])
{
  if (argc != 3) {
    fprintf(stderr, "./use_linux_scheduler input_file output_file");
    return EXIT_FAILURE;
  }
  FILE* fp = fopen(argv[1], "r");
  FILE* outfp = fopen(argv[2], "w");
  char line[1000];
  pthread_t threads[100]; // up to 100 applications
  int programs = 0;
  
  while (fgets(line, 1000, fp)) {
    if (line[0] == '#') {
      continue; // the line is commented
    }

    read_line_output result = read_line(line);
    int num_id = result.num_id;
    int start_time_ms = result.start_time_ms;
    //char* policy = result.policy;
    char** program = result.program;
    line[0] = '\0';

    thread_args * args = malloc(sizeof(thread_args));
    args->program = program;
    args->id = malloc(sizeof(char) * 100);
    args->fp = outfp;
    args->start_time_ms = start_time_ms;
    sprintf(args->id, "%d", num_id);

    struct timeval tm;
    gettimeofday(&tm, NULL);
    printf("time: %lu\n", tm.tv_usec);

    pthread_create(&threads[programs], NULL, execute_process, args);
    programs++;
  }

  int i;
  for (i = 0; i < programs; ++i) {
    void* result;
    pthread_join(threads[i], &result);
  }


  fclose(fp);
  fclose(outfp);

  return 0;
}

