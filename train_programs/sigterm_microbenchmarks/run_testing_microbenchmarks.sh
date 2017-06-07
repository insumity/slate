#!/bin/bash

# for i in $(seq 10 10)
# do
#     echo "going for loc"
#     pin=0
#     sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/clht_mb -t $i -s $pin -p 1>/dev/null 2>clht_mb_loc_$i & sleep 10; sudo pkill -INT clht_mb; sudo pkill -9 gather_data

#     echo "goinf for rr"
#     pin=1
#     sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/clht_mb -t $i -s $pin -p 1>/dev/null 2>clht_mb_rr_$i & sleep 10; sudo pkill -INT clht_mb; sudo pkill -9 gather_data

# done
 
# for i in $(seq 5 10)
# do
#     echo "going for loc"
#     pin=0
#     sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/lock -t $i -s $pin -p 1>/dev/null 2>lock_loc_$i & sleep 10; sudo pkill -INT lock; sudo pkill -9 gather_data

#     echo "going for rr"
#     pin=1
#     sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/lock -t $i -s $pin -p 1>/dev/null 2>lock_rr_$i & sleep 10; sudo pkill -INT lock; sudo pkill -9 gather_data

# done
 
for i in $(seq 5 10)
do
    echo "going for loc"
    pin=0
    sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/sort -t $i -s $pin -p 1>/dev/null 2>sort_loc_$i & sleep 10; sudo pkill -INT sort; sudo pkill -9 gather_data

    echo "going for rr"
    pin=1
    sudo ./gather_data 0 $pin $i /localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/sort -t $i -s $pin -p 1>/dev/null 2>sort_rr_$i & sleep 10; sudo pkill -INT sort; sudo pkill -9 gather_data

done
