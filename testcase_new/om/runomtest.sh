#!/bin/bash

# define test root path
storyTestRoot="js"
testRoots=($storyTestRoot)

libRoot="js/common"
commlibstr="commlib.js"

sdbRoot="../../bin"
uuid=$$
uuname="s$$test"

sshuser="root"
sshpasswd=""
sshport="22"

isLocal=1
deploy=1
omsvcname="11780"
omwebname="8000"
omhostname=""
omuser="admin"
ompasswd="admin"
localHostName=`hostname`
sdbcmPort="11790"

runresult=0

csprefix="local_om_test"
reportDirRoot=${csprefix}"_report"

rsrvnodedir="/opt/sequoiadb"

passDir=""
passFile=""

# define test parameter
testFile=""
stopWhenFailed=1
printOut=0
showNameWidth=60
testType="all"
runAllTest=0
specificDirorFile=0

# define stat parameter
sucNum=0
failedNum=0
useTime=0
beginTime=0
endTime=0
beginTimeSec=0  
endTimeSec=0
testcaseBTimeSec=0
testcaseETimeSec=0

printStr=""
lastCmdStr=""
needExit=0

# define ignore path and file
ignoredPaths=("common")
#ignoredPaths=("vote" "dataCompress" "bakupRestore")
ignoredFiles=()

# print help information
function showHelpInfo()
{
   echo "run om testcase 1.0.0 2018/6/25"
   echo "$0 --help"
   echo "$0 [-p path]|[-f file] [-t type] [-s stopFlag] [-hn] [-sp] [-so] [-ip] [-cm] [-os] [-ow] [-ou] [-op] [-addpid] [-print]"
   echo ""
   echo " -p path       : 运行指定路径下的JS用例。为相对目录，默认根目录为用例目录"
   echo " -f file       : 运行指定的JS用例。为相对目录，默认根目录为用例目录"
   echo " -t type       : 运行指定类型的用例，可取all|deploy|cleanup|onlytest，默认all"
   echo "                 all: 部署测试清理; deploy:只部署; cleanup:清理环境; onlytest:只测试;"
   echo " -s stopFlag   : 发生用例错误是否停止，0表示继续，1表示停止，默认为1"

   echo " -hn           : 指定测试OM主机的HostName或IP"
   echo " -sp           : 指定OM主机root用户的密码"
   echo " -so           : 指定OM主机ssh的端口，默认为22"
   echo " -ip           : 指定SequoiaDB安装路径，默认为/opt/sequoiadb，如果已经安装则跳过"
   echo " -cm           : 指定sdbcm的端口，默认为11790"

   echo " -os           : 指定测试的OM节点服务名，默认为11780"
   echo " -ow           : 指定测试的OM节点web端口，默认为8000"
   echo " -ou           : 指定测试的OM节点用户名，默认为admin"
   echo " -op           : 指定测试的OM节点密码，默认为admin"

   echo " -addpid       : 是否在CHANGEDPREFIX上加上当前进行PID"
   echo " -print        : 是否在屏幕上打印用例的输出"
   echo ""
   exit $1
}

# print content to result.txt
# $1：the content
function printToResultFile() 
{
   echo "$1" >> ${reportDirRoot}/result.txt
}

function showResult()
{
   local flag=$1
   
   echo "***********************************************************"
   echo "                    ***test result*** "
   echo " begin time: $beginTime"
   echo " end time  : $endTime"
   echo " use time  : `expr $endTimeSec - $beginTimeSec`(secs)"
   echo " total     : `expr $sucNum + $failedNum`"
   
   echo -n " succeed   :"
   if [ $flag -ne 0 ] ; then
      echo -e "\033[32;49;1m $sucNum \033[39;49;0m"
   else
      echo " $sucNum"
   fi
   
   echo -n " failed    :"
   if [ $failedNum -ne 0 -a $1 -ne 0 ] ; then
      echo -e "\033[31;49;1m $failedNum \033[39;49;0m"
   else
      echo " $failedNum"
   fi
   
   echo "***********************************************************"
}

