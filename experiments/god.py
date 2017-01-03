#!/usr/bin/env python3
import sys
import string
import os
import time
import tempfile
import csv
import copy
import math

run_programs_concurrently = False
ITERATIONS=2
GLIBC_BUILD_DIRECTORY = "/home/kantonia/GLIBC/glibc-build/"
NONE_SCHEDULER_FILE = "none_scheduler"
LINUX_SCHEDULER_FILE = "linux_scheduler"
SLATE_SCHEDULER_FILE = "slate_scheduler"

experiment_name = sys.argv[1]
input_programs_file = sys.argv[2]
plotted_figure_file = sys.argv[3]

if not os.path.exists(experiment_name):
    os.makedirs(experiment_name)
else:
    print("A directory with the same experiment name: " + experiment_name + ", already exists")
    sys.exit(-1)

os.system("cp " + input_programs_file + " " + experiment_name + "/" + input_programs_file)

none_scheduler_file = os.path.join(experiment_name, NONE_SCHEDULER_FILE)
slate_scheduler_file = os.path.join(experiment_name, SLATE_SCHEDULER_FILE)
linux_scheduler_file = os.path.join(experiment_name, LINUX_SCHEDULER_FILE)
os.system("./experiment.py " + input_programs_file + " " + GLIBC_BUILD_DIRECTORY + " " 
          + none_scheduler_file + " " + slate_scheduler_file + " " + linux_scheduler_file)


def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

def get_line(fname, n):
    with open(fname) as fp:
        for i, line in enumerate(fp):
            if i == n:
                return line

def execute_one_by_one(scheduler_file, output_file):
    lines = file_len(scheduler_file)
    for i in range(0, lines):
        file_name = ""
        f = tempfile.NamedTemporaryFile(mode='w', delete=False)
        print("Line " + str(i) + " .. of file: " + get_line(scheduler_file, i))
        line = get_line(scheduler_file, i)
        f.write(line)
        file_name = f.name
        f.flush()
        print("The file that was used is: " + f.name);
        f.close()
        os.system("../schedule " + f.name + " " + f.name + ".results")
        os.system("cat " + f.name + ".results >> " + output_file)
        os.remove(file_name)
        os.remove(file_name + ".results")


def execute_linux_one_by_one(scheduler_file, output_file):
    lines = file_len(scheduler_file)
    for i in range(0, lines):
        file_name = ""
        f = tempfile.NamedTemporaryFile(mode='w', delete=False)
        print("Line " + str(i) + " .. of file: " + get_line(scheduler_file, i))
        line = get_line(scheduler_file, i)
        f.write(line)
        file_name = f.name
        f.flush()
        f.close()
        os.system("./use_linux_scheduler " + f.name + " " + f.name + ".results")
        os.system("cat " + f.name + ".results >> " + output_file)
        os.remove(file_name)
        os.remove(file_name + ".results")



            
if run_programs_concurrently:
    for i in range(ITERATIONS):
        print("Going to the slate scheduler.") 
        os.system("../schedule " + slate_scheduler_file + " " + slate_scheduler_file + str(i) + ".results")
        #print("Going to the none-slate scheduler.")
        #os.system("../schedule " + none_scheduler_file + " " + none_scheduler_file + str(i) + ".results")
        print("Going to the Linux scheduler.") 
        os.system("./use_linux_scheduler " + linux_scheduler_file + " " + linux_scheduler_file + str(i) + ".results")

        #merge results to a results_file
        os.system("./merge.py " #+ none_scheduler_file + ".results" + " none 0 2 6 9 " ifUNCOe line below as well
          + slate_scheduler_file + str(i) + ".results" " slate 0 2 7 10 "
                  + linux_scheduler_file + str(i) + ".results" " Linux 0 2 6 9 >  " +
                  experiment_name + "/merged_results" + str(i))
    
else:
    # TODO many iterations
    for i in range(ITERATIONS):
        print("Going to the slate scheduler.") 
        execute_one_by_one(slate_scheduler_file, slate_scheduler_file + str(i) + ".results")
        #print("Going to the none-slate scheduler.")
        #execute_one_by_one(none_scheduler_file)
        print("Going to the Linux scheduler.")
        execute_linux_one_by_one(linux_scheduler_file, linux_scheduler_file + str(i) + ".results")
        
        #merge results to a results_file
        os.system("./merge.py " #+ none_scheduler_file + ".results" + " none 0 2 6 9 " ifUNCOe line below as well
          + slate_scheduler_file + str(i) + ".results" " slate 0 2 7 10 "
                  + linux_scheduler_file + str(i) + ".results" " Linux 0 2 6 9 >  " +
                  experiment_name + "/merged_results" + str(i))
        

