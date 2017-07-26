#!/usr/bin/env python3
import re
import sys

time = float(sys.argv[1])
f = open("bw_input", "r")

r = re.compile('[ \t\n\r:]+')
summy = 0.0
while True:
    line1 = f.readline()
    if not line1:
        break
    line2 = f.readline()
    v1 = float(r.split(line1)[1])
    v2 = float(r.split(line2)[1])

    summy = summy + v1 + v2
    summi = (((v1 + v2) * 64) / time) / (1024.0 * 1024.0)
    print(str(v1) + " " + str(v2) +":\t" + str(summi))
          

print("Total: " + str(((summy * 64) / time) / (1024.0 * 1024.0)))
f.close()
