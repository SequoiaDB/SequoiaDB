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
#        Command line mapper for sCC compiler.
#
#        -qtune=Power7
#        -ruvpd              -- dunno; ignore
#        -nog                -- no effect on macros
#        -Q                  -- no effect on macros
#        -opt=FE:-qnoeh      -U__EXCEPTIONS -U_CPPUNWIND
#        -qconvert=819       -- no effect on macros
#        -qvftcheck          -- controls virt. func. table implementation
#        -qnewCheck          -- no effect on macros
#        -qnolimit           -- no effect on macros
#        -qnothresh          -- no effect on macros
#        -qphyp              -- no effect on macros
#        -qannotate=all      -- no effect on macros
#        -qoffsetinfo=hex    -- for listfile stuff I think
#        -compver=v7r2m0     -- version
#        -transver=v7r2m0    -- version
#        -tabver=v7r2m0      -- version
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_sCC2010_flags)] } return
set ::guard(beam_map_sCC2010_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper ::beam::compiler::map_sCC2010_flags

proc ::beam::compiler::map_sCC2010_flags {argv} {
   ::beam::compiler::import

   set orig_argv $argv
   
   set new_argv {}

   lappend new_argv "--no_warnings"

   set langlvl "--no_warnings"

   set len [llength $argv]

   for { set i 0 } { $i < $len } { incr i } {
     set arg [lindex $argv $i]
#
# A '-q' by itself means that its argument is separated by white space.
#
     if { $arg == "-q" } {
       incr i
       set arg "$arg[lindex $argv $i]"
     }
#
# The argument of the -q flag is interpreted in a case insensitive way.
# Simply convert it to all-lower case here.
#
     if { [string match "-q*" $arg] } {
       set arg [string tolower $arg]
     }

     switch -glob -- $arg {

      --edg=*   { list_add new_argv [unescape_arg [string range $arg 6 end]] }

      --*       { ;
# Do not pass any options beginning with '--' downstream. Most likely
# this was an EDG option that was forgotten to be prefixed with --edg. 
# Tough luck. This will be educational ;)
                }

      -I        -
      -D        -
      -U        { lappend new_argv $arg
                  incr i
                  lappend new_argv [lindex $argv $i] 
                }

      -Z        -
      -y        { incr i; # skip the following argument }

      -E        -
      -I*       -
      -D*       -
      -U*       { lappend new_argv $arg }

      -opt=FE:-qnoeh   { lappend new_argv "-U__EXCEPTIONS"
                         lappend new_argv "-U_CPPUNWIND"
                       }

      -opt=FE:qro      { set target_string_literals_are_readonly 1 }
      -opt=FE:qnoro    { set target_string_literals_are_readonly 0 }

      -opt=*           { ; # ignore everything else }

      -qro             { set target_string_literals_are_readonly 1 }
      -qnoro           { set target_string_literals_are_readonly 0 }

      -qlanglvl=ansi     { set langlvl "-D__ANSI__=1"     }
      -qlanglvl=extended { set langlvl "-D__EXTENDED__=1" }
      -qlanglvl=compat   { set langlvl "-D__COMPAT__=1"   }

      -qchars=signed   { set target_plain_char_is_unsigned 0 }
      -qchars=unsigned { set target_plain_char_is_unsigned 1 }

      -q*              { ; # ignore everything else }

      -ruvpd         -
      -nog           -
      -Q             -
      -y*            -
      -X*            -
      -V*            -
      -O*            { ; # ignore this one }


      -+ {
           # Switch languages
           if { [switch_compiler_config "cpp"] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }

      -*        { ;
# Do not pass any options beginning with '-' downstream. This is trouble.
# Either this was an sCC option that EDG does not know about or it was an
# sCC option that also happens to be an EDG option (with a potentially
# different meaning) or it was an EDG option that was forgotten to be 
# prefixed with --edg. Tough luck.
                }

      *.c       -
      *.C       -
      *.cpp     { lappend new_argv $arg }

      default   { ;
#
# This is not a known option or recognized file name.
#
                  beam::warning "Unexpected option '$arg' ignored" 
                }
     }
   }

   lappend new_argv $langlvl
#
# Set an error limit that is not likely to be exceeded.
#
   set new_argv [concat "$new_argv --error_limit 100000"]

   return $new_argv
}
