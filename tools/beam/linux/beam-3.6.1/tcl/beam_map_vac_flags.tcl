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
#        Francis Wallingford, Florian Krohm
#
#    DESCRIPTION:
#
#        This script defines '::beam::compiler::map_vac_flags'.
#
#        For VisualAge 6.x release series.
#        
#        If more than one flag from a group is given the last one wins:
#
#        (1) -qarch=
#        (2) -q32, -q64
#        (3) -qdbcs, -qmbcs, -qnombcs, -qnodbcs
#        (4) -qldbl128, -qlongdouble, -qnoldbl128, -qnolongdouble
#        (5) -qdig, -qnoddig
#        (6) -qchar=
#        (7) -qcpluscmt, -qnocpluscmt
#        (8) -qucs, -qnoucs
#
#        The -q[no]cpluscmt option, if it appears anywhere, takes precedence
#        over any implicit setting that comes with -qlanglvl=*. This is
#        contrary to the documentation (which says -q[no]cpluscmt has to
#        come after the -qlanglvl=* option) but it has been verified to
#        work as documented here on xlc 6, xlc 7, and xlc 8 as installed
#        on AIX 5.3.
#
#        Unsupported flags that change predefined macros or target
#        characteristics are:
#
#        -qarch=auto
#        -qenum=
#
#        More info is here:
#        http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.xlcpp8a.doc/compiler/ref/ruopt64b.htm
#
#
#    NOTE:
#        When adding new macros that are controlled by the mapper do not
#        forget to update function ::beam::compiler_undefine_macros near the
#        end of this file.
#
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_vac_flags)] } return
set ::guard(beam_map_vac_flags) 1

# Load utils
source beam_util
source beam_compiler_proc

# Set up routines
::beam::set_describer  ::beam::compiler::describe_vac_flags
::beam::set_mapper     ::beam::compiler::map_vac_flags
::beam::set_configurer ::beam::compiler::config_vac_flags

#######################################################################
# Command line describer for IBM's Visual Age C and C++ compilers
#######################################################################

