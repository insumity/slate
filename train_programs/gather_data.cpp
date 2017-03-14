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

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "read_counters.h"
#include "read_context_switches.h"
#include "get_tids.h"


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

double divide(long long a, long long b) {
  if (b == 0) {
    return 0; // so we don't get NaN
  }
  return a / ((double) b);
}

long long* read_and_print_perf_counters(long long values[], double m_results[], bool print_to_cerr, std::vector<int> v, FILE* out) {
  long long* results = (long long *) malloc(sizeof(long long) * 9);
  long long LLC_TCA = 0;
  long long LLC_TCM = 0;
  long long LOCAL_DRAM = 0;
  long long REMOTE_DRAM = 0;
  long long ALL_LOADS = 0;
  long long UOPS_RETIRED = 0;
  long long unhalted_cycles = 0;
  long long inter_socket_activity1 = 0;
  long long inter_socket_activity2 = 0;
  
  std::vector<int>::iterator it;

  for (it = v.begin(); it != v.end(); it++) {
    int cpu = *it;
    fprintf(out, "CPU is : %d - ", cpu);
    core_data cd = get_counters(cpu);
    LLC_TCA += cd.values[0];
    LLC_TCM += cd.values[1];
    LOCAL_DRAM += cd.values[2];
    REMOTE_DRAM += cd.values[3];
    ALL_LOADS += cd.values[4];
    UOPS_RETIRED += cd.values[5];
    unhalted_cycles += cd.values[6];
    inter_socket_activity1 += cd.values[7];
    inter_socket_activity2 += cd.values[8];
  }
  fprintf(out, "\n");

  results[0] = LLC_TCA;
  results[1] = LLC_TCM;
  results[2] = LOCAL_DRAM;
  results[3] = REMOTE_DRAM;
  results[4] = ALL_LOADS;
  results[5] = UOPS_RETIRED;
  results[6] = unhalted_cycles;
  results[7] = inter_socket_activity1;
  results[8] = inter_socket_activity2;
  
  LLC_TCA -= values[0];
  LLC_TCM -= values[1];
  LOCAL_DRAM -= values[2];
  REMOTE_DRAM -= values[3];
  ALL_LOADS -= values[4];
  UOPS_RETIRED -= values[5];
  unhalted_cycles -= values[6];
  inter_socket_activity1 -= values[7];
  inter_socket_activity2 -= values[8];

  std::string TCA_str = format_with_commas(LLC_TCA);
  std::string TCM_str = format_with_commas(LLC_TCM);
  std::string LOCAL_str = format_with_commas(LOCAL_DRAM);
  std::string REMOTE_str = format_with_commas(REMOTE_DRAM);
  std::string ALL_LOADS_str = format_with_commas(ALL_LOADS);
  std::string UOPS_RETIRED_str = format_with_commas(UOPS_RETIRED);

  if (out == stdout) {
    std::cout << "1???: " << TCA_str << ", " << TCM_str << ", (local_str: " << LOCAL_str << "), (remote_str: " << REMOTE_str << ") - " << ALL_LOADS_str << " xs " << UOPS_RETIRED_str << std::endl;
  }
  else {
    std::cerr << "1???: " << TCA_str << ", " << TCM_str << ", (local_str: " << LOCAL_str << "), (remote_str: " << REMOTE_str << ") - " << ALL_LOADS_str << " xs " << UOPS_RETIRED_str << std::endl;
  }

  double LLC_hit_rate = 1.0 - (LLC_TCM / (double) LLC_TCA);
  double LOCAL_rate = LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM));

  double LLC_hit_rate_uc = (1.0 - (LLC_TCM / (double) LLC_TCA)) / unhalted_cycles;
  double LOCAL_rate_uc = (LOCAL_DRAM / ((double) (LOCAL_DRAM + REMOTE_DRAM))) / unhalted_cycles;

  fprintf(out, "LLC_hit_rate: %lf, Local %%: %lf, : %lld\n", LLC_hit_rate, LOCAL_rate,  inter_socket_activity1 + inter_socket_activity2);

  if (out == stdout) {
    std::cout.precision(17);
    std::cout << "LLC_hit_rate_uc: " << std::fixed << LLC_hit_rate_uc << std::endl;
    std::cout << "Local_uc %%: " << std::fixed << LOCAL_rate_uc << std::endl;
    double inter_socket = (inter_socket_activity1 + inter_socket_activity2) / ((double) LLC_TCM);
    double inter_socket2 = (inter_socket_activity1 + inter_socket_activity2) / ((double) inter_socket_activity1 + inter_socket_activity2 + LOCAL_DRAM + REMOTE_DRAM);
    std::cout << "inter_socket_activity: " << std::fixed << inter_socket << " other normalizatin: " << inter_socket2 << std::endl;
  }
  else {
    std::cerr.precision(17);
    std::cerr << "LLC_hit_rate_uc: " << std::fixed << LLC_hit_rate_uc << std::endl;
    std::cerr << "Local_uc %%: " << std::fixed << LOCAL_rate_uc << std::endl;
    double inter_socket = (inter_socket_activity1 + inter_socket_activity2) / ((double) LLC_TCM);
    double inter_socket2 = (inter_socket_activity1 + inter_socket_activity2) / ((double) inter_socket_activity1 + inter_socket_activity2 + LOCAL_DRAM + REMOTE_DRAM);
    std::cerr << "inter_socket_activity: " << std::fixed << inter_socket << " other normalizatin: " << inter_socket2 << std::endl;
  }

  if (print_to_cerr) {
    //std::cerr << std::fixed << inter_socket << "\t" << inter_socket2 << "\t" << REMOTE_DRAM << "\t" << (REMOTE_DRAM / ((double) LLC_TCM)) << "\t"
    //<< (inter_socket_activity1 + inter_socket_activity2) << "\t" << std::endl;
  }

  m_results[0] = divide(ALL_LOADS, UOPS_RETIRED);
  m_results[1] = divide(LLC_TCM, LLC_TCA);
  m_results[2] = divide(REMOTE_DRAM, (LOCAL_DRAM + REMOTE_DRAM));
  m_results[3] = divide(inter_socket_activity1 + inter_socket_activity2, LLC_TCM);
  m_results[4] = (m_results[0]) * (m_results[1]) * (m_results[2]) * (m_results[3]);
  
  fprintf(out, "Number of unhalted cycles: %lld\n", unhalted_cycles);
  fprintf(out, "%%loads: %lf, %%LLC miss rate: %lf, %%remote: %lf, intersocket: %lf\n", m_results[0], m_results[1], m_results[2], m_results[3]);
  
  return results;
}

