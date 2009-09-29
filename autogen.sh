#!/bin/bash

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
topdir=`pwd`
cd $srcdir

LIBTOOLIZE=`which glibtoolize 2>/dev/null` 
[ $LIBTOOLIZE ] || LIBTOOLIZE=`which libtoolize`
$LIBTOOLIZE -c -f --automake
aclocal
#autoheader
automake --foreign -a -c -f
autoconf -f

cd $topdir
if test "x$NOCONFIGURE" = "x"; then
	$srcdir/configure $@
fi