proc ::beam::compiler::describe_vac_flags { argv } {
  # Return a list of flags and sources for the driver.
  
  set desc {}
  set forced_lang ""
  
  set len [llength $argv]
  for { set i 0 } { $i < $len } { incr i } {
    set arg [lindex $argv $i]
    
    #
    # A '-q' by itself means that its argument is separated by white space.
    #
    if { $arg == "-q" } {
      incr i
      set arg "$arg[lindex $argv $i]"
      lappend desc { ignore }
    }
    
    switch -glob -- $arg {
      -o -
      -I -
      -D -
      -U -
      -e -
      -f -
      -t -
      -Z -
      -l -
      -F {
        # Two arguments
        incr i
        lappend desc { ignore } { ignore }
      }

      -+ {
        # Switch to C++
        set forced_lang cpp
        lappend desc { ignore }
      }
      
      -* {
        # One argument
        lappend desc { ignore }
      }

      *.s {
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
# Command line argument mapper for IBM's Visual Age C and C++ compilers
#######################################################################

proc ::beam::compiler::map_vac_flags {argv} {
  
  ::beam::compiler::import

  set orig_argv $argv
  
  #
  # The macros that are being defined by the various versions of -qarch.
  # 
  set define_arch(com)   "-D_ARCH_COM=1"
  set define_arch(403)   "-D_ARCH_PPC=1 -D_ARCH_403=1"
  set define_arch(601)   "-D_ARCH_601=1"
  set define_arch(602)   "-D_ARCH_PPC=1 -D_ARCH_602=1"
  set define_arch(603)   "-D_ARCH_PPC=1 -D_ARCH_603=1 -D_ARCH_PPCGR=1"
  set define_arch(604)   "-D_ARCH_PPC=1 -D_ARCH_604=1 -D_ARCH_PPCGR=1"
  set define_arch(pwr)   "-D_ARCH_PWR=1"
  set define_arch(pwr2)  "-D_ARCH_PWR=1  -D_ARCH_PWR2=1"
  set define_arch(pwr3)  "-D_ARCH_PPC=1  -D_ARCH_PWR3=1 -D_ARCH_PPCGR=1"
  set define_arch(pwr4)  "-D_ARCH_PPC=1  -D_ARCH_PWR4=1 -D_ARCH_PPCGR=1"
  set define_arch(pwrx)  "-D_ARCH_PWR=1  -D_ARCH_PWR2=1"
  set define_arch(p2sc)  "-D_ARCH_PWR=1  -D_ARCH_PWR2=1 -D_ARCH_P2SC=1"
  set define_arch(pwr2s) "-D_ARCH_PWR=1  -D_ARCH_PWR2=1 -D_ARCH_PWR2S=1"
  set define_arch(ppc)   "-D_ARCH_PPC=1"
  set define_arch(ppc64) "-D_ARCH_PPC=1  -D_ARCH_PPC64=1"
  set define_arch(ppcgr) "-D_ARCH_PPC=1  -D_ARCH_PPCGR=1"
  set define_arch(rs64a) "-D_ARCH_PPC=1  -D_ARCH_RS64A=1"
  set define_arch(rs64b) "-D_ARCH_PPC=1  -D_ARCH_RS64B=1 -D_ARCH_PPCGR=1"
  set define_arch(rs64c) "-D_ARCH_PPC=1  -D_ARCH_RS64C=1 -D_ARCH_PPCGR=1"

  #
  # The macros that being defined by the various versions of -qlanglvl
  #
  set define_langlvl(extended) "-D__EXTENDED=1 \
                                -D__IBM_GCC__INLINE__=1 \
                                -D__IBM_REGISTER_VARS=1"
  set define_langlvl(saa)      "-D__SAA__=1 \
                                -D__SAA_L2__=1"
  set define_langlvl(saal2)    "-D__SAA_L2__=1"
  set define_langlvl(classic)  "-D__CLASSIC__=1"
  set define_langlvl(extc89)   "-D__IBM_GCC__INLINE__=1 \
                                -D__IBM_REGISTER_VARS=1"

  if { $::beam::lang == "c" } {
    set define_langlvl(ansi)     "-D__ANSI__=1 \
                                  -D__STDC__=1 \
                                  -D__STDC_VERSION__=199409L"
    set define_langlvl(extc99)   "-D__EXTENDED=1"
    set define_langlvl(stdc99)   "-D__C99_FLEXIBLE_ARRAY_MEMBER=1 \
                                  -D__STDC__=1 \
                                  -D__STDC_HOSTED__=1 \
                                  -D__STDC_VERSION__=199901L"
  } else {
    set define_langlvl(ansi)     "-D__ANSI__=1"
    set define_langlvl(extc99)   "-D__C99_FLEXIBLE_ARRAY_MEMBER=1 \
                                  -D__IBM_GCC__INLINE__=1
                                  -D__IBM_REGISTER_VARS=1"
    set define_langlvl(stdc99)   "-D__ANSI__=1"
  }

  #
  # 32bit / 64bit
  #
  set define_bits(32) "-D_ILP32=1"
  set define_bits(64) "-D__64BIT__=1"

  #
  # signed / unsigned char
  #
  set define_chars(signed)   "-D_CHAR_SIGNED=1"
  set define_chars(unsigned) "-D_CHAR_UNSIGNED=1"

  #
  # C++ style comment
  #
  set define_cpluscmt(yes) "-D__C99_CPLUSCMT=1"
  set define_cpluscmt(no)  ""

  #
  # digraphs
  #
  set define_digraphs(yes) "-D__DIGRAPHS__=1"
  set define_digraphs(no)  ""

  #
  # Unicode stuff
  #
  set define_ucs(yes) "-D__C99_UCN=1"
  set define_ucs(no)  ""

  #
  # Initialize a few variables that are used during argument parsing
  #
  set qidirfirst_seen 0
  set arch "com"
  set bits "32"
  set ldbl ""
  set heapdebug ""
  set digraphs "no"
  set langlvl "ansi"
  set chars "unsigned"
  set align "unchanged"
  if { $::beam::lang == "c" } {
    set dbcs ""
    set cpluscmt "no"
  } else {
    set dbcs "-D__DBCS__=1"
    set cpluscmt "yes"
  }
  set cpluscmt_seen 0
  set longlong "-D_LONG_LONG=1"
  set restrict "";
  set ucs "";     # not set
  set rtti "";
  
  # Remember the source file for -qidirfirst below
  set srcfile ""

  # Undefine any macros that are managed by the mapper
  ::beam::compiler::undefine_macros

  # The var new_argv will hold the new command line, which will
  # be returned.
  
  set new_argv {}
  
  #
  # Load up additional arguments from the xlc configuration file
  #
  if { $::beam::lang == "c" } {
    set xlc_opts [get_xlc_config_opts $argv /etc/vac.cfg xlc]
  } elseif { $::beam::lang == "cpp" } {
    set xlc_opts [get_xlc_config_opts $argv /etc/vac.cfg xlC]
  } else {
    beam::internal_error "Bad language - not C or C++"
  }
  
  #
  # Prepend any options from the compiler's configuration file
  #
  set argv [concat $xlc_opts $argv]

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

      --gcc=*   { beam::warning "--gcc is not recognized" }
      
      --edg=*   { list_add new_argv [unescape_arg [string range $arg 6 end]] }

      --*       { ;
  # Do not pass any options beginning with '--' downstream. Most likely
  # this was an EDG option that was forgotten to be # prefixed with --edg. 
  # Tough luck. This will be educational ;)
                }

      -F        { ;
  # The -F flag is handled by "get_xlc_config_opts". So we skip its argument. 
                  incr i
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
      -f -
      -t -
      -Z -
      -l        { incr i;  # Ignore flag and the argument that follows it }

      -F* -
      -e* -
      -f* -
      -t* -
      -Z* -
      -l* -
      -B* -
      -O*       { ; # Ignore this flag }

      -qro             { set target_string_literals_are_readonly 1 }
      -qnoro           { set target_string_literals_are_readonly 0 }

      -qalign=*        { regsub ^-qalign= $arg "" align }

      -qdollar         { set language_allow_dollar_in_id_chars 1 }
      -qnodollar       { set language_allow_dollar_in_id_chars 0 }

      -qbit*=signed    { set target_plain_int_bit_field_is_unsigned 0 }
      -qbit*=unsigned  { set target_plain_int_bit_field_is_unsigned 1 }

      -qchar*=signed   { set target_plain_char_is_unsigned 0
                         set chars "signed"
                       }
      -qchar*=unsigned { set target_plain_char_is_unsigned 1
                         set chars "unsigned"
                       }

      -qcpluscmt       { set language_end_of_line_comments_allowed 1
	                 set cpluscmt "yes"
                         set cpluscmt_seen 1
                       }
      -qnocpluscmt     { set language_end_of_line_comments_allowed 0
                         set cpluscmt "no"
                         set cpluscmt_seen 1
                       }

      -qdig*           { set digraphs "yes" }
      -qnodig*         { set digraphs "no"  }

      -qheapdebug      { ;
  # Contrary to the documentation there is no -qnoheapdebug flag
	                 set heapdebug "-D__DEBUG_ALLOC__=1" 
                       }

      -qidirf*         { set qidirfirst_seen 1 }
      -qnoidirf*       { set qidirfirst_seen 0 }

      -qnostdinc       { set system_include_path {} }

      -qlang*=stdc89   -
      -qlang*=compat   -
      -qlang*=ansi     { set langlvl "ansi"
                         set c99_mode 0
                       }

      -qlang*=extc89   { set langlvl "extc89"
                         set c99_mode 0
                       }

      -qlang*=extc99   { set langlvl "extc99"
                         if { $::beam::lang == "cpp" } {
                           set ucs "yes"
                           set digraphs "yes"
			 } else {
                           set c99_mode 1
                         }
                         if { $cpluscmt_seen == 0 } {
                           set language_end_of_line_comments_allowed 1
                           set cpluscmt "yes" 
                         }
                       }

      -qlang*=ext*     { set langlvl "extended"
                         set c99_mode 0
                       }

      -qlang*=saa      { set langlvl "saa"
                         set c99_mode 0
                       }

      -qlang*=saal2    { set langlvl "saal2"
                         set c99_mode 0
                       }

      -qlang*=cla*     { set langlvl "classic"
                         set c99_mode 0
                         if { $::beam::lang == "c" } {
                           set language_dialect "old_c"
                         }
                       }

      -qlang*=stdc99   { if { $::beam::lang == "c" } {
	                   set langlvl "stdc99"
                           set digraphs "yes"
                           set c99_mode 1
			 } else {
                           set ucs "yes"
                           set digraphs "yes"
			 }
                         if { $cpluscmt_seen == 0 } {
                           set language_end_of_line_comments_allowed 1
                           set cpluscmt "yes" 
                         }
                       }

      -qlang*=ucs      { set ucs "yes"
                         if { $::beam::lang == "c" } {
                           set c99_mode 1
                         }
                       }

      -qlang*=noucs    { set ucs "no" 
                         set c99_mode 0
                       }

      -qldbl128        -
      -qlongdouble     { set ldbl "-D__LONGDOUBLE128=1"
                         set target_sizeof_long_double 16
                         # does not affect alignment
                       }

      -qnoldbl128      -
      -qnolongdouble   { set ldbl ""
                         set target_sizeof_long_double 8
                       }

      -qlonglong       { set longlong "-D_LONG_LONG" }
      -qnolonglong     { set longlong "" }
      
      -qdbcs           -
      -qmbcs           { set dbcs "-D__DBCS__=1" }

      -qnombcs         -
      -qnodbcs         { set dbcs "" }

      -qarch=ppc64     { set arch "ppc64"
                         set bits "64"
                       }
      -qarch=*         { regsub ^-qarch= $arg "" arch }

      -q64             { set bits "64" }
      -q32             { set bits "32" }

      -qcinc*          { regsub ^-qcinc\[^=\]*= $arg "" file
	                 lappend system_include_path $file
	               }

      -qrtti=all       -
      -qrtti=dyna*     { set rtti "-D__RTTI_DYNAMIC_CAST__=1" }

      -qnortti         { set rtti "" }

      -qkeyword=restrict {
        set restrict "-D__C99_RESTRICT=1"
        set language_restrict_keyword_enabled {1}
      }

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
      *.cc             {
                         # Remember the source file. There will be only one
                         # because of how the driver invokes us.
                         set srcfile $arg
                         lappend new_argv $arg
                       }

      *.o              - 
      *.a              - 
      *.so             { beam::warning "File '$arg' will be ignored" }

      default          { beam::warning "Unexpected option '$arg' ignored" }
    }
  }

#
# Change sizes / alignof
#
  if { $bits == "64" } {
    set target_sizeof_long      8
    set target_sizeof_pointer   8
    set target_sizeof_size_t    8
    set target_sizeof_wchar_t   4
    set target_alignof_long     8
    set target_alignof_pointer  8
    set target_alignof_size_t   8
    set target_alignof_wchar_t  4
    set target_size_t_int_kind  {unsigned long}
    set target_wchar_t_int_kind {unsigned int}
  }

#
# If -qalign was given it takes precedence over the alignments from -q32/64
#
  switch -glob -- $align {
      twobyte    -
      mac68k     { set target_alignof_int         2
                   set target_alignof_long        2
                   set target_alignof_long_long   2
                   set target_alignof_float       2
                   set target_alignof_double      2
                   set target_alignof_long_double 2
                   set target_alignof_pointer     2
                   set target_alignof_size_t      2
	         }
      packed     { set target_alignof_short       1
                   set target_alignof_int         1
                   set target_alignof_long        1
                   set target_alignof_long_long   1
                   set target_alignof_float       1
                   set target_alignof_double      1
                   set target_alignof_long_double 1
                   set target_alignof_pointer     1
                   set target_alignof_size_t      1
                   set target_alignof_wchar_t     1
	         }
      bit_packed { set target_alignof_short       1
                   set target_alignof_int         1
                   set target_alignof_long        1
                   set target_alignof_long_long   1
                   set target_alignof_float       1
                   set target_alignof_double      1
                   set target_alignof_long_double 1
                   set target_alignof_pointer     1
                   set target_alignof_size_t      1
                   set target_alignof_wchar_t     1
# FIXME: additional stuff for bitfields:
# FIXME: bitfield data is packed on a bit-wise basis without respect 
# FIXME: to byte boundaries
	         }
     natural     { set target_alignof_double      8
                   set target_alignof_long_double 8
                 }
  }

#
# qlanglvl=stdc99 switches ucs on, unless it was explicitly switched off
#
  if { $langlvl == "stdc99" && $ucs == "" } {
    set ucs "yes"
  }
  if {$ucs == "" } {
    set ucs "no"
  }

#
# Add flags for the -qarch, -q32, -q64
#
  if { [::info exists define_arch($arch)] } {
    set new_argv [concat $new_argv $define_arch($arch)]
  }
  set new_argv [concat $new_argv \
                       $define_bits($bits)   \
                       $define_chars($chars) \
		       $define_ucs($ucs)     \
		       $define_cpluscmt($cpluscmt) \
		       $define_digraphs($digraphs) \
		       $define_langlvl($langlvl)   \
                       $dbcs                 \
                       $ldbl                 \
		       $heapdebug            \
		       $longlong             \
                       $restrict             \
		       $rtti                 \
	       ]
#
# For C++, __STDC__ is always defined to be 0.
#
  if { $::beam::lang == "cpp" } {
    lappend new_argv "-D__STDC__=0"
  }

#
# Special handling for -qidirfirst.
#
  if { $qidirfirst_seen == 1 } {
    set srcdir [beam::dirname $srcfile]
    set new_argv [concat {-I-} $new_argv [list "-I$srcdir"]]
  }

  return $new_argv
}


proc ::beam::compiler::undefine_predefined {macro} {

 if { [ ::info exists ::beam::compiler::${beam::lang}::predefined_macro($macro) ] } {
    unset ::beam::compiler::${beam::lang}::predefined_macro($macro)
  }
}


#
# This function undefines macros that are controlled by the mapper.
# It is possible that beam_configure has defined some of them but we cannot
# have beam_configure interfere here. The dependencies are too complex.
# So we just undefine those macros here.
#
proc ::beam::compiler::undefine_macros {} {

  if { [::info exists ::beam::compiler::${beam::lang}::generated_by_beam_configure] } {
    ::beam::compiler::undefine_predefined "_ARCH_COM";
    ::beam::compiler::undefine_predefined "_ARCH_PPC";
    ::beam::compiler::undefine_predefined "_ARCH_403";
    ::beam::compiler::undefine_predefined "_ARCH_601";
    ::beam::compiler::undefine_predefined "_ARCH_602";
    ::beam::compiler::undefine_predefined "_ARCH_603";
    ::beam::compiler::undefine_predefined "_ARCH_604";
    ::beam::compiler::undefine_predefined "_ARCH_PPCGR";
    ::beam::compiler::undefine_predefined "_ARCH_PWR";
    ::beam::compiler::undefine_predefined "_ARCH_PWR2";
    ::beam::compiler::undefine_predefined "_ARCH_PWR3";
    ::beam::compiler::undefine_predefined "_ARCH_PWR4";
    ::beam::compiler::undefine_predefined "_ARCH_P2SC";
    ::beam::compiler::undefine_predefined "_ARCH_PWR2S";
    ::beam::compiler::undefine_predefined "_ARCH_PPC64";
    ::beam::compiler::undefine_predefined "_ARCH_RS64A";
    ::beam::compiler::undefine_predefined "_ARCH_RS64B";
    ::beam::compiler::undefine_predefined "_ARCH_RS64C";
    ::beam::compiler::undefine_predefined "__EXTENDED";
    ::beam::compiler::undefine_predefined "__IBM_GCC__INLINE__";
    ::beam::compiler::undefine_predefined "__IBM_REGISTER_VARS";
    ::beam::compiler::undefine_predefined "__SAA__";
    ::beam::compiler::undefine_predefined "__SAA_L2__";
    ::beam::compiler::undefine_predefined "__CLASSIC__";
    ::beam::compiler::undefine_predefined "__ANSI__";
    ::beam::compiler::undefine_predefined "__STDC__";
    ::beam::compiler::undefine_predefined "__STDC_VERSION__";
    ::beam::compiler::undefine_predefined "__STDC_HOSTED__";
    ::beam::compiler::undefine_predefined "__C99_FLEXIBLE_ARRAY_MEMBER";
    ::beam::compiler::undefine_predefined "_ILP32";
    ::beam::compiler::undefine_predefined "__64BIT__";
    ::beam::compiler::undefine_predefined "_CHAR_SIGNED";
    ::beam::compiler::undefine_predefined "_CHAR_UNSIGNED";
    ::beam::compiler::undefine_predefined "__C99_CPLUSCMT";
    ::beam::compiler::undefine_predefined "__DIGRAPHS__";
    ::beam::compiler::undefine_predefined "__C99_UCN";
    ::beam::compiler::undefine_predefined "__LONGDOUBLE128";
    ::beam::compiler::undefine_predefined "__DEBUG_ALLOC__";
    ::beam::compiler::undefine_predefined "__RTTI_DYNAMIC_CAST__";
    ::beam::compiler::undefine_predefined "__C99_RESTRICT";
    ::beam::compiler::undefine_predefined "__DBCS__";
    ::beam::compiler::undefine_predefined "_LONG_LONG";
    
    # Also ensure __STDC__ isn't set by EDG for predefined macros.
    lappend ::beam::compiler::${beam::lang}::standard_predefined_macros - __STDC__
  }
}

#
# Return which flags should trigger reconfiguration of vac.
#
proc ::beam::compiler::config_vac_flags { argv } {
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
    
    switch -exact -- $arg {
      -+   -
      -q32 -
      -q64 { lappend new_argv $arg }
      
      -F {
        lappend new_argv $arg
        incr i
        lappend new_argv [lindex $argv $i]
      }

      -o -
      -I -
      -D -
      -U -
      -e -
      -f -
      -t -
      -Z -
      -l { incr i;  # Ignore flag and the argument that follows it }

    }
  }
  
  return $new_argv
}
