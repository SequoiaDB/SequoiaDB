#!/bin/bash

function build_help()
{
   echo ""
   echo "Usage:"
   echo "  --sdb               deploy SequoiaDB"
   echo "  --mysql             deploy SequoiaSQL-MySQL"
   echo "  --pg                deploy SequoiaSQL-PostgreSQL"
   echo "  --cm=Number         set cm port, default: 11790, eg: --cm=11790"
   echo "  --mysqlPath=String  mysql install path. Use it when you have multiple "
   echo "                      installations of SequoiaSQL-MySQL."
   echo "  --pgPath=String     pg install path. Use it when you have multiple "
   echo "                      installations of SequoiaSQL-PostgreSQL."
}

DEPLOY_SDB=0
DEPLOY_MYSQL=0
DEPLOY_PG=0
USER_SET_DEPLOY=0
CM_PORT=""
MySQL_INSTALL_PATH=""
PG_INSTALL_PATH=""

#Parse command line parameters
ARGS=`getopt -o h --long help,sdb,pg,mysql,cm:,mysqlPath:,pgPath: -n 'quickDeploy.sh' -- "$@"`
ret=$?
test $ret -ne 0 && exit $ret

eval set -- "${ARGS}"

while true
do
   case "$1" in
      --sdb )          DEPLOY_SDB=1
                       USER_SET_DEPLOY=1
                       shift
                       ;;
      --pg )           DEPLOY_PG=1
                       USER_SET_DEPLOY=1
                       shift
                       ;;
      --mysql )        DEPLOY_MYSQL=1
                       USER_SET_DEPLOY=1
                       shift
                       ;;
      --cm )           CM_PORT=$2
                       shift 2
                       ;;
      --mysqlPath )    MySQL_INSTALL_PATH=$2
                       shift 2
                       ;;
      --pgPath )       PG_INSTALL_PATH=$2
                       shift 2
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

if [ $USER_SET_DEPLOY -eq 0 ]; then
   DEPLOY_SDB=1
   DEPLOY_MYSQL=1
   DEPLOY_PG=1
fi

##################################
#    main entry
##################################

#get this script path
myPath=`dirname $0`

if [[ ${myPath:0:1} != "/" ]]; then
   myPath=$(pwd)/$myPath  #relative path
else
   myPath=$myPath         #absolute path
fi

#get sdb shell path
sdbShellPath=$myPath/../../bin/sdb

#get quickDeploy.js path
jsFile=$myPath/quickDeploy.js

#generate parameters

if [ $DEPLOY_SDB -eq 1 ]; then
   jsParameter=$jsParameter"var sdb=true;"
fi

if [ $DEPLOY_MYSQL -eq 1 ]; then
   jsParameter=$jsParameter"var mysql=true;"
fi

if [ $DEPLOY_PG -eq 1 ]; then
   jsParameter=$jsParameter"var pg=true;"
fi

if [ $USER_SET_DEPLOY -eq 0 ]; then
   jsParameter=""
fi

if [ "$CM_PORT" != "" ]; then
   jsParameter=$jsParameter"var cm="$CM_PORT";"
fi

if [ "$MySQL_INSTALL_PATH" != "" ]; then
   jsParameter=$jsParameter"var mysqlPath=\""$MySQL_INSTALL_PATH"\";"
fi

if [ "$PG_INSTALL_PATH" != "" ]; then
   jsParameter=$jsParameter"var pgPath=\""$PG_INSTALL_PATH"\";"
fi

jsParameter="'"$jsParameter"'"

#execute command
command=$sdbShellPath" -f "$jsFile" -e "$jsParameter
echo "Execute command: "$command
eval $command

