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
#        Frank Wallingford
#
#    DESCRIPTION:
#
#        Attributes for non C-standard functions that are found in GLIBC.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        06/03/05  florian  Carved out of beam_attributes_libc.tcl
#

namespace eval beam::attribute {

  beam::function_attribute $abort_like -names "__assert"
  beam::function_attribute $abort_like -names "__assert_perror_fail"
  beam::function_attribute $abort_like -names "__assert_fail"

  beam::function_attribute { no_stats } -signatures \
      "gnu_dev_makedev" "gnu_dev_major" "gnu_dev_minor"
}
