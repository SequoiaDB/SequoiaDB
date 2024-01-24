#!/bin/bash
LIBVERSION=
GCCVERSION=
LIBSOFILE=$3/libsdbcpp.so
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=`dirname "$SCRIPT"`

if [ ! -d "${SCRIPTPATH}/build" ];
then
   mkdir build
fi

COMMON="common.cpp"
PROGRAM=$1
SOURCEFILE=$SCRIPTPATH"/"$PROGRAM".cpp"
COMMONFILE=$SCRIPTPATH"/"$COMMON
if [ ! -f $SOURCEFILE ]
then
   echo "Source file $SOURCEFILE does not exist"
   exit 0
fi
function checkLibVersion()
{
  output=`strings -a  ${LIBSOFILE} | grep "GCC:"`
  cut=`echo ${output} | cut -d ')' -f 2`
  for element in $cut
  do
      LIBVERSION=$element
      break
  done
}
function checkGccVersion()
{
  GCCVERSION=`gcc -dumpversion`
}

checkLibVersion
checkGccVersion
if [ ${LIBVERSION} \< "5.1.0" ]
then
   if [ ${GCCVERSION} \> "5.1.0" ]  || [ ${GCCVERSION} = "5.1.0" ]
   then   
     g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM -I$2 -L$3 -lsdbcpp -O0 -ggdb -Wno-deprecated -lm -ldl -D_GLIBCXX_USE_CXX11_ABI=0
     g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM.static -I$2 -L$3 -O0 -ggdb -Wno-deprecated -lm $3/libstaticsdbcpp.a -lpthread -ldl -D_GLIBCXX_USE_CXX11_ABI=0
   fi
else
     g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM -I$2 -L$3 -lsdbcpp -O0 -ggdb -Wno-deprecated -lm -ldl
     g++ $SOURCEFILE $COMMONFILE -o $SCRIPTPATH"/build/"$PROGRAM.static -I$2 -L$3 -O0 -ggdb -Wno-deprecated -lm $3/libstaticsdbcpp.a -lpthread -ldl
fi
cp $3/libsdbcpp.so $SCRIPTPATH"/build"
