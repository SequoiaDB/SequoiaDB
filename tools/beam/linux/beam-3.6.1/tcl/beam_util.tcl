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
#        This file contains Tcl utilities that are generally
#        useful to all of BEAM. They should only depend on
#        things that the ti layer provides, and nothing higher.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

# Guard against multiple sourcing
if { [::info exists ::guard(beam_util)] } return
set ::guard(beam_util) 1

#
# Misc BEAM Utilities. These are in a separate namespace
# so they can be easily imported without getting
# all of ::beam::*.
#
namespace eval beam::util {
  
  ######################################################################
  #
  # List utilities
  #

  proc ut_list_push { list element } {
    # Add the element to the front of the named list.
    #  set x {a b c}
    #  ut_list_push x "before"
    #  => x == {before a b c}
  
    upvar $list my_list
    set my_list [linsert $my_list 0 $element]
  }

  proc ut_list_pop { list } {
    # Remove the first element of $list and
    # return it.
  
    upvar $list my_list
  
    set first_element [lindex $my_list 0]
  
    set my_list [lreplace $my_list 0 0] ;# Remove first element of argv

    return $first_element
  }

  proc ut_list_add { list new_list } {
    # Concat new_list onto list:
    #
    #   set x {a b c}
    #   ut_list_add x {x y z}
    #     => x == {a b c x y z}
  
    upvar $list my_list
    set my_list [concat $my_list $new_list]
  }

  proc ut_list_add_front { list new_list } {
    # Concat new_list onto the front of list:
    #
    #   set x {a b c}
    #   ut_list_add_front x {x y z}
    #     => x == {x y z a b c}
  
    upvar $list my_list
    set my_list [concat $new_list $my_list]
  }
  
  # Filter out a patter from a list. Returns a new
  # list, which contains no items that matched $pattern.
  # The pattern has the same syntax as [string match].
  
  proc ut_list_filter_out { pattern list } {
    set new_list {}
   
    foreach item $list {
      if { ! [string match $pattern $item] } {
        lappend new_list $item
      }
    }
    
    return $new_list
  }
  
  # Reverse a list. Return the result.
  
  proc ut_list_reverse { list } {
    set len [expr { [llength $list] - 1 } ]
    set rev {}
    
    for { set i $len } { $i >= 0 } { incr i -1 } {
      lappend rev [lindex $list $i] 
    }
    
    return $rev
  }
  
  # Remove the element at index i from the list, returning
  # a new list.

  proc ut_list_remove { list i } {
    return [lreplace $list $i $i]
  }

  ######################################################################
  #
  # Description utilities
  #
  
  #
  # Walk the description and command-line in parallel.
  # Run the body once for each item, with:
  #
  #   type     set to "source", "ignore", etc
  #   rest     set to the rest of desc[i], as a list
  #   pos      set to the index
  #

  proc ut_foreach_desc { typevar restvar posvar flags desc body } {
    upvar $typevar type
    upvar $restvar rest
    upvar $posvar  pos
    
    set alen [llength $flags]
    set dlen [llength $desc]
    
    if { $alen != $dlen } {
      beam::internal_error "Args and description are different lengths" 
    }
    
    for { set pos 0 } { $pos < $alen } { incr pos } {
      set item [lindex $desc $pos]
      set type [lindex $item 0]
      set rest [lrange $item 1 end]
 
      uplevel $body     
    }
  }
  
  ######################################################################
  #
  # Misc utilities
  #

  #
  # Fetch a variable value. If the item is unset, return
  # the empty string.
  #
  
  proc ut_value { var } {
    if { [::info exists $var] } {
      return [namespace eval :: [list set $var]]
    }
    
    return ""
  }
  
  #
  # Read a file, returning the contents unchanged as
  # a single string. If an error occurs, this proc
  # exits with beam::error.
  #
  proc ut_read_file { file } {
    set data {}
    
    set script {
      set fh   [open $file]
      set data [read $fh]
      close    $fh
    }
    
    if { [catch $script msg] } {
      beam::error "Can't read file '$file': $msg"
    }
    
    return $data
  }

  #
  # Return 1 if the file exists, and 0 otherwise.
  #
  proc ut_file_exists { file } {
    if { [catch [list file stat $file dummy] errors] } {
      # Stat failed
      return 0
    }
    
    return 1
  }

