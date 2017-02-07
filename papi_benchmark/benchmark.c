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
#include <sys/wait.h>

//#define B_PAPI_ENABLED
//#define B_PAPI_MULTIPLEX
//#define B_READ_FOR 100
//#define B_SLEEP_FOR 400

typedef struct {
  char** dt;
  int length;
} execute_process_data;


int compare (const void * a, const void * b)
{
  return ( *(long long*)a - *(long long*)b );
}

int event_set = PAPI_NULL;
void* execute_process(void* args) {
  int retval;

  
#ifdef B_PAPI_ENABLED
  retval = PAPI_add_event(event_set, PAPI_TOT_CYC);
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCA\n");
    exit(1);
    }
    
  retval = PAPI_add_event(event_set, PAPI_TOT_INS);
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCM\n");
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:LOCAL_DRAM");
  if (retval != PAPI_OK ) {
    printf("couldn't add local dram event: %d\n", retval);
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM");
  if (retval != PAPI_OK ) {
    printf("couldn't add remote dram event: %d\n", retval);
    exit(1);
  }
#endif
  
#ifdef B_PAPI_MULTIPLEX
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

  retval = PAPI_add_named_event(event_set, "ICACHE:MISSES");
  if (retval != PAPI_OK) {
    perror("3) eat shit ..\n");
    exit(1);
  }
  
  retval = PAPI_add_event(event_set, PAPI_L1_DCM);
  if (retval != PAPI_OK) {
    perror("4) eat shit ..\n");
    exit(1);
  }
#endif
  
  struct timespec *start = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, start);
  pid_t pid = fork();
  if (pid == 0) {
    execute_process_data* args2 = (execute_process_data *) args;
    char** foo = args2->dt;

    char* program[5];
    for (int i = 0; i < 4; ++i) {
      program[i] = foo[i + 1];
    }
    program[4] = NULL;
    execv(program[0], program);
    printf("== %s\n", program[0]);
    perror("execve didn't work");
    exit(1);
  }
  
  long long results[8] = {0, 0, 0, 0, 0, 0, 0, 0};

#ifdef B_PAPI_ENABLED
  while (kill(pid, 0) == -1) {
    printf("Cannot kill shit!\n");
    usleep(20);
  }
  retval = PAPI_attach(event_set, (unsigned long) pid);

  bool first = true;
  //retval = PAPI_start(event_set);
  //if (retval != PAPI_OK) {
  //printf("The event hasn't started yet!\n");
  //}


  long long local = 0, remote = 0;
  //long long local_values[100000], remote_values[100000];
  int times = 0;
  while (true) {

#ifdef B_MANY
    retval = PAPI_reset(event_set);
    if (retval != PAPI_OK) {
      printf("The event hasn't started yet!\n");
    }

    retval = PAPI_start(event_set);
    if (retval != PAPI_OK) {
      printf("The event hasn't started yet!\n");
    }

    for (int i = 0; i < 8; ++i) {
      results[i] = 0;
    }
#endif

    usleep(B_READ_FOR * 1000);

#ifdef B_MANY
    retval = PAPI_stop(event_set, results);
    if (retval != PAPI_OK) {
      printf("The event hasn't started yet!\n");
    }
    printf("X 1) TCA:  \t %lld TCM:  \t %lld LOCAL:\t %lld REMOTE:\t %lld\n", results[0], results[1], results[2], results[3]);
    printf("X 2) l1dcm:\t %lld l1icm:\t %lld l2tcm:\t %lld totcyc:\t %lld\n", results[4], results[5], results[6], results[7]);
    /*local += results[2];
    local_values[times] = results[2];
    remote += results[3];
    remote_values[times] = results[3];
    times++;*/
#endif
    
    usleep(B_SLEEP_FOR * 1000);
    {
      int status;
      if (waitpid(pid, &status, WNOHANG) != 0) {
	break;
      }
    }

  }
#endif
  
  int status;
  waitpid(pid, &status, 0);

#ifdef B_PAPI_ENABLED
  //retval = PAPI_stop(event_set, results);
  printf("X 1) TCA: %lld TCM: %lld LOCAL: %lld REMOTE: %lld\n", results[0], results[1], results[2], results[3]);
  printf("X 2) l1dcm: %lld l1icm: %lld l2tcm: %lld totcyc: %lld\n", results[4], results[5], results[6], results[7]);
  if (retval != PAPI_OK) {
    printf("HERE ... eat shit: %d\n", retval);
    perror("What's happenning here when sto ..\n");
  }
#endif


  struct timespec *finish = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);
  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;

  printf("The process just finished.\n");
  fprintf(stderr, "%lf\n", elapsed);

