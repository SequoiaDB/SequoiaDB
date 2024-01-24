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
#        Daniel Brand
#
#    DESCRIPTION:
#
#        This script defines default values for BEAM parameters. Those values
#        have been chosen with the objective to only report complaints most
#        people are interested in. As we add more checks and parameters this
#        script will be adjusted based on our judgment.
#
#        All public parameters are explained in detail in this document:
#
#        https://w3.eda.ibm.com/beam/parms.html
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See ChangeLog for more recent modifications.
#
#        05/16/02  brand    Copied from beam_settings.tcl
#        05/16/02  brand    Changed ERROR29 to MISTAKE13
#        05/02/02  brand    Moved beam_define_proc to beam_default.tcl
#        04/30/02  brand    In MISTAKE3 removed "=" because that actually is
#                           very loose
#        04/26/02  brand    Changed ERROR30::policy to "except_cast" because 
#                           too common in shark
#        04/25/02  brand    Added ERROR29, ERROR30
#        03/07/02  brand    Added WARNING2, WARNING3
#        02/22/02  brand    Moved bm::syntactic_checks into bm_compile
#        02/14/02  brand    Added setting (to bm::root etc) using environment 
#                           variables from beam 1.0
#        01/14/02  brand    Created

#############################################################################
# Set reasonable defaults for parameters common to all checks
#############################################################################

#############################################################################
# beam::root
#
# Specifies one or more prefixes that should be stripped off from filenames
# when printing complaints
#
# Example:
#
#   set beam::root "/u/smith/src"
#
#############################################################################

set beam::root  ".";   # source files in current directory


#############################################################################
# beam::disabled_files
#
# A pattern of files for which no complaints should be issued. This is
# a proper Tcl list.
#
# Examples:
#
#   set beam::disabled_files { *.h };                    # no complaints about .h files
#   set beam::disabled_files { *.h - */src/* };          # no complaints about .h files except those under src directory
#   set beam::disabled_files { * - ./* };                # complain only about files in current directory and its subdirectories
#   set beam::disabled_files [list * - "$beam::root/*"]; # complain only about files in root directory and its subdirectories
#   set beam::disabled_files { */junk/* };               # no complaints about anything in directory tree of junk
#
# You can add to beam::disabled_files in your local parms Tcl file by
#
#   lappend  beam::disabled_files  "*/private/*"
#
#############################################################################
#
# If this is changed testcases/sh-scripts/disabled-files.sh must be changed
# as well.
#
set beam::disabled_files { *.h *.hpp *.H *.HPP }

#############################################################################
# beam::read_disabled_input_files
#
# The default setting is "yes" for backward compatibility of stats file
# contents. Setting it to "no" can speedup builds that disable whole
# subdirectories.
set beam::read_disabled_input_files "yes"


#############################################################################
# beam::disabled_functions
#
# A pattern of function names for which no complaints should be issued.
# They are by name, rather than signature meaning that parameters are not 
# included. This is a proper Tcl list.
#
# Examples:
#
#   set beam::disabled_functions  { main };      # no complaints about function main
#   set beam::disabled_functions  { C::new };    # no complaints about C::new
#   set beam::disabled_functions  { * - *::* };  # complaints only about class member function and those with namespace
#
# You can add to beam::disabled_files in your local parms Tcl file by
#
#   lappend  beam::disabled_functions  "*test*"
#
#############################################################################

set beam::disabled_functions   ""; # all functions are enabled


#############################################################################
# beam::disabled_macros
#
# A pattern of macro names for which no complaints should be issued.
# This is a proper Tcl list.
#
# Examples:
#
#   set beam::disabled_macros { * };               # no complaints in any macro
#   set beam::disabled_macros {};                  # no restriction on complaints in macros
#   set beam::disabled_macros { va_* };            # no complaints in macros starting with va_
#   set beam::disabled_macros { * - GR_* - SE_* }; # complaints allowed only in those macros starting with GR_ or SE_
#
# You can add to beam::disabled_macros in your local parms Tcl file by
#
#   lappend  beam::disabled_macros  "PRINT"
#
#############################################################################

set beam::disabled_macros { va_arg }   ;# Enable all macros except va_arg


#############################################################################
# beam::complaint_file
#
# The name of a file to which BEAM complaints will be appended.
#
# Examples:
#
#   set beam::complaint_file "/u/smith/BEAM-complaints"
#   set beam::complaint_file "/dev/null";   # Print no complaints
#   set beam::complaint_file "stderr";      # Print complaints to stderr
#
#############################################################################

set beam::complaint_file "stdout"


#############################################################################
# beam::stats_file
#
# The name of a file to which BEAM statistics will be appended.
#
# Examples:
#
#  set beam::stats_file "/u/smith/BEAM-stats"
#  set beam::stats_file "/dev/null";        # Print no statistics
#
#############################################################################

set beam::stats_file "/dev/null"


#############################################################################
# beam::suppressed_complaint_file
#
# The name of a file to which will be appended info about complaints not issued
#
# Examples:
#
#  set beam::suppressed_complaint_file "/u/smith/BEAM-ssuppressed"
#  set beam::suppressed_complaint_file "/dev/null";   # Do not generate it
#
#############################################################################

set beam::suppressed_complaint_file "/dev/null"


#############################################################################
# beam::data
#
# The name of a directory, where BEAM may save persistent data.
#
# Example:
#
#   set beam::data "/u/smith/BEAM-data"		
#
#############################################################################

set beam::data  "";     # nothing saved


#############################################################################
# beam::allocation_may_return_null
#
# This setting applies to all allocators which allocate a resource
# with the string "memory" in the resource name.
#############################################################################

set beam::allocation_may_return_null  "maybe"


#############################################################################
# beam::may_free_null
#
# Is it OK to free(NULL) ?
#
# Example:
#
#   set beam::may_free_null  "no"
#
#############################################################################

set beam::may_free_null "yes"


#############################################################################
# beam::may_read_null
#
# Is it OK to read from location 0 ("x = *NULL") ?
#
# Example:
#
#   set beam::may_read_null  "yes";         # on AIX
#
#############################################################################

set beam::may_read_null "no"



#############################################################################
# beam::may_write_null
#
# Is it OK to write to location 0 ("*NULL = x") ?
#
# Example:
#
#   set beam::may_write_null  "yes";         # for kernel of an OS
#
#############################################################################

set beam::may_write_null "no"


#############################################################################
# beam::may_add_null
#
# Is it OK to add to location 0 ("x = &(NULL->field)") ?
#
# Example:
#
#   set beam::may_add_null  "yes"
#
#############################################################################

set beam::may_add_null "no"


#############################################################################
# beam::may_call_null
#
# Is it OK to invoke "f()" where "f" has been assigned NULL?
#
# Example:
#
#   set beam::may_call_null  "yes"
#
#############################################################################

set beam::may_call_null "no"


#############################################################################
# beam::references_may_be_null
#
# In C++ a variable can be declared as "type &x".
# Can "&x" ever be NULL?
#
# Example:
#
#   set beam::references_may_be_null "yes"
#
#############################################################################

set beam::references_may_be_null "no"

#############################################################################
# beam::avalanche_prevention
#
# What kind of avalanche errors should be prevented
#
# Example:
#
#   set beam::avalanche_prevention "";           # allow any duplicate errors
#   set beam::avalanche_prevention "failures";   # print only first failure
#   set beam::avalanche_prevention "resources";  # print only one memory leak
#
#############################################################################

set beam::avalanche_prevention "*";   # try to prevent all avalanche errors


#############################################################################
# beam::assume_no_overflow
#
# Should an error be reported if it happens only in case of overflow on
# given number of bits?
#
# Example:
#
#   set beam::assume_no_overflow "8" ; #all operations fit into 8 bits and more
#
#############################################################################

set beam::assume_no_overflow "32";  #all operations fit into 32 bits




#############################################################################
# beam::stub_function_contents
# beam::stub_function_comment
#
# A stub function is an empty function waiting to be filled later, and
# you do not want BEAM to tell you that it has no effect.
# There are two ways BEAM can recognize stub functions -- by its empty
# contents, or by a comment identifying it as a stub function.
#
# Example:
#
# set beam::stub_function_contents "return_constant" # nothing more than that
# set beam::stub_function_contents ""                # no stub functions by contents
#
# lappend beam::stub_function_comment "to be done"   # in the body of a function 
#
#############################################################################

set     beam::stub_function_contents ""; # no stub function recognized automatically
lappend beam::stub_function_comment "This is just an empty stub function"


#############################################################################
# Expanding functions inline
#
# maximal setting: expand whatever possible
#
#   set beam::expand_kind           "*"
#   set beam::expand_max_lines      "9999999"
#
# moderate setting: expand short inline functions
#
#   set beam::expand_kind           "inline"
#   set beam::expand_max_lines      "10"
#
# expand constructors and destructors:
#
#   set beam::expand_kind           "constructor destructor"
#   set beam::expand_max_lines      "100"
#
#############################################################################

set beam::expand_kind         "";     # expand no functions
set beam::expand_max_lines    "1";    # only functions with 1 line


#############################################################################
# beam::dirty_enum
#
# Examples:
#
#   set beam::dirty_enum "*";               # all enums are dirty
#   set beam::dirty_enum "node_t edge_t";   # list of dirty enums
#
#############################################################################

set beam::clean_enum      "";     # all enums are unspecified
set beam::dirty_enum      "";     # all enums are unspecified
set beam::extended_enum   "";     # all enums are unspecified


#############################################################################
# beam::no_throw_list
#
# Examples:
#
#   According to literal interpretation of the language standard
#   a function without throw list can throw anything
#
#   set "beam::no_throw_list(callee)"         "anything"
#
#############################################################################

set beam::no_throw_list(callee)   "nothing";     # callee does not throw
set beam::no_throw_list(self)     "anything";    # any throw is allowed


#############################################################################
# beam::dont_report_caught_runtime_exceptions
#
# Examples:
#
# Report no runtime exception in code that handles runtime exceptions individually
#   set beam::dont_report_caught_runtime_exceptions "*.ArithmeticException       \
#                                                    *.NullPointerException      \
#                                                    *.IndexOutOfBoundsException"
#
# Report no runtime exception in code that handles RuntimeException
#   set beam::dont_report_caught_runtime_exceptions "*.RuntimeException" ;
#
# Report no runtime exception in code that handles Exception
#   set beam::dont_report_caught_runtime_exceptions "*.Exception" ;
#
# Report no runtime exception in code that handles any exception
#   set beam::dont_report_caught_runtime_exceptions "*";
#
#############################################################################

set beam::dont_report_caught_runtime_exceptions ""; # Report all runtime exceptions
                                                    # even if they are explicitly caught


#############################################################################
# beam::unused_enum_value
#
# Examples:
#
#   set beam::unused_enum_value(COLOR)  "COLOR_NUM"
#   set beam::unused_enum_value(*)      "*_MIN *_MAX *_ERROR"
#
#############################################################################


#############################################################################
# beam::disabling_comment
#
# The default setting does not define a disabling comment that applies to
# all complaints. The example shows how to do that. It defines the structured
# comment "beam" that can be used to suppress any complaint.
#
# Example:
#
#   set beam::disabling_comment "beam"
#
#############################################################################


#############################################################################
# beam::disabling_comment_policy
#
# The default setting requires the comment to be placed on the line of the
# complaint.
#
# Examples:
#
#   set beam::disabling_comment_policy  "L-1 to L"
#                    The above will allow disabling comment
#                    to be placed on the line in the beam complaint (L)
#                    or on the line above it (L-1)
#
#   set beam::disabling_comment_policy  "L-1 to L+1"
#                    The above will allow disabling comment
#                    to be placed anywhere on the three lines
#                    starting one above and ending one below
#                    the line in the beam complaint
#
#############################################################################

set beam::disabling_comment_policy  "L"


#############################################################################
# beam::max_time_per_kloc_in_sec
#
# Number of seconds elapsed time spent on analyzing 1000 lines of code.
#
#############################################################################

set beam::max_time_per_kloc_in_sec   "600"


#############################################################################
# Parameters for interprocedural analysis (IPA)
#
# To switch interprocedural analysis on:
#   set beam::ipa        "all";             # switched on
#   set beam::data        some_directory_to_contain_the_result
#   set beam::build_root  root_of_source_tree_compiled_together
#
#############################################################################

set beam::ipa "all";             # switched on 
set beam::build_root ".";        # a common setting

