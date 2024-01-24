#!/bin/sh
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..

if [ "$#" -ne 2 ]; then
   echo "Usage: $0 sourcePattern targetPattern" >&2
   exit 1
fi

find $ROOTPATH -name "*$1*" -not -path "$ROOTPATH/.git/*" -exec ./renameFile.sh $1 $2 {} \;
