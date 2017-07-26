#!/bin/sh

num_threads=$1
export IM_CONCURRENCY=$num_threads
shift
builddir=$1
shift
program=$@

GCONV_PATH="${builddir}/iconvdata" \
LD_PRELOAD="/lib64/ld-linux-x86-64.so.2 /lib/x86_64-linux-gnu/libuuid.so.1 /usr/lib/x86_64-linux-gnu/libICE.so.6 /usr/lib/x86_64-linux-gnu/libSM.so.6 /usr/lib/x86_64-linux-gnu/libXt.so.6 /usr/lib/x86_64-linux-gnu/libXdmcp.so.6 /usr/lib/x86_64-linux-gnu/libXau.so.6 /usr/lib/x86_64-linux-gnu/libxcb.so.1 /usr/lib/x86_64-linux-gnu/libX11.so.6 /usr/lib/x86_64-linux-gnu/libXext.so.6 /usr/lib/x86_64-linux-gnu/libXmu.so.6 /lib/x86_64-linux-gnu/libacl.so.1 /usr/lib/x86_64-linux-gnu/libnuma.so.1 /lib/x86_64-linux-gnu/libselinux.so.1 /lib/x86_64-linux-gnu/libattr.so.1 /lib/x86_64-linux-gnu/libpcre.so.3 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /lib/x86_64-linux-gnu/libgcc_s.so.1" exec   env GCONV_PATH="${builddir}"/iconvdata LOCPATH="${builddir}"/localedata LC_ALL=C  "${builddir}"/elf/ld-linux-x86-64.so.2 --library-path "${builddir}":"${builddir}"/math:"${builddir}"/elf:"${builddir}"/dlfcn:"${builddir}"/nss:"${builddir}"/nis:"${builddir}"/rt:"${builddir}"/resolv:"${builddir}"/crypt:"${builddir}"/mathvec:"${builddir}"/nptl $program