#############################################################################
# How function names should appear in complaints
#
#   set beam::function_name_style   "unqualified";   # bar
#   set beam::function_name_style   "qualified";     # foo::bar
#   set beam::function_name_style   "signature";     # foo::bar(int, char *)
#
#############################################################################

set beam::function_name_style "qualified"


#############################################################################
# Parameters for developers and advanced users
#############################################################################

source beam_secret_parms.tcl


#############################################################################
# Set defaults for parameters specific to individual checks.
# These settings are minimal in the sense that we expect everybody
# to want to see these complaints.
#############################################################################

#
# ERROR1: variable used but never assigned
#

set      beam::ERROR1::severity             ""

#set     beam::ERROR1::enabling_policy      "allow_partially_initialized_records"
set      beam::ERROR1::enabling_policy      "always"
#set     beam::ERROR1::enabling_policy      ""

lappend  beam::ERROR1::disabling_comment    "uninitialized"
lappend  beam::ERROR1::disabling_comment    "unassigned"

set      beam::ERROR1::disabled_files       "";        # enable all files
set      beam::ERROR1::disabled_functions   "";        # enable all functions
set      beam::ERROR1::disabled_macros      "";        # enable in all macros

#set     beam::ERROR1::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR1::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR2: invalid operation involving NULL pointer
#

set      beam::ERROR2::severity             ""

set      beam::ERROR2::enabling_policy      "always"
#set     beam::ERROR2::enabling_policy      ""

lappend  beam::ERROR2::disabling_comment    "operating on NULL"

set      beam::ERROR2::disabled_files       "";        # enable all files
set      beam::ERROR2::disabled_functions   "";        # enable all functions
set      beam::ERROR2::disabled_macros      "";        # enable in all macros

#set     beam::ERROR2::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR2::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR3: freeing deallocated memory
#

set      beam::ERROR3::severity             ""

set      beam::ERROR3::enabling_policy      "always"
#set     beam::ERROR3::enabling_policy      ""

lappend  beam::ERROR3::disabling_comment    "freeing deallocated memory"

set      beam::ERROR3::disabled_files       "";        # enable all files
set      beam::ERROR3::disabled_functions   "";        # enable all functions
set      beam::ERROR3::disabled_macros      "";        # enable in all macros

#set     beam::ERROR3::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR3::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR4: accessing deallocated location
#

set      beam::ERROR4::severity             ""

set      beam::ERROR4::enabling_policy      "always"
#set     beam::ERROR4::enabling_policy      ""

lappend  beam::ERROR4::disabling_comment    "accessing deallocated"

set      beam::ERROR4::disabled_files       "";        # enable all files
set      beam::ERROR4::disabled_functions   "";        # enable all functions
set      beam::ERROR4::disabled_macros      "";        # enable in all macros

#set     beam::ERROR4::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR4::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR5: dereferencing NULL
#

set      beam::ERROR5::severity             ""

set      beam::ERROR5::enabling_policy      "always"
#set     beam::ERROR5::enabling_policy      ""

lappend  beam::ERROR5::disabling_comment    "dereferencing NULL"
lappend  beam::ERROR5::disabling_comment    "will not be null"

set      beam::ERROR5::disabled_files       "";        # enable all files
set      beam::ERROR5::disabled_functions   "";        # enable all functions
set      beam::ERROR5::disabled_macros      "";        # enable in all macros

#set     beam::ERROR5::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR5::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR6: using NULL class pointer
#

set      beam::ERROR6::severity             ""

set      beam::ERROR6::enabling_policy      "always"
#set     beam::ERROR6::enabling_policy      ""

lappend  beam::ERROR6::disabling_comment    "null instance"
lappend  beam::ERROR6::disabling_comment    "dereferencing NULL"
lappend  beam::ERROR6::disabling_comment    "will not be null"

set      beam::ERROR6::disabled_files       "";        # enable all files
set      beam::ERROR6::disabled_functions   "";        # enable all functions
set      beam::ERROR6::disabled_macros      "";        # enable in all macros

#set     beam::ERROR6::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR6::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR7: subscript out of range
#

set      beam::ERROR7::severity             ""

#set     beam::ERROR7::enabling_policy      "always"
#set     beam::ERROR7::enabling_policy      ""
#set     beam::ERROR7::enabling_policy      "lower_bound"
#set     beam::ERROR7::enabling_policy      "excessive_bound"
set      beam::ERROR7::enabling_policy      "negative_bound + upper_bound + array_bound"

set      beam::ERROR7::extensible_struct    "yes"; # no complaint about struct {.....; int array[1];}

lappend  beam::ERROR7::disabling_comment    "accessing beyond memory"
lappend  beam::ERROR7::disabling_comment    "subscript out of range"
lappend  beam::ERROR7::disabling_comment    "will be in range"

set      beam::ERROR7::disabled_files       "";        # enable all files
set      beam::ERROR7::disabled_functions   "";        # enable all functions
set      beam::ERROR7::disabled_macros      "va_*";    # disable vararg macros

#set     beam::ERROR7::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR7::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR8: passing pointer to deallocated memory
#

set      beam::ERROR8::severity             ""

#set     beam::ERROR8::enabling_policy      ""
#set     beam::ERROR8::enabling_policy      "compared"          # only if pointer compared with another pointer
#set     beam::ERROR8::enabling_policy      "not_compared"      # only if pointer used other than comparison
#set     beam::ERROR8::enabling_policy      "always - compared" # same as not_compared
set      beam::ERROR8::enabling_policy      "always"

lappend  beam::ERROR8::disabling_comment    "using deallocated"
lappend  beam::ERROR8::disabling_comment    "passing deallocated"
lappend  beam::ERROR8::disabling_comment    "may be deallocated"

set      beam::ERROR8::disabled_files       "";        # enable all files
set      beam::ERROR8::disabled_functions   "";        # enable all functions
set      beam::ERROR8::disabled_macros      "";        # enable in all macros

#set     beam::ERROR8::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR8::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR9: passing null object
#

set      beam::ERROR9::severity             ""

set      beam::ERROR9::enabling_policy      "always"
#set     beam::ERROR9::enabling_policy      ""

lappend  beam::ERROR9::disabling_comment    "passing null object"
lappend  beam::ERROR9::disabling_comment    "will not be null"

set      beam::ERROR9::disabled_files       "";        # enable all files
set      beam::ERROR9::disabled_functions   "";        # enable all functions
set      beam::ERROR9::disabled_macros      "";        # enable in all macros

#set     beam::ERROR9::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR9::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR10: f(struct) where f has vararg
#

set      beam::ERROR10::severity             ""

set      beam::ERROR10::enabling_policy      "always"
#set     beam::ERROR10::enabling_policy      ""

lappend  beam::ERROR10::disabling_comment    "struct as vararg"
lappend  beam::ERROR10::disabling_comment    "struct to ..."

set      beam::ERROR10::disabled_files       "";        # enable all files
set      beam::ERROR10::disabled_functions   "";        # enable all functions
set      beam::ERROR10::disabled_macros      "";        # enable in all macros

#set     beam::ERROR10::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR10::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR11(heap_memory): function exposes pointer to deallocated memory
#

set      beam::ERROR11(heap_memory)::severity             ""

#set     beam::ERROR11(heap_memory)::enabling_policy      "always"
#set     beam::ERROR11(heap_memory)::enabling_policy      ""
#set     beam::ERROR11(heap_memory)::enabling_policy      "return";        # complain if exposed via return
#set     beam::ERROR11(heap_memory)::enabling_policy      "parameter";     # complain if exposed by assigning through a parameter
#set     beam::ERROR11(heap_memory)::enabling_policy      "heap";          # complain if exposed by assigning to a heap location
#set     beam::ERROR11(heap_memory)::enabling_policy      "extern";        # complain if exposed by assigning to an extern variable
#set     beam::ERROR11(heap_memory)::enabling_policy      "static-local";  # complain if exposed by assigning to an variable declared static inside the function
#set     beam::ERROR11(heap_memory)::enabling_policy      "static-file";   # complain if exposed by assigning to an variable declared static outside any function
#set     beam::ERROR11(heap_memory)::enabling_policy      "return  parameter  heap  extern  static-local  static-file"; # equivalent to "always"
set      beam::ERROR11(heap_memory)::enabling_policy      "parameter  return"

lappend  beam::ERROR11(heap_memory)::disabling_comment    "pointer to freed memory exposed"
lappend  beam::ERROR11(heap_memory)::disabling_comment    "pointer to deallocated memory exposed"

set      beam::ERROR11(heap_memory)::disabled_files       "";         # enable in all files
set      beam::ERROR11(heap_memory)::disabled_functions   "";         # enable all functions
set      beam::ERROR11(heap_memory)::disabled_macros      "";         # enable in all macros

#set     beam::ERROR11(heap_memory)::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR11(heap_memory)::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR11(stack_memory): function exposes pointer to deallocated memory
#

set      beam::ERROR11(stack_memory)::severity             ""

set      beam::ERROR11(stack_memory)::enabling_policy      "always"
#set     beam::ERROR11(stack_memory)::enabling_policy      ""
#set     beam::ERROR11(stack_memory)::enabling_policy      "return";        # complain if exposed via return
#set     beam::ERROR11(stack_memory)::enabling_policy      "parameter";     # complain if exposed by assigning through a parameter
#set     beam::ERROR11(stack_memory)::enabling_policy      "heap";          # complain if exposed by assigning to a heap location
#set     beam::ERROR11(stack_memory)::enabling_policy      "extern";        # complain if exposed by assigning to an extern variable
#set     beam::ERROR11(stack_memory)::enabling_policy      "static-local";  # complain if exposed by assigning to an variable declared static inside the function
#set     beam::ERROR11(stack_memory)::enabling_policy      "static-file";   # complain if exposed by assigning to an variable declared static outside any function
#set     beam::ERROR11(stack_memory)::enabling_policy      "return  parameter  heap  extern  static-local  static-file"; # equivalent to "always"
#set     beam::ERROR11(stack_memory)::enabling_policy      "parameter  return"

lappend  beam::ERROR11(stack_memory)::disabling_comment    "pointer to local variable exposed"
lappend  beam::ERROR11(stack_memory)::disabling_comment    "pointer to deallocated memory exposed"

set      beam::ERROR11(stack_memory)::disabled_files       "";         # enable in all files
set      beam::ERROR11(stack_memory)::disabled_functions   "";         # enable all functions
set      beam::ERROR11(stack_memory)::disabled_macros      "";         # enable in all macros

#set     beam::ERROR11(stack_memory)::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR11(stack_memory)::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR11(file): function exposes pointer to closed file
#

set      beam::ERROR11(file)::severity             ""

#set     beam::ERROR11(file)::enabling_policy      "always"
#set     beam::ERROR11(file)::enabling_policy      ""
#set     beam::ERROR11(file)::enabling_policy      "return";        # complain if exposed via return
#set     beam::ERROR11(file)::enabling_policy      "parameter";     # complain if exposed by assigning through a parameter
#set     beam::ERROR11(file)::enabling_policy      "heap";          # complain if exposed by assigning to a heap location
#set     beam::ERROR11(file)::enabling_policy      "extern";        # complain if exposed by assigning to an extern variable
#set     beam::ERROR11(file)::enabling_policy      "static-local";  # complain if exposed by assigning to an variable declared static inside the function
#set     beam::ERROR11(file)::enabling_policy      "static-file";   # complain if exposed by assigning to an variable declared static outside any function
#set     beam::ERROR11(file)::enabling_policy      "return  parameter  heap  extern  static-local  static-file"; # equivalent to "always"
set      beam::ERROR11(file)::enabling_policy      "parameter  return"

lappend  beam::ERROR11(file)::disabling_comment    "closed file exposed"
lappend  beam::ERROR11(file)::disabling_comment    "pointer to deallocated memory exposed"

set      beam::ERROR11(file)::disabled_files       "";         # enable in all files
set      beam::ERROR11(file)::disabled_functions   "";         # enable all functions
set      beam::ERROR11(file)::disabled_macros      "";         # enable in all macros

#set     beam::ERROR11(file)::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR11(file)::complaint_options    "show_values  show_calls_with_side_effect"

#
# ERROR11(_recursive_lock): function exposes pointer to released lock.
#
# that is normal and there is nothing wrong with that because 
# the _recursive_lock resource is not something that can be accessed --
# only acquired or released

