#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HRRLAT_init(pin_data** pin, mctop_t* topo);
sem_t* HRRLAT_get_lock();

void HRRLAT_new_process(pid_t pid, int policy);
void HRRLAT_process_exit(pid_t pid);

int HRRLAT_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void HRRLAT_release_hwc(pid_t pid);

