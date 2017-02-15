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
  int retval = PAPI_add_named_event(event_set, "UNHALTED_CORE_CYCLES");
  if (retval != PAPI_OK) {
    printf("Couldn't add L3_TCA\n");
    exit(1);
    }
    
  retval = PAPI_add_named_event(event_set, "INSTRUCTIONS_RETIRED");
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
  retval = PAPI_add_event(event_set, PAPI_L3_TCA);
  if (retval != PAPI_OK) {
    perror("1) eat shit ..\n");
    exit(1);
  }

  printf("I'm here\n");
  retval = PAPI_add_event(event_set, PAPI_L3_TCM);
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

long long* data;
void read_counters(int event_set) {
  long long results[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  int retval;

  retval = PAPI_stop(event_set, results);
  if (retval != PAPI_OK) {
    printf("The event hasn't started yet!\n");
  }

  for (int i = 0; i < 8; ++i) {
    data[i] = results[i];
  }
  
  // printf("X 1) unhalted core cycles:  \t %lld instructions retired:  \t %lld memory local:\t %lld memory remote:\t %lld\n", results[0], results[1], results[2], results[3]);
  // printf("X 2) l3tca:\t %lld L3TCM:\t %lld l2tcm:\t %lld totcyc:\t %lld\n", results[4], results[5], results[6], results[7]);
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
  printf("The filename is: %s\n", filename);
  int fd = open(filename, O_RDWR | O_CREAT, 0777);
  syncfs(fd);
  
  if (fallocate(fd, 0, 0, sizeof(long long) * 8) != 0) {
    perror("couldn' not allocate\n");
  }
  
  data = (long long*) mmap(NULL, sizeof(long long) * 8, PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    fprintf(stderr, "foo barisios\n");
  }


  int retval;
#ifdef B_PAPI_ENABLED
  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    perror("couldn't initialize library\n");
    exit(1);
  }
#endif
  
#ifdef B_PAPI_MULTIPLEX
  printf("PAPI multiplexing baby\n");
  retval = PAPI_multiplex_init();
  if (retval != PAPI_OK) {
    perror("couldn't enable multiplexing\n");
    exit(1);
  }
#endif


  if ((retval = PAPI_set_domain(PAPI_DOM_ALL)) != PAPI_OK) {
    fprintf(stderr, "eat shit!\n");
  }

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

  while (true) {
    start_counters(event_set);
    usleep(100 * 1000);
    read_counters(event_set);
    //printf("====\n");
  }
  
  /* pthread_t threads[48]; */

  /* for (int i = 0; i < 48; ++i) { */
  /*   int* n = malloc(sizeof(int)); */
  /*   *n = i; */
  /*   pthread_create(&threads[i], NULL, foo, n); */
  /* } */


  /* sleep(50); */

  return 0;
}

