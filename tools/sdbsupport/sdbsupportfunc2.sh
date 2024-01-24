#!/bin/bash

# check over username and password
function connectDB()
{
   HOST=$1
   PORT=$2
   INSTALL_DIR=$3

   SDB=$INSTALL_DIR/bin/sdb
   connDB=`$SDB "try{var db=new Sdb( '$HOST', $PORT, '$SDB_USER', '$SDB_PASSWD' )}catch(e){if(-179==e){println('AuthorityForbidden');}else{println('OtherError:'+e);}}"`
   while [ "AuthorityForbidden"X == "$connDB"X ]
   do
      echo "The port:$PORT in host:$HOST's user name:"
      read -s SDB_USER
      echo $SDB_USER
      echo "The port:$PORT in host:$HOST's password:"
      read -s SDB_PASSWD
      connDB=`$SDB "try{var db=new Sdb( '$HOST', $PORT, '$SDB_USER', '$SDB_PASSWD' )}catch(e){if(-179==e){println('AuthorityForbidden');}else{throw e;}}"`
      ret=$?
   done
   if [ "OtherError"X == "$connDB"X ];then
      echo "ERROR,failed to connect host:$HOST, port:$PORT"
      echo ""
      exit 1
   fi
}

#check over the password is right or wrong
function sdbCheckPassword()
{
   HOST=$1
   PASSWD=$2
   INSTALL_DIR=$3

   SDB=$INSTALL_DIR/bin/sdb
#*******************************************************************
#@ if failed to connect, this function will return 5, else return 0
#*******************************************************************
   retVal=`$SDB "try{var ssh = new Ssh('$HOST', '$USER', '$PASSWD' );}
                 catch(e){ if( -6 == e ){ println( 'Failed_Connect' );}
                 if( -13 == e ){ println( 'Error_Host' ); } }"`
   if [ "Failed_Connect"X == "$retVal"X ]; then
      return 5
   fi
   if [ "Error_Host"X == "$retVal"X ]; then
      return 13
   fi
   return 0
}

#ssh host and run sdbsupport
function sdbExpectSshHosts()
{
   HOST=$1
   USER=$2
   PASSWD=$3
   localPath=$4
   sdbsupport=$5
   INSTALL_DIR=$installpath

   SDB=$INSTALL_DIR/bin/sdb
   #echo "var ssh = new Ssh('$HOST', '$USER', '$PASSWD' )"
   $SDB "try{var ssh = new Ssh('$HOST', '$USER', '$PASSWD' );}catch(e){ if( 0 != e ){ println( 'Failed_Connect : ' + e ); }}"
   # the symbol '+' is because of javascript
   #echo "ssh.exec( 'cd $localPath; chmod +x sdbsupport.sh; $sdbsupport; mv sdbsupport.log sdbsupport.log.$HOST;' )"
   $SDB "ssh.exec( 'cd $localPath; chmod +x sdbsupport.sh; $sdbsupport; mv sdbsupport.log sdbsupport.log.$HOST;' )" >> /dev/null 2>&1
   if [ 0 -ne $? ] ; then
      echo "failed to execute the sdbsupport command:$?"
      sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "failed to run sdbsupport command: $EXE"
      return 5
   else
      #echo "Success to collect information from $HOST"
      sdbEchoLog "Event" "$FUNCNAME" "${LINENO}" "Success to collect information from $HOST"
      return 0
   fi
}

function sdbTarGzPack()
{
   HOST=$1
   date=`date '+%y%m%d-%H%M%S'`
   hard="true"
   sdbnode="true"
   osinfo="true"
   sdbsnap="true"

   Folder="$HOST-$date"
#echo "Begin to packaging and compression"
   mkdir -p $Folder/
   if [ $? -ne 0 ] ; then
      echo "Failed to create foler !"
      sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to create foler !"
      exit 1
   fi

   if ls HARDINFO/ >>/dev/null 2>&1
   then
      hard="false"
      mv HARDINFO/ ./$Folder/
   fi

   if ls SDBNODES/ >>/dev/null 2>&1
   then
      sdbnode="false"
      mv SDBNODES/ ./$Folder/
   fi

   if ls OSINFO/ >>/dev/null 2>&1
   then
      osinfo="false"
      mv OSINFO/ ./$Folder/
   fi

   if ls SDBSNAPS/ >>/dev/null 2>&1
   then
      sdbsnap="false"
      mv SDBSNAPS/ ./$Folder/
   fi

   if [ "$hard" == "true" ] && [ "$sdbnode" == "true" ] && [ "$osinfo" == "true" ] && [ "$sdbsnap" == "true" ] ; then
      echo "Error,Failed to collect $HOST information "
      sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Error,Failed to collect $HOST information "
      rm -rf ./$Folder/
      #exit 1
   fi

   if [ $? -ne 0 ] ; then
      echo "Failed to move the collected information to folder"
      sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Error,Failed to move $HOST information to folder"
      exit 1
   fi

   tar -zcvf $Folder.tar.gz ./$Folder/ >>/dev/null 2>&1
   if [ $? -ne 0 ] ; then
      tar -cvf $Folder.tar ./$Folder/ >>/dev/null 2>&1
      if [ 0 -ne $? ]; then
         echo "failed to packaging and compression"
         sdbEchoLog "ERROR" "$HOST/$0/${FUNCNAME}" "${LINENO}" "failed to packaging and compression "
         exit 1
      fi
   else
      #echo "Complete to packaging and compression"
      sdbEchoLog "EVENT" "$HOST/$0/${FUNCNAME}" "${LINENO}" "Success to Complete to packaging and compression"
   fi

   mv $Folder.tar.gz $localPath/log
   if [ $? -ne 0 ] ; then
      echo "failed to move pack-info to log folder."
   fi
   rm -rf ./$Folder/
   echo "success to collect information from $HOST"
}

