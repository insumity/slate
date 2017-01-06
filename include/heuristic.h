#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void H_init(pin_data** pin, mctop_t* topo);
sem_t* H_get_lock();

void H_new_process(pid_t pid, int policy);
void H_process_exit(pid_t pid);

int H_get_hwc(pid_t pid, pid_t tid, int* ret_node);
void H_release_hwc(pid_t pid);
