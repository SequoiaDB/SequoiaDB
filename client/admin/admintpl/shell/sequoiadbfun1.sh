#!/bin/bash

#环境校验的函数

#输出调试信息
#参数1 等级     例如 "Error" "Warning" "Event"
#参数2 函数          $FUNCNAME
#参数3 行数          $LINENO
#参数4 输出信息 例如 "hello world"
function echo_r()
{
   if [ ${IS_PRINGT_DEBUG} -eq 1 ]; then
      printf "\nTime: %s\tLevel: $1\nFunction: $2\tLine: %d\nFile: $0\n%s\n" $(date +%Y-%m-%d-%H.%M.%S) $3 "$4"
   elif [ ${IS_PRINGT_DEBUG} -eq 2 ]; then
      echo "$4"
   fi
}

#检查是否root用户运行脚本
function checkRoot()
{
   if [ `id -u` -ne 0 ]; then
      return 1
   else
      return 0
   fi
}

#检查是否能ping通
#参数1 地址 例如 "192.168.1.1" "ubuntu-test-01"
function checkPing()
{
   local filestat=$(ping -c 1 $1|grep "transmitted"|awk '{print $4}')
   if [[ ${filestat} =~ ^[0-9]+$ ]]; then
      if [ ${filestat} = 1 ]; then
         return 0
      else
         return 1
      fi
   else
      return 1
   fi
}

#检查能否ping通,并返回响应时间
#参数1 地址 例如 "192.168.1.1"
#返回 响应时间 -1 连接失败 非负数则是响应时间
function checkPingAndReturn()
{
   local num=$(ping -c 1 $1|grep 'time='|awk '{print $7}'|cut -d "=" -f 2)
   if [ -z "${num}" ]; then
      return 1
   else
      echo ${num}
      return 0
   fi
}

#检查信任关系
#参数1 地址 例如 "192.168.1.1"
function checkTrust()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ];then
      return 0
   fi
   temp=`ssh -o StrictHostKeyChecking=no -o PasswordAuthentication=no root@${1} echo`
   if [ $? -ne 0 ]; then
      return 1
   else
      return 0
   fi
}
#检查本地hostname与实际机器的hostname是否一致
#参数1 hostname 例如 "ubuntu-test-01"
function checkHostname2()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ];then
      return 0
   fi

   local thisHost=`ssh root@$1 hostname`
   if [ "${thisHost}" = "${1}" ];then
      return 0
   else
      return 1
   fi
}

#获取目标的hostname
#参数1 地址 例如 "192.168.1.1"
function getSSHHostname()
{
   local thisHost=`ssh root@$1 hostname`
   echo "${thisHost}"
}

#获取目标的ip
#参数1 地址 例如 "192.168.1.1"
function getSSHIP()
{
   local thisIP=`ssh root@${1} ifconfig|grep "inet addr"|awk '{print $2}'|cut -d ":" -f 2|grep -v "127.0.0.1"`
   for subIP in ${thisIP}
   do
      echo "${subIP}|"
   done
}

