#!/usr/bin/env python3

# Replace calls to 'cp' with Python

import shutil
import sys

if len(sys.argv) < 3:
    raise ValueError('Usage: %s <srcfiles> <destfile>' % sys.argv[0])

last_arg_index = len(sys.argv) - 1
destdir = sys.argv[last_arg_index]

for x in range(1, last_arg_index):
    shutil.copy(sys.argv[x], destdir)
