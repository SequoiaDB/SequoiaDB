#!/bin/bash

TYPE="install"
SDB=false
MYSQL=false
PG=false
SETUP_CONF="/etc/default/sequoiadb-setup.list"

function install()
{
   local name=$1
   echo "--------------------------begin to install $name-------------------------"
   for file_run in ./$name-*-installer.run;
   do
      echo "$file_run --mode text"
      chmod u+x $file_run
      $file_run --mode text
      test $? -ne 0 && { echo "ERROR: Fail to $file_run --mode text" >&2 && exit 1; }
      test -f $SETUP_CONF || touch $SETUP_CONF
      chmod 755 $SETUP_CONF
      case $name in
         "sequoiadb" )              . "/etc/default/sequoiadb"
                                    sed -i '/\/etc\/default\/sequoiadb,\s*/d' $SETUP_CONF
                                    echo '/etc/default/sequoiadb,'$MD5'' >> $SETUP_CONF
                                    shift
                                    ;;
         "sequoiasql-mysql" )       file=`ls -l -t /etc/default/sequoiasql-mysq* | awk 'NR<2{print $NF}' | awk -F '/' '{print $4}'`
                                    . "/etc/default/$file"
                                    sed -i '/\/etc\/default\/'$file',\s*/d' $SETUP_CONF
                                    echo '/etc/default/'$file','$MD5'' >> $SETUP_CONF
                                    shift
                                    ;;  
         "sequoiasql-postgresql" )  file=`ls -l -t /etc/default/sequoiasql-postgresql* | awk 'NR<2{print $NF}' | awk -F '/' '{print $4}'`
                                    . "/etc/default/$file"
                                    sed -i '/\/etc\/default\/'$file',\s*/d' $SETUP_CONF
                                    echo '/etc/default/'$file','$MD5'' >> $SETUP_CONF
                                    shift
                                    ;;  
      esac
   done
   echo "----------------------------end install $name----------------------------"
   echo 
}

function install_by_ask_user()
{
   read -p "Install sequoiadb Y/n: " choice
   [ -z $choice ] && choice="Y"
   [[ "$choice" == "Y" || "$choice" == "y" ]] && install "sequoiadb"
   
   local file_exist=false
   test -f "/etc/default/sequoiadb" && file_exist=true
   if [ $file_exist == true ]; then
      . "/etc/default/sequoiadb"
      for file_run in ./sequoiasql-*.run;
      do
         cp $file_run $INSTALL_DIR/packet
      done
      chmod u+x $INSTALL_DIR/packet/sequoiasql-*.run
   fi
   
   while :
   do
      read -p "Install 1:sequoiasql-mysql or 2:sequoiasql-postgresql, [1]: " select
      [ -z $select ] && select=1
      [[ "$select" == 1 || "$select" == 2 ]] && break
   done
   [[ "$select" == 1 ]] && install "sequoiasql-mysql" || install "sequoiasql-postgresql"
}

function clean_by_dbtype()
{
   local name=$1
   local installInfos=`cat /etc/default/sequoiadb-setup.list | grep "$name"`
   
   #installInfo record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "/etc/default/sequoiadb,md5"
   for installInfo in $installInfos
   do
      local file=`echo $installInfo |awk -F, '{print $1}'`
      local md5=`echo $installInfo |awk -F, '{print $2}'`
      [ -z $md5 ] && md5="xx"
      
      . $file
      if [ $MD5 == $md5 ]; 
      then
         case $name in
            "sequoiadb")
                          clean_sdb $installInfo
                          shift
                          ;;
            "sequoiasql-mysql" | "sequoiasql-postgresql")
                          clean_sql $installInfo
                          shift
                          ;;
            *)            echo "Internal error!"
                          exit 64
                          ;;
         esac
      fi
   done
}

