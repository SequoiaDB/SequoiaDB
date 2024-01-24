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
#        Command line mapper for IBM VisualAge 3.6 for Windows.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#        11/13/07  fwalling Copied from Nate Daly's beam_map_msvc_flags.tcl
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_vacwin_flags)] } return
set ::guard(beam_map_vacwin_flags) 1

# Load utils
source beam_util
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper    ::beam::mapper::vacwin::map_vacwin_flags
::beam::set_describer ::beam::mapper::vacwin::describe_vacwin_flags
::beam::set_expander  ::beam::mapper::vacwin::expand_vacwin_flags

namespace eval ::beam::mapper::vacwin {

  namespace import ::beam::util::*
  
  proc get_flag {flag prefix} {
    return [string range $flag [string length $prefix] end]
  }

  #
  # Describe the command-line. The beam_compile logic only needs "source"
  # descriptions; the rest of the elements are for map_vacwin_flags below.
  #
  # This sets up the description so that each item is an opaque [name arg arg]
  # list. This is what is expected by the map_flags procedure below.
  #
  # The first argument is not the binary; it is the first command-line flag.
  #
  proc describe_vacwin_flags { flags } {

    # Keep track of the current language
    
    set current_language ""
    
    # This will be the description
    
    set desc {}
    
    # Walk the command-line
    
    set len [llength $flags]
    
    for { set i 0 } { $i < $len } { incr i } {
      set flag [lindex $flags $i]
      
      # Flags for VisualAge for Windows can start with either a
      # "/" or a "-", and it doesn't accept filenames starting with either.
      # We decided, however, that for convenience and testing in Unix
      # environments, we want to be able to recognize filenames starting
      # with a "/" iff the string is not a valid VisualAge option. To
      # avoid having to check twice as many flags, we first convert leading
      # slashes to dashes. Then, if the flag is not a recognized VisualAge
      # flag, we check the original, unmodified flag to see if it's a
      # filename or other BEAM flag. To avoid having to set a state
      # variable every time we identify a VisualAge flag, we keep track of the
      # description length. If the description got longer after the first
      # switch statement, then we know we already found a flag and we don't
      # have to check for anything else in this iteration.

      regsub ^/ $flag - flag_with_dash
      set starting_desc_len [llength $desc]

      # -? is special because it contains glob characters
      if { $flag_with_dash == "-?" } {
        lappend desc {ignore}
        continue
      }
      
      switch -glob -- $flag_with_dash {

        -B*   -
        -C*   -
        -Fa*  -
        -Fc*  -
        -Fe*  -
        -Fi*  -
        -Fm*  -
        -Fn*  -
        -G3   -
        -G4   -
        -G5   -
        -Gf*  -
        -Gi*  -
        -Gk*  -
        -Gl*  -
        -Gp*  -
        -Gr*  -
        -Gs*  -
        -Gv*  -
        -Gx*  -
        -Gy*  -
        -H*   -
        -L*   -
        -M*   -
        -N*   -
        -O*   -
        -Q*   -
        -Sd*  -
        -Sg*  -
        -Si*  -
        -Sp*  -
        -Sq*  -
        -Sr*  -
        -Su*  -
        -Sv*  -
        -Ti*  -
        -Tn*  -
        -Tx*  -
        -V*   -
        -W*   {
                lappend desc {ignore}
              }
              
        -qalias=*         -
        -arch=*           -
        -qautoimported    -
        -qnoautoimported  -
        -qautothread      -
        -qnoautothread    -
        -qdbgunref        -
        -qnodbgunref      -
        -qignerrno        -
        -qnoignerrno      -
        -qignprag=*       -
        -qinitauto=*      -
        -qnoinitauto      -
        -qisolated_call=* -
        -qlibansi         -
        -qnolibansi       -
        -qmakedep         -
        -qrtti=*          -
        -qnortti          -
        -qsomvolattr      -
        -qnosomvolattr    -
        -qtune=*          {
                            lappend desc {ignore}
                          }
  
        -D    { lappend desc {ignore}
                incr i
                lappend desc [list define_macro [lindex $flags $i]]
              }
        -D*   { lappend desc [list define_macro [get_flag $flag_with_dash -D]] }

        -U    { lappend desc {ignore}
                incr i
                lappend desc [list undefine_macro [lindex $flags $i]]
              }
        -U*   { lappend desc [list undefine_macro [get_flag $flag_with_dash -U]] }

        -I    { lappend desc {ignore}
                incr i
                lappend desc [list user_include [lindex $flags $i]]
              }
        -I*   { lappend desc [list user_include [get_flag $flag_with_dash -I]] }
        
        -Fo-  { lappend desc [list output /dev/null] }
        -Fo   -
        -Fo+  { lappend desc {output} }
        -Fo*  { lappend desc [list output [get_flag $flag_with_dash -Fo]] }

        -Ft-  { lappend desc {template_res 0} }
        -Ft   -
        -Ft+  -
        -Ft*  { lappend desc {template_res 1} }
        
        -Gd-  { lappend desc {dynamic_link 0} }
        -Gd   -
        -Gd+  { lappend desc {dynamic_link 1} }
        
        -Ge-  { lappend desc {generate dll} }
        -Ge   -
        -Ge+  { lappend desc {generate exe} }
        
        -Gm-  { lappend desc {multithread 0} }
        -Gm   -
        -Gm+  { lappend desc {multithread 1} }
        
        -Gn-  { lappend desc {no_default_libs 0} }
        -Gn   -
        -Gn+  { lappend desc {no_default_libs 1} }
        
        -Gt-  { lappend desc {tiled_mem 0} }
        -Gt   -
        -Gt+  { lappend desc {tiled_mem 1} }
        
        -J-   { lappend desc {chars signed} }
        -J    -
        -J+   { lappend desc {chars unsigned} }

        -P*   {
                # This option is fairly complicated.
                # We support the following:
                #
                #  /P, /P+ Run preproc to file (based on input file name)
                #  /P-     Don't run the preproc
                #  /Pd+    Run preproc to stdout
                #  /Pd-    Run preproc to file (based on input file name) (like /P+)
                #  
                #  /Pabcde We look for /P...d... and /P...d- and support those as
                #          if they were /Pd[+] and /Pd-, respectively

                switch -glob -- $flag_with_dash {
                  -P   -
                  -P+  { lappend desc {preproc file} }
                  -P-  { lappend desc {preproc disable} }
                  
                  default { # Check for "/Pabcde" with a "d" or "d-" in there
                            if { [string match "*d-" $flag_with_dash] } {
                              lappend desc {preproc file}
                            } elseif { [string match "*d*" $flag_with_dash] } {
                              lappend desc {preproc stdout}
                            } else {
                              lappend desc {ignore}
                            }
                          }
                }
              }
              
        -qbitfields=signed   { lappend desc {targ_plain_int_bit_field_is_unsigned 0} }
        -qbitfields=unsigned { lappend desc {targ_plain_int_bit_field_is_unsigned 1} }
        
        -qdigraph   { lappend desc {language_alternative_tokens_allowed 1} }
        -qnodigraph { lappend desc {language_alternative_tokens_allowed 0} }
        
        -qlonglong { lappend desc {longlong 1} }
        -qnolonglong { lappend desc {longlong 0} }
        
        -qro   { lappend desc {target_string_literals_are_readonly 1} }
        -qnoro { lappend desc {target_string_literals_are_readonly 0} }

        -Re { lappend desc {subsystem 0} }
        -Rn { lappend desc {subsystem 1} }
        
        -Sa { lappend desc {langlvl ansi} }
        -Sc { lappend desc {langlvl compat} }
        -Se { lappend desc {langlvl extended} }
        -S2 { lappend desc {langlvl saal2} }
        
        -Sh- { lappend desc {ddnames 0} }
        -Sh  -
        -Sh+ { lappend desc {ddnames 1} }
        
        -Sm- { lappend desc {ignore_obsolete 0} }
        -Sm  -
        -Sm+ { lappend desc {ignore_obsolete 1} }

        -Sn- { lappend desc {dbcs 0} }
        -Sn  -
        -Sn+ { lappend desc {dbcs 1} }

        -Ss- { lappend desc {language_end_of_line_comments_allowed 0} }
        -Ss  -
        -Ss+ { lappend desc {language_end_of_line_comments_allowed 1} }
        
        -Tc  { lappend desc {delete}
               incr i
               lappend desc [list source c [lindex $flags $i]]
             }
        -Tc* { lappend desc [list source c [get_flag $flag_with_dash -Tc]] }
        
        -Tm- { lappend desc {debug_mem 0} }
        -Tm  -
        -Tm+ { lappend desc {debug_mem 1} }
        
        -Tp  { lappend desc {delete}
               incr i
               lappend desc [list source cpp [lindex $flags $i]]
             }
        -Tp* { lappend desc [list source cpp [get_flag $flag_with_dash -Tp]] }

        -Td  { set current_language "";
               lappend desc {ignore}
             }
        -Tdc { set current_language "c";
               lappend desc {ignore}
             }
        -Tdp { set current_language "cpp";
               lappend desc {ignore}
             }
        
        -Xc- { lappend desc {ignore} }
        -Xc  -
        -Xc+ { lappend desc {reset_user_includes} }
        
        -Xi- { lappend desc {ignore} }
        -Xi  -
        -Xi+ { lappend desc {reset_system_includes} }
        
        @* {
             # NOTE: this gets handled in beam_compile_utils.tcl
             # and is included here only for completeness.
             lappend desc {ignore}
           }
      }

      # If the description didn't get any longer after the first switch
      # statement, then we didn't find any valid VisualAge flags and we can check
      # for filenames and other BEAM flags.

      if { [llength $desc] == $starting_desc_len } {

        switch -glob -- $flag {
          --edg=* { lappend desc [list cmdline [ut_unescape_arg \
                                                 [get_flag $flag --edg=]]]
                  }
    
          *.s -
          *.S { if { $current_language != "" } {
                  lappend desc [list source $current_language $flag]
                } else {
                  lappend desc [list source asm $flag]
                }
              }
      
          *.c -
          *.C { if { $current_language != "" } {
                  lappend desc [list source $current_language $flag]
                } else {
                  lappend desc [list source c $flag]
                }
              }
      
          *.cpp -
          *.cxx -
          *.cc  { if { $current_language != "" } {
                    lappend desc [list source $current_language $flag]
                  } else {
                    lappend desc [list source cpp $flag]
                  }
                }
      
          default {
                    # Ignore unknown flags
                    lappend desc {ignore}
                  }
        }
      }
    }
      
    return $desc
  }

