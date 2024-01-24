#!/bin/bash

INSTALL_MODE=""
CM_PORT=11790
SDB_VERSION=""

function getRootPath()
{
   confFile="/etc/default/sequoiadb"
   if [ -f $confFile ]; then
      source $confFile
   else
      echo "ERROR: File $confFile not exist!"
      exit 1
   fi
}

function getCMConf()
{
   confFile=$INSTALL_DIR/conf/sdbcm.conf
   if [ -f $confFile ]; then
      CM_PORT=`sed -n "/^defaultPort/"p $confFile| awk -F '=' '{print $2}'`
      has_om=`sed -n "/^IsGeneral/p" $confFile| awk -F '=' '{print $2}'`
   fi
   if [ "$has_om" == "" ] || [ "$has_om" == "FALSE" ] || [ "$has_om" == "false" ]; then
      INSTALL_MODE="normal"
   else
      INSTALL_MODE="upgrade"
   fi
}

function checkUser()
{
   user=`whoami`
   if [ $SDBADMIN_USER != $user ]; then
      echo "You should execute this script by user [$SDBADMIN_USER], but current user is [$user]!"
      exit 1
   fi
}

function getDBVersion()
{
   sequoiadb=$INSTALL_DIR/bin/sequoiadb
   SDB_VERSION=`$sequoiadb --version | grep -v "Git" | grep "version" | awk '{print $3}'`
}

function installOm()
{
   sdb=$INSTALL_DIR/bin/sdb
   omPort=0
   httpPort=0
   read -p "SequoiaDB om service port [11780]:" omPort
   omPort=${omPort:-11780}
   read -p "SequoiaDB SAC port [8000]:" httpPort
   httpPort=${httpPort:-8000}
   omPath=$INSTALL_DIR/database/sms/$omPort
   $sdb -s  " var omPort = '${omPort}';                                                 \
              var omPath = '${omPath}';                                                 \
              var httpPort = '${httpPort}';                                             \
              var cmPort = '${CM_PORT}';                                                \
              var arr = [];                                                             \
              var num = 0;                                                              \
              var canRemove = true;                                                     \
              arr = Sdbtool.listNodes( {type:'om', mode:'local', expand:true} );        \
              num = arr.size();                                                         \
              if ( num == 0 ) {                                                         \
                 arr = Sdbtool.listNodes( {type:'om', mode:'run', expand:true} );       \
                 num = arr.size();                                                      \
              }                                                                         \
              if ( num < 0 || num >= 2 ) {                                              \
                 println( 'Error: there are ' + num + ' sdbom exist in localhost' );    \
                 throw SDB_SYS;                                                         \
              }                                                                         \
              if ( num == 1 ) {                                                         \
                 var obj = eval( '(' + arr.pos() + ')' );                               \
                 omPort = obj['svcname'];                                               \
                 omPath = obj['dbpath'];                                                \
                 httpPort = obj['omname'];                                              \
                 canRemove = false;                                                     \
              }                                                                         \
              var oma = new Oma( '127.0.0.1', cmPort );                                 \
              try {                                                                     \
                 oma.createOM( omPort, omPath, {httpname: httpPort} );                  \
              } catch( e ) {                                                            \
                 if ( num == 1 && e == SDBCM_NODE_EXISTED ) {                           \
                    println( 'Warning: sdbom has existed in localhost' );               \
                 } else {                                                               \
                    println( 'Create om fial' );                                        \
                    throw e;                                                            \
                 }                                                                      \
              }                                                                         \
              try {                                                                     \
                  oma.startNode( omPort );                                              \
              } catch( e ) {                                                            \
                 if ( canRemove ) {                                                     \
                    println( 'Start om fail' );                                         \
                    oma.removeOM( omPort );                                             \
                 }                                                                      \
                 throw e;                                                               \
              } "
   isSucc=$?
   if [ $isSucc != 0 ]; then
      echo "Fail to create om"
      exit 1
   else
      echo "Succeed to create om"
   fi
   copyRunPackage
}

function upgradeOm()
{
   echo "Already install om, only copy run package into $ROOT_DIR/packet"
   rm -rf $INSTALL_DIR/packet/*
   copyRunPackage
}

function copyRunPackage()
{
   dbRunPath="/opt"
   mysqlRunPath="/opt"
   pgsqlRunPath="/opt"
   doCopy=""
   packet=$INSTALL_DIR/packet

   read -p "Whether copy run package for SAC use [y/N]:" doCopy
   if [ "$doCopy" == "Y" ] || [ "$doCopy" == "y" ]; then
      read -p "Please enter SequoiaDB run package directory for SAC used [$dbRunPath]:" dbRunPath
      test -z $dbRunPath && dbRunPath="/opt"
      cp $dbRunPath/sequoiadb-$SDB_VERSION*.run $packet > /dev/null 2>&1
      test $? -ne 0 && echo "Can not find SequoiaDB-$SDB_VERSION run package in directory $dbRunPath, please copy SequoiaDB-$SDB_VERSION run package manually to the directory $packet."

      read -p "Please enter Sequoiasql-mysql run package directory for SAC used [$mysqlRunPath]:" mysqlRunPath
      test -z $mysqlRunPath && mysqlRunPath="/opt"
      cp $mysqlRunPath/sequoiasql-mysql-$SDB_VERSION-*.run $packet > /dev/null 2>&1
      test $? -ne 0 && echo "Can not find Sequoiasql-mysql-$SDB_VERSION run package directory in $mysqlRunPath, please copy Sequoiasql-mysql-$SDB_VERSION run package manually to the directory $packet."

      read -p "Please enter Sequoiasql-postgresql run package directory for SAC used [$pgsqlRunPath]:" pgsqlRunPath
      test -z $pgsqlRunPath && pgsqlRunPath="/opt"
      cp $pgsqlRunPath/sequoiasql-postgresql-$SDB_VERSION-*.run $packet > /dev/null 2>&1
      test $? -ne 0 && echo "Can not find Sequoiasql-postgresql-$SDB_VERSION run package in directory $pgsqlRunPath, please copy Sequoiasql-postgresql-$SDB_VERSION run package manually to the directory $packet."
   else
      echo "please copy Sequoiasql run package manually to the directory $packet for SAC install Sequoiasql."
   fi
}

function main()
{
   getRootPath
   checkUser
   getCMConf
   getDBVersion
   if [ "$INSTALL_MODE" == "normal" ]; then
      installOm
   else
      upgradeOm
   fi
}

main
