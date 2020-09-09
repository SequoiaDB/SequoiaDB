#!/bin/bash

if [ $# -lt 4 ] ; then
   echo  "Usage: $0 <hostname> <servicename> <username> <password>" 
   exit 1
fi

echo "###############building file..."
./buildApp.sh connect
./buildApp.sh snap
./buildApp.sh update
./buildApp.sh insert
./buildApp.sh query
./buildApp.sh sampledb
./buildApp.sh sql
./buildApp.sh update_use_id
./buildApp.sh index
./buildApp.sh subArrayLen
./buildApp.sh upsert
./buildApp.sh lob

echo "###############running connect..."
./build/connect $1 $2 "$3" "$4"
echo "###############running snap..."
./build/snap $1 $2 "$3" "$4"
echo "###############running update..."
./build/update $1 $2 "$3" "$4"
echo "###############running insert..."
./build/insert $1 $2 "$3" "$4"
echo "###############running query..."
./build/query $1 $2 "$3" "$4"
echo "###############running sampledb..."
./build/sampledb $1 $2 "$3" "$4"
echo "###############running sql..."
./build/sql $1 $2 "$3" "$4"
echo "###############running update_use..."
./build/update_use_id $1 $2 "$3" "$4"
echo "###############running index..."
./build/index $1 $2 "$3" "$4"
echo "###############running subArrayLen..."
./build/subArrayLen $1 $2 "$3" "$4"
echo "###############running upsert..."
./build/upsert $1 $2 "$3" "$4"
echo "###############running lob..."
./build/lob $1 $2 "$3" "$4"

echo "###############running connect.static..."
./build/connect.static $1 $2 "$3" "$4"
echo "###############running snap.static..."
./build/snap.static $1 $2 "$3" "$4"
echo "###############running update.static..."
./build/update.static $1 $2 "$3" "$4"
echo "###############running insert.static..."
./build/insert.static $1 $2 "$3" "$4"
echo "###############running query.static..."
./build/query.static $1 $2 "$3" "$4"
echo "###############running sampledb.static..."
./build/sampledb.static $1 $2 "$3" "$4"
echo "###############running sql.static..."
./build/sql.static $1 $2 "$3" "$4"
echo "###############running update_use_id.static..."
./build/update_use_id.static $1 $2 "$3" "$4"
echo "###############running index.static..."
./build/index.static $1 $2 "$3" "$4"
echo "###############running subArrayLen.static..."
./build/subArrayLen.static $1 $2 "$3" "$4"
echo "###############running upsert.static..."
./build/upsert.static $1 $2 "$3" "$4"
echo "###############running lob.static..."
./build/lob.static $1 $2 "$3" "$4"

