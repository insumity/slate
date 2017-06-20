#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GATHER_DATA_H
#define __GATHER_DATA_H



  long long* subtract(long long* ar1, long long* ar2, int length);


  void print_perf_counters(bool is_rr, int result, long long values[], long long context_switches, double sockets_bw[]);


  // returns the current values of the read performance counters
  long long* read_perf_counters(long long values[], std::vector<int> hwcs);


#endif

#ifdef __cplusplus
}
#endif