set      beam::ERROR11(_recursive_lock)::enabling_policy      ""

#
# ERROR11: function exposes pointer to deallocated resource
#

set      beam::ERROR11::severity             ""

#set     beam::ERROR11::enabling_policy      "always"
#set     beam::ERROR11::enabling_policy      ""
#set     beam::ERROR11::enabling_policy      "return";        # complain if exposed via return
#set     beam::ERROR11::enabling_policy      "parameter";     # complain if exposed by assigning through a parameter
#set     beam::ERROR11::enabling_policy      "heap";          # complain if exposed by assigning to a heap location
#set     beam::ERROR11::enabling_policy      "extern";        # complain if exposed by assigning to an extern variable
#set     beam::ERROR11::enabling_policy      "static-local";  # complain if exposed by assigning to an variable declared static inside the function
#set     beam::ERROR11::enabling_policy      "static-file";   # complain if exposed by assigning to an variable declared static outside any function
#set     beam::ERROR11::enabling_policy      "return  parameter  heap  extern  static-local  static-file"; # equivalent to "always"
set      beam::ERROR11::enabling_policy      "parameter  return"

lappend  beam::ERROR11::disabling_comment    "pointer to deallocated resource exposed"
lappend  beam::ERROR11::disabling_comment    "pointer to deallocated memory exposed"
lappend  beam::ERROR11::disabling_comment    "closed file exposed"

set      beam::ERROR11::disabled_files       "";         # enable in all files
set      beam::ERROR11::disabled_macros      "";         # enable in all macros
set      beam::ERROR11::disabled_functions   "main";     # does not matter in main

#set     beam::ERROR11::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR11::complaint_options    "show_values  show_calls_with_side_effect"

# should be reported only if valid even under the assumtions of 
# no effect for  unknown functions
set      beam::ERROR11::unknown_functions_effect ""

#
# ERROR12 has been replaced by ERROR23(heap_memory)
#


#
# ERROR13: mismatch in printf arguments
#

set      beam::ERROR13::severity             ""

set      beam::ERROR13::enabling_policy      "always"
#set     beam::ERROR13::enabling_policy      ""

lappend  beam::ERROR13::disabling_comment    "format mismatch"
lappend  beam::ERROR13::disabling_comment    "mean to print"

set      beam::ERROR13::disabled_files       "";        # enable all files
set      beam::ERROR13::disabled_functions   "";        # enable all functions
set      beam::ERROR13::disabled_macros      "";        # enable in all macros

#set     beam::ERROR13::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR13::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR14: too few printf arguments
#

set      beam::ERROR14::severity             ""

set      beam::ERROR14::enabling_policy      "always"
#set     beam::ERROR14::enabling_policy      ""

lappend  beam::ERROR14::disabling_comment    "missing argument"

set      beam::ERROR14::disabled_files       "";        # enable all files
set      beam::ERROR14::disabled_functions   "";        # enable all functions
set      beam::ERROR14::disabled_macros      "";        # enable in all macros

#set     beam::ERROR14::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR14::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR15: too many printf arguments
#

set      beam::ERROR15::severity             ""

set      beam::ERROR15::enabling_policy      "always"
#set     beam::ERROR15::enabling_policy      ""

lappend  beam::ERROR15::disabling_comment    "extra argument"

set      beam::ERROR15::disabled_files       "";        # enable all files
set      beam::ERROR15::disabled_functions   "";        # enable all functions
set      beam::ERROR15::disabled_macros      "";        # enable in all macros

#set     beam::ERROR15::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR15::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR16: invalid format in printf
#

set      beam::ERROR16::severity             ""

set      beam::ERROR16::enabling_policy      "always"
#set     beam::ERROR16::enabling_policy      ""

lappend  beam::ERROR16::disabling_comment    "invalid format"
lappend  beam::ERROR16::disabling_comment    "mean the conversion specification"

set      beam::ERROR16::disabled_files       "";        # enable all files
set      beam::ERROR16::disabled_functions   "";        # enable all functions
set      beam::ERROR16::disabled_macros      "";        # enable in all macros

#set     beam::ERROR16::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR16::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR17: arg to scanf is not a pointer
#

set      beam::ERROR17::severity             ""

set      beam::ERROR17::enabling_policy      "always"
#set     beam::ERROR17::enabling_policy      ""

lappend  beam::ERROR17::disabling_comment    "arg is not a pointer"
lappend  beam::ERROR17::disabling_comment    "not a pointer"

set      beam::ERROR17::disabled_files       "";        # enable all files
set      beam::ERROR17::disabled_functions   "";        # enable all functions
set      beam::ERROR17::disabled_macros      "";        # enable in all macros

#set     beam::ERROR17::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR17::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR18: missing return statement
#

set      beam::ERROR18::severity             ""

#set     beam::ERROR18::enabling_policy      "always"; # complains also about functions without type
set      beam::ERROR18::enabling_policy      ""

lappend  beam::ERROR18::disabling_comment    "no return value"

set      beam::ERROR18::disabled_files       "";        # enable all files
set      beam::ERROR18::disabled_functions   "main";    # enable all functions, except main
set      beam::ERROR18::disabled_macros      "";        # enable in all macros

#set     beam::ERROR18::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR18::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR19: statement always fails
#

set      beam::ERROR19::severity             ""

set      beam::ERROR19::enabling_policy      "always"
#set     beam::ERROR19::enabling_policy      ""

lappend  beam::ERROR19::disabling_comment    "always fails"

set      beam::ERROR19::disabled_files       "";        # enable all files
set      beam::ERROR19::disabled_functions   "";        # enable all functions
set      beam::ERROR19::disabled_macros      "";        # enable in all macros

#set     beam::ERROR19::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR19::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR20: return without a value
#

set      beam::ERROR20::severity             ""

set      beam::ERROR20::enabling_policy      "always"
#set     beam::ERROR20::enabling_policy      ""

lappend  beam::ERROR20::disabling_comment    "no return value"

set      beam::ERROR20::disabled_files       "";        # enable all files
set      beam::ERROR20::disabled_functions   "";        # enable all functions
set      beam::ERROR20::disabled_macros      "";        # enable in all macros

#set     beam::ERROR20::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR20::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR21: expression should be a pointer
#

set      beam::ERROR21::severity             ""

set      beam::ERROR21::enabling_policy      "always"
#set     beam::ERROR21::enabling_policy      ""

lappend  beam::ERROR21::disabling_comment    "should be pointer"

set      beam::ERROR21::disabled_files       "";        # enable all files
set      beam::ERROR21::disabled_functions   "";        # enable all functions
set      beam::ERROR21::disabled_macros      "";        # enable in all macros

#set     beam::ERROR21::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR21::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR22: division by 0
#

set      beam::ERROR22::severity             ""

set      beam::ERROR22::enabling_policy      "always"
#set     beam::ERROR22::enabling_policy      ""

lappend  beam::ERROR22::disabling_comment    "division by 0"

set      beam::ERROR22::disabled_files       "";        # enable all files
set      beam::ERROR22::disabled_functions   "";        # enable all functions
set      beam::ERROR22::disabled_macros      "";        # enable in all macros

#set     beam::ERROR22::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR22::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR23(heap_memory): memory leak
#

lappend  beam::ERROR23(heap_memory)::disabling_comment        "memory leak"
set      beam::ERROR23(heap_memory)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23



#
# ERROR23(file): failure to close a file
#

lappend  beam::ERROR23(file)::disabling_comment        "file leak"
set      beam::ERROR23(file)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23


#
# ERROR23(glob): failure to call globfree()
#

lappend  beam::ERROR23(glob)::disabling_comment        "glob leak"
lappend  beam::ERROR23(glob)::disabling_comment        "resource leak"
set      beam::ERROR23(glob)::unknown_functions_effect "saves_parameter"

# glob(..., GLOB_APPEND, ..., p) does not cause the leak for p
# because the existing content of p is appended to the new one
lappend  beam::ERROR23(glob)::disabling_comment    "GLOB_APPEND"

# All other parameters default to their setting in beam::ERROR23



#
# ERROR23(addrinfo): failure to call freeaddrinfo()
#

lappend  beam::ERROR23(addrinfo)::disabling_comment        "addrinfo leak"
lappend  beam::ERROR23(addrinfo)::disabling_comment        "resource leak"
set      beam::ERROR23(addrinfo)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23



#
# ERROR23(directory): failure to call closedir()
#

lappend  beam::ERROR23(directory)::disabling_comment        "directory leak"
lappend  beam::ERROR23(directory)::disabling_comment        "resource leak"
set      beam::ERROR23(directory)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23


#
# ERROR23(cursor): failure to deallocate cursor
#

lappend  beam::ERROR23(cursor)::disabling_comment        "cursor leak"
lappend  beam::ERROR23(cursor)::disabling_comment        "resource leak"
set      beam::ERROR23(cursor)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR23


#
# ERROR23(lock): failure to uninitialize a lock
#

lappend  beam::ERROR23(lock)::disabling_comment        "lock leak"
lappend  beam::ERROR23(lock)::disabling_comment        "resource leak"
set      beam::ERROR23(lock)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR23


#
# ERROR23(_recursive_lock): failure to release a lock
#

lappend  beam::ERROR23(_recursive_lock)::disabling_comment        "lock leak"
lappend  beam::ERROR23(_recursive_lock)::disabling_comment        "resource leak"
set      beam::ERROR23(_recursive_lock)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR23



#
# ERROR23: resource leak for resources not mentioned above
#

set      beam::ERROR23::severity             ""

set      beam::ERROR23::enabling_policy      "always"
#set     beam::ERROR23::enabling_policy      ""

lappend  beam::ERROR23::disabling_comment    "resource leak"

set      beam::ERROR23::disabled_files       "";        # enable all files
set      beam::ERROR23::disabled_functions   "main";    # does not matter in main
set      beam::ERROR23::disabled_macros      "";        # enable in all macros

#set     beam::ERROR23::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR23::complaint_options    "show_values  show_calls_with_side_effect"

set      beam::ERROR23::unknown_functions_effect "saves_parameter"



#
# ERROR24 has been replaced by ERROR23(file)
#


#
# ERROR25: using garbage value
#

set      beam::ERROR25::severity             ""

set      beam::ERROR25::enabling_policy      ""
#set     beam::ERROR25::enabling_policy      "always"; # complains about members not initialized by a constructor

lappend  beam::ERROR25::disabling_comment    "uninitialized member"
lappend  beam::ERROR25::disabling_comment    "not setting"

set      beam::ERROR25::disabled_files       "";        # enable all files
set      beam::ERROR25::disabled_functions   "";        # enable all functions
set      beam::ERROR25::disabled_macros      "";        # enable in all macros

#set     beam::ERROR25::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR25::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR26(memory allocation source): mismatch between allocator and deallocator
#

lappend  "beam::ERROR26(memory allocation source)::disabling_comment"    "wrong deallocator"
lappend  "beam::ERROR26(memory allocation source)::disabling_comment"    "don't free using"
lappend  "beam::ERROR26(memory allocation source)::disabling_comment"    "violated property"


# ERROR26: violated property

set      beam::ERROR26::severity             ""

set      beam::ERROR26::enabling_policy      "always"
#set     beam::ERROR26::enabling_policy      ""

lappend  beam::ERROR26::disabling_comment    "violated property"

set      beam::ERROR26::disabled_files       "";        # enable all files
set      beam::ERROR26::disabled_functions   "";        # enable all functions
set      beam::ERROR26::disabled_macros      "";        # enable in all macros

#set     beam::ERROR26::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR26::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR27 has been replaced by WARNING6
#


#
# ERROR28: f(...) should be f(..., NULL)
#

set      beam::ERROR28::severity             ""

set      beam::ERROR28::enabling_policy      "always"
#set     beam::ERROR28::enabling_policy      ""

lappend  beam::ERROR28::disabling_comment    "missing terminator"
lappend  beam::ERROR28::disabling_comment    "null needed"

set      beam::ERROR28::disabled_files       "";        # enable all files
set      beam::ERROR28::disabled_functions   "";        # enable all functions
set      beam::ERROR28::disabled_macros      "";        # enable in all macros

# Functions to complain about

#set     beam::ERROR28::functions                   "ut_concat + my_func"
set      beam::ERROR28::functions                   ""

#set     beam::ERROR28::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::ERROR28::functions_by_qualified_name ""
  
