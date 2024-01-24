# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2003-2010 IBM Corporation. All rights reserved.
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
#        Suggested parameter setting for use with Linux Kernel.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.
#
#        12/16/03  florian  Created
#

#
# Turn all checks off as default
#
source beam_never_parms.tcl

set beam::max_time_per_kloc_in_sec 600

#################################################################
# E R R O R S
#################################################################
set beam::ERROR1::enabling_policy "allow_partially_initialized_records"
set beam::ERROR2::enabling_policy "always"
set beam::ERROR3::enabling_policy "always"
set beam::ERROR4::enabling_policy "always"
set beam::ERROR5::enabling_policy "always"
set beam::ERROR6::enabling_policy "always"
set beam::ERROR7::enabling_policy "upper_bound"
set beam::ERROR8::enabling_policy "always"
set beam::ERROR9::enabling_policy "always"
set beam::ERROR10::enabling_policy "always"
set beam::ERROR11::enabling_policy "always"; # more specific options exist
# ERROR12 has been replaced with ERROR23(heap_memory)
set beam::ERROR13::enabling_policy "always"
set beam::ERROR14::enabling_policy "always"
set beam::ERROR15::enabling_policy "always"
set beam::ERROR16::enabling_policy "always" 
set beam::ERROR17::enabling_policy "always"
set beam::ERROR18::enabling_policy "always"
set beam::ERROR19::enabling_policy "always"
set beam::ERROR20::enabling_policy "always"
set beam::ERROR21::enabling_policy "always"
set beam::ERROR22::enabling_policy "always"
set beam::ERROR23(heap_memory)::enabling_policy "always"
set beam::ERROR23(file)::enabling_policy ""; # never
# ERROR24 has been replaced with ERROR24(file)
set beam::ERROR25::enabling_policy "always" 
set beam::ERROR26::enabling_policy "";  # never
# ERROR27 has been replaced with WARNING6
set beam::ERROR28::enabling_policy "";  # never
set beam::ERROR29::enabling_policy "always"
set beam::ERROR30::enabling_policy "";  # never
set beam::ERROR31::enabling_policy "";  # never
set beam::ERROR32::enabling_policy "";  # never
set beam::ERROR33::enabling_policy "always"
set beam::ERROR34::enabling_policy "always"
set beam::ERROR35::enabling_policy "always"
set beam::ERROR36::enabling_policy "always"
set beam::ERROR37::enabling_policy "always"
set beam::ERROR39::enabling_policy "always"

#################################################################
# M I S T A K E S
#################################################################
set beam::MISTAKE1::enabling_policy "always"

#
# Unreachable statement. Yields many not-so-important messages
#
set beam::MISTAKE2::enabling_policy ""; # never

#
# Operator precedence error in conditions. 
# Not likely, plus GCC mangles conditions often massively and
# that confuses the syntax recognizer that deals with this complaint.
# Better switch off.
#
set beam::MISTAKE3::enabling_policy "";  # never

#
# Missing breaks
#
set beam::MISTAKE4::enabling_policy "";  # never

#
# Constant expressions; fine-tuned to avoid messages that stem from 
# kernel configuration policies.
#
set beam::MISTAKE5::enabling_policy {condition_implied_from_local 
                                     condition_implied_from_non_local 
                                     condition_implied_from_declaration 
                                     condition_implied_from_right_of_assignment
                                     condition_inexplicable};

set beam::MISTAKE6::enabling_policy "";  # never
set beam::MISTAKE7::enabling_policy "always"
set beam::MISTAKE8::enabling_policy "always"
# MISTAKE9 does not exist
set beam::MISTAKE10::enabling_policy "always"

#
# Functions invoked repeatedly with identical arguments
#
set beam::MISTAKE11::enabling_policy "";   # never

set beam::MISTAKE12::enabling_policy "always"

#
# Never complain about duplicate enumerator values.
#
set beam::MISTAKE13::enabling_policy "";   # never