  #
  # Return 1 if the character is a directory separator
  # on this platform. Return 0 otherwise.
  #
  proc ut_is_dirsep { char } {
    if { [string first $char $::beam::dirsepstr] == -1 } {
      return 0
    }
    return 1
  }
  
  #
  # This is a file cache. It should be called with the name
  # of a variable that holds the cache (initially empty),
  # the name of a file to add to the cache variable, and
  # a block of code to run if the file did not exist in
  # the cache prior to this call.
  #
  proc ut_file_cache { cachevar file block } {
    #
    # The cache is an array that maps the absolute file
    # name to a "1". This ensures that files with different names
    # (symlinks, relative, etc) are resolved to the same file.
    #
    
    upvar $cachevar cache

    if { [ut_file_exists $file] } {
      set file [::beam::abspath $file]
    }
    
    if { ! [::info exists cache($file)] } {
      set cache($file) 1
      uplevel 1 $block
    }
  }
  
  ######################################################################
  #
  # Arg utilities
  #
  
  #
  # Unescape a string by creating a list of
  # arguments after one level of shell quoting
  # has been removed.
  #
  proc ut_unescape_arg { arg } {
  
    # This takes a string arg of the form
    #
    #   'a " b'   '\c \d'  "e ' f"   "g \" h"   i   -Dfoo="\"j k\""
    #
    # and returns a list of unquoted arguments
    #
    #   {a " b}    {c d}   {e ' f}   {g " h}    i   {-Dfoo="j k"}
    #
    # It performs a similar function to shell evaluation:
    # - Quotes delimit arguments
    # - Single quotes inside double quotes are allowed
    # - Double quotes inside single quotes are allowed
    # - Some escaped characters are allowed inside double quotes
    # - Arguments are separated by whitespace outside of quotes,
    #   and may be pasted together if they are of the form foo"b a r"baz
    #
    # FIXME: Document these rules

    # Parser:
    # - Walk the string, separating args on spaces
    #   - Come to an open-quote? switch to quote mode (still in same arg if no space before)
    #     - Backslashes escape characters
    #     - Any char but the matching close quote is added to this arg
    #       verbatim
    #     - A close quote of the same type as the open quote ends this phase
    # - Keep walking in skip-whitespace mode
    # - Note that if no whitespace is before or after quotes, that the surrounding
    #   non-quoted text is part of the same argument
  
    set result {}
    set current {}

    set in_quotes {}
    set in_arg 0

    set len [string length $arg]

    for { set i 0 } { $i < $len } { incr i } {
      set char [string index $arg $i]

      if { $in_quotes == {} } {
        # Not in a quoted string
        if { [string is space $char] } {
          # Whitespace separates arguments
          if { $in_arg == 1 } {
            lappend result $current
            set current {}
            set in_arg 0
          }
        } elseif { $char == {'} || $char == {"} } {
          # Quote starts a quoted string
          set in_quotes $char
          set in_arg 1
        } elseif { $char == "\\" } {
          # Escaped char
          incr i
          if { $i < $len } {
            append current [string index $arg $i]
            set in_arg 1
          }
        } else {
          # Anything else is part of the current arg
          append current $char
          set in_arg 1
        }
      } else {
        # In a quoted string
        if { $in_quotes == $char } {
          # End of the quoted string
          set in_quotes {}
        } elseif { $in_quotes == {'} } {
          # In single quotes, nothing is escaped - pass everything through
          append current $char
        } elseif { $char == "\\" } {
          # Escaped char
          incr i
          if { $i < $len  } {
            set nextchar [string index $arg $i]
            if { [string first $nextchar "\$'\"\\\n"] != -1 } {
              # This escaped character eats the backslash
              append current $nextchar
            } else {
              # This escaped character keeps the backslash
              append current "\\"
              append current $nextchar
            }
          }
        } else {
          append current $char
        }
      }
    }
  
    # At the end of the loop, we may be in the final arg
    if { $in_arg != 0 } {
      lappend result $current
    }

    return $result
  }

  #
  # Helper to read an @file and return a list of command-line
  # arguments.
  #
  # The file format is not documented well on the Java site,
  # so these rules were created from inspection. This routine
  # is used for more than Java, but the Java rules are currently
  # the ones implemented, since they are fairly generic.
  #
  # The file contains arguments that are separated by whitespace.
  # Arguments may be quoted, in which case quotes are removed,
  # spaces are preserved, and any backslash-escaped characters
  # in between either have their backslash removed (for non-special
  # characters) or are converted (for special characters).
  #
  #    "a b c" d e f  => {a b c} d e f
  #    "a \n c" d e f => {a <newline> c} d e f
  #    'a \' b" c     => {a ' b} c
  #    a \d e         => a \d e
  #    "a \d e"       => {a d e}
  #
  # Quotes end at the end of the line, and empty args are retained,
  # so this:
  #
  #    "a
  #    b" => {a} {b} {}
  #
  # Quotes don't glue arguments together.
  #
  #   a"b"c => {a} {b} {c}
  #
  # This does not re-use ut_unescape_arg because the algorithms are
  # different enough to warrant a copy here with the alternate
  # semantics for @ argument files.
  #
  # Note: The rules are documented in www/atfile.html. The
  #       atfile testcase is in testcases/sh-scripts.
  #
  proc ut_unescape_atfile_arg { arg } {
    set result {}
    set current {}

    set in_quotes {}
    set in_arg 0

    set len [string length $arg]

    for { set i 0 } { $i < $len } { incr i } {
      set char [string index $arg $i]

      if { $in_quotes == {} } {
        # Not in a quoted string
        if { [string is space $char] } {
          # Whitespace separates arguments
          if { $in_arg == 1 } {
            lappend result $current
            set current {}
            set in_arg 0
          }
        } elseif { $char == {'} || $char == {"} } {
          # Quote starts a new argument
          if { $in_arg == 1 } {
            lappend result $current
            set current {}
          }

          set in_quotes $char
          set in_arg 1
        } else {
          # Anything else is part of the current arg
          append current $char
          set in_arg 1
        }
      } else {
        # In a quoted string
        if { $char == $in_quotes || $char == "\n" } {
          # End of the quoted string
          set in_quotes {}
          lappend result $current
          set current {}
          set in_arg 0
        } else {
          # More quoted string
          if { $char == "\\" } {
            # Escaped char
            incr i
            if { $i < $len  } {
              set tmp [string index $arg $i]
              append current [ut_unescape_char $tmp]
            }
          } else {
            append current $char
          }
        }
      }
    }
  
    # At the end of the loop, we may be in the final arg
    if { $in_arg != 0 } {
      lappend result $current
    }

    return $result
  }
  
  #
  # This helper routine takes in a character like "d" or "t"
  # or "n" and unescapes it by returning "d" or "<tab>" or
  # "<newline>" as if the character had a backslash in front
  # of it. This only unescapes those characters that Tcl
  # understands. Others are returned as they are.
  #
  proc ut_unescape_char { char } {
    switch -exact -- $char {
      a { set char "\a" }
      b { set char "\b" }
      f { set char "\f" }
      n { set char "\n" }
      r { set char "\r" }
      t { set char "\t" }
      v { set char "\v" }
    }
    
    return $char
  }

  #
  # Helper to read an MSVC response file and return a list of command-line
  # arguments.
  #
  # - Double-quotes group whitespace-separated strings into a single argument
  # - Single-quotes do not group; they are literal characters
  #
  #      "ab" is {ab}, 'a b' is {'a} {b'}
  #
  # - Strings end at newline and empty strings are ignored:
  #
  #      "a
  #      b"  is two args: {a} {b}
  #
  # - Newline separates arguments even if we're inside a double-quoted string
  # - Backslash is not treated as an escape character, so escape sequences
  #   like \n and \t are preserved literally as two normal characters
  # - Quotes don't separate args: /FD"foo" is {/FDfoo}
  #
  # Note: The rules are documented in www/atfile.html. The
  #       atfile testcase is in testcases/sh-scripts.
  #
  proc ut_unescape_msvc_atfile_arg { arg } {
    set result {}
    set current {}
    set in_quotes 0

    set len [string length $arg]
    for { set i 0 } { $i < $len } { incr i } {
      set char [string index $arg $i]
      if { $in_quotes == 0 } {
        # Not in a quoted string
        if { $char == {"} } {
          set in_quotes 1
        } elseif { [string is space $char] } {
          if { $current != {} } {
            lappend result $current
          }
          set current {}
        } else {
          append current $char
        }
      } else {
        # In a quoted string
        if { $char == {"} } {
          set in_quotes 0
        } elseif { $char == "\n" } {
          set in_quotes 0
          if { $current != {} } {
            lappend result $current
          }
          set current {}
        } else {
          append current $char
        }
      }
    }

    if { $current != {} } {
      lappend result $current
    }

    return $result
  }

  #
  # Expand all @file arguments using the ut_unescape_atfile_arg.
  # Return a new argv.
  #
  proc ut_expand_atfiles { argv } {
    set flat_argv {}

    foreach arg $argv {
      if { [string match @* $arg] } {
        set file [string range $arg 1 end]
        set data [ut_read_file $file]
        ut_list_add flat_argv [ut_unescape_atfile_arg $data]
      } else {
        lappend flat_argv $arg
      }
    }

    return $flat_argv
  }

  #
  # Expand all @file arguments using the ut_unescape_msvc_atfile_arg.
  # Return a new argv.
  #
  proc ut_expand_msvc_atfiles { argv } {
    set flat_argv {}

    foreach arg $argv {
      if { [string match @* $arg] } {
        set file [string range $arg 1 end]
        set data [ut_read_file $file]
        ut_list_add flat_argv [ut_unescape_msvc_atfile_arg $data]
      } else {
        lappend flat_argv $arg
      }
    }

    return $flat_argv
  }

  # Export all ::beam::util routines
  namespace export ut_*
}

#
# Misc utils. These aren't in the namespace to maintain
# backwards compatibility. If a user copied one of our compiler
# configuration files, it would refer to these old names for
# these routines.
#

#
# Print a warning. Don't exit.
#
proc beam::warning { msg args } {
  puts stderr "${::beam::prog_name}: warning: $msg"

  foreach more_msg $args {
    puts stderr "  $more_msg"
  }
}

#
# Print an error and exit.
#
proc beam::error { msg args } {
  puts stderr "${::beam::prog_name}: error: $msg"

  foreach more_msg $args {
    puts stderr "  $more_msg"
  }

  beam::die
}

#
# Print an error and suggest that the user runs with
# --help. Exit.
#
proc beam::usage { msg args } {
  puts stderr "${::beam::prog_name}: error: $msg"

  foreach more_msg $args {
    puts stderr "  $more_msg"
  }
  
  puts stderr "Re-run with '--help' for more information."
  
  beam::die
}

proc beam::help {} {
  puts stderr $::beam::help_text
  beam::die
}

proc beam::version {} {
  puts "$::beam::prog_name $::beam::version"
  exit 0
}

proc beam::internal_error { msg } {
  puts stderr "Internal BEAM error: $msg"
  beam::die
}

#
# Run a command, with stdout and stderr going
# to the terminal. Returns the return code.
#
# If the command couldn't be started, an
# error is printed the return code is set to 1.
# If a status var is given, it is set to "ENOEXEC".
#
# If the command exits because of an interrupt, no
# error is printed, and the return code is set to 1.
# If a status var is given, it is set to "EINT".
#
proc beam::system { cmd args } {
  set     to_exec { exec -keepnewline -- }
  set     to_exec [concat $to_exec $cmd]
  lappend to_exec >@stdout 2>@stderr

  # Initialize the status to the empty string
  if { [llength $args] == 1 } {
    upvar [lindex $args 0] status
  }
  
  set status ""

  # Run  
  set rc [catch $to_exec output]

  # Check for abnormal conditions.
  # Print the output only if the process couldn't be started.

  if { [string match "couldn't execute*" $output] } {
    puts stderr "${::beam::prog_name}: error: $output"

    set rc 1
    set status ENOEXEC
  }
 
  if { [string match "child killed: interrupt*" $output] } {
    set rc 1
    set status EINT
  }
  
  return $rc
}

# Source the given file in the global namespace. If there
# is an error, exit without printing a Tcl backtrace.
# This is most likely the user's problem because of
# a bad file name and the backtrace would be confusing.
# The "source" command will print a decent enough message.

proc beam::gsource { file } {
  set script {
    namespace eval :: [list source $file]
  }
  
  if { [catch $script msg] } {
    beam::die
  }
}

# Source the given file in the global namespace.
# If the file doesn't exist in "." or "beam/tcl",
# silently fail to do anything. Returns 1 if a
# file was loaded and 0 otherwise. Note that
# ".tcl" isn't appended to the file, and the
# file isn't checked for relative or absolute
# paths, so this should only be used on filenames
# that are bare and have an extension. This should
# not be used on files provided to us by the user
# via some option.

proc beam::qsource { file } {
  foreach dir [list . ${::install_dir}/tcl] {
    set script_name "$dir/$file"
    if { [::file exists $script_name] } {
      beam::gsource $script_name
      return 1
    }
  }
  
  return 0
}

# Unset a variable, but do it quietly. Don't complain
# if the variable wasn't set before. Tcl would have.

proc beam::unset { varref } {
  upvar $varref var
  if { [ ::info exists var ] }  {
    ::unset var
  }
}

#
# The BEAM help message
#

set ::beam::help_text "
  ${::beam::prog_name} version ${::beam::version}

  Usage:
  
    ${::beam::prog_name} \[ BEAM options \] \[ compiler options \]
    
  Synopsis:
  
    BEAM is a static analysis tool that will look for bugs in
    software. BEAM tries hard to emulate the native compiler
    that is normally used to build the software so that running
    BEAM on the software is as simple as possible.
   
    BEAM takes in a set of BEAM options, which controls aspects of
    this program independent of any compiler emulation.
    
    BEAM also takes in a set of compiler options, which control
    aspects of this program related to compiler emulation.
    The compiler options also include the names of the source
    files to run BEAM over.

    One of the BEAM options is \"--beam::compiler=...\", which tells
    BEAM how to interpret the compiler options and the source
    file(s) given. Without this configuration option, BEAM will
    assume the most basic settings for a trivial compiler, and
    will probably fail to parse the source code correctly.

  Examples:

    ${::beam::prog_name} --beam::compiler=gcc.tcl -DFOO test.c

    ${::beam::prog_name} --beam::compiler=javac.tcl -d /out Test.java

  BEAM Options:
  
    BEAM supports many options; only the most common are listed here.
  
      --beam::compiler=<file>       Load compiler configuration from the Tcl
                                    file specified. This is usually required
                                    for BEAM to function correctly as it
                                    allows BEAM to emulate the native
                                    compiler.

      --beam::parms=<file>          Load BEAM parameters from the Tcl file
                                    specified. Most projects will have a
                                    BEAM parameter file that should be
                                    loaded via this option. Specifying this
                                    overrides the default, which is to load
                                    the Tcl file \"beam_default_parms.tcl\"
                                    from the BEAM installation.

      --beam::steps=<file>          Load custom BEAM steps from the Tcl file
                                    specified. The default steps that come
                                    with BEAM are normally used, so this is
                                    not usually needed. Specifying this
                                    overrides the default, which is to load
                                    the Tcl file \"beam_default_steps.tcl\"
                                    from the BEAM installation.

      --beam::source=<file>         Load any generic Tcl settings from the 
                                    Tcl file specified. This can be specified
                                    more than once, and all files will be
                                    loaded. This is commonly used for loading
                                    additional BEAM function attributes or
                                    additional BEAM parameters from separate
                                    Tcl files.

                                    This option should not be used to load
                                    the main BEAM parameter file for the
                                    project. It should also not be used for
                                    BEAM steps or for compiler configuration
                                    files.

    BEAM parameters can also be set from the BEAM command-line.

      --beam::<variable>=<value>    This is a generic way to set BEAM Tcl
                                    parameters directly from the command
                                    line. A few of the most common are listed
                                    below.

      --beam::complaint_file=<file> This specifies the file to write BEAM
                                    complaints to. The default is the
                                    standard output of the process.

      --beam::parser_file=<file>    This specifies the file to write parser
                                    errors to. The default is the standard
                                    error of the process.

      --beam::stats_file=<file>     This specifies the file to write BEAM
                                    statistics to, which include how many
                                    files and functions were analyzed. The
                                    default is to not write any statistics.

  Compiler Options:

    Once BEAM is given the correct compiler configuration file via
    \"--beam::compiler=\", BEAM will understand any compiler option
    that would be passed to the native compiler. This includes
    C and C++ options like:

      -DMACRO -I/directory file1.c file2.c

    and Java options like:

      -classpath /foo:/bar -d /output @file.list

    Any compiler that can be configured via a compiler configuration
    file can be emulated in this fashion by BEAM.

  Documentation for BEAM Options:

    https://w3.eda.ibm.com/beam/beam_compile.html

  Documentation for Compiler Configuration Files:

    https://w3.eda.ibm.com/beam/compiler_configuration.html
"
