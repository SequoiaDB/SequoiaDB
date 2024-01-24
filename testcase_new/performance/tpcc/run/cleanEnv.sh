#!/bin/bash

if [ $# -lt 2 ];then
   echo "$0 hostnumber host1 host2 ..."
   exit 0
fi

hostNum=$1
shift

if [ "${hostNum}" -gt 0 ] 2>/dev/null ;then
  :
else
  echo "first parameter must be number"
  exit 1
fi

for((i=0; i<${hostNum}; ++i))
do
   ping -c 5 $1 1>>/dev/null
   if [ $? -ne 0 ];then
      echo "host $1 Unreachable"
      exit 1
   fi
   ssh root@$1 "sync && echo 3 >/proc/sys/vm/drop_caches"
   shift
done
