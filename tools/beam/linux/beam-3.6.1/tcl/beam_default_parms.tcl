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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        This script sources the default parameters for the given language.
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.
#
#        06/04/05  florian  Created

switch -exact -- $beam::lang {
  "c"     { source c_default_parms.tcl   }
  "cpp"   { source cpp_default_parms.tcl }
  "pl8"   { source pl8_default_parms.tcl }
  "asm"   { source asm_default_parms.tcl  }
  "java"  { source java_default_parms.tcl }
  "mc"    { source mc_default_parms.tcl  }
  "unset" { ;# Nothing to load. This happens for ipa_check, for example. }

  default { beam::internal_error "Unknown language $beam::lang"  }
}
