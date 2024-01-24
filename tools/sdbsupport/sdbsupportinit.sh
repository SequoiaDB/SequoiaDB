#!/bin/bash
#variable in bash shell

confpath=""
pHostNum=""

#user permission
USER=`whoami`
if [ "" == $USER ] ; then
   USER="sdbadmin"
fi

#user name and password for db
SDB_USER=""
SDB_PASSWD=""
IS_USER="false"
IS_PASSWD="false"

#gloable variable
all="false";
hostName=""; svcPort=""; sdbuser=""; sdbpassword="";
sysInfo="false"; snapShot="false"; hardInfo="false";
IsHost="false"; IsPort="false";

#sdblog and conf
sdbconf="false"; sdblog="false"; sdbcm="false"

#hardware information variable
cpu="false"; memory="false"; disk="false"; netcard="false"; mainboard="false";

#snapshot variable
rcPort="false"; group="false"; context="false"; session="false";
collection="false"; collectionspace="false"; database="false"
system="false"; catalog="false";

#operation system variable
diskmanage="false"; osystem="false"; env="false"; network="false";
process="false"; login="false"; limit="false"; vmstat="false"; kermode="false";

#the parameter get where location
firstLoc=$1
thirdLoc=$3
fifthLoc=$5
sevenLoc=$7

#the number of concurrent threads
thread=10
timeout=60

#get the number of parameter and what parameters is
ParaNum=$#
ParaPass=$@

#check is local host run
Local=""

function Usage()
{
   echo "Command Options:"
   echo "    --help                 help information"
   echo "    -s [--hostname] arg    host name [eg:-s hostname1:hostname2:.....]"
   echo "    -p [--svcname] arg     service name [eg:-p 50000:30000:......]"
#   echo "    -t [--thread] arg      number of concurrent threads,default:10"
   echo "    -u [--user] arg        user name"
   echo "    -w [--password] arg    password"
   echo "    -n [--snapshot]        snapshot of database "
   echo "    -o [--osinfo]          operating system information"
   echo "    -h [--hardware]        hardware information"
   echo "    --all                  collect all information"
   echo "    --conf                 collect config file"
   echo "    --log                  collect sdb log file"
   echo "    --cm                   collect sdbcm config and log file"

   echo "    --cpu                  host cpu information"
   echo "    --memory               host memory information"
   echo "    --disk                 host disk information"
   echo "    --netcard              host netcard information"
   echo "    --mainboard            host mainboard information"

   echo "    --catalog              catalog snapshot"
   echo "    --group                group information of dababase"
   echo "    --context              context snapshot"
   echo "    --session              session snapshot"
   echo "    --collection           collection snapshot"
   echo "    --collectionspace      collectionspace snapshot"
   echo "    --database             database snapshot"
   echo "    --system               system snapshot"

   echo "    --diskmanage           operating system disk management information"
   echo "    --basicsys             operating system basic information"
   echo "    --module               loadable kernel modules"
   echo "    --env                  operating system environment variable"
   echo "    --network              network information"
   echo "    --process              operating system process"
   echo "    --login                operating system users and history"
   echo "    --limit                limit the use of system-wide resources"
   echo "    --vmstat               Show the server status value of a given time interval"
#   echo "    --timeout              Set too much time to collect,default:50"

}

#the parameters can use
optArg=`getopt -a -o s:p:t:u:w:nohH -l hostname,svcport,thread,sdbuser,sdbpassword,snapshot,osinfo,hardware,help,conf,log,cm,cpu,memory,disk,netcard,mainboard,group,context,session,collection,collectionspace,database,system,diskmanage,basicsys,module,env,network,process,login,limit,vmstat,catalog,all,timeout: -- "$@"`

#check over the option of sdbsupport
rc=$?
if [ "$rc" == "1" ] ; then
   echo "The option don't have,please check by use '--help'!"
   exit 1
fi


eval set -- "$optArg"

