#!/bin/bash

for run in {1..48}
do
    rr_result=$(tail -n 5 loc1_results/loc_pinned_with_rr_threads$run | awk '{ sum += $3 } END { if (NR > 0) print sum / NR }')
    rr_result_n=$(printf '%.0f' $rr_result)
    echo "pame gia locality tora ..." 
    loc_result=$(tail -n 5 loc1_results/loc_pinned_with_loc_threads$run | awk '{ sum += $3 } END { if (NR > 0) print sum / NR }')
    loc_result_n=$(printf '%.0f' $loc_result)

    echo $rr_result " " $loc_result
    echo $rr_result_n " " $loc_result_n
    diff=`expr $rr_result_n - $loc_result_n`
    echo "let's go to the next one: " $run " difference: " $diff
done
