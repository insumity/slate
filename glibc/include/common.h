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

void acquire_lock(int i, communication_slot* slots)
{
  ticketlock_t *lock = &((slots + i)->lock);
  ticket_acquire(lock);
}

void release_lock(int i, communication_slot* slots)
{
  ticketlock_t *lock = &((slots + i)->lock);
  ticket_release(lock);
}
#endif
