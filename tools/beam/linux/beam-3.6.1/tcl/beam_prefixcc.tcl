# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2008-2010 IBM Corporation. All rights reserved.
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
#        This file contains utilities used when handling --beam::prefixcc.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

# Guard against multiple sourcing
if { [::info exists ::guard(beam_prefixcc)] } return
set ::guard(beam_prefixcc) 1

# Load some helpers
source beam_util
source beam_compiler_proc

namespace eval beam::prefixcc {
  
  # Import helpers
  namespace import ::beam::util::*
  
  #
  # Scan the command-line looking for the --beam::prefixcc binary. It is the
  # first non-BEAM option. Note there is no special handling for @file
  # arguments - even though this is called before @file arguments are expanded,
  # we still expect (and require) that the compiler binary is positioned
  # earlier in the argument list than the @file arguments, which is how the
  # compiler would have been invoked natively.
  #
  # Remove and return the binary, modifying argv in place to no longer include it.
  # If one was not given, return the empty string.
  #
  proc bpcc_find_binary { argv_out } {
    upvar $argv_out argv
   
    set len [llength $argv]
    for { set i 0 } { $i < $len } { incr i } {
      set arg [lindex $argv $i]
      switch -glob -- $arg {
        --edg*  -
        --beam* { ;# Skip }
        
        default {
          set argv [ut_list_remove $argv $i]
          return $arg
        }
      }
    }
   
    return "" 
  }
  
  #
  # Given a binary, see if it matches one of our patterns for supported compilers.
  # If so, return the BEAM-ish name of the supported compiler, suitable for
  # ::beam::cc. If not, return the empty string.
  #
  # NOTE: If you change this, update main/trace_dispatch/main_trace_dispatch.c
  #       and the beam_trace docs, if the new compiler is to be automatically
  #       handled by beam_trace.
  #
  proc bpcc_identify { binary } {
    set base [lindex [split $binary $::beam::dirsepstr] end]
    
    switch -glob -- $base {
      gcc* -
      g++* { return "gcc" }
      
      cl     -
      cl.exe { return "msvc" }
      
      xlc*   -
      xlC*   { return "vac" }
      
      javac { return "javac" }
    }

    return ""
  }
  
  #
  # Given the current command-line, look for an existing configuration for it in beam::data.
  # If one is not found, generate one with beam_configure.
  #
  # This appends the appropriate --beam::compiler= options to the command-line and
  # returns a new command-line.
  #
  # This must happen after command-line processing is done. It relies on beam::data, and it
  # will be using mapper routines. It will also load a compiler configuration file.
  #
  proc bpcc_configure { argv } {
    # Find out where to store the files. If beam::data isn't given, use ~/.beam.
    
    if { [ut_value ::beam::data] == "" } {
      set dir "$::env(HOME)/.beam"
    } else {
      set dir $::beam::data
    }

    # Set up beam_configure and the compiler
    
    set config_flags [beam::compiler::config_flags $argv]
    set name $::beam::prefixcc
    set binary [beam::findprog $name]
    
    if { $binary == "" } {
      beam::error "Can't find '$name' in PATH."
    }

    # Configure all languages we were invoked for. Note: If we see
    # the asm language, we ignore it. It doesn't need a configuration.
    
    foreach lang [array names ::beam::langs] {
      # Get the right option for beam_configure
      switch -exact -- $lang {
        asm  { continue }
        c    { set config_lang --c }
        cpp  { set config_lang --cpp }
        java { set config_lang --java }
        
        default {
          beam::error "The --beam::prefixcc option is not supported" \
                      "for the '$lang' language."
        }
      }

      # Set up information for the compiler hash      
      set ident [list "Compiler MD5: [beam::md5sum_file $binary]"]
      lappend ident "Language: $lang"
      lappend ident "Args:"
      ut_list_add ident $config_flags
      
      set hash [beam::md5sum [join $ident "\n"]]

      set lock "$dir/cconf/lock-file"    
      set file "$dir/cconf/${::beam::cc}-${lang}-${hash}.tcl"

      beam::with_lock $lock {      
        if { ! [ut_file_exists $file] } {        
          # Set up the command to run
          set cmd [list "${::install_dir}/bin/beam_configure"]
          lappend cmd $config_lang $name -o $file
          lappend cmd "--"
          ut_list_add cmd $config_flags
                            
          if { $::tcl_platform(platform) == "windows" } {
            ut_list_add_front cmd "perl"
          }
      
          puts "Configuring $name..."

          set rc [beam::system $cmd status]
        
          if { $rc != 0 } {
            if { $status == "EINT" } { beam::interrupt }
            beam::error "Configuration for '$name' failed (rc = $rc)."
          }
        }
      }

      # Load up the config file here (so we can check beam::cc). Also add
      # it to the command-line so beam_lang executables load it.
      
      lappend argv "--beam::compiler=$file"
      beam::gsource $file
    }

    # Verify beam::cc and compiler config::cc agree.
    ::beam::compiler::check_compiler_config

    return $argv 
  }

  # Export our public utils
  namespace export bpcc_*
}
