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
#        This script defines '::beam::compiler::map_gcc_flags'.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_gcc_flags)] } return
set ::guard(beam_map_gcc_flags) 1

# Load utils.
source beam_compiler_proc

# Set up routines.
::beam::set_describer  ::beam::compiler::describe_gcc_flags
::beam::set_mapper     ::beam::compiler::map_gcc_flags
::beam::set_configurer ::beam::compiler::config_gcc_flags

#######################################################################
# Command line describer for GCC compilers
#######################################################################

proc ::beam::compiler::describe_gcc_flags {argv} {
  # Return a list of flags and sources for the driver.
  
  set desc {}
  set forced_lang ""
  
  set len [llength $argv]
  for { set i 0 } { $i < $len } { incr i } {
    set arg [lindex $argv $i]
   
    switch -glob -- $arg {
      -x {
        # Switch language
        incr i
        set forced_lang [lindex $argv $i]
        if { $forced_lang == "c++" } { set forced_lang cpp }
        lappend desc { ignore } { ignore }
      }
      
      -isystem -
      -idirafter -
      -include -
      -imacros -
      -iprefix -
      -iwithprefix -
      -iwithprefixbefore -
      -A -
      -B -
      -L -
      -aux-info -
      -G -
      -l -
      -o -
      -D -
      -U -
      -I {
        # Two arguments
        incr i
        lappend desc { ignore } { ignore }
      }

      -* {
        # One argument
        lappend desc { ignore }
      }
      
      *.s -
      *.S {
        # ASM source, unless forced
        if { $forced_lang != "" } {
          lappend desc [list source $forced_lang $arg]
        } else {
          lappend desc [list source asm $arg]
        }
      }
        
      *.c {
        # C source, unless forced
        if { $forced_lang != "" } {
          lappend desc [list source $forced_lang $arg]
        } else {
          lappend desc [list source c $arg]
        }
      }
      
      *.C   -
      *.cpp -
      *.cxx -
      *.cc  {
        # C++ source, unless forced
        if { $forced_lang != "" } {
          lappend desc [list source $forced_lang $arg]
        } else {
          lappend desc [list source cpp $arg]
        }
      }
      
      default {
        # Ignore the rest
        lappend desc { ignore }
      }
    } 
  }
  
  return $desc
}

#######################################################################
# Command line argument mapper for GCC compilers
#######################################################################

