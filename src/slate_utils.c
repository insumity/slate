#include "slate_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <mctop.h>

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


int open_perf(pid_t pid, uint32_t type, uint64_t perf_event_config, int leader)
{
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = type;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = perf_event_config;
  pe.inherit = 1;
  pe.disabled = 1;

  pe.exclude_user = 0;
  pe.exclude_kernel = 0;
  pe.exclude_idle = 0;

  pe.inherit_stat = 1;
  pe.exclude_callchain_kernel = 0;
  pe.exclude_callchain_user = 0;

  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, pid, -1, leader, 0);
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
    exit(EXIT_FAILURE);

  }
  ret = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
  if (ret == -1) {
    perror("ioctl failed\n");
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }
  return count;
}

// Given "line" that is of a format "POLICY program parameters" returns the program
// as a 2D char array and the policy
read_line_output read_line(char* line)
{
  read_line_output result;
  
  int program_cnt = 0;
  char tmp_line[300];
  char* policy = malloc(100);

  if (line[strlen(line) - 1] == '\n') {
    line[strlen(line) - 1 ] = '\0'; // remove new line character
  }
  strcpy(tmp_line, line);
  int first_time = 1;
  char* word = strtok(line, " ");
  result.start_time_ms = atoi(word);
  printf("The start time is: %d\n", result.start_time_ms);
  word = strtok(NULL, " ");
  result.num_id = atoi(word);
  printf("The num id is: %d\n", result.num_id);
  word = strtok(NULL, " ");
  while (word != NULL)  {
    if (first_time) {
      first_time = 0;
      strcpy(policy, word);
    }
    else {
      program_cnt++;
    }
    word = strtok(NULL, " ");
  }

  char** program = malloc(sizeof(char *) * (program_cnt + 1));
  first_time = 1;
  program_cnt = 0;
  word = strtok(tmp_line, " ");
  strtok(NULL, " "); // skip the start time
  word = strtok(NULL, " "); // skip the number id


  while (word != NULL)  {

    if (first_time) {
      first_time = 0;
    }
    else {
      program[program_cnt] = malloc(sizeof(char) * 200);
      strcpy(program[program_cnt], word);
      program_cnt++;
    }
    word = strtok(NULL, " ");
  }

  program[program_cnt] = NULL;
  result.program = program;
  result.policy = policy;
    
  return result;
}

int compare_pids(void* p1, void* p2) {
  return *((unsigned int *) p1) == *((unsigned int *) p2);
}

int compare_voids(void* p1, void* p2) {
  return p1 == p2;
}

pin_data create_pin_data(int core, int node) {
  pin_data tmp;
  tmp.core = core;
  tmp.node = node;
  return tmp;
}

pin_data** initialize_pin_data(mctop_t* topo) {

  size_t num_nodes = mctop_get_num_nodes(topo);
  size_t num_cores_per_socket = mctop_get_num_cores_per_socket(topo);
  size_t num_hwc_per_socket = mctop_get_num_hwc_per_socket(topo);
  size_t num_hwc_per_core = mctop_get_num_hwc_per_core(topo);

  size_t total_hwcs = num_nodes * num_cores_per_socket * num_hwc_per_core;
  pin_data** pin = malloc(sizeof(pin_data*) * MCTOP_ALLOC_NUM);
  for (int i = 0; i < MCTOP_ALLOC_NUM; ++i) {
    pin[i] = malloc(sizeof(pin_data) * total_hwcs);
  }

  for (int i = 0; i < MCTOP_ALLOC_NUM; ++i) {

    mctop_alloc_policy pol = i;
    /* this policy does not contain any hardware contexts */
    if (i == MCTOP_ALLOC_NONE) {
      pol = i + 1; // the hardware contexts are always skipped actually
      // only done so we can measure cost of communication between slate and glibc
    }

    int n_config = -1;
    // Depending on the policy, n_config (3rd parameter of mctop_alloc_create) has different semantics.
    if (pol == MCTOP_ALLOC_MIN_LAT_HWCS || pol == MCTOP_ALLOC_MIN_LAT_CORES_HWCS || pol == MCTOP_ALLOC_MIN_LAT_CORES) {
      n_config = num_hwc_per_socket;
    }
    else {
      n_config = num_nodes;
    }
    assert(n_config != -1);

    //printf("%d: ", pol);
    mctop_alloc_t* alloc = mctop_alloc_create(topo, total_hwcs, n_config, pol);
    for (uint hwc_i = 0; hwc_i < alloc->n_hwcs; hwc_i++)
      {
	uint hwc = alloc->hwcs[hwc_i];
	mctop_hwcid_get_local_node(topo, hwc);
	pin[i][hwc_i] = create_pin_data(hwc, mctop_hwcid_get_local_node(topo, hwc));
	//	printf("%d, ", hwc);
      }
    //printf("\n\n");
    mctop_alloc_free(alloc);
  }

  return pin;
}


