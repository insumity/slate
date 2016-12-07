#!/usr/bin/python3.4
import sys
import string

input_programs_file = sys.argv[1]
glibc_build_directory = sys.argv[2]

none_scheduler_file = sys.argv[3]
slate_scheduler_file = sys.argv[4]
linux_scheduler_file = sys.argv[5]
LINK_SCRIPT_FILE = "link.sh"

def to_slate_scheduler(policy, program):
    return policy + " " + LINK_SCRIPT_FILE + " " + glibc_build_directory + " " + str.join(' ', program)

def to_none_scheduler(policy, program):
    return "MCTOP_ALLOC_NONE" + " " + LINK_SCRIPT_FILE + " " + glibc_build_directory + " " + str.join(' ', program)

def to_linux_scheduler(policy, program):
    return "LINUX_SCHEDULER" + " " + str.join(' ', program)

def create_scheduler_file(to_scheduler_function, file_name):
    f = open(file_name, 'w')
    ex = 0
    with open(input_programs_file) as opf:
        for line in opf:
            line = line.strip('\n')
            if line[0] == '#':
                    continue
            words = line.split(' ')
            policy = words[0]
            program = words[1:]
            str_ex = str(ex) + " "
            f.write(str_ex + to_scheduler_function(policy, program))
            ex = ex + 1
            f.write('\n')
    f.close()

create_scheduler_file(to_none_scheduler, none_scheduler_file)
create_scheduler_file(to_slate_scheduler, slate_scheduler_file)
create_scheduler_file(to_linux_scheduler, linux_scheduler_file)