pid_t execute_program(char** program) {
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

  char** program = (char**) malloc(sizeof(char *) * argc);
  for (int i = 0; i < argc - 1; ++i) {
    program[i] = (char*) malloc(sizeof(char) * 1000);
    strcpy(program[i], argv[i + 1]);
  }
  program[argc] = NULL;

  pid_t pid = execute_program(program);
  sleep(2);

  
  int number_of_threads;
  int* values = get_thread_ids(pid, &number_of_threads);
  printf("Number of threads: %d\n", number_of_threads);
  for (int i = 0; i < number_of_threads; ++i) {
    printf("%d: %d\n", i, values[i]);
  }

#ifdef LOC_HWCS
  int cores_i[20] = {0, 48, 4, 52, 8, 56, 12, 60, 16, 64, 20, 68, 24, 72, 28, 76, 32, 80, 36, 84};
#elif LOC_CORES
  int cores_i[20] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76};
#else
  int cores_i[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
#endif


  std::vector<int> cores;
#ifdef PIN
  printf("I'm about to pin!\n");

  for (int i = 0; i < number_of_threads; ++i) {
    cpu_set_t st;
    CPU_ZERO(&st);
    CPU_SET(cores_i[i], &st);
    cores.push_back(cores_i[i]);
    printf("Thread: %ld Core: %d going to node!\n", (long int) values[i], cores_i[i]);
    if (sched_setaffinity(values[i], sizeof(st), &st)) {
      perror("Something went wrong while setting the affinity!\n");
      return EXIT_FAILURE;
    }
  }

#endif

  printf("The size of cores is .... %d\n", cores.size());
  
  start_reading();
  long long zeroes[9] = {0};
  long long* prev_counters;
  while (true) {
    double m_results[9];
    prev_counters = read_and_print_perf_counters(zeroes, m_results, false, cores, stdout);
    sleep(1);
    read_and_print_perf_counters(prev_counters, m_results, true, cores, stderr);

    int status;
    if (waitpid(pid, &status, WNOHANG) != 0) {
      break;
    }
  }

  
  return 0;
}

