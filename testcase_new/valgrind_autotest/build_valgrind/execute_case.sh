#!/bin/bash

curpath=$(cd `dirname $0`; pwd)
source ${curpath}/build.conf
source ${curpath}/utils.sh
source ${curpath}/func.sh

eshost=${eshost}
esport=${esport}
coord_svcname=${conn_svcname}

# run jstest config viriables
runtest_path=$(cd `dirname ${runtest_path}`; pwd)
jstest_path=${jstest_path}
jstest_file=${jstest_file}
jstest_type=${jstest_type}
jstest_stop_flag=${jstest_stop_flag}
jstest_s1=${jstest_s1}
jstest_s2=${jstest_s2}
jstest_addpid=${jstest_addpid}
jstest_print=${jstest_print}

# run javatest config viriables
xmlpaths=(${xmlpaths//,/ })
threadexecutor_dir=${threadexecutor_dir}
fulltextprefix=${fulltextprefix}
report_dir=${report_dir}

runtest_type=null
loop_times=1
stop_if_fail=true

function runtest()
{
    runtest_type=$1
    case ${runtest_type} in
        java )         run_javatest
                       ;;
        js )           run_jstest
                       ;;
        all )          run_jstest
                       run_javatest
                       ;;
        * )            echo "Unknown run type with \"${runtest_type}\", select [java, js, all]"
                       exit 6
    esac
}

function loop_execution()
{
    local loop_times=$1
    local stop_if_fail=$2
    for (( loop_time=0; loop_time<${loop_times}; loop_time++ ))
    do
        start_time=$(date +%s)
        runtest ${runtest_type}
        end_time=$(date +%s)
        use_time=$((end_time-start_time))
        get_new_output "$0 ${FUNCNAME}(): Dotimes [$[${loop_time}+1]], use time [${use_time}s]"
        if [ "${stop_if_fail}" = "true" ];then
            if [[ "${runtest_type}" = "java" || "${runtest_type}" = "all" ]];then
                grep -rn "FAIL" ${report_dir}/testng-results.xml >/dev/null
                if [ $? -eq 0 ]; then
                    break
                fi
            fi
            if [[ "${runtest_type}" = "js" || "${runtest_type}" = "all" ]];then
                grep -rn "Failed" ${runtest_path}/local_test_report/result.txt >/dev/null
                if [ $? -eq 0 ]; then
                    break
                fi
            fi
        fi
    done
}

function run_jstest()
{
    local host=${host_address[0]}
    runtest_param=""
    test -n "${jstest_path}" && runtest_param=${runtest_param}" -p "${jstest_path}
    test -n "${jstest_file}" && runtest_param=${runtest_param}" -f "${jstest_file}
    test -n "${jstest_type}" && runtest_param=${runtest_param}" -t "${jstest_type}
    test -n "${jstest_stop_flag}" && runtest_param=${runtest_param}" -s "${jstest_stop_flag}
    if [ -z "${coord_svcname}" ];then
        coord_svcname=($(get_svcnames_by_role ${host} coord))
        exit_if_error "${coord_svcname[*]}"
        coord_svcname=${coord_svcname[0]}
    fi
    test -n "${coord_svcname}" && runtest_param=${runtest_param}" -n "${coord_svcname}
    test -n "${host}" && runtest_param=${runtest_param}" -h "${host}
    test -n "${eshost}" && runtest_param=${runtest_param}" -eh "${eshost}
    test -n "${esport}" && runtest_param=${runtest_param}" -en "${esport}
    test -n "${jstest_s1}" && runtest_param=${runtest_param}" -s1 "${jstest_s1}
    test -n "${jstest_s2}" && runtest_param=${runtest_param}" -s2 "${jstest_s2}
    test -n "${jstest_sp}" && runtest_param=${runtest_param}" -sp "${jstest_sp}
    if [ "${jstest_addpid}" = "true" ];then
        runtest_param=${runtest_param}" -addpid "
    fi
    if [ "${jstest_print}" = "true" ];then
        runtest_param=${runtest_param}" -print "
    fi

    get_new_output "${runtest_path}/runtest.sh${runtest_param}"
    cd ${runtest_path}
    ./runtest.sh${runtest_param}
    add_clean_list ${runtest_path}/local_test_report localhost local_test_report
}

