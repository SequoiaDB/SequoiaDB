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
#        When parsing the command line options we maintain two associative
#        arrays: define() and undefine(). For every key in 'define' a
#        -D command line option will be generated and for every key in 
#        'undefine' a  -U command line option will be generated.
#
#        FIXME: we should recognize this because there has been instances
#               of code containing sequence numbers in the past.
#        -Wc,sequence(left,right)  specifies the columns to be holding
#        sequence numbers. 'right' might be '*' (without the quotes) meaning
#        the rest of the line.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#
#	 12/21/05  florian  Created

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_zosvac_flags)] } return
set ::guard(beam_map_zosvac_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper ::beam::compiler::map_zosvac_flags

#
# OPTIONS is -Wc,arg1[,arg2,.....]  or c,arg1[,arg2,...]
# Split a -Wc option list on the commas. This is tricky because inside
# a single option, a comma may appear in balanced parenthesis, and that
# comma is not an option delimiter, but is part of the option itself.
# No spaces are allowed. The compiler won't accept it. Good!
#
# -Wc,arg1[,arg2,...,argN] where each arg may be "arg" or "arg(a[,b,c,...)".
#
# This returns a list: { arg1 arg2 ... argN }
#
proc ::beam::compiler::split_zosvac_Wc { options } {
  regsub -- {^.*?,} $options {} options ;# Ignore opt before first comma

  set result {}
  set current {}
  set len [string length $options]

  for { set i 0 } { $i < $len } { incr i } {
    set c [string index $options $i]

    if { $c == {(} } {
      # Accumulate to ")", including commas
      append current $c
      while { $c != {)} } {
        incr i
        set c [string index $options $i]
        append current $c
      }
    } elseif { $c == {,} } {
      # Split here and start a new arg
      lappend result $current
      set current {}
    } else {
      # Accumulate everything else
      append current $c
    }
  }

  # Append the final arg
  lappend result $current

  return $result
}

#
#
# We walk over all the arguments, if any. Arguments are case insensitive.
# Therefore, they get converted to lower case first.
#
proc ::beam::compiler::map_zosvac_Wc_for_c { options nargs } {
  upvar $nargs nargv; # by reference

  ::beam::compiler::import

  set options [string tolower $options]
  set optlist [::beam::compiler::split_zosvac_Wc $options]

  foreach arg $optlist {
    switch -glob -- $arg {

        lp64 { set define(_LP64)      1 
               set define(__64BIT__)  1
               set define(__BFP__)    1
               set define(__GOFF__)   1
               set define(__XPLINK__) 1
               set define(__SIZE_TYPE__) {unsigned long}
               set undefine(_ILP32)   1
               set target_sizeof_long     8
               set target_alignof_long    8
               set target_sizeof_pointer  8
               set target_alignof_pointer 8
	     }

        xpl*   { set define(__GOFF__)   1 
                 set define(__XPLINK__) 1
	       }

        arch*(0) { set define(__ARCH__) 0 }
        arch*(1) { set define(__ARCH__) 1 }
        arch*(2) { set define(__ARCH__) 2 }
        arch*(3) { set define(__ARCH__) 3 }
        arch*(4) { set define(__ARCH__) 4 }
        arch*(5) { set define(__ARCH__) 5 }
        arch*(6) { set define(__ARCH__) 6 
                   set define(__TUNE__) 6
                 }
        arch*(7) { set define(__ARCH__) 7 
                   set define(__TUNE__) 7
                 }

        ascii   { set define(__CHARSET_LIB) 1
                  set define(__GOFF__)      1
                  set define(__XPLINK__)    1
	        }
        bitfield(signed)   { set target_plain_int_bit_field_is_unsigned 0 }
        bitfield(unsigned) { set target_plain_int_bit_field_is_unsigned 1 }
        chars(signed)      { set target_plain_char_is_unsigned 0 
                             set undefine(_CHAR_UNSIGNED) 1
                             set undefine(__CHAR_UNSIGNED__) 1
                             set define(_CHAR_SIGNED) 1
                           }
        chars(unsigned)    { set target_plain_char_is_unsigned 1 }

        digr*       { set undefine(__DIGRAPHS__) 1 }

        dll         -
        dll(cba)    { set define(__DLL__) 1 }

        float(ieee) { set define(__BFP__) 1 }

        goff        { set define(__GOFF__) 1 }

        igner*      { set define(__IGNERRNO__) 1 }

        loc*        { set define(__LOCALE__) "" }
        
        longname    { set define(__LONGNAME__) 1 }

        nodigr*     { set undefine(__DIGRAPHS__) 1 }

        noloc*      { set undefine(__CODESET__) 1
                      set undefine(__LOCALE__)  1
                    }
        nolo*       { set undefine(__LONGNAME__) 1 }

        ss*         { set language_end_of_line_comments_allowed   1
                      set define(__C99_CPLUSCMT) 1 
                    }

        lang*(extended) { set define(_DECIMAL)     1
                          set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__BOOL__)     1
                          set define(__EXTENDED__) 1
                          set define(__C99_UCN)    1
                          set define(__C99_FUNC__) 1
                          set define(__C99_FLEXIBLE_ARRAY_MEMBER)   1

                          set undefine(__ANSI__)   1
                          set undefine(__STDC__)   1
                          set undefine(__STDC_VERSION__) 1
                          set gnu_mode 30300
                          set language_end_of_line_comments_allowed 1
                          set language_trigraphs_allowed 1
	                }

        lang*(extc99)   { # Note: Even though this file seems to be V1R7,
                          # this section was written using the V1R9 compiler.
                          # If it's ever a problem that these mix in the same
                          # mapper, we'll have to make versioned mappers.
                          set define(_DECIMAL)     1
                          set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__BOOL__)     1
                          set define(__C99_UCN)    1
                          set define(__C99_FLEXIBLE_ARRAY_MEMBER)   1

                          set undefine(__ANSI__)   1

                          set c99_mode 1
                          set language_end_of_line_comments_allowed 1
                        }

        lang*(stdc99)   { set c99_mode 1 }

        lang*(commonc)  { set define(_DECIMAL)     1
                          set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__COMMONC__)  1
                          set define(__EXTENDED__) 1

                          set undefine(__ANSI__)   1
                          set undefine(__STDC__)   1
                          set undefine(__STDC_VERSION__)   1
                          if { [::info exists c99_mode] } { unset c99_mode }
	                }

        lang*(saa)      { set define(_DECIMAL)     1
                          set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__SAA_L2__)   1
                          set define(__SAA__)      1
                          set define(__EXTENDED__) 1

                          set undefine(__ANSI__)   1
                          set undefine(__STDC__)   1
                          set undefine(__STDC_VERSION__)   1
                          if { [::info exists c99_mode] } { unset c99_mode }
	                }

        lang*(saal2)    { set define(_DECIMAL)     1
                          set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__SAA_L2__)   1
                          set define(__EXTENDED__) 1

                          set undefine(__ANSI__)   1
                          set undefine(__STDC__)   1
                          set undefine(__STDC_VERSION__)   1
                          if { [::info exists c99_mode] } { unset c99_mode }
	                }

        lang*(libext)   { set define(_EXT) 1
                          set define(_LONG_LONG) 1
                          if { [::info exists c99_mode] } { unset c99_mode }
                        }

        lang*(longlong) { set define(_LONG_LONG) 1 
                          if { [::info exists c99_mode] } { unset c99_mode }
                        }

        targ*(zosv1r2)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41020000
	                }
        targ*(zosv1r3)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41030000
	                }
        targ*(zosv1r4)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41040000
	                }
        targ*(zosv1r5)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41050000
	                }
        targ*(zosv1r6)  { set define(__TARGET_LIB__) 0x41060000 }

        targ*(le,*)     { regsub -- {^targ.*\(le,} $arg {} tmp
                          regsub -- {\)$} $tmp {} tmp
                          set define(__TARGET_LIB__) $tmp
                        }

        tun*(0) { set define(__TUNE__) 0 }
        tun*(1) { set define(__TUNE__) 1 }
        tun*(2) { set define(__TUNE__) 2 }
        tun*(3) { set define(__TUNE__) 3 }
        tun*(4) { set define(__TUNE__) 4 }
        tun*(5) { set define(__TUNE__) 5 }
        tun*(6) { set define(__TUNE__) 6 }
        tun*(7) { set define(__TUNE__) 7 }
    }
  }

  if { [::info exists define] } {
    foreach m [array names define] {
       lappend nargv -D$m=$define($m)
    }
  }
  if { [::info exists undefine] } {
    foreach m [array names undefine] {
       lappend nargv -U$m
    }
  }

  return $nargv
}

