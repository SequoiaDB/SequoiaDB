#!/usr/bin/env bash

if [ $# -lt 1 ] ; then
    echo "usage: $(basename $0) PROPS_FILE [ARGS]" >&2
    exit 1
fi

source funcs.sh $1
shift

setCP || exit 1

load="$(grep '^load=' $PROPS | sed -e 's/^load=//')"
if [ "$load" == "True" ];then
   java -cp "$myCP" -Dprop=$PROPS LoadData $*
else
   java -cp "$myCP" -Dprop=$PROPS CopyData $*
fi
