#!/bin/bash
if [ $# -lt 1 ];then
    echo "$0 requires at least 1 parameters" >&2
    exit
fi

progname=$1

case $progname in
    sdbseadapter)    if [ $# -ne 4 ];then
                         echo "$progname requires at least 3 parameters" >&2
                         exit 1
                     else
                         cmddir=$2
                         seadaptDir=$3
                         svcName=$4
                         nohup $cmddir -c $seadaptDir/$svcName  2>&1 &
                     fi
                     ;;
    elasticsearch)   if [ $# -ne 2 ];then
                         echo "$progname requires at least 1 parameters" >&2
                         exit 1
                     else
                         cmddir=$2
                         $cmddir -d
                     fi
                     ;;
    *)               echo "$0 can not match progname[sdbseadapter, elasticsearch]: $1" >&2
                     exit 1
                     ;;
esac