#lappend beam::ERROR28::functions_by_signature      { clas::func(int &) }
set      beam::ERROR28::functions_by_signature      {}
  
#set     beam::ERROR28::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR28::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR29: printf args accessed randomly and sequentially
#

set      beam::ERROR29::severity             ""

set      beam::ERROR29::enabling_policy      "always"
#set     beam::ERROR29::enabling_policy      ""

lappend  beam::ERROR29::disabling_comment    "printf args randomly and sequentially"

set      beam::ERROR29::disabled_files       "";        # enable all files
set      beam::ERROR29::disabled_functions   "";        # enable all functions
set      beam::ERROR29::disabled_macros      "";        # enable in all macros

#set     beam::ERROR29::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR29::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR30: uncaught exception doesn't match function declaration
#

set      beam::ERROR30::severity             ""

set      beam::ERROR30::enabling_policy      "always"
#set     beam::ERROR30::enabling_policy      ""

lappend  beam::ERROR30::disabling_comment    "uncaught exception"

set      beam::ERROR30::disabled_files       "";        # enable all files
set      beam::ERROR30::disabled_functions   "";        # enable all functions
set      beam::ERROR30::disabled_macros      "";        # enable in all macros

#set     beam::ERROR30::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR30::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR31: 'throw;' outside of exception handlers
#

set      beam::ERROR31::severity             ""

set      beam::ERROR31::enabling_policy      "always"
#set     beam::ERROR31::enabling_policy      ""

lappend  beam::ERROR31::disabling_comment    "terminating throw"

set      beam::ERROR31::disabled_files       "";        # enable all files
set      beam::ERROR31::disabled_functions   "";        # enable all functions
set      beam::ERROR31::disabled_macros      "";        # enable in all macros

#set     beam::ERROR31::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR31::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR32: Throwing exceptions during exceptions
#

set      beam::ERROR32::severity             ""

set      beam::ERROR32::enabling_policy      "always"
#set     beam::ERROR32::enabling_policy      ""

lappend  beam::ERROR32::disabling_comment    "exception during exception"
lappend  beam::ERROR32::disabling_comment    "not when handling"

set      beam::ERROR32::disabled_files       "";        # enable all files
set      beam::ERROR32::disabled_functions   "";        # enable all functions
set      beam::ERROR32::disabled_macros      "";        # enable in all macros

#set     beam::ERROR32::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR32::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR33: Invalid shift amount
#

set      beam::ERROR33::severity             ""

set      beam::ERROR33::enabling_policy      "always"
#set     beam::ERROR33::enabling_policy      ""

lappend  beam::ERROR33::disabling_comment    "invalid shift"

set      beam::ERROR33::disabled_files       "";        # enable all files
set      beam::ERROR33::disabled_functions   "";        # enable all functions
set      beam::ERROR33::disabled_macros      "";        # enable in all macros

#set     beam::ERROR33::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR33::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR34: failing assertion
#

set      beam::ERROR34::severity             ""

set      beam::ERROR34::enabling_policy      ""
#set     beam::ERROR34::enabling_policy      "always"

lappend  beam::ERROR34::disabling_comment    "assertion"

set      beam::ERROR34::disabled_files       "";        # enable all files
set      beam::ERROR34::disabled_functions   "";        # enable all functions
set      beam::ERROR34::disabled_macros      "";        # enable in all macros

#set     beam::ERROR34::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR34::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR35: sequence points violation
#

set      beam::ERROR35::severity             ""

set      beam::ERROR35::enabling_policy      "always"
#set     beam::ERROR35::enabling_policy      ""

lappend  beam::ERROR35::disabling_comment    "undefined side-effect"

set      beam::ERROR35::disabled_files       "";        # enable all files
set      beam::ERROR35::disabled_functions   "";        # enable all functions
set      beam::ERROR35::disabled_macros      "";        # enable in all macros

#set     beam::ERROR35::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR35::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR36: null function pointer
#

set      beam::ERROR36::severity             ""

set      beam::ERROR36::enabling_policy      "always"
#set     beam::ERROR36::enabling_policy      ""

lappend  beam::ERROR36::disabling_comment    "null function"

set      beam::ERROR36::disabled_files       "";        # enable all files
set      beam::ERROR36::disabled_functions   "";        # enable all functions
set      beam::ERROR36::disabled_macros      "";        # enable in all macros

#set     beam::ERROR36::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR36::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR37: wrong num args to variadic function
#

set      beam::ERROR37::severity             ""

set      beam::ERROR37::enabling_policy      "always"
#set     beam::ERROR37::enabling_policy      ""

lappend  beam::ERROR37::disabling_comment    "num args"

set      beam::ERROR37::disabled_files       "";        # enable all files
set      beam::ERROR37::disabled_functions   "";        # enable all functions
set      beam::ERROR37::disabled_macros      "";        # enable in all macros

#set     beam::ERROR37::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR37::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR38(stack_memory): exceeding stack limit
#

set      beam::ERROR38(stack_memory)::severity             ""

#set     beam::ERROR38(stack_memory)::enabling_policy      "always"
set      beam::ERROR38(stack_memory)::enabling_policy      "";         # disabled

lappend  beam::ERROR38(stack_memory)::disabling_comment    "stack limit"

set      beam::ERROR38(stack_memory)::disabled_files       "";        # enable in all files
set      beam::ERROR38(stack_memory)::disabled_functions   "";        # enable all functions
set      beam::ERROR38(stack_memory)::disabled_macros      "";        # enable in all macros

#set     beam::ERROR38(stack_memory)::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR38(stack_memory)::complaint_options    "show_values  show_calls_with_side_effect"

set      beam::ERROR38(stack_memory)::limit                "1000000";     # max size of stack in bytes


#
# ERROR38(file): exceeding file descriptors
#

set      beam::ERROR38(file)::severity             ""

#set     beam::ERROR38(file)::enabling_policy      "always"
set      beam::ERROR38(file)::enabling_policy      "";         # disabled

lappend  beam::ERROR38(file)::disabling_comment    "file limit"

set      beam::ERROR38(file)::disabled_files       "";        # enable in all files
set      beam::ERROR38(file)::disabled_functions   "";        # enable all functions
set      beam::ERROR38(file)::disabled_macros      "";        # enable in all macros

#set     beam::ERROR38(file)::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR38(file)::complaint_options    "show_values  show_calls_with_side_effect"

set      beam::ERROR38(file)::limit                "200";         # max number of files open


#
# ERROR38: exceeding resource
#

set      beam::ERROR38::severity             ""

#set     beam::ERROR38::enabling_policy      "always"
set      beam::ERROR38::enabling_policy      "";         # disabled

lappend  beam::ERROR38::disabling_comment    "resource limit"

set      beam::ERROR38::disabled_files       "";        # enable in all files
set      beam::ERROR38::disabled_functions   "";        # enable all functions
set      beam::ERROR38::disabled_macros      "";        # enable in all macros

#set     beam::ERROR38::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR38::complaint_options    "show_values  show_calls_with_side_effect"

set      beam::ERROR38::limit                "0"; # max amount interpreted depending on resource


#
# ERROR39: shifting right negative number
#

set      beam::ERROR39::severity             ""

set      beam::ERROR39::enabling_policy      "always"
#set     beam::ERROR39::enabling_policy      "";         # disabled

lappend  beam::ERROR39::disabling_comment    "shift undefined"

set      beam::ERROR39::disabled_files       "";        # enable in all files
set      beam::ERROR39::disabled_functions   "";        # enable all functions
set      beam::ERROR39::disabled_macros      "";        # enable in all macros

#set     beam::ERROR39::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR39::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR40: using undefined value
#

set      beam::ERROR40::severity             ""

#set     beam::ERROR40::enabling_policy      "always"
set      beam::ERROR40::enabling_policy      "";         # disabled

lappend  beam::ERROR40::disabling_comment    "undefined"

set      beam::ERROR40::disabled_files       "";        # enable in all files
set      beam::ERROR40::disabled_functions   "";        # enable all functions
set      beam::ERROR40::disabled_macros      "";        # enable in all macros

#set     beam::ERROR40::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR40::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR41: overwriting a literal string
#

set      beam::ERROR41::severity             ""

set      beam::ERROR41::enabling_policy      "always" 
#set     beam::ERROR41::enabling_policy      "" 

lappend  beam::ERROR41::disabling_comment    "overwriting"

set      beam::ERROR41::disabled_files       "";        # enable in all files
set      beam::ERROR41::disabled_functions   "";        # enable all functions
set      beam::ERROR41::disabled_macros      "";        # enable in all macros

#set     beam::ERROR41::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR41::complaint_options    "show_values  show_calls_with_side_effect"

#
# ERROR42: changing value of variable to be preserved
#

set      beam::ERROR42::severity             ""

set      beam::ERROR42::enabling_policy      "always" 
#set     beam::ERROR42::enabling_policy      "" 

lappend  beam::ERROR42::disabling_comment    "changed value"

set      beam::ERROR42::disabled_files       "";        # enable in all files
set      beam::ERROR42::disabled_functions   "";        # enable all functions
set      beam::ERROR42::disabled_macros      "";        # enable in all macros

#set     beam::ERROR42::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR42::complaint_options    "show_values  show_calls_with_side_effect"


#
# ERROR43: loop condition unchanged by loop
#

set      beam::ERROR43::severity             ""

set      beam::ERROR43::enabling_policy      "always" 
#set     beam::ERROR43::enabling_policy      "without_exit" 
#set     beam::ERROR43::enabling_policy      "" 

lappend  beam::ERROR43::disabling_comment    "infinite loop"

set      beam::ERROR43::disabled_files       "";        # enable in all files
set      beam::ERROR43::disabled_functions   "";        # enable all functions
set      beam::ERROR43::disabled_macros      "";        # enable in all macros

#set     beam::ERROR43::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR43::complaint_options    "show_values  show_calls_with_side_effect"



#
# ERROR44(heap_memory): failure to free memory
#

lappend  beam::ERROR44(heap_memory)::disabling_comment        "memory not freed"
set      beam::ERROR44(heap_memory)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR44



#
# ERROR44(file): failure to close a file
#

lappend  beam::ERROR44(file)::disabling_comment        "file not closed"
set      beam::ERROR44(file)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR44


#
# ERROR44(glob): failure to call globfree()
#

lappend  beam::ERROR44(glob)::disabling_comment        "glob not freed"
set      beam::ERROR44(glob)::unknown_functions_effect "saves_parameter"

# glob(..., GLOB_APPEND, ..., p) does not cause the leak for p
# because the existing content of p is appended to the new one
lappend  beam::ERROR44(glob)::disabling_comment    "GLOB_APPEND"

# All other parameters default to their setting in beam::ERROR44



#
# ERROR44(addrinfo): failure to call freeaddrinfo()
#

lappend  beam::ERROR44(addrinfo)::disabling_comment        "addrinfo not freed"
set      beam::ERROR44(addrinfo)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR44



#
# ERROR44(directory): failure to call closedir()
#

lappend  beam::ERROR44(directory)::disabling_comment        "directory not closed"
set      beam::ERROR44(directory)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR44


#
# ERROR44(cursor): failure to deallocate cursor
#

lappend  beam::ERROR44(cursor)::disabling_comment        "cursor not deleted"
set      beam::ERROR44(cursor)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR44


#
# ERROR44(lock): failure to uninitialize a lock
#

lappend  beam::ERROR44(lock)::disabling_comment        "lock remains initialized"
set      beam::ERROR44(lock)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR44


#
# ERROR44(_recursive_lock): failure to release a lock
#

lappend  beam::ERROR44(_recursive_lock)::disabling_comment        "lock not released"
set      beam::ERROR44(_recursive_lock)::unknown_functions_effect ""

# All other parameters default to their setting in beam::ERROR44


#
# ERROR44: failure to deallocate a resource (for resources not mentioned above)
#

set      beam::ERROR44::severity             ""

#set     beam::ERROR44::enabling_policy     "always" 
set      beam::ERROR44::enabling_policy     "sometimes" 

lappend  beam::ERROR44::disabling_comment    "resource may remain"

set      beam::ERROR44::unknown_functions_effect "saves_parameter"

set      beam::ERROR44::disabled_files       "";        # enable in all files
set      beam::ERROR44::disabled_functions   "main";    # does not matter in main
set      beam::ERROR44::disabled_macros      "";        # enable in all macros

#set     beam::ERROR44::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::ERROR44::complaint_options    "show_values  show_calls_with_side_effect"





