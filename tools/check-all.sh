#!/bin/bash

BASEDIR=$(dirname $0)
SDBDIR=$BASEDIR/..
BEAM_COMPILE=$BASEDIR/beam/linux/beam-3.6.1/bin/beam_compile
BEAM_COMPILE_OPTIONS="--beam::parms=$BASEDIR/beam/sequoiadb-parms.tcl --beam::prefixcc"
cd $SDBDIR
rm -rf $SDBDIR/build/linux2/dd
scons --dd --all | tee $BASEDIR/compile_output.txt
grep gcc $BASEDIR/compile_output.txt | grep Wall > $BASEDIR/beam_command.lst
grep g++ $BASEDIR/compile_output.txt | grep Wall >> $BASEDIR/beam_command.lst
cat $BASEDIR/beam_command.lst | while read line
do
   echo "$BEAM_COMPILE $BEAM_COMPILE_OPTIONS $line"
   $BEAM_COMPILE $BEAM_COMPILE_OPTIONS $line
done
