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
#include <pthread.h>

#include "include/read_counters.h" // FIXME: wouldn't compile otherwise. I've no idea why!

#include <iostream>
#include <map>
#include <vector>
#include <string>

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

std::vector<int> cores_per_pid;

long long* read_and_print_perf_counters(pid_t pid, long long values[], double* intersocket_activity, bool print_to_stderr) {
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
  
  std::vector<int>::iterator it;

  for (it = cores_per_pid.begin(); it != cores_per_pid.end(); it++) {
    int cpu = *it;
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
  }

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
  std::cout << "1???: " << TCA_str << ", " << TCM_str << ", " << LOCAL_str << ", " << REMOTE_str << " - " << L2_HITS_str << " xs " << L2_MISSES_str << std::endl;

  
  double LLC_hit_rate = 1.0 - (LLC_TCM / (double) LLC_TCA);
  double LOCAL_rate = LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM));
  double L2_hit_rate = L2_HITS / ((double) (L2_HITS + L2_MISSES));

  double LLC_hit_rate_uc = (1.0 - (LLC_TCM / (double) LLC_TCA)) / unhalted_cycles;
  double LOCAL_rate_uc = (LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM))) / unhalted_cycles;
  double L2_hit_rate_uc = (L2_HITS / ((double) (L2_HITS + L2_MISSES))) / unhalted_cycles;


  printf("LLC_hit_rate: %lf, Local %%: %lf, L2_hit_rate: %lf: %lld\n", LLC_hit_rate, LOCAL_rate, L2_hit_rate,
	 inter_socket_activity1 + inter_socket_activity2);

  std::cout.precision(17);
  std::cout << "LLC_hit_rate_uc: " << std::fixed << LLC_hit_rate_uc << std::endl;
  std::cout << "Local_uc %%: " << std::fixed << LOCAL_rate_uc << std::endl;
  std::cout << "L2_hit_rate_uc: " << std::fixed << L2_hit_rate_uc << std::endl;
  double inter_socket = (inter_socket_activity1 + inter_socket_activity2) / ((double) LLC_TCM);
  std::cout << "inter_socket_activity: " << std::fixed << inter_socket << std::endl;

  *intersocket_activity = inter_socket;
  if (print_to_stderr) {
    fprintf(stderr, "%lf\t%lld\t%lld\n", *intersocket_activity, (inter_socket_activity1 + inter_socket_activity2), LLC_TCM);
  }

  printf("Number of unhalted cycles: %lld\n", unhalted_cycles);

  return results;
}

int main(int argc, char* argv[]) {

  cpu_set_t st;
  CPU_ZERO(&st);
  CPU_SET(39, &st);
  
  sched_setaffinity(0, sizeof(cpu_set_t), &st);
  start_reading();

  for (int i = 0; i < 20; ++i) {
    cores_per_pid.push_back(i);
  }

  long long zeroes[9] = {0};
  double foo;
  long long* prev_values;

  while (true) {
    prev_values = read_and_print_perf_counters(-1, zeroes, &foo, false);
    usleep(200 * 1000);
    printf("\n\n---------\n");
    read_and_print_perf_counters(-1, prev_values, &foo, true);
    //fprintf(stderr, "%lf\n", foo);
    printf("---------\n\n\n");

  }
  
  return 0;
}

