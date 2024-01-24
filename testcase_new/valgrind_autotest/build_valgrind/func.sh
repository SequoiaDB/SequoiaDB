#!/bin/bash

host_address=(${host_address//,/ })

# Check host_address must be configured
function check_conf_host()
{
    test -z "${host_address[*]}" && echo "$0 ${FUNCNAME}(): host_address must be configured" && exit 6
    regex="^192\.168\.[0-9]{1,3}\.[0-9]{1,3}$"
    for host in ${host_address[*]}
    do
        if [[ ${host} =~ $regex ]];then
            tmp_host=(${host//\./})
            [[ ${tmp_host[2]} -le 255 && ${tmp_host[3]} -le 255 ]]
            ret=$?
            test ${ret} -ne 0 && echo "$0 ${FUNCNAME}(): Host[${host}] configuration error" && exit ${ret}
        fi
    done
}

# Configure ssh privacy free login
function ssh_copy_id()
{
    if [ ! -f ~/.ssh/id_rsa ];then
        ssh-keygen -t rsa -f ~/.ssh/id_rsa -P ""
    fi
    for host in ${host_address[*]}
    do
        get_new_output "${host} request password: "
        ssh-copy-id -i ~/.ssh/id_rsa ${USER}@${host} >/dev/null 2>&1
    done
}
 
# Get run package, if configure install_latest_version=true, it will return "latest_sdb_package.run", otherwise return first run package from ${curpath}/package
function get_install_package_name()
{
    if [ "${install_latest_version}" = "true" ];then
        package_name="latest_sdb_package.run"
    else
        package_name=$(cd ${curpath}/package; ls | grep sequoiadb.*installer.* | sed '2,$d')
        test -n "${package_name}" && package_name=$(echo "${package_name}" | sed 's/(/\\(/g' | sed 's/)/\\)/g' | sed 's/ /\\ /g')
    fi
    test -z "${package_name}" && echo "$0 ${FUNCNAME}(): Connot match any run package in ${curpath}/package, you can cp a run package to ${curpath}/package or configure install_latest_version=true" && exit 10
    echo "${package_name}"
}

# Check remote host whether exist run package
function check_host_package_exist()
{
    host=$1
    package_name=$2
    check_parameter 2 ${FUNCNAME} host package_name $@
    ret=$(ssh ${USER}@${host} "test -f ~/build_package/${package_name} && echo true")
    if [ "${ret}" != "true" ];then
        echo "false"
    else
        echo "${ret}"
    fi
}

# Scp the run package to the host
function scp_package_to_host()
{   
    host=$1
    package_name=$2
    check_parameter 2 ${FUNCNAME} host package_name $@ 
    eval test ! -f ${curpath}/package/${package_name} && echo "$0 ${FUNCNAME}(): ${curpath}/package/${package_name} does not exist" && exit 10
    ret=$(ssh ${USER}@${host} "test -d ~/build_package && echo true")
    if [ "${ret}" != "true" ];then
        ssh ${USER}@${host} "mkdir -p ~/build_package"
    fi    
    ret=$(ssh ${USER}@${host} "test -f ~/build_package/${package_name} && echo true")
    if [ "${ret}" != "true" ];then        
        eval scp ${curpath}/package/${package_name} ${USER}@${host}:~/build_package/
    fi
}

# Get host sdb install dir
function get_sdb_install_dir()
{
    host=$1
    check_parameter 1 ${FUNCNAME} host $@
    etc_default_sequoiadbs=($(ssh ${USER}@${host} "ls /etc/default/sequoiadb*" 2>/dev/null))
    test -z "${etc_default_sequoiadbs[*]}" && echo "$0 ${FUNCNAME}(): Match \"sequoiadb*\" in ${host}:/etc/default and return null" && exit 10
    default_sdb_path=${etc_default_sequoiadbs[0]}
    path=$(ssh ${USER}@${host} "cat ${default_sdb_path}" 2>/dev/null | grep INSTALL_DIR | sed 's/INSTALL_DIR=//g')
    test -z "${path}" && echo "$0 ${FUNCNAME}(): Cannot match any install sdb version info" && exit 4
    echo ${path}
}

# Mv sdb bin
function mv_sdb_bin()
{
    bin_name=$1
    host=${host_address[0]}
    sdb_path=$(get_sdb_install_dir ${host})
    exit_if_error "${sdb_path}"
    mkdir -p ${curpath}/bin
    add_clean_list ${curpath}/bin localhost bin
    if [ ! -f ${curpath}/bin/${bin_name} ];then
        scp ${USER}@${host}:${sdb_path}/bin/${bin_name} ${curpath}/bin/
    fi
}

# Get sdb java driver info
function get_java_driver_info()
{
    host=${host_address[0]}
    sdb_path=$(get_sdb_install_dir ${host})
    exit_if_error "${sdb_path}"
    version_info=$(ssh ${USER}@${host} "${sdb_path}/bin/sdb -v")
    git_version=$(echo "${version_info}" | grep 'Git version' | sed 's/Git version: //g')
    driver_version=${git_version:0:10}
    if [ -z "${driver_version}" ];then
        driver_version=$(echo "${version_info}" | grep 'Release' | sed 's/Release: //g')
    fi
    driver_info=($(ssh ${USER}@${host} "ls ${sdb_path}/java | grep ${driver_version}"))
    if [ -z "${driver_info}" ];then
        driver_version=$(echo "${version_info}" | grep 'SequoiaDB shell version' | sed 's/SequoiaDB shell version: //g')
        driver_info=($(ssh ${USER}@${host} "ls ${sdb_path}/java | grep ${driver_version}"))
    fi
    test -z ${driver_info} && echo "$0 ${FUNCNAME}(): Cannot find java driver of ${driver_version}" && exit 10
    echo "${driver_info}"
}

# Scp sdb java driver
#  scp_java_driver ${target_dir}: scp remote host sdb java driver to the appoint dir
function scp_java_driver()
{
    check_parameter 1 ${FUNCNAME} target_dir $@
    target_dir=$1
    if [ ! -d ${target_dir} ];then
        mkdir -p ${target_dir}
        target_dir=$(cd ${target_dir}; pwd)
        add_clean_list ${target_dir} localhost `basename ${target_dir}`
    fi
    host=${host_address[0]}
    sdb_path=$(get_sdb_install_dir ${host})
    exit_if_error "${sdb_path}"
    driver_info=$(get_java_driver_info)
    exit_if_error "${driver_info}"
    source_driver_path=${sdb_path}/java/${driver_info}
    get_new_output "${host} scp ${source_driver_path} to ${target_dir}"
    test ! -f ${target_dir}/${driver_info}  && scp ${USER}@${host}:${source_driver_path} ${target_dir}
}

# Add files to be deleted to the clean_list
#  1. add_clean_list ${file}: clean localhost files
#  2. add_clean_list ${file} ${host}: clean remote host files
#  3. add_clean_list ${file} ${host} ${dirname}: clean remote host dir
function add_clean_list()
{
    need_clean=$1
    add_clean_host=$2
    dirname=$3
    test ! -f ${curpath}/clean_list && touch ${curpath}/clean_list
    test -n "${add_clean_host}" && add_clean_host=${add_clean_host}":"
    test -n "${dirname}" && dirname=":"${dirname}
    clean_item=${add_clean_host}${need_clean}${dirname}
    grep -in "${clean_item}" ${curpath}/clean_list >/dev/null 2>&1
    test $? -ne 0 && echo ${add_clean_host}${need_clean}${dirname} >> ${curpath}/clean_list
}

# Clean created files and caches
function clean()
{
    clean_list=($(test -f ${curpath}/clean_list && cat ${curpath}/clean_list))
    test -z "${clean_list}" && return
    for need_clean in ${clean_list[*]}
    do
        ret=(${need_clean//:/ })
        case ${#ret[@]} in
            1 )    test -f ${ret[0]} && rm ${ret[0]}
                   echo "localhost: rm ${ret[0]}"
                   ;;
            2 )    host=${ret[0]}
                   ssh ${USER}@${host} "test -f ${ret[1]} && rm ${ret[1]}"
                   echo "${host}: rm ${ret[1]}"
                   ;;
            3 )    host=${ret[0]}
                   dirname=${ret[2]}
                   act_dirname=$(basename ${ret[1]})
                   echo "${host}: rm -r ${ret[1]}"
                   if [ "${dirname}" = "${act_dirname}" ];then
                       if [ "localhost" = "${host}" ];then
                           test -d ${ret[1]} && rm -r ${ret[1]}
                       else
                           ssh ${USER}@${host} "test -d ${ret[1]} && rm -r ${ret[1]}"
                       fi
                   fi
        esac
    done
    echo "ok"
}

# Exit if last command throw exit error
function exit_if_error()
{
    error_code=$?
    error_output=$1
    check_parameter 1 ${FUNCNAME} error_output $@
    test ${error_code} -ne 0 && echo "${error_output}" && exit ${ret}
}

# Usage: check_parameter 3 ${FUNCNAME} param1 param2 param3 $@
function check_parameter()
{
    param_num=$1
    funcname=$2
    shift 2
    param_names=()
    for (( i=0; i<${param_num}; i++ ))
    do
        param_names[${i}]=$1
        shift
    done
    count=1
    for param_name in ${param_names[@]}
    do
        param_value=$1
        shift
        serial_num=${count}
        case ${count} in
            1 )    serial_num=${serial_num}"st"
                   ;;
            2 )    serial_num=${serial_num}"nd"
                   ;;
            3 )    serial_num=${serial_num}"rd"
                   ;;
            * )    serial_num=${serial_num}"th"
                   ;;
        esac
        count=$[${count}+1]
        test -z ${param_value} && echo "$0 ${funcname}(): Requires ${serial_num} parameter \"${param_name}\" but missing" && exit 6
    done
}

function get_svcnames_by_role()
{   
    host=$1
    role=$2
    check_parameter 2 ${FUNCNAME} host role $@
    sdb_path=$(get_sdb_install_dir ${host})
    exit_if_error "${sdb_path}"
    svcnames=($(ssh ${USER}@${host} "ls ${sdb_path}/database/${role}"))
    echo "${svcnames[*]}"
}
