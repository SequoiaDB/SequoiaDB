#!/bin/bash
. ./sequoiadbconfig.sh
. ./sequoiadbfun1.sh
. ./sequoiadbfun2.sh

#参数1 类型 数值整形

if [ "${1}" = "1" ];then
   #判断文件是否存在
   #参数2 文件路径
   checkFileisExist "${2}"
   if [ $? -eq 0 ]; then
      echo_r "Error" "" 0 "The ${2} install file does not exist"
      exit 1
   fi
elif [ "${1}" = "2" ];then
   #判断ssh连接,获取响应时间
   #参数2 地址
   #参数3 端口(暂时没用)
   pingtime=`checkPingAndReturn "${2}"`
   if [ $? -eq 1 ]; then
      echo_r "Error" "" 0 "The ${2} connection failed"
      exit 1
   fi
   checkTrust "${2}"
   if [ $? -eq 1 ]; then
      echo_r "Error" "" "0" "The ${2} port ${3} ssh connection failed"
      exit 1
   fi
   echo "${pingtime}"
elif [ "${1}" = "3" ];then
   #获取hostname
   #参数2 IP
   host_name=`getSSHHostname "$2"`
   echo ${host_name}
elif [ "${1}" = "4" ];then
   #获取ip
   #参数2 hostname
   ips=`getSSHIP "$2"`
   echo ${ips}
elif [ "${1}" = "5" ];then
   #本机连接目标机器的环境检查
   #参数2 地址

   #安装文件的大小(MB)
   INSTALL_FILE_SIZE=`ls -ld "${INSTALL_PATH}/${INSTALL_NAME}" | awk '{print int($5/1048576+10)}'`
   INSTALL_SDB_SIZE=$[${INSTALL_FILE_SIZE}*3]


   #配置文件检查
   check_conf_base
   if [ $? -ne 0 ]; then
      exit 1
   fi

   #配置文件检查2
   check_conf_advanced
   if [ $? -ne 0 ]; then
      exit 1
   fi

   list_host_name=""
   for array_name in ${LIST_HOST[@]}
   do
      eval "child=(\"\${${array_name}[@]}\")"
      if [ "${2}" = "${child[0]}" ]; then
         list_host_name="${array_name}"
         break
      fi
   done

   #checkEnv "2" "${INSTALL_FILE_SIZE}" "${INSTALL_SDB_SIZE}"

   checkEnvOneHost "${2}" "${list_host_name}" "1" "${INSTALL_FILE_SIZE}" "${INSTALL_SDB_SIZE}"
   if [ $? -ne 0 ]; then
      exit 1
   fi
   
   echo_r "Event" "" "0" "Check the system environment successfully"
   
elif [ "${1}" = "6" ];then
   #分派安装文件
   #参数2 地址

   target=${2}
   copyFile2OtherHost "${INSTALL_PATH}/${INSTALL_NAME}" ${target}
   if [ $? -ne 0 ]; then
      echo_r "Error" "" "0" "Copy ${INSTALL_NAME} file to ${target}:/tmp failed"
      exit 1
   fi
   echo_r "Event" "" "0" "Copy ${INSTALL_NAME} file to ${target}:/tmp success"
elif [ "${1}" = "7" ];then
   #执行安装包安装
   #参数2 地址

   list_host_name=""
   for array_name in ${LIST_HOST[@]}
   do
      eval "child=(\"\${${array_name}[@]}\")"
      if [ "${2}" = "${child[0]}" ]; then
         list_host_name="${array_name}"
         break
      fi
   done

   target="${2}"

   sshInstallon "${target}" "${list_host_name}"
   if [ $? -ne 0 ]; then
      echo_r "Error" "" "0" "${target} /tmp/${INSTALL_NAME} installation failed"
      exit 1
   fi
   echo_r "Event" "" "0" "${target} /tmp/${INSTALL_NAME} installation is complete"
elif [ "${1}" = "8" ];then
   #启动或创建节点
   #参数2 地址
   #参数3 是否首次创建 0：是， 1：否
   #参数4 coord地址
   #参数5 coord端口
   #参数6 运行的节点端口

   list_node_name=""
   for array_name in ${LIST_NODE[@]}
   do
      eval "node_array=(\"\${${array_name}[@]}\")"
      eval "host_array=(\"\${${node_array[2]}[@]}\")"
      eval "node_conf=(\"\${${node_array[3]}[@]}\")"
      num=`get_SDBCONF_num "svcname"`
      svcport="${node_conf[${num}]}"
      if [ -z "${svcport}" ]; then
         svcport="11850"
      fi
      if [ "${2}" = "${host_array[0]}" ] && [ "${6}" = "${svcport}" ]; then
         list_node_name="${array_name}"
         break
      fi
   done

   echo "address ${2}   isfirst ${3}   coordadd ${4}   coordport ${5}   nodeport ${list_node_name}"
   sshNodeStart "${2}" "${3}" "${4}" "${5}" "${list_node_name}"
   if [ $? -ne 0 ]; then
      echo_r "Error" "" "0" "The node ${2} port ${6} create failed"
      exit 1
   fi
   echo_r "Event" "" "0" "The node ${2} port ${6} create complete"
elif [ "${1}" = "9" ];then
   #激活分区组
   #参数2 路径
   #参数3 coord地址
   #参数4 coord端口
   #参数5 分区组名
   echo "path ${2}  coordadd ${3}  coordport ${4}  groupname ${5}"
   sshOneGroupStart "${2}" "${3}" "${4}" "${5}"
   if [ $? -ne 0 ]; then
      echo_r "Error" "" "0" "The group ${5} active failed"
      exit 1
   fi
   echo_r "Event" "" "0" "The group ${5} active complete"
else
   echo "not to do anything"
fi

exit 0