proc ::beam::compiler::map_zosvac_Wc_for_cxx { options nargs } {
  upvar $nargs nargv; # by reference

  ::beam::compiler::import

  set options [string tolower $options]
  set optlist [::beam::compiler::split_zosvac_Wc $options]

  foreach arg $optlist {
    switch -glob -- $arg {

        lp64     { set define(_LP64)      1 
                   set define(__64BIT__)  1
                   set define(__BFP__)    1
                   set define(__GOFF__)   1
                   set define(__OBJECT_MODEL_IBM__) 1
                   set define(__XPLINK__) 1
                   set undefine(_ILP32)   1
                   set target_sizeof_long     8
                   set target_alignof_long    8
                   set target_sizeof_pointer  8
                   set target_alignof_pointer 8
	         }

        xpl*     { set define(__GOFF__)   1 
                   set define(__XPLINK__) 1
	         }

        arch*(0) { set define(__ARCH__) 0 }
        arch*(1) { set define(__ARCH__) 1 }
        arch*(2) { set define(__ARCH__) 2 }
        arch*(3) { set define(__ARCH__) 3 }
        arch*(4) { set define(__ARCH__) 4 }
        arch*(5) { set define(__ARCH__) 5 }
        arch*(6) { set define(__ARCH__) 6 
                   set define(__TUNE__) 6
                 }
        arch*(7) { set define(__ARCH__) 7 
                   set define(__TUNE__) 7
                 }

        ascii    { set define(__CHARSET_LIB) 1
                   set define(__GOFF__)      1
                   set define(__XPLINK__)    1
	         }

        bitfield(signed)   { set target_plain_int_bit_field_is_unsigned 0 }
        bitfield(unsigned) { set target_plain_int_bit_field_is_unsigned 1 }
        chars(signed)      { set target_plain_char_is_unsigned 0 
                             set undefine(_CHAR_UNSIGNED) 1
                             set undefine(__CHAR_UNSIGNED__) 1
                             set define(_CHAR_SIGNED) 1
                           }
        chars(unsigned)    { set target_plain_char_is_unsigned 1 }

        float(ieee) { set define(__BFP__) 1 }

        goff        { set define(__GOFF__) 1 }

        igner*      { set define(__IGNERRNO__)    1 }

        loc*        { set define(__LOCALE__)     "" }

        nodigr*     { set undefine(__DIGRAPHS__)  1 }

        noexh       { set undefine(_CPPUNWIND)    1 
                      set undefine(__EXCEPTIONS__ 1
                    }

        lib*        { set define(__LIBANSI__)     1 }

        noloc*      { set undefine(__CODESET__)   1
                      set undefine(__LOCALE__)    1
                    }

        rtti*       { set __RTII_DYNAMIC_CAST__)  1 }

        notempinc   { set undefine(__TEMPINC__)   1 }

        lang*(extended) { set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__EXTENDED__) 1
                          set define(__RTTI_DYNAMIC_CAST__) 1
                          set language_trigraphs_allowed 1
	                }

        lang*(strict98) -
        lang*(ansi)     { set define(__RTTI_DYNAMIC_CAST__) 1 }

        lang*(compat92) { set define(_EXT)         1
                          set define(_LONG_LONG)   1
                          set define(__COMPATMATH__)  1
                          set define(__EXTENDED__) 1

                          set undefine(__BOOL__)   1
	                }

        lang*(dbcs)     { set define(__DBCS__) 1  }

        lang*(libext)   { set define(_EXT) 1  }

        lang*(longlong) { set define(_LONG_LONG) 1 }

        lang*(oldmath)  { set define(__COMPATMATH__) 1 }

        targ*(zosv1r2)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41020000
	                }
        targ*(zosv1r3)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41030000
	                }
        targ*(zosv1r4)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41040000
	                }
        targ*(zosv1r5)  { set define(__ARCH__) 2
                          set define(__TARGET_LIB__) 0x41050000
	                }
        targ*(zosv1r6)  { set define(__TARGET_LIB__) 0x41060000 }

        tun*(0) { set define(__TUNE__) 0 }
        tun*(1) { set define(__TUNE__) 1 }
        tun*(2) { set define(__TUNE__) 2 }
        tun*(3) { set define(__TUNE__) 3 }
        tun*(4) { set define(__TUNE__) 4 }
        tun*(5) { set define(__TUNE__) 5 }
        tun*(6) { set define(__TUNE__) 6 }
        tun*(7) { set define(__TUNE__) 7 }
    }
  }

  if { [::info exists define] } {
    foreach m [array names define] {
      lappend nargv -D$m=$define($m)
    }
  }
  if { [::info exists undefine] } {
    foreach m [array names undefine] {
      lappend nargv -U$m
    }
  }

  return $nargv
}



