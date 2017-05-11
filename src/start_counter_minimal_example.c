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

#define B_PAPI_ENABLED
#define B_PAPI_MULTIPLEX

int add_events(int event_set) {

#ifdef B_PAPI_ENABLED
  int retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_RETIRED:L3_HIT");
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCA\n");
    exit(1);
    }
    
  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_RETIRED:L3_MISS");
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
  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_RETIRED:L2_MISS");
  if (retval != PAPI_OK) {
    perror("couldn't add mem PAPI_L2_LDM\nn");
    exit(1);
  }

 retval = PAPI_add_named_event(event_set, "UOPS_RETIRED");
  if (retval != PAPI_OK) {
    perror("couldn't add retired l2 miss\n");
    exit(1);
  }

  // this counter should be given for free (one of the predefined ones)
  retval = PAPI_add_named_event(event_set, "UNHALTED_CORE_CYCLES");
  if (retval != PAPI_OK) {
    perror("couldn't add unhalted core cycles\n");
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_FWD");
  if (retval != PAPI_OK) {
    perror("couldn't add remote_fwd llc miss\n");
    exit(1);
  }

  retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_HITM");
  if (retval != PAPI_OK) {
    perror("couldn't add remote_hitm llc miss\n");
    exit(1);
  }

#endif

  return event_set;
}

void start_counters(int event_set) {
  int retval = PAPI_reset(event_set);
  if (retval != PAPI_OK) {
    printf("The event hasn't started yet!\n");
  }

  retval = PAPI_start(event_set);
  if (retval != PAPI_OK) {
    printf("The event hasn't started yet!\n");
  }
}

void read_counters(int event_set) {
  long long results[9] = {0, 0, 0, 0, 0, 0, 0, 0};
  int retval;

  retval = PAPI_read(event_set, results);
  if (retval != PAPI_OK) {
    printf("The event hasn't started yet!\n");
  }
  
  for (int i = 0; i < 9; ++i) {
    i < 8? printf("%lld\t\t", results[i]): printf("%lld\n", results[i]);
  }
}

void kill_events(int event_set) {
#ifdef B_PAPI_ENABLED
    int retval = PAPI_cleanup_eventset(event_set);
    if (retval != PAPI_OK) {
      perror("couldn't clean up event set\n");
      exit(1);
    }
    retval = PAPI_destroy_eventset(&event_set);
    if (retval != PAPI_OK) {
      perror("couldn't destroy event set\n");
      exit(1);
    }
#endif
}



int main(int argc, char* argv[]) {
  int cpu = atoi(argv[1]);

  int retval;
#ifdef B_PAPI_ENABLED
  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    perror("couldn't initialize PAPI\n");
    exit(1);
  }
#endif
  
#ifdef B_PAPI_MULTIPLEX
  printf("PAPI multiplexing enabled\n");
  retval = PAPI_multiplex_init();
  if (retval != PAPI_OK) {
    perror("couldn't enable multiplexing\n");
    exit(1);
  }
#endif

  int event_set = PAPI_NULL;
#ifdef B_PAPI_ENABLED
  retval = PAPI_create_eventset(&event_set);
  if (retval != PAPI_OK) {
    printf("couldn't assign because: %d\n", retval);
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
  PAPI_option_t options2;
  memset(&options2, 0x0, sizeof (PAPI_option_t));
  options2.domain.eventset = event_set;
  options2.domain.domain = PAPI_DOM_USER;
  if (PAPI_set_opt(PAPI_DOMAIN, &options2) != PAPI_OK) {
    perror("Coulnd't set to read only user events\n");
    exit(-1);
  }
  
  PAPI_option_t options;
  memset(&options, 0x0, sizeof (PAPI_option_t));
  options.cpu.eventset = event_set;
  options.cpu.cpu_num = cpu;
  retval = PAPI_set_opt(PAPI_CPU_ATTACH, &options);
  if(retval != PAPI_OK) {
    fprintf(stderr, "Coulnd't attach to cpu %d: %d\n", cpu, retval);
    exit(-1);
  }
#endif

  add_events(event_set);
  start_counters(event_set);
  
  while (true) {
    int one_ms_in_us = 1000;
    int wait_time_in_ms = 500 * one_ms_in_us;
    usleep(wait_time_in_ms);
    read_counters(event_set);
  }
  
  return 0;
}

