#ifdef __cplusplus
extern "C" {
#endif

#ifndef PYTHON_CLASSIFY_H_
#define PYTHON_CLASSIFY_H_

typedef struct {
  int LOC, RR;
  long long l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired, unhalted_cycles, remote_fwd, remote_hitm;
  long long instructions;
  long long context_switches;
  double sockets_bw[4];
  int number_of_threads;
  int response;
} classifier_data;

  classifier_data create_data(int LOC, int RR, long long l3_hit, long long l3_miss, long long local_dram, long long remote_dram, long long l2_miss, long long uops_retired, long long unhalted_cycles, long long remote_fwd, long long remote_hitm, long long instructions, long long context_switches, double sockets_bw1, double sockets_bw2, double sockets_bw3, double sockets_bw4, int number_of_threads);

int classify(classifier_data* data, classifier_data input);

classifier_data* init_classifier_data();

#endif
#ifdef __cplusplus
}
#endif
