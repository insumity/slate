#!/bin/bash

bash tools/clean.sh

# Generate wr input with many keys and many duplicates
dd if=data/wr/100MB_1M_Keys.txt of=data/wr/5MB.txt~ count=1 bs=5000000
i=0
cp data/wr/5MB.txt~ data/wr/800MB.txt
while [ "$i" -lt "160" ]; do
  cat data/wr/5MB.txt~ >> data/wr/800MB.txt
  i=$((i+1))
done
rm data/wr/5MB.txt~

# Generate wr input with many keys and many duplicates, but unpredicatable
make obj/tools/gen
./obj/tools/gen 500000 4 > data/wr/500MB.txt

# Generate hist input
i=0
while [ "$i" -lt "887" ]; do
  cat data/3MB.bmp >> data/hist-2.6g.bmp
  i=$((i+1))
done

# Generate linear regression input
dd if=/dev/urandom of=data/lr_10MB.txt~ count=1024 bs=10240
i=0
while [ "$i" -lt "400" ]; do
  cat data/lr_10MB.txt~ >> data/lr_4GB.txt
  i=$((i+1))
done
rm data/lr_10MB.txt~

# Generate string match input
i=0
while [ "$i" -lt "100" ]; do
  cat data/wc/10MB.txt >> data/sm_1GB.txt
  i=$((i+1))
done

