#!/bin/bash
CUR_PATH=$(pwd)

faileTestCaseFile="$CUR_PATH/failedTestcase.txt"
needUpdate=1
needCompile=1
isRelease=0

hostname="localhost"
svcname="50000"
user=""
passwd=""
dshostname="localhost"
dssvcname="50000"
dbpath="/opt/sequoiadb/database/"
useSSL=0
transactionOn=0
stopFlag=1
autoTest=0

sslFile=( "sslTrue9648" "sslTrue9648.static" )
transFile=( "getListTransaction12526" "getListTransaction12526.static" 
            "getSnapshotTransaction12662" "getSnapshotTransaction12662.static"
            "trans12520_12675" "trans12520_12675.static" )

# display help
function display()
{
   echo "$0 --help | -h"
   echo -e "$0 [-noup] [-nocompile] [-release] [-hostname hostname] [-svcname svcname]\c"
   echo " [-user user] [-passwd passwd] [-dbpath dbpath] [-usessl] [-transactionOn] [-test]"
   echo ""
   echo " -noup               : 不更新svn，不加表示更新svn"
   echo " -nocompile          : 不执行编译，不加表示重新编译"
   echo " -release            : 编译release版本，不加表示编译debug版本"
   echo " -hostname hostname  : 指定主机名（默认localhost）"
   echo " -svcname svcname    : 指定节点端口号（独立模式或协调节点，默认50000）"
   echo " -user user          : 指定sdb登录用户名（默认为空）"
   echo " -passwd passwd      : 指定sdb登录用户密码（默认为空）"
   echo " -dshostname dshost  : 指定数据源主机名（默认localhost）"
   echo " -dssvcname dssvc    : 指定数据源节点端口号（协调节点，默认50000）"
   echo " -dbpath dbpath      : 指定节点路径（默认/opt/sequoiadb/database/）" 
   echo " -usessl             : 是否已开启SSL（默认未开启）"
   echo " -transactionOn      : 是否已开启事务（默认未开启）"
   echo " -test               : 是否执行测试用例（默认不执行测试）"
   echo " -stopFlag           : 遇到用例错误是否停止，0表示继续，1表示停止，默认为1"
   echo " -runLastFailed      : 是否执行上次运行失败的测试用例，默认不执行，执行上次失败用例时"
   echo "                       无须再次指定 hostname、svcname、user 等用例参数，"

   echo ""
   exit $1
}

# svn update
function svnUp()
{
   echo "======================Begin to update files========================="
   svn up
   if [ $? -ne 0 ];then
      echo "svn update failed"
      exit 1
   fi
   echo "======================End to update files==========================="
}

# compile testcase
function compile()
{
   echo "======================Begin to remove builded files=================="
   rm -rf build
   rm -rf build_test
   echo "======================End to remove builded files===================="

   echo "====================Begin to complie================================="
   compileCmd="scons "
   if [ $isRelease -eq 0 ] ; then
      compileCmd=${compileCmd}" --dd"
   fi
   echo "CMD: $compileCmd"
   $compileCmd
   if [ $? -ne 0 ] ; then
      echo "compile failed"
      exit 1
   fi
   echo "====================End to compile===================================="
}

function addFailedTestcase()
{
   local failedCase="$1"
   if [ ! -f "$faileTestCaseFile" ] ; then
      touch "$faileTestCaseFile"
   fi
   if [ "$failedCase" != "" ] ; then
       echo -e "$failedCase" >> "$faileTestCaseFile"
   fi
}

function runLastFailedTestcase()
{
   local failedCaseList=()
   if [ ! -f "$faileTestCaseFile" ] ; then
      echo "There are no testcases that failed last time"
      exit $1
   else
      failedCaseList=$(cat "$faileTestCaseFile")
      if [ ${#failedCaseList} = 0 ]; then
         echo "There are no testcases that failed last time"
         exit $1
      fi
      rm -f "$faileTestCaseFile"
   fi
   echo "$failedCaseList" | while read line
   do
      runTestcase "$line"
   done
   exit $1
}

function runTestcase()
{
   local testCmd="$@"
   echo ""
   echo "Execution the testcase cmd: $testCmd"
   eval "$testCmd"
   if [ $? != 0 ]; then
      echo "[ERROR] Execution of the testcase failed"
      addFailedTestcase "$testCmd"
      if [ $stopFlag = 0 ] ; then
         continue
      else
         break
      fi
   fi
}

# run testcase
function autoTest()
{
   rm -f "$faileTestCaseFile"
   testcases=`ls build_test`
   for testcase in ${testcases}
   do
      # check useSSL
      if [ $useSSL -eq 0 ]; then
         found=false
         for file in ${sslFile[@]}
         do
            if [ ${file} == ${testcase} ]; then
               found=true
               break
            fi
         done
         if [ ${found} = true ]; then
            continue
         fi
      fi
   
      # check transactionOn
      if [ $transactionOn -eq 0 ]; then
         found=false
         for file in ${transFile[@]}
         do
            if [ ${file} == ${testcase} ]; then
               found=true
               break
            fi
         done
         if [ ${found} = true ]; then
            continue
         fi
      fi
   
      # run testcase
      testCmd="build_test/${testcase} -n $hostname -s $svcname -d $dbpath -o $dshostname -p $dssvcname"
      runTestcase "$testCmd"
   done
}

# read param
while [ "$1" != "" ]; do
   case $1 in
      -noup )           needUpdate=0
                        ;;
      -nocompile )      needCompile=0
                        ;;
      -release )        isRelease=1
                        ;;
      -hostname )       shift
                        hostname=$1
                        ;;
      -svcname )        shift
                        svcname=$1
                        ;;
      -user )           shift
                        user=$1
                        ;;
      -passwd )         shift
                        passwd=$1
                        ;;
      -dshostname )     shift
                        dshostname=$1
                        ;;
      -dssvcname )      shift
                        dssvcname=$1
                        ;;
      -dbpath )         shift
                        dbpath=$1
                        ;;
      -usessl )         useSSL=1
                        ;;
      -transactionOn )  transactionOn=1
                        ;;
      -stopFlag )       shift
                        stopFlag=$1
                        ;;
      -runLastFailed )  runLastFailedTestcase 0
                        ;;
      -test )           autoTest=1
                        ;;
      --help | -h )     display 0
                        ;;
      * )               echo "Invalid argument: $1"
                        display 1
   esac
   shift
done

if [ $needUpdate -ne 0 ] ; then
   svnUp
fi

if [ $needCompile -ne 0 ] ; then
   compile
fi

if [ $autoTest -ne 0 ] ; then
   autoTest
fi
