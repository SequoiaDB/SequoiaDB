#!/bin/bash

processes=`ps -ef | grep '\<sdbcm\>'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

processes=`ps -ef | grep '\<sdbstart\>'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

processes=`ps -ef | grep '\<sdbstop\>'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

processes=`ps -ef | grep '\<sdb\>'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

processes=`ps -ef | grep '\<sdbbp\>'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

processes=`ps -ef | grep '\<sequoiadb\>(\w\{1,\})'|grep -v '\<grep\>'|awk '{print $2}'`
for process in ${processes}
do
   kill -9 ${process}
done

if [ -n "${1}" ]; then
   if [ -d "${1}" ]; then
      echo "delete `hostname` ${1}"
      rm -r ${1}
   fi
fi

if [ -n "${2}" ]; then
   if [ -d "${2}" ]; then
      echo "delete `hostname` ${2}"
      rm -r ${2}
   fi
fi

if [ -n "${3}" ]; then
   if [ -d "${3}" ]; then
      echo "delete `hostname` ${3}"
      rm -r ${3}
   fi
fi

if [ -n "${4}" ]; then
   if [ -d "${4}" ]; then
      echo "delete `hostname` ${4}"
      rm -r ${4}
   fi
fi

if [ -n "${5}" ]; then
   if [ -d "${5}" ]; then
      echo "delete `hostname` ${5}"
      rm -r ${5}
   fi
fi

if [ -n "${6}" ]; then
   if [ -d "${6}" ]; then
      echo "delete `hostname` ${6}"
      rm -r ${6}
   fi
fi

rc=$?
exit 0