#
# MISTAKE1: statement without effect
#

set      beam::MISTAKE1::severity             ""

set      beam::MISTAKE1::enabling_policy      "always"
#set     beam::MISTAKE1::enabling_policy      ""

lappend  beam::MISTAKE1::disabling_comment    "no effect"
lappend  beam::MISTAKE1::disabling_comment    "BEAM_preserve"

set      beam::MISTAKE1::disabled_files       "";        # enable all files
set      beam::MISTAKE1::disabled_functions   "";        # enable all functions
set      beam::MISTAKE1::disabled_macros      "*";       # disable in all macros

#set     beam::MISTAKE1::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE1::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE2: unreachable statement
#

set      beam::MISTAKE2::severity             ""

#set     beam::MISTAKE2::enabling_policy      "always"
set      beam::MISTAKE2::enabling_policy      ""

set      beam::MISTAKE2::enabled_for_handler  "no"; # no complaint about unreachable handlers

lappend  beam::MISTAKE2::disabling_comment    "unreachable"
lappend  beam::MISTAKE2::disabling_comment    "not reach"

set      beam::MISTAKE2::disabled_files       "";        # enable all files
set      beam::MISTAKE2::disabled_functions   "";        # enable all functions
set      beam::MISTAKE2::disabled_macros      "*";       # disable in all macros

#set     beam::MISTAKE2::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE2::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE3: == binds more tightly than &
#

set      beam::MISTAKE3::severity             ""

set      beam::MISTAKE3::enabling_policy      "always"
#set     beam::MISTAKE3::enabling_policy      ""

lappend  beam::MISTAKE3::disabling_comment    "precedence"
lappend  beam::MISTAKE3::disabling_comment    "meant single"

set      beam::MISTAKE3::disabled_files       "";        # enable all files
set      beam::MISTAKE3::disabled_functions   "";        # enable all functions
set      beam::MISTAKE3::disabled_macros      "";        # enable in all macros

#lappend "beam::MISTAKE3::unexpectedly_tighter(=)"  "==" "!=" "<" ">" "<=" ">="
lappend  "beam::MISTAKE3::unexpectedly_tighter(&)"  "==" "!=" "<" ">" "<=" ">="
lappend  "beam::MISTAKE3::unexpectedly_tighter(|)"  "==" "!=" "<" ">" "<=" ">="
lappend  "beam::MISTAKE3::unexpectedly_tighter(^)"  "==" "!=" "<" ">" "<=" ">="
#lappend "beam::MISTAKE3::unexpectedly_tighter(?)"   "|" "+" "-" "/"

#set     beam::MISTAKE3::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE3::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE4: Missing break in a case
#

set      beam::MISTAKE4::severity             ""

set      beam::MISTAKE4::enabling_policy      "always"
#set     beam::MISTAKE4::enabling_policy      ""

lappend  beam::MISTAKE4::disabling_comment    "fall through"
lappend  beam::MISTAKE4::disabling_comment    "fall-through"
lappend  beam::MISTAKE4::disabling_comment    "fall thru"
lappend  beam::MISTAKE4::disabling_comment    "fall-thru"
lappend  beam::MISTAKE4::disabling_comment    "fall to"
lappend  beam::MISTAKE4::disabling_comment    "fall into"
lappend  beam::MISTAKE4::disabling_comment    "fallthrough"
lappend  beam::MISTAKE4::disabling_comment    "fallthru"
lappend  beam::MISTAKE4::disabling_comment    "falling through"
lappend  beam::MISTAKE4::disabling_comment    "falling thru"
lappend  beam::MISTAKE4::disabling_comment    "falling to"
lappend  beam::MISTAKE4::disabling_comment    "falling into"
lappend  beam::MISTAKE4::disabling_comment    "falls through"
lappend  beam::MISTAKE4::disabling_comment    "falls thru"
lappend  beam::MISTAKE4::disabling_comment    "falls to"
lappend  beam::MISTAKE4::disabling_comment    "falls into"
lappend  beam::MISTAKE4::disabling_comment    "drop through"
lappend  beam::MISTAKE4::disabling_comment    "drop thru"
lappend  beam::MISTAKE4::disabling_comment    "move to the next case"
lappend  beam::MISTAKE4::disabling_comment    "go to the next case"

# disabling_comment_policy says where a relevant
# disabling comment might be.
# Below are all the cases we recognize, where
# L1 is the line of the current case label,
# L2 is the line of case label into which the current one is falling,

#set     beam::MISTAKE4::disabling_comment_policy        "L1+1 to L2-1"
#set     beam::MISTAKE4::disabling_comment_policy        "L1   to L2-1"
#set     beam::MISTAKE4::disabling_comment_policy        "L1+1 to L2"
# NOTE: changing the comment_policy changes innocence codes !
set      beam::MISTAKE4::disabling_comment_policy        "L1   to L2"
#set     beam::MISTAKE4::disabling_comment_policy                "L2-1"

set      beam::MISTAKE4::disabled_files       "";        # enable all files
set      beam::MISTAKE4::disabled_functions   "";        # enable all functions
set      beam::MISTAKE4::disabled_macros      "*";       # disable all macros

#set     beam::MISTAKE4::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE4::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE5: condition has constant value
#

set      beam::MISTAKE5::severity             ""

#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_local";                 # from within same statement
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_non_local";             # from another statement
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_call";                  # from properties of func call
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_declaration";           # e.g. if(x <= MAX)
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_sizeof";                # e.g. if(sizeof(int) == sizeof(log))
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_literal";               # e.g. if(1)
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_operation_on_literals"; # e.g. if(1 + 1 == 2)
#set     beam::MISTAKE5::enabling_policy      "condition_implied_from_right_of_assignment";   # e.g. if(x = 1)
#set     beam::MISTAKE5::enabling_policy      "condition_inexplicable";
#set     beam::MISTAKE5::enabling_policy      "always";
#set     beam::MISTAKE5::enabling_policy      ""
set      beam::MISTAKE5::enabling_policy      {whole_condition_implied_from_declaration
                                               condition_implied_from_right_of_assignment
                                               sub_condition_implied_from_literal
                                               case_condition_*}

lappend  beam::MISTAKE5::disabling_comment    "constant condition"
lappend  beam::MISTAKE5::disabling_comment    "condition always"
lappend  beam::MISTAKE5::disabling_comment    "case unreachable"

set      beam::MISTAKE5::disabled_files       "";        # enable all files
set      beam::MISTAKE5::disabled_functions   "";        # enable all functions
set      beam::MISTAKE5::disabled_macros      "*";       # disable in all macros

#set     beam::MISTAKE5::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE5::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE6: strncmp has third argument that does not match 1st or 2nd
#

set      beam::MISTAKE6::severity             ""

set      beam::MISTAKE6::enabling_policy      "always - strlen+1"
#set     beam::MISTAKE6::enabling_policy      ""

lappend  beam::MISTAKE6::disabling_comment    "length does not match"
lappend  beam::MISTAKE6::disabling_comment    "mean length"

set      beam::MISTAKE6::disabled_files       "";        # enable all files
set      beam::MISTAKE6::disabled_functions   "";        # enable all functions
set      beam::MISTAKE6::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE6::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE6::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE7: for(i ...)   for (i ...)
#

set      beam::MISTAKE7::severity             ""

#set     beam::MISTAKE7::enabling_policy      "always"
#set     beam::MISTAKE7::enabling_policy      ""
set      beam::MISTAKE7::enabling_policy      "only_for_loops"

lappend  beam::MISTAKE7::disabling_comment    "shared iterator"
lappend  beam::MISTAKE7::disabling_comment    "effect on enclosing loop"

set      beam::MISTAKE7::disabled_files       "";        # enable all files
set      beam::MISTAKE7::disabled_functions   "";        # enable all functions
set      beam::MISTAKE7::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE7::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE7::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE8: arg of malloc should involve sizeof
#

set      beam::MISTAKE8::severity             ""

#set     beam::MISTAKE8::enabling_policy      "always"
set      beam::MISTAKE8::enabling_policy      ""

lappend  beam::MISTAKE8::disabling_comment    "allocated size"

set      beam::MISTAKE8::disabled_files       "";        # enable all files
set      beam::MISTAKE8::disabled_functions   "";        # enable all functions
set      beam::MISTAKE8::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE8::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE8::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE9 no longer exists.
#


#
# MISTAKE10: allocating invalid amount (0 or negative)
#

set      beam::MISTAKE10::severity             ""

#set     beam::MISTAKE10::enabling_policy      "always"
set      beam::MISTAKE10::enabling_policy      ""

lappend  beam::MISTAKE10::disabling_comment    "bad allocated size"

set      beam::MISTAKE10::disabled_files       "";        # enable all files
set      beam::MISTAKE10::disabled_functions   "";        # enable all functions
set      beam::MISTAKE10::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE10::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE10::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE11: multiple invocations
#

set      beam::MISTAKE11::severity             ""

#set     beam::MISTAKE11::enabling_policy      "no_side_effect";      # complain if the function is known to BEAM to have no side-effect
#set     beam::MISTAKE11::enabling_policy      "some_side_effect";    # complain if the function is known to BEAM to have a  side-effect
#set     beam::MISTAKE11::enabling_policy      "unknown_side_effect"; # complain if the function is unknown to BEAM
#set     beam::MISTAKE11::enabling_policy      "always";	       # all of the above
set      beam::MISTAKE11::enabling_policy      "";	               # none of the above

lappend  beam::MISTAKE11::disabling_comment    "multiple invocations"

set      beam::MISTAKE11::disabled_files       "";        # enable all files
set      beam::MISTAKE11::disabled_functions   "";        # enable all functions
set      beam::MISTAKE11::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE11::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE11::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE12: abs(1.2) instead of fabs(1.2)
#

set      beam::MISTAKE12::severity             ""

set      beam::MISTAKE12::enabling_policy      "always"
#set     beam::MISTAKE12::enabling_policy      ""

lappend  beam::MISTAKE12::disabling_comment    "loss of precision"

set      beam::MISTAKE12::disabled_files       "";        # enable all files
set      beam::MISTAKE12::disabled_functions   "";        # enable all functions
set      beam::MISTAKE12::disabled_macros      "";        # enable in all macros

set      "beam::MISTAKE12::instead_of(abs,float)"  "fabs"
set      "beam::MISTAKE12::instead_of(abs,double)" "fabs"
set      "beam::MISTAKE12::instead_of(abs,long)"   "labs"

#set     beam::MISTAKE12::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE12::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE13: two enumerators have the same value
#

set      beam::MISTAKE13::severity             ""

#set     beam::MISTAKE13::enabling_policy      "always"
set      beam::MISTAKE13::enabling_policy      ""

lappend  beam::MISTAKE13::disabling_comment    "duplicate enumerator value"
lappend  beam::MISTAKE13::disabling_comment    "shared value"

set      beam::MISTAKE13::disabled_files       "";        # enable all files
set      beam::MISTAKE13::disabled_functions   "";        # enable all functions
set      beam::MISTAKE13::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE13::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE13::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE14: call f() where f is not declared
#

set      beam::MISTAKE14::severity             ""

#set     beam::MISTAKE14::enabling_policy      "always"
#set     beam::MISTAKE14::enabling_policy      ""
set      beam::MISTAKE14::enabling_policy      "capitals"

lappend  beam::MISTAKE14::disabling_comment    "undeclared function"

set      beam::MISTAKE14::disabled_files       "";        # enable all files
set      beam::MISTAKE14::disabled_functions   "";        # enable all functions
set      beam::MISTAKE14::disabled_macros      "";        # enable in all macros

#set     beam::MISTAKE14::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE14::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE15 no longer exists.
#


#
# MISTAKE16(while): while (   );
#

set      beam::MISTAKE16(while)::enabling_policy      "block_stmt + indented_stmt"
lappend  beam::MISTAKE16(while)::disabling_comment    "empty while"


#
# MISTAKE16(for): for (   );
#

set      beam::MISTAKE16(for)::enabling_policy      "block_stmt + indented_stmt"
lappend  beam::MISTAKE16(for)::disabling_comment    "empty for"


#
# MISTAKE16(switch): switch (   );
#

set      beam::MISTAKE16(switch)::enabling_policy      "always"
lappend  beam::MISTAKE16(switch)::disabling_comment    "empty switch"