#
# Do not complain about implicitly declared functions
#
set beam::MISTAKE14::enabling_policy "";   # never

# MISTAKE15 does not exist

# Complains about if statements that neither have a then nor an else
# part. Is reported several times for macros that may be
# configured to be empty. Turn this off.
set beam::MISTAKE16::enabling_policy ""; # never

set beam::MISTAKE17::enabling_policy "always"

#
# Comparing pointers to strings instead of strings themselves
#
set beam::MISTAKE18::enabling_policy "no_overlap"

set beam::MISTAKE19::enabling_policy "always"

#
# Kernel uses no exception handlers
#
set beam::MISTAKE20::enabling_policy "";  # never

#
# Don't tell kernel developers to use strncpy instead of strcpy...
#
set beam::MISTAKE21::enabling_policy "";  # never

set beam::MISTAKE22::enabling_policy "";  # never
set beam::MISTAKE23::enabling_policy "";  # never

#
# Using boolean expressions in non-boolean contexts.
# Off by default - assume the kernel uses some crazy
# !!x trickery
#
set beam::MISTAKE24::enabling_policy ""; # never


#################################################################
# W A R N I N G S
#################################################################
#
# Do not complain about unused static variables with file scope.
# Due to different kernel configurations (#ifdef) almost all variables
# are used in some configuration. But we cannot know that.
# Also, there are a lot of static variables that are special in ways
# that we do not understand. Examples are  __setup_XYZ and __initcall_XYZ.
#
set beam::WARNING1::enabling_policy "";    # never

#
# Unused local variable. This is useful only for cleanups. 
#
set beam::WARNING2::enabling_policy "";    # never

#
# Do not complain about unused parameters
#
set beam::WARNING3::enabling_policy "";    # never

#
# Passing arguments that don't fit in registers
#
set beam::WARNING4::enabling_policy    "always - inlined"
set beam::WARNING4::threshold_in_bytes "8"

# Currently disabled. There are too may instances of if(a = foo() && b == a)
# for which we should not be complaining. Requires more work..
set beam::WARNING5::enabling_policy "";  # use to be "access"

set beam::WARNING6::enabling_policy "";  # never
set beam::WARNING7::enabling_policy "";  # never
set beam::WARNING8::enabling_policy "";  # never
set beam::WARNING9::enabling_policy "";  # never

set beam::WARNING10::enabling_policy "always"

set beam::WARNING11::enabling_policy "";  # never
set beam::WARNING12::enabling_policy "";  # never

#
# Do not complain about unused static functions.
# Again, due to multiple kernel configurations (#ifdef) most functions
# are used in some configuration. Others are used only once and
# could possibly be moved under the corresponding #ifdef. 
# 
set beam::WARNING13::enabling_policy "";  # never


set beam::WARNING14::enabling_policy "";  # never
set beam::WARNING15::enabling_policy "";  # never
set beam::WARNING16::enabling_policy "always"
set beam::WARNING17::enabling_policy "always"

# Kernel uses no classes, so turn this off
set beam::WARNING18::enabling_policy "";  # never

#
# Beginner's mistake. Do not check.
#
set beam::WARNING19::enabling_policy "";  # never

set beam::WARNING20::enabling_policy "";  # never
set beam::WARNING21::enabling_policy "";  # never


#################################################################
# P O R T A B I L I T Y
#################################################################

# Currently switched off
set beam::PORTABILITY1::disabled_files "*"
set beam::PORTABILITY2::disabled_files "*"
set beam::PORTABILITY3::disabled_files "*"
set beam::PORTABILITY4::disabled_files "*"
set beam::PORTABILITY5::disabled_files "*"


#################################################################
# S E C U R I T Y
#################################################################
set beam::SECURITY1::enabling_policy "";  # never
set beam::SECURITY2::enabling_policy "";  # never


#################################################################
# Function attributes
#################################################################

beam::function_attribute { noreturn } -names "panic";

