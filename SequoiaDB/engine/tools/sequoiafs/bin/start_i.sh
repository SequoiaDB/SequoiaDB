#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)


if [ -f "/opt/sequoiadb/fuse/bin/fusermount" ]; then
    PATH=/opt/sequoiadb/fuse/bin:$PATH
fi
if [ ! -x "$(command -v fusermount)" ] ; then
  echo "cannot find fusermount"
  exit 1
fi 

fsbin="$BashPath/sequoiafs"
confrootpath="$BashPath/../conf/local"
logrootpath="$BashPath/../log"
fusetype="fuse.sequoiafs"
if [ -d "$confrootpath" ]; then
  confrootpath=$(cd "$BashPath/../conf/local"; pwd)
fi
if [ -d "$logrootpath" ]; then
  logrootpath=$(cd "$BashPath/../log"; pwd)
fi 

function Usage()
{
    echo  "Usage: fsstart [options] [args]"
    echo  "Command options:"
    echo  "  -h [ --help ]             help information"
    echo  "  -a [ --all ]              mount all sequoiafs with config in conf dir"
    echo  "  -m [ --mountpoint ] arg   mount the specified mountpoint"
    echo  "  --alias arg               mount the specified mountpoint with alias "
    echo  "  -c [ --confpath ] arg     mount the specified mountpoint with config file in specified conf path"
}

