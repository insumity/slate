#!/usr/bin/env python3

import os
import timeit

for i in range(1, 14):
    os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma")
    os.system("sudo ./gather_data PIN LOC_CORES ./bandwidth2 " + str(i) + " 2>LOC/bandwidth2_from_LOC_to_LOC" + str(i)) 

for i in range(14, 21):
    os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma")
    os.system("sudo ./gather_data PIN LOC_CORES ./bandwidth2 " + str(i) + " 2>RR/bandwidth2_from_LOC_to_RR" + str(i)) 

for i in range(2, 21):
    os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma")
    os.system("sudo ./gather_data PIN RR ./bandwidth2 " + str(i) + " 2>RR/bandwidth2_from_RR_to_RR" + str(i)) 


for i in range(2, 21):
    os.system("gcc locality_MIN_LAT_CORES.c -o locality -lpthread -lnuma")
    os.system("sudo ./gather_data PIN RR ./locality " + str(i) + " 2>LOC/locality_from_RR_to_LOC" + str(i)) 
    os.system("sudo ./gather_data PIN LOC_CORES ./locality " + str(i) + " 2>LOC/locality_from_LOC_to_LOC" + str(i)) 


