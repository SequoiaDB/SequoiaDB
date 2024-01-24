# -*-Tcl-*-
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
# AUTHOR
#   
#        Florian Krohm
#
# DESCRIPTION
# 
#        BEAM initialization script. This is called when BEAM binaries
#        are run that need the full BEAM Tcl layer. This loads up the
#        main helper scripts and sets a few initial variables.
#
# MODIFICATIONS
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------
#        See ChangeLog for recent modifications.

#
# Load our helper routines early. Some of thses
# are loaded so that users always have access (like beam_util).
#
source beam_util
source beam_define_proc
source beam_compiler_proc
source beam_attributes

#
# Set ::beam::target_os   (assume native by default).
#
set ::beam::target_os_junk [string tolower $tcl_platform(os)]
regsub -all {[^a-zA-Z0-9_]} $::beam::target_os_junk _ ::beam::target_os
