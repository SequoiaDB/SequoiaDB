# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2002-2010 IBM Corporation. All rights reserved.
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
#    WARNING:
#
#        This file should not be used 'as is' as a BEAM parameter setting.
#        Doing so will result in many complaints that, most likely, you are
#        not interested in seeing. Please read 
#
#        https://w3.eda.ibm.com/beam/faq.html#Q15
#        
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
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


source beam_default_parms.tcl


# Reset parameters beam_default_parms.tcl to maximize complaints

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

set beam::clean_enum      "";     # all enums are unspecified
set beam::dirty_enum      "";     # all enums are unspecified
set beam::extended_enum   "";     # all enums are unspecified

set beam::avalanche_prevention  "";     # allow duplicates

# Allow errors dependent on macros
set beam::evidence(macro)  "inadmissible"


#
# Turn on all the checks
#

beam::foreach_complaint {
  set   enabling_policy                 "always"
  set   disabled_for_user_written       "no"
  set   disabled_files                  ""
  set   disabled_macros                 ""
  set   unknown_functions_effect        "";  # does not impede leak complaints
}


# Even if user wants all complaints, today he does not want 
# complaints about Java heap memory
#
set  beam::ERROR44(java_heap_memory)::enabling_policy  ""
set  beam::ERROR23(java_heap_memory)::enabling_policy  ""
set  beam::ERROR11(java_heap_memory)::enabling_policy  ""


# Turn on all options
beam::foreach_complaint {
  beam::enable_option "show_source"
  beam::enable_option "show_values"
  beam::enable_option "show_calls_with_side_effect"
}

#
# Other complaint-specific settings and adjustments
#

# Complain about struct { ...; int array[1]; }
set beam::ERROR7::extensible_struct   "no"

# Do not complain about stack limit all the time
set beam::ERROR38::limit   "2147483647"

# Missing break in a case
set beam::MISTAKE4::disabling_comment_policy "L2-1"

# Boolean used incorrectly
set beam::MISTAKE24::operators { < > <= >= <? >? >> << & | ~ ^ / + - * % [] }

# Complain whenever a return value is not being used
set beam::WARNING9::functions "*"

# Complaint about all instances of f(...,&x,..., &x,...)
set beam::WARNING16::functions "*"


# Set agressive attributes as to what should be checked 

namespace eval beam::attribute {
  beam::function_attribute $can_return_null_like -signatures "fopen"
  beam::function_attribute $can_return_null_like -signatures "strchr" "strstr"
  beam::function_attribute $can_return_null_like -signatures "strpbrk" "strrpbrk" "strrchr"
  beam::function_attribute $can_return_null_like -signatures "strtok"
}
