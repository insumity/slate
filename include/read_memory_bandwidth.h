#ifdef __cplusplus
extern "C" {
#endif

#ifndef __READ_MEMORY_BANDWIDTH_H
#define __READ_MEMORY_BANDWIDTH_H

  FILE* start_reading_memory_bandwidth();

  double read_memory_bandwidth(int socket, FILE* bw_file);

  void close_memory_bandwidth();
  
#endif

#ifdef __cplusplus
}
#endif

