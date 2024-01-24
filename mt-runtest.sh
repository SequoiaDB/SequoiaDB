#!/bin/bash

# define test root path
storyTestRoot="testcase_new/story/js"
storyATTestRoot="testcase_new/story_at/js"
sdvTestRoot="testcase_new/sdv/js"
testRoots=($storyTestRoot)

coordsvcname="50000"
coordhostname="localhost"

csprefix="local_test"
reportDirRoot=${csprefix}"_report"

# define test parameter
stopWhenFailed=1
showNameWidth=60
testType="story"
threadNum=1

# define stat parameter
totalDoneNum=0
totalFailedNum=0
beginTime=0
endTime=0
beginTimeSec=0
endTimeSec=0

declare -a testPids

# define ignore path and file
ignoredPaths=()
#ignoredPaths=("vote" "dataCompress" "bakupRestore")
ignoredFiles=("commlib.js")

# print help information
function showHelpInfo()
{
   echo "run testcase 1.0.0 2014/2/25"
   echo "$0 --help"
   echo "$0 [-s stopFlag] [-n svcname] [-h hostname] [-j thnum]"
   echo ""
   echo " -t type     : 运行指定类型的用例，可取story|story_at|sdv|dev|all，dev 表示运行 story 及 story_at 用例"
   echo " -s stopFlag : 发生用例错误是否停止，0表示继续，1表示停止，默认为1"
   echo " -n svcname  : 指定测试的COORD节点服务名"
   echo " -h hostname : 指定测试的COORD节点HostName或IP"
   echo " -j thnum    : 指定测试的线程数"
   echo ""
   exit $1
}

# print content to result.txt
# $1: the content
function printToResultFile()
{
   echo "$1" >> ${reportDirRoot}/result.txt
}

function showResult()
{
   local colorFlag=$1

   echo "***********************************************************"
   echo "                    ***test result*** "
   echo " num of thd: $threadNum"
   echo " begin time: $beginTime"
   echo " end time  : $endTime"
   echo " use time  : `expr $endTimeSec - $beginTimeSec`(secs)"
   echo " total     : `expr $totalDoneNum + $totalFailedNum`"

   echo -n " succeed   :"
   if [ $colorFlag -ne 0 ] ; then
      echo -e "\033[32;49;1m $totalDoneNum \033[39;49;0m"
   else
      echo " $totalDoneNum"
   fi

   echo -n " failed    :"
   if [ $totalFailedNum -ne 0 -a $colorFlag -ne 0 ] ; then
      echo -e "\033[31;49;1m $totalFailedNum \033[39;49;0m"
   else
      echo " $totalFailedNum"
   fi

   echo "***********************************************************"
}

#analysis parameters
function analyPara()
{
   if [ $# -eq 1 -a "$1" = "--help" ] ; then
   showHelpInfo 0
   fi

   while [ "$1" != "" ]; do
      case $1 in
         -s )            shift
                         stopWhenFailed=$(($1))
                         ;;
         -n )            shift
                         coordsvcname="$1"
                         ;;
         -h )            shift
                         coordhostname="$1"
                         ;;
         -t )            shift
                         testType="$1"
                         ;;
         -j )            shift
                         threadNum="$1"
                         ;;
         * )             echo "invalid arguments: $1"
                         showHelpInfo 1
                         ;;
      esac
   shift
   done
}

function analyTestType()
{
   case $testType in
      story )  testRoots[0]=$storyTestRoot
               ;;
      story_at ) testRoots[0]=$storyATTestRoot
               ;;
      dev )    testRoots[0]=$storyTestRoot
               testRoots[1]=$storyATTestRoot
               ;;
      sdv )    testRoots[0]=$sdvTestRoot
               ;;
      all )    testRoots[0]=$sdvTestRoot
               testRoots[1]=$storyTestRoot
               testRoots[2]=$storyATTestRoot
               ;;
      * )      echo "invalid testType: $testType"
               showHelpInfo 1
               ;;
   esac
}

function filterTestcase()
{
   local testRoot=$1

   pathLists=(`sed -n '2,7p' $testRoot/testcase.conf |awk -F '=' '{print $2}'`)
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

   # construct exclude dirs and exclude files
   pathString=""
   fileString=""
   findCmdStr="find $testDir/* "

   for data in ${ignoredPaths[@]}
   do
      if [ "$pathString" != "" ] ; then
         pathString=${pathString}" -o "
      fi
      pathString=${pathString}"-path ""\""*/${data}"\""
   done

   for data in ${ignoredFiles[@]}
   do
      if [ "$fileString" != "" -o "$pathString" != "" ] ; then
         fileString=${fileString}" -o "
      fi
      fileString=${fileString}"-name ""\""${data}"\""
   done

   # construct find command
   findCmdStr=${findCmdStr}"\( "${pathString}${fileString}" \) -prune -o -type d -print"
   echo "Find command  : $findCmdStr"
}

