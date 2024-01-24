#bash

runtest_path=`pwd`

sdbRoot="bin"

function exec_cmd()
{
   local cmd=$1
   eval "${cmd}"
   return $?
}

function installES()
{
   local installES=$1
   if [ "$installES" == "true" ]; then
      clearES
      exec_cmd "cp /mnt/soft/elastic/elasticsearch-6.8.5.tar.gz $runtest_path"
      exec_cmd "tar -zxf $runtest_path/elasticsearch-6.8.5.tar.gz -C $runtest_path"
      local cur_user=`whoami`
      local cur_group=`groups | awk '{print $1}'`
      
      if [ "$cur_user" != "root" ];then
         exec_cmd "chown $cur_user:$cur_group $runtest_path/elasticsearch-6.8.5 -R"
         `ulimit -n 131072`
         `ulimit -v 655360`
         exec_cmd "$runtest_path/elasticsearch-6.8.5/bin/elasticsearch -d"
      else
         exec_cmd "chown sdbadmin:sdbadmin_group $runtest_path/elasticsearch-6.8.5 -R"
         exec_cmd "su sdbadmin -c 'ulimit -n 131072'"
         exec_cmd "su sdbadmin -c 'ulimit -v 655360'"
         exec_cmd "su sdbadmin -c '$runtest_path/elasticsearch-6.8.5/bin/elasticsearch -d'"
      fi
   else
      echo "not install es"
   fi
}

function installESAdapter()
{
   local adapter=$1
   if [ "$adapter" == "true" ]; then
      clearAdpter
      local ret=`$sdbRoot/sdblist -t db -r data | grep -v Total|awk '{print $1}'  | awk -F '(' '{print $2}'|awk -F ')' '{print $1}'`
      exec_cmd "mkdir -p $runtest_path/conf/sdbseadapter"
      OLD_IFS="$IFS"
      IFS=" "
      arr=($ret)
      IFS="$OLD_IFS"
      for arg in ${arr[@]}
      do
         exec_cmd "mkdir -p $runtest_path/conf/sdbseadapter/$arg"
         exec_cmd "cp $runtest_path/conf/samples/sdbseadapter.conf $runtest_path/conf/sdbseadapter/$arg"
         sed -i 's/datanodehost=.*/datanodehost=localhost/g'   $runtest_path/conf/sdbseadapter/$arg/sdbseadapter.conf
         sed -i 's/#\ datasvcname=.*/datasvcname='$arg'/g'   $runtest_path/conf/sdbseadapter/$arg/sdbseadapter.conf
         sed -i 's/searchenginehost=.*/searchenginehost=localhost/g'   $runtest_path/conf/sdbseadapter/$arg/sdbseadapter.conf
         sed -i 's/searchengineport=.*/searchengineport=9200/g'   $runtest_path/conf/sdbseadapter/$arg/sdbseadapter.conf
         sed -i 's/diaglevel=3*/diaglevel=5/g'   $runtest_path/conf/sdbseadapter/$arg/sdbseadapter.conf
      done
      exec_cmd "chmod 755 $runtest_path/conf/sdbseadapter -R"
      #start sdbseadapter
      for arg in ${arr[@]}
      do
         exec_cmd "$sdbRoot/sdbseadapter -c $runtest_path/conf/sdbseadapter/$arg &"
      done
      sleep 20
   fi
}

function clearAdpter()
{
   `ps -ef | grep sdbseadapter |grep -v grep | awk '{print $2}' |  xargs kill -9`
   exec_cmd "rm -rf $runtest_path/conf/sdbseadapter"
   exec_cmd "rm -rf $runtest_path/conf/log"
}

function clearES()
{
   `ps -ef | grep elasticsearch |grep -v grep | awk '{print $2}' |  xargs kill -9`
   exec_cmd "rm -rf $runtest_path/elasticsearch-6.8.5"
}

# print help information
function showHelpInfo()
{
   echo "install es 1.0.0 2018/10/18"
   echo "$0 --help"
   echo "$0 [-installES] [-adapter]"
   echo ""
   echo " -installES     : 指定是否安装es环境，true表示安装，false表示不安装，默认为true"
   echo " -adapter       : 指定是否安装es适配器，true表示安装，false表示不安装，默认为true"
   echo ""
   exit $1
}

#analysis parameters
#函数参数为：脚本参数
function analyPara()
{
   if [ $# -eq 1 -a "$1" = "--help" ] ; then
   showHelpInfo 0
   fi

   while [ "$1" != "" ]; do
      case $1 in
         -installES )    shift
                         installES="$1"
                         ;;
         -adapter )      shift
                         adapter="$1"
                         ;;
         * )             echo "invalid arguments: $1"
                         showHelpInfo 1
                         ;;
      esac
   shift
   done     
}

# ***************************************************************
#                       run entry: main
# ***************************************************************

analyPara $*

if [ "$installES" == "" ]; then
   installES="true"
fi
if [ "$adapter" == "" ]; then
   adapter="true"
fi

installES $installES

installESAdapter $adapter
