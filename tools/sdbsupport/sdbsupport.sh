#!/bin/bash
BashPath=$(dirname $(readlink -f $0))
. $BashPath/sdbsupportinit.sh
. $BashPath/sdbsupportfunc1.sh
. $BashPath/sdbsupportfunc2.sh
. /etc/default/sequoiadb

echo ""
echo "************************************Sdbsupport***************************"
echo "* This program run mode will collect all configuration and "
echo "* system environment information.Please make sure whether"
echo "* you need !"
echo "* Begin ....."
echo "*************************************************************************"
echo ""

#******************************************************************************
#@ Step 1 : check over environment
#******************************************************************************
# mv sdbsupport.log sdbsupport.log.1 >>sdbsupport.log 2>&1
rm -rf sdbsupport.log
if [ $? -ne 0 ] ; then
   echo "failed to remove sdbsupport.log file."
fi
# get the local path
dirpath=`pwd`
if [ "$dirpath" == "" ] ; then
   echo "failed to get local directory."
   echo ""
   exit 1
fi
# get local host and local path
localhost=`hostname`
localPath=$(dirname $(readlink -f $0))
if [ "$localhost" == "" ] || [ "$localPath" == "" ] ; then
   echo "failed to get local host and local path."
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Failed to get local host:$localhost and local path:$localPath."
   echo ""
   exit 1
fi
# create local path folder log that store information
mkdir -p $localPath/log
if [ $? -ne 0 ] ;then
   echo "failed to create directory for sdbsupport.sh"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Failed to create folder log in local path"
   echo ""
   exit 1
fi
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 0: passed comman:$0 $ParaPass"
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 1: Success to check over environment"

#******************************************************************************
#@ Step 2 : get install path from /etc/default/sequoiadb file and check path
#@ Description :
#   NAME=sdbcm
#   SDBADMIN_USER=sdbadmin
#   INSTALL_DIR=/opt/sequoiadb
#******************************************************************************
# get install path
installpath=$INSTALL_DIR
if [ "" == $installpath ] ; then
   echo "don't have file /etc/default/sequoiadb, make sure your host is
         installed sequoiadb or not"
   echo ""
   exit -1
fi
ls $installpath >/dev/null 2>&1
if [ $? -ne 0 ] ; then
   echo "Wrong install path, Please check over the sdbsupport path and installpath!"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Wrong install path:$installpath"
   echo ""
   exit 1
else
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Success to get install path:$installpath"
fi
cd $dirpath
if [ $? -ne 0 ] ; then
   echo "Failed to come back to sdbsupport directory"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Failed to come back to sdbsupport directory:$localPath"
   echo ""
   exit 1
fi
# get config file's path
confpath=$installpath/conf/local
if ls $confpath >>/dev/null 2>&1
then
   retval=`ls $confpath`
   if [ "$retval" == "" ] ; then
      echo "database don't have sdb nodes!"
      sdbEchoLog "ERROR" "$0" "${LINENO}" "Database don't have sdb nodes. config path:$confpath"
      echo ""
      exit 1
   else
      echo "check over environment, correct!"
      sdbEchoLog "EVENT" "$0" "${LINENO}" "Database have nodes correct config path:$confpath"
   fi
else
   echo "wrong config path: $confpath"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Wrong Config path:$confpath"
   echo ""
   exit 1
fi
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 2: Success to get install path: $installpath and config path: $confpath"

#******************************************************************************
#@ Step 3 : create concurrent threads
#******************************************************************************
fifo="/tmp/$$.fiofo"
mkfifo $fifo
if [ $? -ne 0 ] ; then
   echo "failed to create FIFO, have no threads"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Failed to create concurrent threads"
else
   exec 6<>$fifo
   rm -rf $fifo
   for ((i=0;i<$thread;i++))
   do
      echo ""
   done >&6
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 3: Success to create concurrent threads"
fi

#******************************************************************************
#@ Step 4 : get quantity of all hosts and local service port
#******************************************************************************
cd $confpath
aloneRole=`find -name "*.conf"|xargs grep "\brole.*=.*standalone\b"|cut -d "/" -f 2`
coordRole=`find -name "*.conf"|xargs grep "\brole.*=.*coord\b"|cut -d "/" -f 2`
cataRole=`find -name "*.conf"|xargs grep "\brole.*=.*cata\b"|cut -d "/" -f 2`
dataRole=`find -name "*.conf"|xargs grep  "\brole.*=.*data\b"|cut -d "/" -f 2`
#echo "dataRole:$dataRole : aloneRole:$aloneRole"
cd $localPath
# check DB is used or not. check it's database
if [ "$aloneRole" == "" ] && [ "$coordRole" == "" ] &&
   [ "$cataRole" == "" ] && [ "$dataRole"=="" ] ; then
   echo "local host don't create database"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "Local host don't create database database"
   echo ""
   exit 1
