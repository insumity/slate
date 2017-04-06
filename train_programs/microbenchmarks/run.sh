#!/bin/bash

for run in {1..48}
do
    sudo ./gather_data 1 $run /localhome/kantonia/slate/train_programs/microbenchmarks/loc -t  $run -s 1 -p 2>loc_pinned_with_rr_threads$run &
    sleep 16;
    sudo pkill -9 gather_data;
    sudo pkill -9 loc;
done	   