# remove all reports in "local_test_report"
function removeReport()
{
   if [[ -d $reportDirRoot ]] ; then
      rm -rf $reportDirRoot/*
   else
      mkdir ${reportDirRoot}
   fi
}

function testThreadMain()
{
   local tid=$1
   local testType=$2
   local findCmdStr=$3

   local localPathId=0
   local expectedPathId=0

   echo "0">$reportDirRoot/.test.$tid.status

   for path in `eval ${findCmdStr} | sort`
   do
      testPath=`basename ${path}`
      localPathId=`expr $localPathId + 1`
      (
         flock -x -w 10 203 || exit 1
         globalPathId=`cat $reportDirRoot/.test.status`
         if [ $globalPathId -ne -1 ]
         then
            if [ $localPathId -eq $globalPathId ]
            then
               globalPathId=`expr $globalPathId + 1`
               echo $globalPathId>$reportDirRoot/.test.status
               echo $localPathId>$reportDirRoot/.test.$tid.status
               printf "===> %-${showNameWidth}s" "[ $testType: $testPath ]"
               echo -e "\033[33;49;1m [ TID: $tid Start ] \033[39;49;0m"
            fi
         else
            echo "-1">$reportDirRoot/.test.$tid.status
         fi
      )203>$reportDirRoot/.test.lock

      globalPathId=`cat $reportDirRoot/.test.status`
      expectedPathId=`cat $reportDirRoot/.test.$tid.status`
      if [ $expectedPathId -eq -1 -o $globalPathId -eq -1 ]
      then
         break
      fi

      if [ $expectedPathId -ne $localPathId ]
      then
         continue
      fi

      local testcaseBTimeSec=`date +%s`
      ./runtest.sh -addpid -h ${coordhostname} -n ${coordsvcname} -t $testType -p $testPath -s $stopWhenFailed 1>/dev/null 2>&1 &
      runPid=$!

      local localDirRoot="local_para_${runPid}_report"
      local finishedNum=0
      while true
      do
         sleep 1

         globalPathId=`cat $reportDirRoot/.test.status`
         expectedPathId=`cat $reportDirRoot/.test.$tid.status`
         if [ $expectedPathId -eq -1 -o $globalPathId -eq -1 ]
         then
            kill -TERM ${runPid}
            if [[ -e ${localDirRoot}/${testType}/${testPath} ]]
            then
               mv ${localDirRoot}/${testType}/${testPath} ${reportDirRoot}/${testType}/${testPath}
            fi
            rm -rf ${localDirRoot}
            exit 0
         fi

         local runExist=`ps -ef | grep runtest.sh | grep ${runPid} | wc -l`
         if [ ${runExist} -ne 0 ]
         then
            if [[ -e ${localDirRoot}/result.txt ]]
            then
               local curReport=`grep -e Done -e Failed ${localDirRoot}/result.txt`
               local curFinishedNum=`echo "${curReport}" | wc -l`
               if [ ${curFinishedNum} -gt ${finishedNum} -a "${curReport}" != "" ]
               then
                  local printCount=`expr ${curFinishedNum} - ${finishedNum}`
                  local printReport=`echo "${curReport}" | tail -n ${printCount}`
                  (
                     flock -x -w 10 206 || exit 1
                     echo -e "${printReport}"
                  )206>$reportDirRoot/.test.lock
                  finishedNum=${curFinishedNum}
               fi
            fi
         else
            break
         fi
      done

      local testcaseETimeSec=`date +%s`
      local localReport=`grep -e Done -e Failed ${localDirRoot}/result.txt`
      local localDoneNum=`grep Done ${localDirRoot}/result.txt | wc -l`
      local localFailedNum=`grep Failed ${localDirRoot}/result.txt | wc -l`

      mv ${localDirRoot}/${testType}/${testPath} ${reportDirRoot}/${testType}/${testPath}
      rm -rf $localDirRoot

      (
         flock -x -w 10 202 || exit 1

         printf "===> %-${showNameWidth}s" "[ $testType: $testPath ]"
         if [ $localFailedNum -ne 0 ]
         then
            echo -e "\033[33;49;1m [ TID: $tid\033[32;49;1m Done: $localDoneNum\033[39;49;0m\033[31;49;1m Failed: $localFailedNum\033[39;49;0m\033[33;49;1m Time: `expr $testcaseETimeSec - $testcaseBTimeSec`(s) ] \033[39;49;0m"
         else
            echo -e "\033[33;49;1m [ TID: $tid\033[32;49;1m Done: $localDoneNum\033[39;49;0m\033[33;49;1m Time: `expr $testcaseETimeSec - $testcaseBTimeSec`(s) ] \033[39;49;0m"
         fi
         printToResultFile "${localReport}"

         if [ $localDoneNum -ne 0 ]
         then
            local doneNum=`cat $reportDirRoot/.succeed.log`
            local doneNum=`expr $doneNum + $localDoneNum`
            echo $doneNum>$reportDirRoot/.succeed.log
         fi
         if [ $localFailedNum -ne 0 ]
         then
            local failedNum=`cat $reportDirRoot/.failed.log`
            local failedNum=`expr $failedNum + $localFailedNum`
            echo $failedNum>$reportDirRoot/.failed.log

            # Stop when failed
            if [ $stopWhenFailed -ne 0 ]
            then
               echo "-1">$reportDirRoot/.test.status
               break
            fi
         fi
      )202>$reportDirRoot/.test.lock
   done

   (
      flock -x -w 10 205 || exit 1
      printf "===> %-${showNameWidth}s" ""
      echo -e "\033[33;49;1m [ TID: $tid End ] \033[39;49;0m"
   )205>$reportDirRoot/.test.lock
}

function cleanUp()
{
   echo "-1">$reportDirRoot/.test.status
   for tid in `seq $threadNum`;
   do
      if [ ${testPids[$tid]} -ne 0 ]
      then
         wait ${testPids[$tid]}
         testPids[$tid]=0
      fi
   done
   exit 1
}

# ***************************************************************
#                       run entry
# ***************************************************************

analyPara $*

analyTestType

removeReport

#print all parameter to screen
echo ""
echo "**************************************************************************************"
echo "CHANGEDPREFIX : $csprefix"
echo "COORDSVCNAME  : $coordsvcname"
echo "COORDHOSTNAME : $coordhostname"

# generate command of find test files, and print
declare -a findCmds                         #define findCmds as array
for testRoot in ${testRoots[@]}
do
   unset ignoredPaths
   ignoredPaths+=(".svn")
   ignoredPaths+=(".git")
   filterTestcase $testRoot
   generateFindCmd $testRoot
   findCmds[${#findCmds[@]}]="$findCmdStr"  #add element in tail of array
done
echo "**************************************************************************************"

# get test files and run
beginTime=`date`
beginTimeSec=`date +%s`

for tid in `seq $threadNum`;
do
   testPids[$tid]=0
done

trap cleanUp SIGINT

for(( i=0; i<${#testRoots[@]}; i++ ))
do
   testType=""
   case ${testRoots[$i]} in
      $storyTestRoot )  testType="story"
                        ;;
      $sdvTestRoot )    testType="sdv"
                        ;;
      $storyATTestRoot ) testType="story_at"
                        ;;
   esac

   reportDir="${reportDirRoot}/$testType"
   mkdir ${reportDir}

   echo -e "\e[46;31m ======>Begin to test $testType   =====> \e[0m"      #print bule font

   echo "0">$reportDirRoot/.succeed.log
   echo "0">$reportDirRoot/.failed.log
   echo "1">$reportDirRoot/.test.status
   echo "0">$reportDirRoot/.generateFiles.lock

   # run all test-cases
   for tid in `seq $threadNum`
   do
      testThreadMain $tid $testType "${findCmds[$i]}" &
      testPids[$tid]=$!
   done

   for tid in `seq $threadNum`;
   do
      wait ${testPids[$tid]}
      testPids[$tid]=0
   done

   # collect test stats
   localDoneNum=`cat $reportDirRoot/.succeed.log`
   localFailedNum=`cat $reportDirRoot/.failed.log`

   totalDoneNum=`expr $totalDoneNum + $localDoneNum`
   totalFailedNum=`expr $totalFailedNum + $localFailedNum`
done

endTime=`date`
endTimeSec=`date +%s`
echo -e "\e[46;31m <======Finish test all testcases<===== \e[0m"

# show result to screen and file "result.txt"
showResult 1
printToResultFile ""
printToResultFile "$(showResult 0)"

exit 0
