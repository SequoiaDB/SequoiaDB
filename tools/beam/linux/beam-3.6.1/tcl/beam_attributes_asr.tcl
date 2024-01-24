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
#        This Tcl file defines attributes for ASR built-in functions.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        07/27/06  florian  Created
#

namespace eval beam::attribute {

# Associate attributes with functions

  beam::function_attribute "$abort_like" -signatures "__asr_abort"

  beam::function_attribute "$exit_like" -signatures "__asr_exit"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2 ),
                            no_other_side_effects" -signatures "__asr_init"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type = read,
                                     size_index = 2 ),
                            no_other_side_effects" -signatures "__asr_read"
}
