#!/bin/bash

#******************************************************************************
#@Destcription:
#              SequoiaDB Port Config Or Sdbdialog Information Collect
#@Function List:
#               sdbPortInfo[Main]
#               sdbPortConf
#               sdbPortLog
#               sdbCmConfLog
#******************************************************************************
function sdbPortInfo()
{
   HOST=$1
   DBPATH=$2
   PORT=$3
   installpath=$4

   if [ "$PORT"X != ""X ] || [ "$sdbconf"X == "true"X ] ||
      [ "$sdbcm"X == "true"X ] || [ "$sdblog"X == "true"X ]; then
      # set sdbconf/sdblog/sdbcm true
      mkdir -p SDBNODES/
      if [ 0 -eq $? ]; then
         sdbPortConf
         sdbPortLog
         sdbCmConfLog
      else
         echo "failed to create folder SDBNODES, return code = $?"
      fi
      #if folder don't have file ,then delete it
      lsfold=`ls SDBNODES/` >>/dev/null 2>&1
      if [ "$lsfold" == "" ] ; then
         rm -rf SDBNODES/
      else
         sdbEchoLog "EVENT" "$FUNCNAME" "${LINENO}" "Success to collect config and log file in [$HOST:$PORT]"
      fi
   fi
}

# SVCPort_1: collect config file of service port
function sdbPortConf()
{
   if [ "$sdbconf" == "true" ] && [ "$PORT"X != ""X ]; then
      #get sequoiadb config path
      confpath=$installpath/conf/local
      ls $confpath/$PORT >> /dev/null 2>&1
      if [ $? -eq 0 ] ; then
         cp $confpath/$PORT/sdb.conf SDBNODES/$HOST.$PORT.sdb.conf >> /dev/null 2>&1
         if [ $? -ne 0 ] ; then
            echo "failed to collect sdb.conf from [ $HOST:$PORT ]"
            sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect $HOST:$confpath/$PORT/sdb.conf"
         fi
      fi
   fi
}
# SVCPort_2 : collect diaglog file of service port
function sdbPortLog()
{
   if [ "$sdblog"X == "true"X ] && [ "$PORT"X != ""X ]; then
      #collect sdbdiag.log file
      core=`find -name "*core*"`
      if [ "$core" != "" ] ; then
         tar -czvf $HOST.$PORT.diaglog.tar.gz $DBPATH/diaglog/ >>/dev/null 2>&1
         if [ 0 -ne $? ]; then
            tar -cvf $HOST.$PORT.diaglog.tar $DBPATH/diaglog/ >>/dev/null 2>&1
            if [ 0 -ne $? ]; then
               echo "failed to collect sdbdiag.log and core file in [ $HOST:$PORT ]"
               sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect $HOST:$DBPATH/diaglog/, include core"
            fi
         fi
      else
         cp $DBPATH/diaglog/sdbdiag.log SDBNODES/$HOST.$PORT.diaglog >>/dev/null 2>&1
         if [ $? -ne 0 ] ; then
            echo "failed to collect sdbdiag.log [ $HOST:$PORT ]"
            sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect $HOST:$DBPATH/diaglog/sdbdiag.log"
         fi
      fi
   fi
}
# SVCPort_3 : collect CM config file and CM diaglog file of service port
function sdbCmConfLog()
{
   if [ "$sdbcm"X == "true"X ] ; then
      #collect sdbcm log
      cmlogpath=$installpath/conf/log
      cmcfpath=$installpath/conf
      #collect sdbcmd log
      ls SDBNODES/$HOST.sdbcmd.log >>/dev/null 2>&1
      if [ $? -ne 0 ] ; then
         cp $cmlogpath/sdbcmd.log SDBNODES/$HOST.sdbcmd.log
         if [ $? -ne 0 ] ; then
            echo "failed to collect sdbcmd.log file from [ $HOST:$PORT ]"
            sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect sdbcmd.log file from [ $HOST:$PORT ]"
         fi
      fi
      #collect sdbcm log
      ls SDBNODES/$HOST.sdbcm.log >>/dev/null 2>&1
      if [ $? -ne 0 ] ; then
         cp $cmlogpath/sdbcm.log SDBNODES/$HOST.sdbcm.log
         if [ $? -ne 0 ] ; then
            echo "failed to collect sdbcm.log file"
            sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect sdbcm.log file [ $HOST:$PORT ]"
         fi
      fi
      #collect sdbcm config file
      ls SDBNODES/$HOST.sdbcm.conf >>/dev/null 2>&1
      if [ $? -ne 0 ] ; then
         cp $cmcfpath/sdbcm.conf SDBNODES/$HOST.sdbcm.conf
         if [ $? -ne 0 ] ; then
            echo "failed to collect sdbcm.conf file [ $HOST:$PORT ]"
            sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to collect sdbcm.conf file [ $HOST:$PORT ]"
         fi
      fi
   fi
}

