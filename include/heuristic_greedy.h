#ifdef __cplusplus
extern "C" {
#endif
  
#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HGREEDY_init(pin_data** pin, mctop_t* topo);
sem_t* HGREEDY_get_lock();

void HGREEDY_new_process(pid_t pid, int policy);
void HGREEDY_process_exit(pid_t pid);

int HGREEDY_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void HGREEDY_release_hwc(pid_t pid);

void HGREEDY_reschedule(pid_t pid, int new_policy);

#ifdef __cplusplus
}
#endif
