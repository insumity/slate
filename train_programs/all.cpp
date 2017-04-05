#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <numa.h>
#include <strings.h>
#include <mctop.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <vector>
#include <iomanip>
#include <iostream>

#include "../include/read_counters.h"
#include "../include/read_context_switches.h"

using namespace std;

mctop_t* topo;
FILE* fp; // used to have blocking syscalls, TODO build in each thread

typedef struct {
  float operations_percentage;

  int* local_memory, local_memory_size;
  int* remote_memory, remote_memory_size;

  float local_memory_percentage;
  float remote_memory_percentage;

  bool context_switch;
  int context_every_loop;

  bool to_communicate;
  int communicate_every_loop;

} inc_dt;

inc_dt* create_thread_input(int hwcid, float operations_percentage, int local_memory_size, int remote_memory_size,
			    float local_memory_percentage, float remote_memory_percentage, bool context_switch, int context_every_loop,
			    bool to_communicate, int communicate_every_loop) {

  inc_dt* dt = (inc_dt*) malloc(sizeof(inc_dt));
  dt->operations_percentage = operations_percentage;
  
  dt->local_memory_size = local_memory_size;
  uint local_node = mctop_hwcid_get_local_node(topo, hwcid);
  dt->local_memory = (int*) numa_alloc_onnode(sizeof(int) * local_memory_size, local_node);
  
  dt->remote_memory_size = remote_memory_size;
  
  uint remote_node = mctop_hwcid_get_local_node(topo, (hwcid + 1) % 96); // FIXME: specific for quad
  dt->remote_memory = (int*) numa_alloc_onnode(sizeof(int) * remote_memory_size, remote_node);

  dt->local_memory_percentage = local_memory_percentage;
  dt->remote_memory_percentage = remote_memory_percentage; 

  
  dt->context_switch = context_switch;
  dt->context_every_loop = context_every_loop;

  dt->to_communicate = to_communicate;
  dt->communicate_every_loop = communicate_every_loop;
  return dt;
}

bool time_passed(int seconds, int milliseconds, struct timespec* start)
{
    struct timespec *finish = (struct timespec *) malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_MONOTONIC, finish);
    double elapsed = (finish->tv_sec - start->tv_sec);
    double elapsed_ms = (finish->tv_nsec - start->tv_nsec) / (1000. * 1000.);
    elapsed = elapsed * 1000 + elapsed_ms;
    if (elapsed >= (seconds * 1000 + milliseconds)) {
      return true;
    }
    return false;
}


long long read_all_context_switches(std::vector<pid_t> threads, bool isVoluntary) {
  long long context_switches = 0;

  std::vector<pid_t>::iterator it;
  for (it = threads.begin(); it != threads.end(); it++) {
    context_switches += read_context_switches(isVoluntary, *it);
  }

  // IS IT THOUGH??? FIXME TODO
  // context_switches += read_context_switches(isVoluntary, pid);


  return context_switches;
}
  

