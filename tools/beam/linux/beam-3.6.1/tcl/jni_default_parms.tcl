# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2006-2010 IBM Corporation. All rights reserved.
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
#        Goh Kondoh
#
#    DESCRIPTION:
#
#        BEAM configuration for checking JNI code.
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        11/12/07  gkondo   Created
#

source beam_default_c_and_cpp_parms

#
# JNI1: assigning a local reference to a global variable
#

set      jni::JNI1::severity            ""
set      jni::JNI1::enabling_policy     "always"

lappend  jni::JNI1::disabling_comment   "JNI invalid local reference"

set      jni::JNI1::disabled_files      "";        # enable all files

# enable only top-level JNI functions, whose name starts with "Java_"
# don't issue a complaint for a normal function in case that sucn the
# assigned global variable is cleared later.

set      jni::JNI1::disabled_functions  "* - Java_*";
#set     jni::JNI1::disabled_functions   "";        # enable all functions

set      jni::JNI1::disabled_macros     "";        # enable in all macros

#set     jni::JNI1::complaint_options   "show_source"; # print source line
# print values of variables and calls with side-effects
set      jni::JNI1::complaint_options   "show_values  show_calls_with_side_effect"

if { $::beam::lang == "cpp" } {
  set jni::JNI1::jnienv JNIEnv_
} else {
  set jni::JNI1::jnienv JNINativeInterface_
}


#
# JNI2: assigning a local reference to a global variable
#

set      jni::JNI2::severity            ""

# disabled by default.  classpath should be set before enabling.
# set    jni::JNI2::enabling_policy     "always"
set      jni::JNI2::enabling_policy     ""

set      jni::JNI2::classpath           ""

lappend  jni::JNI2::disabling_comment   "JNI type error"

set      jni::JNI2::disabled_files      "";        # enable all files

set      jni::JNI2::disabled_functions  "";        # enable all functions

set      jni::JNI2::disabled_macros     "";        # enable in all macros

#set     jni::JNI2::complaint_options   "show_source"; # print source line
# print values of variables and calls with side-effects
set      jni::JNI1::complaint_options   "show_values  show_calls_with_side_effect"

if { $::beam::lang == "cpp" } {
  set jni::JNI2::jnienv JNIEnv_
} else {
  set jni::JNI2::jnienv JNINativeInterface_
}
