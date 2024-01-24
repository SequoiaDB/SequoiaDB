############################################################
# Writing custom describe_flags and map_flags procedures
############################################################

###
### This file provides example Tcl procedures to
### both describe and translate command-line flags
### from native compiler options like "-isystem <dir>"
### to BEAM options.
###
### BEAM comes with a few Tcl procedures pre-defined for
### common compilers like gcc and vac. They live in
### tcl/beam_map_*_flags.tcl, which are loaded on-demand.
###
### This file serves as a simple template. Consider looking at
### tcl/beam_map_*_flags.tcl for additional ideas.
###
### For more information, refer to the documentation in
### the install point:
###
###   install_point/www/argument_mapper.html

source beam_util
source beam_compiler_proc


# Set these as the default procedures unless some
# are loaded already to override them.
#
# This should always come first.

::beam::set_describer  ::example::describe_flags
::beam::set_mapper     ::example::map_flags
::beam::set_expander   ::example::expand_flags
::beam::set_configurer ::example::config_flags

# A namespace to keep things clean

namespace eval ::example {
  
  # Bring in utilities
  
  namespace import ::beam::util::*
  

  # Here is the argument describer. The name here
  # should match the name used in set_mapper above.

  proc describe_flags { flags } {
    # Store the return list here
    set desc {}

    # Walk the flags
    set len [llength $flags]
    
    for { set i 0 } { $i < $len } { incr i } {
      set flag [lindex $flags $i]
      
      switch -glob -- $flag {
        -fsigned-chars { lappend desc { config_set target_plain_char_is_unsigned 0 } }
        
        -l             {
                         # Skip two opts
                         lappend desc {ignore}
                         incr i
                         lappend desc {ignore}
                       }

        -l*            {
                         # Skip this opt
                         lappend desc {ignore}
                       }
                       
        --edg=* { lappend desc [concat {flags} [ut_unescape_arg [string range $flag 6 end]]] }
        
        -* { lappend desc {ignore} }

        *.c   { lappend desc [list source c $flag] }

        *.cpp -
        *.cxx -
        *.C   { lappend desc [list source cpp $flag] }

        default {
                  # Skip the rest
                  lappend desc {ignore}
                }

      }
    }

    return $desc
  }
  
  
  # Here is the main mapper. The name here should match
  # the name used in ::beam::set_mapper above.

  proc map_flags { flags } {

    # This call imports all of the configuration
    # variables into this procedure so that they
    # can be set and read without being fully
    # qualified.
    
    ::beam::compiler::import
    
    # The var new_flags will hold the new command line, which will
    # be returned.
    
    set new_flags {}
    
    # Describe the flags and use that to set up the compiler
    # configuration namespace and the new_flags list.
    
    set desc [describe_flags $flags]
    
    ut_foreach_desc type rest pos $flags $desc {
      
      switch -exact -- $type {
        
        source { lappend new_flags [my_massage_file [lindex $rest 1]] }
        
        flags { ut_list_add new_flags $rest }
        
        config_set { set [lindex $rest 0] [lindex $rest 1] }
        
        default { # Ignore }
      }
    }
    
    # This list of arguments is what EDG will use
    return $new_flags
  }

  
  # Here is the main expander. The name here should match
  # the name used in ::beam::set_expander above.

  proc expand_flags { flags } {
    # Here you can add things from the environment to $flags.
    # You should also do all @file expansion here if your
    # compiler supports it or you want BEAM to support it.
    
    return [ut_expand_atfiles $flags]
  }


  # Here is the main configurer. The name here should match
  # the name used in ::beam::set_configurer above.

  proc config_flags { flags } {
    # You should return a list of those flags that should be
    # passed through to beam_configure. These are the flags
    # that, when given to your compiler, change the compiler
    # configuration enough so that BEAM should re-configure
    # the compiler.
    
    return {}
  }

  
  ############################################################
  # Massaging the input file
  ############################################################
  
  ### Here is an example of a routine that you can use if you
  ### need to actually modify the source file before compiling
  ### it with BEAM. This should be called from your custom
  ### map_flags whenever you see a source file name (see the
  ### example map_flags above).
  ###
  ### This routine takes the original source filename as
  ### an argument. If you are not changing the file in-place,
  ### use beam::get_temp_file to obtain a temporary file. The temp
  ### file will be deleted at the end of the BEAM run.
  ### Independent of whether a temp file is used or not the function
  ### must return its argument (the original file name).
  ###
  ### Make sure you use the same name here and when you
  ### call it (above, in map_flags).

  proc my_massage_file {filename} {

    set temp_file [::beam::get_temp_file]

    set reader [open $filename "r"]
    set writer [open $temp_file "w"]

    # Process $filename, writing $temp_file

    set file_contents [read $reader]
    close $reader

    regsub -all -- {unknown_code;} $file_contents {(void)0;} file_contents

    puts $writer $file_contents
    close $writer
    
    return $filename

    ### Instead of the above, maybe you want to use an external program:

    #  set temp_file [::beam::get_temp_file]
    #
    #  exec -- sed s/a/b/g $filename > $temp_file
    #
    #  # Return the the original $filename.
    #
    #  return $filename
  }

  
  # End of the ::example namespace 
}
