#!/bin/bash
. ${1}/common.sh
cleanENV()
{
   #
   # choose uninstall Sequoiadb software fail on error 
   # if $1 = 1 will be fail on error
   # if $1 = 0 will ignore fail 
   #
   # stop all the Sequoiadb process
   
         
   echo -e "###########################################################\n"
   echo "clear the environment"
   if test -f "${SDB_INSTALL_DIR}/bin/sdbstop"
   then
      echo "stop all the sdb process"
      $SDB_INSTALL_DIR"/bin/sdbstop"
   fi
   if test -f "${SDB_INSTALL_DIR}/bin/sdbcmtop"
   then
      echo "stop the sdbcm"
      $SDB_INSTALL_DIR"/bin/sdbcmtop"
   fi
   #uninstall Sequoiadb software
   if test -f "${SDB_INSTALL_DIR}/uninstall"
   then 
     ${SDB_INSTALL_DIR}/uninstall --mode unattended ;
     uninstallOnfail=$?;
     if [ 1 -eq $1 ] ; then
       if [ 0 -ne $uninstallOnfail ];then
         echo "uninstall Sequoiadb software fail on "`hostname`
         exit 0 ;
       fi
     fi     
   fi
   
   #delete the install directory
   echo "rm -rf ${SDB_INSTALL_DIR}"
   rm -rf $SDB_INSTALL_DIR ;
   
   echo -e "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
}

installSoftware()
{
   #
   # install the Sequoiadb software 
   # mkdir two directory
   #
   echo -e "###########################################################\n"
   echo "start to install the software"
   install_arg="--mode unattended --prefix $SDB_INSTALL_DIR --username sdbadmin --userpasswd sdbadmin"
   chmod a+x $SOFTWARE_FILE_DIR"/"$INSTALL_SOFTWARE_FILE ;
   $SOFTWARE_FILE_DIR"/"$INSTALL_SOFTWARE_FILE $install_arg 
   
   if [ 0 -ne $? ]; then 
     echo "install the Sequoiadb software fail on "`hostname` 
     exit 0 ;
   fi
   
   mkdir -p $SDB_INSTALL_DIR"/"database
   mkdir -p $SDB_INSTALL_DIR"/conf/local/50000"
   cp $SDB_INSTALL_DIR"/conf/samples/sdb.conf.coord" $SDB_INSTALL_DIR"/conf/local/50000/sdb.conf"
   
   #ignore this choose
   #sed -i "s:# logpath=:logpath=${SDB_INSTALL_DIR}:g" $SDB_INSTALL_DIR/conf/local/50000/sdb.conf
   echo -e "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
            
}

write_coord_conf()
{
   #
   # enter first value for HOSTLIST? name
   #
   cata_args=""
   i=0
   
   case $1 in 
   1 )
	   for cataHostName in $HOSTLIST1
	   do
       if [ $i -ne 2 ];then
       
         let i=i+1
         cata_args=$cata_args"${cataHostName}:30003,"
       else
         let i=i+1
         cata_args=$cata_args"${cataHostName}:30003"
       fi
	   done
   ;;
   2 )
	   for cataHostName in $HOSTLIST2
     do
       if [ $i -ne 2 ];then
       
         let i=i+1
         cata_args=$cata_args"${cataHostName}:30003,"
       else
         let i=i+1
         cata_args=$cata_args"${cataHostName}:30003"
       fi
	   done
   ;;
   
   * )
	   echo "enter error"
	 ;;
	 esac 
   sed -i "s/# catalogaddr=/catalogaddr=${cata_args}/g" $SDB_INSTALL_DIR/conf/local/50000/sdb.conf
   
   #transaction on or off
   #sed -i 's:transactionon=false:transactionon=true:g' $SDB_INSTALL_DIR/conf/local/50000/sdb.conf
}

start_deploy()
{
   echo "start the "`hostname`" coord process"
   su - sdbadmin -c "${SDB_INSTALL_DIR}/bin/sdbstart -c ${SDB_INSTALL_DIR}/conf/local/50000"
   #su - sdbadmin -c "${SDB_INSTALL_DIR}/bin/sdbcmart"
}