#******************************************************************************
#@Destcription:
#              SequoiaDB Snapshot Information Collect
#@Function List:
#               sdbSnapShotInfo[Main]
#               sdbSnapShotGeneral
#               sdbSnapShotGroupCatalog
#******************************************************************************
function sdbSnapShotInfo()
{
   HOST=$1
   PORT=$2
   installpath=$3
   ROLE=$4

   if [ "$catalog"X != "false"X ] || [ "$group"X != "false"X ] ||
      [ "$context" == "true" ] || [ "$session" == "true" ] ||
      [ "$collection" == "true" ] || [ "$collectionspace" == "true" ] ||
      [ "$database" == "true" ] || [ "$system" == "true" ] ; then
      mkdir -p SDBSNAPS/
      if [ 0 -eq $? ]; then
         if [ "$PORT"X != ""X ]; then
            sdbSnapShotGeneral
            sdbSnapShotGroupCatalog
         fi
         return 0
      else
         echo "failed to create folder SDBSNAPS, return code = $?"
      fi
      #if folder don't have file ,then delete it
      lsfold=`ls SDBNODES/` >>/dev/null 2>&1
      if [ "$lsfold" == "" ] ; then
         rm -rf SDBNODES/
      else
         sdbEchoLog "EVENT" "$FUNCNAME" "${LINENO}" "Success to collect config and log file in [$HOST:$PORT]"
      fi
   fi
}

# SnapShot_1 : collect catalog snapshot information group information
function sdbSnapShotGroupCatalog()
{
   SDB=$installpath/bin/sdb
   if [ "$ROLE" == "coord" ] && [ "$catalog" == "true" ]; then
      $SDB "var db=new Sdb( '$HOST', $PORT, '$SDB_USER', '$SDB_PASSWD' )"
   if [ $? -ne 0 ]; then
      $SDB "quit"
      #echo "failed to connect to Sdb. please check your
      #      hostname/svcport/user/password"
   else
      if [ "$catalog" == "true" ] ; then
         $SDB "db.snapshot(SDB_SNAP_CATALOG)" >> SDBSNAPS/$HOST.$PORT.snapshot_catalog 2>&1
      fi
      if [ "$group" == "true" ] ; then
         $SDB "db.listReplicaGroups()" >> SDBSNAPS/$HOST.$PORT.listGroups 2>&1
      fi
      $SDB "quit"
   fi

   fi
}
# SnapShot_2 : collect snapshot information while in standalone or group
function sdbSnapShotGeneral()
{

   SDB=$installpath/bin/sdb
   if [ "data"X == "$ROLE"X ] || [ "coord"X == "$ROLE"X ] ||
      [ "cata"X == "$ROLE"X ]; then
      $SDB "var db=new Sdb( '$HOST', $PORT, '$SDB_USER', '$SDB_PASSWD' )"

if [ $? -ne 0 ] ; then
 $SDB "quit"
 echo "failed to connect to db[ '$HOST':$PORT ]. please check your hostname/svcport/user/password"
   sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "Failed to connect to sdb : $HOST/$PORT/$USER/$PASSWD"
else
   if [ "$context" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_CONTEXTS)" >> SDBSNAPS/$HOST.$PORT.snapshot_contexts
   fi
   if [ "$session" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_SESSIONS)" >> SDBSNAPS/$HOST.$PORT.snapshot_sessions
   fi
   if [ "$collection" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_COLLECTIONS)" >> SDBSNAPS/$HOST.$PORT.snapshot_collections
   fi
   if [ "$collectionspace" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_COLLECTIONSPACES)" >> SDBSNAPS/$HOST.$PORT.snapshot_collectionspace
   fi
   if [ "$database" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_DATABASE)" >> SDBSNAPS/$HOST.$PORT.snapshot_database
   fi
   if [ "$system" == "true" ] ; then
      $SDB "db.snapshot(SDB_SNAP_SYSTEM)" >> SDBSNAPS/$HOST.$PORT.snapshot_system
   fi
   $SDB "quit"
 fi


   fi
}


