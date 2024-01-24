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

set beam::compiler::cpp::cc "cxlc400"

############################################################
# Section 1: Source language dialect
############################################################

### Miscellaneous language options

set beam::compiler::cpp::language_friend_injection_enabled            1
set beam::compiler::cpp::language_use_nonstandard_for_init_scope      0
# set beam::compiler::cpp::language_string_literals_are_const      1
# set beam::compiler::cpp::language_allow_dollar_in_id_chars       1

############################################################
# Section 2: Default include paths
############################################################

set beam::compiler::cpp::system_include_path {}

  
############################################################
# Section 3: Target machine configuration
############################################################

set beam::compiler::cpp::target_char_bit 8

set beam::compiler::cpp::target_little_endian 0

#
# Sizes of built-in types
#
set beam::compiler::cpp::target_sizeof_short         2
# set beam::compiler::cpp::target_sizeof___int32 4
set beam::compiler::cpp::target_sizeof_int           4
set beam::compiler::cpp::target_sizeof_long          4
set beam::compiler::cpp::target_sizeof_long_long     8
set beam::compiler::cpp::target_sizeof_float         4
set beam::compiler::cpp::target_sizeof_double        8
set beam::compiler::cpp::target_sizeof_long_double   8
set beam::compiler::cpp::target_sizeof_pointer      16
set beam::compiler::cpp::target_sizeof_size_t        4
set beam::compiler::cpp::target_sizeof_wchar_t       2

#
# Alignment of built-in types
#
set beam::compiler::cpp::target_alignof_short       2
# set beam::compiler::cpp::target_alignof___int32 4
set beam::compiler::cpp::target_alignof_int         4
set beam::compiler::cpp::target_alignof_long        4
set beam::compiler::cpp::target_alignof_long_long   8
set beam::compiler::cpp::target_alignof_float       4
set beam::compiler::cpp::target_alignof_double      8
set beam::compiler::cpp::target_alignof_long_double 8
set beam::compiler::cpp::target_alignof_pointer    16

#
# Issues of signedness
#
set beam::compiler::cpp::target_enum_bit_fields_are_always_unsigned 1
set beam::compiler::cpp::target_plain_char_is_unsigned              1
set beam::compiler::cpp::target_plain_int_bit_field_is_unsigned     1
set beam::compiler::cpp::target_wchar_t_is_unsigned                 1

#
# Characteristics of floating point data
# This is not exact, but we don't reason this amount of detail.
# So we simply pick a configuration that is consistent with the sizeof's
# and which EDG accepts. 
#
set beam::compiler::cpp::target_flt_max_exp    -37
set beam::compiler::cpp::target_flt_min_exp     38
set beam::compiler::cpp::target_dbl_max_exp   -307
set beam::compiler::cpp::target_dbl_min_exp    308
set beam::compiler::cpp::target_ldbl_max_exp  -307
set beam::compiler::cpp::target_ldbl_min_exp   308

#
# Miscellaneous type stuff
#
set beam::compiler::cpp::target_size_t_int_kind   {unsigned int}
set beam::compiler::cpp::target_wchar_t_int_kind  {short int}

# Other
set beam::compiler::cpp::target_string_literals_are_readonly  0

############################################################
# Section 4: Built-in macros
############################################################
set beam::compiler::cpp::standard_predefined_macros "* - __STDC__ __STDC_VERSION__"

# Compiler built-ins
#
set beam::compiler::cpp::predefined_macro(__AIXxxlC400__)     1
set beam::compiler::cpp::predefined_macro(__ALIGN)            1
set beam::compiler::cpp::predefined_macro(_ARCH_COM)          1
# _ARCH_PPC is handled in mapper
set beam::compiler::cpp::predefined_macro(__BOOL__)           1
set beam::compiler::cpp::predefined_macro(__C99_UCN__)        1
set beam::compiler::cpp::predefined_macro(__C99__FUNC__)      1
# __DIGRAPHS__ is handled in the mapper
set beam::compiler::cpp::predefined_macro(_EXT)               1
# __EXTENDED__ is handled in the mapper
set beam::compiler::cpp::predefined_macro(__HHW_BIG_ENDIAN__) 1
set beam::compiler::cpp::predefined_macro(__HHW_RS6000__)     1
set beam::compiler::cpp::predefined_macro(__HOS_AIX__)        1
set beam::compiler::cpp::predefined_macro(__IBMCPP__)         601
set beam::compiler::cpp::predefined_macro(__IBM_LOCAL_LABEL)  1
# _LONG_LONG is handled in the mapper
set beam::compiler::cpp::predefined_macro(_LONG_LONG)         1
set beam::compiler::cpp::predefined_macro(__MULTI__)          1
set beam::compiler::cpp::predefined_macro(__OS400__)          1
set beam::compiler::cpp::predefined_macro(__OS400_TGTVRM__)   540
set beam::compiler::cpp::predefined_macro(__RTTI_DYNAMIC_CAST__) 1
set beam::compiler::cpp::predefined_macro(__STDC__)           0
set beam::compiler::cpp::predefined_macro(__TEMPINC__)        1
# __TERASPACE__ is handled in the mapper
set beam::compiler::cpp::predefined_macro(__THW_AS400__)      1
set beam::compiler::cpp::predefined_macro(__TIMESTAMP__)      "some-time"
set beam::compiler::cpp::predefined_macro(__TOS_OS400__)      1
set beam::compiler::cpp::predefined_macro(__cplusplus_nomacro__)        1
set beam::compiler::cpp::predefined_macro(__wchar_t)          1

# Hacks to work around language extensions
set beam::compiler::cpp::predefined_macro(__cdecl)    {}
set beam::compiler::cpp::predefined_macro(_Packed)    {}
set beam::compiler::cpp::predefined_macro(__ptr64)    {}
set beam::compiler::cpp::predefined_macro(__ptr128)   {}
set beam::compiler::cpp::predefined_macro(__int8)     char
set beam::compiler::cpp::predefined_macro(__int16)    short
set beam::compiler::cpp::predefined_macro(__int32)    int
set beam::compiler::cpp::predefined_macro(__int64)    {long long}
set beam::compiler::cpp::predefined_macro(__align(x)) {}
set beam::compiler::cpp::predefined_macro(__offsetof(type,field)) {((size_t)&(((type*)0)->field))}

############################################################
# Section 5: Miscellaneous options
############################################################
set beam::compiler::cpp::extern(builtin)    "C++"
set beam::compiler::cpp::extern(C)          "C++"
set beam::compiler::cpp::extern(C\ nowiden) "C++"
set beam::compiler::cpp::extern(OS)         "C++"

# Odd, but true.
set beam::compiler::cpp::function_name_is_string_literal(__FUNCTION__) 1
set beam::compiler::cpp::function_name_is_string_literal(__func__)     0
