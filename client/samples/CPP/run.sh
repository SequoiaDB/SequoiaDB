#!/bin/bash

if [ $# -lt 4 ] ; then
   echo  "Usage: $0 <hostname> <servicename> <username> <password>"
   exit 1
fi

echo "###############building file..."
./buildApp.sh connect
./buildApp.sh query
./buildApp.sh replicaGroup
./buildApp.sh index
./buildApp.sh sql
./buildApp.sh insert
./buildApp.sh update
./buildApp.sh lob

echo "###############running connect..."
./build/connect $1 $2 "$3" "$4"
echo "###############running snap..."
./build/query $1 $2 "$3" "$4"
#echo "###############running replicaGroup..."
#./build/replicaGroup $1 $2 "" ""
echo "###############running index..."
./build/index $1 $2 "$3" "$4"
echo "###############running sql..."
./build/sql $1 $2 "$3" "$4"
echo "###############running insert..."
./build/insert $1 $2 "$3" "$4"
echo "###############running update..."
./build/update $1 $2 "$3" "$4"
echo "###############running lob..."
./build/lob $1 $2 "$3" "$4"



echo "###############running connect.static..."
./build/connect.static $1 $2 "$3" "$4"
echo "###############running snap.static..."
./build/query.static $1 $2 "$3" "$4"
#echo "###############running replicaGroup.static..."
#./build/replicaGroup.static $1 $2 "$3" "$4"
echo "###############running index.static..."
./build/index.static $1 $2 "$3" "$4"
echo "###############running sql.static..."
./build/sql.static $1 $2 "$3" "$4"
echo "###############running insert.static..."
./build/insert.static $1 $2 "$3" "$4"
echo "###############running update.staic..."
./build/update.static $1 $2 "$3" "$4"
echo "###############running lob.staic..."
./build/lob.static $1 $2 "$3" "$4"

