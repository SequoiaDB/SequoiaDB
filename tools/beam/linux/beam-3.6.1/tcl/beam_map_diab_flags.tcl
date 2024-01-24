# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2005-2010 IBM Corporation. All rights reserved.
#
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp
#
# The source code for this program is not published or otherwise divested
# of its trade secrets, irrespective of what has been deposited within
# the U.S. Copyright Office.
#
#
#    AUTHOR:
#
#        Florian Krohm
#
#    DESCRIPTION:
#
#        This script translates the command line for the Diab Data compiler.
#        It is based on scripts/beam_map_diab_flags.sh but differs in two
#        ways:
#
#        (1) It does not depend on the shell variable diab_qidirfirst
#        (2) It does not pass "-I@" to the parser
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.
#
#        04/11/05  florian  Created; based on scripts/beam_map_diab_flags.sh

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_diab_flags)] } return
set ::guard(beam_map_diab_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
# This must happen before other mappers are loaded.
::beam::set_mapper ::beam::compiler::map_diab_flags

# This mapper uses map_xlc_flags under the covers.
source beam_map_xlc_flags.tcl

#######################################################################
# Command line argument mapper for diab
#######################################################################

proc ::beam::compiler::map_diab_flags {argv} {
#
# Make a pass and see whether -I@ was defined. Build up NEW_ARGV which is
# the same as ARGV except that -I@ will not be included.
#
  set len [llength $argv]
  set new_argv {}
  set diab_qidirfirst 0
  for { set i 0 } { $i < $len } { incr i } {
    set arg [lindex $argv $i]
    if { [string match $arg "-I@"] } {
      set diab_qidirfirst 1
    } else {
      lappend new_argv $arg
    }
  }
#
# Invoke the command line mapper for xlc.
#
  if { $diab_qidirfirst == 1 } {
    return [map_xlc_flags [concat $new_argv "-D__packed__(x,y)=" "--edg=--gcc" \
            "-qidirfirst"]]
  }

  return [map_xlc_flags [concat $new_argv "-D__packed__(x,y)=" "--edg=--gcc"]]
}