  #
  # This is a helper procedure that takes in the flags, the desc,
  # and a list of exceptions to this rule.
  #
  # This procedure removes items from flags and desc, leaving only
  # the last occurrence of each descriptor in place.
  #
  # For example, given this flags and desc:
  #
  #  flags:  {  -a    -b     -c    -d   }
  #  desc:   { {a 1} {b 1}  {a 2} {b 3} }
  #
  # The following would be returned (assuming nothing is in
  # the exception list):
  #
  #  flags:  {  -c    -d   }
  #  desc:   { {a 2} {b 3} }
  #
  # The last "a" and the last "b" remained (from the description),
  # and the other flags and descriptions were removed.
  #
  # For those descriptions in the exception list, all will be
  # left alone.
  #
  proc remove_dups { flagsref descref except_list } {
    upvar $flagsref flags
    upvar $descref desc
    
    set new_flags {}
    set new_desc  {}
    
    # Cache the exceptions in an array for quick lookup
    foreach ex $except_list { set except($ex) 1 }
    
    # Walk the lists backwards so we can just treat them as if
    # the first one wins, which is much easier
    
    set rev_flags [ut_list_reverse $flags]
    set rev_desc  [ut_list_reverse $desc]
    
    ut_foreach_desc type rest pos $rev_flags $rev_desc {
      set add 0
      
      # If this type has an exception, add it unconditionally
      if { [::info exists except($type)] } {
        set add 1
      } else {
        # Add it only if it hasn't been seen before
        if { ! [::info exists seen($type)] } {
          set seen($type) 1
          set add 1
        }
      }
      
      if { $add } {
        lappend new_flags [lindex $rev_flags $pos]      
        lappend new_desc  [lindex $rev_desc  $pos]
      }
    }
    
    # Reverse the results and store them back to the caller
    set flags [ut_list_reverse $new_flags]
    set desc  [ut_list_reverse $new_desc]
  }
  