# copy the file from remote host
function sdbExpectScpHosts()
{
   HOST=$1
   localPath=$2
   PASSWD=$3

   $SDB "try{var ssh = new Ssh('$HOST', '$USER', '$PASSWD' );}
         catch(e){ if( 0 != e ){ println( 'Failed_Connect' ); }}"
   #echo "$SDB \"ssh.exec( 'ls  $localPath/log/*$HOST*.tar.gz' )\""
   #$SDB "ssh.exec( 'ls $localPath/log/*$HOST*.tar.gz 2>/dev/null' )"
   verifyInfo=`$SDB "ssh.exec( 'ls $localPath/log/*$HOST*.tar.gz 2>/dev/null' )"`
   if [ ""X != "$verifyInfo"X ]; then
   for CPFILE in `$SDB "ssh.exec( 'ls  $localPath/log/*$HOST*.tar.gz' )"`
   do
      #FILE= `$SDB "ssh.exec( 'ls  $localPath/log/*$HOST*.tar.gz' )"`
      $SDB "ssh.pull( '$CPFILE', '$CPFILE' );"
   done

   if [ 0 -ne $? ] ; then
      echo "Failed to copy $HOST:$localPath/*$HOST*.tar.gz"
      sdbEchoLog "EVENT" "$HOST/$0/${FUNCNAME}" "${LINENO}" "Failed to scp $USER@$HOST:$localPath/*$HOST*.tar.gz"
   else
      #echo "Success to copy information from $HOST"
      sdbEchoLog "EVENT" "$HOST/$0/${FUNCNAME}" "${LINENO}" "Success to copy information from $HOST"
   fi
   echo "success to collect information from $HOST"
   else
      echo "failed to collect information from $HOST"
   fi
}

function sdbSupportLog()
{
   HOST=$1
   localPath=$2
   PASSWD=$3

   $SDB "try{var ssh = new Ssh('$HOST', '$USER', '$PASSWD' );}
         catch(e){ if( 0 != e ){ println( 'Failed_Connect' ); }}"
   $SDB "ssh.pull( '$localPath/sdbsupport.log.$HOST',
                   '$localPath/log/sdbsupport.log.$HOST' );"

   if [ 0 -ne $? ] ; then
      echo "Failed to copy $HOST:$localPath/sdbsupport.log"
      sdbEchoLog "EVENT" "$HOST/$0/${FUNCNAME}" "${LINENO}" "Failed to scp $USER@$HOST:$localPath/sdbsupport.log"
   else
      #echo "Success to copy sdbsupport.log"
      sdbEchoLog "EVENT" "$HOST/$0/${FUNCNAME}" "${LINENO}" "Success to copy sdbsupport.log"
   fi
}

function sdbSSHRemove()
{
   HOST=$1
   PASSWD=$2
   localPath=$3

   $SDB "try{var ssh = new Ssh('$HOST', '$USER', '$PASSWD' );}
         catch(e){ if( 0 != e ){ println( 'Failed_Connect' ); }}"
   $SDB "ssh.exec( 'cd $localPath; rm -rf log/ sdbsupport.log.$HOST;' );"

   if [ 0 -ne $? ] ; then
      echo "failed to clean the environment of host : $HOST"
      sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "
                  failed to clean the environment of host : $HOST"
   else
      #echo "Success to clean $HOST's Environment."
      sdbEchoLog "Event" "$FUNCNAME" "${LINENO}" "Success to clean $HOST's Environment."
   fi
}

function sdbEchoLog()
{
   Date=`date +%Y-%m-%d-%H:%M:%S.%N`

   echo "$Date          Level:$1"
   echo "File/Function:$2"
   echo "Line:$3"
   echo "Message:"
   echo "$4"
   echo ""
} >> sdbsupport.log

