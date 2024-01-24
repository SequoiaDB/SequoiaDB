#!/bin/sh
# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTPATH=$(dirname "$SCRIPT")
ROOTPATH=$SCRIPTPATH/..

if [ "$#" -ne 1 ]; then
   echo "Usage: $0 targetProjectName" >&2
   exit 1
fi

if ! [ -f "$ROOTPATH/gitbuild" ]; then
   echo "The script must run against github build"
   exit 1
fi

# warning message
echo "You want to rename the project to $1"
echo "You have 10 seconds to cancel the operation by press Ctrl-C"
sleep 10

echo "Start..."

projname=$1
upperprojname="${projname^^}"

# rename files
while true
do
	echo "rename sequoiadb"
	out=`$SCRIPTPATH/renameFileName.sh sequoiadb $projname 2>&1`
	if [ "$out" == "" ]; then
		break
	fi
done
while true
do
	echo "rename SequoiaDB"
        out=`$SCRIPTPATH/renameFileName.sh SequoiaDB $projname 2>&1`
        if [ "$out" == "" ]; then
                break
        fi
done
while true
do
	echo "rename Sequoiadb"
        out=`$SCRIPTPATH/renameFileName.sh Sequoiadb $projname 2>&1`
        if [ "$out" == "" ]; then
                break
        fi
done
while true
do
        echo "rename SEQUOIADB"
        out=`$SCRIPTPATH/renameFileName.sh SEQUOIADB $upperprojname 2>&1`
        if [ "$out" == "" ]; then
                break
        fi
done

# replace string
find $ROOTPATH -type f -not -path "$ROOTPATH/gittools/*" -not -path "$ROOTPATH/.git/*" -exec sed -i 's/sequoiadb/'$projname'/g' {} \;
find $ROOTPATH -type f -not -path "$ROOTPATH/gittools/*" -not -path "$ROOTPATH/.git/*" -exec sed -i 's/SequoiaDB/'$projname'/g' {} \;
find $ROOTPATH -type f -not -path "$ROOTPATH/gittools/*" -not -path "$ROOTPATH/.git/*" -exec sed -i 's/Sequoiadb/'$projname'/g' {} \;
find $ROOTPATH -type f -not -path "$ROOTPATH/gittools/*" -not -path "$ROOTPATH/.git/*" -exec sed -i 's/SEQUOIADB/'$upperprojname'/g' {} \;
