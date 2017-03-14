#ifdef __cplusplus
extern "C" {
#endif

#ifndef __READ_COUNTERS_H
#define __READ_COUNTERS_H
  typedef struct {
    volatile long long values[9];
    pthread_mutex_t lock;
  } core_data;
  
  
void start_reading();
core_data get_counters(int cpu);

#endif
#ifdef __cplusplus
}
#endif
