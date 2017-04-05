#!/usr/bin/env python3

import csv
import sys
import glob
import math

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1




all_RRs = glob.glob("./LOC/*to_RR*") + glob.glob("./RR/*to_RR*")

mean_sum_all = [0] * 10
total = 0
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0
    for row in reader:
        
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            k = 0
            for col in row:
                mean_sum_all[k] += int(col)
                k = k + 1
            total += 1
        rownum += 1


all_RRs = glob.glob("./LOC/*to_LOC*") + glob.glob("./RR/*to_LOC*")
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0

    for row in reader:
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            k = 0
            for col in row:
                mean_sum_all[k] += int(col)
                k = k + 1
            total += 1
        rownum += 1


# calculate mean and std for LOC
all_RRs = glob.glob("./LOC/*to_RR*") + glob.glob("./RR/*to_RR*")

mean = [0] * 10
print("double mean[11] = {", end='')
for i in range(0, 10):
    mean[i] = mean_sum_all[i] / float(total)
    print(str(mean[i]) + ", ", end='')
print(" 0};")

std_sum_all = [0] * 10
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0
    for row in reader:
        
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            k = 0
            for col in row:
                std_sum_all[k] += ((int(col) - mean[k]) ** 2.0)
                k = k + 1
        rownum += 1


all_RRs = glob.glob("./LOC/*to_LOC*") + glob.glob("./RR/*to_LOC*")
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0

    for row in reader:
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            k = 0
            for col in row:
                std_sum_all[k] += ((int(col) - mean[k]) ** 2.0)
                k = k + 1
        rownum += 1





# get std per feature
std = [0] * 10
print("double std[11] = {", end='')
for i in range(0, 10):
    std[i] = math.sqrt(std_sum_all[i] / float(total))
    print(str(std[i]) + ", ", end='')
print(" 0};")

# print the actual results
all_RRs = glob.glob("./LOC/*to_RR*") + glob.glob("./RR/*to_RR*")
result = 'RR'
data = 0
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0
    for row in reader:
        
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            columnnum = 0
            print("ME_Sample dt" + str(data) + "(\"" + result + "\");")
            if "from_LOC" in RR_filename:
                print("dt" + str(data) + ".add_feature(\"LOC\", 1);")
                sys.stderr.write("1, ")

                print("dt" + str(data) + ".add_feature(\"RR\", 0);")
                sys.stderr.write("0, ")

            else:
                print("dt" + str(data) + ".add_feature(\"LOC\", 0);")
                sys.stderr.write("0, ")

                print("dt" + str(data) + ".add_feature(\"RR\", 1);")
                sys.stderr.write("1, ")

                
            k = 0
            for col in row:
                resy = (int(col) - float(mean[k])) / std[k]
                print("dt" + str(data) + ".add_feature(\"" + header[columnnum] + "\", " + str(resy) + ");")
                sys.stderr.write(str(resy) + ", ")

                k += 1
                columnnum += 1
                
            sys.stderr.write("0\n")

            print("model.add_training_sample(dt" + str(data) + ");")
            data += 1
        rownum += 1
    

                      
all_RRs = glob.glob("./LOC/*to_LOC*") + glob.glob("./RR/*to_LOC*")
result = 'LOC'
for RR_filename in all_RRs:
    f = open(RR_filename, 'r')
    reader = csv.reader((line.replace('\t\t', '\t') for line in f), delimiter='\t')
    rownum = 0

    for row in reader:
        if rownum == 0:
            header = row
        else:
            foo = True
            for col in row:
                if float(col) < 0:
                    foo = False
            if not foo:
                break

            columnnum = 0
            print("ME_Sample dt" + str(data) + "(\"" + result + "\");")
            if "from_LOC" in RR_filename:
                print("dt" + str(data) + ".add_feature(\"LOC\", 1);")
                sys.stderr.write("1, ")

                print("dt" + str(data) + ".add_feature(\"RR\", 0);")
                sys.stderr.write("0, ")

            else:
                print("dt" + str(data) + ".add_feature(\"LOC\", 0);")
                sys.stderr.write("0, ")

                print("dt" + str(data) + ".add_feature(\"RR\", 1);")
                sys.stderr.write("1, ")


            k = 0
            for col in row:
                resy = (int(col) - float(mean[k])) / std[k]
                print("dt" + str(data) + ".add_feature(\"" + header[columnnum] + "\", " + str(resy) + ");")
                sys.stderr.write(str(resy) + ", ")
                k += 1
                columnnum += 1
            sys.stderr.write("1\n")
            print("model.add_training_sample(dt" + str(data) + ");")
            data += 1
        rownum += 1
