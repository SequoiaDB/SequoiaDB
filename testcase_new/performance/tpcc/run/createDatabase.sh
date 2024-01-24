#!/bin/bash

if [ $# -lt 3 ];then
   echo "$0 user pgsqlInstallPath hostnumber host1 host2 ..."
fi

user=$1
shift 

installPath=$1
shift

hostNum=$1
shift

if [ "${hostNum}" -gt 0 ] 2>/dev/null ;then
  :
else
  echo "third parameter must be number"
  exit 1
fi

for((i=0; i<${hostNum}; ++i))
do
   ping -c 5 $1 1>>/dev/null 
   if [ $? -ne 0 ];then
      echo "host $1 Unreachable"
      exit 1
   fi
   ssh ${user}@$1 "${installPath}/bin/psql -d postgres -c \"drop user if exists benchmarksql\""
   ssh ${user}@$1 "${installPath}/bin/psql -d postgres -c \"CREATE USER benchmarksql WITH SUPERUSER ENCRYPTED PASSWORD 'changeme'\""
   ssh ${user}@$1 "${installPath}/bin/psql -d postgres -c \"DROP DATABASE IF EXISTS benchmarksql\""
   ssh ${user}@$1 "${installPath}/bin/psql -d postgres -c \"CREATE DATABASE benchmarksql OWNER benchmarksql\""
   shift
done