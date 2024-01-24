#!/bin/bash

curpath=$(cd `dirname $0`; pwd)
source ${curpath}/func.sh

function help()
{
    echo ""
    echo "Configure the build.conf configuration file, and then use the following options"
    echo ""
    echo "Command options:"
    echo "  -h|--help            return the help information"
    echo "  -i|--install         install and deploy sequoiadb environment"
    echo "  -v|--valgrind        start valgrind node processes"
    echo "  -r|--run [param]     run test by configured with [js|java|all], and get the valgrind result to ${curpath}/result"
}

test -z "$*" && help && exit 0
args=`getopt -o h,i,v,r: -l help,install,valgrind,run: -n $0 -- "$@"`
test $? -ne 0 && exit $?
eval set -- "${args}"

install_deploy=0
start_valgrind=0
run_and_get_result=0
run_type=null

while true
do
    case "$1" in
        -h | --help )     help
                          exit 0
                          ;;
        -i | --install )  install_deploy=1
                          shift
                          ;;
        -v | --valgrind ) start_valgrind=1
                          shift
                          ;;
        -r | --run )      run_and_get_result=1
                          run_type=$2
                          shift 2
                          ;;
        -- )              shift
                          break
                          ;;
        * )               echo "Unknown argument: $1"
                          exit 4
                          ;;
    esac
done

${curpath}/sequoiadb_ctl.sh --ssh-without-pwd

test ${install_deploy} -eq 1 && ${curpath}/sequoiadb_ctl.sh --install --deploy
test ${start_valgrind} -eq 1 && ${curpath}/sequoiadb_ctl.sh --valgrind
test ${run_and_get_result} -eq 1 && ${curpath}/execute_case.sh --runtest ${run_type} --loop 1 --stop_if_fail true && test ${start_valgrind} -eq 1 && ${curpath}/sequoiadb_ctl.sh --stop && ${curpath}/sequoiadb_ctl.sh --result ${curpath}/result
