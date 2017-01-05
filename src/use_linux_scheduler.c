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
  hw_counters_fd* cnts;
} calculate_data;

void* calculate_IPC_every(void* time_in_ms)
{
  calculate_data* dt = (calculate_data *) time_in_ms;
  int program = dt->program;

  char program_str[30];
  sprintf(program_str, "%d", program);
  
  FILE* fp = fopen(program_str, "w");
  
  int every = dt->time;
  hw_counters_fd* fd = dt->cnts;
  
  long instructions = 0;
  long cycles = 0;

  int i = 0;
  while (1) {
    usleep(every * 1000);

    long long int new_instructions = read_perf_counter(fd->instructions);
    long long int new_cycles = read_perf_counter(fd->cycles);

    long long inst = new_instructions - instructions;
    long long cycl = new_cycles - cycles;
    fprintf(fp, "%d\t%d\t%lf\n", i, program, inst / ((double) cycl));
    fflush(fp);
    i++;
    
    instructions = new_instructions;
    cycles = new_cycles;
  }
}

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

  hw_counters_fd* cnt2 = malloc(sizeof(hw_counters_fd));

  pid_t pid = fork();
  if (pid == 0) {
    execv(program[0], program); // SPPEND TWO HOURS ON this, if you use execve instead with envp = {NULL} it doesn't work
    perror("Couldn't run execv\n");
  }
  cnt2->instructions = open_perf(pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
  cnt2->cycles = open_perf(pid, PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES);
  cnt2->ll_cache_read_accesses = open_perf(pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, ACCESS));
  cnt2->ll_cache_read_misses = open_perf(pid, PERF_TYPE_HW_CACHE, CACHE_EVENT(LL, READ, MISS));

  start_perf_reading(cnt2->instructions);
  start_perf_reading(cnt2->cycles);
  start_perf_reading(cnt2->ll_cache_read_accesses);
  start_perf_reading(cnt2->ll_cache_read_misses);


  calculate_data* cd = malloc(sizeof(calculate_data));
  cd->program = atoi(a->id);
  cd->time = 100;
  cd->cnts = cnt2;
  //pthread_t foo;
  //pthread_create(&foo, NULL, calculate_IPC_every, cd);

  int status;
  waitpid(pid, &status, 0);

  clock_gettime(CLOCK_MONOTONIC, &end);

  long long int instructions = read_perf_counter(cnt2->instructions);
  long long int cycles = read_perf_counter(cnt2->cycles);
  long long ll_cache_read_accesses = read_perf_counter(cnt2->ll_cache_read_accesses);
  long long ll_cache_read_misses = read_perf_counter(cnt2->ll_cache_read_misses);

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
  char line[300];
  pthread_t threads[100]; // up to 100 applications
  int programs = 0;
  
  while (fgets(line, 300, fp)) {
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

