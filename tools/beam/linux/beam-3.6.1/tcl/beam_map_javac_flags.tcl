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
#        This script set up the describer and mapper for javac-like
#        compilers.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_javac_flags)] } return
set ::guard(beam_map_javac_flags) 1

# Load utils.
source beam_util
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper    ::beam::mapper::javac::map_javac_flags
::beam::set_describer ::beam::mapper::javac::describe_javac_flags

namespace eval ::beam::mapper::javac {

  namespace import ::beam::util::*
  
  #
  # Describe the command-line. The beam_compile logic only needs "source"
  # descriptions; the rest of the elements are for map_javac_flags below.
  # The first argument is not the binary; it is the first command-line flag.
  #

  proc describe_javac_flags {flags} {
    
    # This will be the description
    set desc {}
    
    # Walk the command-line
    set len [llength $flags]
    
    for { set i 0 } { $i < $len } { incr i } {
      set flag [lindex $flags $i]
     
      switch -glob -- $flag {
        -d {
          lappend desc {ignore}
          incr i
          set output_dir [lindex $flags $i]
          lappend desc [list config_set system_output_directory $output_dir]
        }
      
        -cp        -
        -classpath {
          lappend desc {ignore}
          incr i
          set classpath [split [lindex $flags $i] $::beam::pathsep]
          lappend desc [list config_set system_classpath $classpath]
        }
      
        -encoding {
          lappend desc {ignore}
          incr i
          set encoding [lindex $flags $i]
          lappend desc [list config_set language_encoding $encoding]
        }
      
        -source {
          lappend desc {ignore}
          incr i
          set source_level [lindex $flags $i]
          lappend desc [list config_set language_source_level $source_level]
        }

        -sourcepath {
          lappend desc {ignore}
          incr i
          set sourcepath [split [lindex $flags $i] $::beam::pathsep]
          lappend desc [list config_set system_sourcepath $sourcepath]
        }

        -bootclasspath {
          lappend desc {ignore}
          incr i
          set classpath [split [lindex $flags $i] $::beam::pathsep]
          lappend desc [list config_set system_boot_classpath $classpath]
        }

        -Xbootclasspath:* {
          regsub -- ^-Xbootclasspath: $flag "" tmp
          set classpath [split $tmp $::beam::pathsep]
          lappend desc [list config_set system_boot_classpath $classpath]
        }
      
        -Xbootclasspath/a:* {
          regsub -- ^-Xbootclasspath/a: $flag "" tmp
          set classpath [split $tmp $::beam::pathsep]
          lappend desc [list boot_classpath_append $classpath]
        }

        -Xbootclasspath/p:* {
          regsub -- ^-Xbootclasspath/p: $flag "" tmp
          set classpath [split $tmp $::beam::pathsep]
          lappend desc [list boot_classpath_prepend $classpath]
        }

        -extdirs {
          lappend desc {ignore}
          incr i
          set ext_dirs [split [lindex $flags $i] $::beam::pathsep]
          lappend desc [list config_set system_ext_dirs $ext_dirs]
        }
      
        -Djava.ext.dirs=* {
          regsub -- ^-Djava.ext.dirs= $flag "" tmp
          set ext_dirs [split $tmp $::beam::pathsep]          
          lappend desc [list config_set system_ext_dirs $ext_dirs]
        }
      
        -endorseddirs {
          lappend desc {ignore}
          incr i
          set endorsed_dirs [split [lindex $flags $i] $::beam::pathsep]
          lappend desc [list config_set system_endorsed_dirs $endorsed_dirs]
        }

        -Djava.endorsed.dirs=* {
          regsub -- ^-Djava.endorsed.dirs= $flag "" tmp
          set endorsed_dirs [split $tmp $::beam::pathsep]
          lappend desc [list config_set system_endorsed_dirs $endorsed_dirs]
        }
      
        -Xmaxerrors -
        -Xmaxwarns  -
        -Xstdout    -
        -target {
          # Skip
          incr i
          lappend desc {ignore} {ignore}
        }
      
        --edg=* {
          set tmp [ut_unescape_arg [string range $flag 6 end]]
          lappend desc [list cmdline $tmp]
        }

        *.java {
          lappend desc [list source java $flag]
        }

        default {
          # Skip
          lappend desc {ignore}
        }
      } 
    }

    return $desc
  }    

  #
  # Translate javac flags to BEAM's compiler configuration
  # and EDG flags. Return the new command-line.
  #
  # Note that the single .java file passed in from the driver
  # is moved to the end of the command-line, since that
  # is where ji2a_fe_run_frontend expects it.
  # FIXME: This could be done better than moving it to the end.
  #
  
  proc map_javac_flags { flags } {
    ::beam::compiler::import
    
    set new_flags {}

    set source_file ""
    
    # Get the description
    set desc [describe_javac_flags $flags]

    # Walk them both, setting up new_flags
    ut_foreach_desc type rest pos $flags $desc {
      switch -exact -- $type {
        
        source { set source_file [lindex $rest 1] }
        
        config_set { set [lindex $rest 0] [lindex $rest 1] }
        
        boot_classpath_append {
          set classpath [lindex $rest 0]
          ut_list_add system_boot_classpath $classpath
        }
        
        boot_classpath_prepend {
          set classpath [lindex $rest 0]
          ut_list_add_front system_boot_classpath $classpath
        }
    
        cmdline {
          set args [lindex $rest 0]
          ut_list_add new_flags $args
        }
                
        default { # ignore }
      }
    }
      
    # Append the source file
    lappend new_flags $source_file
    
    return $new_flags
  }
}
