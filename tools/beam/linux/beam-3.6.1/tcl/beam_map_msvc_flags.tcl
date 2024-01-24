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
#        Nate Daly
#
#    DESCRIPTION:
#
#        Command line mapper for Microsoft Visual Studio 8.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#
#	 11/01/06  ndaly    Created
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_msvc_flags)] } return
set ::guard(beam_map_msvc_flags) 1

# Load utils.
source beam_util
source beam_compiler_proc

# Set up routines.
::beam::set_mapper     ::beam::mapper::msvc::map_msvc_flags
::beam::set_describer  ::beam::mapper::msvc::describe_msvc_flags
::beam::set_expander   ::beam::mapper::msvc::expand_msvc_flags
::beam::set_configurer ::beam::mapper::msvc::config_msvc_flags

namespace eval ::beam::mapper::msvc {

  namespace import ::beam::util::*
  
  proc get_flag {flag prefix} {
    return [string range $flag [string length $prefix] end]
  }

  proc lrepeat { strings n } {
    set lst {}

    for { set i 0 } { $i < $n } { incr i } {
      lappend lst $strings
    }

    return $lst
  }

  #
  # Describe the command-line. The beam_compile logic only needs "source"
  # descriptions; the rest of the elements are for map_msvc_flags below.
  # The first argument is not the binary; it is the first command-line flag.
  #
  
