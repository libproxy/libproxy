#!/bin/bash
# $Id$

   #############################################################
  #                                                           ##
 #                   Written: Oct 23, 2008                   # #
#############################################################  #
#                                                           #  #
# testsuite.sh - Small test harness for libproxy regression #  #
#                testing. This should be self contained.    #  #
#                                                           # #
#############################################################

#  (c) 2008 Jeff Schroeder <jeffschroeder@computer.org>
#  License: LGPL 2.1+

detect_proxy_command() {
    proxy=$(which proxy 2>/dev/null)
    build=src/bin/proxy

    # Prefer the freshly built version over the system installed version
    if [ ! -x "$proxy" -a -x $build ]; then
        proxy=$build
        export PATH="$(dirname $build):${PATH}"
    fi  
}

output_result() {
    test=$1
    pass=$2
    shift; shift
    reason="$*"

    if [ "$pass" = "True" -o "$pass" = "true" ]; then
        printf '%s:\t\t\t %s\n' "$test" '[PASS]'
    else
        # oooooohhhh pretty colors
        printf '%s:\E[1;31m\t\t\t %s - %s\n' "$test" '[FAIL]' "${reason:-Unknown}" >&2
        tput sgr0
    fi
}

printf "\E[1;32m%s\t\t\t %s\n" "TEST NAME" STATUS
tput sgr0
for test in $(ls test.d/*.test 2>/dev/null); do
    name=$(basename $test | sed -e 's:libproxy_::' -e 's:\.test$::')
    unset reason pass
    if [ -x "$test" ]; then
        eval $($test)
        output_result "$name" "${pass:-False}" "${reason:-UNKNOWN}"
    fi
   
done