#******************************************************************************
#@Destcription:
#              Computer Hardware Information Collect
#@Function List:
#               sdbHWinfoCollect[Main]
#               sdbCpu
#               sdbMemory
#               sdbDisk
#               sdbNetcard
#               sdbMainboard
#******************************************************************************
function sdbHWinfoCollect()
{
   HOST=$1

   if [ "true"X == "$cpu"X ] || [ "true"X == "$memory"X ] ||
      [ "true"X == "$disk"X ] || [ "true"X == "$netcard"X ] ||
      [ "true"X == "$mainboard"X ]; then
      mkdir -p HARDINFO/
      if [ 0 -eq $? ]; then
         sdbCpu
         sdbMemory
         sdbDisk
         sdbNetcard
         sdbMainboard
      else
         echo "failed to create folder HARDINFO, return code = $?"
      fi
      #if folder don't have file ,then delete it
      lsfold=`ls HARDINFO/` >>/dev/null 2>&1
      if [ "$lsfold" == "" ] ; then
         rm -rf HARDINFO/
      else
         sdbEchoLog "EVENT" "$FUNCNAME" "${LINENO}" "Success to collect Hardware Information in host:$HOST"
      fi
   fi

}

# HWinfo_1 : collect cpu information
function sdbCpu()
{
   if [ "$cpu" == "true" ] ; then
      echo "######>lscpu" >> HARDINFO/$HOST.cpu.info 2>&1
      lscpu >> HARDINFO/$HOST.cpu.info 2>&1
      rc=$?
      echo "######>cat /proc/cpuinfo" >> HARDINFO/$HOST.cpu.info 2>&1
      cat /proc/cpuinfo >> HARDINFO/$HOST.cpu.info 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collec cpu information."
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{lscpu:Failed}"
         rm HARDINFO/$HOST.cpu.info
      fi
   fi
}
# HWinfo_2 : collect memory information
function sdbMemory()
{
   if [ "$memory" == "true" ] ; then
      echo "######>free -m" >> HARDINFO/$HOST.memory.info 2>&1
      free -m >> HARDINFO/$HOST.memory.info 2>&1
      rc=$?
      echo "######>cat /proc/meminfo" >> HARDINFO/$HOST.memory.info 2>&1
      cat /proc/meminfo >> HARDINFO/$HOST.memory.info 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect memory information."
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{free -m
         :Failed}"
         rm HARDINFO/$HOST.memory.info
      fi
   fi
}
# HWinfo_3 : collect disk information
function sdbDisk()
{
   if [ "$disk" == "true" ] ; then
      echo "######>lsblk" >> HARDINFO/$HOST.disk.info 2>&1
      lsblk >> HARDINFO/$HOST.disk.info 2>&1
      rc=$?
      echo "######>df -h" >> HARDINFO/$HOST.disk.info 2>&1
      df -h >> HARDINFO/$HOST.disk.info 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect disk information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{lsblk:Failed}"
         rm HARDINFO/$HOST.disk.info
      fi
   fi
}
# HWinfo_4 : collect network card information
function sdbNetcard()
{
   if [ "$netcard" == "true" ] ; then
      echo "######>lspci|grep -i 'eth'" >> HARDINFO/$HOST.netcard.info 2>&1
      lspci|grep -i 'eth' >> HARDINFO/$HOST.netcard.info 2>&1
      if [ $? -ne 0 ] ; then
         echo "Failed to collect netcard information"	
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{lspci|grep -i
         'eth':Failed}"
         rm HARDINFO/$HOST.netcard.info
      fi
   fi
}
# HWinfo_5: collect mainboard information
function sdbMainboard()
{
   if [ "$mainboard" == "true" ] ; then
      echo "######>lspci" >> HARDINFO/$HOST.mainboard.info 2>&1
      lspci >> HARDINFO/$HOST.mainboard.info 2>&1
      rc=$?
      echo "lspci -vv" >> HARDINFO/$HOST.mainboard.info 2>&1
      lspci -vv >> HARDINFO/$HOST.mainboard.info 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect mainboard information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{lspci:Failed}"
         rm HARDINFO/$HOST.mainboard.info 
      fi
   fi
}


