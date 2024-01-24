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
#        BEAM configuration for checking PL8 code.
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        12/12/03  florian  Created


source beam_default_c_and_cpp_parms.tcl

beam::foreach_complaint {
  set enabling_policy ""
  beam::disable_option "show_source"
  beam::disable_option "show_calls_with_side_effect"
}

set beam::disabled_files "";        # enable all files
set beam::disabled_macros "";       # enable all macros

# Reset parameters beam_secret_parms.tcl to maximize complaints

set beam::disabling_comment_capitalization   "ignore"
set beam::allocation_may_return_null         "yes"

set beam::may_free_null   "no"
set beam::may_read_null   "no"
set beam::may_write_null  "no"
set beam::may_add_null    "no"
set beam::may_call_null   "no"

set beam::clean_enum     "";     # all enums are unspecified
set beam::dirty_enum     "";     # all enums are unspecified
set beam::extended_enum  "";     # all enums are unspecified

set beam::function_name_style "qualified"

#######################################################################
# ERRORs (If not explicitly enabled here, the ERROR is turned off)
#######################################################################

set beam::ERROR1::enabling_policy "allow_partially_initialized_records"
set beam::ERROR2::enabling_policy "always"
set beam::ERROR3::enabling_policy "always"
set beam::ERROR4::enabling_policy "always"
set beam::ERROR5::enabling_policy "always"
# ERROR6 is C++ specific
#
# For ERROR7 we do not want to see complaints about using a negative index.
#
set beam::ERROR7::enabling_policy "upper_bound";
set beam::ERROR8::enabling_policy "always"
set beam::ERROR9::enabling_policy "always"
# ERROR10 is C++ specific
set beam::ERROR11(heap_memory)::enabling_policy "parameter  return"
# ERROR12 has been replaced with ERROR23(heap_memory)
# ERROR13 is C/C++ specific (printf stuff)
# ERROR14 is C/C++ specific (printf stuff)
# ERROR15 is C/C++ specific (printf stuff)
# ERROR16 is C/C++ specific (printf stuff)
# ERROR17 is C/C++ specific (scanf stuff)
set beam::ERROR18::enabling_policy "always"
set beam::ERROR19::enabling_policy "always"
set beam::ERROR20::enabling_policy "always"
# ERROR21 is C/C++ specific
set beam::ERROR22::enabling_policy "always"
#
# There is no dynamic memory allocation in PL8. Therefore, looking
# for memory leaks is a waste of time. 
#
set beam::ERROR23(heap_memory)::enabling_policy ""; # never
set beam::ERROR23(file)::enabling_policy ""; # never
# ERROR24 has been replaced with ERROR23(file)
# ERROR25 is off for now; requires experiment
# ERROR26 is C/C++ specific
# ERROR27 has been replaced with WARNING6
# ERROR28 is site specific (EDA, Fishkill)
# ERROR29 is C/C++ specific (printf stuff)
# ERROR30 is C++ specific (exception handling)
# ERROR31 is C++ specific (exception handling)
# ERROR32 is C++ specific (exception handling)
# ERROR33 is C/C++ specific
# ERROR34 (detects when $SIGNAL is called)
set beam::ERROR34::enabled_for_compiler_generated  "yes"
# ERROR35 is C/C++ specific
set beam::ERROR36::enabling_policy "always"
# ERROR37 is for variadic function 

#######################################################################
# MISTAKEs (If not explicitly enabled here the MISTAKE is turned off).
#######################################################################

#
# MISTAKE1 is switched off because it will be generated for assignments
# of compound objects (struct assignment). The PL8 frontend will generate
# a sequence of statements (which assign the components of the compound),
# concatenate them using the ',' operator and then finally concatenate
# the LHS of the assignment. BEAM will generate MISTAKE1 for that last
# expression which has no effect. The frontend should omit concatenating
# the LHS if the value of the assignment is not needed. Unless that's
# done we switch MISTAKE1 off.
#
set beam::MISTAKE2::enabling_policy "always"
# MISTAKE3 is C/C++ specific
# MISTAKE4 is C/C++ specific
# MISTAKE5 is very noisy; requires experiment
# MISTAKE6 is C/C++ specific
set beam::MISTAKE7::enabling_policy "always"
# MISTAKE8 is C/C++ specific
# MISTAKE9 does not exist
# MISTAKE10 is C/C++ specific
# MISTAKE11 is noisy; requires experiment
# MISTAKE12 is C/C++ specific
# MISTAKE13 is C/C++ specific
# MISTAKE14 is C/C++ specific
# MISTAKE16 is C/C++ specific
set beam::MISTAKE17::enabling_policy "always"
# MISTAKE18 is C/C++ specific
# MISTAKE19 is C/C++ specific
# MISTAKE20 is C++ specific
set beam::MISTAKE21::enabling_policy "always"
# MISTAKE22 is C++ specific
set beam::MISTAKE23::enabling_policy "always"
# MISTAKE24 is C/C++ specific


#######################################################################
# WARNINGs (If not explicitly enabled here the WARNING is turned off).
#######################################################################

# WARNING1: requires experiment
# WARNING2: requires experiment
# WARNING3: requires experiment
# WARNING4: requires experiment
# WARNING5: is C/C++ specific
#
# WARNING6 warns about out-of-range values.  We suppress it because
# the PL8 frontend already warns about those and we get BEAM
# complaints related to the implementation of the "exists" built in
# function. That functions generates a cast of the value -1 to a reference
# type which BEAM complains about. So there is no added value in having
# WARNING6 switched on.
#
# WARNING7: We don't want to see warnings about assignments from, say, 
# 64bit variables to 32bit variables etc.
#
# WARNING8: is off because the frontend generates many unused labels.
# WARNING9: is very noisy
# WARNING10: is C/C++ specific
# WARNING11: cannot distinguish between generated and written gotos
# WARNING12: the frontend generates loads of these casts
# WARNING13: requires experiment
# WARNING14: the frontend generates loads of these comparisons
# WARNING15: is C/C++ specific
#
# WARNING16 is a check that was specifically being asked for by PL8
# developers.
set beam::WARNING16::enabling_policy "always"
# WARNING17: requires experiment
# WARNING18: does not exist
# WARNING19: is C/C++ specific

#######################################################################
# PORTABILITY (Remain turned off)
#######################################################################
