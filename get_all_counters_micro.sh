#!/bin/bash

DEMO_DIR="/localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks"

policies=( 0 1 )
for POLICY in "${policies[@]}"
do
    pkill -9 python

    (cd $DEMO_DIR && numactl --physcpubind=95 ./demo_poly.py testing2.csv $POLICY &)

    sleep 2 # wait 2 seconds to make sure the model got trained

    echo "about to run application"
#    files=( "ONE_LOC1_6" "ONE_LOC1_10" "ONE_LOC1_20" )
#    files=( "ONE_LOC2_6" "ONE_LOC2_10" "ONE_LOC2_20" "ONE_LOC3_6" "ONE_LOC3_10" "ONE_LOC3_20" "ONE_LOC4_6" "ONE_LOC4_10" "ONE_LOC4_20" "ONE_LOC5_6" "ONE_LOC5_10" "ONE_LOC5_20" )
#    files=( "ONE_RR1_6" "ONE_RR1_10" "ONE_RR1_20" )
#    files=( "ONE_RR2_6" "ONE_RR2_10" "ONE_RR2_20" )
#    files=( "ONE_RR3_6" "ONE_RR3_10" "ONE_RR3_20" )
    files=( "ONE_RR4_6" "ONE_RR4_10" "ONE_RR4_20" "ONE_RR5_6" "ONE_RR5_10" "ONE_RR5_20" "ONE_RR6_6" "ONE_RR6_10" "ONE_RR6_20" )
    iterations=( 1 2 3 )

    for APP_FILE_NAME in "${files[@]}"
    do
	for ITERATION in "${iterations[@]}"
	do
	    sudo ./schedule $APP_FILE_NAME foo. GREEDY true $POLICY 2>gathered_counter_results/$APP_FILE_NAME.pol_$POLICY.$ITERATION.results &
	    sleep 60
	    sudo pkill -9 loc
	    sudo pkill -9 rr
	    sudo pkill -9 schedule
	    sudo pkill -9 ld-linux-x86-64
	    sudo pkill -9 ld-linux-x86
	done
    done

    pkill -9 python
done
