# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2004-2010 IBM Corporation. All rights reserved.
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
#        This script is for users interested in security checking. 
#        It should be sourced in their own configuration Tcl file
#        after a mandatory script, e.g., beam_default_parms.tcl,
#        like so
#            source beam_default_parms.tcl
#            source beam_security.tcl
#        
#        The user is encouraged to crate his own local version,
#        in particular by adding advisory attributes for functions
#        considered unsafe.
#        Think of this file only as a template

#############################################################################
# security_mode increases conservatism:
#   buffers copied are assumed to contain as long a string as possible.
#############################################################################

set beam::security_mode "1"


#############################################################################
#   a function writing into a buffer is assumed write longest possible string
#############################################################################

set beam::evidence(buffer)  "arbitrary"

#############################################################################
# Turn on SECURITY checks
#############################################################################

set beam::SECURITY1::enabling_policy  "always"
set beam::SECURITY2::enabling_policy  "always"


#############################################################################
# Issue MISTAKE21 for functions with a security advisory 
#############################################################################

set beam::MISTAKE21::enabling_policy  "security*"

