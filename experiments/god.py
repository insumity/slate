#!/usr/bin/python3.4
import sys
import string
import os

GLIBC_BUILD_DIRECTORY = "/localhome/kantonia/glibc-build2/"
NONE_SCHEDULER_FILE = "none_scheduler"
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
os.system("./experiment.py " + input_programs_file + " " + GLIBC_BUILD_DIRECTORY + " " 
          + none_scheduler_file + " " + slate_scheduler_file)

print("cd ..; ./schedule " + none_scheduler_file + " " + none_scheduler_file + ".results")
os.system("cd ..; ./schedule " + none_scheduler_file + " " + none_scheduler_file + ".results")
print("Going to the slate scheduler.") 
os.system("cd ..; schedule " + slate_scheduler_file + " " + slate_scheduler_file + ".results")

#run scheduler with results1
#run scheduler with results2

#merge results to a results_file
#plot the file and creat the png

