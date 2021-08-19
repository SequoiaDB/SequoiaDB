#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)
SYS_CONF_FILE=/etc/default/sequoiadb
USER=sdbadmin
cur_user=`whoami`
confrootpath="$BashPath/../conf/local"
logrootpath="$BashPath/../log"
  
function check_user()
{
  if [ -f "$SYS_CONF_FILE" ]; then
    . $SYS_CONF_FILE
  else
    echo "ERROR: $SYS_CONF_FILE does not exist"
    exit 1
  fi

  if [ -n "$SDBADMIN_USER" ];then
    USER=$SDBADMIN_USER
    if [ "$cur_user" != "$SDBADMIN_USER" -a "$cur_user" != "root" ]; then
      echo "ERROR: fsstart requires USER [$USER] permission"
      exit 126
    fi
  else
    echo "ERROR: SDBADMIN_USER is null"
    exit 125
  fi
}


check_user

startfs="$BashPath/start_i.sh"

if [ "$cur_user" == "root" ]; then
  su - $USER -c "$startfs $*"
else
  $startfs $*
fi  
