#!/usr/bin/python3.5
import sys
import string
import os
import time
import tempfile

run_programs_concurrently = False
GLIBC_BUILD_DIRECTORY = "/localhome/kantonia/glibc-build2/"
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

def execute_one_by_one(scheduler_file):
    lines = file_len(scheduler_file)
    for i in range(0, lines):
        f = tempfile.NamedTemporaryFile(mode='w', delete=False)
        print("Line " + str(i) + " .. of file: " + get_line(scheduler_file, i))
        line = get_line(scheduler_file, i)
        f.write(line)
        f.flush()
        print("The file that was used is: " + f.name);
        f.close()
        os.system("../schedule " + f.name + " " + f.name + ".results")
        os.system("cat " + f.name + ".results >> " + scheduler_file + ".results")

def execute_linux_one_by_one(scheduler_file):
    lines = file_len(scheduler_file)
    for i in range(0, lines):
        f = tempfile.NamedTemporaryFile(mode='w', delete=False)
        print("Line " + str(i) + " .. of file: " + get_line(scheduler_file, i))
        line = get_line(scheduler_file, i)
        f.write(line)
        f.flush()
        print("The file that was used is: " + f.name);
        f.close()
        os.system("./use_linux_scheduler " + f.name + " " + f.name + ".results")
        os.system("cat " + f.name + ".results >> " + scheduler_file + ".results")

            
if run_programs_concurrently:
    print("Going to the slate scheduler.") 
    os.system("../schedule " + slate_scheduler_file + " " + slate_scheduler_file + ".results")
    print("Going to the none-slate scheduler.")
    os.system("../schedule " + none_scheduler_file + " " + none_scheduler_file + ".results")
    print("Going to the Linux scheduler.") 
    os.system("./use_linux_scheduler " + linux_scheduler_file + " " + linux_scheduler_file + ".results")
else:
    print("Going to the slate scheduler.") 
    execute_one_by_one(slate_scheduler_file)
    print("Going to the none-slate scheduler.")
    execute_one_by_one(none_scheduler_file)
    print("Going to the Linux scheduler.")
    execute_linux_one_by_one(linux_scheduler_file)
    
#merge results to a results_file
os.system("./merge.py " + none_scheduler_file + ".results" + " none 0 2 "
          + slate_scheduler_file + ".results" " slate 0 2 "
          + linux_scheduler_file + ".results" " Linux 0 2 >  " + experiment_name + "/merged_results")

#plot the file and creat the png
os.system("gnuplot -e \"filename='" + experiment_name + "/merged_results" 
          + "'; from='2'; to='4'; result_file='" + experiment_name +"/" + plotted_figure_file + "'\" plot.plt")