long long* read_and_print_perf_counters(std::vector<pid_t> threads, std::vector<int> cores, long long old_values[], bool print_to_cerr, long long new_values[], int number_of_threads) {
  long long* results = (long long *) malloc(sizeof(long long) * 10);
  long long LLC_TCA = 0;
  long long LLC_TCM = 0;
  long long LOCAL_DRAM = 0;
  long long REMOTE_DRAM = 0;
  long long ALL_LOADS = 0;
  long long UOPS_RETIRED = 0;
  long long inter_socket_activity1 = 0; // MEM_LOAD_UOPS_LLC_MISS_..._REMOTE_FWD
  long long inter_socket_activity2 = 0; // MEM_LOAD_UOPS_LLC_MISS_..._REMOTE_HITM
  long long context_switches = 0;
  long long unhalted_cycles = 0;

  std::vector<int>::iterator it;
  for (it = cores.begin(); it != cores.end(); it++) {
    int cpu = *it;
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
  context_switches = read_all_context_switches(threads, true);
  
  results[0] = LLC_TCA;
  results[1] = LLC_TCM;
  results[2] = LOCAL_DRAM;
  results[3] = REMOTE_DRAM;
  results[4] = ALL_LOADS;
  results[5] = UOPS_RETIRED;
  results[6] = unhalted_cycles;
  results[7] = inter_socket_activity1;
  results[8] = inter_socket_activity2;
  results[9] = context_switches;

  LLC_TCA -= old_values[0];
  LLC_TCM -= old_values[1];
  LOCAL_DRAM -= old_values[2];
  REMOTE_DRAM -= old_values[3];
  ALL_LOADS -= old_values[4];
  UOPS_RETIRED -= old_values[5];
  unhalted_cycles -= old_values[6];
  inter_socket_activity1 -= old_values[7];
  inter_socket_activity2 -= old_values[8];
  context_switches -= old_values[9];

  new_values[0] = UOPS_RETIRED;
  new_values[1] = ALL_LOADS;
  new_values[2] = LLC_TCM;
  new_values[3] = LLC_TCA;
  new_values[4] = LOCAL_DRAM;
  new_values[5] = REMOTE_DRAM;
  new_values[6] = inter_socket_activity1;
  new_values[7] = inter_socket_activity2;
  new_values[8] = context_switches;
  new_values[9] = number_of_threads;

  return results;
}

volatile int counter;

std::vector<pid_t> threads_v;
std::vector<int> cores_v;

int number_of_threads;
pthread_mutex_t threads_v_lock;

void* inc2(void* index_pt) {
  pid_t tid = syscall(__NR_gettid);
  pthread_mutex_lock(&threads_v_lock);
  threads_v.push_back(tid);
  pthread_mutex_unlock(&threads_v_lock);

  
  inc_dt* dt = (inc_dt *) index_pt;

  float operations_percentage = dt->operations_percentage;
  int* local_memory = dt->local_memory;
  int local_memory_size = dt->local_memory_size;
  int* remote_memory = dt->remote_memory;
  int remote_memory_size = dt->remote_memory_size;

  float local_memory_percentage = dt->local_memory_percentage;
  float remote_memory_percentage = dt->remote_memory_percentage;

  bool context_switch = dt->context_switch;
  int context_every_loop = dt->context_every_loop; // in how may loops to we context switch?

  bool to_communicate = dt->to_communicate;
  int communicate_every_loop = dt->communicate_every_loop;

  long long sum = 0;
  int local_from = 0;
  int remote_from = 0;

  struct timespec *start = (struct timespec *) malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, start);

  long long loops = 0;
  bool first_time = true;
  while (true) {
    loops++;

    int total_ops = 1000;
    int operations = total_ops * operations_percentage;
    int memory_accesses = total_ops * (1. - operations_percentage);
    int local_memory_accesses = memory_accesses * local_memory_percentage;
    int remote_memory_accesses = memory_accesses * remote_memory_percentage;
    
    for (int i = 0; i < operations; ++i) {
      for (int j = 0; j < 5; ++j) {
	sum += 534 - 13 * i;
	sum -= ((double) 13.5) * sum + 99.234 / 23. - 13.0 /i;
      }
    }

    for (int i = 0; i < local_memory_accesses; ++i) {
      sum += local_memory[local_from];
      local_memory[local_from] = sum;
      local_from++;
      if (local_from == local_memory_size) {
	local_from = 0;
      }
    }

    for (int i = 0; i < remote_memory_accesses; ++i) {
      sum += remote_memory[remote_from];
      remote_memory[remote_from] = sum;
      remote_from++;
      if (remote_from == remote_memory_size) {
	remote_from = 0;
      }
    }

    if (to_communicate && loops % communicate_every_loop == 0) {
      counter++;
    }

    if ((context_every_loop != 0) && (loops % context_every_loop == 0) && context_switch) {
      fputc('A', fp);
      fputc('B', fp);
      fflush(fp);
    }

    const int PASSED_SECONDS = 2;
    const int PASSED_MILLISECONDS = 500;
    if (loops % 1000 == 0 && time_passed(PASSED_SECONDS, PASSED_MILLISECONDS, start)) {
      free(start);
      break;
    }
  }

  long long* sum_pt = (long long*) malloc(sizeof(long long));
  *sum_pt = sum;
  *sum_pt = loops;
  return sum_pt;
}


