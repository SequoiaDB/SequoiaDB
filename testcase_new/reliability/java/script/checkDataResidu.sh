#-----------------------------------------------------------------
# Filename: checkPortOccupied.sh
# Revision: 1.0
# Date:  2017/2/28
# Author: wenjingwang
# Description：用于检查数据目录下是否存在残留的配置文件
#              $?为0时无残留
#              $?为2时有残留，残留目录直接输出到屏幕
#-----------------------------------------------------------------
#!/bin/bash

if [ $# -ne 1 ];then
   echo "$0 datapath"
   exit 1
fi

files=`ls $1`
if [ -z "$files" ];then
   exit 0
else
   echo ${files}
   exit 2
fi
