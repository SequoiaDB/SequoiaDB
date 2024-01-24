#!/bin/bash

curpath=$(cd `dirname $0`; pwd)
source ${curpath}/build.conf
source ${curpath}/utils.sh
source ${curpath}/func.sh

needcover=${needcover}
needinstall=${needinstall}
package_name=""
install_latest_version=${install_latest_version}
latest_version_info_url="http://192.168.28.27:8080/job/compile_db_trunk/lastSuccessfulBuild/artifact/release/sequoiadb.version"

test -z ${needinstall} && needinstall=false
test -z ${needcover} && needcover=false
test -z ${install_latest_version} && install_latest_version=false

function install()
{
    if [ "${needinstall}" = "false" ];then
        echo "$0 install(): The intstallation task has been cancelled, please configure needinstall=true"
        return
    fi
    package_name=$(get_install_package_name)
    exit_if_error "${package_name}"
    if [ ! -f ${curpath}/package/${package_name} ];then
        if [ "${install_latest_version}" = "true" ];then
            get_new_output "Downloading latest sdb version to ${curpath}/package"
            if [ ! -f ${curpath}/sequoiadb.version ];then
                wget -P ${curpath} ${latest_version_info_url}
                add_clean_list "${curpath}/sequoiadb.version"
            fi
            latest_version=$(cat ${curpath}/sequoiadb.version | grep "SequoiaDB version" | sed 's/SequoiaDB version: //g')
            latest_version_url="http://192.168.28.27:8080/job/compile_db_trunk/lastSuccessfulBuild/artifact/release/sequoiadb-${latest_version}-linux_x86_64-enterprise-installer.run"
            wget -O ${curpath}/package/${package_name} ${latest_version_url}
            add_clean_list "${curpath}/package/${package_name}"
        else
            echo "$0 install(): No run package can be found under ${curpath}/package, you can cp the run package to the directory or configure install_latest_version=true"
            exit 10
        fi
    fi
    chmod u+x ${curpath}/package/${package_name}
    get_new_output "scp ${curpath}/package/${package_name} to [${host_address[*]}]"
    for host in ${host_address[*]}
    do
        ret=$(check_host_package_exist ${host} ${package_name})
        exit_if_error "${ret}"
        if [ "${ret}" = "false" ];then
            scp_package_to_host ${host} ${package_name} &
            add_clean_list ~/build_package ${host} build_package
        fi
    done
    wait
    if [ "${needcover}" = "true" ];then
        uninstall true
    fi
    first_install_sms=${host_address[0]}
    get_new_output "Installing sequoiadb in [${host_address[*]}]"
    for host in ${host_address[*]}
    do
        install_param=""
        if [ "${host}" = "${first_install_sms}" ];then
            install_param=" --SMS true"
        fi
        echo "Installing sequoiadb in ${host}: ~/build_package/${package_name} --mode unattended${install_param}"
        ssh ${USER}@${host} "~/build_package/${package_name} --mode unattended${install_param}" >/dev/null &
    done
    wait
}

function uninstall()
{
    rm_install_dir=${1}
    case ${rm_install_dir} in
        true )    ;;
        false )   ;;
        * )       echo "$0 ${FUNCNAME}(): Requires 1st parameter [true, false]"
                  exit 6
                  ;;
    esac
    save_or_remove=save
    test "${rm_install_dir}" = "true" && save_or_remove=remove
    get_new_output "Uninstalling [${host_address[*]}] sequoiadb environment and ${save_or_remove} data files"
    for host in ${host_address[*]}
    do
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        if [ -f ${sdb_path}/uninstall ];then
            echo -e "Y\n" | ssh ${USER}@${host} "${sdb_path}/uninstall" && echo -e "\n" >/dev/null &
        fi
    done
    wait
    for host in ${host_address[*]}
    do
        if [ "${rm_install_dir}" = "true" ];then
            ssh ${USER}@${host} "rm -rf ${sdb_path}" &
        fi
    done
    wait
}

function deploy()
{
    hosts="" 
    for host in ${host_address[*]}
    do  
        hosts=${hosts}\"${host}\"\,
    done
    host=${host_address[0]}
    mv_sdb_bin sdb
    cmd=${curpath}/bin/sdb" -f "${curpath}/deploy.js" -e "\'"var hosts=[${hosts}]"\'
    get_new_output "deploy sdb command: ${cmd}"
    eval ${cmd}
    test $? -ne 0 && exit $?
    check_sdb
}

# Check sdb cluster environment is ok
function check_sdb()
{
    mv_sdb_bin sdb
    host=${host_address[0]}
    cmd=${curpath}/bin/sdb" -f "${curpath}/checksdb.js" -e \"var host='${host}'\""
    get_new_output "Check whether the remote host cluster is normal: ${host_address[*]}"
    eval ${cmd}
}

# clean the files or dirs in clean_list
function clean_env()
{
    clean
    test -f ${curpath}/clean_list && rm ${curpath}/clean_list
}

function start()
{
    for host in ${host_address[*]}
    do
        get_new_output "Starting the node of ${host}"
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        ssh ${USER}@${host} "${sdb_path}/bin/sdbstart -t db"
    done
}