function run_javatest()
{
    for xmlpath in ${xmlpaths[*]}
    do
        xmlname=$(basename ${xmlpath})
        xmlpath=$(cd `dirname ${xmlpath}`; pwd)
        cd ${xmlpath}
        runtest_param=""
        runtest_param=${runtest_param}" -DxmlFileName="${xmlname}
        driver_info=$(get_java_driver_info)
        exit_if_error "${driver_info}"
        sdbdriver=$(echo ${driver_info} | sed 's/sequoiadb-driver-//g' | sed 's/.jar//g')
        runtest_param=${runtest_param}" -Dsdbdriver="${sdbdriver}
        host=${host_address[0]}
        sdb_path=$(get_sdb_install_dir ${host})
        exit_if_error "${sdb_path}"
        scp_java_driver ${curpath}/java
        sdbdriverDir=${curpath}/java
        runtest_param=${runtest_param}" -DsdbdriverDir="${sdbdriverDir}
        test -z ${report_dir} && mkdir -p ${curpath}/java/output/`basename ${xmlname} .xml` && report_dir=${curpath}/java/output/`basename ${xmlname} .xml`
        add_clean_list ${curpath}/java/output localhost output
        runtest_param=${runtest_param}" -DreportDir="${report_dir}
        runtest_param=${runtest_param}" -DHOSTNAME="${host}
        
        test -z "${coord_svcname}" && coord_svcname=($(get_svcnames_by_role ${host} coord))
        runtest_param=${runtest_param}" -DSVCNAME="${coord_svcname[0]}
        runtest_param=${runtest_param}" -DESHOSTNAME="${eshost}
        runtest_param=${runtest_param}" -DESSVCNAME="${esport}
        if [ -n "${threadexecutor_dir}" ];then
            runtest_param=${runtest_param}" -DthreadexecutorDir="${threadexecutor_dir}
        fi
        runtest_param=${runtest_param}" -DSCRIPTDIR="${xmlpath}/script
        runtest_param=${runtest_param}" -DFULLTEXTPREFIX="${fulltextprefix}
        get_new_output "mvn surefire-report:report${runtest_param}"
        mvn surefire-report:report${runtest_param}
    done
}

function help()
{
    echo ""
    echo "Configure the build.conf configuration file, and then use the following options"
    echo ""
    echo "Command options:"
    echo "  -h|--help                 return the help information"
    echo "  -r|--runtest [param]      runtest which you config, and you need appoint a type [js, java, all] to run the testcases"
    echo "  -l|--loop [param]         execute loop times"
    echo "  -s|--stop_if_fail [param] stop the script if fail [true|false], true will stop, and default true"  
}

test -z "$*" && help && exit 0
args=`getopt -o h,r:,l:,s: -l help,runtest:,loop:,stop_if_fail: -n $0 -- "$@"`
test $? -ne 0 && exit $?
eval set -- "${args}"

while true
do
    case "$1" in
        -h | --help )         help
                              exit 0
                              ;;
        -r | --runtest )      runtest_type=$2
                              shift 2
                              ;;
        -l | --loop )         loop_times=$2
                              shift 2
                              ;;
        -s | --stop_if_fail ) stop_if_fail=$2
                              shift 2
                              ;;
        -- )                  shift
                              continue
                              ;;
        "" )                  shift
                              break
                              ;;
        * )                   echo "Unknown argument: $1"
                              exit 6
                              ;;
    esac
done

loop_execution ${loop_times} ${stop_if_fail}
