#!/bin/bash

for run in {1..2}
do
    sudo ./gather_data 1 $run /localhome/kantonia/slate/train_programs/microbenchmarks/rr -t  $run -r 1000 -s 1 -p 2>rr_pinned_rr_threads$run &
    sleep 10;
    sudo pkill -9 gather_data;
    sudo pkill -9 rr;
done	   
