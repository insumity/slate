#include "slate_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

mctop_alloc_policy get_policy(char* policy) {
  mctop_alloc_policy pol;
  if (strcmp(policy, "MCTOP_ALLOC_NONE") == 0) {
    pol = MCTOP_ALLOC_NONE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_SEQUENTIAL") == 0) {
    pol = MCTOP_ALLOC_SEQUENTIAL;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_HWCS_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_HWCS_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_MIN_LAT_CORES_BALANCE") == 0) {
    pol = MCTOP_ALLOC_MIN_LAT_CORES_BALANCE;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS") == 0) {
    pol = MCTOP_ALLOC_BW_ROUND_ROBIN_HWCS;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_ROUND_ROBIN_CORES") == 0) {
    pol = MCTOP_ALLOC_BW_ROUND_ROBIN_CORES;
  }
  else if (strcmp(policy, "MCTOP_ALLOC_BW_BOUND") == 0) {
    pol = MCTOP_ALLOC_BW_BOUND;
  }
  else {
    perror("Not recognized policy\n");
    return -1;
  }

  return pol;
}

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		  int cpu, int group_fd, unsigned long flags)
{
  int ret;

  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

#define CACHE_EVENT(cache, operation, result) (\
					     (PERF_COUNT_HW_CACHE_ ## cache) | \
					     (PERF_COUNT_HW_CACHE_OP_ ## operation << 8) | \
					     (PERF_COUNT_HW_CACHE_RESULT_ ## result << 16))


int open_perf(pid_t pid, uint32_t type, uint64_t perf_event_config)
{
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = type;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = perf_event_config;
  pe.disabled = 1;
  pe.exclude_kernel = 0;
  /*pe.exclude_hv = 1; */

  fd = perf_event_open(&pe, pid, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening leader %lld\n", pe.config);
    perror("");
    exit(EXIT_FAILURE);
  }

  return fd;
}

void close_perf(int fd)
{
  close(fd);
}

void start_perf_reading(int fd)
{
  int ret = ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  if (ret == -1) {
    perror("ioctl failed\n");
  }
  ret = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
  if (ret == -1) {
    perror("ioctl failed\n");
  }
}

void reset_perf_counter(int fd) {
  int ret = ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  if (ret == -1) {
    perror("ioctl failed\n");
  }
}

long long read_perf_counter(int fd)
{
  long long count = 0;
  int ret = read(fd, &count, sizeof(long long));
  if (ret == -1) {
    perror("read failed\n");
  }
  return count;
}

