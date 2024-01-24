#!/bin/bash

function getHosts()
{
   cd data
   hosts=()
   for dirent in $(dir)
   do
      if [ -d $dirent ];then
         hosts=(${hosts[*]} $dirent)
      fi
   done
   cd .. 
   echo ${hosts[*]}
}

function getHardWareItemByName()
{
   if [ $# -eq 2 ];then
      hardwareinfo=data/$1/hardware.txt
   else
      hardwareinfo=data/hardware.txt
   fi
   
   if [ ! -f "${hardwareinfo}" ];then
      echo "${hardwareinfo} not exist"
      return
   fi 
   itemName=$2

   cat ${hardwareinfo}|awk -F '=' '/'"${itemName}"'/{print $2}'
   
}

function getSoftWareItemByName()
{

   if [ $# -eq 2 ];then
      softwareinfo=data/$1/software.txt
   else
      softwareinfo=data/software.txt
   fi
   
   if [ ! -f "${softwareinfo}" ];then
      echo "${softwareinfo} not exist"
      return
   fi

   itemName=$2
   cat ${softwareinfo} | awk -F '=' '/'"${itemName}"'/{print $2}'
}

function getCpuModeName()
{
   getHardWareItemByName $1 "model name"
}

function getPhysicalCpuNum()
{
   getHardWareItemByName $1 "Physical number"
}

function getCoreNumPerCpu()
{
   getHardWareItemByName $1 "Core\(s\) per socket"
}

function getThreadNumPerCore()
{
   getHardWareItemByName $1 "Thread\(s\) per core"
}

function getTotalMemoryNum()
{
   getHardWareItemByName $1 "MemTotal"
}

function getNicInfo()
{
   getHardWareItemByName $1 "NIC"
}

function getDiskCap()
{
   getHardWareItemByName $1 "Disk capacity"
}

function getOsRelease()
{
   getSoftWareItemByName $1 "OS Release"
}

function getKernelVersion()
{
   getSoftWareItemByName $1 "kernel"
}

function getSequoiaDBVersion()
{
   getSoftWareItemByName $1 "SequoiaDB version"
}

function getPostgreSQLVersion()
{
   getSoftWareItemByName $1 "PostgreSQL"
}

function getFileByHost()
{
   for file in `ls $1`;
   do
      if [ $file == $2 ];then
         echo $file
         break
      fi

      IPAddr=$(grep IP ${1}/$file|awk -F ':' '{print $2}'|tr -d ' ')
      if [ $IPAddr'M' == $2'M' ];then
         echo $file
         break
      fi
   done
}

function getAllGroupName()
{
   maxNum=0
   for file in `ls ${1}`
   do
      lineNum=$(wc -l ${1}/${file}|awk '{print $1}')
      if [ $maxNum -lt $lineNum ];then
         usedFile=${1}/${file}
         maxNum=${lineNum}
      fi
   done

   for((i=2; i <=${maxNum}; ++i))
   do
      groupName=(${groupName[@]} $(sed -n "${i}p" ${usedFile}|awk -F ":" '{print $1}'))
   done
   echo ${groupName[@]}
}

function getNodeNumOfGroupPerHost()
{
   maxNum=0
   for file in `ls ${1}`
   do
      lineNum=$(wc -l ${1}/${file}|awk '{print $1}')
      if [ $maxNum -lt $lineNum ];then
         usedFile=${1}/${file}
         maxNum=${lineNum}
      fi
   done
   
   for((i=2; i <=${maxNum}; ++i))
   do
      nodes=($(getNodesOfGroup $i $usedFile))
      nodesNumofGroup=(${nodesNumofGroup[@]} ${#nodes[*]})
   done
   echo ${nodesNumofGroup[@]}  
}

function getNodesOfGroup()
{
   OIFS=$IFS
   IFS=','
   echo $(sed -n "${1}p" ${2}|awk -F ':' '{print $2}')
   IFS=$OIFS
}

function getGroupName()
{
   echo $(sed -n "${1}p" ${2}|awk -F ':' '{print $1}')
}

function getHostsBySdbUrl()
{
    hosts=()
    OIFS=$IFS
    IFS=','
    addrs=($1)
    for ((i=0; i < ${#addrs[*]};++i))
    do
       host=$(echo ${addrs[$i]}|awk -F ':' '{print $1}')
       find=0
       for ((j=0; j <${#hosts[*]};++j))
       do
         if [ ${hosts[$j]} == ${host} ];then
            find=1
            break
         fi
       done
       if [ ${find} -eq 0 ];then
          hosts=(${hosts[@]} ${host})
       fi
    done
    IFS=$OIFS
    echo ${hosts[@]}
}

