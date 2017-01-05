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
#include <time.h>

typedef struct ticketlock_t 
{
    volatile uint32_t head;
    uint8_t padding2[4];
} ticketlock_t;


int ticket_trylock(ticketlock_t* lock);
void ticket_acquire(ticketlock_t* lock);
void ticket_release(ticketlock_t* lock);
int is_free_ticket(ticketlock_t* t);

int create_ticketlock(ticketlock_t* the_lock);
ticketlock_t* init_ticketlocks(uint32_t num_locks);
//void init_thread_ticketlocks(uint32_t thread_num);
void free_ticketlocks(ticketlock_t* the_locks);

#define SLOTS_FILE_NAME "/tmp/scheduler_slots"
#define NUM_SLOTS 10

/* Corresponds to who used the slot latest.
 NONE corresponds to the empty slot. */
typedef enum {NONE, SCHEDULER, START_PTHREADS, END_PTHREADS} used_by;
typedef struct {
  int core, node;
  ticketlock_t lock;
  pid_t tid, pid;
  used_by used;
} communication_slot;


void acquire_lock(int i, communication_slot* slots);

void release_lock(int i, communication_slot* slots);


void
__pthread_exit (void *value)
{
  //clock_t start = clock();
  int fd = open(SLOTS_FILE_NAME, O_RDWR);
  if (fd == -1) {
    fprintf(stderr, "Couldnt' open file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
  }

  if (ftruncate(fd, sizeof(communication_slot) * NUM_SLOTS) == -1) {
    fprintf(stderr, "Couldn't ftruncate file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
  }
  communication_slot* slots = mmap(NULL, sizeof(communication_slot) * NUM_SLOTS, 
	       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (slots == MAP_FAILED) {
    fprintf(stderr, "main mmap failed for file %s: %s\n", SLOTS_FILE_NAME, strerror(errno));
  }

  // inform the scheduler that this thread has finished
  int found_slot = 0;
  while(found_slot != 1) {
    int k;
    for (k = 0; k < NUM_SLOTS; ++k) {
      acquire_lock(k, slots);
      //printf("I'm here in pthread_exit() before closing a thread:%d ...\n", k);
      communication_slot *b = &slots[k];

      if (b->used == NONE) {
	b->used = END_PTHREADS;
	b->tid = syscall(__NR_gettid);
	b->pid = getpid();
	//printf("A thread with tid %ld and ppid %ld is closing in pthread_exit() ... kill it\n", (long) b->tid, (long) getpid());
	found_slot = 1;
      }

      release_lock(k, slots);
      if (found_slot == 1) {
	break;
      }
    }
  }
  close(fd);

  //clock_t end = clock();
  //double time = (double) (end - start) / CLOCKS_PER_SEC;
  //printf("Time spent in exit: %lf\n", time);

  THREAD_SETMEM (THREAD_SELF, result, value);
  __do_cancel ();
}
strong_alias (__pthread_exit, pthread_exit)

/* After a thread terminates, __libc_start_main decrements
   __nptl_nthreads defined in pthread_create.c.  */
PTHREAD_STATIC_FN_REQUIRE (pthread_create)
