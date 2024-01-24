# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2002-2010 IBM Corporation. All rights reserved.
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
#        This file is the main BEAM driver. It is responsible for
#        running the correct BEAM binary for each source file.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

####################################################################
# Load some helpers.
####################################################################

source beam_util
source beam_options
source beam_compile_utils
source beam_compile_java_utils
source beam_compiler_proc
source beam_prefixcc

####################################################################
# Some versions of glibc have bugs in the "locale" handling that cause
# valgrind to report invalid writes and make BEAM dump core. These errors 
# mysteriously disappear when LANG=C. The user impact is minimal, for 
# Americans anyway. So we set it here.
####################################################################

set env(LANG) C

####################################################################
# We use a namespace to keep things somewhat contained
####################################################################

namespace eval beam::compile {

  # Imports
  namespace import ::beam::util::*
  namespace import ::beam::options::*
  namespace import ::beam::compile::util::*
  namespace import ::beam::compile::java::*
  namespace import ::beam::prefixcc::*

  #
  # This is the main routine for the driver. This is called from C
  # once the C layer has been initialized.
  #
  proc main { argv } {
    # Parse command line and set up Tcl variables
    set argv [bo_parse_opts $argv 1]

    # Auto-configure the compiler, if requested
    if { [ut_value ::beam::prefixcc] != "" } {
      set argv [bpcc_configure $argv]
    }
    
    # Set up our libpath so we can run other BEAM executables.
    # This should be done after bpcc_configure above.
    bcu_set_libpath
    
    # Get the command-line description and remove deleted flags
    set desc [::beam::compiler::describe_flags $argv]
    bcu_remove_deleted argv desc
  
    # Sanity checks
    bcu_check argv desc

    # If all of the language files are Java, we need to expand
    # java dependencies here.

    if { $::beam::lang == "java" } {  
      bcuj_expand_implicit_source_files argv desc
    }

    # Invoke BEAM proper
    set rc [bcu_run_beam $argv $desc]
    return $rc
  }
}

