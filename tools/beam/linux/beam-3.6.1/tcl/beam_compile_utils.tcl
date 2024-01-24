# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 1996-2010 IBM Corporation. All rights reserved.
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
#        Francis Wallingford and Daniel Brand
#
#    DESCRIPTION:
#
#        This file contains utilities that beam_compile.tcl uses.
#        It was carved out of shell scripts.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

# Guard against multiple sourcing
if { [::info exists ::guard(beam_compile_utils)] } return
set ::guard(beam_compile_utils) 1

# Load some utils
source beam_util
source beam_compiler_proc

namespace eval beam::compile::util {

  #
  # Pull in some utils
  #

  namespace import ::beam::util::*

  #
  # Variables
  #
  
  # Which proc to use for a given language
  variable runner
  
  set runner(c)    run_beam_cc
  set runner(cpp)  run_beam_cc
  set runner(asm)  run_beam_cc
  set runner(pl8)  run_beam_pl8
  set runner(java) run_beam_java
  set runner(mc)   run_beam_mc

  #
  # Set up the environment to find our shared libraries.
  #
  # This may be called more than once. Only the first call sets up
  # the environment.
  #
  # This should be done as late as possible because this libpath
  # setting is only meant to affect the BEAM binaries. Running
  # other programs while this is set (like beam_configure) may
  # cause those other programs to behave incorrectly.
  #
  proc bcu_set_libpath {} {
    if { [::info exists ::beam::compile::util::libpath_is_set] } { return }
    set ::beam::compile::util::libpath_is_set 1
    
    set plat [string tolower $::tcl_platform(os)]
  
    if { $plat == "aix" } {
      set env_name LIBPATH
    } elseif { $plat == "linux" } {
      set env_name LD_LIBRARY_PATH
    } elseif { [string match "windows*" $plat] } {
      set env_name PATH
    } else {
      return ;# Nothing to do
    }
  
    set path "${::install_dir}/dll"

    if { [::info exists ::env($env_name)] } {
      append path $::beam::pathsep [set ::env($env_name)]
    }

    set ::env($env_name) $path
  }
  
  #
  # Check the Tcl settings and the arguments to ensure BEAM
  # will run. They may be modified if needed.
  #
  proc bcu_check { flagsvar descvar } {
    upvar $flagsvar  flags
    upvar $descvar   desc

    # If ipa check is needed, there should be no source files.
    # We will add a dummy one ourselves to please BEAM.
    
    # If ipa check is not needed, there should be at least one
    # source file.

    set first_source_pos -1
    
    ut_foreach_desc type rest pos $flags $desc {
      if { $type != "source" } { continue }
        
      if { $first_source_pos == -1 } {
        set first_source_pos $pos
        break
      }
    }

    if { [ut_value beam::ipa_check] == "yes" } {
      if { $first_source_pos != -1 } {
        beam::warning "--beam::ipa_check does not require input files. They will be ignored."
        remove_other_sources flags desc $first_source_pos
      }
      
      lappend flags beam_dummy_file.c
      lappend desc { source c }
    } else {
      if { $first_source_pos == -1 } {
        beam::usage "No input files were found to analyze"
      }      
    }
  }    

  #
  # This function invokes the BEAM executable once for
  # each language file on the command-line as given by
  # the description.
  #
  # If beam::dev::debugger is set, the debugger binary
  # is executed instead with the BEAM command-line as an
  # option.
  #
  # If beam::dev::display_analyzed_files is set, each file
  # is displayed to the user before BEAM is actually executed
  # on the file.
  #
  proc bcu_run_beam { flags desc } {
    variable runner
    
    set final_rc 0

    # Walk the sources and decide how to run BEAM

    ut_foreach_desc type rest pos $flags $desc {
      if { $type != "source" } { continue }
      
      set lang [lindex $rest 0]
      set file [lindex $rest 1]
      
      set new_flags $flags
      set new_desc  $desc
     
      remove_other_sources new_flags new_desc $pos
    
      display_file $file

      # Prep the per-file environment variables
      
      set ::env(beam_language) $lang
      set ::env(beam_source_file) $file

      # Run BEAM language executable

      set rc [$runner($lang) $lang $file $new_flags]
      
      if { $rc != 0 } { set final_rc 1 }        
    }
    
    return $final_rc
  }

