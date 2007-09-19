#!/bin/bash

libtoolize -c -f
autoconf -f
automake -a -c -f
./configure $@