# run a js file
# $1: js file directory
function runJSFile()
{
   local file=$1
   
   result=0
   lastCmdStr="$sdbRoot/sdb -e \"var CHANGEDPREFIX='${csprefix}'; var OMSVCNAME='${omsvcname}'; var OMWEBNAME='${omwebname}'; var OMHOSTNAME='${omhostname}'; var OM_USER='${omuser}'; var OM_PASSWD='${ompasswd}'; var IS_LOCAL_TEST=true; var UUID=$uuid; var UUNAME='${uuname}'; var RUNRESULT=$runresult; \" -f \"${libRoot}/func.js,$file\""
#   runresult=0
   if [ $printOut -eq 1 -o $# -gt 1 ] ; then
      echo "CMD: $lastCmdStr"
      eval $lastCmdStr
      result=$?
   else
      echo "CMD: $lastCmdStr" >> ${printOutFile}
      eval $lastCmdStr >> ${printOutFile}  
      result=$?
   fi
   return $result ;
}

# process a js testcase
function procJSFile()
{
   local file=$1    
   local testRoot=$2

   
   shortFile="${file#$testRoot/}"
   shortDir="${shortFile%/*}"
   
   if [ "${shortDir:0:1}" == "/" ] ; then  
      shortDir=""
   fi
   
   shortDir=${reportDir}"/"${shortDir} 
   printOutFile=${reportDir}"/"${shortFile}"_out.txt" 

   
   postfix="${file##*.}"
   if [ "$postfix" != "js" ] ; then
      return 1  
   fi

   libJSStr="${file%/*}"
   libJSStr=${libJSStr}/${commlibstr}
   if [ -e $libJSStr ] ; then
         testFile=${libJSStr}","${file}
   else
         testFile=${file}
   fi

   if [ $printOut -eq 1 ] ; then 
      echo "===>[$shortFile]"
   else
      #echo -n "$shortFile   "
      printf "===> %-${showNameWidth}s" $shortFile
   fi

   # run prepare for testcase
   runJSFile "${libRoot}/before_usecase.js"

   testcaseBTimeSec=`date +%s`
   $sdbRoot/sdb -s "try{ db.msg('Begin testcase[$file]') ; } catch( e ) { } "
   runJSFile "$testFile"
   ret=$?
#   runresult=$ret
   $sdbRoot/sdb -s "try{ db.msg('End testcase[$file]') ; } catch( e ) {} "
   testcaseETimeSec=`date +%s`
   if [ $printOut -eq 1 ] ; then
      echo -n "<===[$shortFile]"
   fi
   
   if [ $ret -ne 0 ]
   then
      failedNum=`expr $failedNum + 1`
      #printToResultFile "$shortFile --- [ Failed ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s)"
      printToResultFile "$(printf "===> %-${showNameWidth}s" $shortFile) [ Failed ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s)"
      echo -e "\033[31;49;1m [ Failed:$failedNum ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s) \033[39;49;0m"
   else
      sucNum=`expr $sucNum + 1`
      #printToResultFile "$shortFile --- [ Done ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s)"
      printToResultFile "$(printf "===> %-${showNameWidth}s" $shortFile) [ Done ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s)"
      echo -e "\033[32;49;1m [ Done:$sucNum ] `expr $testcaseETimeSec - $testcaseBTimeSec`(s) \033[39;49;0m"
   fi

   # run clear for testcase  
   if [ $ret -ne 0 -a $stopWhenFailed -ne 0 ] ; then
      runresult=$ret
      runJSFile "${libRoot}/after_usecase.js"
      return 2
   fi
   
   runresult=0
   runJSFile "${libRoot}/after_usecase.js"

   if [ $printOut -eq 1 ] ; then
      echo ""
   fi
   
   return 0
}

# process basic testcases
function procBasicTestCase()
{
   local testRoot=$2
   
   if [ $needExit -eq 1 ]
   then
      return
   fi
   if [ -d "$1" ];then
      for dir in $ignoredPaths
      do
         if [ "$1" == "$testRoot/$dir" ]
         then
            return;
         fi
      done
      
      if [  -f "$1/basic_testcases.list" ]
      then
         IFS=,
         arr=(`cat $1/basic_testcases.list |awk -F '=' '{print $2}'`)
         IFS=
         for item in ${arr[*]}
         do
            if [ -f "$1/$item" ]
            then
               procJSFile "$1/$item" $testRoot
               retCode=$?
               if [ $retCode -eq 2 ]
               then
                 needExit=1
                 break; 
               fi
            fi
         done
         return;
      fi
   
      #if [ -f "$1/basic.txt" ];then
      #   while read line
      #   do
      #      if [ -f "$1/$line" ];then
      #         procJSFile "$1/$line"
      #      fi
      #   done < "$1/basic.txt"
      #   return
      #fi
      
      for cur in `ls -l $1 |awk '{print $9}'`
      do
         procBasicTestCase "$1/$cur" $testRoot
      done
   else
      #procJSFile $1
      return;
   fi
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
         -p )            shift
                         specificDirorFile=1
                         passDir=$1
                         ;;
         -f )            shift
                         specificDirorFile=1
                         passFile=$1
                         ;;
         -s )            shift
                         stopWhenFailed=$(($1))
                         ;;
         -hn )           shift
                         omhostname="$1"
                         if [ "$omhostname"x = "localhostx" -o "$omhostname"x = "127.0.0.1x" -o "$omhostname" = "$localHostName" ] ; then
                           isLocal=1
                         else
                           isLocal=0
                         fi
                         ;;
         -sp )           shift
                         sshpasswd="$1"
                         ;;
         -so )           shift
                         sshport="$1"
                         ;;
         -ip )           shift
                         rsrvnodedir="$1"
                         ;;
         -cm )           shift
                         sdbcmPort="$1"
                         ;;
         -os )           shift
                         omsvcname="$1"
                         ;;
         -ow )           shift
                         omwebname="$1"
                         ;;
         -ou )           shift
                         omuser="$1"
                         ;;
         -ow )           shift
                         ompasswd="$1"
                         ;;
         -t )            shift   
                         testType="$1"
                         ;;

         -print )        printOut=1
                         ;;
         -addpid )       csprefix="local_para_$$"
                         reportDirRoot=${csprefix}"_report"
                         ;;
         * )             echo "invalid arguments: $1"
                         showHelpInfo 1
                         ;;
      esac
   shift
   done
}

