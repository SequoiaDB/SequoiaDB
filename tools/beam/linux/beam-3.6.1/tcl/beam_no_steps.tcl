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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        This script will only run the compiler frontend and not invoke
#        BEAM's bug finding machinery. This is handy when you're experimenting
#        with emulating a new compiler and have lots of syntax errors. In that
#        case you're not interested (initially) in finding any bugs but
#        increase parser coverage.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent revisions
#
#        03/18/05  florian    Created

source beam_first_step
source beam_last_step
