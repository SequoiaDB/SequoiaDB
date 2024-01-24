#!/bin/bash
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..
if [ "$#" -ne 2 ] || ! [ -f "$1" ]; then
   echo "Usage: $0 listFile destinationPath" >&2
   exit 1
fi

DESTPATH=$2

# loop through the file
while read line ;do
   TARGETPATH=$DESTPATH/$line
   if [[ -f $TARGETPATH ]]; then
      # if the source is a file, let's just remove file
      echo "remove file $TARGETPATH"
      rm -f $TARGETPATH
   elif [[ -d $TARGETPATH ]]; then
      # if the source is a directory, let's remove dir
      echo "remove directory $TARGETPATH"
      rm -rf $TARGETPATH
   else
      echo "$TARGETPATH is not a file nor dir"
   fi
done < $1
