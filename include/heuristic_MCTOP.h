#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HMCTOP_init(pin_data** pin, mctop_t* topo);
sem_t* HMCTOP_get_lock();

void HMCTOP_new_process(pid_t pid, int policy);
void HMCTOP_process_exit(pid_t pid);

int HMCTOP_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void HMCTOP_release_hwc(pid_t pid);
