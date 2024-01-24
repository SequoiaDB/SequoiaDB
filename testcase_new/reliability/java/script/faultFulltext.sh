#!/bin/bash

param_nums=$#
scriptname=$0

function check_param_nums()
{
    if [ ${param_nums} -lt $2 ];then
        echo "$1 requires at least $2 parameters" >&2
        exit 1
    fi
}

check_param_nums ${scriptname} 2

progname=$1
killrestart=$2

if [ $# -eq 3 ];then
    svcname=$3
fi

function check_pid()
{
    if [[ -z $1 ]];then
        echo "${scriptname} can not find progname: ${progname}" >&2
        exit 1
    fi
}

case $progname in
    sdbseadapter)    check_param_nums ${progname} 3
                     pid=$(ps -ef | grep ${progname} | grep -v grep | grep ${svcname} | grep -v ${scriptname} | awk '{print $2}')
                     check_pid $pid
                     cmddir=$(ls -l /proc/${pid}/exe | awk '{print $11}')
                     ;;

    elasticsearch)   check_param_nums ${progname} 3
                     pid=$(lsof -nP -iTCP:${svcname} -sTCP:LISTEN | sed '1d' | awk '{print $2}')
                     check_pid $pid
                     cmddir=$(ls -l /proc/${pid}/cwd | awk '{print $11}')
                     ;;

    *)               echo "$0 can not match progname[sdbseadapter, elasticsearch]: $1" >&2
                     exit 1
                     ;;
esac

kill $killrestart $pid
echo $pid:$cmddir