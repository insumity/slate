#!/usr/bin/env python3

from subprocess import call
call(["/home/kantonia/kyotocabinet-1.2.76/bin/kccachetest_MUTEX", "wicked", "-th", "6", "10000000"])




