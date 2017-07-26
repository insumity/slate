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

typedef struct {
  volatile long long values[10];
  pthread_mutex_t lock;
} core_data;


int add_events(int event_set) {

#ifdef B_PAPI_ENABLED

  unsigned int native = 0x03d3;
  int retval = PAPI_add_named_event(event_set, "MEM_LOAD_UOPS_RETIRED:L3_HIT");
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCA: %d\n", retval);
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

  retval = PAPI_add_named_event(event_set, "ARITH:DIVIDER_UOPS");
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

  // should be a fixed-counter event
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

  // should be a fixed-counter event
  retval = PAPI_add_named_event(event_set, "INSTRUCTIONS_RETIRED");
  if (retval != PAPI_OK) {
    perror("couldn't add instructions retired\n");
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

core_data* data;
void read_counters(int event_set) {
  long long results[10] = {0, 0, 0, 0, 0, 0, 0, 0};
  int retval;

  retval = PAPI_read(event_set, results);
  if (retval != PAPI_OK) {
    printf("The event hasn't started yet!\n");
  }

  pthread_mutex_lock(&(data->lock));

  for (int i = 0; i < 10; ++i) {
    data->values[i] = results[i];
  }
  
  msync(data, sizeof(core_data), MS_SYNC);
  pthread_mutex_unlock(&(data->lock));
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

  char filename[100] = {'\0'};
  strcpy(filename, "/tmp/cpu");
  char id[100];
  sprintf(id, "%d", cpu);
  strcat(filename, id);
  int fd = open(filename, O_RDWR | O_CREAT, 0777);
  syncfs(fd);
  
  if (fallocate(fd, 0, 0, sizeof(core_data)) != 0) {
    perror("couldn' not allocate\n");
  }
  
  data = (core_data *) mmap(NULL, sizeof(core_data), PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    fprintf(stderr, "couldn't memory map\n");
  }

  pthread_mutexattr_t attrmutex;

  pthread_mutexattr_init(&attrmutex);
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
  
  pthread_mutex_init(&(data->lock), &attrmutex);

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

  /* I'm setting the domain below and only for user-level events */
  /* if ((retval = PAPI_set_domain(PAPI_DOM_ALL)) != PAPI_OK) { */
  /*   fprintf(stderr, "couldn't set domain!\n"); */
  /* } */

  int event_set = PAPI_NULL;
#ifdef B_PAPI_ENABLED
  retval = PAPI_create_eventset(&event_set);
  if (retval != PAPI_OK) {
    printf("couldn't assign because: %d\n", retval);
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

  //printf(">>>\t\tRTDL3_HIT\t\tRTDL3_MISS\t\tLOCAL_RAM\t\tREMOTE_RAM\t\tEMPTY_CYCLES\t\tUOPS_RTD\t\tUNHALTED\t\tREM_HITM\t\tREMOTE_FWD\n");
  add_events(event_set);
  start_counters(event_set);
  
  while (true) {
    int one_ms_in_us = 1000;
    int wait_time_in_ms = 125 * one_ms_in_us;
    usleep(wait_time_in_ms);
    read_counters(event_set);
  }
  
  return 0;
}

