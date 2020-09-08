#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=`dirname "$SCRIPT"`

if [ ! -d "${SCRIPTPATH}/build" ];
then
   mkdir build
fi

INCLUDEPATH=$SCRIPTPATH"/../../include"
LIBPATH=$SCRIPTPATH"/../../lib"
COMMON="common.cpp"
if [ $# != 1 ]
then
   echo "Syntax: `basename $0` <program>"
   exit 0
fi
PROGRAM=$1
SOURCEFILE=$SCRIPTPATH"/"$PROGRAM".cpp"
COMMONFILE=$SCRIPTPATH"/"$COMMON
if [ ! -f $SOURCEFILE ]
then
   echo "Source file $SOURCEFILE does not exist"
   exit 0
fi

g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM -I$INCLUDEPATH -L$LIBPATH -lsdbcpp -O0 -ggdb -Wno-deprecated -lm -ldl
cp $LIBPATH/libsdbcpp.so $SCRIPTPATH"/build"

g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM.static -I$INCLUDEPATH -L$LIBPATH -O0 -ggdb -Wno-deprecated -lm $LIBPATH/libstaticsdbcpp.a -lpthread -ldl
