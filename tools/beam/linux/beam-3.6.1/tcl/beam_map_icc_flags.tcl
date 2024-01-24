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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        Command line mapper for Intel's ICC (version 9.0) compiler.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_icc_flags)] } return
set ::guard(beam_map_icc_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper ::beam::compiler::map_icc_flags

proc ::beam::compiler::map_icc_flags {argv} {

  # Get the variables and helper routines
  
  ::beam::compiler::import

  # The var new_argv will hold the new command line, which will
  # be returned. 

  set orig_argv $argv  
  set new_argv {}

  set defines() ""
  set defines(-ansi)  "-D__STRICT_ANSI__=1 -Uunix -Ulinux -Ui386"
  set defines(-fast)  "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-msse)  "-D__MMX__=1 -D__SSE__=1"
  set defines(-msse2) "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-msse3) "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-xK)    "-D__MMX__=1 -D__SSE__=1"
  set defines(-xW)    "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-xN)    "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-xB)    "-D__SSE2__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-xP)    "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                       -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"
  set defines(-O0)    "-U__OPTIMIZE__ -D__NO_INLINE__=1"
  set defines(-O1)    "-D__OPTIMIZE_SIZE__=1"
  set defines(-O2)    ""
  set defines(-O3)    ""
  set defines(-Ob0)   "-D__NO_INLINE__=1"
  set defines(-Ob1)   ""
  set defines(-Ob2)   ""
  set defines(-Os)    ""

  set defines(-openmp)         "-D_MT=1 -D_OPENMP 2002=3"
  set defines(-openmp-profile) "-D_MT=1 -D_OPENMP 2002=3"
  set defines(-parallel)       "-D_MT=1"

  set defines(-strict-ansi)  "-D__STRICT_ANSI__=1 -D__INTEL_STRICT_ANSI__=1 \
                              -Uunix -Ulinux -Ui386"
  
  set defines(-march=pentiumpro) "-D__i686=1 -D__i686__=1 -D__pentiumpro=1 \
                                  -D__pentiumpro__=1 -D__tune_i686__=1 \
                                  -D__tune_pentiumpro__=1"
  set defines(-march=pentiumii)  "-D__MMX__=1"
  set defines(-march=pentiumiii) "-D__MMX__=1 -D__SSE__=1"
  set defines(-march=pentium4)   "-D__pentium4=1 -D__pentium4__=1 -D__SSE2__=1 \
                                  -D__tune_pentium4__=1 -D__MMX__=1 -D__SSE__=1"

  set defines(-mcpu=pentium)    "-D__tune_i586__=1 -D__tune_pentium__=1"
  set defines(-mcpu=pentiumpro) "-D__tune_i686__=1 -D__tune_pentiumpro__=1"
  set defines(-mcpu=pentium4)   "-D__tune_pentium4__=1"


  set defines(-funsigned-char) "-U__SIGNED_CHARS__ -D_CHAR_UNSIGNED=1 \
                                -D__CHAR_UNSIGNED__=1"


  if { $::beam::lang == "c" } {
    set defines(-std=c99)  "-D__STDC_VERSION__=199901L"
    set defines(-c99)      "-D__STDC_VERSION__=199901L"
    set defines(-no-c99)   "-U__STDC_HOSTED__"
    set defines(-fnoexceptions) ""
    set defines(-frtti)    ""
    set defines(-fno-rtti) "-U__INTEL_RTTI__"
    set defines(-no-gcc)   "-U__GNUC__ -U__GNUC_MINOR__ -U__GNUC_PATCHLEVEL__ \
                            -U__NO_INLINE__"
  } else {
    set defines(-std=c99)  ""
    set defines(-c99)      ""
    set defines(-no-c99)   ""
    set defines(-fnoexceptions) "-U__PLACEMENT_DELETE -U__EXCEPTIONS \
                                 -D_HAS_EXCEPTIONS=0"
    set defines(-frtti)    "-D_CPPRTTI=1"
    set defines(-fno-rtti) "-U__RTTI_ -U__INTEL_RTTI__"
    set defines(-no-gcc)   "-U__GNUC__ -U__GNUC_MINOR__ -U__GNUC_PATCHLEVEL__ \
                            -U__NO_INLINE__ -U__GNUG__"
  }

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

    set defs "";   # the built-in #defines for this option
    
    # The right hand side of each pair is a code block.
    # A single hyphen means fall-through to the next case.

    switch -glob -- $arg {

      -O0               -
      -O1               -
      -O2               -
      -O3               -
      -Ob0              -
      -Ob1              -
      -Ob2              -
      -Os               -
      -no-c99           -
      -ansi             -
      -fast             -
      -fnoexceptions    -
      -frtti            -
      -fno-rtti         -
      -fno-gcc          -
      -march=pentiumiii -
      -march=pentiumpro -
      -march=pentium4   -
      -mcpu=pentium     -
      -mcpu=pentiumpro  -
      -mcpu=pentium4    -
      -msse             -
      -msse2            -
      -msse3            -
      -openmp           -
      -openmp-profile   -
      -parallel         -
      -strict-ansi      -
      -xK               -
      -xW               -
      -xN               -
      -xB               -
      -xP               { set defs $defines($arg) }

      -gcc-version=320  { set gnu_mode 30200
                          set defs $defines($arg)
                        }
      -gcc-version=330  { set gnu_mode 30300
                          set defs $defines($arg)
                        }
      -gcc-version=340  { set gnu_mode 30400
                          set defs $defines($arg)
                        }
      -fsigned-char*          { set target_plain_char_is_unsigned 0 }
      -funsigned-char*        { set target_plain_char_is_unsigned 1 
                                set defs $defines($arg)
                              }
      -fno-unsigned-bitfields { set target_plain_int_bit_field_is_unsigned 0 }
      -funsigned-bitfields    { set target_plain_int_bit_field_is_unsigned 1 }

      -fwritable-strings      { set target_string_literals_are_readonly 0 }
      -fno-writable-strings   { set target_string_literals_are_readonly 1 }

      -ffor-scope    { 
                       # undocumented, but accepted
                       set language_use_nonstandard_for_init_scope 0 
                     }
      -fno-for-scope { 
                       # undocumented, but accepted
                       set language_use_nonstandard_for_init_scope 1 
                     }

      -X -
      -nostdinc { set system_include_path {} }            

      
      -isystem  {
                  incr i
                  lappend isystem_dirs [lindex $argv $i]
                }
                
      -idirafter {
                   incr i
                   lappend system_include_path [lindex $argv $i]
                 }

      -include {
                 lappend new_argv {--preinclude}
                 incr i
                 lappend new_argv [lindex $argv $i]
               }

      -imacros {
                 lappend new_argv {--preinclude_macros}
                 incr i
                 lappend new_argv [lindex $argv $i]
               }
               
      -iprefix { 
                 incr i
                 set iprefix [lindex $argv $i]
               }
      
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
                         
      -A- { # undefine any predefined macros
            foreach pdef [array names predefined_macro] {
              regsub {\(.*} $pdef "" def;  # remove (.....) if any
              lappend defs "-U$def"
            }
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

      -Kc++ {
           # Switch languages
           set new_lang cpp
           if { [switch_compiler_config $new_lang] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }

      -Kc {
           # Switch languages
           set new_lang c
           if { [switch_compiler_config $new_lang] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }
                         
      -c99      -
      -std=c99  { set c99_mode 1
                  set defs $defines($arg)
                }
      
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
      
           
      --edg=*  { list_add new_argv [unescape_arg [string range $arg 6 end]] }

      -w       { lappend new_argv {--no_warnings} }
                         

      -A         -      
      -G         -
      -L         -
      -MF        -
      -MQ        -
      -MT        -
      -Qinstall  -
      -T         -
      -Xlinker   -
      -aux-info  -
      -create-pch     -
      -dynamic-linker -
      -export-dir     -
      -fp-model       -
      -opt-report-file    -
      -opt-report-level   -
      -opt-report-phase   -
      -opt-report-routine -
      -pc        -
      -pch-dir   -
      -prof-dir  -
      -prof-file -
      -use-pch   -
      -u         -
      -l         { incr i ;# Skip this flag and its argument }

      -*         { ;# Ignore any flags that start with - }
      
      *.s        -
      *.S        -
      *.c        -
      *.C        -
      *.cpp      -
      *.cxx      -
      *.cc       { lappend new_argv $arg }

      default    { ;# Ignore unknown flags }
    }

# Merge any new defines into new_argv
    set new_argv [concat $new_argv $defs]   
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