#ifdef B_PAPI_ENABLED

  long double mean_local = local / ((long double) times);
  long double mean_remote = remote / ((long double) times);

  
  
  long double v_local = 0.0, v_remote = 0.0;
  //qsort(local_values, times, sizeof(long long), &compare);
  //qsort(remote_values, times, sizeof(long long), &compare);

  //printf("%lld %lld %lld\n", local_values[times / 2 - 1], local_values[times / 2], local_values[times/ 2 + 1]);
  //printf("%lld %lld %lld\n", remote_values[times / 2 - 1], remote_values[times / 2], remote_values[times/ 2 + 1]);

  for (int i = 0; i < times; ++i) {
    //printf("%d: %lld %lld\n", i, local_values[i], remote_values[i]);
    //v_local += (local_values[i] - mean_local) * (local_values[i]  - mean_local);
    //v_remote += (remote_values[i] - mean_remote) * (remote_values[i]  - mean_remote);
  }

  /*printf("LOCAL: %lf REMOTE: %lf\n", local / ((double) times), remote / ((double) times));
  printf("LOCAL: %lf REMOTE: %lf\n", sqrt(v_local / ((long double) times)), sqrt(v_remote / ((long double) times)));

  printf("1) TCA: %lld TCM: %lld LOCAL: %lld REMOTE: %lld\n", results[0], results[1], results[2], results[3]);
  printf("2) l1dcm: %lld l1icm: %lld l2tcm: %lld totcyc: %lld\n", results[4], results[5], results[6], results[7]); */
#endif

#ifdef B_PAPI_ENABLED
  retval = PAPI_cleanup_eventset(event_set);
  retval = PAPI_destroy_eventset(&event_set);
#endif

    long long ll_cache_read_accesses = 1;
    long long ll_cache_read_misses = 2;
    long long retired_local_dram = 0;
    long long retired_remote_dram = 3;
#ifdef B_PAPI_ENABLED
      ll_cache_read_accesses = results[0];
      ll_cache_read_misses = results[1];
      retired_local_dram = results[2];
      retired_remote_dram = results[3];
#endif

      fprintf(stdout, "F: %lld\t%lld\t%lf\t%lld\t%lld\t%lf\n", 
	      ll_cache_read_accesses, ll_cache_read_misses,
	      ((double) ll_cache_read_misses / ll_cache_read_accesses),
	      retired_local_dram, retired_remote_dram, retired_local_dram / ((double) retired_remote_dram + retired_local_dram)
	      );
	  
  
  return NULL;
}



int main(int argc, char* argv[]) {

  int retval;
#ifdef B_PAPI_ENABLED
  int retval3 = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval3 != PAPI_VER_CURRENT) {
    perror("couldn't initialize library\n");
    exit(1);
  }
#endif
  /* PAPI_option_t options; */
  /* int num = PAPI_get_opt(PAPI_LIB_VERSION, &options); */
  /* printf("PAPI version: %d\n", num); */

  
#ifdef B_PAPI_MULTIPLEX
  printf("PAPI multiplexing baby\n");
  int retval2 = PAPI_multiplex_init();
  if (retval2 != PAPI_OK) {
    perror("couldn't enable multiplexing\n");
    exit(1);
  }

  /*num = PAPI_get_opt(PAPI_MAX_MPX_CTRS, &options);
  printf("PAPI multiplex counters: %d\n", num);

  num = PAPI_get_opt(PAPI_MULTIPLEX, &options);
  printf("PAPI is it enabled mpx?: %d\n", num); */

#endif

#ifdef B_PAPI_ENABLED
  retval = PAPI_create_eventset(&event_set);
  if (retval != PAPI_OK) {
    perror("couldn't assign the component\n");
    exit(1);
  }

  retval = PAPI_assign_eventset_component(event_set, 0);
  if (retval != PAPI_OK) {
    perror("couldn't assign the component\n");
    exit(1);
  }
#endif
  
#ifdef B_PAPI_MULTIPLEX
  retval = PAPI_set_multiplex(event_set);
  if (retval != PAPI_OK) {
    perror("couldn't set multiplexing\n");
    exit(1);
  }
#endif
  
#ifdef B_PAPI_ENABLED
  PAPI_option_t opt;
  memset(&opt, 0x0, sizeof (PAPI_option_t) );
  opt.inherit.inherit = PAPI_INHERIT_ALL;
  opt.inherit.eventset = event_set;
  if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK) {
    perror("no!\n");
    exit(1);
  }
#endif


  
  execute_process_data data;
  data.dt = malloc(sizeof(char *) * 10);
  for (int i = 0; i < 10; ++i) {
    data.dt[i] = argv[0];
  }

  pthread_t thread;
  pthread_create(&thread, NULL, execute_process, &argv);
  pthread_join(thread, NULL);
  
  return 0;
}

