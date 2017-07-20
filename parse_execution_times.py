#!/usr/bin/env python3

import re

def remove_last_char(s):
    return s[0:len(s) - 1]

def remove_last_two_chars(s):
    return s[0:len(s) - 2]


with open("execution_times") as f:
    content = f.readlines()

    for line in content:
        l = line.split(' ')

        app = l[0].split('_')[1]
        threads = l[0].split('_')[2]
        policy = int(remove_last_char(l[2]))
        avg = float(remove_last_char(l[7]))
        std = float(remove_last_two_chars(l[10]))

        pol = "mem"
        if policy == 0:
            pol = "loc"

#        print(line)
        print(app.lower() + "_times[" + str(threads) + "][\"" + str(pol) + "\"] = " + str(avg))
        print(app.lower() + "_times[" + str(threads) + "][\"" + str(pol) + "\"][\"std\"] = " + str(std))

