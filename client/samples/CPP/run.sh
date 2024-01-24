#!/bin/bash
hostname=`hostname`
svcname=11810
username="sdbadmin"
password="sdbadmin"
include_path="../../include"
lib_path="../../lib"
# print help information
function showHelpInfo()
{  
   echo "$0 --help"
   echo "$0 [-h hostname] [-s svcname] [-u username] [-p password] [-l lib_path] [-i include_path]"
   echo ""
   echo " -h hostname            : 指定测试的COORD节点HostName或IP，默认为本地机器的主机名"
   echo " -s svcname             : 指定测试的COORD节点服务名，默认为11810"
   echo " -u username            : 指定测试的用户名，默认为sdbadmin"
   echo " -p password            : 指定测试的用户名对应的用户密码，默认为sdbadmin"
   echo " -l lib_path            : 指定cpp驱动的lib目录路径，默认为../../lib"
   echo " -i include_path        ：指定cpp驱动的include目录路径，默认为../../include"
   echo ""
   exit $1
}
function analyPara()
{
   if [ $# -eq 1 -a "$1" = "--help" ] ; then
   showHelpInfo 0
   fi   
 
   while [ "$1" != "" ]; do
      case $1 in         
         -h )            shift
                         hostname=$1
                         ;;
         -s )            shift
                         svcname=$1
                         ;;
         -u )            shift
                         username="$1"
                         ;;
         -p )            shift
                         password="$1"
                         ;;
         -l )            shift
                         lib_path="$1"
                         ;;
         -i )            shift
                         include_path="$1"
                         ;;
          * )            echo "invalid arguments: $1"
                         showHelpInfo 1
                         ;;
      esac
   shift
   done
}
analyPara $*
echo ${hostname} ${svcname} ${username} ${password} ${lib_path} ${include_path}
echo "###############building file..."
./buildApp.sh connect ${include_path} ${lib_path}
./buildApp.sh query ${include_path} ${lib_path}
./buildApp.sh replicaGroup ${include_path} ${lib_path}
./buildApp.sh index ${include_path} ${lib_path}
./buildApp.sh sql ${include_path} ${lib_path}
./buildApp.sh insert ${include_path} ${lib_path}
./buildApp.sh update ${include_path} ${lib_path}
./buildApp.sh lob ${include_path} ${lib_path}

echo "###############running connect..."
./build/connect ${hostname} ${svcname} ${username} ${password}
echo "###############running snap..."
./build/query ${hostname} ${svcname} ${username} ${password}
#echo "###############running replicaGroup..."
#./build/replicaGroup ${hostname} ${svcname} "" ""
echo "###############running index..."
./build/index ${hostname} ${svcname} ${username} ${password}
echo "###############running sql..."
./build/sql ${hostname} ${svcname} ${username} ${password}
echo "###############running insert..."
./build/insert ${hostname} ${svcname} ${username} ${password}
echo "###############running update..."
./build/update ${hostname} ${svcname} ${username} ${password}
echo "###############running lob..."
./build/lob ${hostname} ${svcname} ${username} ${password}



echo "###############running connect.static..."
./build/connect.static ${hostname} ${svcname} ${username} ${password}
echo "###############running snap.static..."
./build/query.static ${hostname} ${svcname} ${username} ${password}
#echo "###############running replicaGroup.static..."
#./build/replicaGroup.static ${hostname} ${svcname} ${username} ${password}
echo "###############running index.static..."
./build/index.static ${hostname} ${svcname} ${username} ${password}
echo "###############running sql.static..."
./build/sql.static ${hostname} ${svcname} ${username} ${password}
echo "###############running insert.static..."
./build/insert.static ${hostname} ${svcname} ${username} ${password}
echo "###############running update.staic..."
./build/update.static ${hostname} ${svcname} ${username} ${password}
echo "###############running lob.staic..."
./build/lob.static ${hostname} ${svcname} ${username} ${password}

