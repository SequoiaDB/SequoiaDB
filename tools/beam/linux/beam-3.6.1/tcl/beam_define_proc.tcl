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
#        Daniel Brand
#
#    DESCRIPTION:
#
#        This file collects proc definitions that are needed in general    
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.
#
#        02/25/02  brand    Disabled write_tlu until we work out how things 
#                           should be stored.
#        02/14/02  brand    Any var must be made global. 
#                           There is no point declaring encountered namespaces
#                           because that has no effect in a proc. Instead
#                           any user namespaces must be declared in his settings
#        01/14/02  brand    Created

# Guard against multiple sourcing
if { [::info exists ::guard(beam_define_proc)] } return
set ::guard(beam_define_proc) 1

source beam_util

#
# This will read in a file, and produce a list of lines from
# the file. This will ignore empty lines and lines starting
# with '#'
#

proc beam::read_file {file} {
  set list {}
  
  set data [::beam::util::ut_read_file $file]

  foreach line [split $data "\n"] {

    # Strip leading and trailing whitespace
    regsub {^[[:space:]]*}  $line {} line
    regsub  {[[:space:]]*$} $line {} line
    
    # Skip empty lines
    if { [regexp {^$} $line] } { continue }
    
    # Skip lines starting with '#'
    if { [regexp {^#} $line] } { continue }
    
    # Add to $line $list
    lappend list $line
  }

  return $list
}

#
# This will create a pattern from the items in list separated
# by the pattern string.
#

proc beam::make_pattern {pattern_string list} {
  set len [llength $list]
  
  if { $len <= 1 } {
    return $list
  }
  
  set res [list [lindex $list 0]]
  
  for { set i 1 } { $i < $len } { incr i } {
    lappend res $pattern_string
    lappend res [lindex $list $i]
  }

  return $res
}

#
# This will add the given option to $complaint_options
# (in the caller's scope)
#

proc beam::enable_option { option } {
  upvar complaint_options opts

  if { [::info exists opts] } {
    append opts " + $option"
  }
}

#
# This will remove the given option from $complaint_options
# (in the caller's scope)
#

proc beam::disable_option { option } {
  upvar complaint_options opts

  if { [::info exists opts] } {
    append opts " - $option"
  }  
}

#
# Create a tempfile and make sure it gets cleaned up later on.
# Also tell BEAM, that a temp file is being used. There can only be
# one temp file per process.
#
proc beam::get_temp_file { } {

  if { [::info exists ::beam::temp_counter] } {
    beam::error "Attempt to use more than one temp file"
  } else {
    set ::beam::temp_counter 1
  }

  set file "/tmp/my_temp_source.[pid].$::beam::temp_counter"

  ::beam::set_temp_file -file $file

  return $file
}

#
# This is a list of glob-style patterns that match BEAM complaint
# namespaces when matched to namespaces that are children of the
# ::beam namespace. These patterns should conform to ones used
# in Tcl's [string match] command.
#
set beam::complaint_namespaces {
  ::beam::ERROR*
  ::beam::WARNING*
  ::beam::MISTAKE*
  ::beam::PORTABILITY*
  ::beam::SECURITY*
  ::beam::SHARK*
  ::beam::MILLI*
  ::beam::JNI*
  ::beam::CONCUR*
}

#
# This will evaluate the block of code in every BEAM complaint
# namespace. A BEAM complaint namespace is a namespace in ::beam
# that matches any of the [string match] style patterns in the list
# $::beam::complaint_namespaces
#
proc beam::foreach_complaint { body } {

  # Loop over all ::beam namespaces
  set namespaces [::namespace children ::beam]
  
  foreach namespace $namespaces {
    # See if this namespace matches any pattern in our list
    foreach pattern $::beam::complaint_namespaces {
      if { [::string match $pattern $namespace] } {
        # Evaluate body in the namespace
        ::namespace eval $namespace $body
      }
    }
  }
}

#
# Load function attributes
#
proc beam::load_function_attributes {} {

# Guard against multiple sourcing
if { [::info exists ::guard(load_function_attributes)] } return
set ::guard(load_function_attributes) 1

# If --beam::nostdattr was given (or set to "yes") don't parse the 
# library-specific attributes.
  if { ! ([::info exists beam::nostdattr] && $beam::nostdattr == "yes") } {

    beam::gsource beam_attributes_def.tcl

    #################################################################
    # Load attributes for functions defined by the language standards
    #################################################################
    if { $::beam::lang == "c" } {

      beam::gsource beam_attributes_libc.tcl
      beam::gsource beam_attributes_libm.tcl
      beam::gsource beam_attributes_posix.tcl
  
    } elseif { $::beam::lang == "cpp" } {
  
      beam::gsource beam_attributes_libc.tcl
      beam::gsource beam_attributes_libm.tcl
      beam::gsource beam_attributes_posix.tcl
      beam::gsource beam_attributes_cpp.tcl
      beam::gsource beam_attributes_libc_cpp.tcl
      beam::gsource beam_attributes_libm_cpp.tcl

    } elseif { $::beam::lang == "pl8" } {
  
      beam::gsource beam_attributes_pl8.tcl
      beam::gsource beam_attributes_gcc.tcl

    } elseif { $::beam::lang == "java" } {

      beam::gsource beam_attributes_java.tcl

    } elseif { $::beam::lang == "mc" } {
  
      beam::gsource beam_attributes_mc.tcl

    }

    #################################################################
    # Load attributes for compiler builtin functions
    #################################################################
    if { [::info exists ::beam::cc] } {
      set attfile "${::install_dir}/tcl/beam_attributes_${::beam::cc}.tcl"
      if { [file exists "$attfile"] } {
        beam::gsource $attfile
      }
    }

    #################################################################
    # Load attributes for OS specific functions found in libc
    #################################################################
    set attfile "${::install_dir}/tcl/beam_attributes_libc_${::beam::target_os}.tcl"
    if { [file exists "$attfile"] } {
      beam::gsource $attfile
    }
  }
}

proc beam::display_namespace_vars { n } {
  foreach v [info vars ${n}::*] {  
    if { $v == "::beam::help_text" } { continue }
    if { $v == "::beam::complaint_namespaces" } { continue }
    if { $v == "::beam::compiler::java_config_var_list" } { continue }
    if { $v == "::beam::compiler::c_config_var_list" } { continue }
    if { [info exists $v] } {  
      if { [array size $v] == 0 } {  
# scalar variable
          puts "set $v \"[set $v]\""  
      } else {  
# array variable
          foreach z [array names $v] {
            if { [info exists ${v}(${z})] } {  
              puts "set ${v}(${z}) \"[set ${v}(${z})]\""
            }
          }
      }  
    }
  }  
}

proc beam::display_namespace { n } {
  if { $n == "::beam::attribute" } { return }
  if { $n == "::beam::options" } { return }
  if { $n == "::beam::dev" } { return }
  beam::display_namespace_vars $n
  foreach child [namespace children $n] {
    beam::display_namespace $child
  }
}

# This function gets called when --beam::display_parms is given.
proc beam::do_display_parms { } {
  beam::display_namespace ::beam
}