#判断指定的远程目录可用空间是否达到指定大小(MB)
#参数1 地址 例如 "192.168.1.1"
#参数2 路径 例如 "/opt/sequoiadb/"
#参数3 大小 例如 256
function sshCheckAvailable()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      local available=`df -m ${2} | tail -n1|awk '{print $4}'`
      if [ ${available} -lt ${3} ]; then
         return 1
      else
         return 0
      fi
   else
      local available=`ssh root@${1} df -m ${2} | tail -n1|awk '{print $4}'`
      if [ ${available} -lt ${3} ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行远程系统的环境检查
#参数1 地址 例如 "192.168.1.1"
#参数2 主机列表的元素名
#参数3 安装包大小
#参数4 安装后大小需求(MB)
#参数5 数据库版本
function sshCheckEnv()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      /tmp/sequoiadbsub.sh "1" "${2}" "${3}" "${4}" "${5}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      ssh root@$1 "/tmp/sequoiadbsub.sh 1 ${2} ${3} ${4} ${5}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行节点启动或创建
#参数1 地址 例如 "192.168.1.1"
#参数2 是否第一次创建 0:是,1:否
#参数3 coord的地址
#参数4 coord的端口
#参数5 节点列表的元素名
function sshNodeStart()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      /tmp/sequoiadbsub.sh "3" "${2}" "${3}" "${4}" "${5}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      ssh root@$1 "/tmp/sequoiadbsub.sh 3 ${2} ${3} ${4} ${5}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行安装包安装
#参数1 地址
#参数2 主机列表的元素名
function sshInstallon()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      /tmp/sequoiadbsub.sh "2" "${2}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      ssh root@${1} "/tmp/sequoiadbsub.sh 2 ${2}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行回滚操作
#参数1 地址
function sshRevokeTask()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      fileexit=`if [ -f "/tmp/sequoiadbsub.sh" ]; then echo '1'; else echo '0'; fi`
      if [ $? -ne 1 ]; then
         #不存在
         return 0
      fi
      /tmp/sequoiadbsub.sh "5"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      fileexit=`ssh root@${1} "if [ -f "/tmp/sequoiadbsub.sh" ]; then echo '1'; else echo '0'; fi"`
      if [ $? -ne 1 ]; then
         #不存在
         return 0
      fi
      ssh root@${1} "/tmp/sequoiadbsub.sh 5"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行coord启动所有分区组
#参数1 路径
#参数2 coord的地址
#参数3 coord的端口
function sshGroupStart()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${2}" ]; then
      /tmp/sequoiadbsub.sh "4" "${1}" "${2}" "${3}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      ssh root@${2} "/tmp/sequoiadbsub.sh 4 ${1} ${2} ${3}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#执行coord启动指定分区组
#参数1 路径
#参数2 coord的地址
#参数3 coord的端口
#参数4 分区组名
function sshOneGroupStart()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${2}" ]; then
      /tmp/sequoiadbsub.sh "6" "${1}" "${2}" "${3}" "${4}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      ssh root@${2} "/tmp/sequoiadbsub.sh 6 ${1} ${2} ${3} ${4}"
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#提升远程文件的权限
#参数1 地址
#参数2 文件名
function changeFilePower()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${1}" ]; then
      chmod u+x ${2}
   else
      ssh root@${1} "chmod u+x ${2}"
   fi
}

#复制文件到其他主机上的/tmp目录
#参数1 文件名 例如 "/opt/sequoiadb/sequoiadb.run"
#参数2 地址   例如 "ubuntu-test-01"
function copyFile2OtherHost()
{
   local localHostname=`hostname`
   if [ "${localHostname}" = "${2}" ]; then
      temp=`cp ${1} /tmp`
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   else
      temp=`scp $1 root@$2:/tmp`
      if [ $? -ne 0 ]; then
         return 1
      else
         return 0
      fi
   fi
}

#步骤1，当检查完所有的系统环境之后运行
#拷贝安装包到机器上
function distribution()
{
   local target=""
   for array_name in ${LIST_HOST[@]}
   do
      eval "host_array=(\"\${${array_name}[@]}\")"
      local target="${host_array[0]}"
      #复制安装文件
      copyFile2OtherHost "${INSTALL_PATH}/${INSTALL_NAME}" "${target}"
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "Copy ${INSTALL_NAME} file to ${target}:/tmp failed"
         return 1
      fi
      changeFilePower "${target}" "/tmp/${INSTALL_NAME}"
      #echo_r "Event" $FUNCNAME $LINENO "Copy ${INSTALL_NAME} file to ${target}:/tmp successfully"
   done
   return 0
}

