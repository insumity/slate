#ifdef __cplusplus
extern "C" {
#endif
  
#ifndef SLATE_UTILS_H
#define SLATE_UTILS_H 1

#include <mctop.h>
#include <mctop_alloc.h>
#include <stdint.h>

#define MCTOP_ALLOC_SLATE 13

#define CONCAT(a, b) a##b
#define CACHE_EVENT(cache,operation,result) (\
					     (PERF_COUNT_HW_CACHE_ ## cache) | \
					     (PERF_COUNT_HW_CACHE_OP_ ## operation << 8) | \
					     (PERF_COUNT_HW_CACHE_RESULT_ ## result << 16))

mctop_alloc_policy get_policy(char* policy);

int open_perf(pid_t pid, uint32_t type, uint64_t perf_event_config, int leader);
void close_perf(int fd);
void start_perf_reading(int fd);
void reset_perf_counter(int fd);
long long read_perf_counter(int fd);

typedef struct {
  int core;
  int node;
} pin_data;

typedef struct {
  char* policy;
  int num_id;
  char **program;
  int start_time_ms;
} read_line_output;

read_line_output read_line(char* line);

/*typedef struct {
  int instructions;
  int ref_cycles; // not affected by frequency scaling

  int l1i_cache_read_accesses;
  int l1i_cache_write_accesses;
  int l1d_cache_read_accesses;
  int l1d_cache_write_accesses;
  int l1i_cache_read_misses;
  int l1i_cache_write_misses;
  int l1d_cache_read_misses;
  int l1d_cache_write_misses;

  int ll_cache_read_accesses;
  int ll_cache_write_accesses;
  int ll_cache_read_misses;
  int ll_cache_write_misses;
  } hw_counters_fd; */

typedef struct {
  int number_of_counters;
  int counters_fd[6];
} hw_counters;

hw_counters* create_counters(pid_t pid, int counters, uint64_t* raw_event_codes);
void start_counters(hw_counters cnts);
void read_counters(hw_counters cnts, long long* results);


int compare_pids(void* p1, void* p2);
int compare_voids(void* p1, void* p2);

pin_data create_pin_data(int core, int node);
pin_data** initialize_pin_data(mctop_t* topo);

void pin(pid_t pid, int core, int node);


#endif

#ifdef __cplusplus
}
#endif
