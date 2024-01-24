#!/bin/bash
. ./sequoiadbconfig.sh
. ./sequoiadbfun1.sh
. ./sequoiadbfun2.sh

#分系统安装
#参数1 类型
if [ ${1} -eq 1 ]; then
   #启动所有分区组
   #参数2 sdb的路径
   #参数3 coord的地址
   #参数4 coord的端口
   groupStart "${2}" "${3}" "${4}"
   if [ $? -ne 0 ]; then
      exit 1
   fi
elif [ ${1} -eq 2 ]; then
   #启动指定分区组
   #参数2 sdb的路径
   #参数3 coord的地址
   #参数4 coord的端口
   #参数5 分区组名
   oneGroupStart "${2}" "${3}" "${4}" "${5}"
   if [ $? -ne 0 ]; then
      exit 1
   fi
fi

exit 0
