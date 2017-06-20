#!/bin/bash

files="/localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/new_loc_results/loc*"
for file in $files
do
    egrep -v "(o|^$)" $file | tail -n 5
done

