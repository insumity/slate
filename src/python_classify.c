#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <numaif.h>
#include <papi.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>


#include "python_classify.h"

#define REQUEST_UNANSWERED -1


int classify(classifier_data* data, classifier_data input) {

  data->LOC = input.LOC;
  data->RR = input.RR;
  data->l3_hit = input.l3_hit;
  data->l3_miss = input.l3_miss;
  data->local_dram = input.local_dram;
  data->remote_dram = input.remote_dram;
  data->l2_miss = input.l2_miss;
  data->uops_retired = input.uops_retired;
  data->unhalted_cycles = input.unhalted_cycles;
  data->remote_fwd = input.remote_fwd;
  data->remote_hitm = input.remote_hitm;
  data->instructions = input.instructions;
  data->context_switches = input.context_switches;

  for (int i = 0; i < 4; ++i) {
    data->sockets_bw[i] = input.sockets_bw[i];
  }
  
  data->number_of_threads = input.number_of_threads;
  data->response = REQUEST_UNANSWERED;

  //printf("RESPONSE before classification: %d\n", data->response);
  usleep(50 * 1000);
  int res = data->response;
  printf("RESPONSE after classification: %d\n", res);

  return res;
}

classifier_data create_data(int LOC, int RR, long long l3_hit, long long l3_miss, long long local_dram, long long remote_dram, long long l2_miss, long long uops_retired, long long unhalted_cycles, long long remote_fwd, long long remote_hitm, long long instructions, long long context_switches, double sockets_bw1, double sockets_bw2, double sockets_bw3, double sockets_bw4, int number_of_threads) {

  classifier_data input;
  input.LOC = LOC;
  input.RR = RR;
  input.l3_hit = l3_hit;
  input.l3_miss = l3_miss;
  input.local_dram = local_dram;
  input.remote_dram = remote_dram;
  input.l2_miss = l2_miss;
  input.uops_retired = uops_retired;
  input.unhalted_cycles = unhalted_cycles;
  input.remote_fwd = remote_fwd;
  input.remote_hitm = remote_hitm;
  input.instructions = instructions;
  input.context_switches = context_switches;

  input.sockets_bw[0] = sockets_bw1;
  input.sockets_bw[1] = sockets_bw2;
  input.sockets_bw[2] = sockets_bw3;
  input.sockets_bw[3] = sockets_bw4;
  
  input.number_of_threads = number_of_threads;

  return input;
}


classifier_data* init_classifier_data() {
  int fd = open("/tmp/classifier", O_RDWR, 0777);
  
  classifier_data* data = mmap(0, sizeof(classifier_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("couldn't memory map in classify\n");
    exit(EXIT_FAILURE);
  }

  return data;

  /* classifier_data input = create_data(1, 0, 2725282, 1667, 89, 72, 79834194, 3100781160, 23752689518, 33, 620, 1, 55.095, 38.17, 46.19, 46.755, 10); */
  /* input.response = -1; */
  
  /* int res = classify(data, input); */

  /* printf("RESPONSE after classification: %d\n", res); */



  /* input = create_data(1, 0, 28475414, 17787077, 16205992, 211387, 71191923, 25140067766, 47573035950, 66, 495, 470646, 4148.56, 151.38, 215.975, 118.575, 11); */
  /* input.response = -1; */

  /* res = classify(data, input); */
  /* printf("RESPONSE after classification: %d\n", res); */



  /* close(fd); */

  /* return 0; */
}
