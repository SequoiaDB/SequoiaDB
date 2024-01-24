#!/bin/bash
. ./sequoiadbconfig.sh
. ./sequoiadbfun1.sh
. ./sequoiadbfun2.sh

#主系统检查

#安装文件的大小(MB)
INSTALL_FILE_SIZE=`ls -ld "${INSTALL_PATH}/${INSTALL_NAME}" | awk '{print int($5/1048576+10)}'`

#安装后的总大小(MB)
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

echo_r "Event" "" "0" "Check the configuration file successfully"

checkEnv "1" "${INSTALL_FILE_SIZE}" "${INSTALL_SDB_SIZE}"
if [ $? -ne 0 ]; then
   sshUninstallSDB
   exit 1
fi

echo_r "Event" "" "0" "Check the system environment successfully"

distribution
if [ $? -ne 0 ]; then
   sshUninstallSDB
   exit 1
fi

echo_r "Event" "" "0" "The end of the installation package file distribution"

sshInstallSDB
if [ $? -ne 0 ]; then
   sshUninstallSDB
   exit 1
fi

echo_r "Event" "" "0" "Cluster installation is complete"

exit 0