deploy_sequoiadb_env()
{  
   #
   # you should enter one value for this function to choose the deploy type
   #
   
   $SDB_CMD "db = new Sdb('${SDB_HOST_NAME}',50000)"
   case $1 in
     catalog | cata )
	   i=0;
	   # create catalog nodes
	   for nowHOST in $HOSTLIST1
	   do
	     if [ $i -eq 0 ];then
		     echo "create the primary catalog node "
		     $SDB_CMD "db.createCataRG('${nowHOST}',30000,'${SDB_INSTALL_DIR}/database/cata/30000')"
		     echo "wait 30 secends for vote primary  catalog node"
		     #create catalog and choose the catalog primary node
		     sleep 30
		     let "i=i+1"
		   else
		     echo ""
		     $SDB_CMD "var catarg = db.getRG(1)"
		     $SDB_CMD "var node = catarg.createNode('${nowHOST}',30000,'${SDB_INSTALL_DIR}/database/cata/30000')"
		     $SDB_CMD "node.start()"
		     sleep 10
		   fi
	   done
	   
	   if false ;then
	   i=0;
	   for nowHOST in $HOSTLIST2
	   do
	     if [ $i -eq 0 ];then
		     
		     $SDB_CMD "db.createCataRG('"$nowHOST"',30000,'"$SDB_INSTALL_DIR"/database/cata/30000')"
		     #create catalog and choose the catalog primary node
		     sleep 30
		     let "i=i+1"
		   else
		     $SDB_CMD "var catarg = db.getRG(1)"
		     $SDB_CMD "var node = catarg.createNode('"$nowHOST"',30000,'"$SDB_INSTALL_DIR"/database/cata/30000')"
		     
		     $SDB_CMD "node.start()"
		     sleep 10
		   fi
	   done
	   fi
	   
	   echo "create Catalog Group done"
	   
	   ;;
	   
	   data )
	     #create data nodes
	     
	     
	     dataPort=${dataNodeBastPort}
	     group_list=${GROUP_LIST}
	     for i in $group_list
	     do
		     #create data group?
		     $SDB_CMD "var rg = db.createRG('group"$i"')" 
		     for nowHOST in $HOSTLIST1
		     do
		       $SDB_CMD "rg.createNode('"$nowHOST"',"$dataPort",'"$SDB_INSTALL_DIR"/database/data/"$dataPort"')"
		       
		       sleep 7
		     done
		     echo "start the group${i}"
		     $SDB_CMD "rg.start()"
		     let "dataPort=dataPort+10"
		     sleep 30
	     done
	     
	     if false ; then
		   dataPort=51010
	     group_list=${GROUP_LIST}
	     for i in $group_list
	     do
		     #create data group?
		     $SDB_CMD "var rg = db.createRG('group"$i"')" 
		     for nowHOST in $HOSTLIST2
		     do
		       $SDB_CMD "rg.createNode('"$nowHOST"',"$dataPort",'"$SDB_INSTALL_DIR"/database/data/"$dataPort"')"
		       
		       sleep 7
		     done
		     echo "start the group${i}"
		     $SDB_CMD "rg.start()"
		     let "dataPort=dataPort+10"
		     sleep 30
	     done
	     fi
	     
	     echo "create data Groups done"
	   ;;
	   
	   * )
	     echo "enter error"
	   ;;
	   esac
	   $SDB_CMD "db.close()"

}

#scp local:${secend value} to root:${first value}:$SOFTWARE_FILE_DIR
cpToMachine()
{
  #
  # enter first value as the scp hostname
  # enter second value as the scp file's name
  #
  if [ -z $1 ]; then 
    echo "function cpToMachine should enter first value as the scp hostname"
    exit 0;
  fi
  
  if [ -z $2 ]; then 
    echo "function cpToMachine should enter second value as the scp file name"
    exit 0;
  fi
  
  echo "scp ${2} root@${1}:$SOFTWARE_FILE_DIR"
  
  scp ${2} root@${1}":"$SOFTWARE_FILE_DIR
  
  if [ $? -ne 0 ];then
    echo "scp ${2} to ${1} fail"
    exit 0;
  fi
  

}

mountDist()
{
   dataPort=51010
   for distName in $DISTLIST
   do
     mkdir -p $DATABASE_DIR"/"$dataPort
     mount $distName $DATABASE_DIR"/"$dataPort
     echo "${distName}   ${DATABASE}/${dataPort}  ext4  defaults  0  0" >> /etc/fstab
     let dataPort=dataPort+10
   done
  

}
formatDist()
{
   for nowDist in $DISTLIST
   do
     /sbin/mkfs -t ext4  $nowDist
   done
}


writeSYSFile()
{
  # enter first value as the machine memory
  memory=$1
  # write /etc/security/limits.conf
  echo "#<domain>      <type>    <item>     <value>"    >> /etc/security/limits.conf
  echo "*               soft      core       0"         >> /etc/security/limits.conf
  echo "*               soft      data       unlimited" >> /etc/security/limits.conf
  echo "*               soft      fsize      unlimited" >> /etc/security/limits.conf
  echo "*               soft      rss        unlimited" >> /etc/security/limits.conf
  echo "*               soft      as         unlimited" >> /etc/security/limits.conf
  
  #write /etc/sysctl.conf
  if [ $memory -ge 8 ];then
    echo "vm.swappiness = 0"                         >> /etc/sysctl.conf
  fi
  echo "vm.dirty_ratio = 100"                        >> /etc/sysctl.conf
  echo "vm.dirty_background_ratio = 10"              >> /etc/sysctl.conf
  echo "vm.dirty_expire_centisecs = 50000"           >> /etc/sysctl.conf
  echo "vm.vfs_cache_pressure = 200"                 >> /etc/sysctl.conf
  memory_min_free=$(echo "scale=2;${memory}*0.05"|bc)
  memory_min_free=$(echo "scale=2;${memory_min_free}*1024"|bc)
  echo "vm.min_free_kbytes = ${memory_min_free}"     >> /etc/sysctl.conf
  
  echo "vm.swappiness = "`cat /proc/sys/vm/swappiness`                           >> /etc/sysctl.conf.bak
  echo "vm.dirty_ratio = "`cat /proc/sys/vm/dirty_ratio`                         >> /etc/sysctl.conf.bak
  echo "vm.dirty_background_ratio = "`cat /proc/sys/vm/dirty_background_ratio`   >> /etc/sysctl.conf.bak
  echo "vm.dirty_expire_centisecs = "`cat /proc/sys/vm/dirty_expire_centisecs`   >> /etc/sysctl.conf.bak
  echo "vm.vfs_cache_pressure = "`cat /proc/sys/vm/vfs_cache_pressure`           >> /etc/sysctl.conf.bak
  echo "vm.min_free_kbytes = "`cat /proc/sys/vm/min_free_kbytes`                 >> /etc/sysctl.conf.bak
  
  /sbin/sysctl -p


}