function checkPara()
{
   if [ "$omhostname"x == "x" ] ; then
      echo "-hn hostname are required"
      exit 1
   fi
}

function analyTestType()
{
   case $testType in
      all )       testRoots[0]=$storyTestRoot
                  runAllTest=1
                  deploy=1
                  ;;
      deploy )    runAllTest=0
                  deploy=2
                  ;;
      cleanup )   runAllTest=0
                  deploy=4
                  ;;
      onlytest )  testRoots[0]=$storyTestRoot
                  runAllTest=1
                  deploy=3
                  ;;
      * )         echo "invalid testType: $testType"
                  showHelpInfo 1
                  ;;
   esac                                    
}

function filterTestcase()
{  
   local testRoot=$1
   
   pathLists=(`sed -n '2,6p' $testRoot/testcase.conf |awk -F '=' '{print $2}'`)
   for pathList in ${pathLists[@]} 
   do
      path2Space=${pathList//,/ }
      for path in $path2Space
      do
         ignoredPaths+=($path)
      done
   done
}
   
function generateFindCmd()
{  
   local testRoot=$1
   testDir=$testRoot 
   
   if [ "$passDir" != "" ]; then   
      if [ ${passDir:0:1} = "/" ] ; then
         testDir="$passDir"
      else
         testDir="$testRoot/$passDir"        
      fi
   fi 
   
   if [ "$passFile" != "" ]; then   
      if [ ${passFile:0:1} = "/" ] ; then
         testFile="$passFile"
      else
         testFile="$testRoot/$passFile"
      fi
   fi
   
   if [ "$testFile" != "" ] ; then
      testDir="$testFile"
      unset ignoredFiles
   fi
   
   if [ "$testDir" != "$testRoot" ] ; then
      unset ignoredPaths
   fi 
   
   # construct exclude dirs and exclude files
   pathString=""
   fileString=""
   findCmdStr="find $testDir "
   beginPrefix=""
   endPrefix=""

   for data in ${ignoredPaths[@]}
   do
      if [ "$pathString" != "" ] ; then
         pathString=${pathString}" -o "
      fi
      pathString=${pathString}"-path ""\""*/${data}"\""
      beginPrefix=" "
      endPrefix=" -prune -o  "
   done

   for data in ${ignoredFiles[@]}
   do
      if [ "$fileString" != "" -o "$pathString" != "" ] ; then
         fileString=${fileString}" -o "
      fi
      fileString=${fileString}"-name ""\""${data}"\""
      beginPrefix=" "
      endPrefix=" -prune -o "
   done

   # construct find command
   if [ "$pathString" != "" -o "$fileString" != "" ] ; then
      findCmdStr=${findCmdStr}${beginPrefix}"\( "${pathString}${fileString}" \)"${endPrefix}"-type f -print"
   else
      findCmdStr=${findCmdStr}${beginPrefix}${endPrefix}"-type f -print"
   fi

   echo "Find command  : $findCmdStr"
}

# remove all reports in "local_test_report"
function removeReport()
{       
   if [ -d $reportDirRoot ] ; then
      rm -rf $reportDirRoot/*
   else
      mkdir ${reportDirRoot}
   fi
}

function installPacket()
{
   if [ $deploy -eq 1 -o $deploy -eq 2 ] ; then
      echo "=== install packet "
      ret=`$sdbRoot/sdb -s "function _isSdbPacket( filePath ){                         \
               var rc = filePath.match( new RegExp( '[\.a-z0-9]+[-_run]', 'g' ) ) ;    \
               if( rc == null ) {                                                      \
                  return false ;                                                       \
               }                                                                       \
               if( rc.length < 7 ) {                                                   \
                  return false ;                                                       \
               }                                                                       \
               if( rc[0] != 'sequoiadb-' ) {                                           \
                  return false ;                                                       \
               }                                                                       \
               return true ;                                                           \
            }                                                                          \
            var _hostName  = '${omhostname}' ;                                         \
            var _sdbcmPort = '${sdbcmPort}' ;                                          \
            try {                                                                      \
               var oma = new Oma( _hostName, _sdbcmPort ) ;                            \
            } catch( e ) {                                                             \
               var _rootPath = 'packet' ;                                              \
               var files = File.list( { 'pathname': _rootPath } ).toArray() ;          \
               var _srcFile = '' ;                                                     \
               for( var i in files ) {                                                 \
                  var fileName = JSON.parse( files[i] )['name'] ;                      \
                  var filePath = catPath( _rootPath, fileName ) ;                      \
                  if ( File.isFile( filePath ) && _isSdbPacket( fileName ) ) {         \
                     _srcFile = filePath ;                                             \
                     break ;                                                           \
                  }                                                                    \
               }                                                                       \
               if( _srcFile.length == 0 ) {                                            \
                  var errStr = 'sequoiadb packet not found' ;                          \
                  println( errStr ) ;                                                  \
                  throw new Error( errStr ) ;                                          \
               }                                                                       \
               var _user      = '${sshuser}' ;                                         \
               var _passwd    = '${sshpasswd}' ;                                       \
               var _sshPort   = parseInt( '${sshport}' ) ;                             \
               try {                                                                   \
                  var ssh = new Ssh( _hostName, _user, _passwd, _sshPort ) ;           \
                  ssh.push( _srcFile, '/tmp/sequoiadb_om_test.run' ) ;                 \
                  ssh.exec( '/tmp/sequoiadb_om_test.run --mode unattended --prefix ${rsrvnodedir} --port ${sdbcmPort}' ) ;   \
               } catch( e ) {                                                          \
                  println( 'failed to install sequoiadb packet, detail: ' + e ) ;      \
                  throw e ;                                                            \
               }                                                                       \
            }"`
      if [ -n "$ret" ] ; then
         echo "$ret"
         exit 1
      fi
   fi
}

function copyPacket()
{
   if [ $deploy -eq 1 -o $deploy -eq 2 ] ; then
      echo "=== copy packet "
      ret=`$sdbRoot/sdb -s "function _isPacket( filePath ){                            \
               var rc = filePath.match( new RegExp( '[\.a-z0-9]+[-_run]', 'g' ) ) ;    \
               if( rc == null ) {                                                      \
                  return false ;                                                       \
               }                                                                       \
               if( rc.length < 7 ) {                                                   \
                  return false ;                                                       \
               }                                                                       \
               return true ;                                                           \
            }                                                                          \
            var _hostName = '${omhostname}' ;                                          \
            var _sdbcmPort = '${sdbcmPort}' ;                                          \
            var _packetPath = catPath( '${rsrvnodedir}', 'packet' ) ;                  \
            var _rootPath = 'packet' ;                                                 \
            var files = File.list( { 'pathname': _rootPath } ).toArray() ;             \
            for( var i in files ) {                                                    \
               var fileName = JSON.parse( files[i] )['name'] ;                         \
               var filePath = catPath( _rootPath, fileName ) ;                         \
               if ( File.isFile( filePath ) && _isPacket( fileName ) ) {               \
                  File.scp( filePath, _hostName + ':' + _sdbcmPort + '@' + catPath( _packetPath, fileName ) ) ;            \
               }                                                                       \
            }"`
   fi
}

function createOMsvc()
{
   if [ $deploy -eq 1 -o $deploy -eq 2 ] ; then
      echo "=== create om "
      ret=`$sdbRoot/sdb -s  "var _hostName = '${omhostname}' ;                         \
               var _sdbcmPort = '${sdbcmPort}' ;                                       \
               var _svcName = '${omsvcname}' ;                                         \
               var _dbPath = catPath( '${rsrvnodedir}', 'database/sms/' + _svcName ) ; \
               var _restPort = '${omwebname}' ;                                        \
               var _oma = new Oma( _hostName, _sdbcmPort ) ;                           \
               try {                                                                   \
                  _oma.createOM( _svcName, _dbPath, {httpname: _restPort} ) ;          \
               } catch( e ) {                                                          \
                  if( e != SDBCM_NODE_EXISTED )                                        \
                  {                                                                    \
                     println( 'Failed to create om: ' + e ) ;                          \
                     throw e ;                                                         \
                  }                                                                    \
               }                                                                       \
               try {                                                                   \
                  _oma.startNode( _svcName ) ;                                         \
               } catch( e ) {                                                          \
                  println( 'Failed to start om: ' + e ) ;                              \
                  throw e ;                                                            \
               }"`
      if [ -n "$ret" ] ; then
         echo "$ret"
         exit 1
      fi
   fi
}

function mainRun()
{
   local findCmdStr=$1
   local testRoot=$2
   
   libJSStr=""
   postfix=""
   testFile=""
   shortFile=""
   printOutFile=""
   shortDir=""
   retCode=0

   if [ $deploy -eq 1 -o $deploy -eq 2 ] ; then
      procJSFile "${libRoot}/all_prepare.js" "${libRoot}"
      retCode=$?
      if [ $retCode -ne 0 ] ; then
         return
      fi
   fi

   if [ $deploy -eq 1 -o $deploy -eq 3 ] ; then
      # run all test-cases 
      if [ $runAllTest -eq 1 -o $specificDirorFile -eq 1 ] ; then
         for file in `eval $findCmdStr`
         do
            procJSFile $file $testRoot
            retCode=$?
            if [ $retCode -eq 1 ] ; then
               continue
            elif [ $retCode -eq 2 ] ; then
               break
            fi
         done
      else
         procBasicTestCase $testDir $testRoot
      fi
   fi

   if [ $deploy -eq 1 -o $deploy -eq 4 ] ; then
      if [ $retCode -ne 2 ] ; then
         procJSFile "${libRoot}/all_clean.js" "${libRoot}"
      fi
   fi
}

# ***************************************************************
#                       run entry: main
# ***************************************************************

analyPara $*

checkPara

analyTestType

removeReport

installPacket

copyPacket

createOMsvc

echo "********************** Initialization environment completion *************************"

#print all parameter to screen
echo ""
echo "**************************************************************************************"
echo "CHANGEDPREFIX : $csprefix"
echo "UUID          : $uuid"
echo "UUNAME        : $uuname"
echo "OMSVCNAME     : $omsvcname"
echo "OMHOSTNAME    : $omhostname"

# generate command of find test files, and print 
declare -a findCmds                         #define findCmds as array
for testRoot in ${testRoots[@]}
do 
   #unset ignoredPaths
   filterTestcase $testRoot
   generateFindCmd $testRoot
   findCmds[${#findCmds[@]}]="$findCmdStr"  #add element in tail of array    
done
echo "**************************************************************************************"

# get test files and run
beginTime=`date`
beginTimeSec=`date +%s`

for(( i=0; i<${#testRoots[@]}; i++ ))
do
   case ${testRoots[i]} in
      $storyTestRoot )  reportDir="${reportDirRoot}/story"
                        tmpType="story"
                        mkdir ${reportDir}
                        ;;
   esac  
   echo -e "\e[46;31m ======>Begin to test $tmpType   =====> \e[0m"      #print bule font
                                                                
   mainRun "${findCmds[i]}" ${testRoots[i]}  
done

endTime=`date`
endTimeSec=`date +%s`
echo -e "\e[46;31m <======Finish test all testcases<===== \e[0m"
                   
# show result to screen and file "result.txt"
showResult 1
printToResultFile ""
printToResultFile "$(showResult 0)"

exit 0
