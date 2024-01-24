#!/bin/sh 
#
#    AUTHOR:
#
#        Florian Krohm
#
#    DESCRIPTION:
#
#        Shell script for processing PL8 files.
#
#    MODIFICATIONS:
#
#        See cvs log for modifications.

$beam_debugger "$beam_bindir/beam_pl8c" "$@"

#
# !!!! THE INVOCATION OF BEAM MUST BE THE LAST COMMAND IN THIS FILE.
# !!!! NO COMMANDS MAY FOLLOW IT.
