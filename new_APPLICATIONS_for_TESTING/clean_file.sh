#!/bin/bash

args=("$@")
file=${args[0]}

cp $file /tmp/tmp$file
egrep -v "(o|^$)" /tmp/tmp$file > $file