void loop_internal(int *threads_a, int threads_size, float operations_percentage, float local_memory_percentage, float remote_memory_percentage, int context_every_loop, int to_communicate,
		   int communicate_every_loop, int memory_size) {
  int* cores_loc_cores = (int*) malloc(sizeof(int) * 24);
  int loc_cores[24] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92};
  for (int i = 0; i < 24; ++i) {
    cores_loc_cores[i] = loc_cores[i];
  }

  int* cores_rr = (int *) malloc(sizeof(int ) * 24);
  int rr[24] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
  for (int i = 0; i < 24; ++i) {
    cores_rr[i] = rr[i];
  }
  int* foo[2] = {cores_loc_cores, cores_rr};
  int* cores;


  int local_memory_size, remote_memory_size;
  local_memory_size = remote_memory_size = memory_size;


  struct timespec start_x;
  clock_gettime(CLOCK_MONOTONIC, &start_x);

  const int ITERATIONS = 3;
  double results[ITERATIONS][2][25];
  long long int perf_counters[ITERATIONS][2][25][10];
  for (int it = 0; it < ITERATIONS; ++it) {
    fprintf(stderr, "\t\t\t\t\tIteration: %d\n", it);
    for (int policy = 0; policy < 2; ++policy) {
      fprintf(stderr, "\t\t\t\t\t\tPolicy: %d\n", policy);
      cores = foo[policy];

      for (int t = 0; t < threads_size; ++t) {
	int threads = threads_a[t];
	fprintf(stderr, "\t\t\t\t\t\t\tThreads: %d\n", threads);

	inc_dt* mem[25];
	int mem_i = 0;

	number_of_threads = threads;
	pthread_t threads_[threads];
	for (int i = 0; i < threads; ++i) {
	  int hwcid = cores[i];
	  inc_dt* dt = create_thread_input(hwcid, operations_percentage, local_memory_size,
					   remote_memory_size, local_memory_percentage, remote_memory_percentage, true, context_every_loop, to_communicate == 1? true: false,
					   communicate_every_loop);

	  cores_v.push_back(hwcid);
		    
	  mem[mem_i] = dt;
	  mem_i++;

	  if (pthread_create(&threads_[i], NULL, inc2, dt)) {
	    fprintf(stderr, "Error creating thread\n");
	    return;
	  }


	  cpu_set_t st;
	  CPU_ZERO(&st);
	  CPU_SET(hwcid, &st);
	  if (pthread_setaffinity_np(threads_[i], sizeof(cpu_set_t), &st)) {
	    perror("Something went wrong while setting the affinity!\n");
	    return;
	  }
	} // for i < threads 

	long long zeroes[10] = {0};
	long long* prev_counters;

	long long new_values[10];

	sleep(1);
	prev_counters = read_and_print_perf_counters(threads_v, cores_v, zeroes, false, new_values, number_of_threads);


	sleep(1);
	read_and_print_perf_counters(threads_v, cores_v, prev_counters, true, new_values, number_of_threads);
	for (int i = 0; i < 10; ++i) {
	  perf_counters[it][policy][number_of_threads][i] = new_values[i];
	  printf("%lld ", new_values[i]);
	}
	printf("\n");


	long long all_loops = 0;
	void* status;
	for (int i = 0; i < threads; ++i) {
	  pthread_join(threads_[i], &status);
	  all_loops += (*(long long*) status);
	  free(status);
	}

	for (int i = 0; i < mem_i; ++i) {
	  numa_free(mem[i]->local_memory, sizeof(int) * local_memory_size);
	  numa_free(mem[i]->remote_memory, sizeof(int) * remote_memory_size);
	  free(mem[i]);
	}
	cores_v.clear();
	threads_v.clear();


	double f = results[it][policy][threads] = all_loops / (double) threads;
	bool best_rr = true;
      } // threads for
    } // policy for
  } // iteration for


  // find best policy for specific amount of threads
  double sum_results[2][25] = {0};
  for (int it = 0; it < ITERATIONS; ++it) {
    for (int t = 0; t < threads_size; t++) {
      int threads = threads_a[t];
      sum_results[0][threads] += results[it][0][threads];
      sum_results[1][threads] += results[it][1][threads];
    }
  }

  double sum_perf_counters[2][25][10] = {0};

  for (int it = 0; it < ITERATIONS; ++it) {
    for (int t = 0; t < threads_size; t++) {
      int threads = threads_a[t];
      for (int k = 0; k < 10; ++k) {
	  
	sum_perf_counters[0][threads][k] += perf_counters[it][0][threads][k];
	sum_perf_counters[1][threads][k] += perf_counters[it][1][threads][k];
      }

      for (int policy = 0; policy < 2; ++policy) {
	long long uops_retired = perf_counters[it][policy][threads][0];
	long long all_loads = perf_counters[it][policy][threads][1];
	long long llc_tcm = perf_counters[it][policy][threads][2];
	long long llc_tca = perf_counters[it][policy][threads][3];
	long long local_dram = perf_counters[it][policy][threads][4];
	long long remote_dram = perf_counters[it][policy][threads][5];
	long long inter_socket1 = perf_counters[it][policy][threads][6];
	long long inter_socket2 = perf_counters[it][policy][threads][7];

	fprintf(stderr, "policy: %d, iteration: %d, threads: %d loads: %lf == llc miss rate: %lf == local: %lf == is1 + is2: %lf, " \
		"inter_socket1: %lld, inter_socket2: %lld\n", policy, it, threads, all_loads / ((double) uops_retired),
		llc_tcm / ((double) llc_tca), local_dram / ((double) local_dram + remote_dram), (inter_socket1 + inter_socket2) / ((double) llc_tcm), inter_socket1, inter_socket2);
      }
    }
  }
  printf("\n");
  


  for (int t = 0; t < threads_size; t++) {
    int threads = threads_a[t];

    double loc_cores_d = sum_results[0][threads] / ITERATIONS;
    double rr_d = sum_results[1][threads] / ITERATIONS;

    // 0 for loc_cores, 1 for rr
    int best_policy = (loc_cores_d >= rr_d)? 0: 1;

    printf("Average perf counters for threads: %d\n", threads);
    {
      double loc[10], rr[10];
      for (int k = 0; k < 10; ++k) {
	loc[k] = sum_perf_counters[0][threads][k] / ITERATIONS;
	if (k == 9) {
	  cerr << "How much is : loc[9]" << loc[9] << " and ITERA: " << ITERATIONS << endl;
	}
	rr[k] = sum_perf_counters[1][threads][k] / ITERATIONS;
      }

      cerr << fixed << "1, 0, ";
      for (int k = 0; k < 9; ++k) {
	cerr << fixed << setprecision(0) << loc[k] << ", ";
      }
      cerr << fixed << setprecision(0) << loc[9] << ", " << loc_cores_d << ", " << rr_d << endl;

      cerr << fixed << "0, 1, ";
      for (int k = 0; k < 9; ++k) {
	cerr << fixed << setprecision(0) << rr[k] << ", ";
      }
      cerr << fixed << setprecision(0) << rr[9] << ", " << loc_cores_d << ", " << rr_d << endl;
    }
    //printf("--- Threads: %d => LOC_CORES(%lf), RR(%lf) = %s ---\n", threads, loc_cores, rr,
    //loc_cores >= rr? "LOC": "RR");
  }

  struct timespec end_x;
  clock_gettime(CLOCK_MONOTONIC, &end_x);
  double elapsed_x = (end_x.tv_sec - start_x.tv_sec);
  elapsed_x += (start_x.tv_nsec - start_x.tv_nsec) / 1000000000.0;
  printf("\n\n\n\n\n... SECONDS: %lf\n", elapsed_x);
	    
}