  #
  # This splits macro text like "FOO=BAR" into a macro definition
  # by setting the name to "FOO" and the value to "BAR".
  #
  # If the text is "FOO", then the value will be 1.
  # if the text is "FOO=" or "FOO::", then the value will be "".
  # Otherwise, name and value are set as expected (NAME=VALUE).
  #
  proc split_macro {text nameref valueref} {
    upvar $nameref name
    upvar $valueref value
   
    if { [regexp {^(.*)=$} $text null m1] } {
      # NAME=
      set name $m1
      set value ""
    } elseif { [regexp {^(.*)::$} $text null m1] } {
      # NAME::
      set name $m1
      set value ""
    } elseif { [regexp {=} $text] } {
      # NAME=VALUE
      set pair [split $text =]
      set name [lindex $pair 0]
      set value [join [lrange $pair 1 end] =]
    } else {
      # NAME
      set name $text
      set value 1
    }
  }
  
  #
  # Turns VisualAge for Windows flags into BEAM settings and EDG flags.
  # Uses the describe_vacwin_flags routine above to do the heavy lifting.
  # This interperets the resulting description.
  #
  proc map_vacwin_flags { flags } {
    ::beam::compiler::import

    set new_flags {}

    # Track a few settings that we revisit after we process
    # the flags.
    set input ""
    set user_includes {}
    array set user_macro {}
    set preproc ""
    
    # Get the description
    set desc [describe_vacwin_flags $flags]

    # For VisualAge for Windows, there are different types of flags.
    #
    # 1) Some, like "source" and "user_include" and "define_macro"
    #    are special. They are processed as they are seen.
    # 2) Most of the rest are a last-one-wins. They describe features
    #    that have been turned on and/or off on the command-line,
    #    and the final setting is the setting we care about.
    #
    # The easiest way to process the last-one-wins flags is to make
    # one pass over the command-line, removing any flags that are
    # later overridden (except for our special ones).
    #
    # That way, the second pass can just process what it sees, and
    # the side-effects that are done won't have to be undone later.

    #    
    # Pass 1: Remove flags that are later overridden by other flags.
    #         The exceptions are listed here.
    #
    
    set exceptions { source cmdline
                     user_include reset_user_includes reset_system_includes
                     define_macro undefine_macro }

    remove_dups flags desc $exceptions

    #
    # Pass 2: Walk what is left, setting up new new_flags and our state
    #
    
    ut_foreach_desc type rest pos $flags $desc {
      switch -exact -- $type {
        source { set input [lindex $rest 1]
                 lappend new_flags $input }

        output { if { [llength $rest] == 0 } {
                   beam::unset output
                 } else {
                   set output [lindex $rest 0]
                 }
               }

        preproc { set preproc [lindex $rest 0] }
    
        cmdline { ut_list_add new_flags [lindex $rest 0] }

        ignore_obsolete { set enable [lindex $rest 0]
                          if { $enable } {
                            set predefined_macro(near) "    "
                            set predefined_macro(far)  "   "
                            set predefined_macro(huge) "    "
                          } else {
                            beam::unset predefined_macro(near)
                            beam::unset predefined_macro(far)
                            beam::unset predefined_macro(huge)
                          }
                        }
        
        longlong { set enable [lindex $rest 0]
                   if { $enable } {
                     set predefined_macro(_LONG_LONG) 1
                   } else {
                     beam::unset predefined_macro(_LONG_LONG)
                   }
                 }

        langlvl { set langlvl [lindex $rest 0]
                  switch -exact -- $langlvl {
                    ansi {
                      set c99_mode 0
                      set predefined_macro(__ANSI__) 1
                      beam::unset predefined_macro(__COMPAT__)
                      beam::unset predefined_macro(__EXTENDED__)
                      beam::unset predefined_macro(__SAA__)
                      beam::unset predefined_macro(__SAAL2__)
                    }
                    compat {
                      set c99_mode 0
                      if { $::beam::lang == "cpp" } {
                        set predefined_macro(__COMPAT__) 1
                      } else {
                        beam::unset predefined_macro(__COMPAT__)
                      }
                      beam::unset predefined_macro(__ANSI__)
                      beam::unset predefined_macro(__EXTENDED__)
                      beam::unset predefined_macro(__SAA__)
                      beam::unset predefined_macro(__SAAL2__)
                    }
                    extended {
                      set c99_mode 1
                      set predefined_macro(__EXTENDED__) 1
                      beam::unset predefined_macro(__ANSI__)
                      beam::unset predefined_macro(__COMPAT__)
                      beam::unset predefined_macro(__SAA__)
                      beam::unset predefined_macro(__SAAL2__)
                    }
                    saal2 {
                      set c99_mode 0
                      set predefined_macro(__EXTENDED__) 1
                      if { $::beam::lang == "c" } {
                        set predefined_macro(__SAA__) 1
                        set predefined_macro(__SAAL2__) 1
                      } else {
                        beam::unset predefined_macro(__SAA__)
                        beam::unset predefined_macro(__SAAL2__)                        
                      }
                      beam::unset predefined_macro(__ANSI__)
                      beam::unset predefined_macro(__COMPAT__)
                    }
                    default {
                      # ignore
                    }
                  }
                }

        chars { set chars [lindex $rest 0]
                if { $chars == "signed" } {
                  set target_plain_char_is_unsigned 0
                  set predefined_macro(_CHAR_SIGNED) 1
                  beam::unset predefined_macro(_CHAR_UNSIGNED)
                } elseif { $chars == "unsigned" } {
                  set target_plain_char_is_unsigned 1
                  set predefined_macro(_CHAR_UNSIGNED) 1      
                  beam::unset predefined_macro(_CHAR_SIGNED)
                } else {
                  # Leave the compiler defaults
                }
              }
              
        dbcs { set enable [lindex $rest 0]
               if { $enable } {
                 set predefined_macro(__DBCS__) 1
               } else {
                 beam::unset predefined_macro(__DBCS__)
               }
             }
             
        ddnames { set enable [lindex $rest 0]
                  if { $enable } {
                    set predefined_macro(__DDNAMES__) 1
                  } else {
                    beam::unset predefined_macro(__DDNAMES__)
                  }
                }

        debug_mem { set enable [lindex $rest 0]
                    if { $enable } {
                      set predefined_macro(__DEBUG_ALLOC__) 1
                    } else {
                      beam::unset predefined_macro(__DEBUG_ALLOC__)
                    }
                  }

        generate { set generate [lindex $rest 0]
                   if { $generate == "dll" } {
                     set predefined_macro(_DLL) 1
                     set predefined_macro(__DLL__) 1
                   } else {
                     beam::unset predefined_macro(_DLL)
                     beam::unset predefined_macro(__DLL__)
                   }
                 }
                   
        dynamic_link { set enable [lindex $rest 0]
                       if { $enable } {
                         set predefined_macro(__IMPORTLIB__) 1
                       } else {
                         beam::unset predefined_macro(__IMPORTLIB__)
                       }
                     }

        multithread { set enable [lindex $rest 0]
                      if { $enable } {
                        set predefined_macro(_MT) 1
                        set predefined_macro(__MULTI__) 1
                      } else {
                        beam::unset predefined_macro(_MT)
                        beam::unset predefined_macro(__MULTI__)                        
                      }
                    }

        no_default_libs { set no_libs [lindex $rest 0]
                          if { $no_libs } {
                            beam::unset predefined_macro(__NO_DEFAULT_LIBS__)
                          } else {
                            set predefined_macro(__NO_DEFAULT_LIBS__) 1
                          }
                        }

        subsystem { set enable [lindex $rest 0]
                    if { $enable } {
                      set predefined_macro(__SPC__) 1
                    } else {
                      beam::unset predefined_macro(__SPC__)
                    }
                  }

        template_res { set enable [lindex $rest 0]
                       if { $enable } {
                         set predefined_macro(__TEMPINC__) 1   
                       } else {
                         beam::unset predefined_macro(__TEMPINC__)
                       }
                     }
                     
        tiled_mem { set enable [lindex $rest 0]
                    if { $enable } {
                      set predefined_macro(__TILED__) 1   
                    } else {
                      beam::unset predefined_macro(__TILED__)
                    }
                  }
                    
        define_macro { set macro_text [lindex $rest 0]
                       split_macro $macro_text name value

                       # It is a warning to redefine a macro. The first
                       # one wins.
                       
                       if { [::info exists user_macro($name)] } {
                         beam::warning "Ignoring redefinition of macro '$name'"
                       } elseif { [::info exists predefined_macro($name)] } {
                         beam::warning "Ignoring redefinition of macro '$name'"
                       } else {
                         set user_macro($name) $value
                       }
                     }
                     
        undefine_macro { # Undefining on the command-line only affects other
                         # command-line macros specified by the user
                         set name [lindex $rest 0]
                         array unset user_macro $name ;# Works for "*" too
                       }
        
        user_include { set dirs [split [lindex $rest 0] $::beam::pathsep]
                       ut_list_add user_includes $dirs }
                 
        reset_user_includes { set user_includes {} }
        reset_system_includes { set system_include_path {} }

        language_alternative_tokens_allowed {
          set language_alternative_tokens_allowed [lindex $rest 0]
          if { $language_alternative_tokens_allowed } {
            set predefined_macro(__DIGRAPHS__) 1
          } else {
            beam::unset predefined_macro(__DIGRAPHS__)
          }
        }
        
        ignore { }
        
        default {
                  # By default, we just set var to value from $rest
                  set var [lindex $rest 0]
                  set val [lindex $rest 1]
                  set $var $val
                }
      }
    }

    # Post-processing.
    
    # Add in user-defined macros
    foreach name [array names user_macro] {
      set value $user_macro($name)
      lappend new_flags "-D${name}=${value}"
    }
    
    # Add in search paths.
    foreach dir $user_includes {
      lappend new_flags -I $dir
    }
    
    # Set up preprocessing options, if given.
    # This is done late because we need the name of the input
    # file.
    if { $preproc == "stdout" } {
      lappend new_flags -E
    } elseif { $preproc == "file" } {
      set basename [beam::basename $input]
      regsub {\.[^.]*?$} $basename .i basename_i
      lappend new_flags -E -o $basename_i
    }
    
    # Mangle the output file name based on VisualAge for
    # Windows rules.
    #
    # If the output file ends in a slash, it is a directory 
    # in which the input.beam should be written.
    #
    # Note that if $preproc == "file", no output is given here.
    # Output is done in the preproc setup in that case.
    if { [::info exists output ] && $preproc != "file" } {
      if { [ut_is_dirsep [string index $output end]] } {
        set basename [beam::basename $input]
        regsub {\.[^.]*?$} $basename .o basename_o
        append output $basename_o
      } else {
        # Replace .obj with .o so BEAM recognizes "file.o"
        regsub {\.obj$} $output .o output
      }

      lappend new_flags -o $output
    }
    
    return $new_flags
  }

  proc expand_vacwin_flags { flags } {
    # The ICC environment variable is a list of options for the
    # VisualAge for Windows compiler.

    if { [::info exists ::env(ICC)] } {
      ut_list_add_front flags [ut_unescape_arg $::env(ICC)]
    }
    
    # @file arguments are expanded normally.
    return [ut_expand_atfiles $flags]
  }
}
