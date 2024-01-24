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
#        This script defines the default BEAM mapper and describer.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_default_flags)] } return
set ::guard(beam_map_default_flags) 1

# Load utils.
source beam_util
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper     ::beam::mapper::default::map_default_flags
::beam::set_describer  ::beam::mapper::default::describe_default_flags
::beam::set_expander   ::beam::mapper::default::expand_default_flags
::beam::set_configurer ::beam::mapper::default::config_default_flags

namespace eval ::beam::mapper::default {
  
  namespace import ::beam::util::*

  #######################################################################
  # BEAM default command line argument describer
  #######################################################################

  proc describe_default_flags {flags} {
    # If this looks like Java, we use the default Java describer,
    # so that it will match up with the default Java mapper used
    # below in map_default_flags.

    if { [lsearch -glob $flags *.java] != -1 } {
      return [describe_java_flags $flags]
    }
    
    # Otherwise, assume C/C++ format
    return [describe_c_cpp_flags $flags]
  }

  #######################################################################
  # BEAM C/C++ default command line argument describer
  #######################################################################
  
  proc describe_c_cpp_flags {flags} {
    # Simply describe the command-line in a dumb way. We try
    # to support basics from all of BEAM's supported languages.
    #
    # The first argument is not the binary; it is the first
    # command-line flag.

    set desc {}
  
    set len [llength $flags]
  
    for { set i 0 } { $i < $len } { incr i } {
      set flag [lindex $flags $i]
    
      switch -glob -- $flag {

        -f        -
        -t        -
        -include  -
        -imacros  -
        -e        -
        -Z        -
        -L        { 
                    incr i
                    lappend desc {ignore} {ignore}
                  }

        -d        -        
        -o        -
        -D        -
        -U        -
        -I        { lappend desc [list cmdline $flag]
                    incr i
                    lappend desc [list cmdline [lindex $flags $i]]
                  }

        -E        -
        -o*       -
        -D*       -
        -U*       -
        -I*       { lappend desc [list cmdline $flag] }
        
        --gcc=*   { lappend desc { warning "--gcc is not recognized" } }
      
        --edg=*   { lappend desc [concat {cmdline} [ut_unescape_arg [string range $flag 6 end]]] }

        -*  { lappend desc {ignore} }

        *.c { lappend desc [list source c $flag] }
      
        *.C   -
        *.cpp -
        *.CPP -
        *.cxx -
        *.cc  { lappend desc [list source cpp $flag] }
      
        *.pl8 -
        *.ipl { lappend desc [list source pl8 $flag] }

        *.s -
        *.S { lappend desc [list source asm $flag] }
      
        *.lst { lappend desc [list source mc $flag] }

        default   { lappend desc {ignore} }
      }
    }

    return $desc
  }

  #######################################################################
  # BEAM Java default command line argument describer
  #######################################################################
  
  proc describe_java_flags {flags} {
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
  
  #######################################################################
  # BEAM default command line argument mapper
  #######################################################################

  proc map_default_flags {flags} {
    switch -exact -- $::beam::lang {
      c   -
      cpp -
      asm {
        return [map_c_cpp_flags $flags]
      }
        
      java {
        # Use map_javac_flags as the default for now, since most java compilers
        # seem fairly standard
        return [map_java_flags $flags]
      }
         
      default {
        ::beam::internal_error \
          "Default map flags called with bad language $::beam::lang"
      }
    }
  }

  #######################################################################
  # BEAM C/C++ default command line argument mapper
  #######################################################################

  proc map_c_cpp_flags {flags} {

    # The var new_flags will hold the new command line, which will
    # be returned.
  
    set new_flags {}
    
    # Get the description
    
    set desc [describe_default_flags $flags]
  
    # Loop over them both, translating things into $new_flags and other
    # configuration settings.
    
    ut_foreach_desc type rest pos $flags $desc {
      switch -exact -- $type {
        source { lappend new_flags [lindex $rest 1] }
        cmdline { ut_list_add new_flags $rest }
        warning { beam::warning [lindex $rest 1] }
        default { # ignore }
      }
    }

    # This list of arguments is what EDG will use  
    return $new_flags  
  }
  
  #######################################################################
  # BEAM Java default command line argument mapper
  #######################################################################
  
  proc map_java_flags { flags } {
    ::beam::compiler::import
    
    set new_flags {}

    set source_file ""
    
    # Get the description
    set desc [describe_java_flags $flags]

    # Walk them both, setting up new_flags
    ut_foreach_desc type rest pos $flags $desc {
      switch -exact -- $type {
        
        source { set source_file [lindex $rest 1] }
        
        config_set { set [lindex $rest 0] [lindex $rest 1] }
        
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
  
  #######################################################################
  # BEAM default command line expander
  #######################################################################
  
  proc expand_default_flags { flags } {
    # By default, we don't add any environment variables
    
    # By default, we expand @file arguments normally.
    return [ut_expand_atfiles $flags]
  }

  #######################################################################
  # BEAM default command line configurer
  #######################################################################
  
  proc config_default_flags { flags } {
    # By default, we don't pass any command-line flags through to beam_configure
    return {}
  }
}
