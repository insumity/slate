#!/bin/bash

execution_time=20
minimum_number_of_threads=11
maximum_number_of_threads=13

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc;
    sudo pkill -9 gather_data;
    mv total_loops_* loc1_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc;
    sudo pkill -9 gather_data;
    mv total_loops_* loc1_results/
done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc2;
    sudo pkill -9 gather_data;
    mv total_loops_* loc2_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_rr_threads$threads &


    sleep $execution_time;
    sudo pkill -INT loc2;
    sudo pkill -9 gather_data;
    mv total_loops_* loc2_results/
done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc3;
    sudo pkill -9 gather_data;
    mv total_loops_* loc3_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc3;
    sudo pkill -9 gather_data;
    mv total_loops_* loc3_results/
done

execution_time=$((execution_time+30))
for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr;
    sudo pkill -9 gather_data;
    mv total_loops_* rr1_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr;
    sudo pkill -9 gather_data;
    mv total_loops_* rr1_results/
done
execution_time=$((execution_time-30))

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_loc_threads$threads &

    
    sleep $execution_time;
    sudo pkill -INT rr2;
    sudo pkill -9 gather_data;
    mv total_loops_* rr2_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr2;
    sudo pkill -9 gather_data;
    mv total_loops_* rr2_results/
done	   

