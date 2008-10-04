#!/bin/bash

LIBTOOLIZE=`which glibtoolize 2>/dev/null` 
[ $LIBTOOLIZE ] || LIBTOOLIZE=`which libtoolize`
$LIBTOOLIZE -c -f --automake
aclocal
#autoheader
automake --foreign -a -c -f
autoconf -f
