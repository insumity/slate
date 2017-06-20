#!/bin/bash

execution_time=10
minimum_number_of_threads=2
increment=1
maximum_number_of_threads=24

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>new_loc1_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc1_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>new_loc1_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc1_results/
done	   

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>new_loc2_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc2;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc2_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>new_loc2_results/loc_pinned_with_rr_threads$threads &


    sleep $execution_time;
    sudo pkill -INT loc2;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc2_results/
done

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>new_loc3_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc3;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc3_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>new_loc3_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc3;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc3_results/
done

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc4 -t $threads -s $policy -p 2>new_loc4_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc4;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc4_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc4 -t $threads -s $policy -p 2>new_loc4_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc4;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc4_results/
done

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc5 -t $threads -s $policy -p 2>new_loc5_results/loc_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc5;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc5_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/loc5 -t $threads -s $policy -p 2>new_loc5_results/loc_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT loc5;
    sudo pkill -9 gather_data;
    mv total_loops_* new_loc5_results/
done


# RR
execution_time=$((execution_time+30))
for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>new_rr1_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr1_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>new_rr1_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr1_results/
done
execution_time=$((execution_time-30))

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>new_rr2_results/rr_pinned_with_loc_threads$threads &

    
    sleep $execution_time;
    sudo pkill -INT rr2;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr2_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>new_rr2_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr2;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr2_results/
done	   


execution_time=$((execution_time+30))
for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr3 -t $threads -s $policy -p 2>new_rr3_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr3;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr3_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr3 -t $threads -s $policy -p 2>new_rr3_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr3
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr3_results/
done
execution_time=$((execution_time-30))


execution_time=$((execution_time+5))
for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr4 -t $threads -s $policy -p 2>new_rr4_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr4;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr4_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr4 -t $threads -s $policy -p 2>new_rr4_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr4
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr4_results/
done
execution_time=$((execution_time-5))

execution_time=$((execution_time+1))
for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr5 -t $threads -s $policy -p 2>new_rr5_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr5;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr5_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr5 -t $threads -s $policy -p 2>new_rr5_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr5
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr5_results/
done
execution_time=$((execution_time-1))

execution_time=$((execution_time+30))
for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr6 -t $threads -s $policy -p 2>new_rr6_results/rr_pinned_with_loc_threads$threads &
    
    sleep $execution_time;
    sudo pkill -INT rr6;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr6_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/rr6 -t $threads -s $policy -p 2>new_rr6_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr6;
    sudo pkill -9 gather_data;
    mv total_loops_* new_rr6_results/
done
execution_time=$((execution_time-30))
