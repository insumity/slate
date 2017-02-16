#ifdef __cplusplus
extern "C" {
#endif
  
#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HSPLIT_init(pin_data** pin, mctop_t* topo);
sem_t* HSPLIT_get_lock();

void HSPLIT_new_process(pid_t pid, int policy);
void HSPLIT_process_exit(pid_t pid);

int HSPLIT_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void HSPLIT_release_hwc(pid_t pid);

#ifdef __cplusplus
}
#endif