#获取本机的数据库版本
function getLocalDBVersion()
{
   local INSTALL_DIR=""
   if [ -f /etc/default/sequoiadb ] ; then
      source /etc/default/sequoiadb
   else
      INSTALL_DIR=/opt/sequoiadb
   fi
   SDB_BIN="${INSTALL_DIR}/bin/sequoiadb"
   local version=""
   if [ -f "${SDB_BIN}" ]; then 
      version=`${SDB_BIN} "--version"`
   fi
   echo ${version} | cut -d " " -f 5
}

#检查主机系统环境，指定机器
#参数1 hostname
#参数2 主机列表元素名
#参数3 类型 如果是主系统 1  如果是分系统 2
#参数4 安装文件包的大小
#参数5 安装后大小
function checkEnvOneHost()
{
   local target="${1}"
   local array_ele="${2}"
   local type="${3}"
   local installFileSize="${4}"
   local installSize="${5}"
   local localHostname=`hostname`

   #检查网络连接是否正常
   checkPing ${target}
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "The ${target} is not ping"
      return 1
   fi

   #检查信任关系
   checkTrust ${target}
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "The ${localHostname} and ${target} is not a trust relationship"
      return 1
   fi

   #检查hosts与目标hostname是否匹配
   checkHostname2 ${target}
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "The target ${target} host and local hosts is not match"
      return 1
   fi

   #这是主系统执行的
   if [ "${type}" = "1" ]; then
      #远程检查/tmp路径可用空间
      sshCheckAvailable ${target} "/tmp" ${installFileSize}
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "The ${target} /tmp directory available space is less than ${installFileSize}MB"
         return 1
      fi

      #复制子程序到目标机器
      copyFile2OtherHost "${PWD}/sequoiadbconfig.sh" ${target}
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "Copy the sequoiadbconfig.sh file to the ${target} failure"
         return 1
      fi
      changeFilePower "${target}" "/tmp/sequoiadbconfig.sh"

      copyFile2OtherHost "${PWD}/sequoiadbfun1.sh" ${target}
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "Copy the sequoiadbfun1.sh file to the ${target} failure"
         return 1
      fi
      changeFilePower "${target}" "/tmp/sequoiadbfun1.sh"

      copyFile2OtherHost "${PWD}/sequoiadbfun2.sh" ${target}
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "Copy the sequoiadbfun2.sh file to the ${target} failure"
         return 1
      fi
      changeFilePower "${target}" "/tmp/sequoiadbfun2.sh"

      copyFile2OtherHost "${PWD}/sequoiadbsub.sh" ${target}
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "Copy the sequoiadbsub.sh file to the ${target} failure"
         return 1
      fi
      changeFilePower "${target}" "/tmp/sequoiadbsub.sh"

      #获取本机的sequoiadb版本
      local sdbVersion=`getLocalDBVersion`

      #远程控制其他主机进行环境检查
      sshCheckEnv "${target}" "${array_ele}" "${installFileSize}" "${installSize}" "${sdbVersion}"
      if [ ${?} -ne 0 ]; then
         #echo_r "Error" $FUNCNAME $LINENO "The ${target} Environmental inspection error"
         return 1
      fi
   fi
   return 0
}

#主系统检查系统环境
#参数1 类型 如果是主系统 1  如果是分系统 2
#参数2 安装文件包的大小
#参数3 安装后大小
function checkEnv()
{
   local type="${1}"
   local installFileSize="${2}"
   local installSize="${3}"
   local localHostname=`hostname`

   #检查root权限启动
   checkRoot
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "Please use the root user"
      return 1
   fi
   local target=""
   #遍历主机列表
   for array_name in ${LIST_HOST[@]}
   do
      eval "child=(\"\${${array_name}[@]}\")"
      target="${child[0]}"
      checkEnvOneHost "${target}" "${array_name}" "${type}" "${installFileSize}" "${installSize}"
      if [ $? -ne 0 ]; then
         return 1
      fi
   done
   return 0
}

