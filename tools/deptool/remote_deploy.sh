#!/bin/bash
. ${2}/common.sh
. ${2}/deploy_function.sh ${2}

cleanENV 0

installSoftware

cataHostName=`hostname`
if [ "$cataHostName" != "$HOST1" -a "$cataHostName" != "$HOST4" ];then
  write_coord_conf $1
fi
  
#writeSYSFile
   
#formatDist

#mountDist

chown sdbadmin:sdbadmin_group -R ${SDB_INSTALL_DIR}
start_deploy
