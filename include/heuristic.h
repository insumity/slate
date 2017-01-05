#include <mctop.h>
#include "slate_utils.h"

void H_init(pin_data** pin, mctop_t* topo);
void H_new_process(pid_t pid, int policy);
int H_get_hwc(pid_t pid, int* ret_node);
void H_release_hwc(int hwc, pid_t pid);
