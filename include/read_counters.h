#ifdef __cplusplus
extern "C" {
#endif
  
typedef struct {
  volatile long long values[8];
  pthread_mutex_t lock;
} core_data;

void start_reading();
core_data get_counters(int cpu);

#ifdef __cplusplus
}
#endif