fi

if [ "$dataRole" != "" ] ; then
   echo "complete database cluster"
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Complete database database cluster"
   dataRole=$dataRole
else
   echo "there are not database cluster"
fi

data=`echo $dataRole | cut -d " " -f 1`
cataddr=`grep -E "catalogaddr.*=" $confpath/$data/sdb.conf|cut -d '=' -f 2`
HostNum=`awk 'BEGIN{print split('"\"$cataddr\""',cateArr,",")}'`
PortNum=`ls -l $confpath|grep "^d"|wc -l`

if [ "$HostNum" != "0" ] && [ "$PortNum" != "0" ] ; then
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Success to get the number of host in group and port in localhost"
else
   echo "No host and port,Please check!"
   sdbEchoLog "ERROR" "$0" "${LINENO}" "No host and port,Please check!"
   echo ""
   exit 1
fi
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 4: Success to get quantity of host: $HostNum and port: $PortNum"
#*******************************************************************************
#@ Step 5 : get all hosts in database and local host's port/dbpath/role
#         HOST      Exp : Array variable used to store hosts in database
#         PORT      Exp : Array variable store local host's sevice port
#         DBPATH    Exp : Array variable store local host's dbpath
#         ROLE      Exp : Array variable store local hsot's sevice port's role
#*******************************************************************************
for i in $(seq 1 $HostNum)
do
   hostcata[$i]=`awk 'BEGIN{split('"\"$cataddr\""',cateArr,",");print cateArr['$i']'}`
   HOST[$i]=`echo ${hostcata[$i]}|cut -d ":" -f 1 `
   if [ "${HOST[$i]}" == "$localhost" ] ; then
      for j in $(seq 1 $PortNum)
      do
         PortArr=`ls $confpath`
         PORT[$j]=`echo $PortArr|cut -d " " -f $j`
         if [ "${PORT[$j]}" == "$aloneRole" ] ; then
            PORT[$j]=""
            continue
         fi
         DBPATH[$j]=`grep -E "dbpath" $confpath/${PORT[$j]}/sdb.conf|cut -d '=' -f 2`
         #delete the space in config file and put in tmpconf
         sed -i 's/\ //g' $confpath/${PORT[$j]}/sdb.conf 
            ROLE[$j]=`grep -E "role=" $confpath/${PORT[$j]}/sdb.conf|cut -d '=' -f 2`
      done
   fi
done
DbPath=`echo ${DBPATH[@]}`
AllHost=`echo ${HOST[@]}`
AllPort=`echo ${PORT[@]}`
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 5: Success to get all host: [$AllHost], all port: [$AllPort] and all dbpath: [$DbPath]"
#*************************************************************************************************
#@ Step 6 : Get parameter passed in and check over them wether or not correct,if don't have this
#           Host or Port ,will delete the wrong host and port
#         pHostNum  Exp : quantity of parameter hosts, such as :--hostname ubunt-dev1:ubunt-dev2:
#         pPortNum  Exp : quantity of parameter sevice port, such as:--svcport 51111:61111
#         HostPara  Exp : Array variable to store hosts parameter
#         PortPara  Exp : Array variable to store local hosts' sevice port
#*************************************************************************************************
pHostNum=`awk 'BEGIN{print split('"\"$hostName\""',hostarr,":")}'`
pPortNum=`awk 'BEGIN{print split('"\"$svcPort\""',portarr,":")}'`
#when have parameter ,but not --all ,we must specify the hosts[--hostname]
if [ $pHostNum -eq 0 ] && [ "$all" == "false" ] && [ "$fifthLoc" != "" ] &&
   [ "true"X == "$IS_USER"X ] && [ "true"X == "$IS_PASSWD"X ]; then
   echo "Warning ! Please specify hosts!"
   echo ""
   exit 1