slate = dict()
linux = dict()
for i in range(ITERATIONS):
    
    with open(experiment_name + "/merged_results" + str(i), 'r') as csvfile:
        reader = csv.reader(csvfile, delimiter='\t')
        firstRow = True
        for row in reader:
            if firstRow:
                firstRow = False
                continue
            id = row[0]
            if id not in slate:
                slate[id] = dict()
                slate[id]['runtime'] = 0.0
                slate[id]['IPC'] = 0.0
                slate[id]['LLC'] = 0.0

            if id not in linux:
                linux[id] = dict()
                linux[id]['runtime'] = 0.0
                linux[id]['IPC'] = 0.0
                linux[id]['LLC'] = 0.0

            print("===")
            print(row)
            slate[id]['runtime'] += float(row[1])
            linux[id]['runtime'] += float(row[2])
            slate[id]['IPC'] += float(row[3])
            slate[id]['LLC'] += float(row[4])
            linux[id]['IPC'] += float(row[5])
            linux[id]['LLC'] += float(row[6])

applications_run = 0
with open(experiment_name + "/merged_results" + str(0), 'r') as csvfile:
    reader = csv.reader(csvfile, delimiter='\t')
    firstRow = True
    for row in reader:
        if firstRow:
            firstRow = False
            continue
        id = row[0]
        applications_run += 1
        slate[id]['runtime'] = slate[id]['runtime'] / ITERATIONS
        slate[id]['IPC'] = slate[id]['IPC'] / ITERATIONS
        slate[id]['LLC'] = slate[id]['LLC'] / ITERATIONS
        linux[id]['runtime'] = linux[id]['runtime'] / ITERATIONS
        linux[id]['IPC'] = linux[id]['IPC'] / ITERATIONS
        linux[id]['LLC'] = linux[id]['LLC'] / ITERATIONS
            
meanslate = copy.deepcopy(slate)
meanlinux = copy.deepcopy(linux)
#print(meanslate)
#print(meanlinux)
#print("============================\n")

stdslate = dict()
stdlinux = dict()
for i in range(ITERATIONS):
    slate = dict()
    linux = dict()

    with open(experiment_name + "/merged_results" + str(i), 'r') as csvfile:
        reader = csv.reader(csvfile, delimiter='\t')
        firstRow = True
        for row in reader:
            if firstRow:
                firstRow = False
                continue
            id = row[0]
            if id not in slate:
                slate[id] = dict()
                slate[id]['runtime'] = float(row[1])
                slate[id]['IPC'] = float(row[3])
                slate[id]['LLC'] = float(row[4])

            if id not in linux:
                linux[id] = dict()
                linux[id]['runtime'] = float(row[2])
                linux[id]['IPC'] = float(row[5])
                linux[id]['LLC'] = float(row[6])

            slate[id]['runtime'] -= meanslate[id]['runtime']
            slate[id]['runtime'] = slate[id]['runtime'] ** 2
            linux[id]['runtime'] -= meanlinux[id]['runtime']
            linux[id]['runtime'] = linux[id]['runtime'] ** 2

            slate[id]['IPC'] -= meanslate[id]['IPC']
            slate[id]['IPC'] = slate[id]['IPC'] ** 2
            slate[id]['LLC'] -= meanslate[id]['LLC']
            slate[id]['LLC'] = slate[id]['LLC'] ** 2
            linux[id]['IPC'] -= meanlinux[id]['IPC']
            linux[id]['IPC'] = linux[id]['IPC'] ** 2
            linux[id]['LLC'] -= meanlinux[id]['LLC']
            linux[id]['LLC'] = linux[id]['LLC'] ** 2
            
            if id not in stdslate:
                stdslate[id] = dict()
                stdslate[id]['runtime'] = 0.0
                stdslate[id]['IPC'] = 0.0
                stdslate[id]['LLC'] = 0.0

            if id not in stdlinux:
                stdlinux[id] = dict()
                stdlinux[id]['runtime'] = 0.0
                stdlinux[id]['IPC'] = 0.0
                stdlinux[id]['LLC'] = 0.0

            stdslate[id]['runtime'] += slate[id]['runtime']
            stdslate[id]['IPC'] += slate[id]['IPC']
            stdslate[id]['LLC'] += slate[id]['LLC']

            stdlinux[id]['runtime'] += linux[id]['runtime']
            stdlinux[id]['IPC'] += linux[id]['IPC']
            stdlinux[id]['LLC'] += linux[id]['LLC']