  proc describe_msvc_flags { flags } {

    # When we see the /link flag, set this to 1 so we know to treat
    # all subsequent flags as linker flags

    set pass_flags_to_linker 0
    
    # Keep track of the current language
    
    set current_language ""

    # This will be the description
    
    set desc {}
    
    # Walk the command-line
    
    set len [llength $flags]
    
    for { set i 0 } { $i < $len } { incr i } {
      set flag [lindex $flags $i]
      
      # If we've already seen the /link flag, all subsequent flags are
      # treated as linker flags
 
      if { $pass_flags_to_linker == 1 } {
        lappend desc {linker_flag}
        continue
      }

      # Flags for MSVC can start with either a "/" or a "-", and it doesn't
      # accept filenames starting with either. We decided, however, that for
      # convenience and testing in Unix environments, we want to be able to 
      # recognize filenames starting with a "/" iff the string is not a valid
      # MSVC option. To avoid having to check twice as many flags, we first
      # convert leading slashes to dashes. Then, if the flag is not a 
      # recognized MSVC flag, we check the original, unmodified flag to see
      # if it's a filename or other BEAM flag. To avoid having to set a state
      # variable every time we identify an MSVC flag, we keep track of the
      # description length. If the description got longer after the first
      # switch statement, then we know we already found a flag and we don't
      # have to check for anything else in this iteration.

      regsub ^/ $flag - flag_with_dash
      set starting_desc_len [llength $desc]

      switch -glob -- $flag_with_dash {

        -W    -
        -wl   -
        -wd   -
        -we   -
        -wo   -
        -AI   -
        -FU   -
        -H    -
        -F    -
        -o    -
        -V    {
                incr i
                lappend desc {ignore} {ignore}
              }
  
        -arch:SSE  { lappend desc {cmdline {all -D_M_IX86_FP=1}} }
  
        -arch:SSE2 { lappend desc {cmdline {all -D_M_IX86_FP=2}} }
    
        -clr:safe  { lappend desc {cmdline {cpp -D_CPPUNWIND=1 \
                                            -D_DLL=1 \
                                            -D__CLR_VER=20050727 \
                                            -D__cplusplus_cli=200406L \
                                            -D_M_CEE=001 \
                                            -D_M_CEE_PURE=001 \
                                            -D_M_CEE_SAFE=001 \
                                            -D_MANAGED=1}}
                   }
      
        -clr:oldSyntax { lappend desc {cmdline {all -D_DLL=1 \
                                                -D__CLR_VER=20050727 \
                                                -D_M_CEE=001 \
                                                -D_MANAGED=1}
                                           {cpp -D_CPPUNWIND=1}}
                       }
    
        -clr:noAssembly { lappend desc {cmdline {all -D_DLL=1 \
                                                 -D__CLR_VER=20050727 \
                                                 -D_M_CEE=001 \
                                                 -D_MANAGED=1}
                                            {cpp -D__cplusplus_cli=200406L}}
                        }
    
        -clr:pure { lappend desc {cmdline {all -D_DLL=1 \
                                           -D__CLR_VER=20050727 \
                                           -D_M_CEE=001 \
                                           -D_M_CEE_PURE=001 \
                                           -D_MANAGED=1}
                                      {cpp -D_CPPUNWIND=1 \
                                           -D__cplusplus_cli=200406L}}
                  }
    
        -clr:initialAppDomain -
        -clr          { lappend desc {cmdline {all -D_DLL=1 \
                                               -D__CLR_VER=20050727 \
                                               -D_M_CEE=001 \
                                               -D_MANAGED=1}
                                          {cpp -D_CPPUNWIND=1 \ 
                                               -D__cplusplus_cli=200406L}}
                      }
    
        -clr:*        { lappend desc {cmdline {all -D_DLL=1 -D__CLR_VER=20050727}} }
    
        -EHs          -
        -EHsc*        -
        -EHa          -
        -EHac*        { lappend desc {cmdline {cpp -D_CPPUNWIND=1}} } 
    
        -J            { lappend desc {char_is_unsigned} }
    
        -GR           { lappend desc {cmdline {cpp -D_CPPRTTI=1}} }
    
        -GR-          { lappend desc {cmdline {cpp -U_CPPRTTI}} }
    
        -GX           { lappend desc {cmdline {cpp -D_CPPUNWIND=1}} }
    
        -GX-          { lappend desc {cmdline {cpp -U_CPPUNWIND}} }
    
        -MD           { lappend desc {cmdline {all -D_MT=1 -D_DLL=1}} }
    
        -MDd          { lappend desc {cmdline {all -D_MT=1 -D_DLL=1 -D_DEBUG=1}} }
    
        -MT           { lappend desc {cmdline {all -D_MT=1}} }
    
        -MTd          { lappend desc {cmdline {all -D_MT=1 -D_DEBUG=1}} }
    
        -GZ           { lappend desc {cmdline {all -D__MSVC_RUNTIME_CHECKS=1}} }
    
        -RTC*         { lappend desc {cmdline {all -D__MSVC_RUNTIME_CHECKS=1}} }
    
        -Wp64         { lappend desc {cmdline {all -D_Wp64=1}} }
    
        -D            -
        -U            -
        -I            { lappend desc [list cmdline [list all $flag_with_dash]]
                        incr i
                        lappend desc [list cmdline [list all [lindex $flags $i]]]
                      }
    
        -C            -
        -E            -
        -D*           -
        -U*           -
        -I*           { lappend desc [list cmdline [list all $flag_with_dash]] }
    
        -X            { lappend desc {config_set system_include_path {}} }
    
        -u            { lappend desc {unset_macros} }
    
        -EP           { lappend desc {cmdline {all -P}}
                        # NOTE: this also prevents expansion of __FUNCDNAME__, 
                        # __FUNCSIG__, and __FUNCTION__
                      }
    
        -FI*          { lappend desc [list cmdline [list all --preinclude [get_flag $flag_with_dash -FI]]] }
    
        -Fo*          { lappend desc [list output [get_flag $flag_with_dash -Fo]] }
    
        -openmp       { lappend desc {cmdline {all -D_OPENMP=200203}} }
    
        -Za           { lappend desc {cmdline {all -U_MSC_EXTENSIONS}
                                              {c -D__STDC__=1}}
                        # FIXME: disables microsoft extensions
                      }
    
        -Ze           {
                        # FIXME: enables microsoft extensions
                        lappend desc {ignore}
                      }
    
        -Zc:wchar_t   { lappend desc {cmdline {all -D_WCHAR_T_DEFINED=1 -D_NATIVE_WCHAR_T_DEFINED=1}} }
    
        -Zc:wchar_t-  { lappend desc {cmdline {all -U_WCHAR_T_DEFINED -U_NATIVE_WCHAR_T_DEFINED}} }
    
        -Zc:forScope  { lappend desc {config_set language_use_nonstandard_for_init_scope 0} }
    
        -Zc:forScope- { lappend desc {config_set language_use_nonstandard_for_init_scope 1} }
    
        -Zl           { lappend desc {cmdline {all -D_VC_NODEFAULTLIB=1}} }
    
        -LDd          { lappend desc {cmdline {all -D_DEBUG=1}} }
    
        -TC           {
                        set current_language "c"
                        lappend desc {configure}
                      }
    
        -TP           {
                        set current_language "cpp"
                        lappend desc {configure}
                      }
    
        @*            { # NOTE: this gets handled in beam_compile_utils.tcl
                        # and is included here only for completeness.
                        # no separable args

                        lappend desc {ignore}
                      }
    
        -ZI           { # NOTE: this redefines __FILE__ to absolute path 
                        # of __FILE__, no args 

                        lappend desc {ignore}
                      }
    
        -FC           { # NOTE: this redefines __FILE__ to absolute path 
                        # of __FILE__, no args 

                        lappend desc {ignore}
                    }
    
        -Zp*          { # FIXME: might need to handle this, no defines, 
                        # no separable args

                        lappend desc {ignore}
                      }
    
        -Tc           {
                        lappend desc {delete}
                        incr i
                        lappend desc [list source c [lindex $flags $i]]
                      }
  
        -Tc*          { lappend desc [list source c [get_flag $flag_with_dash -Tc]] }
    
        -Tp           {
                        lappend desc {delete}
                        incr i
                        lappend desc [list source cpp [lindex $flags $i]]
                      }
        
        -Tp*          { lappend desc [list source cpp [get_flag $flag_with_dash -Tp]] }
    
        -Fx           -
        -G1           -
        -G2           -
        -Gd           -
        -Ge           -
        -GF           -
        -Gh           -
        -GH           -
        -Gm           -
        -Gr           -
        -GT           -
        -Gz           -
        -homeparams   -
        -Yd           -
        -Z7           -
        -Zi           -
        -Zx           -
        -vmb          -
        -vmg          -
        -vmm          -
        -vms          -
        -vmv          -
        -Zg           -
        -hotpatch     -
        -nologo       -
        -showIncludes -
        -bigobj       -
        -?            -
        -HELP         -
        -help         -
        -c            -
        -Zs           -
        -LD           -
        -LN           -
        -Y-           -
        -Yd           {
                        # ignore, no defines, no args
                        lappend desc {ignore}
                      }

        -P            { # ignore, no defines, no args 
                        # NOTE: this also prevents expansion of __FUNCDNAME__, 
                        # __FUNCSIG__, and __FUNCTION__
          
                        lappend desc {ignore}
                      }
    
        -doc*         -
        -GL*          -
        -Gy*          -
        -FA*          -
        -GS*          -
        -Zc*          -
        -analyze*     -
        -errorReport* -
        -Gs*          -
        -Fa*          -
        -Fd*          -
        -Fe*          -
        -Fm*          -
        -Fp*          -
        -FR*          -
        -Fr*          -
        -EH*          -
        -favor*       -
        -fp*          -
        -O*           -
        -QI*          -
        -vd*          -
        -Yc*          -
        -Yl*          -
        -Yu*          -
        -Zm*          {
                        # ignore, no defines, no separable args
                        lappend desc {ignore}
                      }
    
        -link   { 
                  set pass_flags_to_linker 1
                  lappend desc {link}
                }

      }

      # If the description didn't get any longer after the first switch
      # statement, then we didn't find any valid MSVC flags and we can check
      # for filenames and other BEAM flags.

      if { [llength $desc] == $starting_desc_len } {

        switch -glob -- $flag {
          --edg=*   { lappend desc [list cmdline [concat all [ut_unescape_arg \
                                                             [get_flag $flag --edg=]]]] 
                    }
    
          *.s -
          *.S {
                if { $current_language != "" } {
                  lappend desc [list source $current_language $flag]
                } else {
                  lappend desc [list source asm $flag]
                }
              }
      
          *.c -
          *.C {
                if { $current_language != "" } {
                  lappend desc [list source $current_language $flag]
                } else {
                  lappend desc [list source c $flag]
                }
              }
      
          *.cpp -
          *.cxx -
          *.cc  {
                  if { $current_language != "" } {
                    lappend desc [list source $current_language $flag]
                  } else {
                    lappend desc [list source cpp $flag]
                  }
                }
      
          default  {
                     # Ignore unknown flags
                     lappend desc {ignore}
                   }
        }
      }
    }

    return $desc
  }
  
