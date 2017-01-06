#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

void HX_init(pin_data** pin, mctop_t* topo);
sem_t* HX_get_lock();
void HX_new_process(pid_t pid, int policy);
int HX_get_hwc(pid_t pid, int* ret_node);
void HX_release_hwc(int hwc, pid_t pid);
