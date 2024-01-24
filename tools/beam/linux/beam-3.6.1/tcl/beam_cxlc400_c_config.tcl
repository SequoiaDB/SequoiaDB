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
#        Configuration file for cxlc400 emulation.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See cvs log for more recent modifications.
#
#	 04/04/05  florian  Created
#

set beam::compiler::c::cc "cxlc400"

############################################################
# Section 1: Source language dialect
############################################################

### Miscellaneous language options

# set beam::compiler::c::language_string_literals_are_const      1
# set beam::compiler::c::language_allow_dollar_in_id_chars       1
# set beam::compiler::c::language_end_of_line_comments_allowed   1

############################################################
# Section 2: Default include paths
############################################################

set beam::compiler::c::system_include_path {}

  
############################################################
# Section 3: Target machine configuration
############################################################

set beam::compiler::c::target_char_bit 8

set beam::compiler::c::target_little_endian 0

#
# Sizes of built-in types
#
set beam::compiler::c::target_sizeof_short         2
# set beam::compiler::c::target_sizeof___int32 4
set beam::compiler::c::target_sizeof_int           4
set beam::compiler::c::target_sizeof_long          4
set beam::compiler::c::target_sizeof_long_long     8
set beam::compiler::c::target_sizeof_float         4
set beam::compiler::c::target_sizeof_double        8
set beam::compiler::c::target_sizeof_long_double   8
set beam::compiler::c::target_sizeof_pointer      16
set beam::compiler::c::target_sizeof_size_t        4
set beam::compiler::c::target_sizeof_wchar_t       2

#
# Alignment of built-in types
#
set beam::compiler::c::target_alignof_short       2
# set beam::compiler::c::target_alignof___int32 4
set beam::compiler::c::target_alignof_int         4
set beam::compiler::c::target_alignof_long        4
set beam::compiler::c::target_alignof_long_long   8
set beam::compiler::c::target_alignof_float       4
set beam::compiler::c::target_alignof_double      8
set beam::compiler::c::target_alignof_long_double 8
set beam::compiler::c::target_alignof_pointer    16

#
# Issues of signedness
#
set beam::compiler::c::target_enum_bit_fields_are_always_unsigned 1
set beam::compiler::c::target_plain_char_is_unsigned              1
set beam::compiler::c::target_plain_int_bit_field_is_unsigned     1
set beam::compiler::c::target_wchar_t_is_unsigned                 1

#
# Characteristics of floating point data
# This is not exact, but we don't reason this amount of detail.
# So we simply pick a configuration that is consistent with the sizeof's
# and which EDG accepts. 
#
set beam::compiler::c::target_flt_max_exp    -37
set beam::compiler::c::target_flt_min_exp     38
set beam::compiler::c::target_dbl_max_exp   -307
set beam::compiler::c::target_dbl_min_exp    308
set beam::compiler::c::target_ldbl_max_exp  -307
set beam::compiler::c::target_ldbl_min_exp   308

#
# Miscellaneous type stuff
#
set beam::compiler::c::target_size_t_int_kind   {unsigned int}
set beam::compiler::c::target_wchar_t_int_kind  {short int}

# Other
set beam::compiler::c::target_string_literals_are_readonly  0

############################################################
# Section 4: Built-in macros
############################################################

set beam::compiler::c::standard_predefined_macros "* - __STDC__ __STDC_VERSION__"

set beam::compiler::c::predefined_macro(__ALIGN)            1
# _CHAR_UNSIGNED is handled in the mapper
# __EXTENDED__ is handled in the mapper
set beam::compiler::c::predefined_macro(__HHW_BIG_ENDIAN__) 1
set beam::compiler::c::predefined_macro(__HHW_RS6000__)     1
set beam::compiler::c::predefined_macro(__HOS_AIX__)        1
set beam::compiler::c::predefined_macro(__IBMC__)           {}
set beam::compiler::c::predefined_macro(__IBM_GCC_INLINE__) 1
set beam::compiler::c::predefined_macro(__IBM_REGISTER_VARS) 1
set beam::compiler::c::predefined_macro(__ILEC400__)        1
set beam::compiler::c::predefined_macro(__ILEC400_TGTVRM__) 540
set beam::compiler::c::predefined_macro(_ILP32)             1
set beam::compiler::c::predefined_macro(_LONG_LONG)         1
set beam::compiler::c::predefined_macro(__MATH__)           1
set beam::compiler::c::predefined_macro(__OS400__)          1
set beam::compiler::c::predefined_macro(__OS400_TGTVRM__)   540
set beam::compiler::c::predefined_macro(__STDC_VERSION__)   0
set beam::compiler::c::predefined_macro(__STR__)            1
set beam::compiler::c::predefined_macro(__THW_AS400__)      1
set beam::compiler::c::predefined_macro(__THW_BIG_ENDIAN__) 1
set beam::compiler::c::predefined_macro(__TIMESTAMP__)      "some-time"
set beam::compiler::c::predefined_macro(__TOS_OS400__)      1
set beam::compiler::c::predefined_macro(__XLC121__)         1
set beam::compiler::c::predefined_macro(__XLC13__)          1
set beam::compiler::c::predefined_macro(__xlC__)            {}
set beam::compiler::c::predefined_macro(__xlc__)            "some-version"

# Hacks to work around language extensions
set beam::compiler::c::predefined_macro(__cdecl)    {}
set beam::compiler::c::predefined_macro(_Packed)    {}
set beam::compiler::c::predefined_macro(__ptr64)    {}
set beam::compiler::c::predefined_macro(__ptr128)   {}
set beam::compiler::c::predefined_macro(__int8)     char
set beam::compiler::c::predefined_macro(__int16)    short
set beam::compiler::c::predefined_macro(__int32)    int
set beam::compiler::c::predefined_macro(__int64)    {long long}
set beam::compiler::c::predefined_macro(__align(x)) {}
set beam::compiler::c::predefined_macro(__offsetof(type,field)) {((size_t)&(((type*)0)->field))}

############################################################
# Section 5: Miscellaneous options
############################################################
  
# set beam::compiler::c::extern(builtin) "C++"

# Only __FUNCTION__ is defined and replaced with a string literal
set beam::compiler::c::function_name_is_string_literal(__FUNCTION__) 1
