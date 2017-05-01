#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <numaif.h>
#include <papi.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <iostream>
#include <vector>

#include "read_counters.h"
#include "read_context_switches.h"
#include "get_tids.h"
#include "read_memory_bandwidth.h"
#include "gather_data.h"

#define COUNTERS_NUMBER 9

// returns the old value if new_value - old_value < 00
long long get_difference(long long new_value, long long old_value) {
  return (new_value - old_value >= 0)? new_value - old_value: old_value;
}

long long* subtract(long long* ar1, long long* ar2, int length) {

  long long* result = (long long *) malloc(sizeof(long long) * length);

  for (int i = 0 ; i < length; ++i) {
    result[i] = get_difference(ar1[i], ar2[i]);
  }

  return result;
}

void print_perf_counters(bool is_rr, int result, long long values[], long long context_switches, double sockets_bw[]) {

  if (is_rr) {
    std::cerr << "0, 1, ";
  }
  else {
    std::cerr << "1, 0, ";
  }

  for (int i = 0; i < COUNTERS_NUMBER; ++i) {
    std::cerr << values[i] << ", ";
  }
  std::cerr << context_switches << ", ";
  for (int i = 0; i < 4; ++i) {
    std::cerr << sockets_bw[i] << ", ";
  }

  std::cerr << result << std::endl;
}

// returns the current values of the read performance counters
long long* read_perf_counters(long long values[], std::vector<int> hwcs) {


  // Events being read by start_counter in this order
  // MEM_LOAD_UOPS_RETIRED:L3_HIT
  // MEM_LOAD_UOPS_RETIRED:L3_MISS
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:LOCAL_DRAM
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM
  // MEM_LOAD_UOPS_RETIRED:L2_MISS
  // UOPS_RETIRED
  // UNHALTED_CORE_CYCLES
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_FWD
  // MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_HITM

  long long* results = (long long *) malloc(sizeof(long long) * COUNTERS_NUMBER);
  long long L3_HIT = 0; 
  long long L3_MISS = 0;
  long long LOCAL_DRAM = 0;
  long long REMOTE_DRAM = 0;
  long long L2_MISS = 0;
  long long UOPS_RETIRED = 0;
  long long UNHALTED_CORE_CYCLES = 0;
  long long REMOTE_FWD = 0;
  long long REMOTE_HITM = 0;
  

  for (std::vector<int>::iterator it = hwcs.begin(); it != hwcs.end(); it++) {
    int cpu = *it;

    core_data cd = get_counters(cpu);
    L3_HIT += cd.values[0];
    L3_MISS += cd.values[1];
    LOCAL_DRAM += cd.values[2];
    REMOTE_DRAM += cd.values[3];
    L2_MISS += cd.values[4];
    UOPS_RETIRED += cd.values[5];
    UNHALTED_CORE_CYCLES += cd.values[6];
    REMOTE_FWD += cd.values[7];
    REMOTE_HITM += cd.values[8];
  }

  results[0] = L3_HIT;
  results[1] = L3_MISS;
  results[2] = LOCAL_DRAM;
  results[3] = REMOTE_DRAM;
  results[4] = L2_MISS;
  results[5] = UOPS_RETIRED;
  results[6] = UNHALTED_CORE_CYCLES;
  results[7] = REMOTE_FWD;
  results[8] = REMOTE_HITM;
  
  return results;
}

pid_t execute_program(char* program[]) {
  pid_t pid = fork();

  
  if (pid == 0) {
    execv(program[0], program);
    printf("== %s\n", program[0]);
    perror("execve didn't work");
    exit(1);
  }

  return pid;
}


int main(int argc, char* argv[])
{
  int final_result = atoi(argv[1]);
  int result = atoi(argv[2]);
  bool is_rr = result == 1;

  int number_of_threads = atoi(argv[3]);

  int cores_loc_cores[48] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44,
			     1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45,
			     2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46,
			     3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47};
  

  int cores_rr[48];
  for (int i = 0; i < 48; ++i) {
    cores_rr[i] = i;
  }


  std::vector<int> hwcs;
  for (int i = 0; i < number_of_threads; ++i) {
    int core = is_rr? cores_rr[i]: cores_loc_cores[i];
    printf("(%d) ", core);
    hwcs.push_back(core);
  }
  printf("\n");

  
  int start = 4;
  argc -= 4;

  char* program[argc  + 1];
  for (int i = 0; i < argc; ++i) {
    program[i] = (char*) malloc(sizeof(char) * 1000);
    strcpy(program[i], argv[i + start]);
    printf("p(%d): %s\n", i, program[i]);
  }
  program[argc] = NULL;

  pid_t pid = execute_program(program);

  FILE* fp = start_reading_memory_bandwidth();
  sleep(1);
  
  int* tids = get_thread_ids(pid, &number_of_threads);

  //fprintf(stderr, "retired_uops\tloads_retired_ups\tLLC_misses\tLLC_references\tlocal_accesses\tremote_accesses\tinter_socket1\tinter_socket2\tcontext_switches\tnumber_of_threads\n");
  start_reading();
  long long prev_context_switches = 0;
  long long *prev_counters, *cur_counters;
  prev_counters = (long long *) malloc(sizeof(long long) * COUNTERS_NUMBER);
  bzero(prev_counters, sizeof(long long) * COUNTERS_NUMBER);
  
  while (true) {

    cur_counters = read_perf_counters(prev_counters, hwcs);
    sleep(1);

    double sockets_bw[4];
    for (int i = 0; i < 4; ++i) {
      sockets_bw[i] = read_memory_bandwidth(i, fp);
      rewind(fp);
    }

    long cur_context_switches = 0;
    for (int i = 0; i < number_of_threads; ++i) {
      long long result = read_context_switches(true, tids[i]);
      if (result == -1) {
	printf("cannot read any more context switches. probably files are missing!");
	int status;
	while (waitpid(pid, &status, WNOHANG) != 0) {
	  usleep(100);
	  return 0;
	}
	
      }
      cur_context_switches += result;
    }


    prev_counters = read_perf_counters(cur_counters, hwcs);

    long long* actual_values = subtract(prev_counters, cur_counters, COUNTERS_NUMBER);
    print_perf_counters(is_rr, final_result, actual_values, cur_context_switches - prev_context_switches, sockets_bw);
    free(actual_values);
    
    prev_context_switches = cur_context_switches;
    
    int status;
    if (waitpid(pid, &status, WNOHANG) != 0) {
      break;
    }
  }


  fclose(fp);
  return 0;
}