function stop()
{
    for host in ${host_address[*]}
    do
        get_new_output "Stopping the node of ${host}"
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        ssh ${USER}@${host} "${sdb_path}/bin/sdbstop -t db" 
    done
}

function restart()
{
    stop
    stop_valgrind
    start
}

function valgrind()
{
    stop
    for host in ${host_address[*]}
    do
        get_new_output "Starting valgrind node process of ${host}"
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        roles=(coord catalog data)
        for role in ${roles[*]}
        do
            svcnames=($(get_svcnames_by_role ${host} ${role}))
            exit_if_error "${svcnames[*]}"
            for svcname in ${svcnames[*]}
            do
                echo "${host} start valgrind ${role} node process: ${svcname}"
                ssh ${USER}@${host} "valgrind --error-limit=no --tool=memcheck --leak-check=full --log-file=${sdb_path}/database/${role}/${svcname}/diaglog/valgrind.txt ${sdb_path}/bin/sequoiadb -c ${sdb_path}/conf/local/${svcname}/ >/dev/null 2>&1 & disown"
            done
        done
    done
    check_sdb
}

function stop_valgrind()
{
    for host in ${host_address[*]}
    do
        get_new_output "Stopping valgrind node process of ${host}"
        node_pids=($(ssh ${USER}@${host} "ps -ef | grep 'valgrind.*conf/local'" | grep -v "grep" | awk '{print $2}'))
        test -n "${node_pids[*]}" && ssh ${USER}@${host} "kill -15 ${node_pids[*]}"
     done
}

function get_valgrind_result()
{
    dir_name=$1
    if [ ! -d ${dir_name} ];then
        mkdir -p ${dir_name}
    fi
    resultdir=$(cd ${dir_name}; pwd)
    add_clean_list ${resultdir} localhost `basename ${dir_name}`
    for host in ${host_address[*]}
    do
        get_new_output "Scp the valgrind test result to ${resultdir} from ${host}"
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        roles=(coord catalog data)
        for role in ${roles[*]}
        do  
            svcnames=($(get_svcnames_by_role ${host} ${role}))
            exit_if_error "${svcnames[*]}"
            for svcname in ${svcnames[*]}
            do
                ret=$(ssh ${USER}@${host} "test -f ${sdb_path}/database/${role}/${svcname}/diaglog/valgrind.txt && echo ok")
                if [ "$ret"M = "ok"M ];then
                    count=0
                    while true
                    do
                        ret=$(ssh ${USER}@${host} "grep -in 'LEAK SUMMARY' ${sdb_path}/database/${role}/${svcname}/diaglog/valgrind.txt")
                        if [ -n "${ret}" ];then
                            break
                        fi
                        count=$[${count}+1]
                        test ${count} -eq 60 && break
                        sleep 1
                    done
                    echo "copy ${host} ${role}:${svcname} to ${resultdir}"
                    scp ${USER}@${host}:${sdb_path}/database/${role}/${svcname}/diaglog/valgrind.txt ${resultdir}/${host}_${role}_${svcname}_valgrind.txt >/dev/null
                fi 
            done
        done
    done
}

function ssh_without_pwd()
{
    ssh_copy_id
}

function help()
{
    echo ""
    echo "Configure the build.conf configuration file, and then use the following options"
    echo ""
    echo "Command options:"
    echo "  -h|--help            return the help information"
    echo "  --install            install sequoiadb"
    echo "  --uninstall [param]  uninstall sequoiadb, optional parameter [true|false], true will delete the installation directory, false will keep"
    echo "  --deploy             deploy sequoiadb cluster"
    echo "  --check              check whether the cluster environment is ok"
    echo "  --start              start cluster nodes"
    echo "  --stop               stop cluster nodes"
    echo "  --restart            restart cluster nodes"
    echo "  --valgrind           start valgrind cluster nodes process"
    echo "  --result [param]     get valgrind execute result to the specific dir"
    echo "  --ssh-without-pwd    configure ssh pravicy free login"
    echo "  --clean              clean all the shell script generated files"
}

test -z "$*" && help && exit 0

args=`getopt -o h -l help,stop,start,restart,valgrind,install,uninstall:,deploy,check,clean,result:,ssh-without-pwd -n $0 -- "$@"`
test $? -ne 0 && exit $?
eval set -- "${args}"

while true
do
    case "$1" in
        -h | --help )           help
                                shift
                                ;;
        --install )             install
                                shift
                                ;;
        --uninstall )           rm_install_dir=$2
                                uninstall ${rm_install_dir}
                                shift 2
                                ;;
        --deploy )              deploy
                                shift
                                ;;
        --check )               check_sdb
                                shift
                                ;;
        --start )               start
                                shift
                                ;;
        --stop )                stop
                                stop_valgrind
                                shift
                                ;;
        --restart )             restart
                                shift
                                ;;
        --valgrind )            valgrind
                                shift
                                ;;
        --result )              resultdir=$2
                                get_valgrind_result ${resultdir}
                                shift 2
                                ;;
        --ssh-without-pwd )     ssh_without_pwd
                                shift
                                ;;
        --clean )               clean_env
                                shift
                                ;;
        -- )                    shift
                                continue
                                ;;
        "" )                    shift
                                break
                                ;;
        * )                     echo "Unknown argument: $1"
                                exit 6
                                ;;
    esac
done
