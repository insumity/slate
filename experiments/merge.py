#!/usr/bin/env python3
import sys
import csv
import string

def get_file_data(file_name, column1, column2):
    result = []
    with open(file_name, 'r') as csvfile:
        reader = csv.reader(csvfile, delimiter='\t')
        for row in reader:
            result.append({'id':row[column1], 'time':row[column2]})
    return result

input_files = (len(sys.argv) - 1) / 4
if (len(sys.argv) - 1) % 4 != 0 or input_files == 0:
    print("Invalid number of merge arguments. Try again.")
    sys.exit(-1)

applications_run = 0 # how many applications did we run
final_results = {}
for i in range(0, int(input_files)):
    input_file_index = i * 4 + 1
    file_name = sys.argv[input_file_index]
    column1 = int(sys.argv[input_file_index + 2])
    column2 = int(sys.argv[input_file_index + 3])
    res = get_file_data(file_name, column1, column2)
    if (applications_run != 0 and len(res) != applications_run):
        print("Different scheduling options run different number of applications")
        sys.exit(-1)
    applications_run = len(res)
    final_results[file_name] = res


print("id\t", end="")
for i in range(0, int(input_files)):
    input_file_index = i * 4 + 1
    file_name = sys.argv[input_file_index + 1]
    print(file_name+"\t", end="")
print()

for j in range(0, applications_run):
    print(str(j) + "\t", end ="")
    for i in range(0, int(input_files)):
        input_file_index = i * 4 + 1
        file_name = sys.argv[input_file_index]
        for k in final_results[file_name]:
            if int(k['id']) == j:
                print(k['time'] + '\t', end="")
    print()
    

