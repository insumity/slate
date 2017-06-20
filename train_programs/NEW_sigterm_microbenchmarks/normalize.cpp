#include <stdio.h>
#include <stdlib.h>

#include <iostream>

using namespace std;

typedef struct {
  int LOC, RR;
  long long l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired, unhalted_cycles, remote_fwd, remote_hitm;
  long long context_switches;
  double sockets_bw[4];
  int number_of_threads;
} my_features;

typedef struct {
  my_features feat;

  double llc_miss_rate, llc_miss_rate_square;
  double local_memory_rate, local_memory_rate_square;
  double intra_socket, intra_socket_square;
  double inter_socket, inter_socket_square;

  double llc_miss_rate_times_local_memory_rate;
  
} extended_features;

double* normalize_features(extended_features ef)
{
  double* normalized_features = (double*) malloc(26 * sizeof(double));
  my_features f = ef.feat;

  int k = 0;
  normalized_features[k++] = f.LOC;
  normalized_features[k++] = f.RR;
  normalized_features[k++] = f.l3_hit / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.l3_miss / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.local_dram / ((double) f.l3_miss);
  normalized_features[k++] = f.remote_dram / ((double) f.l3_miss);
  normalized_features[k++] = f.l2_miss / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.uops_retired / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.unhalted_cycles / (1000. * 1000. * 1000.);
  normalized_features[k++] = f.remote_fwd / ((double) f.l3_miss);
  normalized_features[k++] = f.remote_hitm / ((double) f.l3_miss);

  normalized_features[k++] = f.context_switches;

  for (int s = 0; s < 4; ++s) {
    normalized_features[k++] = f.sockets_bw[s];
  }

  normalized_features[k++] = f.number_of_threads;
  
  normalized_features[k++] = ef.llc_miss_rate;
  normalized_features[k++] = ef.llc_miss_rate_square;
  normalized_features[k++] = ef.local_memory_rate;
  normalized_features[k++] = ef.local_memory_rate_square;
  normalized_features[k++] = ef.intra_socket / f.l2_miss; // normalize
  normalized_features[k++] = (ef.intra_socket / f.l2_miss) * (ef.intra_socket / f.l2_miss);
  normalized_features[k++] = ef.inter_socket / f.l3_miss; // normalize
  normalized_features[k++] = (ef.inter_socket / f.l3_miss) * (ef.inter_socket / f.l3_miss);
  normalized_features[k++] = ef.llc_miss_rate_times_local_memory_rate;

  ///normalized_features[k++] = 0; //f.sockets_bw[0] + f.sockets_bw[1] + f.sockets_bw[2] + f.sockets_bw[3]);
  //normalized_features[k++] = 0; //((f.local_dram + f.remote_dram) / ((double) f.l3_miss)) * ef.llc_miss_rate;

  // 28 so far
  //normalized_features[k++] = (f.uops_retired / ((double) f.unhalted_cycles)) * (f.uops_retired / ((double) f.unhalted_cycles));
  //normalized_features[k++] = (f.unhalted_cycles / (- 100.)) * (f.unhalted_cycles / (- 100.));


  return normalized_features;
}

extended_features introduce_features(my_features f)
{
  extended_features ef;
  ef.feat = f;
  
  ef.llc_miss_rate = (f.l3_hit + f.l3_miss > 0)? f.l3_miss / ((double) f.l3_hit + f.l3_miss): 0;
  ef.llc_miss_rate_square = ef.llc_miss_rate * ef.llc_miss_rate;
  ef.local_memory_rate = (f.local_dram + f.remote_dram > 0)? f.local_dram / ((double) f.local_dram + f.remote_dram): 0;
  ef.local_memory_rate_square = ef.local_memory_rate * ef.local_memory_rate;

  double intra_socket = f.l2_miss - (f.l3_hit + f.l3_miss);
  ef.intra_socket = intra_socket >= 0? intra_socket: 0;
  ef.intra_socket_square = ef.intra_socket * ef.intra_socket;
  ef.inter_socket = f.remote_fwd + f.remote_hitm;
  ef.inter_socket_square = ef.inter_socket * ef.inter_socket;

  ef.llc_miss_rate_times_local_memory_rate = ef.llc_miss_rate * ef.local_memory_rate;

  return ef;
}


int main(int argc, char** argv)
{
  FILE* fp = fopen(argv[1], "r");

  char line[5000];
  while (fgets(line, 5000, fp) != NULL) {
    int loc, rr;
    long long int new_values[11];
    int final_result;
    double socket_values[4];

    sscanf(line, "%d, %d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lld, %d", &loc, &rr,
	   &new_values[0], &new_values[1], &new_values[2], &new_values[3], &new_values[4],
	   &new_values[5], &new_values[6], &new_values[7], &new_values[8], &new_values[9],
	   &socket_values[0], &socket_values[1], &socket_values[2], &socket_values[3], &new_values[10],
	   &final_result);


    // printf("=== > %d, %d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lld, %d\n", loc, rr,
    // 	   new_values[0], new_values[1], new_values[2], new_values[3], new_values[4],
    // 	   new_values[5], new_values[6], new_values[7], new_values[8], new_values[9],
    // 	   socket_values[0], socket_values[1], socket_values[2], socket_values[3], new_values[10],
    // 	   final_result);

    my_features feat;
    feat.LOC = loc;
    feat.RR = rr;
    feat.l3_hit = new_values[0];
    feat.l3_miss = new_values[1];
    feat.local_dram = new_values[2];
    feat.remote_dram = new_values[3];
    feat.l2_miss = new_values[4];
    feat.uops_retired = new_values[5];
    feat.unhalted_cycles = new_values[6];
    feat.remote_fwd = new_values[7];
    feat.remote_hitm = new_values[8];
    feat.context_switches = new_values[9];

    for (int i = 0; i < 4; ++i) {
      feat.sockets_bw[i] = socket_values[i];
    }

    feat.number_of_threads = new_values[10];

    double* normalized = normalize_features(introduce_features(feat));

    for (int k = 0; k < 26; ++k) {
      cout << normalized[k] << ", ";
    }
    cout << final_result << "\n";
  }
  fclose(fp);


  return 0;
}
