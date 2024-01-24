#!/bin/bash

paramscount=$#
scriptname=$0
faultname=$1

check_params()
{
    pcount=$[${paramscount}-1]
    if [ ${pcount} -ne $1 ];then
        echo "${faultname} requires $1 parameters" >&2
        exit 1
    fi
}

case ${faultname} in
    MemoryLimit)                 check_params 1
                                 svcname=$2
                                 pid=$(lsof -nP -iTCP:${svcname} -sTCP:LISTEN | sed '1d' | awk '{print $2}')
                                 output=$(fiu-ctrl -c 'disable name=posix/mm/mmap' ${pid})
                                 echo ${pid}:${output}
                                 ;;
    DiskLimit)                   check_params 1
                                 svcname=$2
                                 pid=$(lsof -nP -iTCP:${svcname} -sTCP:LISTEN | sed '1d' | awk '{print $2}')
                                 output=$(fiu-ctrl -c 'disable name=posix/io/rw/write' ${pid})
                                 echo ${pid}:${output}
                                 ;;
    *)                           echo "${scriptname} can not match any fault injection with ${faultname}" >&2
                                 exit 2
                                 ;;
esac