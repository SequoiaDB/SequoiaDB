#!/bin/bash
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..

if [ "$#" -eq 2 ] && [ -f "$1" ] && [ -d "$2" ]; then
   MACROS=$1
   DESTPATH=$2
   echo "MACROS=$1, DESTPATH=$2"
else
   echo "Usage: $0 [destinationPath] [macrosListFile]" >&2
   exit 1
fi

ENGINEPATH=$DESTPATH/SequoiaDB/engine
CPPPATTERN="*.cpp"
CPATTERN="*.c"
HPPPATTERN="*.hpp"
HPATTERN="*.h"
PATTERNARRAY=($CPPPATTERN $CPATTERN $HPPPATTERN $HPATTERN)
PATHARRAY=($ENGINEPATH)
for (( i=0; i < ${#PATTERNARRAY[@]}; i++))
do
   for (( j=0; j < ${#PATHARRAY[@]}; j++))
   do
      fileList=`find ${PATHARRAY[$j]} -name ${PATTERNARRAY[$i]}`
      for file in $fileList
      do
         echo "Remove macros for $file"
         # loop through macros list file
         while read line
         do
            echo "remove macro $line"
            python $SCRIPTPATH/removeMacro.py $file $line > $file.trim_$line
            if [ $? -ne 0 ]; then
               echo "Failed to remove macro for $file"
               exit 1
            fi
            cp $file.trim_$line $file
            rm $file.trim_$line
         done < $MACROS
      done
   done
done
exit 0
