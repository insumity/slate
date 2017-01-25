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
#include <sys/types.h>
#include <sys/wait.h>


//#define PAPI_ENABLED
//#define PAPI_MULTIPLEX

typedef struct {
  char** dt;
  int length;
} execute_process_data;

void* execute_process(void* args) {
  int retval;
  int event_set = PAPI_NULL;

#ifdef PAPI_ENABLED
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
    exit(1);
  }

  
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
    printf("Something wrong is going here normal\n");
    exit(1);
  }

  if (PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM") != PAPI_OK ) {
    perror("Something wrong is going here normal2\n");
    exit(1);
  }

#ifdef PAPI_MULTIPLEX
  retval = PAPI_add_named_event(event_set, "L2_TRANS:RFO");
  if (retval != PAPI_OK) {
    perror("1) eat shit ..\n");
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "L2_TRANS:CODE_RD");
  if (retval != PAPI_OK) {
    perror("2) eat shit ..\n");
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "L2_TRANS:ALL_PF");
  if (retval != PAPI_OK) {
    perror("3) eat shit ..\n");
    exit(1);
  }
  
  retval = PAPI_add_named_event(event_set, "L2_TRANS:L1D_WB");
  if (retval != PAPI_OK) {
    perror("4) eat shit ..\n");
    exit(1);
  }
#endif

#endif
  
  struct timespec *start = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, start);
  pid_t pid = fork();
  if (pid == 0) {
    execute_process_data* args2 = (execute_process_data *) args;
    char** foo = args2->dt;

    char* program[3];
    program[0] = foo[1];
    program[1] = foo[2];
    program[2] = NULL;
    execv(program[0], program);
    printf("== %s\n", program[0]);
    perror("execve didn't work");
    exit(1);
  }
  
  long long results[8] = {1, 2, 3, 4, 5, 6, 7, 8};
#ifdef PAPI_ENABLED
  retval = PAPI_attach(event_set, ( unsigned long ) pid);
  retval = PAPI_start(event_set);

  usleep(500 * 1000); // wait 100ms

  retval = PAPI_stop(event_set, results);
  printf("1) TCA: %lld TCM: %lld LOCAL: %lld REMOTE: %lld\n", results[0], results[1], results[2], results[3]);
  printf("2) l1dcm: %lld l1icm: %lld l2tcm: %lld totcyc: %lld\n", results[4], results[5], results[6], results[7]);


  printf(" .... \n");
  for (int i = 0; i < 8; ++i) {
    results[i] = 0;
  }
  if (retval != PAPI_OK) {  
    printf("HERE ... eat shit: %d\n", retval);
  }
  usleep(1000 * 1000);

  retval = PAPI_start(event_set);
  usleep(500 * 1000);
  retval = PAPI_stop(event_set, results);
  if (retval != PAPI_OK) {
    printf("XXXX eat shit!!!\n");
  }
  
#endif
  
  int status;
  waitpid(pid, &status, 0);



  struct timespec *finish = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);
  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;

  printf("The process just finished.\n");
  fprintf(stderr, "%lf\n", elapsed);

  printf("1) TCA: %lld TCM: %lld LOCAL: %lld REMOTE: %lld\n", results[0], results[1], results[2], results[3]);
  printf("2) l1dcm: %lld l1icm: %lld l2tcm: %lld totcyc: %lld\n", results[4], results[5], results[6], results[7]);


#ifdef PAPI_ENABLED
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

      fprintf(stdout, "%lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", 
	      ll_cache_read_accesses, ll_cache_read_misses,
	      ((double) ll_cache_read_misses / ll_cache_read_accesses),
	      retired_local_dram, retired_remote_dram, retired_local_dram / ((double) retired_remote_dram + retired_local_dram)
	      );
	  
  
  return NULL;
}



int main(int argc, char* argv[]) {

#ifdef  PAPI_ENABLED
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    perror("couldn't initialize library\n");
    exit(1);
  }

#ifdef PAPI_MULTIPLEX
  printf("PAPI multiplexing baby\n");
  retval = PAPI_multiplex_init();
  if (retval != PAPI_OK) {
    perror("couldn't enable multiplexing\n");
    exit(1);
  }
#endif
#endif


  execute_process_data data;
  data.dt = malloc(sizeof(char *) * 3);
  for (int i = 0; i < 3; ++i) {
    data.dt[i] = argv[0];
  }

  pthread_t thread;
  pthread_create(&thread, NULL, execute_process, &argv);
  pthread_join(thread, NULL);
  
  return 0;
}

