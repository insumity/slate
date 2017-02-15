#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "counters.h"

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
			   int cpu, int group_fd, unsigned long flags)
{
  int ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);

  if (ret == -1) {
    perror("couldn't call the perf_event_open syscall");
    exit(EXIT_FAILURE);
  }
  return ret;
}

static int open_perf_counter(pid_t pid, int cpu, uint32_t type,
			     uint64_t perf_event_config, int leader)
{
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = type;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = perf_event_config;
  pe.inherit = 1;

  if (leader == -1) {
    pe.disabled = 0;
  }
  else {
    pe.disabled = 0;
  }

  pe.enable_on_exec = 0;
  pe.exclude_user = 0;
  pe.exclude_kernel = 0;
  pe.exclude_idle = 0;

  pe.inherit_stat = 1;
  pe.exclude_callchain_kernel = 0;
  pe.exclude_callchain_user = 0;

  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, pid, cpu, leader, 0);

  if (fd == -1) {
    fprintf(stderr, "Error opening leader %lld\n", pe.config);
    exit(EXIT_FAILURE);
  }

  return fd;
}

static void reset_perf_counter(int fd) {
  int ret = ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  if (ret != 0) {
    perror("ioctl failed\n");
    exit(EXIT_FAILURE);
  }
}

static void enable_perf_counter(int fd) {
  int ret = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
  if (ret != 0) {
    perror("ioctl failed\n");
    exit(EXIT_FAILURE);
  }
}

static void start_perf_counter(int fd)
{
  reset_perf_counter(fd);
  enable_perf_counter(fd);
}

static void close_perf_counter(int fd)
{
  close(fd);
}

static void disable_perf_counter(int fd) {
  int ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  if (ret != 0) {
    perror("ioctl failed\n");
    exit(EXIT_FAILURE);
  }
}

hw_counters* create_counters(int cpu, int counters, uint64_t* raw_event_codes) {
  if (counters > 8) {
    fprintf(stderr, "Cannot create more than 8 counters per CPU.\n");
    exit(EXIT_FAILURE);
  }
  
  hw_counters* res = malloc(sizeof(hw_counters)); 
  res->number_of_counters = counters;

  res->counters_fd[0] = open_perf_counter(-1, cpu, PERF_TYPE_RAW, raw_event_codes[0], -1);
  for (int j = 1; j < counters; ++j) {
    res->counters_fd[j] = open_perf_counter(-1, cpu, PERF_TYPE_RAW, raw_event_codes[j], res->counters_fd[0]);
  }
  
  return res;
}

void start_counters(hw_counters* cnts) {
  int counters = cnts->number_of_counters;
  for (int i = 0; i < counters; ++i) {
    start_perf_counter(cnts->counters_fd[i]);
  }
}

static long long stop_and_read_perf_counter(int fd)
{
  disable_perf_counter(fd);

  long long count = 0;
  int ret = read(fd, &count, sizeof(long long));
  if (ret == -1) {
    perror("read failed\n");
    exit(EXIT_FAILURE);
  }

  return count;
}

void stop_and_read_counters(hw_counters* cnts, long long* results) {
  int counters = cnts->number_of_counters;

  for (int i = 0; i < counters; i++) {
    results[i] = stop_and_read_perf_counter(cnts->counters_fd[i]);
  }
}

void free_counters(hw_counters* counters) {
  free(counters);
}


