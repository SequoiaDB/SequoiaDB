#!/bin/bash

#-----------------------------------------------------------------
# Filename: checkPortOccupied.sh
# Revision: 1.0
# Date:  2017/2/28
# Author: wenjingwang
# Description
#-----------------------------------------------------------------

if [ $# -ne 2 ];then
   echo "$0 RsrvportTbegin Rsrvportend"
   exit 1
fi

if  [ $1 -gt $2 ];then
   beginPort=$2
   endPort=$1
else
   beginPort=$1
   endPort=$2
fi

occupied=0
for ((port=${beginPort}; port < ${endPort}; ++port))
do
   netstat -an|grep ${port}|grep LISTEN 1>/dev/null
   if [ $? -eq 0 ];then
      occupied=1
      usedPort=(${usedPort[*]} ${port})
   fi
done

if [ $occupied -eq 0 ];then
  exit 0
else
  echo ${usedPort[*]}
  exit 2
fi