fi
#Check over Host
for i in $(seq 1 $pHostNum)
do
   HostPara[$i]=`awk 'BEGIN{split('"\"$hostName\""',hostarr,":");print hostarr['$i']}'`
   HostNumAdd=$(($HostNum+1))
   for j in $(seq 1 $HostNumAdd)
   do
      if [ "${HostPara[$i]}" == "${HOST[$j]}" ] ; then
         break
      fi
      #******************************************************************************
      #Note:check the argument of HOST,when the host para not equal the HOST that we
      #     get from database config file .We put null in the para localed in Array.
      #******************************************************************************
      if [ $j -gt $HostNum ] ; then
         echo "WARNIGN,database don't have host:${HostPara[$i]}"
         sdbEchoLog "WARNING" "$0" "${LINENO}" "Don't have host:${HostPara[$i]}"
         HostPara[$i]=""
      fi
   done
done
#Check over Port
for i in $(seq 1 $pPortNum)
do
   PortPara[$i]=`awk 'BEGIN{split('"\"$svcPort\""',portarr,":");print portarr['$i']}'`
   PortNumAdd=$(($PortNum+1))
   for j in $(seq 1 $PortNumAdd)
   do
      if [ "${PortPara[$i]}" == "${PORT[$j]}" ] ; then
         DbPath[$i]=${DBPATH[$j]}
         Role[$i]=${ROLE[$j]}
         break
      fi
      if [ $j -gt $PortNum ] ; then
         echo "WARNIGN,database don't have port:${PortPara[$i]}"
         sdbEchoLog "WARNING" "$0" "${LINENO}" "Don't have port:${PortPara[$i]}"
         PortPara[$i]=""
      fi
   done
done
paraHost=`echo ${HostPara[@]}`
paraPort=`echo ${PortPara[@]}`
paraDbpath=`echo ${DbPath[@]}`
sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 6: passed host:[$paraHost], passed port:[$paraPort] and passed dbpath:[$paraDbpath]"

#******************************************************************************
#@ Step 7 : get password of host when you begin to collect information
#******************************************************************************
# when collect all infomation
user=$USER
if [ "$all" == "true" ] ; then
   for i in $(seq 1 $HostNum)
   do
      if [ "${HOST[$i]}" != "$localhost" ] ; then
         echo "The host $user@${HOST[$i]}'s password :"
         read -s PASSWD[$i]
         sdbCheckPassword "${HOST[$i]}" "${PASSWD[$i]}" "$installpath"
         retVal=$?
         if [ 13 -eq $retVal ]; then
            echo "failed to connect to Host : ${HOST[$i]}"
            HOST[$i]=""
         fi
         while [ "$retVal" == "5" ]
         do
            PASSWD[$i]=""
            echo "wrong password of host: $user@${HOST[$i]}, please enter again :"
            read -s PASSWD[$i]
            sdbCheckPassword "${HOST[$i]}" "${PASSWD[$i]}" "$installpath"
            retVal=$?
         done
         echo "correct password for ${HOST[$i]}"
         StorePasswd[$i]=${HOST[$i]}":"${PASSWD[$i]}
      fi
   done
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 7: Check over password of all host"
fi
# when collect remote host information
if [ "$pHostNum" -gt 0 ] && [ "$all" == "false" ] ; then
   for i in $(seq 1 $pHostNum)
   do
      if [ "${HostPara[$i]}" != "" ] && [ "${HostPara[$i]}" != "$localhost" ] ; then
         echo "The host $user@${HostPara[$i]}'s password :"
         read -s PASSWD[$i]
         sdbCheckPassword "${HostPara[$i]}" "${PASSWD[$i]}" "$installpath"
         retVal=$?
         if [ 13 -eq $retVal ]; then
            echo "failed to connect to Host : ${HostPara[$i]} "
            HostPara[$i]=""
         fi
         while [ "$retVal" == "5" ]
         do
            PASSWD[$i]=""
            echo "wrong password of host: $user@${HostPara[$i]}, please enter again :"
            read -s PASSWD[$i]
            sdbCheckPassword "${HostPara[$i]}" "${PASSWD[$i]}" "$installpath"
            retVal=$?
         done
         echo "correct password for ${HostPara[$i]}"
         StorePasswd[$i]=${HostPara[$i][$i]}":"${PASSWD[$i]}
      fi
   done
   sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 7: Check over password of specify host"
fi

