#!/usr/bin/env python3
import csv
import sys
import glob
import math


filename = sys.argv[1]
f = open(filename, 'r')
reader = csv.reader(f, delimiter=',')

avg = 0
lines = 0
for row in reader:

    for i in range(0, 12):
        print(str(int(row[i])) + ", ", end='')

    loc = int(row[12])
    rr = int(row[13])

    result = 0
    
    is_loc = int(row[0]) == 1
    if (is_loc):
        if (rr - loc >= 2000):
            result = 1
        else:
            result = 0
    else:
        if (loc - rr >= 2000):
            result = 0
        else:
            result = 1
            
     
    #print(str(loc - rr ) + " (" + str(result) + ")")
    print(str(result))



