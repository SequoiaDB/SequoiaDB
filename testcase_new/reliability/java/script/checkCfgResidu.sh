#!/bin/bash

#-----------------------------------------------------------------
# Filename: checkCfgResidu.sh
# Revision: 1.0
# Date:  2017/2/28
# Author: wenjingwang
# Description：用于检查部署目录下是否存在残留的配置文件
#              $?为0时无残留
#              $?为2时有残留，残留目录直接输出到屏幕
#-----------------------------------------------------------------

if [ $# -ne 2 ];then
   echo "$0 RsvBeginPort RsvEndPort"
   exit 1
fi

source /etc/default/sequoiadb
cfgPath="${INSTALL_DIR}/conf/local"
if [ ! -d ${cfgPath} ];then
   echo "${cfgPath} must be exist path"
   exit 1
fi

if [ $1 -gt $2 ];then
   beginPort=$2
   endPort=$1
else
   beginPort=$1
   endPort=$2
fi

exist=0
for dir in `ls ${cfgPath}`
do
   subDir=`basename ${dir}`
   if [[ $subDir -ge ${beginPort}  &&  $subDir -le ${endPort} ]];then
      exist=1
      paths=(${paths[*]} ${subDir})
   fi
done

if [ $exist -eq 0 ];then
   exit 0
else
   echo "(${paths[*]})"
   exit 2
fi