#把所有分区组启动
#参数1 sdb路径
#参数2 coord的地址
#参数3 coord的端口
function groupStart()
{
   local rc=0

   ${1} "var db = new Sdb('${2}','${3}')"
   rc=$?
   if [ ${rc} -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "Failed to connect data, rc=${rc}"
      return 1
   fi
   for group_name in ${LIST_GROUP[@]}
   do
      ${1} "db.startRG('${group_name}')"
      echo_r "Event" $FUNCNAME $LINENO "group ${group_name} is start"
   done
   return 0
}

#把所有分区组启动
#参数1 sdb路径
#参数2 coord的地址
#参数3 coord的端口
#参数4 分区组名
function oneGroupStart()
{
   rc=0
   ${1} "var db = new Sdb('${2}','${3}')"
   rc=$?
   if [ ${rc} -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "Failed to connect coord ${2}:${3}, rc=${rc}"
      return 1
   fi
   ${1} "db.startRG('${4}')"
   rc=$?
   if [ ${rc} -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "Failed to activation ${4}, rc=${rc}"
      return 1
   fi
   echo_r "Event" $FUNCNAME $LINENO "group ${4} is start"
   return 0
}

#执行远程卸载脚本
function sshUninstallSDB()
{
   target=""
   if [ "${IS_ROLLBACK}" = "0" ]; then
      return 0
   fi
   for array_name in ${LIST_HOST[@]}
   do
      eval "host_array=(\"\${${array_name}[@]}\")"
      
      target="${host_array[0]}"
      sshRevokeTask "${target}"
   done
   #echo_r "Event" $FUNCNAME $LINENO "All uninstall"
   return 0
}

#执行远程安装脚本
function sshInstallSDB()
{
   local target=""
   local coordf=0
   local cataf=0
   local dataf=0
   local coordsdb=""
   local coordaddr=""
   local coordport=""

   #先执行安装包安装
   for array_name in ${LIST_HOST[@]}
   do
      eval "host_array=(\"\${${array_name}[@]}\")"
      target="${host_array[0]}"
      sshInstallon "${target}" "${array_name}"
      if [ $? -ne 0 ]; then
         return 1
      fi
   done

   #开始启动节点
   for array_name in ${LIST_NODE[@]}
   do
      eval "node_array=(\"\${${array_name}[@]}\")"
      eval "host_array=(\"\${${node_array[2]}[@]}\")"
      eval "node_conf=(\"\${${node_array[3]}[@]}\")"

      target="${host_array[0]}"

      if [ ${node_array[0]} = "coord" ]; then
         if [ ${coordf} -eq 0 ]; then
            coordaddr="${host_array[0]}"
            coordsdb="${host_array[1]}/bin/sdb"
            num=`get_SDBCONF_num "svcname"`
            coordport="${node_conf[${num}]}"
            if [ -z "${coordport}" ]; then
               coordport="11810"
            fi
         fi
         sshNodeStart "${target}" "${coordf}" "${coordaddr}" "${coordport}" "${array_name}"
         if [ $? -ne 0 ]; then
            return 1
         fi
         if [ ${coordf} -eq 0 ]; then
            coordf=1
         fi
      elif [ ${node_array[0]} = "cata" ]; then
         sshNodeStart "${target}" "${cataf}" "${coordaddr}" "${coordport}" "${array_name}"
         if [ $? -ne 0 ]; then
            return 1
         fi
         if [ ${cataf} -eq 0 ]; then
            cataf=1
         fi
      else
         sshNodeStart "${target}" "${dataf}" "${coordaddr}" "${coordport}" "${array_name}"
         if [ $? -ne 0 ]; then
            return 1
         fi
         if [ ${dataf} -eq 0 ]; then
            dataf=1
         fi
      fi
   done
   sshGroupStart ${coordsdb} ${coordaddr} ${coordport}
   return 0
}

#检验配置文件端口和路径
function check_conf_advanced()
{
   local cursor_node_name=""
   local cursor_host_name=""

   #端口检查
   #SDBCM_PORT
   local svcname_c=""
   local replname_c=""
   local shardname_c=""
   local catalogname_c=""
   local httpname_c=""

   #路径检查
   local installpath=""
   local installlen=0
   local logpath_c=""
   local diagpath_c=""
   local dbpath_c=""
   local indexpath_c=""
   local bkuppath_c=""

   #获取可配参数表的数组长度
   local conf_len=${#SDB_CONFIG[@]}

   for array_name in ${LIST_NODE[@]}
   do
      eval "node_array=(\"\${${array_name}[@]}\")"
      eval "host_array=(\"\${${node_array[2]}[@]}\")"
      eval "node_conf=(\"\${${node_array[3]}[@]}\")"

      #获取可配参数的数组长度
      local node_conf_len=${#node_conf[@]}
      if [ ${conf_len} -ne ${node_conf_len} ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} parameter length is ${node_conf_len}, parameter table length is ${conf_len}"
         return 1
      fi

      installpath="${host_array[1]}"
      installlen=${#installpath}
      cursor_node_name="${array_name}"
      cursor_host_name="${node_array[2]}"

      #取得端口
      num=`get_SDBCONF_num "svcname"`
      svcname_c="${node_conf[${num}]}"
      if [ -z "${svcname_c}" ]; then
         svcname_c="11810"
      fi

      num=`get_SDBCONF_num "replname"`
      replname_c="${node_conf[${num}]}"
      if [ -z "${replname_c}" ]; then
         let "replname_c=svcname_c+1"
      fi

      num=`get_SDBCONF_num "shardname"`
      shardname_c="${node_conf[${num}]}"
      if [ -z "${shardname_c}" ]; then
         let "shardname_c=svcname_c+2"
      fi

      num=`get_SDBCONF_num "catalogname"`
      catalogname_c="${node_conf[${num}]}"
      if [ -z "${catalogname_c}" ]; then
         let "catalogname_c=svcname_c+3"
      fi

      num=`get_SDBCONF_num "httpname"`
      httpname_c="${node_conf[${num}]}"
      if [ -z "${httpname_c}" ]; then
         let "httpname_c=svcname_c+4"
      fi

      #检查端口是否冲突
      if [ "${svcname_c}" = "${replname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of replname"
         return 1
      fi
      if [ "${svcname_c}" = "${shardname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of shardname"
         return 1
      fi
      if [ "${svcname_c}" = "${catalogname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of catalogname"
         return 1
      fi
      if [ "${svcname_c}" = "${httpname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of httpname"
         return 1
      fi
      if [ "${svcname_c}" = "${SDBCM_PORT}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of SDBCM_PORT"
         return 1
      fi

      if [ "${replname_c}" = "${shardname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} replname port is the same of shardname"
         return 1
      fi
      if [ "${replname_c}" = "${catalogname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} replname port is the same of catalogname"
         return 1
      fi
      if [ "${replname_c}" = "${httpname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} replname port is the same of httpname"
         return 1
      fi
      if [ "${replname_c}" = "${SDBCM_PORT}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} replname port is the same of SDBCM_PORT"
         return 1
      fi

      if [ "${shardname_c}" = "${catalogname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} shardname port is the same of catalogname"
         return 1
      fi
      if [ "${shardname_c}" = "${httpname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} shardname port is the same of httpname"
         return 1
      fi
      if [ "${shardname_c}" = "${SDBCM_PORT}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} shardname port is the same of SDBCM_PORT"
         return 1
      fi

      if [ "${catalogname_c}" = "${httpname_c}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} catalogname port is the same of httpname"
         return 1
      fi
      if [ "${catalogname_c}" = "${SDBCM_PORT}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} catalogname port is the same of SDBCM_PORT"
         return 1
      fi

      if [ "${httpname_c}" = "${SDBCM_PORT}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} httpname port is the same of SDBCM_PORT"
         return 1
      fi

      local templen=0
      #取得路径
      num=`get_SDBCONF_num "dbpath"`
      dbpath_c=${node_conf[${num}]}
      if [ -z "${dbpath_c}" ]; then
         dbpath_c="${host_array[1]}/bin"
      fi
      templen=${#dbpath_c}
      if [ ${templen} -le ${installlen} ]; then
         if [ "${installpath:0:${templen}}" = "${dbpath_c}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} ${dbpath_c} path can not be install path ${installpath} sub path "
            return 1
         fi
      fi

      num=`get_SDBCONF_num "indexpath"`
      indexpath_c=${node_conf[${num}]}
      if [ -z "${indexpath_c}" ]; then
         indexpath_c="${dbpath_c}"
      fi
      templen=${#indexpath_c}
      if [ ${templen} -le ${installlen} ]; then
         if [ "${installpath:0:${templen}}" = "${indexpath_c}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} ${indexpath_c} path can not be install path ${installpath} sub path "
            return 1
         fi
      fi

      num=`get_SDBCONF_num "logpath"`
      logpath_c=${node_conf[${num}]}
      if [ -z "${logpath_c}" ]; then
         logpath_c="${dbpath_c}/replicalog"
      fi
      templen=${#logpath_c}
      if [ ${templen} -le ${installlen} ]; then
         if [ "${installpath:0:${templen}}" = "${logpath_c}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} ${logpath_c} path can not be install path ${installpath} sub path "
            return 1
         fi
      fi

      num=`get_SDBCONF_num "diagpath"`
      diagpath_c=${node_conf[${num}]}
      if [ -z "${diagpath_c}" ]; then
         diagpath_c="${dbpath_c}/diaglog"
      fi
      templen=${#diagpath_c}
      if [ ${templen} -le ${installlen} ]; then
         if [ "${installpath:0:${templen}}" = "${diagpath_c}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} ${diagpath_c} path can not be install path ${installpath} sub path "
            return 1
         fi
      fi

      num=`get_SDBCONF_num "bkuppath"`
      bkuppath_c=${node_conf[${num}]}
      if [ -z "${bkuppath_c}" ]; then
         bkuppath_c="${dbpath_c}/bakfile"
      fi
      templen=${#bkuppath_c}
      if [ ${templen} -le ${installlen} ]; then
         if [ "${installpath:0:${templen}}" = "${bkuppath_c}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} ${bkuppath_c} path can not be install path ${installpath} sub path "
            return 1
         fi
      fi

      for array_name_2 in ${LIST_NODE[@]}
      do
         eval "node_array_2=(\"\${${array_name_2}[@]}\")"
         eval "host_array_2=(\"\${${node_array_2[2]}[@]}\")"
         eval "node_conf_2=(\"\${${node_array_2[3]}[@]}\")"

         #如果是同一节点，就跳过不检查
         if [ "${cursor_node_name}" = "${array_name_2}" ]; then
            continue
         fi
         #如果不是同一主机，就跳过不检查
         if [ "${cursor_host_name}" != "${node_array_2[2]}" ]; then
            continue
         fi

         #取得端口
         num=`get_SDBCONF_num "svcname"`
         svcname_t="${node_conf_2[${num}]}"
         if [ -z "${svcname_t}" ]; then
            svcname_t="11810"
         fi

         num=`get_SDBCONF_num "replname"`
         replname_t="${node_conf_2[${num}]}"
         if [ -z "${replname_t}" ]; then
            let "replname_t=svcname_t+1"
         fi

         num=`get_SDBCONF_num "shardname"`
         shardname_t="${node_conf_2[${num}]}"
         if [ -z "${shardname_t}" ]; then
            let "shardname_t=svcname_t+2"
         fi

         num=`get_SDBCONF_num "catalogname"`
         catalogname_t="${node_conf_2[${num}]}"
         if [ -z "${catalogname_t}" ]; then
            let "catalogname_t=svcname_t+3"
         fi

         num=`get_SDBCONF_num "httpname"`
         httpname_t="${node_conf_2[${num}]}"
         if [ -z "${httpname_t}" ]; then
            let "httpname_t=svcname_t+4"
         fi

         #取得路径
         num=`get_SDBCONF_num "dbpath"`
         dbpath_t=${node_conf_2[${num}]}
         if [ -z "${dbpath_t}" ]; then
            dbpath_t="${host_array_2[1]}/bin"
         fi

         num=`get_SDBCONF_num "indexpath"`
         indexpath_t=${node_conf_2[${num}]}
         if [ -z "${indexpath_t}" ]; then
            indexpath_t="${dbpath_t}"
         fi

         num=`get_SDBCONF_num "logpath"`
         logpath_t=${node_conf_2[${num}]}
         if [ -z "${logpath_t}" ]; then
            logpath_t="${dbpath_t}/replicalog"
         fi

         num=`get_SDBCONF_num "diagpath"`
         diagpath_t=${node_conf_2[${num}]}
         if [ -z "${diagpath_t}" ]; then
            diagpath_t="${dbpath_t}/diaglog"
         fi

         num=`get_SDBCONF_num "bkuppath"`
         bkuppath_t=${node_conf_2[${num}]}
         if [ -z "${bkuppath_t}" ]; then
            bkuppath_t="${dbpath_t}/bakfile"
         fi

         #检查端口冲突
         if [ "${svcname_c}" = "${svcname_t}" ] || [ "${svcname_c}" = "${replname_t}" ] || [ "${svcname_c}" = "${shardname_t}" ] || [ "${svcname_c}" = "${catalogname_t}" ] || [ "${svcname_c}" = "${httpname_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} svcname port is the same of ${array_name_2}"
            return 1
         fi
         if [ "${replname_c}" = "${svcname_t}" ] || [ "${replname_c}" = "${replname_t}" ] || [ "${replname_c}" = "${shardname_t}" ] || [ "${replname_c}" = "${catalogname_t}" ] || [ "${replname_c}" = "${httpname_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} replname port is the same of ${array_name_2}"
            return 1
         fi
         if [ "${shardname_c}" = "${svcname_t}" ] || [ "${shardname_c}" = "${replname_t}" ] || [ "${shardname_c}" = "${shardname_t}" ] || [ "${shardname_c}" = "${catalogname_t}" ] || [ "${shardname_c}" = "${httpname_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} shardname port is the same of ${array_name_2}"
            return 1
         fi
         if [ "${catalogname_c}" = "${svcname_t}" ] || [ "${catalogname_c}" = "${replname_t}" ] || [ "${catalogname_c}" = "${shardname_t}" ] || [ "${catalogname_c}" = "${catalogname_t}" ] || [ "${catalogname_c}" = "${httpname_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} catalogname port is the same of ${array_name_2}"
            return 1
         fi
         if [ "${httpname_c}" = "${svcname_t}" ] || [ "${httpname_c}" = "${replname_t}" ] || [ "${httpname_c}" = "${shardname_t}" ] || [ "${httpname_c}" = "${catalogname_t}" ] || [ "${httpname_c}" = "${httpname_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} httpname port is the same of ${array_name_2}"
            return 1
         fi

         #检查路径冲突
         if [ "${logpath_c}" = "${logpath_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} logpath ${logpath_c} is the same of ${array_name_2} ${logpath_t}"
            return 1
         fi
         if [ "${diagpath_c}" = "${diagpath_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} diagpath ${diagpath_c} is the same of ${array_name_2} ${diagpath_t}"
            return 1
         fi
         if [ "${dbpath_c}" = "${dbpath_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} dbpath {dbpath_c} is the same of ${array_name_2} ${dbpath_t}"
            return 1
         fi
         if [ "${indexpath_c}" = "${indexpath_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} indexpath ${indexpath_c} is the same of ${array_name_2} ${indexpath_t}"
            return 1
         fi
         if [ "${bkuppath_c}" = "${bkuppath_t}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${cursor_node_name} bkuppath ${bkuppath_c} is the same of ${array_name_2} ${bkuppath_t}"
            return 1
         fi
      done
   done
}

#校验配置文件基础
function check_conf_base()
{
   #检查调试信息
   if [ -z "${IS_PRINGT_DEBUG}" ]; then
      echo_r "Error" $FUNCNAME $LINENO "IS_PRINGT_DEBUG can not null"
      return 1
   fi

   #检查安装包路径
   if [ -z "${INSTALL_PATH}" ]; then
      echo_r "Error" $FUNCNAME $LINENO "INSTALL_PATH can not null"
      return 1
   fi

   #检查安装包的文件名
   if [ -z "${INSTALL_NAME}" ]; then
      echo_r "Error" $FUNCNAME $LINENO "INSTALL_NAME can not null"
      return 1
   fi

   #检查sdbcm端口
   if [ -z "${SDBCM_PORT}" ]; then
      echo_r "Error" $FUNCNAME $LINENO "SDBCM_PORT can not null"
      return 1
   fi

   #检查主机列表
   for array_name in ${LIST_HOST[@]}
   do
      eval "child=(\"\${${array_name}[@]}\")"

      #检查主机配置的数组长度
      num=${#child[@]}
      if [ ${num} -ne 5 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} array length is not 5"
         return 1
      fi

      #检查变量
      if [ -z "${child}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} can not null"
         return 1
      fi

      #检查hostname
      if [ -z "${child[0]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} hostname can not null"
         return 1
      fi

      #检查安装路径
      if [ -z "${child[1]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} install path can not null"
         return 1
      fi

      #检查用户组
      if [ -z "${child[2]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} system group can not null"
         return 1
      fi

      #检查用户名
      if [ -z "${child[3]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} system user can not null"
         return 1
      fi

      #检查密码
      if [ -z "${child[4]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} system user's password can not null"
         return 1
      fi
   done

   #检查节点列表
   for array_name in ${LIST_NODE[@]}
   do
      eval "child=(\"\${${array_name}[@]}\")"

      #检查节点配置的数组长度
      num=${#child[@]}
      if [ ${num} -ne 4 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} array length is not 4"
         return 1
      fi

      #检查变量
      if [ -z "${child}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} can not null"
         return 1
      fi

      #检查角色
      if [ -z "${child[0]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} role can not null"
         return 1
      fi

      #检查角色是否填错
      if [ "${child[0]}" != "coord" ] && [ "${child[0]}" != "cata" ] && [ "${child[0]}" != "data" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} role must be coord,cata,data, can not ${child[0]}"
         return 1
      fi

      #检查数据节点的分区组
      if [ "${child[0]}" = "data" ]; then
         if [ -z "${child[1]}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${array_name} group can not null"
            return 1
         fi
         #检查分区组是否存在列表中
         check_group_is_exist "${child[1]}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${array_name} group can not null"
         fi
      fi

      #检查主机关联
      if [ -z "${child[2]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} host_link can not null"
         return 1
      fi

      #检查配置信息关联
      if [ -z "${child[3]}" ]; then
         echo_r "Error" $FUNCNAME $LINENO "${array_name} conf_link can not null"
         return 1
      fi

   done
}

#判断分区组是否存在列表中
#参数1 分区组名
function check_group_is_exist()
{
   for group_name in ${LIST_GROUP[@]}
   do
      if [ "${1}" = "${group_name}" ] && [ -n "${group_name}" ]; then
         return 0
      fi
   done
   return 1
}
