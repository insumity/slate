#include <stdio.h>
#include <stdlib.h>

// results[0] = L3_HIT;
// results[1] = L3_MISS;
// results[2] = LOCAL_DRAM;
// results[3] = REMOTE_DRAM;
// results[4] = L2_MISS;
// results[5] = UOPS_RETIRED;
// results[6] = UNHALTED_CORE_CYCLES;
// results[7] = REMOTE_FWD;
// results[8] = REMOTE_HITM;
  

long long* introduce_features(long long* values) {
  long long L3_HIT = values[0];
  long long L3_MISS = values[1];
  long long LOCAL_DRAM = values[2];
  long long REMOTE_DRAM = valules[3];
  long long L2_MISS = values[4];
  long long UOPS_RETIRED = values[5];
  long long UNHALTED_CORE_CYCLES = values[6];
  long long REMOTE_FWD = 0;
  long long REMOTE_HITM = 0;

  
  double LLC_miss_rate = L3_

}
