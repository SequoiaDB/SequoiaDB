#/bin/sh

if [ $# -lt 1 ] ; then
    echo "usage: $(basename $0) PROPS [OPT VAL [...]]" >&2
    exit 1
fi

PROPS="$1"
shift

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

BEFORE_LOAD="tableCreates"
if [ "$testType" = "fdw" ]; then
   BEFORE_LOAD="foreignTableCreates"
fi

AFTER_LOAD="indexCreates foreignKeys extraHistID buildFinish"
if [ "$testType" = "fdw" ]; then
   AFTER_LOAD="foreignExtraHistID buildFinish"
fi

for step in ${BEFORE_LOAD} ; do
    ./runSQL.sh "${PROPS}" $step
    ret=$?
    if [ ${ret} -ne 0 ];then
       echo "exec $step failed" 
       exit $ret
    fi
done

coordAddrs="$(grep '^sdburl=' $PROPS | sed -e 's/^sdburl=//')"
echo $coordAddrs
if [ "$testType" = "fdw" ];then
   ./collection.py "${coordAddrs}" 0  
   ret=$?
   if [ ${ret} -ne 0 ];then
      echo "create collection failed"
      exit $ret
   fi
fi

./runLoader.sh "${PROPS}" $*
ret=$?
if [ ${ret} -ne 0 ];then
   echo "load failed"
   exit $ret
fi

for step in ${AFTER_LOAD} ; do
    ./runSQL.sh "${PROPS}" $step
    ret=$?
    if [ ${ret} -ne 0 ];then
       exit $ret
    fi
done
