#!/bin/bash

FILES="*_results/*_pinned_*"

for f in $FILES
do
    tail -n 28 $f | head -n 20 | egrep -v "(pin|could|Core|-)"
done

