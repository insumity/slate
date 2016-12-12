#ifndef SLATE_UTILS_H
#define SLATE_UTILS_H 1

#include <mctop_alloc.h>
#include <stdint.h>
mctop_alloc_policy get_policy(char* policy);

int open_perf(pid_t pid, uint32_t type, uint64_t perf_event_config);
void close_perf(int fd);
void start_perf_reading(int fd);
void reset_perf_counter(int fd);
long long read_perf_counter(int fd);


#endif
