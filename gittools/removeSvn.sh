#!/bin/bash
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..
if [ "$#" -eq 0 ]; then
   DESTPATH=$ROOTPATH
elif [ "$#" -eq 1 ] && [ -d "$1" ]; then
   DESTPATH=$1
else
   echo "Usage: $0 [destinationPath]" >&2
   exit 1
fi
echo "Ready to remove svn dir from ${DESTPATH}, press Ctrl-C in 5 seconds to cancel the operation"
sleep 5

find ${DESTPATH} -name ".svn" -exec rm -rf {} \;
exit 0
