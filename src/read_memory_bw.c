#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <mctop.h>

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		  int cpu, int group_fd, unsigned long flags)
{
  int ret;

  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

int open_one_perf(int cpu, pid_t pid, uint32_t type, uint64_t perf_event_config, int inherit, int exclude_guest, long flags)
{
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = type;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = perf_event_config;
  pe.inherit = inherit;
  pe.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
  pe.exclude_guest = exclude_guest;

  int group_fd = -1;
  fd = perf_event_open(&pe, pid, cpu, group_fd, flags);

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

void read_perf_counter(int fd, long long results[])
{
  int ret;
  /* ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); */
  /* if (ret == -1) { */
  /*   perror("ioctl failed\n"); */
  /*   exit(EXIT_FAILURE); */
  /* } */

  ret = read(fd, results, 3 * sizeof(long long));
  if (ret == -1) {
    perror("read failed\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char* argv[]) {

  /* sudo perf stat -vv -a -e uncore_imc_0/cas_count_read/  -C 0 */
  /* ------------------------------------------------------------ */
  /* perf_event_attr: */
  /*   type                             22 */
  /*   size                             112 */
  /*   config                           0x304 */
  /*   read_format                      TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING */
  /*   inherit                          1 */
  /*   exclude_guest                    1 */
  /* ------------------------------------------------------------ */
  /* sys_perf_event_open: pid -1  cpu 0  group_fd -1  flags 0x8 */
  int read_fds[96][4] = {0}; // one for each integrated memory controller (IMCs 1, 3, 5, 7 always return 0)

  for (int i = 0; i < 96; ++i) {
    long int  flags = 0x8;
    int cpu = i;
    for (int j = 0; j < 4; ++j) {
      read_fds[i][j] = open_one_perf(cpu, -1, 22 + 2 * j, 0x304, 1, 1, flags);
    }
  }

  
  /* sudo perf stat -vv -a -e uncore_imc_0/cas_count_write/ -C 0 */
  /*  ------------------------------------------------------------ */
  /*  perf_event_attr: */
  /*     type                             22 */
  /*     size                             112 */
  /*     config                           0xc04 */
  /*     read_format                      TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING */
  /*     inherit                          1 */
  /*     exclude_guest                    1 */
  /*   ------------------------------------------------------------ */
  /*   sys_perf_event_open: pid -1  cpu 0  group_fd -1  flags 0x8 */
  int write_fds[96][4] = {0};

  for (int i = 0; i < 96; ++i) {
    long int  flags = 0x8;
    int cpu = i;
    for (int j = 0; j < 4; ++j) {
      write_fds[i][j] = open_one_perf(cpu, -1, 22 + 2 * j, 0xc04, 1, 1, flags);
    }
  }


  while (true) {
    for (int i = 0; i < 96; ++i) {
      for (int j = 0; j < 4; ++j) {
	reset_perf_counter(read_fds[i][j]);
	start_perf_reading(read_fds[i][j]);

	reset_perf_counter(write_fds[i][j]);
	start_perf_reading(write_fds[i][j]);
      }
    }
  
    sleep(1);

    for (int i = 0; i < 96; ++i) {
      for (int j = 0; j < 4; ++j) {
	int ret = ioctl(read_fds[i][j], PERF_EVENT_IOC_DISABLE, 0);
	if (ret == -1) {
	  perror("ioctl failed\n");
	  exit(EXIT_FAILURE);
	}

	ret = ioctl(write_fds[i][j], PERF_EVENT_IOC_DISABLE, 0);
	if (ret == -1) {
	  perror("ioctl failed\n");
	  exit(EXIT_FAILURE);
	}
      }
    }

    int cores_in_socket[24][4];

    for (int i = 0; i < 24; ++i) {
      for (int j = 0; j < 4; ++j) {
	cores_in_socket[i][j] = i * 4 + j;
      }
    }

    long long bw_per_socket[4];

    for (int i = 0; i < 4; ++i) {
      bw_per_socket[i] = 0;
      for (int j = 0; j < 24; ++j) {
	int core = cores_in_socket[j][i];
	long long results[3] = {0};

	for (int imc = 0; imc < 1; ++imc) {
	  read_perf_counter(read_fds[core][imc], results);
	  bw_per_socket[i] += results[0];

	  //	  read_perf_counter(write_fds[core][imc], results);
	  //bw_per_socket[i] += results[0];
	}
      }
      printf("Bandwidth for Socket %d is: %.2lfMB/s\n", i, (bw_per_socket[i] / (1024 * 1024.)) * 64.);
    }
    printf("====\n");

  }

  
  return 0;
}

