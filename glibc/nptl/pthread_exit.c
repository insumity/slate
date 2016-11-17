/* Copyright (C) 2002-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include "pthreadP.h"

#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define NUM_SLOTS 10

/* Corresponds to who used the slot latest.
 NONE corresponds to the empty slot. */
typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;
typedef struct {
  int core, node;
  pid_t tid, pid;
  used_by used;
} communication_slot;


void
__pthread_exit (void *value)
{
  THREAD_SETMEM (THREAD_SELF, result, value);

  /*  int fd = open("/tmp/scheduler", O_RDWR);
  if (fd == -1) {
    fprintf(stderr, "Open failed : %s\n", strerror(errno));
  }
  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "ftruncate : %s\n", strerror(errno));
  }

  communication_slot* addr = mmap(NULL, sizeof(communication_slot) * NUM_SLOTS, PROT_READ | PROT_WRITE,
              MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    fprintf(stderr, "mmap failed:%s\n", strerror(errno));
  }

  // inform the scheduler that this thread has finished
  int found_slot = 0;

  while(found_slot != 1) {
    int k;
    usleep(5000);
    for (k = 0; k < NUM_SLOTS; ++k) {
      printf("ERROR: %s\n", strerror(errno));
      communication_slot *b = addr + k;
      int res = sem_wait(&(b->lock));
      if (res != 0) {
	printf("resutl was WONG %d\n", res);
      }
      if (b->used == NONE) {
	printf("A thread is closing from pthread_exit with tid: %ld\n", syscall(__NR_gettid));
	b->used = END_PTHREADS;
	b->tid = syscall(__NR_gettid);
	b->pid = getppid();
	found_slot = 1;
      }
      res = sem_post(&(b->lock));
      if (res != 0) {
	printf("resutl was WRONG %d\n", res);
      }

      if (found_slot == 1) {
	break;
      }
    }
  }

*/
  __do_cancel ();
}
strong_alias (__pthread_exit, pthread_exit)

/* After a thread terminates, __libc_start_main decrements
   __nptl_nthreads defined in pthread_create.c.  */
PTHREAD_STATIC_FN_REQUIRE (pthread_create)
