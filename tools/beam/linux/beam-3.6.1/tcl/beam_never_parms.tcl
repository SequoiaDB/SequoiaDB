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
#    DESCRIPTION:
#
#        Disable all reporting
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#	 07/19/02  florian  Include beam_default_parms.tcl such that this file
#                           can be used as the base for custom parameter 
#                           settings.
#        06/03/02  brand    Do not set user_innocent
#        05/16/02  brand    Created

source beam_default_parms.tcl

beam::foreach_complaint {
  set enabling_policy ""
}
