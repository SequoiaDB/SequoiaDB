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
#        Nate Daly
#
#    DESCRIPTION:
#
#        Attributes for XLC's builtin functions.
#
#        This is based on the documentation for V9.0 of the XLC
#        compiler which can be found here:
#
#        http://publib.boulder.ibm.com/infocenter/compbgpl/v9v111/index.jsp
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        06/03/08  natedaly Created
#

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # Fixed-point built-ins
  #----------------------------------------------------------------------------

  beam::function_attribute "const, no_other_side_effects" -signatures \
      "_labs" "__llabs" "__cntlz4" "__cntlz8" "__cnttz4" "__cnttz8" \
      "__mulhd" "__mulhdu" "__mulhw" "__mulhwu" "__popcnt4" "__popcnt8" \
      "__popcntb" "__poppar4" "__poppar8" "__rdlam" "__rldimi" "__rlwimi" \
      "__rlwnm" "__rotatel4" "__rotatel8"
  

  # NOTE: "int __assert1(int, int, int)" asserts when the first 2 arguments
  # are the same, not matter what they are it seems. I have been unable to
  # determine what the third argument is for. The return value is always the
  # same and appears to be completely unrelated to the arguments. We cannot
  # express the condition of the first 2 arguments being the same, so here
  # we just say it asserts if the first 2 arguments are 0, which is the
  # only case that shows up in the system headers. This may be totally 
  # correct because is not documented and was arrived at through testing.
  
  beam::function_attribute {
    assert if ( index = 1,
                type = input,
                test_type = equal,
                test_value = 0 )
          and ( index = 2,
                type = input,
                test_type = equal,
                test_value = 0 )
  } -signatures "__assert1"

  # NOTE: "void __assert2(int)" does absolutely nothing. Tried it with
  # every value from INT_MIN to INT_MAX and it doesn't assert.

  beam::function_attribute {
    buffer ( buffer_index = 1,
             type = read,
             units = elements,
             size = 1 ),
    const,
    no_other_side_effects
  } -signatures "__load2r" "__load4r"

  # NOTE: __tdw and __tw may assert but the conditions are too complicated
  # to express.

  beam::function_attribute {
    assert if ( index = 1,
                type = input,
                test_type = not_equal,
                test_value = 0 )
  } -signatures "__trap" "__trapd"

  beam::function_attribute {
    buffer ( buffer_index = 2,
             type = write,
             units = elements,
             size = 1 ),
    no_other_side_effects
  } -signatures "__store2r" "__store4r"

  #----------------------------------------------------------------------------
  # Binary floating-point built-ins
  #----------------------------------------------------------------------------

  beam::function_attribute "const, no_other_side_effects" -signatures \
      "__fabss" "__fnabs" "__fnabss" "__cmplx" "__cmplxf" "__cmplxl" \
      "__fcfid" "__fctid" "__fctidz" "__fctiw" "__fctiwz" \
      "__ibm2gccldbl" "__ibm2gccldbl_cmplx" "__fmadd" "__fmadds" \
      "__fmsub" "__fmsubs" "__fnmadd" "__fnmadds" "__fnmsub" "__fnmsubs" \
      "__fre" "__fres" "__frim" "__frims" "__frin" "__frins" \
      "__frip" "__frips" "__friz" "__frizs" "__fsel" "__fsels" \
      "__frsqrte" "__frsqrtes" "__fsqrt" "__fsqrts" "__swdiv" "__swdivs" \
      "__swdiv_nochk" "__swdivs_nochk"

  beam::function_attribute "$side_effect_on_environment,
                            property ( index = 1,
                                       property_type = requires,
                                       type = input,
                                       range_min = 0,
                                       range_max = 31 ),
                            no_other_side_effects" -signatures \
      "__mtfsb0" "__mtfsb1"

  beam::function_attribute "$side_effect_on_environment,
                            property ( index = 1,
                                       property_type = requires,
                                       type = input,
                                       range_min = 0,
                                       range_max = 255 ),
                            no_other_side_effects" -signatures \
      "__mtfsf"

  beam::function_attribute "$side_effect_on_environment,
                            property ( index = 1,
                                       property_type = requires,
                                       type = input,
                                       range_min = 0,
                                       range_max = 7 ),
                            property ( index = 2,
                                       property_type = requires,
                                       type = input,
                                       range_min = 0,
                                       range_max = 15 ),
                            no_other_side_effects" -signatures \
      "__mtfsfi"

  beam::function_attribute "pure, no_other_side_effects" -signatures \
      "__readflm"

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "__setflm"

  beam::function_attribute "$side_effect_on_environment,
                            property ( index = 1,
                                       property_type = requires,
                                       type = input,
                                       range_min = 0,
                                       range_max = 3 ),
                            no_other_side_effects" -signatures \
      "__setrnd"

  beam::function_attribute {
    buffer ( buffer_index = 1,
             type = write,
             units = elements,
             size = 1 ),
    no_other_side_effects
  } -signatures "__stfiw"

  #----------------------------------------------------------------------------
  # Synchronization and atomic built-ins
  #----------------------------------------------------------------------------

  ## SKIPPED

  #----------------------------------------------------------------------------
  # Cache-related built-ins
  #----------------------------------------------------------------------------

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "__dcbf" "__dcbfl" "__dcbst" "__dcbt" "__dcbtst" "__dcbz" \
      "__prefetch_by_load" "__prefetch_by_stream"

  ## Skipped the protected stream functions.

  #----------------------------------------------------------------------------
  # Block-related built-ins
  #----------------------------------------------------------------------------

  beam::function_attribute {
    buffer ( buffer_index = 1,
             type = read,
             size_index = 3 ),
    buffer ( buffer_index = 2,
             type = write,
             size_index = 3 ),
    no_other_side_effects
  } -signatures "__bcopy"

  #----------------------------------------------------------------------------
  # Miscellaneous built-ins
  #----------------------------------------------------------------------------

  # NOTE: __alignx, __builtin_expect, and __fence are just used as 
  # hints to the compiler about various potential optimizations and
  # have no attributes or effects that we care about.

  beam::function_attribute "pure, no_other_side_effects" -signatures \
      "__mftb" "__mftbu" "__mfspr" "__builtin_frame_address" \
      "__builtin_return_address"

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "__mfmsr" "__mtmsr" "__mtspr"

  beam::function_attribute $alloca_like -signatures "__alloca"
  
  #----------------------------------------------------------------------------
  # Built-in functions for parallel processing
  #----------------------------------------------------------------------------

  ## SKIPPED

}