while true
do
#eval set -- "$optArg"
   case $1 in
   -s|--hostname)
      hostName=$2
      IsHost="true"
      shift
      ;;
   -p|--svcport)
      svcPort=$2
      IsPort="true"
      shift
      ;;
   -t|--thread)
      thread=$2
      shift
      ;;
   -u|--user)
      SDB_USER=$2
      IS_USER="true"
      shift
      ;;
   -w|--password)
      SDB_PASSWD=$2
      IS_PASSWD="true"
      shift
      ;;
   --timeout)
      timeout=$2
      shift
      ;;
   -n|--snapshot)
      snapShot="true"
      ;;
   -o|--osinfo)
      sysInfo="true"
      ;;
   -h|--hardware)
      hardInfo="true"
      ;;
   --conf)
      sdbconf="true"
      ;;
   --log)
      sdblog="true"
      ;;
   --cm)
      sdbcm="true"
      ;;
   --cpu)
      cpu="true"
      ;;
   --memory)
      memory="true"
      ;;
   --disk)
      disk="true"
      ;;
   --netcard)
      netcard="true"
      ;;
   --mainboard)
      mainboard="true"
      ;;
   --group)
      group="true"
      rcPort="true"
      ;;
   --context)
      context="true"
      rcPort="true"
      ;;
   --session)
      session="true"
      rcPort="true"
      ;;
   --collection)
      collection="true"
      rcPort="true"
      ;;
   --collectionspace)
      collectionspace="true"
      rcPort="true"
      ;;
   --database)
      database="true"
      rcPort="true"
      ;;
   --system)
      system="true"
      rcPort="true"
      ;;
   --diskmanage)
      diskmanage="true"
      ;;
   --basicsys)
      osystem="true"
      ;;
   --module )
      kermode="true"
      ;;
   --env)
      env="true"
      ;;
   --network)
      network="true"
      ;;
   --process)
      process="true"
      ;;
   --login)
      login="true"
      ;;
   --limit)
      limit="true"
      ;;
   --vmstat)
      vmstat="true"
      ;;
   --catalog)
      catalog="true"
      ;;
   --all)
      all="true"
      ;;
   -H|--help)
      Usage
      exit 1
      ;;
   --)
      shift
      break
      ;;
   esac
shift
done


#******************************************************************************
#@Description: releationship of every arguement value
#******************************************************************************
# [command: ./sdbsupport.sh ]
if [ ""X == "$firstLoc"X ]; then
   sysInfo="true"; snapShot="true"; hardInfo="true";
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsupport.sh -u username -p password]
if [ ""X == "$fifthLoc"X ] && [ "true"X == "$IS_USER"X ] &&
   [ "true"X == "$IS_PASSWD"X ]; then
   sysInfo="true"; snapShot="true"; hardInfo="true";
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsupport.sh --all]
if [ "true"X == "$all"X ]; then
   sysInfo="true"; snapShot="true"; hardInfo="true";
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsupport.sh -s hostname1 -u username -p password]
if [ "true"X == "$IsHost"X ] && [ "false"X == "$IsPort"X ] &&
   [ ""X == "$sevenLoc"X ] && [ "true"X == "$IS_USER"X ] &&
   [ "true"X == "$IS_PASSWD"X ]; then
   sysInfo="true"; snapShot="true"; hardInfo="true";
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsuport.sh -s hostname1]
if [ "true"X == "$IsHost"X ] && [ "false"X == "$IsPort"X ] &&
   [ ""X == "$thirdLoc"X ]; then
   sysInfo="true"; snapShot="true"; hardInfo="true";
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsupport.sh -s hostname1 -p 50000]
if [ "false"X == "$sdbconf"X ] && [ "false"X == "$sdblog"X ] &&
   [ "false"X == "$sdbcm"X ] && [ "true"X == "$IsPort"X ]; then
   sdbconf="true"; sdblog="true"; sdbcm="true";
fi

# [command: ./sdbsupport.sh -s hostname1 -p 50000 --hardware]
if [ "true"X == "$hardInfo"X ]; then
   cpu="true"; memory="true"; disk="true"; netcard="true"; mainboard="true";
fi

# [command: ./sdbsupport.sh -s hostname1 -p 50000 --snapshot]
if [ "true"X == "$snapShot"X ]; then
   rcPort="true"; group="true"; context="true"; session="true";
   collection="true"; collectionspace="true"; database="true"
   system="true"; catalog="true";
fi

# [command: ./sdbsupport.sh -s hostname1 -p 50000 --osinfo]
if [ "true"X == "$sysInfo"X ]; then
   diskmanage="true"; osystem="true"; env="true";
   network="true"; process="true"; login="true";
   limit="true"; vmstat="true"; kermode="true";
fi
