# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2007-2010 IBM Corporation. All rights reserved.
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
#        This file contains utilities that beam_compile_utils.tcl
#        needs for finding Java dependencies.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

# Guard against multiple sourcing
if { [::info exists ::guard(beam_compile_java_utils)] } return
set ::guard(beam_compile_java_utils) 1

# Load some utils
source beam_util

namespace eval beam::compile::java {
  
  #
  # Pull in some utils
  #
  
  namespace import ::beam::util::*

  #
  # This contains patterns that match BEAM options that cause BEAM
  # to do no processing and exit early, one per line
  #  
  variable early_option_patterns {
    --beam-cout
    --beam-dot*
    --beam::list*
    --beam::dump*
    --beam::ipa_check
    --beam-java-deps
  }

  #
  # Return 1 if the flags would cause BEAM to exit early.
  # In these cases, there is no need to find dependent Java
  # files - we just want to pass the flags through as-is.
  #
  proc flags_exit_early { flags } {
    variable early_option_patterns
    
    foreach flag $flags {
      foreach pat $early_option_patterns {
        if { [string match $pat $flag] } {
          return 1
        }
      }
    }
    
    return 0
  }
  
  # 
  # Dissect the command line and find the java source files.
  # Add dependent source files to the end of both the command-line
  # and the description so they will be executed as the rest
  # of the files already listed would be.
  #
  # This emulates 'javac' by pulling in required files implicitly.
  #
  # Note that some sources may be removed if they have already
  # been analyzed, depending on the current Tcl settings.
  #
  proc bcuj_expand_implicit_source_files { flagsvar descvar } {
    upvar $flagsvar  flags
    upvar $descvar   desc

    set skip_deps    [ut_value beam::skip_dependencies]
    set skip_cmdline [ut_value beam::skip_cmdline_files_if_analyzed]
    
    if { ! [flags_exit_early $flags] } {
      if { $skip_deps != "yes" } {
        find_java_deps flags desc $skip_cmdline
      }
    }
  }

  proc find_java_deps { flagsvar descvar skip_cmdline } {
    # Take the arguments and description and add on
    # a list of all of the source files that should
    # also be analyzed.
    #
    # This is done by walking the source list given
    # on the command-line, asking Java BEAM to
    # decide which dependent files need to be
    # analyzed from it, and adding those to a
    # new list if they don't exist there already.
    #
    # Note that some sources may be removed if they
    # have already been analyzed, depending on the
    # current Tcl settings.
    #
    # Unless $skip_cmdline is "yes", files listed
    # on the command line are always analyzed.
    # This mimics javac.
    
    upvar $flagsvar flags
    upvar $descvar  desc

    # Remove the sources from the command-line
    set sources [remove_sources flags desc]
    
    # Add new sources back on to a new flags and desc
    set new_flags $flags
    set new_desc  $desc

    foreach source $sources {
      # Files on the command-line are added by default;
      # A BEAM option will skip them.

      if { $skip_cmdline != "yes" } {
        ut_file_cache file_cache $source {
          lappend new_flags $source
          lappend new_desc [list source java $source]
        }
      }

      # Run BEAM and list out the dependencies of this file.
      # This will come back with those files that don't have
      # .beam files yet.

      set dependent_sources [find_java_deps_single $flags $source errors]
      
      foreach dep $dependent_sources {
        ut_file_cache file_cache $dep {
          lappend new_flags $dep
          lappend new_desc [list source java $dep]
        }
      }

      # If there were errors looking for dependent files,
      # add the file to the command line regardless
      # of the value of $skip_cmdline. This is so the file
      # is re-run and the user sees the error.

      if { $errors } {
        ut_file_cache file_cache $source {
          lappend new_flags $source
          lappend new_desc [list source java $source]
        }
      }
    }
    
    set flags $new_flags
    set desc  $new_desc
  }

  #
  # Take the source files out of $flags and $desc and
  # return a list of the sources.
  #  
  proc remove_sources { flagsvar descvar } {
    upvar $flagsvar flags
    upvar $descvar  desc
    
    set new_flags {}
    set new_desc  {}
    set sources   {}

    ut_foreach_desc type rest pos $flags $desc {
      if { $type == "source" } {
        lappend sources [lindex $rest 1]
      } else {
        lappend new_flags [lindex $flags $pos]
        lappend new_desc  [lindex $desc $pos]
      }       
    }

    # Update caller's flags and desc
    
    set flags $new_flags
    set desc  $new_desc
    
    return $sources
  }
  
  proc find_java_deps_single { flags source errors_var } {
    # Run Java BEAM and get back a list of the Java files to
    # be analyzed.
    
    # Get the errors variable
    upvar $errors_var errors

    # Filter out all --beam-trace= options. We don't want any tracing
    # done during the dependent analysis step.
    
    set flags [ut_list_filter_out --beam-trace=* $flags]

    set         to_exec [list exec --]
    lappend     to_exec "${::install_dir}/bin/beam_java"
    ut_list_add to_exec $flags
    lappend     to_exec $source
    lappend     to_exec "--beam-java-deps"

    # We also want the parser_file to be stderr so we can
    # redirect stuff to /dev/null and ignore it

    lappend to_exec --beam::parser_file=stderr

    if { $::tcl_platform(platform) == "windows" } {
        lappend to_exec 2>nul
    } else {
        lappend to_exec 2>/dev/null
    }
    
    # Disable asserts - they print to stdout and return zero,
    # which will confuse us greatly. It's better to have no output
    # and assume no dependencies.

    if { [::info exists ::env(BEAM_ASSERT)] } {
      set old_assert $::env(BEAM_ASSERT)
      unset ::env(BEAM_ASSERT)
    }
    
    # Set the language to java and the source file
    set ::env(beam_language) java
    set ::env(beam_source_file) $source
    
    set deps {}

    if { [catch $to_exec output] } {
      # Unless BEAM was interrupted, ignore the error. The file
      # will be executed again in the normal path, and the error
      # will be shown to the user at that point. Just let the
      # caller know that the error happend by setting $errors.
      #
      # However, if BEAM was interrupted, we should die with an
      # interrupt too.
      
      if { [string match "child killed: interrupt*" $output] } {
        beam::interrupt
        # Not reached
      }
      
      set errors 1
    } else {
      set deps [split $output "\n"]
      set errors 0
    }
    
    # Restore BEAM_ASSERT if we saved it off
    
    if { [::info exists old_assert] } {
      set ::env(BEAM_ASSERT) $old_assert
    }

    return $deps
  }
  
  # Export our public utils
  namespace export bcuj_*
}
