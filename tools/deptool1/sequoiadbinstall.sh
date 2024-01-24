#!/bin/bash
. ./sequoiadbconfig.sh
. ./sequoiadbfun1.sh
. ./sequoiadbfun2.sh

#分系统安装

if [ ${1} -eq 1 ]; then
   installsdb "${2}"
   if [ $? -ne 0 ]; then
      exit 1
   fi
elif [ ${1} -eq 2 ]; then
   SDBstart "${2}" "${3}" "${4}" "${5}"
   if [ $? -ne 0 ]; then
      exit 1
   fi
fi

exit 0
