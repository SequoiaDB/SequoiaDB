#!/bin/bash
.  ./common.sh
.  ./deploy_function.sh `pwd` 


for nowHost in $HOSTLIST1
do
   #ssh root@$nowHost rm -rf $SOFTWARE_FILE_DIR
   #ssh root@$nowHost mkdir -p $SOFTWARE_FILE_DIR
   #echo "scp ${INSTALL_DIR}/${INSTALL_SOFTWARE_FILE} to ${nowHost} "
   #cpToMachine $nowHost $INSTALL_DIR"/"$INSTALL_SOFTWARE_FILE
   
   echo "scp ${INSTALL_DIR}/common.sh to ${nowHost} "
   cpToMachine $nowHost $INSTALL_DIR"/common.sh"
   
   echo "scp ${INSTALL_DIR}/deploy_function.sh to ${nowHost} "
   cpToMachine $nowHost $INSTALL_DIR"/deploy_function.sh"
   
   echo "scp ${INSTALL_DIR}/remote_deploy.sh to ${nowHost} "
   cpToMachine $nowHost $INSTALL_DIR"/remote_deploy.sh"
   
   ssh root@$nowHost chmod a+x $SOFTWARE_FILE_DIR"/"$INSTALL_SOFTWARE_FILE
   ssh root@$nowHost chmod a+x $SOFTWARE_FILE_DIR"/common.sh"
   ssh root@$nowHost chmod a+x $SOFTWARE_FILE_DIR"/deploy_function.sh"
   ssh root@$nowHost chmod a+x $SOFTWARE_FILE_DIR"/remote_deploy.sh"
   
   #ssh root#$nowHost $SOFTWARE_FILE_DIR"/"remote_deploy.sh 

done

for nowHost in $HOSTLIST1
do
  echo -e "###########################################################\n"
  echo "work in "${nowHost}
  ssh root@${nowHost} $SOFTWARE_FILE_DIR"/"remote_deploy.sh "1" "${SOFTWARE_FILE_DIR}"
  echo -e "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
done
:<<BLOCK
for nowHost in $HOSTLIST2
do
  echo -e "###########################################################\n"
  echo "work in "${nowHost}
  ssh root@${nowHost} $SOFTWARE_FILE_DIR"/"remote_deploy.sh 2 ${SOFTWARE_FILE_DIR}
  echo -e "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
        
done
BLOCK

deploy_sequoiadb_env cata
deploy_sequoiadb_env data
