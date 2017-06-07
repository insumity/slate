#!/bin/bash

execution_time=20
minimum_number_of_threads=2
maximum_number_of_threads=21

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc1_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc1_results/
# done	   

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc2;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc2_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_rr_threads$threads &


#     sleep $execution_time;
#     sudo pkill -INT loc2;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc2_results/
# done	   

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc4 -t $threads -s $policy -p 2>loc4_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc4;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc4_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc4 -t $threads -s $policy -p 2>loc4_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc4;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc4_results/
# done

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc5 -t $threads -s $policy -p 2>loc5_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc5;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc5_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc5 -t $threads -s $policy -p 2>loc5_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc5;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc5_results/
# done

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc3;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc3_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT loc3;
#     sudo pkill -9 gather_data;
#     mv total_loops_* loc3_results/
# done

# execution_time=$((execution_time+30))
# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=1
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr1_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr1_results/
# done
# execution_time=$((execution_time-30))

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=1
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_loc_threads$threads &

    
#     sleep $execution_time;
#     sudo pkill -INT rr2;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr2_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr2;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr2_results/
# done	   


# execution_time=$((execution_time+30))
# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=1
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr3 -t $threads -s $policy -p 2>rr3_results/rr_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr3;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr3_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr3 -t $threads -s $policy -p 2>rr3_results/rr_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr3
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr3_results/
# done
# execution_time=$((execution_time-30))


# execution_time=$((execution_time+5))
# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=1
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr4 -t $threads -s $policy -p 2>rr4_results/rr_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr4;
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr4_results/


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr4 -t $threads -s $policy -p 2>rr4_results/rr_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -INT rr4
#     sudo pkill -9 gather_data;
#     mv total_loops_* rr4_results/
# done
# execution_time=$((execution_time-5))

execution_time=$((execution_time+2))
for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr5 -t $threads -s $policy -p 2>rr5_results/rr_pinned_with_loc_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr5;
    sudo pkill -9 gather_data;
    mv total_loops_* rr5_results/


    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/rr5 -t $threads -s $policy -p 2>rr5_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -INT rr5
    sudo pkill -9 gather_data;
    mv total_loops_* rr5_results/
done
execution_time=$((execution_time-2))