#
# MISTAKE16: if (   );
#
# The default case of MISTAKE16 describes MISTAKE16(if) for 
# backward compatibility -- MISTAKE16 used to apply to "if" only

set      beam::MISTAKE16::severity             ""

set      beam::MISTAKE16::enabling_policy      "always"
#set     beam::MISTAKE16::enabling_policy      ""

lappend  beam::MISTAKE16::disabling_comment    "empty if"

set      beam::MISTAKE16::disabled_files       "";        # enable all files
set      beam::MISTAKE16::disabled_functions   "";        # enable all functions
set      beam::MISTAKE16::disabled_macros      "*";       # disable in all macros

#set     beam::MISTAKE16::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE16::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE17: loop executed at most once
#

set      beam::MISTAKE17::severity             ""

set      beam::MISTAKE17::enabling_policy      "always"
#set     beam::MISTAKE17::enabling_policy      ""

lappend  beam::MISTAKE17::disabling_comment    "loop doesn't iterate"

set      beam::MISTAKE17::disabled_files       "";        # enable all files
set      beam::MISTAKE17::disabled_functions   "";        # enable all functions
set      beam::MISTAKE17::disabled_macros      "*";       # disable in all macros

#set     beam::MISTAKE17::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE17::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE18: comparing pointers to strings
#

set      beam::MISTAKE18::severity             ""

#set     beam::MISTAKE18::enabling_policy      "always"
#set     beam::MISTAKE18::enabling_policy      "no_overlap"; # only when they do not point into same memory
set      beam::MISTAKE18::enabling_policy      ""

lappend  beam::MISTAKE18::disabling_comment    "comparing pointers"

set      beam::MISTAKE18::disabled_files       "";        # enable all files
set      beam::MISTAKE18::disabled_functions   "";        # enable all functions
set      beam::MISTAKE18::disabled_macros      "*";       # disable in all macros

# Suggestions for comparing strings

set      "beam::MISTAKE18::compare_function(char *)"          "strcmp"
set      "beam::MISTAKE18::compare_function(unsigned char *)" "strcmp"

#set     beam::MISTAKE18::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE18::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE19: missing \0 in: char s[2] = "ab";
#

set      beam::MISTAKE19::severity             ""

#set     beam::MISTAKE19::enabling_policy      "always"
set      beam::MISTAKE19::enabling_policy      "unterminated"; # no complaint for char s[2] = "a\0";
#set     beam::MISTAKE19::enabling_policy      ""

lappend  beam::MISTAKE19::disabling_comment    "terminating 0 missed"

set      beam::MISTAKE19::disabled_files       "";       # enable all files
set      beam::MISTAKE19::disabled_functions   "";       # enable all functions
set      beam::MISTAKE19::disabled_macros      "";       # enable in all macros

#set     beam::MISTAKE19::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE19::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE20: unreachable exception handler
#

set      beam::MISTAKE20::severity             ""

set      beam::MISTAKE20::enabling_policy      "always"
#set     beam::MISTAKE20::enabling_policy      ""

lappend  beam::MISTAKE20::disabling_comment    "unreachable handler"

set      beam::MISTAKE20::disabled_files       "";       # enable all files
set      beam::MISTAKE20::disabled_functions   "";       # enable all functions
set      beam::MISTAKE20::disabled_macros      "";       # enable in all macros

#set     beam::MISTAKE20::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE20::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE21: print advisory message
#

set      beam::MISTAKE21::severity             ""

#set     beam::MISTAKE21::enabling_policy      "* - informational" # Patterns match advisory category
#set     beam::MISTAKE21::enabling_policy      "security + inf*"
#set     beam::MISTAKE21::enabling_policy      "always"
set      beam::MISTAKE21::enabling_policy      ""

lappend  beam::MISTAKE21::disabling_comment    "discouraged function"

set      beam::MISTAKE21::disabled_files       "";       # enable all files
set      beam::MISTAKE21::disabled_functions   "";       # enable all functions
set      beam::MISTAKE21::disabled_macros      "";       # enable in all macros

#set     beam::MISTAKE21::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE21::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE22: missing default
#

set      beam::MISTAKE22::severity             ""

#set     beam::MISTAKE22::enabling_policy      "always"
set      beam::MISTAKE22::enabling_policy      ""

lappend  beam::MISTAKE22::disabling_comment    "default omitted"

set      beam::MISTAKE22::disabled_files       "";       # enable all files
set      beam::MISTAKE22::disabled_functions   "";       # enable all functions
set      beam::MISTAKE22::disabled_macros      "";       # enable in all macros

#set     beam::MISTAKE22::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE22::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE23: missing test
#

set      beam::MISTAKE23::severity             ""

#set     beam::MISTAKE23::enabling_policy      "always"
set      beam::MISTAKE23::enabling_policy      ""

lappend  beam::MISTAKE23::disabling_comment    "omitted test"

set      beam::MISTAKE23::disabled_files       "";       # enable all files
set      beam::MISTAKE23::disabled_functions   "";       # enable all functions
set      beam::MISTAKE23::disabled_macros      "";       # enable in all macros

# Functions to complain about

#set     beam::MISTAKE23::functions                   "ut_foo + my_func"
set      beam::MISTAKE23::functions                   "*";                   # enable all callees

#set     beam::MISTAKE23::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::MISTAKE23::functions_by_qualified_name ""
  
#lappend beam::MISTAKE23::functions_by_signature      { clas::func(int &) }
set      beam::MISTAKE23::functions_by_signature      {}
  
#set     beam::MISTAKE23::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE23::complaint_options    "show_values  show_calls_with_side_effect"


#
# MISTAKE24: boolean used incorrectly
#

set      beam::MISTAKE24::severity             ""

set      beam::MISTAKE24::enabling_policy      "always"
#set     beam::MISTAKE24::enabling_policy      "exact_operators"
#set     beam::MISTAKE24::enabling_policy      ""

lappend  beam::MISTAKE24::disabling_comment    "not boolean"

set      beam::MISTAKE24::disabled_files       "";       # enable all files
set      beam::MISTAKE24::disabled_functions   "";       # enable all functions
set      beam::MISTAKE24::disabled_macros      "";       # enable in all macros

# The complaint will be issued if the boolean value
# is used in one of the following operators.
#
# For enabling_policy="always", a "|" in the list also matches "|=", etc.
#
# For enabling_policy="exact_operators", a "|" in the list won't
# match "|=", etc. "|=" would have to be in the list explicitly.

#set     beam::MISTAKE24::operators { < > <= >= <? >? >> << & | ~ ^ / + - * % [] }
set      beam::MISTAKE24::operators { < > <= >= <? >? & | ~ ^ / * % }

# The complaint will also be issued if the boolean value
# is used in one of these mixed operators and other operands
# are not also boolean. "bool | bool" would be ok where as
# "bool | int" would not, if "|" appeared in mixed_operators.
#
# This list also obeys the rule that enabling_policy="always"
# does a fuzzy match and "exact_operators" requires an exact match.
  
#set     beam::MISTAKE24::mixed_operators { & | ~ ^ }
set      beam::MISTAKE24::mixed_operators {}

#set     beam::MISTAKE24::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::MISTAKE24::complaint_options    "show_values  show_calls_with_side_effect"



#
# MISTAKE25: pointer addition out of range
#

set         beam::MISTAKE25::severity                        ""

#set        beam::MISTAKE25::enabling_policy                 "always"
#set        beam::MISTAKE25::enabling_policy                 ""
#set        beam::MISTAKE25::enabling_policy                 "lower_bound"
#set        beam::MISTAKE25::enabling_policy                 "upper_bound"
#set        beam::MISTAKE25::enabling_policy                 "array_bound"
set         beam::MISTAKE25::enabling_policy                 "negative_bound + excessive_bound"
  
lappend     beam::MISTAKE25::disabling_comment               "out of range pointer"
lappend     beam::MISTAKE25::disabling_comment               "subscript out of range"
lappend     beam::MISTAKE25::disabling_comment               "will be in range"

set         beam::MISTAKE25::disabled_files                  "";        # enable all files
set         beam::MISTAKE25::disabled_functions              "";        # enable all functions
set         beam::MISTAKE25::disabled_macros                 "va_*";    # disable vararg macros

  
#set        beam::MISTAKE25::complaint_options              "show_source"; # print source line
# print values of variables and calls with side-effect
set         beam::MISTAKE25::complaint_options              "show_values  show_calls_with_side_effect"


#
# WARNING1: static variable never fetched
#

set      beam::WARNING1::severity             ""

#set     beam::WARNING1::enabling_policy      "always"
#set     beam::WARNING1::enabling_policy      "assigned";             # complain only if assigned
#set     beam::WARNING1::enabling_policy      "initialized";          # complain only if initialized explicitly
#set     beam::WARNING1::enabling_policy      "assigned_initialized"; # complain only if assigned or initialized explicitly
set      beam::WARNING1::enabling_policy      ""

lappend  beam::WARNING1::disabling_comment    "not used"
lappend  beam::WARNING1::disabling_comment    "not needed"
lappend  beam::WARNING1::disabling_comment    "unused"

set      beam::WARNING1::disabled_files       "";        # enable all files
set      beam::WARNING1::disabled_functions   "";        # enable all functions
set      beam::WARNING1::disabled_macros      "*";       # disable in all macros

#set     beam::WARNING1::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING1::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING2: automatic variable never fetched
#

set      beam::WARNING2::severity             ""

#set     beam::WARNING2::enabling_policy      "always"
#set     beam::WARNING2::enabling_policy      "assigned";             # complain only if assigned
#set     beam::WARNING2::enabling_policy      "initialized";          # complain only if initialized
#set     beam::WARNING2::enabling_policy      "assigned_initialized"; # complain only if assigned or initialized
set      beam::WARNING2::enabling_policy      ""

lappend  beam::WARNING2::disabling_comment    "not used"
lappend  beam::WARNING2::disabling_comment    "not needed"
lappend  beam::WARNING2::disabling_comment    "unused"

set      beam::WARNING2::disabled_files       "";        # enable all files
set      beam::WARNING2::disabled_functions   "";        # enable all functions
set      beam::WARNING2::disabled_macros      "*";       # disable in all macros

#set     beam::WARNING2::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING2::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING3: parameter never fetched
#

set      beam::WARNING3::severity             ""

#set     beam::WARNING3::enabling_policy      "always"
set      beam::WARNING3::enabling_policy      ""

lappend  beam::WARNING3::disabling_comment    "not used"
lappend  beam::WARNING3::disabling_comment    "unused"

set      beam::WARNING3::disabled_files       "";        # enable all files
set      beam::WARNING3::disabled_functions   "";        # enable all functions
set      beam::WARNING3::disabled_macros      "";        # enable in all macros

#set     beam::WARNING3::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING3::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING4: f(struct)
#

set      beam::WARNING4::severity             ""

#set     beam::WARNING4::enabling_policy      "always"
#set     beam::WARNING4::enabling_policy      "always - inlined - return" # Complain for non-inlined parameters
#set     beam::WARNING4::enabling_policy      "return - inlined"          # Complain for non-inlined returns
set      beam::WARNING4::enabling_policy      ""

lappend  beam::WARNING4::disabling_comment    "structure parameter"
lappend  beam::WARNING4::disabling_comment    "structure return"

set      beam::WARNING4::disabled_files       "";        # enable all files
set      beam::WARNING4::disabled_functions   "";        # enable all functions
set      beam::WARNING4::disabled_macros      "";        # enable in all macros
set      beam::WARNING4::disabled_types       "";        # enable in all types

#
# Value -1 is magic. It will be replaced under the covers with sizeof(long)
# for the target machine.
#
set      beam::WARNING4::threshold_in_bytes   -1

#set     beam::WARNING4::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING4::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING5: if (a = b)
#

set      beam::WARNING5::severity             ""

#set     beam::WARNING5::enabling_policy      "always"
#set     beam::WARNING5::enabling_policy      ""
#set     beam::WARNING5::enabling_policy      "access"
set      beam::WARNING5::enabling_policy      "constant"
#set     beam::WARNING5::enabling_policy      "gcc_convention"

lappend  beam::WARNING5::disabling_comment   "assignment in condition"
lappend  beam::WARNING5::disabling_comment   "meant single"

set      beam::WARNING5::disabled_files       "";        # enable all files
set      beam::WARNING5::disabled_functions   "";        # enable all functions
set      beam::WARNING5::disabled_macros      "";        # enable in all macros

