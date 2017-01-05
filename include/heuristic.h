#include <mctop.h>
#include "slate_utils.h"

#define CONCAT(a, b) a##b


void CONCAT(H, _init)(pin_data** pin, mctop_t* topo);
		     void H_new_process(pid_t pid, int policy);
int CONCAT(H, _get_hwc)(pid_t pid, int *node);
void CONCAT(H, _release_hwc)(int hwc, pid_t pid);


