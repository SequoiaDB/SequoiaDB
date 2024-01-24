#!/bin/bash
pid=$(lsof -nP -iTCP:$1 -sTCP:LISTEN|sed '1d'|awk '{print $2}')
if test -z $pid
then
  echo "can not find this svcName: $1" >&2
  exit 1
fi
kill -9 $pid
echo $pid