int main(int argc, char* argv[]) {


  srand(time(NULL));
  pthread_mutex_init(&threads_v_lock, NULL);
  start_reading();
  fp = fopen("/tmp/foo.txt", "w+");
  
  topo = mctop_load(NULL);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }



  int KB = 1024 / sizeof(int);
  int MB = (1024 * 1024) / sizeof(int);

  int threads_a[] = {4, 8, 12, 20};

  float memory_percentage[5][2] = {{0.1, 0.1}, {0.5, 0.5}, {0, 1}, {1, 0}, {0.3, 0.7}}; // first is local ... the other is remote
  for (float operations_percentage = 0.; operations_percentage <= 1.1; operations_percentage += 1.) {
    fprintf(stderr, "Operations: %lf\n", operations_percentage);

    for (int memory_percentage_index = 0; memory_percentage_index < 5; ++memory_percentage_index) {
      fprintf(stderr, "\tMemory percentage: (local: %lf, remote: %lf)\n", memory_percentage[memory_percentage_index][0],
	      memory_percentage[memory_percentage_index][1]);
      float local_memory_percentage = memory_percentage[memory_percentage_index][0];
      float remote_memory_percentage = memory_percentage[memory_percentage_index][1];
      
      for (int context_every_loop = 20; context_every_loop <= 110; context_every_loop += 90) {
	fprintf(stderr, "\t\tContext every loop: %d\n", context_every_loop);
	
	for (int to_communicate = 0; to_communicate < 2; ++to_communicate) {
	  fprintf(stderr, "\t\t\tTo communicate: %d\n", to_communicate);
	  
	  for (int communicate_every_loop = 2; communicate_every_loop <= 5000; communicate_every_loop += 4998) {

	    for (int memory_size = 1 * KB; memory_size <= 2 * 5 * MB + 1 * KB; memory_size += 5 * MB) { // start with 1 * KB
	      fprintf(stderr, "\t\t\t\tMemory size in KB: %d\n", (int) (memory_size / 1024.0));

	      // FIXME threads
	      loop_internal(threads_a, 4, operations_percentage, local_memory_percentage, remote_memory_percentage, context_every_loop, to_communicate, communicate_every_loop, memory_size);
	    }
	  }
	}
      }
    }
  }



  return 0;
}