  #
  # Return a new argument list with all "delete" flags
  # removed from both the flags and the description.
  #
  proc bcu_remove_deleted { flagsvar descvar } {
    upvar $flagsvar flags
    upvar $descvar  desc
    
    set new_flags {}
    set new_desc {}

    ut_foreach_desc type rest pos $flags $desc {
      if { $type == "delete" } { continue }
      
      lappend new_flags [lindex $flags $pos]
      lappend new_desc  [lindex $desc $pos]
    }
    
    set flags $new_flags
    set desc  $new_desc
  }
  
  #
  # Return a new argument list with all source files except
  # for the one at $pos removed
  #
  proc remove_other_sources { flagsvar descvar keep } {
    upvar $flagsvar flags
    upvar $descvar  desc
    
    set new_flags {}
    set new_desc {}

    ut_foreach_desc type rest pos $flags $desc {
      if { $type == "source" && $pos != $keep } {
        continue
      }
      
      lappend new_flags [lindex $flags $pos]
      lappend new_desc  [lindex $desc $pos]
    }
    
    set flags $new_flags
    set desc  $new_desc
  }
    
  proc run_beam_cc { lang file flags } {
    set cmd ${::install_dir}/bin/beam_new_cc${::beam::binext}
    
    return [run_beam_default $lang $file $cmd $flags]
  }
  
  proc run_beam_java { lang file flags } {
    set cmd ${::install_dir}/bin/beam_java${::beam::binext}

    return [run_beam_default $lang $file $cmd $flags]
  }
  
  proc run_beam_mc { lang file flags } {
    set cmd ${::install_dir}/bin/beam_mc${::beam::binext}
    
    return [run_beam_default $lang $file $cmd $flags]
  }
  
  proc run_beam_pl8 { lang file flags } {
    set cmd ${::install_dir}/bin/beam_gcc_pl8${::beam::binext}

    ut_list_add_front flags { -c --beam }
    ut_list_push flags $cmd
    
    ####################################################################
    # GCC Fix: GCC tries to access files in afs because that was its
    # original install point. The sources have been changed to no
    # longer access files in afs if the following environment variables
    # are set. Note: this must come after the language is known because
    # it only runs for pl8.
    ####################################################################

    set gccdir "${::install_dir}/gpl8"

    set ::env(GCC_EXEC_PREFIX) "${gccdir}/libexec/gcc/"

    # Set up some environment variables for beam_pl8c.sh
    # so it can find bin/beam_pl8c and run it under the
    # right debugger.

    set ::env(beam_bindir)   "${::install_dir}/bin"
    set ::env(beam_debugger) [ut_value beam::dev::debugger]

    return [run $lang $file $cmd $flags]
  }

  #
  # Add the debugger if it is set, and add the command.
  # run and return the status code.
  #  
  proc run_beam_default { lang file cmd flags } {
    set binary $cmd
    ut_list_push flags $cmd

    set debugger [ut_value beam::dev::debugger]
    
    if { $debugger != "" } {
      ut_list_add_front flags [ut_unescape_arg $debugger]
    }
    
    return [run $lang $file $binary $flags]
  }
  
  #
  # Run the command. On interrupt, interrupt ourselves. Return
  # the return code.
  #
  # If the $binary does not exist, print a warning about the
  # language ($lang) for the $file being unsupported on this
  # platform.
  #
  # Only the $cmd list is executed - the rest are used for
  # diagnostics.
  #
  # $binary must be a full path to an executable.
  #
  proc run { lang file binary argv } {
    set status ""

    if { [file exists $binary] } {
      # Run
      set rc [beam::system $argv status]
      
      if { $status == "EINT" } {
        beam::interrupt
        # Not reached
      }
    } else {
      beam::warning "BEAM for $lang does not appear to be supported" \
                    "on this platform. `$file' will not be analyzed."
      set rc 0
    }
    
    return $rc
  }
  
  proc display_file { file } {
    if { [ut_value beam::display_analyzed_files] == "yes" } {
      if { [file exists $file] } {
        set file [beam::abspath $file]
      }

      puts stderr "BEAM: Analyzing `$file'"
    }
  }
  
  # Export our public utils
  namespace export bcu_*
}
