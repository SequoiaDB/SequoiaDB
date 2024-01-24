#!/bin/bash

if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
   echo "Usage: $0 FileName" >&2
   exit 1
fi
# skip PD_TRACE_DECLARE_FUNCTION comment, and remove all other lines starting
# with /
sed '/PD_TRACE_DECLARE_FUNCTION/n;/^[[:space:]]*\/\//d' $1 > $1.commentTrim
cp $1.commentTrim $1
rm $1.commentTrim
exit 0
