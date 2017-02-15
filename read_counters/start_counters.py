#!/usr/bin/env python3

import sys
import os

from_core = int(sys.argv[1])
to_core = int(sys.argv[2])

for i in range(from_core, to_core + 1):
    os.system("./start_counter " + str(i) + "&")
