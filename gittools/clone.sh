#!/bin/bash
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..

if [ "$#" -ne 1 ]; then
   echo "Usage: $0 destinationPath" >&2
   exit 1
fi

# warning message
echo "You want to clone svn build to git"
echo "The destination directory $1 will be cleaned up"
echo "You have 10 seconds to cancel the operation by press Ctrl-C"
sleep 10

echo "Start..."

# clean up the target directory
echo "Clean up $1"
rm -rf $1/* 2>/dev/null

# manually build SequoiaDB/engine/include/ossVer_Autogen.h
# replace WCREV to svn version
echo "Build svn version"
# just in case SVN use chinese
export LANG='en_US'
sed "s/WCREV/$(svn info | grep Revision | awk '{print $2}')/g" $ROOTPATH/misc/autogen/ver_conf.h.in > $ROOTPATH/ver_conf.h.in.tmp
# replace $ to empty string and replace ossVer_autogen.h
sed 's/\$//g' $ROOTPATH/ver_conf.h.in.tmp > $ROOTPATH/misc/autogen/ver_conf.h

sed -i '/#define GENERAL_OPT_DOC_FILE/'d $ROOTPATH/misc/autogen/config.h

# copy file to new location
echo "Copy files"
$SCRIPTPATH/copyFiles.sh $SCRIPTPATH/required.lst $1
if [ $? -ne 0 ]; then
   echo "Failed to copy file to $1"
   exit 1
fi

# remove files from exclusive list
echo "Remove files"
$SCRIPTPATH/removeFiles.sh $SCRIPTPATH/exclusive.lst $1
if [ $? -ne 0 ]; then
   echo "Failed to remove file from $1"
   exit 1
fi

# create gitfile to prevent svn loopup during compile
echo "Create gitbuild file"
touch $1/gitbuild

# remove all svn info
echo "Remove svn subdir from $1"
$SCRIPTPATH/removeSvn.sh $1
if [ $? -ne 0 ]; then
   echo "Failed to remove svn from $1"
   exit 1
fi

# make sure there's no svn directory exist
output=`find $1 -name ".svn" -print`
if [ x"$output" != x ]; then
   echo "SVN subdirectory exist"
   exit 1
fi

# remove comments
echo "Remove all comments from $1"
$SCRIPTPATH/removeCommentForSource.sh $1
if [ $? -ne 0 ]; then
   echo "Failed to remove comment from $1"
   exit 1
fi

# remove macros
echo "Remove all macros from $1"
$SCRIPTPATH/removeMacroForSource.sh $SCRIPTPATH/removedMacros.lst $1
if [ $? -ne 0 ]; then
   echo "Failed to remove macro from $1"
   exit 1
fi

echo "Clone finished!"

