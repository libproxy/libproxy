#!/bin/bash

libtoolize -c -f --automake
aclocal
#autoheader
automake --foreign -a -c -f
autoconf -f
./configure $@
