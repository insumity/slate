#!/bin/bash

for run in {1..48}
do
    sudo ./gather_data 0 $run /localhome/kantonia/slate/train_programs/microbenchmarks/rr -t  $run -r 1000 -s 0 -p 2>rr_pinned_with_loc_threads$run &
    sleep 16;
    sudo pkill -9 gather_data;
    sudo pkill -9 rr;
done	   