function StartOne()
{
  mountpointarg=$1
  aliasarg=$2
  confpatharg=$3
  logpath=$4
  otherargs=$5
  
  nessargs=()
  nesscount=0
  
  if [ -z "$confpatharg" ]; then
    if [ ! -d "$confrootpath" ]; then
      echo "Failed: cannot find the configuration directory: $confrootpath"
      return 1
    fi 
    if [ "$aliasarg" != "" ]; then
      confpath="$confrootpath/$aliasarg"
      if [ ! -d "$confpath" ]; then
        echo "Failed: cannot find the configuration directory: $confpath"
        return 1
      fi
      confpatharg=$confpath
    else
    # remove the last slash
      mountpointarg=$(echo ${mountpointarg%*/})
      for dir in `ls "$confrootpath"`
      do
        tmpconfpath="$confrootpath/$dir"
        if [ -f "$tmpconfpath/sequoiafs.conf" ]; then  
          source "$tmpconfpath/sequoiafs.conf"
          mountcfginfo=$(echo ${mountpoint%*/})
          if [ "$mountpointarg" == "$mountcfginfo" ]; then
            confpatharg=$tmpconfpath;
            break
          fi
        fi        
      done  
      if [ -z "$confpatharg" ]; then
        echo "Failed: cannot find the configuration file of $mountpointarg"
        return 1
      fi
    fi  
  fi

  mountinfo=""
  if [ -d "$confpatharg" ]; then
    nessargs[$nesscount]="-c"
    let "nesscount++"
    nessargs[$nesscount]="$confpatharg"
    let "nesscount++"  
    if [ -f "$confpatharg/sequoiafs.conf" ]; then
      source "$confpatharg/sequoiafs.conf"   
      mountinfo=$(echo ${mountpoint%*/})
    else
      echo "Failed: cannot find the configuration file: $confpatharg/sequoiafs.conf"
      return 1    
    fi
  else
    echo "Failed: cannot find the configuration directory: $confpatharg"
    return 1
  fi

  if [ "$mountpointarg" != "" ]; then
    nessargs[$nesscount]="-m"
    let "nesscount++"
    nessargs[$nesscount]="$mountpointarg"    
    let "nesscount++"
    mountinfo=$(echo ${mountpointarg%*/})
  fi
    
  mountpid=$( mount -t $fusetype |grep $mountinfo" " | awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')    
  if [ -n "$mountpid" ]; then   
    cmdconfpathinfo=$( ps -ef| grep  $mountpid | grep " -c " |grep -v grep | awk -F" -c " '{print $2}' | awk -F" " '{print $1}')
    if [ -z $cmdconfpathinfo ]; then
      cmdconfpathinfo=$( ps -ef| grep  $mountpid | grep " --confpath " |grep -v grep | awk -F" --confpath " '{print $2}' | awk -F" " '{print $1}')
    fi
    if [ -n $cmdconfpathinfo ]; then
      cmdconfpathinfo=$(cd "$cmdconfpathinfo"; pwd)    
      if [ "$confpatharg" == "$cmdconfpathinfo" ]; then
        echo "Succeed: sequoiafs($mountinfo) is already started ($mountpid)."
        return 0 
      fi        
    fi
    echo "Failed: sequoiafs($mountinfo) has been mounted by other process($mountpid)."
    return 1	
  fi  
  
  if [ "$aliasarg" != "" ]; then
    nessargs[$nesscount]="--alias"
    let "nesscount++"
    nessargs[$nesscount]=$aliasarg
    let "nesscount++"
  fi

  if [ "$logpath" != "" ]; then
    nessargs[$nesscount]="--diagpath"
    let "nesscount++"
    nessargs[$nesscount]=$logpath
    let "nesscount++"
  else
    diagpath=""
    if [ -f "$confpatharg/sequoiafs.conf" ]; then
      source "$confpatharg/sequoiafs.conf"
    fi
    if [ -z $diagpath ]; then
      lastdir=`echo $confpatharg | sed "s/\// /g" | awk 'NR==1{print $NF}' `      
      logpath="$logrootpath/$lastdir"    
      nessargs[$nesscount]="--diagpath"
      let "nesscount++"
      nessargs[$nesscount]=$logpath
      let "nesscount++"
    else 
      logpath=$diagpath      
    fi
  fi

  echo "Start $fsbin ${nessargs[*]} ${otherargs[*]} "
  $fsbin ${nessargs[*]} ${otherargs[*]} & 
  curfspid="$!"

  sleep 1

  loop=0
  while(( $loop < 100 ))
  do
    let "loop++"
    mountpidinfo=$( mount -t $fusetype|grep $mountinfo" " |  awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')
    if [ "$mountpidinfo" == "$curfspid" ]; then
      break
    else
      if [ -n "$( ps -ef |grep $curfspid |grep -v grep)" ]; then
        sleep 1
        continue
      else
        break
      fi              
    fi
  done

  if [ -f "$logpath/sequoiafs.pid" ]; then
    fslogpid=$(cat "$logpath/sequoiafs.pid")
    if [ "$fslogpid" == "$curfspid" ]; then
      echo "Succeed: $curfspid"
      return 0
    else
      echo "Failed"
      return 1  
    fi
  else
    echo "Failed"
    return 1
  fi
}

function Start()
{
  startall=$1
  mountpointarg=$2
  aliasarg=$3
  confpatharg=$4
  logpath=$5
  otherargs=$6
  
  successcount=0
  failcount=0
  
  if [ -z "$startall" ] && [ -z "$mountpointarg" ] && [ -z "$aliasarg" ] && [ -z "$confpatharg" ]; then
    echo -e "fsstart.sh requires at least one parameter."
    Usage
    exit 127
  fi    
  
  if [ -n "$startall" ] ; then
    if [ -d "$confrootpath" ]; then
      for dir in `ls "$confrootpath"`
      do
        confpatharg="$confrootpath/$dir"
    
        StartOne "" "" "$confpatharg" "" $6
        result=`echo $?` 
    
        if [ $result == 0 ]; then
          let "successcount++"
        else
          let "failcount++"
        fi   
      done
    fi   
  else
    StartOne "$2" "$3" "$4" "$5" $6
    result=`echo $?` 
    
    if [ $result == 0 ]; then
      let "successcount++"
    else
      let "failcount++"
    fi
  fi
  
  total=$(($successcount+$failcount))
  echo "Total: $total; Succeed: $successcount; Failed: $failcount"
  if [[ $failcount > 0 && $successcount > 0 ]]; then
    exit 2
  fi
  if [[ $failcount > 0 && $successcount == 0 ]]; then
    exit 4
  fi

  return 0
}

all=""
mountpointin=""
alias=""
logpath=""
configpath=""
otherargs=""
count=0

while [ -n "$1" ]
do
    case $1 in
    -h|--help)
      Usage
      exit 0
      ;;
    -a|--all)
      all="true"
      ;;
    -m|--mountpoint)
      mountpointin=$2
      shift
      ;;
    --alias)
      alias=$2
      shift
      ;;
    -c|--confpath)
      configpath=$2
      shift
      ;;
    --diagpath)
      logpath=$2
      shift
      ;;      
    *)
      otherargs[$count]=$1
      let count++
      otherargs[$count]=$2
      let count++
      shift
      ;;
    esac
shift
done

Start "$all" "$mountpointin" "$alias" "$configpath" "$logpath" ${otherargs[*]}

exit 0
