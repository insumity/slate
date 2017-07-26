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
#include <linux/hw_breakpoint.h>



#include <string>
#include <sstream>

#include <iostream>
#include "read_counters.h"

// taken from SO
std::string format_with_commas(long long value)
{
  std::string numWithCommas = std::to_string(value);

  int insertPosition = numWithCommas.length() - 3;
  while (insertPosition > 0) {
    numWithCommas.insert(insertPosition, ",");
    insertPosition-=3;
  }
  return numWithCommas;
}

long long* read_and_print_perf_counters(int cpu, long long values[]) {
  long long* results = (long long *) malloc(sizeof(long long) * 9);
  long long LLC_TCA = 0;
  long long LLC_TCM = 0;
  long long LOCAL_DRAM = 0;
  long long REMOTE_DRAM = 0;
  long long L2_HITS = 0;
  long long L2_MISSES = 0;
  long long unhalted_cycles = 0;
  long long inter_socket_activity1 = 0;
  long long inter_socket_activity2 = 0;
  
  core_data cd = get_counters(cpu);
  LLC_TCA += cd.values[0];
  LLC_TCM += cd.values[1];
  LOCAL_DRAM += cd.values[2];
  REMOTE_DRAM += cd.values[3];
  L2_HITS += cd.values[4];
  L2_MISSES += cd.values[5];
  unhalted_cycles += cd.values[6];
  inter_socket_activity1 += cd.values[7];
  inter_socket_activity2 += cd.values[8];

  results[0] = LLC_TCA;
  results[1] = LLC_TCM;
  results[2] = LOCAL_DRAM;
  results[3] = REMOTE_DRAM;
  results[4] = L2_HITS;
  results[5] = L2_MISSES;
  results[6] = unhalted_cycles;
  results[7] = inter_socket_activity1;
  results[8] = inter_socket_activity2;
  
  LLC_TCA -= values[0];
  LLC_TCM -= values[1];
  LOCAL_DRAM -= values[2];
  REMOTE_DRAM -= values[3];
  L2_HITS -= values[4];
  L2_MISSES -= values[5];
  unhalted_cycles -= values[6];
  inter_socket_activity1 -= values[7];
  inter_socket_activity2 -= values[8];

  std::string TCA_str = format_with_commas(LLC_TCA);
  std::string TCM_str = format_with_commas(LLC_TCM);
  std::string LOCAL_str = format_with_commas(LOCAL_DRAM);
  std::string REMOTE_str = format_with_commas(REMOTE_DRAM);
  std::string L2_HITS_str = format_with_commas(L2_HITS);
  std::string L2_MISSES_str = format_with_commas(L2_MISSES);
  std::cout << "Stringified: " << TCA_str << ", " << TCM_str << ", " << LOCAL_str << ", "
	    << REMOTE_str << " - " << L2_HITS_str << " xs " << L2_MISSES_str << std::endl;

  
  double LLC_hit_rate = 1.0 - (LLC_TCM / (double) LLC_TCA);
  double LOCAL_rate = LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM));
  double L2_hit_rate = L2_HITS / ((double) (L2_HITS + L2_MISSES));

  double LLC_hit_rate_uc = (1.0 - (LLC_TCM / (double) LLC_TCA)) / unhalted_cycles;
  double LOCAL_rate_uc = (LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM))) / unhalted_cycles;
  double L2_hit_rate_uc = (L2_HITS / ((double) (L2_HITS + L2_MISSES))) / unhalted_cycles;


  printf("LLC_hit_rate: %lf, Local %%: %lf, L2_hit_rate: %lf: inter_socket events: %lld\n", LLC_hit_rate, LOCAL_rate, L2_hit_rate,
	 inter_socket_activity1 + inter_socket_activity2);

  std::cout.precision(17);
  std::cout << "LLC_hit_rate_uc: " << std::fixed << LLC_hit_rate_uc << std::endl;
  std::cout << "Local_uc %%: " << std::fixed << LOCAL_rate_uc << std::endl;
  std::cout << "L2_hit_rate_uc: " << std::fixed << L2_hit_rate_uc << std::endl;
  double inter_socket = (inter_socket_activity1 + inter_socket_activity2) / ((double) LLC_TCM);
  std::cout << "inter_socket_activity: " << std::fixed << inter_socket << std::endl;

  printf("Number of unhalted cycles: %lld\n", unhalted_cycles);

  return results;
}


int main(int argc, char* argv[]) {

  start_reading();

  sleep(1);
  
  long long* res = (long long *) malloc(sizeof(long long) * 9);
  for (int i = 0; i < 96; ++i) {

    // zero-ify res
    for (int l = 0; l < 9; ++l) {
      res[l] = 0;
    }

    printf("-----\nCPU to check: %d\n", i);
    for (int k = 0; k < 10; ++k) {
      long long* res_cur = read_and_print_perf_counters(i, res);
      for (int j = 0; j < 9; ++j) {
	res[j] = res_cur[j];
      }
      sleep(1);
    }
    printf("-----\n");
  }

  return 0;
}

