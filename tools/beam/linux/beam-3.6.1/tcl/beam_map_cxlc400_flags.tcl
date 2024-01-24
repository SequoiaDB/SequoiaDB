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
#        This script contains the mappers for the cxlc/C400 compilers
#        
#        Several -qarch=   --> first one wins.
#        Several -qdigraph -qnodigraph --> first one wins
#        Several -qlonglong -qnolonglong --> first one wins
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.
#
#        04/24/05  florian  Created
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_map_cxlc400_flags)] } return
set ::guard(beam_map_cxlc400_flags) 1

# Load utils.
source beam_compiler_proc

# Set this as the default mapper unless one is loaded.
::beam::set_mapper ::beam::compiler::map_cxlc400_flags

proc ::beam::compiler::map_cxlc400_flags {argv} {
  if { $::beam::lang == "c" } {
    return [::beam::compiler::map_cxlc400_c_flags $argv]
  } else {
    return [::beam::compiler::map_cxlc400_cpp_flags $argv]
  }
}

proc ::beam::compiler::map_cxlc400_c_flags {argv} {
  ::beam::compiler::import

  set orig_argv $argv
   
  set new_argv {}

#
# The macros for -qasyncsignal
#
  set define_asyncsignal(default) ""
  set define_asyncsignal(yes)     "-D__ASYNC_SIG__=1"
  
#
# The macros for -qlocale=....
#
  set define_locale(default)     ""
  set define_locale(locale)      "-D__POSIX_LOCALE__=1"
  set define_locale(localeucs2)  "-D__POSIX_LOCALE__=1 -D__UCS2__=1"
  set define_locale(localeutf)   "-D__POSIX_LOCALE__=1 -D__UTF32__=1"
  
#
# The macros for -qteraspace=....
#
  set define_teraspace(default)     ""
  set define_teraspace(no)          ""
  set define_teraspace(notsifc)     ""
  set define_teraspace(tsifc)       "-D__TERASPACE__=1"

#
# The macros for -qdigraph -qnodigraph
#
  set define_digraph(default) ""
  set define_digraph(yes)     "-D__DIGRAPHS__=1"
  set define_digraph(no)      ""

#
# The macros for -qdbcs -qmbcs
#
  set define_dbcs(default) ""
  set define_dbcs(yes)     "-D__DBCS__=1"
  set define_dbcs(no)      ""

#
# The macros for -qchars=
#
  set define_chars(default)  "-D_CHAR_UNSIGNED=1"
  set define_chars(unsigned) "-D_CHAR_UNSIGNED=1"
  set define_chars(signed)   "-D_CHAR_SIGNED=1"

#
# The macros for -qcpluscmt
#
  set define_cpluscmt(default)  ""
  set define_cpluscmt(yes)      "-D__C99_CPLUSCMT=1"
  set define_cpluscmt(no)       ""

#
# The macros for -qlanglvl=
#
   set define_langlvl(ansi)     "-D__ANSI__=1 -D__STDC__=1 \
                                 -D__STDC_VERSION__=199409L"
   set define_langlvl(extc89)   "-D__EXTENDED__=1"
   set define_langlvl(extc99)   "-D__EXTENDED__=1 -D__STDC_HOSTED__=1"
   set define_langlvl(extended) "-D__EXTENDED__=1 -D__STDC_VERSION__=0"
   set define_langlvl(saa)      "-D__ANSI__=1 -D__STDC__=1 \
                                 -D__STDC_VERSION__=199409L -D__SAA__=1 \
                                 -D__SAA_L2__=1"
   set define_langlvl(saal2)    "-D__ANSI__=1 -D__STDC__=1 \
                                 -D__STDC_VERSION__=199409L -D__SAA_L2__=1"
   set define_langlvl(stdc89)   "-D__ANSI__=1 -D__STDC__=1 \
                                 -D__STDC_VERSION__=199409L"
   set define_langlvl(stdc99)   "-D__ANSI__=1 -D__STDC__=1 \
                                 -D__STDC_VERSION__=199901L \
                                 -D__STDC_HOSTED__=1 \
                                 -D__C99_FLEXIBLE_ARRAY_MEMBER -D__C99_UCN"
   set define_langlvl(default)  $define_langlvl(extended)

#
# The macros for -qdatamodel=
#
   set define_datamodel(default)  ""
   set define_datamodel(llp64)    "-D__LLP64_IFC__=1"
   set define_datamodel(p128)       ""

#
# The macros for -qifs=
#
   set define_ifs(default)  ""
   set define_ifs(noifs)    ""
   set define_ifs(ifs)      "-D__IFS_IO__=1"
   set define_ifs(ifs=64)   "-D__IFS_IO__=1 -D__IFS64_IO__=1"

#
# Initialize a few variables that are used during argument parsing
#
   set arch        "default"
   set asyncsignal "default"
   set chars       "default"
   set cpluscmt    "default"
   set datamodel   "default"
   set dbcs        "default"
   set digraph     "default"
   set ifs         "default"
   set langlvl     "default"
   set locale      "default"
   set teraspace   "default"
   set qidirfirst_seen 0
   
  # Remember the source file for -qidirfirst below
  set srcfile ""

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

      -o=*      { ;
# Translate -o=something into -o something
	          regsub ^-o= $arg "" file
                  lappend new_argv "-o" $file 
                }

      -P        { lappend new_argv "-E" }

      -I        -
      -D        -
      -U        { lappend new_argv $arg
                  incr i
                  lappend new_argv [lindex $argv $i] 
                }

      -I*       -
      -D*       -
      -U*       -
      -E        { lappend new_argv $arg }

      -+ {
           # Switch languages
           if { [switch_compiler_config "cpp"] } {
             return [::beam::compiler::map_flags $orig_argv]
           }
         }        

      -qro             { set target_string_literals_are_readonly 1 }
      -qnoro           { set target_string_literals_are_readonly 0 }

      -qasyncsignal    { if { $asyncsignal == "default" } {
                           set asyncsignal "yes"
                         }
                       }

      -qbitfields=signed    { set target_plain_int_bit_field_is_unsigned 0 }
      -qbitfields=unsigned  { set target_plain_int_bit_field_is_unsigned 1 }

      -qchars=signed   { if { $chars == "default" } {
                           set chars "signed"
                           set target_plain_char_is_unsigned 0 
                         }
                       }
      -qchars=unsigned { if { $chars == "default" } {
                           set chars "unsigned"
                           set target_plain_char_is_unsigned 1
                         }
                       }

      -qdatamodel=llp64 { if { $datamodel == "default" } {
                            set datamodel "llp64"
                            set target_sizeof_pointer  8
                            set target_alignof_pointer 8
                          }
                        }

      -qdatamodel=p128  { if { $datamodel == "default" } {
                            set datamodel "llp64"
                            set target_sizeof_pointer  16
                            set target_alignof_pointer 16
                          }
                        }

      -qmbcs           -
      -qdbcs           { if { $dbcs == "default" } {
                           set dbcs "yes"
                         }
                       }
      -qnombcs         -
      -qnodbcs         { if { $dbcs == "default" } {
                           set dbcs "no"
                         }
                       }

      -qdigraph        { if { $digraph == "default" } {
	                   set digraph "yes"
                         }
                       }
      -qnodigraph      { if { $digraph == "default" } {
	                   set digraph "no"
                         }
                       } 

      -qdollar         { set language_allow_dollar_in_id_chars 1 }
      -qnodollar       { set language_allow_dollar_in_id_chars 0 }

      -qidirfirst      { set qidirfirst_seen 1 }
      -qnoidirfirst    { set qidirfirst_seen 0 }

      -qcpluscmt       { if { $cpluscmt == "default" } {
                           set cpluscmt "yes"
                         }
                       }
      -qnocpluscmt     { if { $cpluscmt == "default" } {
                           set cpluscmt "no"
                         }
                       }

      -qifs            { if { $ifs == "default" } {
                           set ifs "ifs"
                         }
                       }
      -qifs=64         { if { $ifs == "default" } {
                           set ifs "ifs=64"
                         }
                       } 
      -qnoifs          { if { $ifs == "default" } {
                           set ifs "noifs"
                         }
                       }

      -qlanglvl=ansi   { if { $langlvl == "default" } {
                           set langlvl "ansi"
                           set longlong "no"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=extc89 { if { $langlvl == "default" } {
                           set langlvl "extc89"
                           set longlong "yes"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=extc99 { if { $langlvl == "default" } {
                           set langlvl "extc99"
                           set longlong "yes"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=ext*   { if { $langlvl == "default" } {
                           set langlvl "extended"
                           set longlong "yes"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=saa    { if { $langlvl == "default" } {
                           set langlvl "saa"
                           set longlong "yes"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=saal2  { if { $langlvl == "default" } {
                           set langlvl "saal2"
                           set longlong "yes"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=stdc89 { if { $langlvl == "default" } {
                           set langlvl "stdc89"
                           set longlong "no"
                           set digraph  "no"
                           set cpluscmt "no"
                         }
                       }

      -qlanglvl=stdc99 { if { $langlvl == "default" } {
                           set langlvl "stdc99"
                           set longlong "yes"
                           set digraph  "yes"
                           set cpluscmt "yes"
                         }
                       }

      -qlocale=locale  { if { $locale == "default" } {
                           set locale "locale"
                         }
                       }

      -qlocale=localeucs2  { if { $locale == "default" } {
                           set locale "localeucs2"
                         }
                       }

      -qlocale=localeutf  { if { $locale == "default" } {
                           set locale "localeutf"
                         }
                       }

      -qteraspace=no       { if { $teraspace == "default" } {
                             set teraspace "no"
                           }
                       }

      -qteraspace=notsifc  { if { $teraspace == "default" } {
                             set teraspace "notsifc"
                           }
                       }

      -qteraspace=tsifc    { if { $teraspace == "default" } {
                             set teraspace "tsifc"
                           }
                       }

      -*               { ; 
# All other flags beginning with '-' are ignored.
                       }

      *.c              {
                         # Remember the source file. There will be only one
                         # because of how the driver invokes us.
                         set srcfile $arg
                         lappend new_argv $arg
                       }

      default          { beam::warning "Unexpected option '$arg' ignored" }
     }
   }
   set new_argv [concat $new_argv \
                        $define_chars($chars) \
                        $define_cpluscmt($cpluscmt) \
	 	        $define_datamodel($datamodel) \
		        $define_dbcs($dbcs) \
		        $define_digraph($digraph) \
		        $define_ifs($ifs) \
		        $define_langlvl($langlvl)   \
		        $define_locale($locale)   \
		        $define_teraspace($teraspace)   \
		        $define_asyncsignal($asyncsignal)   \
	        ]
#
# Special handling for -qidirfirst.
#
   if { $qidirfirst_seen == 1 } {
     set srcdir [beam::dirname $srcfile]
     set new_argv [concat {-I-} $new_argv [list "-I$srcdir"]]
   }
#
# Set an error limit that is not likely to be exceeded.
#
   set new_argv [concat "$new_argv --error_limit 100000"]

   return $new_argv
}



proc ::beam::compiler::map_cxlc400_cpp_flags {argv} {
  ::beam::compiler::import

  set new_argv {}

#
# The macros that are being defined by the various versions of -qarch.
# 
  set define_arch(default) "-D_ARCH_PPC=1"
  set define_arch(com)     "-D_ARCH_COM=1"
  set define_arch(403)     "-D_ARCH_PPC=1 -D_ARCH_403=1"
  set define_arch(601)     "-D_ARCH_PPC=1 -D_ARCH_601=1"
  set define_arch(602)     "-D_ARCH_PPC=1 -D_ARCH_602=1"
  set define_arch(603)     "-D_ARCH_PPC=1 -D_ARCH_603=1 -D_ARCH_PPCGR=1"
  set define_arch(604)     "-D_ARCH_PPC=1 -D_ARCH_604=1 -D_ARCH_PPCGR=1"
  set define_arch(pwr)     "-D_ARCH_PWR=1"
  set define_arch(pwr2)    "-D_ARCH_PWR=1 -D_ARCH_PWR2=1"
  set define_arch(pwr3)    "-D_ARCH_PPC=1 -D_ARCH_PWR3=1 -D_ARCH_PPCGR=1 \
                            -D_ARCH_PPC64=1"
  set define_arch(pwr4)    "-D_ARCH_PPC=1 -D_ARCH_PWR3 -D_ARCH_PWR4=1 \
                            -D_ARCH_PPCGR=1 -D_ARCH_PPC64"
  set define_arch(pwrx)    "-D_ARCH_PWR=1  -D_ARCH_PWR2=1"
  set define_arch(p2sc)    "-D_ARCH_PWR=1  -D_ARCH_PWR2=1 -D_ARCH_P2SC=1"
  set define_arch(pwr2s)   "-D_ARCH_PWR=1  -D_ARCH_PWR2=1 -D_ARCH_PWR2S=1"
  set define_arch(ppc)     "-D_ARCH_PPC=1"
  set define_arch(ppc64)   "-D_ARCH_PPC=1 -D_ARCH_PPC64=1"
  set define_arch(ppcgr)   "-D_ARCH_PPC=1 -D_ARCH_PPCGR=1"
  set define_arch(rs64a)   "-D_ARCH_PPC=1 -D_ARCH_RS64A=1 -D_ARCH_PPC64=1"
  set define_arch(rs64b)   "-D_ARCH_PPC=1 -D_ARCH_RS64B=1 -D_ARCH_PPCGR=1 \
                            -D_ARCH_PPC64=1"
  set define_arch(rs64c)   "-D_ARCH_PPC=1 -D_ARCH_RS64C=1 -D_ARCH_PPCGR=1 \
                            -D_ARCH_PPC64=1"

#
# The macros for -qdigraph -qnodigraph
#
  set define_digraph(default) "-D__DIGRAPHS__=1"
  set define_digraph(yes)     "-D__DIGRAPHS__=1"
  set define_digraph(no)      ""

#
# The macros for -qlonglong -qnolonglong
#
  set define_longlong(default) "-D_LONG_LONG=1"
  set define_longlong(yes)     "-D_LONG_LONG=1"
  set define_longlong(no)      ""

#
# The macros for -qdbcs -qmbcs
#
  set define_dbcs(default) ""
  set define_dbcs(yes)     "-D__DBCS__=1"
  set define_dbcs(no)      ""

#
# The macros for -qchars=
#
  set define_chars(default)  "-D_CHAR_UNSIGNED=1"
  set define_chars(unsigned) "-D_CHAR_UNSIGNED=1"
  set define_chars(signed)   "-D_CHAR_SIGNED=1"

#
# The macros for -qlanglvl=
#
   set define_langlvl(ansi)     "-D__ANSI__=1"
   set define_langlvl(extended) "-D__EXTENDED__=1 -D__C99__FUNC__=1 \
                                 -D__IBM_LOCAL_LABEL=1"
   set define_langlvl(default)  $define_langlvl(extended)

#
# The macros for -qlocale=....
#
  set define_locale(default)     ""
  set define_locale(locale)      ""
  set define_locale(localeucs2)  "-D__UCS2__=1 -D__IFS_IO__=1"
  set define_locale(localeutf)   "-D__POSIX_LOCALE__=1 -D__UTF32__=1"
  
#
# The macros for -qdatamodel=
#
   set define_datamodel(default)  ""
   set define_datamodel(llp64)    "-D__LLP64_IFC__=1"
   set define_datamodel(p128)     ""

#
# The macros for -qifs=
#
   set define_ifs(default)  ""
   set define_ifs(noifs)    ""
   set define_ifs(ifs)      "-D__IFS_IO__=1"
   set define_ifs(ifs=64)   "-D__IFS_IO__=1 -D__IFS64_IO__=1"

#
# The macros for -qteraspace=....
#
  set define_teraspace(default)     "-D__TERASPACE__=1"
  set define_teraspace(no)          ""
  set define_teraspace(notsifc)     ""
  set define_teraspace(tsifc)       "-D__TERASPACE__=1"

#
# Initialize a few variables that are used during argument parsing
#
   set arch      "default"
   set chars     "default"
   set datamodel "default"
   set dbcs      "default"
   set digraph   "default"
   set ifs       "default"
   set heapdebug    ""
   set posix_locale ""
   set langlvl   "default"
   set longlong  "default"
   set locale    "default"
   set teraspace "default"
   set qidirfirst_seen 0

   # Remember the source file for -qidirfirst below
   set srcfile ""

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

      -o=*      { ;
# Translate -o=something into -o something
	          regsub ^-o= $arg "" file
                  lappend new_argv "-o" $file 
                }

      -P        { lappend new_argv "-E" }

      -I        -
      -D        -
      -U        { lappend new_argv $arg
                  incr i
                  lappend new_argv [lindex $argv $i] 
                }

      -I*       -
      -D*       -
      -U*       -
      -E        { lappend new_argv $arg }

      -qarch=*         { if { $arch == "default" } {
                           regsub ^-qarch= $arg "" arch 
                         }
                       }

      -qro             { set target_string_literals_are_readonly 1 }
      -qnoro           { set target_string_literals_are_readonly 0 }

      -qbitfields=signed    { set target_plain_int_bit_field_is_unsigned 0 }
      -qbitfields=unsigned  { set target_plain_int_bit_field_is_unsigned 1 }

      -qchars=signed   { if { $chars == "default" } {
                           set chars "signed"
                           set target_plain_char_is_unsigned 0 
                         }
                       }
      -qchars=unsigned { if { $chars == "default" } {
                           set chars "unsigned"
                           set target_plain_char_is_unsigned 1
                         }
                       }

      -qmbcs           -
      -qdbcs           { if { $dbcs == "default" } {
                           set dbcs "yes"
                         }
                       }
      -qnombcs         -
      -qnodbcs         { if { $dbcs == "default" } {
                           set dbcs "no"
                         }
                       }

      -qdigraph        { if { $digraph == "default" } {
	                   set digraph "yes"
                         }
                       }
      -qnodigraph      { if { $digraph == "default" } {
	                   set digraph "no"
                         }
                       } 

      -qdollar         { set language_allow_dollar_in_id_chars 1 }
      -qnodollar       { set language_allow_dollar_in_id_chars 0 }

      -qheapdebug      { 
# Contrary to the documentation there is no -qboheapdebug flag
	                 set heapdebug "-D__DEBUG_ALLOC__=1" 
                       }

      -qposix_locale   { 
	                 set posix_locale "-D__POSIX_LOCALE__=1"
                       }

      -qidirfirst      { set qidirfirst_seen 1 }
      -qnoidirfirst    { set qidirfirst_seen 0 }

      -qifs            { if { $ifs == "default" } {
                           set ifs "ifs"
                         }
                       }
      -qifs=64         { if { $ifs == "default" } {
                           set ifs "ifs=64"
                         }
                       } 
      -qnoifs          { if { $ifs == "default" } {
                           set ifs "noifs"
                         }
                       }

      -qlanglvl=stdc89 -
      -qlanglvl=ansi   { if { $langlvl == "default" } {
                           set langlvl "ansi"
                           set longlong "no"
                         }
                       }

      -qlanglvl=extc89   -
      -qlanglvl=extc99   -
      -qlanglvl=stdc99   -
      -qlanglvl=saa      -
      -qlanglvl=saal2    -
      -qlanglvl=ext*     { if { $langlvl == "default" } {
                             set langlvl "extended"
                             set longlong "no"
                           }
                         }

      -qlocale=locale     { if { $locale == "default" } {
                              set locale "locale"
                            }
                          }

      -qlocale=localeucs2 { if { $locale == "default" } {
                              set locale "localeucs2"
                            }
                          }

      -qlocale=localeutf  { if { $locale == "default" } {
                              set locale "localeutf"
                            }
                          }

      -qlonglong       { if { $longlong == "default" } {
                           set longlong "yes"
                         }
                       }
      -qnolonglong     { if { $longlong == "default" } {
                           set longlong "no"
                         }
                       } 

      -qnostdinc       { set system_include_path {} }

      -qdatamodel=llp64 { if { $datamodel == "default" } {
                            set datamodel "llp64"
                            set target_sizeof_pointer  8
                            set target_alignof_pointer 8
                          }
                        }

      -qdatamodel=p128  { if { $datamodel == "default" } {
                            set datamodel "llp64"
                            set target_sizeof_pointer  16
                            set target_alignof_pointer 16
                          }
                        }

      -qteraspace=no       { if { $teraspace == "default" } {
                             set teraspace "no"
                           }
                       }

      -qteraspace=notsifc  { if { $teraspace == "default" } {
                             set teraspace "notsifc"
                           }
                       }

      -qteraspace=tsifc    { if { $teraspace == "default" } {
                             set teraspace "tsifc"
                           }
                       }

      -q64             { set target_sizeof_pointer  8
                         set target_alignof_pointer 8
                       }

      -q32             { set target_sizeof_pointer  16 
                         set target_alignof_pointer 16 
                       }

      -*               { ; 
# All other flags beginning with '-' are ignored.
                       }

      *.c              -
      *.C              -
      *.CPP            -
      *.cpp            {
# The .c file extension is accepted in combination with -+.
                         # Remember the source file. There will be only one
                         # because of how the driver invokes us.
                         set srcfile $arg
                         lappend new_argv $arg
                       }

      default          { beam::warning "Unexpected option '$arg' ignored" }
     }
   }
#
# Add flags for the -qarch, -q32, -q64
#
  if { [::info exists define_arch($arch)] } {
    set new_argv [concat $new_argv $define_arch($arch)]
  }
  set new_argv [concat $new_argv \
                       $define_chars($chars) \
		       $define_datamodel($datamodel) \
		       $define_dbcs($dbcs) \
		       $define_digraph($digraph) \
                       $define_ifs($ifs) \
		       $define_langlvl($langlvl)   \
		       $define_longlong($longlong) \
		       $heapdebug            \
		       $posix_locale            \
		       $define_locale($locale) \
		       $define_teraspace($teraspace) \
	       ]
#
# Special handling for -qidirfirst.
#
   if { $qidirfirst_seen == 1 } {
     set srcdir [beam::dirname $srcfile]
     set new_argv [concat {-I-} $new_argv [list "-I$srcdir"]]
   }
#
# Set an error limit that is not likely to be exceeded.
#
   set new_argv [concat "$new_argv --error_limit 100000"]

   return $new_argv
}
