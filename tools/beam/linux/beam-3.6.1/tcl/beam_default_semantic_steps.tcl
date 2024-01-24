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
#        Daniel Brand
#
#    DESCRIPTION:
#
#        This script runs BEAM's built-in semantic checks. It requires that
#        beam_first_step was run.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#	 04/17/03  brand    Extracted from beam_default_steps.tcl so that site
#                           specific steps have a place to be inserted
#	 08/03/02  florian  Explicitly request ::info, so it works even within
#                           the beam namespace
#        06/24/02  brand    Added beam::info
#        05/16/02  brand    Copied from beam_default.tcl 
#                           Removed "source beam_settings"
#        05/02/02  brand    Copied from beam_compile.tcl
#                           Moved beam_define_proc here from beam_define_proc
#        05/01/02  brand    Added local_script
#        02/07/02  brand    Use bm::process_tlu instead of listing all the 
#                           commands explictly so that can modify our list of 
#                           commands
#        01/14/02  brand    Created

beam::semantic -tlu $tlu
