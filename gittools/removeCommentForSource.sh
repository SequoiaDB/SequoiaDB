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

ENGINEPATH=$DESTPATH/SequoiaDB
MISCPATH=$DESTPATH/misc
DRIVERPATH=$DESTPATH/driver
CPPPATTERN="*.cpp"
CPATTERN="*.c"
HPPPATTERN="*.hpp"
HPATTERN="*.h"
JAVAPATTERN="*.java"
PATTERNARRAY=($CPPPATTERN $CPATTERN $HPPPATTERN $HPATTERN $JAVAPATTERN)
PATHARRAY=($ENGINEPATH $MISCPATH $DRIVERPATH)
for (( i=0; i < ${#PATTERNARRAY[@]}; i++))
do
   for (( j=0; j < ${#PATHARRAY[@]}; j++))
   do
      fileList=`find ${PATHARRAY[$j]} -name ${PATTERNARRAY[$i]}`
      for file in $fileList
      do
         echo "Remove comment for $file"
         $SCRIPTPATH/removeComment.sh $file
         if [ $? -ne 0 ]; then
            echo "Failed to remove comment for $file"
            exit 1
         fi
      done
   done
done
exit 0
