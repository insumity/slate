# scheduler

**Why we changed the pthread library?**

When MCTOP was introduced in the `wc` application of METIS, the `set_mempolicy` functionality was used.
We do not know of any way to call `set_mempolicy` for another process. To solve this problem, we change the pthread libary (nptl) of glibc and then run applications by linking them to our own library. By doing so, we can inject code in the `pthread_create` function that the application calls without its knowledge. To be exact, when an application calls `pthread_create(..., function, ...)`, we call `injected_function` that runs some extra code before the actual `function` gets executed. This extra code calls `set_mempolicy` and `set_schedaffinity` for the corresponding thread.

**How does the pthread library communicate with the scheduler?**

The idea is the following. Both the pthread and the scheduler issue read and writes to the same file (`/tmp/scheduler`) that is memory mapped by both. I couldn't use shared memory (`shm_open`) because it seems that `lrt`(the shared memory component of glibc) was using threads, so there was a cycle I couldn't kill.
This files contain some slots (`communication_slot`) and their access is controlled by semaphores.
The protocol is the following:

1. The scheduler initializes its slots to `NONE` (meaning noone is using them)
2. When the pthread library creates a new thread, it uses an emtpy slot and changes is it to `PTHREADS`
3. The scheduler keeps on checking the slots: When it reads a `PTHREADS` slot, it changes its `core` and `node` values and changes the slot to `SCHEDULER`
4. The pthread library sees that its slots got changed to `SCHEDULER`, so it gets the data (`core` and `node`) and changes its affinity and memory policy. Afterwards it erases the slot by making it as a `NONE`


The "Assymatry Matters" papers uses 2 programs from PARSEC: swaptions and streamcluster. They can be executed as follows:
/extra/parsec-3.0/bin/parsecmgmt -a run -p streamcluster -i simlarge -n A (with n = 5 takes about 22seconds)
/extra/parsec-3.0/bin/parsecmgmt -a run -p swaptions -i simlarge -n B (with n = 4 takes about 10 secons)
where A and B is the minimum amount of threads. Increasing them will increase the total execution time.

cat ../parsec-3.0/log/amd64-linux.gcc/* | ./read_parsec.py  |  awk -F ', '  '{   sum=sum+$1 ; sumX2+=(($1)^2)} END { printf "Average: %f. Standard Deviation: %f \n", sum/NR, sqrt(sumX2/(NR) - ((sum/NR)^2) )}'     
MCTOP_ALLOC_BW_ROUND_ROBIN_CORES /home/kantonia/Metis/obj/wc /home/kantonia/Metis/300MB_1M_Keys.txt -p 6  


MCTOP_ALLOC_BW_ROUND_ROBIN_CORES /localhome/kantonia/METIS_local/wc /localhome/kantonia/METIS_local/300MB_1M_Keys.txt -p 6  



MCTOP_ALLOC_BW_ROUND_ROBIN_CORES /localhome/kantonia/glibc-build2/testapp.sh


MCTOP_ALLOC_BW_ROUND_ROBIN_CORES /localhome/kantonia/glibc-build2/testrun.sh /localhome/kantonia/METIS_local/wc /localhome/kantonia/METIS_local/300MB_1M_Keys.txt -p 6 

LD_PRELOAD="/lib/x86_64-linux-gnu/libacl.so.1 /lib/x86_64-linux-gnu/libselinux.so.1 /lib/x86_64-linux-gnu/libattr.so.1 /lib/x86_64-linux-gnu/libpcre.so.3" ./testrun.sh /bin/ls

LD_PRELOAD="/lib/x86_64-linux-gnu/libacl.so.1 /lib/x86_64-linux-gnu/libselinux.so.1 /lib/x86_64-linux-gnu/libattr.so.1 /lib/x86_64-linux-gnu/libpcre.so.3 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /lib/x86_64-linux-gnu/libgcc_s.so.1" ./testrun.sh /localhome/kantonia/METIS_local/wc /localhome/kantonia/METIS_local/300MB_1M_Keys.txt -p 6 