  proc map_msvc_flags { flags } {
    ::beam::compiler::import

    set new_flags {}

    # Save the input and output file for later mangling
    set input ""
    # Leave output unset and test later

    # Get the description
    set desc [describe_msvc_flags $flags]
    
    # Walk them both, setting up new_flags

    ut_foreach_desc type rest pos $flags $desc {

      switch -exact -- $type {
  
        source { set input [lindex $rest 1]; lappend new_flags $input }

	output { set output [lindex $rest 0] }
  
        config_set { set [lindex $rest 0] [lindex $rest 1] }
  
        cmdline { foreach cmd_desc $rest {
                    set target_lang [lindex $cmd_desc 0]
                    if { $target_lang == "all" || $target_lang == $::beam::lang} {
                      ut_list_add new_flags [lrange $cmd_desc 1 end]
                    }
                  }
                }
  
        char_is_unsigned { lappend new_flags {-D_CHAR_UNSIGNED} 
                           set target_plain_char_is_unsigned 1
                         }
  
        unset_macros { unset predefined_macro
                       if { $::beam::lang != "c" } {
                         set predefined_macro(_NATIVE_WCHAR_T_DEFINED) {1}
                         set predefined_macro(_WCHAR_T_DEFINED)        {1}
                       }
                     }
  
        default { # ignore }
      }
    }

    # Mangle the output file name based on MSVC rules.
    #
    # If the output file is empty, the output file should
    # be written with the basename of the input file.
    #
    # If the output file ends in a slash, it is a directory 
    # in which the basename input.obj should be written.

    if { [::info exists output ] } {
      set basename [beam::basename $input]
      regsub {\.[^.]*?$} $basename .o basename_o

      if { $output == "" } {
        set output $basename_o
      } elseif { [string match "*/" $output] || [string match "*\\\\" $output] } {
        set output "$output/$basename_o"
      } else {
        # Replace .obj with .o so BEAM recognizes "file.o"
        regsub {\.obj$} $output .o output
      }

      ut_list_add new_flags [list -o $output]
    }
    
    return $new_flags
  }
  
  proc expand_msvc_flags { flags } {
    # No environment variables.
    
    # @file args are expanded specially.
    return [ut_expand_msvc_atfiles $flags]
  }
  
  proc config_msvc_flags { flags } {
    # Return those flags that should cause reconfiguration.
    set conf_flags {}
    
    # Get the description
    set desc [describe_msvc_flags $flags]
    
    # Walk them both, setting up conf_flags
    ut_foreach_desc type rest pos $flags $desc {
      switch -exact -- $type {
        configure { lappend conf_flags [lindex $flags $pos] }  
        default { # ignore }
      }
    }
    
    return $conf_flags
  }
}
