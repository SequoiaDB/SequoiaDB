#!/bin/sh
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..
ROOTABSPATH=$(readlink -f "$ROOTPATH")

if [ "$#" -ne 3 ]; then
   echo "Usage: $0 sourcePattern targetPattern filename" >&2
   exit 1
fi

source=$1
target=$2
relativetargetfile=`echo $3 | sed 's|'$ROOTABSPATH'||g'`
targetfile=`echo $relativetargetfile | sed 's/'$source'/'$target'/g'`
git mv "$3" "$ROOTABSPATH/$targetfile"