proc ::beam::compiler::map_zosvac_flags {argv} {
  
  ::beam::compiler::import

  set orig_argv $argv
  
  # The var new_argv will hold the new command line, which will
  # be returned.
  
  set new_argv {}
  
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
  # this was an EDG option that was forgotten to be # prefixed with --edg. 
  # Tough luck. This will be educational ;)
                }

      -W0* -
      -Wc*      { if { $::beam::lang == "c" } {
                   ::beam::compiler::map_zosvac_Wc_for_c   $arg new_argv
                  } else {
                   ::beam::compiler::map_zosvac_Wc_for_cxx $arg new_argv
                  }
                }

      -W        { incr i
	          set arg [lindex $argv $i]
	          if { $::beam::lang == "c" } {
                   ::beam::compiler::map_zosvac_Wc_for_c   $arg new_argv
                  } else {
                   ::beam::compiler::map_zosvac_Wc_for_cxx $arg new_argv
                  }
                }

      -o -
      -I -
      -D -
      -U        { lappend new_argv $arg 
	          incr i
                  lappend new_argv [lindex $argv $i] 
                }

      -o* -
      -I* -
      -D* -
      -U* -
      -E        { lappend new_argv $arg }

      -e -
      -u -
      -l -
      -L        { incr i;  # Ignore flag and the argument that follows it }

      -+ {
           # Switch languages
           if { [switch_compiler_config "cpp"] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }        

      -*               { ; 
# All other flags beginning with '-' are ignored.
                       }

      *.s              -
      *.c              -
      *.C              -
      *.cpp            -
      *.cxx            -
      *.cc             { lappend new_argv $arg }

      *.o              - 
      *.a              - 
      *.so             { beam::warning "File '$arg' will be ignored" }

      default          { beam::warning "Unexpected option '$arg' ignored" }
    }
  }


  return $new_argv
}