#set     beam::WARNING5::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING5::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING6: dirty cast, e.g., big number into small enum
#

set      beam::WARNING6::severity             ""

#set     beam::WARNING6::enabling_policy      "always"
set      beam::WARNING6::enabling_policy      ""

lappend  beam::WARNING6::disabling_comment    "value out of range"
lappend  beam::WARNING6::disabling_comment    "dirty cast"

set      beam::WARNING6::disabled_files       "";        # enable all files
set      beam::WARNING6::disabled_functions   "";        # enable all functions
set      beam::WARNING6::disabled_macros      "";        # enable in all macros

#set     beam::WARNING6::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING6::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING7: dangerous cast, e.g., (char) int
#

set      beam::WARNING7::severity             ""

#set     beam::WARNING7::enabling_policy      "always"
set      beam::WARNING7::enabling_policy      ""

set      beam::WARNING7::enabled_for_compiler_generated  "yes"; # If you turn it on you probably want it
set      beam::WARNING7::disabled_for_user_written       "yes"; # only for compiler generated casts

lappend  beam::WARNING7::disabling_comment    "dangerous cast"

set      beam::WARNING7::disabled_files       "";        # enable all files
set      beam::WARNING7::disabled_functions   "";        # enable all functions
set      beam::WARNING7::disabled_macros      "";        # enable in all macros

#set     beam::WARNING7::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING7::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING8: unused label
#

set      beam::WARNING8::severity             ""

#set     beam::WARNING8::enabling_policy      "always"
set      beam::WARNING8::enabling_policy      ""

lappend  beam::WARNING8::disabling_comment    "not used"
lappend  beam::WARNING8::disabling_comment    "unused"

set      beam::WARNING8::disabled_files       "";        # enable all files
set      beam::WARNING8::disabled_functions   "";        # enable all functions
set      beam::WARNING8::disabled_macros      "*";       # disable in all macros

#set     beam::WARNING8::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING8::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING9: unused return value
#

set      beam::WARNING9::severity             ""

#set     beam::WARNING9::enabling_policy      "always"
set      beam::WARNING9::enabling_policy      ""

lappend  beam::WARNING9::disabling_comment    "ignored return"

set      beam::WARNING9::disabled_files       "";        # enable all files
set      beam::WARNING9::disabled_functions   "";        # enable all functions
set      beam::WARNING9::disabled_macros      "*";       # disable in all macros

# Functions to complain about

#set     beam::WARNING9::functions                   "ut_foo + my_func"
set      beam::WARNING9::functions                   ""

#set     beam::WARNING9::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::WARNING9::functions_by_qualified_name ""
  
#lappend beam::WARNING9::functions_by_signature      { clas::func(int &) }
set      beam::WARNING9::functions_by_signature      {}
  
#set     beam::WARNING9::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING9::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING10: assert with side effect
#

set      beam::WARNING10::severity             ""

#set     beam::WARNING10::enabling_policy      "always"
set      beam::WARNING10::enabling_policy      ""

lappend  beam::WARNING10::disabling_comment    "side effect"

set      beam::WARNING10::disabled_files       "";        # enable all files
set      beam::WARNING10::disabled_functions   "";        # enable all functions
set      beam::WARNING10::disabled_macros      "";        # enable in all macros

#set     beam::WARNING10::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING10::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING11: goto statement
#

set      beam::WARNING11::severity             ""

#set     beam::WARNING11::enabling_policy      "always"
set      beam::WARNING11::enabling_policy      ""

lappend  beam::WARNING11::disabling_comment    "goto required"

set      beam::WARNING11::disabled_files       "";        # enable all files
set      beam::WARNING11::disabled_functions   "";        # enable all functions
set      beam::WARNING11::disabled_macros      "";        # enable in all macros

#set     beam::WARNING11::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING11::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING12: (signed)  unsigned
#

set      beam::WARNING12::severity             ""

#set     beam::WARNING12::enabling_policy      "always"
set      beam::WARNING12::enabling_policy      ""

lappend  beam::WARNING12::disabling_comment    "(signed) unsigned"

set      beam::WARNING12::disabled_files       "";        # enable all files
set      beam::WARNING12::disabled_functions   "";        # enable all functions
set      beam::WARNING12::disabled_macros      "";        # enable in all macros

#set     beam::WARNING12::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING12::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING13: static function never used
#

set      beam::WARNING13::severity             ""

#set     beam::WARNING13::enabling_policy      "always"
set      beam::WARNING13::enabling_policy      ""

lappend  beam::WARNING13::disabling_comment    "not used"
lappend  beam::WARNING13::disabling_comment    "not needed"
lappend  beam::WARNING13::disabling_comment    "unused"

set      beam::WARNING13::disabled_files       "";        # enable all files
set      beam::WARNING13::disabled_functions   "";        # enable all functions
set      beam::WARNING13::disabled_macros      "*";       # disable in all macros

#set     beam::WARNING13::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING13::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING14: if (A == B), where A and B have different types
#

set      beam::WARNING14::severity             ""

#set     beam::WARNING14::enabling_policy      "always"
set      beam::WARNING14::enabling_policy      "enum_enum"; # complain if A and B have both enum types
#set     beam::WARNING14::enabling_policy      ""

lappend  beam::WARNING14::disabling_comment    "incompatible types"

set      beam::WARNING14::disabled_files       "";        # enable all files
set      beam::WARNING14::disabled_functions   "";        # enable all functions
set      beam::WARNING14::disabled_macros      "";        # enable in all macros
set      beam::WARNING14::disabled_types       "";        # enable in all types

#set     beam::WARNING14::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING14::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING15: if (A) stmt  without braces around stmt
#

set      beam::WARNING15::severity             ""

#set     beam::WARNING15::enabling_policy      "always";
#set     beam::WARNING15::enabling_policy      "same_line"; # complain if then clause on same line as if
#set     beam::WARNING15::enabling_policy      "next_line"; # complain if then clause lower than if
set      beam::WARNING15::enabling_policy      ""

lappend  beam::WARNING15::disabling_comment    "no braces"

set      beam::WARNING15::disabled_files       "*.pl8 *.ipl"; # does not work for pl8
set      beam::WARNING15::disabled_functions   "";        # enable all functions
set      beam::WARNING15::disabled_macros      "*";       # disable all macros because do not know line numbers

#set     beam::WARNING15::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING15::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING16: f(&x, &x)
#

set      beam::WARNING16::severity             ""

set      beam::WARNING16::enabling_policy      "like_scanf";       # apply to those like scanf
#set     beam::WARNING16::enabling_policy      "explicitly_given"; # apply to those matching the pattern functions
#set     beam::WARNING16::enabling_policy      "always";           # apply under either condition above
#set     beam::WARNING16::enabling_policy      "";                 # never issue this complaint

lappend  beam::WARNING16::disabling_comment    "parameter aliased"

set      beam::WARNING16::disabled_files       "";        # enable all files
set      beam::WARNING16::disabled_functions   "";        # enable all functions
set      beam::WARNING16::disabled_macros      "";        # enable in all macros

# Functions to complain about

#set     beam::WARNING16::functions                   "ut_foo + my_func"
# Printing a pointer twice is OK
set      beam::WARNING16::functions                   "* - *printf"

#set     beam::WARNING16::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::WARNING16::functions_by_qualified_name ""
  
#lappend beam::WARNING16::functions_by_signature      { clas::func(int &) }
set      beam::WARNING16::functions_by_signature      {}
  
#set     beam::WARNING16::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING16::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING17: repeated assignment
#

set      beam::WARNING17::severity             ""

#set     beam::WARNING17::enabling_policy      "always"
set      beam::WARNING17::enabling_policy      ""

lappend  beam::WARNING17::disabling_comment    "repeated assignment"

set      beam::WARNING17::disabled_files       "";        # enable all files
set      beam::WARNING17::disabled_functions   "";        # enable all functions
set      beam::WARNING17::disabled_macros      "";        # enable in all macros

#set     beam::WARNING17::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING17::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING18: assignment without copy constructor
#

set      beam::WARNING18::severity             ""

#set     beam::WARNING18::enabling_policy      "always"
set      beam::WARNING18::enabling_policy      ""

lappend  beam::WARNING18::disabling_comment    "no copy constructor"

set      beam::WARNING18::disabled_files       "";        # enable all files
set      beam::WARNING18::disabled_functions   "";        # enable all functions
set      beam::WARNING18::disabled_macros      "";        # enable in all macros
set      beam::WARNING18::disabled_classes     "";        # enable in all classes

#set     beam::WARNING18::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING18::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING19: if (strcmp(a, b))
#

set      beam::WARNING19::severity             ""

#set     beam::WARNING19::enabling_policy      "always"
set      beam::WARNING19::enabling_policy      ""

lappend  beam::WARNING19::disabling_comment    "strings unequal"

set      beam::WARNING19::disabled_files       "";        # enable all files
set      beam::WARNING19::disabled_functions   "";        # enable all functions
set      beam::WARNING19::disabled_macros      "";        # enable in all macros

# Functions to complain about

#set     beam::WARNING19::functions                   "my_func"
set      beam::WARNING19::functions                   ""

#set     beam::WARNING19::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::WARNING19::functions_by_qualified_name ""
  
#lappend beam::WARNING19::functions_by_signature      { clas::func(int &) }
lappend  beam::WARNING19::functions_by_signature      "strcmp" "strncmp"
  
#set     beam::WARNING19::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING19::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING20: shadow declarations
#

set      beam::WARNING20::severity             ""

#set     beam::WARNING20::enabling_policy      "always"
#set     beam::WARNING20::enabling_policy      "must_not_shadow"
set      beam::WARNING20::enabling_policy      ""

lappend  beam::WARNING20::disabling_comment    "shadows variable"

set      beam::WARNING20::disabled_files       "";        # enable all files
set      beam::WARNING20::disabled_functions   "";        # enable all functions
set      beam::WARNING20::disabled_macros      "";        # enable in all macros

#set     beam::WARNING20::must_not_shadow(global)         "parameter"
#set     beam::WARNING20::must_not_shadow(parameter)      "*"

#set     beam::WARNING20::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING20::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING21: strlen() as bool
#

set      beam::WARNING21::severity             ""

#set     beam::WARNING21::enabling_policy      "always"
set      beam::WARNING21::enabling_policy      ""

lappend  beam::WARNING21::disabling_comment    "boolean result"

set      beam::WARNING21::disabled_files       "";        # enable all files
set      beam::WARNING21::disabled_functions   "";        # enable all functions
set      beam::WARNING21::disabled_macros      "";        # enable in all macros

# Functions to complain about

#set     beam::WARNING21::functions                   "ut_foo + my_func"
set      beam::WARNING21::functions                   "strlen"

#set     beam::WARNING21::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::WARNING21::functions_by_qualified_name ""
  
#lappend beam::WARNING21::functions_by_signature      { clas::func(int &) }
set      beam::WARNING21::functions_by_signature      {}

#set     beam::WARNING21::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING21::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING22: a | b with booleans (Java only)
#

set      beam::WARNING22::severity             ""

#set     beam::WARNING22::enabling_policy      "always"
set      beam::WARNING22::enabling_policy      ""

lappend  beam::WARNING22::disabling_comment    "no short-circuit"

set      beam::WARNING22::disabled_files       "";        # enable all files
set      beam::WARNING22::disabled_functions   "";        # enable all functions
set      beam::WARNING22::disabled_macros      "";        # enable in all macros

#set     beam::WARNING22::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING22::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING23: arg#/parm# mismatch
#

set      beam::WARNING23::severity             ""

#set     beam::WARNING23::enabling_policy      "always"
set      beam::WARNING23::enabling_policy      ""

lappend  beam::WARNING23::disabling_comment    "args OK"

set      beam::WARNING23::disabled_files       "";        # enable all files
set      beam::WARNING23::disabled_functions   "";        # enable all functions
set      beam::WARNING23::disabled_macros      "";        # enable in all macros

lappend  beam::WARNING23::functions            "*";  # callees

#set     beam::WARNING23::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING23::complaint_options    "show_values  show_calls_with_side_effect"


#
# WARNING24: new StringBuffer('c') or 'c' + 'd' (Java only)
#
  
set      beam::WARNING24::severity             ""
  