proc ::beam::compiler::map_gcc_flags {argv} {

  # Get the variables and helper routines
  
  ::beam::compiler::import

  # Save off the original gnu_mode
  set orig_gnu_mode 0
  
  if { [::info exists gnu_mode] } {
    set orig_gnu_mode $gnu_mode
  }
  
  # The var new_argv will hold the new command line, which will
  # be returned.

  set orig_argv $argv  
  set new_argv {}
  
  # All -isystem dirs will be prepended to system_include_path.
  # This variable will gather them up.
  
  set isystem_dirs {}
  
  # -iprefix sets this prefix. Note: GCC defaults this to be the
  # install dir, but we don't know it. We will default it to be
  # "/" arbitrarily.
  
  set iprefix "/"
  
  # Loop over $argv, translating things into $new_argv and other
  # configuration settings.

  set len [llength $argv]

  for { set i 0 } { $i < $len } { incr i } {

    set arg [lindex $argv $i]
    
    # The right hand side of each pair is a code block.
    # A single hyphen means fall-through to the next case.

    switch -glob -- $arg {

      -fwritable-strings      { set target_string_literals_are_readonly 0 }
      -fno-writable-strings   { set target_string_literals_are_readonly 1 }

      -fno-unsigned-char*  -
      -fsigned-char*       { set target_plain_char_is_unsigned 0 }
      
      -fno-signed-char*    -
      -funsigned-char*     { set target_plain_char_is_unsigned 1 }
      
      -fno-unsigned-bitfields -
      -fsigned-bitfields      { set target_plain_int_bit_field_is_unsigned 0 }
      
      -fno-signed-bitfields   -
      -funsigned-bitfields    { set target_plain_int_bit_field_is_unsigned 1 }

      -fconst-strings      { set language_string_literals_are_const 1 }
      -fno-const-strings   { set language_string_literals_are_const 0 }
      
      -fshort-enums     { set target_enum_types_can_be_smaller_than_int 1 }
      -fno-short-enums  { set target_enum_types_can_be_smaller_than_int 0 }

      -ffor-scope    { set language_use_nonstandard_for_init_scope 0 }
      -fno-for-scope { set language_use_nonstandard_for_init_scope 1 }

      -fdollars-in-identifiers    { set language_allow_dollar_in_id_chars 1 }
      -fno-dollars-in-identifiers { set language_allow_dollar_in_id_chars 0 }
      
      -nostdinc { set system_include_path {} }            

      
      -m32 {
             # FIXME: For now, 32 vs 64 bit is handled via different configs.
             #        Warn if it looks like we are using a 64-bit config and
             #        the user tries -m32.
        
             if { $target_sizeof_long != 4 } {
               beam::warning "Option -m32 is ignored; Use a separate config file for 32-bit"
             }
           }
      
      -m64 {
             # FIXME: For now, 32 vs 64 bit is handled via different configs.
             #        Warn if it looks like we are using a 64-bit config and
             #        the user tries -m32.
        
             if { $target_sizeof_long == 4 } {
               beam::warning "Option -m64 is ignored; Use a separate config file for 64-bit"
             }
           }

           
      -isystem  {
                  incr i
                  lappend isystem_dirs [lindex $argv $i]
                }
                
      -isystem* { lappend isystem_dirs [string range $arg 8 end] }
                
      -idirafter  {
                    incr i
                    lappend system_include_path [lindex $argv $i]
                  }

      -idirafter* { lappend system_include_path [string range $arg 10 end] }

      -include  {
                  lappend new_argv {--preinclude}
                  incr i
                  lappend new_argv [lindex $argv $i]
                }

      -include* { 
                  lappend new_argv {--preinclude}
                  lappend new_argv [string range $arg 8 end] 
                }

      -imacros  {
                  lappend new_argv {--preinclude_macros}
                  incr i
                  lappend new_argv [lindex $argv $i]
                }
               
      -imacros* {
                  lappend new_argv {--preinclude_macros}
                  lappend new_argv [string range $arg 8 end] 
                }
               
      -iprefix  { 
                  incr i
                  set iprefix [lindex $argv $i]
                }
      
      -iprefix* { set iprefix [list [string range $arg 8 end]] }
      
      -iwithprefix {
                     incr i
                     set dir $iprefix
                     append dir [lindex $argv $i]
                     lappend system_include_path $dir
                   }
               
      -iwithprefixbefore {
                           incr i
                           set dir $iprefix
                           append dir [lindex $argv $i]
                           lappend new_argv {-I}
                           lappend new_argv $dir
                         }
                         
      -B  {
            incr i
            set dir [lindex $argv $i]
            lappend isystem_dirs "${dir}/include"
          }

      -B* {
            set dir [string range $arg 2 end]
            lappend isystem_dirs "${dir}/include"
          }

      -x {
           # Switch languages
           incr i
           set new_lang [lindex $argv $i]
           if { $new_lang == "c++" } {
             set new_lang cpp
           }
           if { [switch_compiler_config $new_lang] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }

      -trigraphs { set language_trigraphs_allowed 1 }
                         
      -std=c89          -
      -std=iso9899:1990 {
                          # FIXME: Want to do this, but it
                          # FIXME: disables __extension__
                          # FIXME: and friends
                          # set gnu_mode 0

                          if { $::beam::lang == "c" } {
                            set c99_mode 0
                            set language_trigraphs_allowed 1
                            set predefined_macro(__STRICT_ANSI__) 1
                          }
                        }

      -ansi {
              # FIXME: Want to do this, but it
              # FIXME: disables __extension__
              # FIXME: and friends
              # set gnu_mode 0

              if { $::beam::lang == "c" } {
                set c99_mode 0
              }

              set language_trigraphs_allowed 1
              set predefined_macro(__STRICT_ANSI__) 1
            }
      
      -std=gnu89 {
                   if { $::beam::lang == "c" } {
                     set gnu_mode $orig_gnu_mode
                     set c99_mode 0
                     set language_trigraphs_allowed 0

                     if { [::info exists predefined_macro(__STRICT_ANSI__)] } {
                       unset predefined_macro(__STRICT_ANSI__)
                     }
                   }
                 }

      -std=iso9899:199409 {
                            if { $::beam::lang == "c" } {
                              # FIXME: Same problem as '-ansi': set gnu_mode 0
                              set c99_mode 0
                              set language_trigraphs_allowed 1
                            }
                          }
      
      -std=c9x          -
      -std=c99          -
      -std=iso9899:199x -
      -std=iso9899:1999 {
                          if { $::beam::lang == "c" } {
                            # FIXME: Same problem as '-ansi': set gnu_mode 0
                            set c99_mode 1
                            set language_trigraphs_allowed 1
                          }
                        }

      -std=gnu9x -
      -std=gnu99 {
                   if { $::beam::lang == "c" } {
                     set gnu_mode $orig_gnu_mode
                     set c99_mode 1
                     set language_trigraphs_allowed 0
                   }
                 }
      
      -std=c++98 {
                   if { $::beam::lang == "cpp" } {
                     # FIXME: Same problem as '-ansi': set gnu_mode 0
                     set language_trigraphs_allowed 1
                     set predefined_macro(__STRICT_ANSI__) 1
                   }
                 }
      
      -std=gnu++98 {
                     if { $::beam::lang == "cpp" } {
                       set gnu_mode $orig_gnu_mode
                       set language_trigraphs_allowed 0

                       if { [::info exists predefined_macro(__STRICT_ANSI__)] } {
                         unset predefined_macro(__STRICT_ANSI__)
                       }
                     }
                   }

      -pedantic        { # Do nothing; GCC just puts out more warnings }
      -pedantic-errors { # FIXME: Same problem as '-ansi': set strict_mode 1 }
      
      -w    { lappend new_argv {--no_warnings} }
                         
      -o    -
      -D    -
      -U    -
      -I    {
              lappend new_argv $arg
              incr i
              lappend new_argv [lindex $argv $i]
            }

      
      -E    -
      -o*   -      
      -D*   -
      -U*   -
      -I*   { lappend new_argv $arg }
      
           
      --gcc=*    { beam::warning "--gcc is not recognized" }
      
      --edg=*    { list_add new_argv [unescape_arg [string range $arg 6 end]] }


      -A         -      
      -L         -
      -aux-info  -
      -G         -
      -l         { incr i ;# Skip this opt and next opt }

      -*         { ;# Ignore the rest of the flags that start with - }
      
      *.s        -
      *.S        -
      *.c        -
      *.C        -
      *.cpp      -
      *.cxx      -
      *.cc       { lappend new_argv $arg }

      default    { ;# Ignore unknown flags }
    }
  }

  # Add all of the -isystem dirs to the front of system_include_path
  if { [::info exists system_include_path] } {
    set system_include_path [concat $isystem_dirs $system_include_path]
  } else {
    set system_include_path $isystem_dirs
  }
  
  
  # This list of arguments is what EDG will use  
  return $new_argv  
}

#######################################################################
# Command line configurer for GCC compilers
#######################################################################

proc ::beam::compiler::config_gcc_flags {argv} {
  # Return those command-line flags that would affect beam_configure
  
  set for_configure {}
  
  set len [llength $argv]
  
  for { set i 0 } { $i < $len } { incr i } {
    set arg [lindex $argv $i]
   
    switch -glob -- $arg {
      -x -
      -B {
        lappend for_configure $arg
        incr i
        lappend for_configure [lindex $argv $i]
      }

      -m32 -
      -m64 -
      -B*  { lappend for_configure $arg }
      
      -isystem -
      -idirafter -
      -include -
      -imacros -
      -iprefix -
      -iwithprefix -
      -iwithprefixbefore -
      -A -
      -L -
      -aux-info -
      -G -
      -l -
      -o -
      -D -
      -U -
      -I { incr i ;# Skip the next option }
    } 
  }
  
  return $for_configure
}
