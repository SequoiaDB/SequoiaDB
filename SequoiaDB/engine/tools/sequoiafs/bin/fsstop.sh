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

unmount="fusermount -u"
fusetype="fuse.sequoiafs"

function Usage()
{
    echo  "Usage: fsstop.sh [options] <args>"
    echo  "   -h [ --help ]            help information"
    echo  "   -a [ --all ]             unmount all mountpoint"
    echo  "   -m [ --mountpoint ] arg  unmount the specified mountpoint"
    echo  "   --alias arg              unmount the specified mountpoint by alias name"
}

function Stop()
{
  stopall=$1
  mountpoint=$2
  alias=$3
  
  successcount=0
  failcount=0
 
  if [ -z "$stopall" ] && [ -z "$mountpoint" ] && [ -z "$alias" ]; then
    echo -e "fsstop.sh requires at least one parameter."
    Usage
    exit 127
  fi    
  
  if [ -n "$stopall" ] ; then
    mountlist=$(mount -t $fusetype| awk '{print $3}')
    for mountdir in $mountlist
    do
      if [ -n "$mountdir"  ]; then
        aliasinfo=$( mount -t $fusetype|grep $mountdir" " |  awk '{print $1}' | awk -F"(" '{print $1}')
        pidinfo=$( mount -t $fusetype|grep $mountdir" " |  awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')    
        echo "Terminating process $pidinfo: $mountdir($aliasinfo)"
        $unmount "$mountdir"
        if [ $? -eq 0 ]; then
          echo "DONE"
          let "successcount++"
        else
          echo "FAILED"
          let "failcount++"    
        fi
      fi
    done
  else
    if [ -n "$mountpoint" ]; then
      # remove the last slash
      mountpoint=$(echo ${mountpoint%*/})
      aliasinfo=$( mount -t $fusetype|grep $mountpoint" " |  awk '{print $1}' | awk -F"(" '{print $1}')
      pidinfo=$( mount -t $fusetype|grep $mountpoint" " |  awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')    
      if [ -n "$pidinfo" ]; then
        echo "Terminating process $pidinfo: $mountpoint($aliasinfo)"
        $unmount "$mountpoint"
        if [ $? -eq 0 ]; then
          echo "DONE"
          let "successcount++"
        else
          echo "FAILED"
          let "failcount++"    
        fi
      fi    
    else
      mountpoint=$(mount -t $fusetype | grep $alias"(" | awk '{print $3}')
      pidinfo=$( mount -t $fusetype | grep $alias"(" |  awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')     
      if [ -n "$mountpoint"  ]; then
        echo "Terminating process $pidinfo: $mountpoint($alias)"
        $unmount "$mountpoint"
        if [ $? -eq 0 ]; then
          echo "DONE"
          let "successcount++"
        else
          echo "FAILED"
          let "failcount++"    
        fi
      fi
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

  exit 0
}

all=""
mountpoint=""
alias=""

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
      mountpoint=$2
      shift
      ;;
    --alias)
      alias=$2
      shift
      ;;  
    *)
      Usage
      exit 127
    esac
shift
done


Stop "$all" "$mountpoint" "$alias"

exit 0