#******************************************************************************
#@ Step 8 : 1.create folder OSINFO/SDBNODES/SDBSNAPS/HARDINFO in local path
#@          2.begin to collect infomation in here [Main Run]
#@ Description1 :
#      OSINFO   [directory for Operation System Information]
#      SDBNODES [directory for database all nodes]
#      SDBSNAPS [directory for database snapshot]
#      HARDINFO [directory for hardware information]
#@ Description2 :
#      Collect local host information about database,such as 'Dialog',
#      'Conf', 'Group', 'Snapshot', 'Hardware' and 'System information' !
#******************************************************************************
# remove the folder that store collect information
   rm -rf HARDINFO/ OSINFO/ SDBNODES/ SDBSNAPS/
   if [ $? -ne 0 ] ; then
      echo "failed to remove folder HARDINFO/ OSINFO/ SDBNODES/ SDBSNAPS/"
      sdbEchoLog "ERROR" "$0" "${LINENO}" "Failed to remove folder HARDINFO/ OSINFO/ SDBNODES/ SDBSNAPS/"
   else
      sdbEchoLog "EVENT" "$0" "${LINENO}" "Success to remove the folder HARDINFO/ OSINFO/ SDBNODES/ SDBSNAPS/"
   fi
# check passed parameters
#if [ ""X == "${HostPara[@]}"X ] && [ "true"X == "$IsHost"X ]; then
#   echo ""
#   exit 1
#fi
echo ""
echo "Begin to Collect information..."

# COLLECT1: collect local host all information.[ exp: ./sdbsupport.sh]
for i in $(seq 1 $HostNum)
do
   if [ "$firstLoc"X == ""X ] && [ "$localhost"X == "${HOST[$i]}"X ] ; then
      for j in $(seq 1 $PortNum)
      do
         #echo "localhost:$localhost:${PORT[$j]}"
         sdbEchoLog "EVENT" "$0" "${LINENO}" "Start collect localhost only:[HostNum:$HostNum]-[PortNume:$PortNum]"
         sdbPortInfo "${HOST[$i]}" "${DBPATH[$j]}" "${PORT[$j]}" "$installpath"
         if [ "coord"X == "${ROLE[$j]}"X ] || [ "cata"X == "${ROLE[$j]}"X ] || [ "data"X == "${ROLE[$j]}"X ];then
            connectDB "${HOST[$i]}" "${PORT[$j]}" "$installpath"
            sdbSnapShotInfo "${HOST[$i]}" "${PORT[$j]}" "$installpath" "${ROLE[$j]}"
         fi
      done
      sdbHWinfoCollect "${HOST[$i]}"
      sdbOSinfoCollect "${HOST[$i]}"
      sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 8: Success to collect information from localhost: $localhost"
   fi
done

