#!/bin/sh

LD_PRELOAD="/lib/x86_64-linux-gnu/libacl.so.1 /lib/x86_64-linux-gnu/libselinux.so.1 /lib/x86_64-linux-gnu/libattr.so.1 /lib/x86_64-linux-gnu/libpcre.so.3 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /lib/x86_64-linux-gnu/libgcc_s.so.1" /localhome/kantonia/glibc-build2/testrun.sh /localhome/kantonia/METIS_local/wc /localhome/kantonia/METIS_local/300MB_1M_Keys.txt -p 6