set      beam::WARNING24::enabling_policy      "always"
#set     beam::WARNING24::enabling_policy      ""
  
lappend  beam::WARNING24::disabling_comment    "promoted character literal"

set      beam::WARNING24::disabled_files       "";        # enable all files
set      beam::WARNING24::disabled_functions   "";        # enable all functions
set      beam::WARNING24::disabled_macros      "";        # enable in all macros
  
# Functions to complain about

#set     beam::WARNING24::functions                   "ut_foo + my_func"
set      beam::WARNING24::functions                   ""

#set     beam::WARNING24::functions_by_qualified_name "ns::foo* - ns::foobar"
set      beam::WARNING24::functions_by_qualified_name ""
  
#lappend beam::WARNING24::functions_by_signature      { clas::func(int &) }
set      beam::WARNING24::functions_by_signature      {}

#set     beam::WARNING24::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::WARNING24::complaint_options    "show_values  show_calls_with_side_effect"

#
# Each of the portability checks will use this information to
# decide if a problem would occur when porting code from the
# currently configured target machine to a machine described
# by this target.
#

set      beam::portability_target::target_sizeof_bool       1
set      beam::portability_target::target_sizeof_short      2
set      beam::portability_target::target_sizeof_int        4
set      beam::portability_target::target_sizeof_long       8
set      beam::portability_target::target_sizeof_long_long  8
set      beam::portability_target::target_sizeof_pointer    8

# Sizes of any user-defined type can be set here if
# they are different on the portability target machine.
# For example, size_t may be typedef'd to int on the
# native machine, and long on the target machine. In
# those cases, BEAM needs more information about
# the size of the type.

# These should match target_sizeof_pointer.
set      beam::portability_target::target_sizeof_type(intptr_t)   8
set      beam::portability_target::target_sizeof_type(uintptr_t)  8


#
# PORTABILITY1: (int) ptr
#

set      beam::PORTABILITY1::severity             ""

#set     beam::PORTABILITY1::enabling_policy      "always"
set      beam::PORTABILITY1::enabling_policy      ""

set      beam::PORTABILITY1::enabled_for_compiler_generated  "yes"
set      beam::PORTABILITY1::disabled_for_user_written       "yes"; # do not complain about explicit cast

lappend  beam::PORTABILITY1::disabling_comment    "pointer cast as int"
lappend  beam::PORTABILITY1::disabling_comment    "fits into int"

set      beam::PORTABILITY1::disabled_files       "";        # enable all files
set      beam::PORTABILITY1::disabled_functions   "";        # enable all functions
set      beam::PORTABILITY1::disabled_macros      "";        # enable in all macros

#set     beam::PORTABILITY1::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::PORTABILITY1::complaint_options    "show_values  show_calls_with_side_effect"


#
# PORTABILITY2: (int) long
#

set      beam::PORTABILITY2::severity             ""

#set     beam::PORTABILITY2::enabling_policy      "always"
set      beam::PORTABILITY2::enabling_policy      ""

set      beam::PORTABILITY2::enabled_for_compiler_generated  "yes"
set      beam::PORTABILITY2::disabled_for_user_written       "yes"; # do not complain about explicit cast

lappend  beam::PORTABILITY2::disabling_comment    "long cast as int"
lappend  beam::PORTABILITY2::disabling_comment    "fits into int"

set      beam::PORTABILITY2::disabled_files       "";        # enable all files
set      beam::PORTABILITY2::disabled_functions   "";        # enable all functions
set      beam::PORTABILITY2::disabled_macros      "";        # enable in all macros

#set     beam::PORTABILITY2::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::PORTABILITY2::complaint_options    "show_values  show_calls_with_side_effect"


#
# PORTABILITY3: (ptr) num
#

set      beam::PORTABILITY3::severity             ""

#set     beam::PORTABILITY3::enabling_policy      "always"
set      beam::PORTABILITY3::enabling_policy      ""

set      beam::PORTABILITY3::enabled_for_compiler_generated  "yes"
set      beam::PORTABILITY3::disabled_for_user_written       "yes"; # do not complain about explicit cast

lappend  beam::PORTABILITY3::disabling_comment    "number cast as pointer"
lappend  beam::PORTABILITY3::disabling_comment    "treating as ptr"

set      beam::PORTABILITY3::disabled_files       "";        # enable all files
set      beam::PORTABILITY3::disabled_functions   "";        # enable all functions
set      beam::PORTABILITY3::disabled_macros      "";        # enable in all macros

#set     beam::PORTABILITY3::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::PORTABILITY3::complaint_options    "show_values  show_calls_with_side_effect"


#
# PORTABILITY4: (long) ptr
#

set      beam::PORTABILITY4::severity             ""

#set     beam::PORTABILITY4::enabling_policy      "always"
set      beam::PORTABILITY4::enabling_policy      ""

set      beam::PORTABILITY4::enabled_for_compiler_generated  "yes"
set      beam::PORTABILITY4::disabled_for_user_written       "yes"; # do not complain about explicit cast

lappend  beam::PORTABILITY4::disabling_comment    "pointer cast as a number"
lappend  beam::PORTABILITY4::disabling_comment    "irreproducible behavior"

set      beam::PORTABILITY4::disabled_files       "";        # enable all files
set      beam::PORTABILITY4::disabled_functions   "";        # enable all functions
set      beam::PORTABILITY4::disabled_macros      "";        # enable in all macros

# Pointers cast to types that match this pattern will not be complained about
set      beam::PORTABILITY4::disabled_types       "intptr_t + uintptr_t"

#set     beam::PORTABILITY4::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::PORTABILITY4::complaint_options    "show_values  show_calls_with_side_effect"


#
# PORTABILITY5: (long *) (int *)
#

set      beam::PORTABILITY5::severity             ""

#set     beam::PORTABILITY5::enabling_policy      "always"
set      beam::PORTABILITY5::enabling_policy      ""

set      beam::PORTABILITY5::enabled_for_compiler_generated  "yes"
set      beam::PORTABILITY5::disabled_for_user_written       "yes"; # do not complain about explicit cast

lappend  beam::PORTABILITY5::disabling_comment    "pointer to int cast as pointer to long"
lappend  beam::PORTABILITY5::disabling_comment    "no indirect assignment"

set      beam::PORTABILITY5::disabled_files       "";        # enable all files
set      beam::PORTABILITY5::disabled_functions   "";        # enable all functions
set      beam::PORTABILITY5::disabled_macros      "";        # enable in all macros

#set     beam::PORTABILITY5::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::PORTABILITY5::complaint_options    "show_values  show_calls_with_side_effect"


#
# SECURITY1: printf(s) where s is not explicit string
#

set      beam::SECURITY1::severity            ""

#set     beam::SECURITY1::enabling_policy     "always"
set      beam::SECURITY1::enabling_policy     ""

lappend  beam::SECURITY1::disabling_comment   "non-explicit pattern"

set      beam::SECURITY1::disabled_files      "";        # enable all files
set      beam::SECURITY1::disabled_functions  "";        # enable all functions
set      beam::SECURITY1::disabled_macros     "";        # enable in all macros

#set     beam::SECURITY1::complaint_options   "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::SECURITY1::complaint_options   "show_values  show_calls_with_side_effect"


#
# SECURITY2: copying string without checking length
#

set      beam::SECURITY2::severity            ""

#set     beam::SECURITY2::enabling_policy     "always"
set      beam::SECURITY2::enabling_policy     ""

lappend  beam::SECURITY2::disabling_comment   "length not checked"

set      beam::SECURITY2::disabled_files      "";        # enable all files
set      beam::SECURITY2::disabled_functions  "";        # enable all functions
set      beam::SECURITY2::disabled_macros     "";        # enable in all macros

#set     beam::SECURITY2::complaint_options   "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::SECURITY2::complaint_options   "show_values  show_calls_with_side_effect"


#
# Some CONCURRENCY checks uses this list of class names to decide
# which classes are shared and which ones aren't. This should be
# added to before the CONCURRENCY checks are enabled like so:
#
#   lappend beam::concurrency::shared_classes "MyClass" "AnotherClass" ...
#
# This is a pattern, so it can also contain wildcards and +/-.
#

set      beam::concurrency::shared_classes {}


#
# CONCURRENCY1: modifying global shared classes
#

set      beam::CONCURRENCY1::severity            ""

# User should set "beam::concurrency::shared_classes" before enabling this

#set     beam::CONCURRENCY1::enabling_policy     "always"
set      beam::CONCURRENCY1::enabling_policy     ""

lappend  beam::CONCURRENCY1::disabling_comment   "modifying shared"

set      beam::CONCURRENCY1::disabled_files      "";        # enable all files
set      beam::CONCURRENCY1::disabled_functions  "";        # enable all functions
set      beam::CONCURRENCY1::disabled_macros     "";        # enable in all macros

#set     beam::CONCURRENCY1::complaint_options   "show_source"; # print source line
set      beam::CONCURRENCY1::complaint_options   "show_values"; # print values of variables


#
# CONCURRENCY2: storing non-shared pointer into shared class field
#

set      beam::CONCURRENCY2::severity            ""

# User should set "beam::concurrency::shared_classes" before enabling this

#set     beam::CONCURRENCY2::enabling_policy     "always"
set      beam::CONCURRENCY2::enabling_policy     ""

lappend  beam::CONCURRENCY2::disabling_comment   "non-shared pointer"

set      beam::CONCURRENCY2::disabled_files      "";        # enable all files
set      beam::CONCURRENCY2::disabled_functions  "";        # enable all functions
set      beam::CONCURRENCY2::disabled_macros     "";        # enable in all macros

#set     beam::CONCURRENCY2::complaint_options   "show_source"; # print source line
set      beam::CONCURRENCY2::complaint_options   "show_values"; # print values of variables


#
# CONCURRENCY3: deadlock
#

set      beam::CONCURRENCY3::severity             ""

set      beam::CONCURRENCY3::enabling_policy      "always"
#set     beam::CONCURRENCY3::enabling_policy      ""

lappend  beam::CONCURRENCY3::disabling_comment    "deadlock"

set      beam::CONCURRENCY3::disabled_files       "";        # enable in all files
set      beam::CONCURRENCY3::disabled_functions   "";        # enable all functions
set      beam::CONCURRENCY3::disabled_macros      "";        # enable in all macros

#set     beam::CONCURRENCY3::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::CONCURRENCY3::complaint_options    "show_values  show_calls_with_side_effect"



#
# CONCURRENCY4: non-recursive lock acquired multiple times
#

set      beam::CONCURRENCY4::severity             ""

set      beam::CONCURRENCY4::enabling_policy      "always"
#set     beam::CONCURRENCY4::enabling_policy      ""

# In the absence of information about a lock, should it be assumed recursive?
# Setting of "yes" will yield fewer complaints
set      beam::CONCURRENCY4::unknown_lock_is_recursive  "yes"
#set     beam::CONCURRENCY4::unknown_lock_is_recursive  "no"

lappend  beam::CONCURRENCY4::disabling_comment    "acquiring held lock"

set      beam::CONCURRENCY4::disabled_files       "";        # enable in all files
set      beam::CONCURRENCY4::disabled_functions   "";        # enable all functions
set      beam::CONCURRENCY4::disabled_macros      "";        # enable in all macros

#set     beam::CONCURRENCY4::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::CONCURRENCY4::complaint_options    "show_values  show_calls_with_side_effect"



# CONCURRENCY5 unprotected operation

set      beam::CONCURRENCY5::severity             ""

#set     beam::CONCURRENCY5::enabling_policy     "always" 
#set     beam::CONCURRENCY5::enabling_policy     "assign" 
#set     beam::CONCURRENCY5::enabling_policy     "fetch" 
#set     beam::CONCURRENCY5::enabling_policy     "unknown_assign" 
#set     beam::CONCURRENCY5::enabling_policy     "unknown_fetch" 
set      beam::CONCURRENCY5::enabling_policy     ""

lappend  beam::CONCURRENCY5::disabling_comment    "no lock"

set      beam::CONCURRENCY5::disabled_files       "";        # enable in all files
set      beam::CONCURRENCY5::disabled_functions   "";        # enable in all functions
set      beam::CONCURRENCY5::disabled_macros      "";        # enable in all macros

#set     beam::CONCURRENCY5::complaint_options    "show_source"; # print source line
# print values of variables and calls with side-effect
set      beam::CONCURRENCY5::complaint_options    "show_values  show_calls_with_side_effect"


