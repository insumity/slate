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
#include <papi.h>

#include "slate_utils.h"

#define PAPI_ENABLED
//#define PAPI_MULTIPLEX


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

  #ifdef PAPI_ENABLED
  int retval;
  int event_set = PAPI_NULL;
  retval = PAPI_create_eventset(&event_set);
  retval = PAPI_assign_eventset_component(event_set, 0);


#ifdef PAPI_MULTIPLEX
  retval = PAPI_set_multiplex(event_set);
  if (retval != PAPI_OK) {
    perror("couldn't set multiplexing\n");
    exit(1);
  }
#endif
  
  PAPI_option_t opt;
  memset(&opt, 0x0, sizeof (PAPI_option_t) );
  opt.inherit.inherit = PAPI_INHERIT_ALL;
  opt.inherit.eventset = event_set;
  if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK) {
    perror("no!\n");
  }
  
  //retval = PAPI_add_event(event_set, PAPI_TOT_INS);
  //retval = PAPI_add_event(event_set, PAPI_TOT_CYC);
  retval = PAPI_add_event(event_set, PAPI_L3_TCA);
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCA\n");
    exit(1);
  }
  retval = PAPI_add_event(event_set, PAPI_L3_TCM);
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCM\n");
    exit(1);
  }

  if (PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:LOCAL_DRAM") != PAPI_OK ) {
    printf("Something wrong is going here\n");
    exit(1);
  }

  if (PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM") != PAPI_OK ) {
    printf("Something wrong is going here\n");
    exit(1);
  }
#endif

  pid_t pid = fork();
  if (pid == 0) {
    execv(program[0], program); // SPPEND TWO HOURS ON this, if you use execve instead with envp = {NULL} it doesn't work
    perror("Couldn't run execv\n");
  }

  #ifdef PAPI_ENABLED
  retval = PAPI_start(event_set);
  retval = PAPI_attach(event_set, ( unsigned long ) pid);
#endif

  
  int status;
  waitpid(pid, &status, 0);

  clock_gettime(CLOCK_MONOTONIC, &end);
long long results[5] = {0};
#ifdef PAPI_ENABLED
    retval = PAPI_stop(event_set, results);
    retval = PAPI_cleanup_eventset(event_set);
    retval = PAPI_destroy_eventset(&event_set);
#endif

    long long ll_cache_read_accesses = 1;
    long long ll_cache_read_misses = 2;
    long long retired_local_dram = 0;
    long long retired_remote_dram = 3;
#ifdef PAPI_ENABLED
    ll_cache_read_accesses = results[0];
    ll_cache_read_misses = results[1];
    retired_local_dram = results[2];
    retired_remote_dram = results[3];
#endif


  double *elapsed = malloc(sizeof(double));
  *elapsed = (end.tv_sec - start.tv_sec);
  *elapsed += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
  fprintf(fp, "%s\t%ld\t%lf\tprogram\t" \
	  "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n",
	  id, (long) syscall(SYS_gettid), *elapsed,
ll_cache_read_accesses, ll_cache_read_misses,
	  ((double) ll_cache_read_misses / (ll_cache_read_accesses)),
	  	    retired_local_dram, retired_remote_dram, retired_local_dram / ((double) retired_remote_dram + retired_local_dram)
	  );
	  

  return elapsed;
}


int main(int argc, char* argv[])
{
#ifdef PAPI_ENABLED
  // papi stuff
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    perror("couldn't initialize library\n");
    exit(1);
  }

#ifdef PAPI_MULTIPLEX
  retval = PAPI_multiplex_init();
  if (retval != PAPI_OK) {
    perror("couldn't enable multiplexing\n");
    exit(1);
  }
#endif
  
#endif
  


  
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
    printf("num: %d, start_time: %d, progr: %d\n", num_id, start_time_ms, program);
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

