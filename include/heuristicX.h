#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HX_init(pin_data** pin, mctop_t* topo);
sem_t* HX_get_lock();

void HX_new_process(pid_t pid, int policy);
void HX_process_exit(pid_t pid);

int HX_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void HX_release_hwc(pid_t pid);

