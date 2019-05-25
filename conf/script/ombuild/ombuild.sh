#bin/bash

CUR_PATH=$(cd `dirname $0`; pwd)
DEF_INSTALL_PATH=/etc/default/sequoiadb
opt_configfile=""
opt_action=""
arr_options=( "-h" "-c" "-a" "--help" "--conf" "--action" )
#define for separate
arr_separate_result=()



#
#@description: display usage
#@argument: null
#@return: null
#
function displayUsage()
{
   echo "  --help            display the usage "
   echo "  --action <action> [--conf <config file>] "
   echo "                    specified the action for running, can be one of the follow: "
   echo "                       'buildom'|'removeom'|'addbusiness'|'updatecoord',"
   echo "                    default to be 'addbusiness'. e.g. --action addbusiness --conf ombuild.conf"
   echo "                    when action is 'removeom', no need to specifiy the config file"
}

#
#@description: verify argument
#@argument: the argument get from command line
#@return: when the argument is ok, return 0, otherwise,
#         stop running
#
function verifyArgument()
{
   local val_opt=$1
   local val_arg=$2
   local elem=""

   if [ "" = "$val_arg" ] ; then
      echo "Error: invalid argument for option '$val_opt'."
      exit
   fi

   for elem in ${arr_options[*]}
   do
      if [ "$val_arg" = "$elem" ] ; then
         echo "Error: invalid argument for option '$val_opt'."
         exit
      fi
   done

   return 0
}

#
#@description: separate by mark
#@argument: 1. input string
#           2. mark
#@return: 0 for ok
#
function separateByMark()
{
   local argument=$1
   local mark=$2
   arr_separate_result=()

   OLD_IFS=$IFS
   IFS="$mark"
   arr_separate_result=($argument)
   IFS=$OLD_IFS

   return 0
}

#
#@description: get the full path of the specified program
#@arguments: the name of the program
#@return: the full path of the program. e.g. /opt/sequoiadb/bin/sdbexprt
#         when can't find the program, stop running
#
function getProgFullPath()
{
   local ret_str=""
   local val_prog=$1
   local val_install_path=""
   local arr_tmp=()
   local val_tmp_str1=""
   local val_tmp_str2=""
   local val_flag1=0
   local val_flag2=0
   local len=0
   local i=0

   # get from install path
   if [ -f "${DEF_INSTALL_PATH}" ] ; then
      source ${DEF_INSTALL_PATH}
      if [ -f "${INSTALL_DIR}/bin/${val_prog}" ] ; then
         val_tmp_str1=${INSTALL_DIR}/bin/${val_prog}
         val_flag1=1
      fi
   fi

   # get from whereis
   arr_tmp=(`whereis "${val_prog}"`)
   for (( i = 1; i < ${#arr_tmp[@]}; i++ ))
   do
      val_tmp_str2=${arr_tmp[$i]}
      separateByMark "$val_tmp_str2" "/"
      len=${#arr_separate_result[@]}
      if [ "${val_prog}" = "${arr_separate_result[$len-1]}" ]
      then
         val_flag2=1
         break
      fi
   done

   # get program full path
   if [ -f "${CUR_PATH}/${val_prog}" ] ; then # get from current path
      ret_str="${CUR_PATH}/${val_prog}"
   elif [ 1 -eq ${val_flag1} ] ; then # get from default install path
      ret_str="${val_tmp_str1}"
   elif [ -f "${CUR_PATH}/../../bin/${val_prog}" ] ; then # get from relative path "../../bin/"
      ret_str="${CUR_PATH}/../../bin/${val_prog}"
   elif [ 1 -eq ${val_flag2} ] ; then # get from whereis ${val_prog}
      ret_str="${val_tmp_str2}"
   else # can't find, echo error and stop running
      echo "Error: can not find program ${val_prog}."
      exit
   fi

   echo ${ret_str}
}



################################################################################



# check argument
if [ $# -eq 0 ] ; then
   displayUsage
   exit
fi


# parse arguments
while [ -n "$1" ] && [ $# -gt 0 ]
do
   case "$1" in
   -h | --help)
      displayUsage
      exit ;;
   -c | --conf)
      opt_configfile=$2
      verifyArgument "--conf" "$opt_configfile"
      ;;
   -a | --action)
      opt_action=$2
      verifyArgument "--action" "$opt_action"
      ;;
   *)
      echo "Error: unkown option: $1"
      displayUsage
      exit ;;
   esac
   shift # never use "shift 2"
   shift
done


SDB=`getProgFullPath "sdb"`
PROG_PATH=`dirname ${SDB}`
SCRIPT_PATH=${PROG_PATH}/../conf/script

SDBADMIN_USER=`cat /etc/default/sequoiadb | grep "SDBADMIN_USER="`
if [ $? -ne 0 ]
then
   echo "Error: failed to access 'SDBADMIN_USER' in '/etc/default/sequoiadb' to get the sequoiadb admin user"
   exit
fi

EXPECT_USER=${SDBADMIN_USER#*=}
ACTUAL_USER=`whoami`

if [ $EXPECT_USER != $ACTUAL_USER ]
then
   echo "Error: current account is '${ACTUAL_USER}', please use '${EXPECT_USER}' account to run current shell script"
   exit
fi

js_file_define="${SCRIPT_PATH}/define.js"
js_file_common="${SCRIPT_PATH}/common.js"
js_file_log="${SCRIPT_PATH}/log.js"
js_file_func="${SCRIPT_PATH}/func.js"
js_file_item="${SCRIPT_PATH}/checkHostItem.js"
js_file_om_define="${SCRIPT_PATH}/ombuild/define.js"
js_file_build_ombuild="${SCRIPT_PATH}/ombuild/ombuild.js"

js_file="${js_file_define};${js_file_common};"
js_file="${js_file};${js_file_log};${js_file_func};${js_file_item};"
js_file="${js_file};${js_file_om_define};${js_file_build_ombuild};"


${SDB} -e "var OM_CONF_FILE='$opt_configfile'; var ACTION='$opt_action'" -f "${js_file}"



