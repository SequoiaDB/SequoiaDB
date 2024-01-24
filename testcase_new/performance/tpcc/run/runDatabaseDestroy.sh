#!/bin/sh

if [ $# -ne 1 ] ; then
    echo "usage: $(basename $0)" >&2
    exit 1
fi

PROPS="$1"
if [ ! -f "${PROPS}" ] ; then
    echo "${PROPS}: no such file or directory" >&2
    exit 1
fi

testType="$(grep '^testType=' $PROPS | sed -e 's/^testType=//')"
if [ "$testType" != "fdw" -a "$testType" != "original" ];then
   echo "testType is $testType,but must fdw or original"
   exit 1
fi
DB="$(grep '^db=' $PROPS | sed -e 's/^db=//')"

coordAddrs="$(grep '^sdburl=' $PROPS | sed -e 's/^sdburl=//')"
STEPS="tableDrops"
if [ "$testType" = "fdw" ];then
   ./collection.py ${coordAddrs} 1 
   STEPS="foreignTableDrops"
fi

for step in ${STEPS} ; do
    ./runSQL.sh "${PROPS}" $step
done
