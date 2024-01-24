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
#        BEAM default parameter setting for checking Java code.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.

# For now, use C and C++ parms...
source beam_default_c_and_cpp_parms.tcl

# Report no runtime exception in code that handles RuntimeException 
set beam::dont_report_caught_runtime_exceptions "java.lang.RuntimeException" 


# variable used but never assigned
set  beam::ERROR1::enabling_policy  ""

# freeing deallocated memory
# Does not apply to Java
set  beam::ERROR3::enabling_policy  ""

# accessing deallocated location
# Does not apply to Java, and because of limitations in our attributes,
# this would complain about double-close of files, which is ok...
set  beam::ERROR4::enabling_policy  ""

# passing pointer to deallocated memory
# Does not apply to Java, and because of limitations in our attributes,
# this would complain about double-close of files, which is ok...
set  beam::ERROR8::enabling_policy  ""

# No dangling pointers in Java
set  beam::ERROR11(java_heap_memory)::enabling_policy  ""

# No memory leaks in Java
set  beam::ERROR23(java_heap_memory)::enabling_policy  ""

# No need to free memory in Java
set  beam::ERROR44(java_heap_memory)::enabling_policy  ""


#
# ERROR23(stream): failure to close a stream
#

lappend  beam::ERROR23(stream)::disabling_comment        "stream leak"
lappend  beam::ERROR23(stream)::disabling_comment        "resource leak"
set      beam::ERROR23(stream)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23

#
# ERROR23(socket): failure to close a socket
#

lappend  beam::ERROR23(socket)::disabling_comment        "socket leak"
lappend  beam::ERROR23(socket)::disabling_comment        "resource leak"
set      beam::ERROR23(socket)::unknown_functions_effect "saves_parameter"

# All other parameters default to their setting in beam::ERROR23

# C++ specific
set  beam::ERROR30::enabling_policy  ""
set  beam::ERROR31::enabling_policy  ""
set  beam::ERROR32::enabling_policy  ""

set  beam::ERROR33::severity  "This is less severe in Java than C/C++."

# undefined side-effect
set  beam::ERROR35::enabling_policy  ""

set  beam::MISTAKE10::enabling_policy  "always"

# comparing objects
#set  beam::MISTAKE18::enabling_policy "always"
#set "beam::MISTAKE18::compare_function(java.lang.String)" "String::equals"

set  "beam::MISTAKE18::compare_function(java.lang.Boolean)"   "Boolean::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Byte)"      "Byte::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Character)" "Character::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Short)"     "Short::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Integer)"   "Integer::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Long)"      "Long::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Float)"     "Float::equals"
set  "beam::MISTAKE18::compare_function(java.lang.Double)"    "Double::equals"


# set beam::MISTAKE21::enabling_policy "discouraged_method_call"

# Java has no such confusion with "bool" and integral types
set beam::MISTAKE24::enabling_policy ""


# Ignore java.lang.String.indexOf and java.lang.String.lastIndexOf
# because their 'int' version is for characters.
#
# Ignore java.io.* because they all have 'write()' that takes
# 'int' for characters.
#
# Ignore javax.swing.text.html.parser.DTD.defEntity for the
# same reason.
  
set beam::WARNING24::functions_by_qualified_name \
      [concat "*" \
              " - java.io.*" \
              " - java.lang.String.indexOf" \
              " - java.lang.String.lastIndexOf" \
              " - javax.swing.text.html.parser.DTD.defEntity"]

lappend beam::WARNING24::complaint_options show_source
