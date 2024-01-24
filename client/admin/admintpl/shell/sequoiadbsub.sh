#!/bin/bash
. /tmp/sequoiadbconfig.sh
. /tmp/sequoiadbfun1.sh
. /tmp/sequoiadbfun2.sh

#分系统

type="${1}"

if [ "${type}" = "1" ]; then
   array_ele="${2}"
   installFileSize="${3}"
   installSize="${4}"
   sdbVersion="${5}"
   localHostname=`hostname`
   #初始化删除列表
   DELETE_PATH_ARR=()
   #初始化删除文件
   echo "delete file list" > /tmp/sdbdel
   echo "revoke task" > /tmp/sdbrevtask
   #检查系统root权限 信任关系 hosts
   checkEnv "2" "${installFileSize}" "${installSize}"
   if [ $? -ne 0 ]; then
      echoDelList
      exit 1
   fi
   #检查sdbcm端口 用户组 用户 安装路径 权限 空间
   checkLocalEnv "${array_ele}" "${sdbVersion}" "${installSize}"
   if [ ${?} -ne 0 ]; then
      echoDelList
      exit 1
   fi

   echoDelList

   exit 0
elif [ "${type}" = "2" ]; then
   array_ele="${2}"
   #初始化删除列表
   DELETE_PATH_ARR=()
   #初始化撤销任务
   REVOKE_TASK_ARR=()

   installsdb "${array_ele}"
   if [ $? -ne 0 ]; then
      echoRevokeList
      echoDelList
      exit 1
   fi

   echoRevokeList
   echoDelList

   exit 0
elif [ "${type}" = "3" ]; then
   #参数2 是否第一次创建 0:是,1:否
   #参数3 coord的地址
   #参数4 coord的端口
   #参数5 节点列表的元素名
   isfirst="${2}"
   coordaddr="${3}"
   coordport="${4}"
   array_ele="${5}"
   #初始化删除列表
   DELETE_PATH_ARR=()
   #初始化撤销任务
   REVOKE_TASK_ARR=()

   SDBstart "${isfirst}" "${coordaddr}" "${coordport}" "${array_ele}"
   if [ $? -ne 0 ]; then
      echoRevokeList
      echoDelList
      exit 1
   fi

   echoDelList
   echoRevokeList

   exit 0
elif [ "${type}" = "4" ]; then
   #启动所有分区组
   #参数2 sdb的路径
   #参数3 coord的地址
   #参数4 coord的端口
   sdbpath="${2}"
   coordaddr="${3}"
   coordport="${4}"
   #初始化删除列表
   DELETE_PATH_ARR=()

   groupStart "${sdbpath}" "${coordaddr}" "${coordport}"
   if [ $? -ne 0 ]; then
      echoDelList
      exit 1
   fi

   echoDelList

   exit 0
elif [ "${type}" = "5" ]; then
   revokeListTask
   delListFile
   exit 0
elif [ "${type}" = "6" ]; then
   #启动指定分区组
   #参数2 sdb的路径
   #参数3 coord的地址
   #参数4 coord的端口
   #参数5 分区组名
   sdbpath="${2}"
   coordaddr="${3}"
   coordport="${4}"
   shardname="${5}"
   #初始化删除列表
   DELETE_PATH_ARR=()

   oneGroupStart "${sdbpath}" "${coordaddr}" "${coordport}" "${shardname}"
   if [ $? -ne 0 ]; then
      echoDelList
      exit 1
   fi

   echoDelList

   exit 0
fi