#!/bin/bash

needUpdate=1
needCompile=1
isRelease=0

hostname="localhost"
svcname="11810"
user=""
passwd=""
dbpath="/opt/sequoiadb/database/"
useSSL=0

autoTest=0

sslFile=( "sslTrue" "sslTrue.static" )

# display help
function display()
{
   echo "$0 --help | -h"
   echo -e "$0 [-noup] [-nocompile] [-release] [-hostname hostname] [-svcname svcname]\c"
   echo " [-user user] [-passwd passwd] [-dbpath dbpath] [-usessl] [-test]"
   echo ""
   echo " -noup               : 不更新svn，不加表示更新svn"
   echo " -nocompile          : 不执行编译，不加表示重新编译"
   echo " -release            : 编译release版本，不加表示编译debug版本"
   echo " -hostname hostname  : 指定主机名（默认localhost）"
   echo " -svcname svcname    : 指定节点端口号（独立模式或协调节点，默认11810）"
   echo " -user user          : 指定sdb登录用户名（默认为空）"
   echo " -passwd passwd      : 指定sdb登录用户密码（默认为空）"
   echo " -dbpath dbpath      : 指定节点路径（默认/opt/sequoiadb/database/）" 
   echo " -usessl             : 是否已开启SSL（默认未开启）"
   echo " -test               : 是否执行测试用例（默认不执行测试）"

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

# run testcase
function autoTest()
{
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
   
      # run testcase
      build_test/${testcase} -n $hostname -s $svcname -d $dbpath
      if [ $? != 0 ]; then
         echo $file" run failed with hostname = "$hostname" svcname = "$svcname" dbpath = "$dbpath
         break
      fi
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
      -dbpath )         shift
                        dbpath=$1
                        ;;
      -usessl )         useSSL=1
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
