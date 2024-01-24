#!/bin/bash

curpath=$(cd `dirname $0`; pwd)
source ${curpath}/build.conf
source ${curpath}/utils.sh
source ${curpath}/func.sh

eshost=${eshost}
esport=${esport}
adapterpath=${adapterpath}

function stop()
{
    for host in ${host_address[*]}
    do
        get_new_output "${host} stopping the sdbseadapter process"
        adapt_pids=($(ssh ${USER}@${host} "ps -ef | grep sdbseadapter" | grep -v "grep" | grep -v "sdbseadapter_ctl" | awk '{print $2}'))
        for pid in ${adapt_pids[*]}
        do
            echo "${host} stop sdbseadapter process: ${pid}"
            ssh ${USER}@${host} "kill -15 ${pid}"
        done
    done
}

function start()
{
    for host in ${host_address[*]}
    do
        get_new_output "${host} starting the sdbseadapter process"
        check_host_adapterpath ${host}
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        svcnames=($(ssh ${USER}@${host} "ls ${adapterpath}"))
        for svcname in ${svcnames[*]}
        do
            echo "Starting the sdbseadapter process ${svcname}"
            ssh ${USER}@${host} "nohup ${sdb_path}/bin/sdbseadapter -c ${adapterpath}/${svcname} >/dev/null 2>&1 &"
        done
    done
}

function restart()
{
    stop
    start
}

function valgrind()
{
    stop
    for host in ${host_address[*]}
    do
        get_new_output "${host} starting the valgrind sdbseadapter process"
        check_host_adapterpath ${host}
        adapter_dirname=$(basename ${adapterpath})
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        svcnames=($(ssh ${USER}@${host} "ls ${adapterpath}"))
        for svcname in ${svcnames[*]}
        do
            echo "${host} start valgrind sdbseadapter process: ${svcname}"
            ssh ${USER}@${host} "valgrind --error-limit=no --tool=memcheck --leak-check=full --log-file=${sdb_path}/conf/log/sdbseadapterlog/${svcname}/valgrind.txt ${sdb_path}/bin/sdbseadapter -c ${sdb_path}/conf/${adapter_dirname}/${svcname}/ >/dev/null 2>&1 & disown"
        done
    done
}

function install()
{
    for host in ${host_address[*]}
    do
        get_new_output "${host} installing the sdbseadapter"
        check_parameter 2 ${FUNCNAME} eshost esport ${eshost} ${esport}
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        data_svcnames=($(get_svcnames_by_role ${host} data))
        exit_if_error "${data_svcnames[*]}"
        for svcname in ${data_svcnames[*]}
        do
            echo "${host} install the sdbseadapter node ${svcname}"
            ssh ${USER}@${host} "mkdir -p ${sdb_path}/conf/sdbseadapter/${svcname}"
            ssh ${USER}@${host} "cp ${sdb_path}/conf/samples/sdbseadapter.conf ${sdb_path}/conf/sdbseadapter/${svcname}"
            ssh ${USER}@${host} "sed -i -e '/searchenginehost=/c searchenginehost=${eshost}' ${sdb_path}/conf/sdbseadapter/${svcname}/sdbseadapter.conf"
            ssh ${USER}@${host} "sed -i -e '/searchengineport=/c searchengineport=${esport}' ${sdb_path}/conf/sdbseadapter/${svcname}/sdbseadapter.conf"
            ssh ${USER}@${host} "sed -i -e '/datanodehost=/c datanodehost=${host}' ${sdb_path}/conf/sdbseadapter/${svcname}/sdbseadapter.conf"
            ssh ${USER}@${host} "sed -i -e '/datasvcname=/c datasvcname=${svcname}' ${sdb_path}/conf/sdbseadapter/${svcname}/sdbseadapter.conf"
        done
    done
    adapterpath=${sdb_path}/conf/sdbseadapter
    start
}

function get_valgrind_result()
{
    resultdir=$1
    if [ ! -d ${resultdir} ];then
        mkdir -p ${resultdir}
    fi
    resultdir=$(cd ${resultdir}; pwd)
    for host in ${host_address[*]}
    do
        get_new_output "copy ${host} valgrind result to ${resultdir}"
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        svcnames=($(ssh ${USER}@${host} "ls ${sdb_path}/conf/log/sdbseadapterlog/"))
        exit_if_error "${svcnames[*]}"
        for svcname in ${svcnames[*]}
        do
            ret=$(ssh ${USER}@${host} "test -f ${sdb_path}/conf/log/sdbseadapterlog/${svcname}/valgrind.txt && echo ok")
            if [ "$ret"M = "ok"M ];then
                count=0
                while true
                do
                    ret=$(ssh ${USER}@${host} "grep -in 'LEAK SUMMARY' ${sdb_path}/conf/log/sdbseadapterlog/${svcname}/valgrind.txt")
                    if [ -n "${ret}" ];then
                        break
                    fi
                    count=$[${count}+1]
                    test ${count} -eq 60 && break
                    sleep 1
                done
                echo "copy ${host} sdbseadapter ${svcname} to ${resultdir}"
                scp ${USER}@${host}:${sdb_path}/conf/log/sdbseadapterlog/${svcname}/valgrind.txt ${resultdir}/${host}_sdbseadapter_${svcname}_valgrind.txt >/dev/null
            fi
        done
    done
}

function get_svcnames()
{
    host=$1
    check_host_adapterpath ${host}
    svcnames=($(ssh ${USER}@${host} "ls ${adapterpath}"))
    echo "${svcnames[*]}"
}

function check_host_adapterpath()
{
    host=$1
    ret=$(ssh ${USER}@${host} "test -d '${adapterpath}' && echo ok")
    if [ "${ret}"M != "ok"M ];then
        echo "${host} ${adapterpath} is not a directory"
        exit 19
    fi
}

function help()
{
    echo ""
    echo "Configure the build.conf configuration file, and then use the following options, at least, you need configure [host_address, eshost, esport, adapterpath]"
    echo ""
    echo "Command options:"
    echo "  -h|--help          return the help information"
    echo "  --install          install the sdbseadapter and start the process"
    echo "  --start            start the sdbseadapter process, if don't install sdb, you need --install first"
    echo "  --stop             stop the sdbseadapter process by kill -15"
    echo "  --restart          restart the sdbseadapter process"
    echo "  --valgrind         start the valgrind sdbseadapter process"
    echo "  --result [param]   get the valgrind result, you must stop sdbseadapter process first and appoint a directory to save the result"
}

test -z "$*" && help
args=`getopt -o h -l help,stop,start,restart,valgrind,install,result: -n $0 -- "$@"`
test $? -ne 0 && exit $?
eval set -- "${args}"

while true
do
    case "$1" in
        -h | --help )   help
                        exit 0
                        ;;
        --stop )        stop
                        shift
                        ;;
        --start )       start
                        shift
                        ;;
        --restart )     restart
                        shift
                        ;;
        --valgrind )    valgrind
                        shift
                        ;;
        --install )     install
                        shift
                        ;;
        --result )      resultdir=$2
                        get_valgrind_result ${resultdir}
                        shift 2
                        ;;
        get_svcnames )  host=$2
                        shift_num=2
                        test -z "${host}" && host=${host_address[0]} && shift_num=1
                        get_svcnames ${host}
                        shift ${shift_num}
                        ;;
        -- )            shift
                        continue
                        ;;
        "" )            shift
                        break
                        ;;
        * )             echo "Unknown argument: $1"
                        exit 6 
                        ;;
    esac
done


