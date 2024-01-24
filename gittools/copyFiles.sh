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

# if the destination path doesn't exist, we have to create one
function testAndCreateDir {
   if [ $# -ne 1 ]
   then
      echo "path is expected"
      exit 1
   fi
   if [[ ! -e $1 ]]; then
      mkdir -p $1
      if [ $? -ne 0 ]; then
         echo "Not able to create path $1"
         exit 1
      fi
   elif [[ ! -d $DESTPATH ]]; then
      echo "DESTPATH already exists but is not a directory" 1>&2
   fi
}

testAndCreateDir $DESTPATH

# loop through the file
while read line ;do
   SOURCEPATH=$ROOTPATH/$line
   TARGETPATH=$DESTPATH/$line
   TARGETDIR=$DESTPATH/$(dirname $line)
   if [[ -f $SOURCEPATH ]]; then
      # if the source is a file, let's just copy
      testAndCreateDir $TARGETDIR
      echo "copy file $SOURCEPATH to $TARGETPATH"
      cp $SOURCEPATH $TARGETPATH
   elif [[ -d $SOURCEPATH ]]; then
      # if the source is a directory, let's copy dir
      mkdir -p $TARGETPATH
      echo "copy directory $SOURCEPATH to $TARGETDIR"
      cp -rf $SOURCEPATH $TARGETDIR
   else
      echo "$SOURCEPATH is not a file nor dir"
      exit 1
   fi
done < $1
