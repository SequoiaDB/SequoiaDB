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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        Attributes for non C-standard functions.
#        
#        FIXME: This is highly incomplete. The full list of functions
#        FIXME: can be found at this URL:
#        FIXME: http://msdn2.microsoft.com/en-us/library/634ca0c2(VS.80).aspx
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#
#        11/15/06  florian  Created
#

namespace eval beam::attribute {


  beam::function_attribute  $free_like    -names "_aligned_free"
  beam::function_attribute  $malloc_like  -names "_aligned_malloc"
  beam::function_attribute  $realloc_like -names "_aligned_realloc"
  beam::function_attribute  $alloca_like  -names "_alloca"
  beam::function_attribute  $calloc_like  -names "_calloc_dbg"
  beam::function_attribute  $free_like    -names "_free_dbg"
  beam::function_attribute  $malloc_like  -names "_malloc_dbg"
  beam::function_attribute  $realloc_like -names "_realloc_dbg"

  # We can't model the size exactly yet
  beam::function_attribute $unknown_allocator_like -signatures "_strdup" "_wcsdup" "_mbsdup" "_strdup_dbg" "_wcsdup_dbg"
  beam::function_attribute {
      property (index = return,             # Which parameter has he property
                property_type = provides,                  
                type = output,                  
                property_name = "memory allocation source",
                property_value = "from malloc") 
  } -signatures "_strdup" "_wcsdup" "_mbsdup" "_strdup_dbg" "_wcsdup_dbg"
  
 
  beam::function_attribute $fopen_like -names "_wfopen"
  beam::function_attribute {
      property (index = return,
                property_type = provides,
                type = output,
                property_name = "file source",
                property_value = "from fopen")
  } -signatures "_wfopen"


# Functions with side-effects on some hidden variables
  beam::function_attribute $side_effect_on_environment -names "_wfopen" "_wfreopen"

#######################################################################
# The functions below are somewhat secret and are used internally
#######################################################################

# This one comes from the expansion of assert(expr)
  beam::function_attribute $abort_like -names "_wassert"

# This one comes from the expansion of _ASSERT and _ASSERTE (with -D_DEBUG)
  beam::function_attribute $abort_like -names "_CrtDbgReportW"
}
