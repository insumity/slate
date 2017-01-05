#include <mctop.h>
#include <semaphore.h>
#include "slate_utils.h"

#define CONCAT(a, b) a##b

typedef struct {
  void (*init)();
  void (*new_process)(pid_t pid, int policy);
  int (*get_hwc)(pid_t pid, int* node);
  void (*release_hwc)(int hwc, pid_t pid);

  sem_t lock;
} heuristic_t;
heuristic_t h;


void CONCAT(H, _init)(pin_data** pin, mctop_t* topo);
		     void H_new_process(pid_t pid, int policy);
int CONCAT(H, _get_hwc)(pid_t pid, int *node);
void CONCAT(H, _release_hwc)(int hwc, pid_t pid);