with open(experiment_name + "/merged_results" + str(0), 'r') as csvfile:
    reader = csv.reader(csvfile, delimiter='\t')
    firstRow = True
    for row in reader:
        if firstRow:
            firstRow = False
            continue
        id = row[0]
        stdslate[id]['runtime'] = stdslate[id]['runtime'] / ITERATIONS
        stdslate[id]['runtime'] = math.sqrt(stdslate[id]['runtime'])
        stdslate[id]['IPC'] = stdslate[id]['IPC'] / ITERATIONS
        stdslate[id]['IPC'] = math.sqrt(stdslate[id]['IPC'])
        stdslate[id]['LLC'] = stdslate[id]['LLC'] / ITERATIONS
        stdslate[id]['LLC'] = math.sqrt(stdslate[id]['LLC'])
        
        stdlinux[id]['runtime'] = stdlinux[id]['runtime'] / ITERATIONS
        stdlinux[id]['runtime'] = math.sqrt(stdlinux[id]['runtime'])
        stdlinux[id]['IPC'] = stdlinux[id]['IPC'] / ITERATIONS
        stdlinux[id]['IPC'] = math.sqrt(stdlinux[id]['IPC'])
        stdlinux[id]['LLC'] = stdlinux[id]['LLC'] / ITERATIONS
        stdlinux[id]['LLC'] = math.sqrt(stdlinux[id]['LLC'])
        
results = open(experiment_name + "/merged_results", 'w')
results.write("id\tslate\tslate-IPC\tslate-LLC-hit-rate\tslate-std\tslate-IPC-std\tslate-LLC-hit-rate-std\tLinux\tLinux-IPC\tLinux-LLC-hit-rate\tLinux-std\tLinux-IPC-std\tLinux-LLC-hit-rate-std\n")

for i in range(0, applications_run):
    results.write(str(i) + "\t")
    for key in meanslate:
        if int(key) == i:
            results.write(str(meanslate[key]['runtime']) + "\t" + str(meanslate[key]['IPC']) + "\t" + str(meanslate[key]['LLC']) + "\t") 

    for key in stdslate:
        if int(key) == i:
            results.write(str(stdslate[key]['runtime']) + "\t" + str(stdslate[key]['IPC']) + "\t" + str(stdslate[key]['LLC']) + "\t") 
    for key in meanlinux:
        if int(key) == i:
            results.write(str(meanlinux[key]['runtime']) + "\t" + str(meanlinux[key]['IPC']) + "\t" + str(meanlinux[key]['LLC']) + "\t") 

    for key in stdlinux:
        if int(key) == i:
            results.write(str(stdlinux[key]['runtime']) + "\t" + str(stdlinux[key]['IPC']) + "\t" + str(stdlinux[key]['LLC']) + "\n") 


results.close()

#print(stdslate)
#print(stdlinux)
    
#plot the file and creat the png
os.system("gnuplot -e \"filename='" + experiment_name + "/merged_results" 
          + "'; yaxislabel='Time (s)'; result_file='" + experiment_name +"/" + plotted_figure_file + "'\" plot.plt")


os.system("gnuplot -e \"filename='" + experiment_name + "/merged_results" 
          + "'; yaxislabel=''; result_file='" + experiment_name +"/hw_" + plotted_figure_file + "'\" plot_hw.plt")


#os.system("gnuplot5-qt -e \"filename='" + experiment_name + "/merged_results" 
#          + "'; from='2'; to='3'; yaxislabel='Time (s)'; result_file='" + experiment_name +"/" + plotted_figure_file +# "'\" plot.plt")

#os.system("gnuplot5-qt -e \"filename='" + experiment_name + "/merged_results" 
#          + "'; from='4'; to='7'; yaxislabel=''; result_file='" + experiment_name +"/hw_" + plotted_figure_file + "'\"# plot.plt")


