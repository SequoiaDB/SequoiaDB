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
#        Francis Wallingford
#
#    DESCRIPTION:
#
#        This script defines '::beam::compiler::map_none_flags'.
#        This is used with --beam::cc=none or "set cc none" in a
#        compiler config file.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_none_flags)] } return
set ::guard(beam_map_none_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper     ::beam::compiler::map_none_flags
::beam::set_expander   ::beam::compiler::expand_none_flags
::beam::set_configurer ::beam::compiler::config_none_flags

#######################################################################
# Simple routines for passing through arguments
#######################################################################

proc ::beam::compiler::map_none_flags {argv} {
  return $argv
}

proc ::beam::compiler::expand_none_flags {argv} {
  return $argv
}

proc ::beam::compiler::config_none_flags {argv} {
  return {}
}