function clean_sdb()
{
   local installInfo=$1
   local file=`echo $installInfo |awk -F, '{print $1}'`
   
   . $file
   #filter record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "sequoiadb,md5"
   local filter=`echo $installInfo |awk -F / '{print $4}'`
   
   read -p "clean $INSTALL_DIR $name Y/n: " choice
   [ -z $choice ] && choice="Y"
   if [[ "$choice" == "Y" || "$choice" == "y" ]];then
      
      local datadir_list=`$INSTALL_DIR/bin/sdblist -l | grep -v "Total"|awk 'NR>1{print $NF}'`
      echo "begin to uninstall $name"
      echo "$INSTALL_DIR/uninstall --mode unattended"
      $INSTALL_DIR/uninstall --mode unattended
      test $? -ne 0 && { echo "ERROR: Fail to $INSTALL_DIR/uninstall --mode unattended" >&2 && exit 1; }
      
      echo "ok"
      for datadir in $datadir_list
      do
         echo "rm -rf $datadir"
         rm -rf $datadir
      done
      echo "begin to clean install dir"
      echo "rm -rf $INSTALL_DIR"
      rm -rf $INSTALL_DIR
      
      `sed -i '/'$filter'/d' /etc/default/sequoiadb-setup.list`
      echo "ok"
   fi
   return 0
}

function clean_sql()
{
   local installInfo=$1
   local file=`echo $installInfo |awk -F, '{print $1}'`
   
   . $file
   #filter record the installed config file and md5 in the /etc/default/sequoiadb-setup.list, eg: "sequoiasql-mysql,md5"
   local filter=`echo $installInfo |awk -F / '{print $4}'`
   
   read -p "clean $INSTALL_DIR $name Y/n: " choice
   [ -z $choice ] && choice="Y"
   if [[ "$choice" == "Y" || "$choice" == "y" ]];then

      local datadir_list=`$INSTALL_DIR/bin/sdb_sql_ctl listinst | grep -v "Total"|awk 'NR>1{print $2 " " $3}'`
      echo "begin to uninstall $name"
      echo "$INSTALL_DIR/uninstall --mode unattended"
      $INSTALL_DIR/uninstall --mode unattended
      test $? -ne 0 && { echo "ERROR: Fail to $INSTALL_DIR/uninstall --mode unattended" >&2 && exit 1; }
      
      echo "ok"
      for datadir in $datadir_list
      do
         echo "rm -rf $datadir"
         rm -rf $datadir
      done
      echo "begin to clean install dir"
      echo "rm -rf $INSTALL_DIR"
      if [ $INSTALL_DIR != '/' ];then
         rm -rf $INSTALL_DIR
      fi
      
      
      `sed -i '/'$filter'/d' /etc/default/sequoiadb-setup.list`
      echo "ok"
   fi
      
   return 0
}

function build_help()
{
   echo ""
   echo "Usage:"
   echo "  --sdb        install or clean sequoiadb"
   echo "  --pg         install or clean sequoiasql-postgresql"
   echo "  --mysql      install or clean sequoiasql-mysql"
   echo "  --clean      clean installation"
}

#Parse command line parameters
#test $# -eq 0 && { build_help && exit 64; }

ARGS=`getopt -o h --long help,sdb,pg,mysql,clean -n 'test' -- "$@"`
ret=$?
test $ret -ne 0 && exit $ret

eval set -- "${ARGS}"

while true
do
   case "$1" in
      --sdb )          SDB=true
                       shift
                       ;;
      --pg )           PG=true
                       shift
                       ;;
      --mysql )        MYSQL=true
                       shift
                       ;;
      --clean )        TYPE="clean"
                       shift
                       ;;
      -h | --help )    build_help
                       exit 0
                       ;;
      --)              shift
                       break
                       ;;
      *)               echo "Internal error!"
                       exit 64
                       ;;
   esac
done

if [ $SDB == false ] && [ $PG == false ] && [ $MYSQL == false ];then
   if [ $TYPE == "install" ];then
      install_by_ask_user
   elif [ $TYPE == "clean" ];then
      SDB=true
      MYSQL=true
      PG=true
   fi
fi

if [ $TYPE == "install" ];then
   if [ $SDB == true ];then
      install "sequoiadb"
   fi
   if [ $MYSQL == true ];then
      install "sequoiasql-mysql"
   fi
   if [ $PG == true ];then
      install "sequoiasql-postgresql"
   fi
elif [ $TYPE == "clean" ];then
   if [ $SDB == true ];then
      clean_by_dbtype "sequoiadb"
   fi
   if [ $MYSQL == true ];then
      clean_by_dbtype "sequoiasql-mysql"
   fi
   if [ $PG == true ];then
      clean_by_dbtype "sequoiasql-postgresql"
   fi
fi
