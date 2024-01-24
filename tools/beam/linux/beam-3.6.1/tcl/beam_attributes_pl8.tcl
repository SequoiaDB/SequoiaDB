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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        This Tcl file defines default attributes for PL8 library functions.
#        Note, that we only need to model those functions that appear in
#        d_lib_d.h and "libpl8".
#        PL8 "builtin" functions (pl8-builtin.c) are expanded at compile 
#        time to expressions or calls to library functions. So we do not
#        need function attributes for those.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        12/17/03  florian  Created
#

# FIXME: allocation / free... Are these used ?
#    "$EMPTY32",
#    "$EMPTY64",
#    "$BFREE32",
#    "$BFREE64",
#    "$BALCT32",
#    "$BALCT64",

namespace eval beam::attribute {

  set const_funcs {
      $NDXC
      $NDXB32
      $NDXB64
      $BOOL32
      $BOOL64
      $CEILFS
      $CEILFL
      $CEILFD
      $FLOORFS
      $FLOORFL
      $FLOORFD
      $XPID
      $XPIF
      $XPFL
      $XPFS
  }

  foreach func $const_funcs {
    beam::function_attribute "$attr(const_function)" -names $func
  }

# Functions that convert integers / floats / bitstrings to a character string
  set to_string_funcs {
      $IHCH
      $IFCH
      $IFCSF
      $IDCH
      $IDCSF
      $FLCH
      $FLCSF
      $FDCH
      $BTCH
      $BT32CH
      $BT64CH
  }

  foreach func $to_string_funcs {
    beam::function_attribute "$attr(const_function)" -names $func
  }

# Functions that convert a character string to integer / float / bitstring
# The first parameter is a pointer to the character. The second parameter
# is the length of the string. 
  set from_string_funcs {
      $CHIF
      $CHID
      $CHFL
      $CHFD
      $CHBT
      $CHBT32
      $CHBT64
  }

  foreach func $from_string_funcs {
    beam::function_attribute "buffer ( buffer_index = 1,
                                       type = read,
                                       size_index = 2),
                              pure,
                              no_other_side_effects" -names $func
  }

#
# Miscellaneous
#
  beam::function_attribute $abort_like -names { $SIGNAL }

  beam::function_attribute "buffer ( buffer_index = 1, 
                                     type = read,
                                     size_index = 2 ),
                            pure,
                            no_other_side_effects" -names \
      { $WTRMF } { $CHAR }

  beam::function_attribute "buffer ( buffer_index = 1, 
                                     type = read,
                                     size_index = 2 ),
                            buffer ( buffer_index = 3, 
                                     type = read,
                                     size_index = 4 ),
                            property ( index = return, 
                                       property_type = provides, 
                                       type = output, 
                                       test_type = greater_than_or_equal, 
                                       test_value = 0),
                            property ( index = return, 
                                       property_type = provides, 
                                       type = output, 
                                       test_type = less_than_or_equal, 
                                       test_value = 1),
                            pure,
                            no_other_side_effects" -names \
      { $CMPEQ } { $STRLT } { $STRGT }

  beam::function_attribute "buffer ( buffer_index = 1, 
                                     type = read,
                                     size_index = 2 ),
                            buffer ( buffer_index = 3, 
                                     type = read,
                                     size_index = 4 ),
                            pure,
                            no_other_side_effects" -names \
      { $VERFY }

# Internally generated functions
  beam::function_attribute "pure,
                            no_other_side_effects" -names "beam_get_bits"

  beam::function_attribute "property ( index = 1,
                                       property_type = requires,
                                       type = input,
                                       test_type = not_equal,
                                       test_value = 0 )
                            " -names "beam_set_bits"


}
