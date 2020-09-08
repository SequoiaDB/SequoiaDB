#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=`dirname "$SCRIPT"`

if [ ! -d "${SCRIPTPATH}/build" ];
then
   mkdir build
fi

INCLUDEPATH=$SCRIPTPATH"/../../include"
LIBPATH=$SCRIPTPATH"/../../lib"
COMMON="common.c"
if [ $# != 1 ]
then
   echo "Syntax: `basename $0` <program>"
   exit 0
fi
PROGRAM=$1
SOURCEFILE=$SCRIPTPATH"/"$PROGRAM".c"
COMMONFILE=$SCRIPTPATH"/"$COMMON
if [ ! -f $SOURCEFILE ]
then
   echo "Source file $SOURCEFILE does not exist"
   exit 0
fi

cc $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM -I$INCLUDEPATH -L$LIBPATH -lsdbc -O0 -ggdb -lm -ldl
cp $LIBPATH/libsdbc.so $SCRIPTPATH"/build"

cc $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM.static -I$INCLUDEPATH -L$LIBPATH -O0 -ggdb $LIBPATH/libstaticsdbc.a -lm -ldl -lpthread
