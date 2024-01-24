#!/bin/bash
all=$(df -l  $1 | sed '1d' | awk '{print $3,$4,$5}')
used=$(echo $all | awk '{print $1}')
available=$(echo $all | awk '{print $2}')
percent=$(echo $all | awk '{print $3}')
percent=${percent%%\%}
if [ "$percent" -gt "$2" ]
then
  echo "NothingToDo"
  exit 0
fi
#echo $used $available $percent
total=$[$used+$available]
padSize=$(expr $total \* $2 / 100 - $used)
padCount=$(expr $padSize / 1024 / 100 + 1)
#echo $padSize
#echo $padCount
fileName=$1"/"${padSize}"byte.pad.tmp"
dd if=/dev/zero of=$fileName bs=100M count=$padCount > /dev/null 2>&1
echo $fileName