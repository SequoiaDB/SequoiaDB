#!/bin/bash
####################################################################
# Decription: 
#    Start or stop service in install or upgrade Sequoiadb
####################################################################

EXEC_PATH=""
CM_PORT=0
DO_FORCE_STOP_SERVICE=0
DO_START_SERVICE=0
DO_STOP_SERVICE=0
DO_CREATE_DIR=0
USER_NAME=""
GROUP_NAME=""
AUTO_START=""

function stopService()
{
   # InstallBuild will copy sdbcmtop/sdbstop into /tmp directory,
   # make sure it have the right permission
   if [ "$EXEC_PATH" == "" ]; then
      exit 1
   fi
   chmod 0755 $EXEC_PATH/sdbcmtop
   chmod 0755 $EXEC_PATH/sdbstop
   $EXEC_PATH/sdbcmtop > /dev/null 2>&1
   test $? -ne 0 && exit $?
   $EXEC_PATH/sdbstop -t all > /dev/null 2>&1
   test $? -ne 0 && exit $?
}

function forceStopService()
{
   if [ "$EXEC_PATH" == "" ]; then
      exit 1
   fi
   service sdbcm stop
   $EXEC_PATH/sdblist -t all | grep -v "Total" | awk '{print $2}' | awk -F '(' '{print $2}' | awk -F ')' '{print "kill -9 " $1}' | bash
   nodeNum=`$EXEC_PATH/sdblist -t all | grep -v "Total" | wc -l`
   test $nodeNum -ne 0 && exit 1 
}

function startService()
{
   if [ "$EXEC_PATH" == "" ]; then
      exit 1
   fi
   service sdbcm start
   # return 127 when didnot have node, so, donot check return code
   $EXEC_PATH/sdbstart -t all > /dev/null 2>&1
}

function createDir()
{
   # check userName and groupName is empty
   if [ "$USER_NAME" == "" ] || [ "$GROUP_NAME" == "" ]; then
      exit 1
   fi

   # if /var/sequoiadb not exist, create it
   if [ ! -d /var/sequoiadb ]; then
      mkdir -p /var/sequoiadb
   fi

   chown $USER_NAME:$GROUP_NAME /var/sequoiadb
   test $? -ne 0 && exit $?
   chmod 777 /var/sequoiadb
   test $? -ne 0 && exit $?
}

main()
{
   ARGS=`getopt -o h --long path:,cmPort:,startService:,stopService:,forceStopService:,createDir,userName:,groupName: -n 'service_control.sh' -- "$@"`
   ret=$?
   test $ret -ne 0 && return $ret
   eval set -- "${ARGS}"

   while true
   do
      case "$1" in
         --path )             EXEC_PATH=$2
                              shift 2
                              ;;
         --startService )     DO_START_SERVICE=$2
                              shift 2
                              ;;
         --stopService )      DO_STOP_SERVICE=$2
                              shift 2
                              ;;
         --forceStopService ) DO_FORCE_STOP_SERVICE=$2
                              shift 2
                              ;;
         --createDir )        DO_CREATE_DIR=1
                              shift 1
                              ;;
         --userName )         USER_NAME=$2
                              shift 2
                              ;;
         --groupName )        GROUP_NAME=$2
                              shift 2
                              ;;
         -- )                 shift
                              break
                              ;;
         * )                  echo "Invalid parameters: $1"
                              return 1
                              ;;
      esac
   done


   if [ "$DO_START_SERVICE" == "true" ]; then
      startService
      exit 0
   fi
   
   if [ "$DO_FORCE_STOP_SERVICE" == "1" ]; then
      forceStopService
      exit 0
   fi
   
   if [ "$DO_STOP_SERVICE" == "true" ]; then
      stopService
      exit 0
   fi

   if [ "$DO_CREATE_DIR" == "1" ]; then
      createDir
      exit 0
   fi
}

main "$@"