#******************************************************************************
#@Destcription:
#              Operation System Information Collect
#@Function List:
#               sdbOSinfoCollect[Main]
#               sdbDiskManage
#               sdbSystemOS
#               sdbModules
#               sdbEnvVar
#               sdbNetworkInfo
#               sdbProcess
#               sdbLogin
#               sdbLimit
#               sdbVmstat
#******************************************************************************
function sdbOSinfoCollect()
{
   HOST=$1

   #echo "process: $process"
   if [ "$diskmanage" == "true" ] || [ "$osystem" == "true" ] ||
      [ "$kermode" == "true" ] || [ "$env" == "true" ] ||
      [ "$network" == "true" ] || [ "$process" == "true" ] ||
      [ "$login" == "true" ] || [ "$limit" == "true" ] ||
      [ "$vmstat" == "true" ] ; then
      mkdir -p OSINFO/
      if [ 0 -eq $? ]; then
         sdbDiskManage
         sdbSystemOS
         sdbModules
         sdbEnvVar
         sdbNetworkInfo
         sdbProcess
         sdbLogin
         sdbLimit
         sdbVmstat
      else
         echo "failed to create folder OSINFO, return code = $?"
      fi
      #if folder don't have file ,then delete it
      lsfold=`ls OSINFO/` >>/dev/null 2>&1
      if [ "$lsfold" == "" ] ; then
         rm -rf OSINFO/
      else
         sdbEchoLog "EVENT" "$FUNCNAME" "${LINENO}" "Success to collect OS information in host:$HOST"
      fi
   fi

}
# OSinfo_1 : collect disk manage information
function sdbDiskManage()
{
   if [ "$diskmanage"X == "true"X ] ; then
      echo "######>df ./   " >> OSINFO/$HOST.diskmanage.sys
      df ./ >> OSINFO/$HOST.diskmanage.sys 2>&1
      rc=$?
      echo "######>mount   " >> OSINFO/$HOST.diskmanage.sys
      mount >> OSINFO/$HOST.diskmanage.sys 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "Failed to collect disk manage information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "df ./:Failed"
         rm OSINFO/$HOST.diskmanage.sys
      fi
   fi
}
# OSinfo_2 : collect system information
function sdbSystemOS()
{
   if [ "$osystem"X == "true"X ] ; then
      echo "######>head -n 1 /etc/issue" >> OSINFO/$HOST.system.sys 2>&1
      head -n 1 /etc/issue >> OSINFO/$HOST.system.sys 2>&1
      rc=$?
      echo "######>cat /proc/version" >> OSINFO/$HOST.system.sys 2>&1
      cat /proc/version >> OSINFO/$HOST.system.sys 2>&1
      rc1=$?
      echo "######>hostname" >> OSINFO/$HOST.system.sys 2>&1
      hostname >> OSINFO/$HOST.system.sys 2>&1
      rc2=$?
      echo "######>getconf LONG_BIT" >> OSINFO/$HOST.system.sys 2>&1
      getconf LONG_BIT >> OSINFO/$HOST.system.sys 2>&1
      rc3=$?
      echo "######>ulimit -a" >> OSINFO/$HOST.system.sys 2>&1
      ulimit -a >> OSINFO/$HOST.system.sys 2>&1
      rc4=$?
      echo "######>lsb_release -a" >> OSINFO/$HOST.system.sys 2>&1
      lsb_release -a >> OSINFO/$HOST.system.sys 2>&1
      rc5=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] && [$rc2 -ne 0 ] &&
         [ $rc3 -ne 0 ] && [ $rc4 -ne 0 ] && [ $rc5 -ne 0 ] ; then
         echo "Failed to collect system information "
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "OSsys:Failed"
         rm OSINFO/$HOST.system.sys
      fi
   fi
}
# OSinfo_3 : collect modules information
function sdbModules()
{
   if [ "$kermode"X == "true"X ] ; then
      echo "######>lsmod" >> OSINFO/$HOST.modules.sys 2>&1
      lsmod >> OSINFO/$HOST.modules.sys 2>&1
      if [ $? -ne 0 ] ; then
         echo "failed to collect modules information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{lsmod:Failed}"
         rm $HOST.modules.sys
      fi
   fi
}
# OSinfo_4 : collect environment variable
function sdbEnvVar()
{
   if [ "$env"X == "true"X ] ; then
      echo "######>env" >> OSINFO/$HOST.environmentvar.sys 2>&1
      env >> OSINFO/$HOST.environmentvar.sys 2>&1
      if [ $? -ne 0 ] ; then
         echo "failed to collect environment variable"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{env:Failed}"
         rm OSINFO/$HOST.environmentvar.sys
      fi
   fi
}
# OSinfo_5 : collect network information
function sdbNetworkInfo()
{
   if [ "$network"X == "true"X ] ; then
      echo "######>netstat -s" >> OSINFO/$HOST.networkinfo.sys 2>&1
      netstat -s >> OSINFO/$HOST.networkinfo.sys 2>&1
      rc=$?
      echo "######>netstat" >> OSINFO/$HOST.networkinfo.sys 2>&1
      netstat >> OSINFO/$HOST.networkinfo.sys 2>&1
      rc1=$?
      if [ $? -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect network information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{netstat -s/netstat:Failed}"
         rm OSINFO/$HOST.networkinfo.sys
      fi
   fi
}
# OSinfo_6 : collet process information
function sdbProcess()
{
   if [ "$process"X == "true"X ] ; then
      echo "######>ps -elf|sort -rn" >> OSINFO/$HOST.process.sys 2>&1
      ps -elf|sort -rn >> OSINFO/$HOST.process.sys 2>&1
      rc=$?
      echo "######>ps aux" >> OSINFO/$HOST.process.sys 2>&1
      ps aux >> OSINFO/$HOST.process.sys 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect process information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{ps aux/ps
         -elf|sort -rn:Failed}"
         rm OSINFO/$HOST.process.sys
      fi
   fi
}
# OSinfo_7 : collect login information
function sdbLogin()
{
   if [ "$login"X == "true"X ] ; then
      echo "######>last" >> OSINFO/$HOST.logininfo.sys 2>&1
      last >> OSINFO/$HOST.logininfo.sys 2>&1
      rc=$? 
      echo "######>history" >> OSINFO/$HOST.logininfo.sys 2>&1
      history >> OSINFO/$HOST.logininfo.sys 2>&1
      rc1=$?
      if [ $rc -ne 0 ] && [ $rc1 -ne 0 ] ; then
         echo "failed to collect login information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "last/history:Failed"
         rm OSINFO/$HOST.logininfo.sys
      fi
   fi
}
# OSinfo_8 : collect limit information
function sdbLimit()
{
   if [ "$limit"X == "true"X ] ; then
      echo "######>ulimit -a" >> OSINFO/$HOST.ulimit.sys 2>&1
      ulimit -a >> OSINFO/$HOST.ulimit.sys 2>&1
      if [ $? -ne 0 ] ; then
         echo "failed to collect system limit information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{ulimit -a:Failed}"
         rm OSINFO/$HOST.ulimit.sys
      fi
   fi
}
# OSinfo_9 : collect vmstat information
function sdbVmstat()
{
   if [ "$vmstat"X == "true"X ] ; then
      echo "######>vmstat" >> OSINFO/$HOST.vmstat.sys 2>&1
      vmstat >> OSINFO/$HOST.vmstat.sys 2>&1
      if [ $? -ne 0 ] ; then
         echo "failed to collect vmstat information"
         sdbEchoLog "ERROR" "$FUNCNAME" "${LINENO}" "{vmstat:Failed}"
         rm OSINFO/$HOST.vmstat.sys
      fi
   fi
}