c_user=$USER
if [ ""X != "$firstLoc"X ]; then
   for i in $(seq 1 $HostNum)
   do
      # get the host that you passed in.
      cnt=0
      for m in $(seq 1 $pHostNum)
      do
         if [ "false"X == "$all"X ] &&
            [ "${HOST[$i]}"X != "${HostPara[$m]}"X ]; then
            cnt=$((cnt+1))
            continue
         else
            break
         fi
      done
      if [ "$cnt"X == "$pHostNum"X ] && [ 0 -ne $pHostNum ]; then
         HOST[$i]=""
         continue
      fi
      # parsed parameters that you passed in. exp: ./sdbsupport.sh -s host1:host2 -p 11810:11820 ...
      PARAPASS=""
      for n in $(seq 1 $ParaNum)
      do
         Para[$n]=`echo $ParaPass|cut -d " " -f $n`
         if [ "${Para[$n]}"X != "-s"X ] &&
            [ "${Para[$n]}"X != "--hostname"X ] &&
            [ "${Para[$n]}"X != "$hostName"X ] &&
            [ "${Para[$n]}"X != "-u"X ] &&
            [ "${Para[$n]}"X != "--user"X ] &&
            [ "${Para[$n]}"X != "$SDB_USER"X ] &&
            [ "${Para[$n]}"X != "-w"X ] &&
            [ "${Para[$n]}"X != "--password"X ] &&
            [ "${Para[$n]}"X != "$SDB_PASSWD"X ]; then 
            PARAPASS="$PARAPASS ${Para[$n]}"
            #echo ">>$n>>$PARAPASS"
         fi
      done
      # verify username and password in database
      for j in $(seq 1 $PortNum)
      do
         if [ "coord"X == "${ROLE[$j]}"X ]; then
            connectDB "$localhost" "${PORT[$j]}" "$installpath"
         fi
      done
      # when host equal localhost, we collect information
      if [ "${HOST[$i]}"X == "$localhost"X ] ; then
         for j in $(seq 1 $PortNum)
         do
            # get the port that you passed in.
            cnt=0
            for n in $(seq 1 $pPortNum)
            do
               if [ "false"X == "$all"X ] &&
                  [ "${PORT[$j]}"X != "${PortPara[$n]}"X ]; then
                  cnt=$((cnt+1))
                  continue
               else
                  break
               fi
            done
            if [ "$cnt"X == "$pPortNum"X ] && [ 0 -ne $pPortNum ]; then
               PORT[$j]=""
               continue
            fi
            sdbPortInfo "${HOST[$i]}" "${DBPATH[$j]}" "${PORT[$j]}" "$installpath"
            sdbSnapShotInfo "${HOST[$i]}" "${PORT[$j]}" "$installpath" "${ROLE[$j]}"
         done
         sdbHWinfoCollect "${HOST[$i]}"
         sdbOSinfoCollect "${HOST[$i]}"
         Local=$localhost
         sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 8: Success to collect information from host: $localhost"
         continue
      fi
      StoreNum=${#StorePasswd[@]}
      PASSWD=""
      for k in $(seq 1 $((StoreNum+1)))
      do
         SHost=`echo ${StorePasswd[$k]} | cut -d ":" -f 1`
         SPasswd=`echo ${StorePasswd[$k]} | cut -d ":" -f 2`
         if [ "${HOST[$i]}"X == "$SHost"X ];then
            PASSWD=$SPasswd
            break
         fi
      done
      # combine passed parameter
      if [ "true"X == "$all"X ]; then
         if [ ""X != "$SDB_USER"X ] && [ ""X != "$SDB_PASSWD"X ]; then
            sdbsupport="./sdbsupport.sh -s ${HOST[$i]} -u $SDB_USER -w $SDB_PASSWD"
         else
            sdbsupport="./sdbsupport.sh -s ${HOST[$i]}"
         fi
      else
         if [ ""X != "$SDB_USER"X ] && [ ""X != "$SDB_PASSWD"X ]; then
            sdbsupport="./sdbsupport.sh -s ${HOST[$i]} $PARAPASS -u $SDB_USER -w $SDB_PASSWD"
         else
            sdbsupport="./sdbsupport.sh -s ${HOST[$i]} $PARAPASS"
         fi
      fi
      # if run command [./sdbsupport.sh -u username -p password] 
      if [ ""X == "$fifthLoc"X ] && [ "true"X == "$IS_USER"X ] &&
         [ "true"X == "$IS_PASSWD"X ]; then
         continue
      fi

      read -u 6
      if [ "${HOST[$i]}"X != ""X ] && [ "${HOST[$i]}"X != "$localhost"X ] ; then
      {
         if [ ""X == "$PASSWD"X ]; then
            echo "the ${HOST[$i]} don't get correct password"
            break
         fi
         #echo "SDBSUPPORT: $sdbsupport"
         #ssh host and collect information
         sdbExpectSshHosts "${HOST[$i]}" "$c_user" "$PASSWD" "$localPath" "$sdbsupport"
         sdbExpectScpHosts "${HOST[$i]}" "$localPath" "$PASSWD"
         sdbSupportLog "${HOST[$i]}" "$localPath" "$PASSWD"
         sdbSSHRemove "${HOST[$i]}" "$PASSWD" "$localPath"
         sdbEchoLog "EVENT" "$0" "${LINENO}" "Step 8: Success to collect information from host: $localhost"
         echo "" >&6
      }&
      fi
   done
   wait
fi

#******************************************************************************
#@ Step 9 : 1.compressed collection file into packet in local host
#@          2.clean the temp file and temp folder in local host
#******************************************************************************
# compressed the all collect information into packet
if [ "$firstLoc" == "" ] || [ "$Local" == "$localhost" ]; then
   sdbTarGzPack $localhost
fi

# clean environment
exec 6>&-
sdbEchoLog "EVENT" "$FUNCNAME" "${LINENO}" "Collect information Over"
cp sdbsupport.log ./log/sdbsupport.log.$localhost >> /dev/null 2>&1
rc=$?
if [ $rc -ne 0 ] ;then
   echo "Failed to copy local sdbsupport.log to log folder."
fi

# copy log folder in sdbsupport directory to local directory
if [ "$localPath" != "$dirpath" ]; then
   cp -r $localPath/log $dirpath
   if [ $? -ne 0 ] ; then
      echo "Failed to copy information to local directory."
   fi
   #rm -rf $localPath/log
   if [ $? -ne 0 ] ;then
      echo "Failed to remove the log folder in directory:$localPath"
   fi
fi
echo ""
