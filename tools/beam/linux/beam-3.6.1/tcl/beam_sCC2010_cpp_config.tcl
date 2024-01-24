# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2010 IBM Corporation. All rights reserved.
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
#        Configuration file for sCC emulation.
#
  
set beam::compiler::cpp::cc "sCC2010"

############################################################
# Section 1: Source language dialect
############################################################

set beam::compiler::cpp::language_dialect c++
set beam::compiler::cpp::c99_mode         0
set beam::compiler::cpp::strict_mode      0;    # certainly
set beam::compiler::cpp::gnu_mode         0;    # we'll see
set beam::compiler::cpp::msvc_mode        0;    # certainly not

set beam::compiler::cpp::language_friend_injection_enabled           0
set beam::compiler::cpp::language_use_nonstandard_for_init_scope     0
set beam::compiler::cpp::language_string_literals_are_const          0
set beam::compiler::cpp::language_allow_dollar_in_id_chars           0
set beam::compiler::cpp::language_allow_spaces_in_include_directive  0
set beam::compiler::cpp::language_restrict_keyword_enabled           1
set beam::compiler::cpp::language_trigraphs_allowed                  1
set beam::compiler::cpp::language_wchar_t_is_keyword                 1
set beam::compiler::cpp::language_bool_is_keyword                    1

set beam::compiler::cpp::language_custom_int_size_modifier {__int32}


############################################################
# Section 2: Default include paths
############################################################
set beam::compiler::cpp::system_include_path {}

  
############################################################
# Section 3: Target machine configuration
############################################################
set beam::compiler::cpp::target_char_bit      8

set beam::compiler::cpp::target_little_endian 0; # FIXME; clarify

#-----------------------------------------------------------
# Sizes of built-in types
#-----------------------------------------------------------
set beam::compiler::cpp::target_sizeof_short         2
set beam::compiler::cpp::target_sizeof_int           8
set beam::compiler::cpp::target_sizeof_long          8
set beam::compiler::cpp::target_sizeof_long_long     8
set beam::compiler::cpp::target_sizeof_float         4
set beam::compiler::cpp::target_sizeof_double        8
set beam::compiler::cpp::target_sizeof_long_double   8
set beam::compiler::cpp::target_sizeof_pointer       8
set beam::compiler::cpp::target_sizeof_custom_int    4

#-----------------------------------------------------------
# Alignment of built-in types
#-----------------------------------------------------------
set beam::compiler::cpp::target_alignof_short        2
set beam::compiler::cpp::target_alignof_int          8
set beam::compiler::cpp::target_alignof_long         8
set beam::compiler::cpp::target_alignof_long_long    8
set beam::compiler::cpp::target_alignof_float        4
set beam::compiler::cpp::target_alignof_double       8
set beam::compiler::cpp::target_alignof_long_double  8
set beam::compiler::cpp::target_alignof_pointer      8
set beam::compiler::cpp::target_alignof_custom_int   4

#-----------------------------------------------------------
# size_t
#
# This type cannot be used in declaration. But it exists (the
# result of sizeof has that type). It consumes 8 bytes and
# requires an 8-byte alignment. It is sufficient to specify the
# underlying type.
#-----------------------------------------------------------
set beam::compiler::cpp::target_size_t_int_kind     {unsigned long}

#-----------------------------------------------------------
# ptrdiff_t
#
# This type cannot be used in declaration. But it exists (the
# result of subtracting two pointers has that type). It consumes 8 bytes and
# requires an 8-byte alignment. It is sufficient to specify the
# underlying type.
#-----------------------------------------------------------
set beam::compiler::cpp::target_ptrdiff_t_int_kind  {long}

#-----------------------------------------------------------
# wchar_t
#
# Objects of this type consume 2 bytes and require a 2-byte alignment.
# The type is unsigned. It is sufficient to specify the underlying type.
#-----------------------------------------------------------
set beam::compiler::cpp::target_wchar_t_int_kind    {unsigned short}

# char16_t   not supported; using default settings
# char32_t   not supported; using default settings
# wint_t     not supported; using default settings

#-----------------------------------------------------------
# Issues of signedness
#-----------------------------------------------------------
set beam::compiler::cpp::target_plain_char_is_unsigned              1
set beam::compiler::cpp::target_plain_int_bit_field_is_unsigned     1;  # FIXME: clarify
set beam::compiler::cpp::target_enum_bit_fields_are_always_unsigned 0;  # FIXME: clarify

#-----------------------------------------------------------
# Characteristics of floating point data
#
# This is not exact, but we don't reason this amount of detail.
# So we simply pick a configuration that is consistent with the sizeof's
# and which EDG accepts. Copied from beam_sCC_cpp_config.tcl
#-----------------------------------------------------------
set beam::compiler::cpp::target_flt_max_exp    -37
set beam::compiler::cpp::target_flt_min_exp     38
set beam::compiler::cpp::target_dbl_max_exp   -307
set beam::compiler::cpp::target_dbl_min_exp    308
set beam::compiler::cpp::target_ldbl_max_exp  -307
set beam::compiler::cpp::target_ldbl_min_exp   308

#-----------------------------------------------------------
# Miscellaneous
#-----------------------------------------------------------
set beam::compiler::cpp::target_string_literals_are_readonly   1;  # FIXME: clarify (they are "const")

# More bit-field related stuff. 
# set beam::compiler::cpp::target_bit_field_container_size -1
# set beam::compiler::cpp::target_zero_width_bit_field_alignment -1
# set beam::compiler::cpp::target_zero_width_bit_field_affects_struct_alignment 0
# set beam::compiler::cpp::target_unnamed_bit_field_affects_struct_alignment 0


############################################################
# Section 4: Built-in macros
############################################################

# sCC defines the __STDC__ macro in a non-standard way
set beam::compiler::cpp::standard_predefined_macros "* - __STDC__"
set beam::compiler::cpp::predefined_macro(__STDC__)    0

set beam::compiler::cpp::predefined_macro(NOLIBERROR) {1}
set beam::compiler::cpp::predefined_macro(_CHAR_UNSIGNED) {1}
set beam::compiler::cpp::predefined_macro(_CPPUNWIND) {1}
set beam::compiler::cpp::predefined_macro(_EXT) {1}
set beam::compiler::cpp::predefined_macro(_LONG_LONG) {1}
set beam::compiler::cpp::predefined_macro(_WCHAR_T) {1}
set beam::compiler::cpp::predefined_macro(__AIXxSLIC__) {1}
set beam::compiler::cpp::predefined_macro(__ALIGN) {1}
set beam::compiler::cpp::predefined_macro(__BOOL__) {1}
set beam::compiler::cpp::predefined_macro(__C99__FUNC__) {1}
set beam::compiler::cpp::predefined_macro(__CHAR_UNSIGNED__) {1}
set beam::compiler::cpp::predefined_macro(__DIGRAPHS__) {1}
set beam::compiler::cpp::predefined_macro(__EXCEPTIONS) {1}
set beam::compiler::cpp::predefined_macro(__HHW_BIG_ENDIAN__) {1}
set beam::compiler::cpp::predefined_macro(__HHW_RS6000__) {1}
set beam::compiler::cpp::predefined_macro(__HOS_AIX__) {1}
set beam::compiler::cpp::predefined_macro(__IBMCPP__) {900}
set beam::compiler::cpp::predefined_macro(__IBM_LOCAL_LABEL) {1}
set beam::compiler::cpp::predefined_macro(__OBJECT_MODEL_COMPAT__) {1}
set beam::compiler::cpp::predefined_macro(__XPLINK_CALLBACK__) {1}


#-----------------------------------------------------------
# Alternate type names and spellings
#-----------------------------------------------------------
set beam::compiler::cpp::predefined_macro(_int8)      {signed char}
set beam::compiler::cpp::predefined_macro(__int8)     {signed char}
set beam::compiler::cpp::predefined_macro(_int16)     short
set beam::compiler::cpp::predefined_macro(__int16)    short
set beam::compiler::cpp::predefined_macro(_int32)     __int32
set beam::compiler::cpp::predefined_macro(_int64)     {  long}
set beam::compiler::cpp::predefined_macro(__int64)    {   long}
set beam::compiler::cpp::predefined_macro(__char16_t) {  char16_t}
set beam::compiler::cpp::predefined_macro(__char32_t) {  char32_t}
set beam::compiler::cpp::predefined_macro(__static_assert) {  static_assert}

# These are accepted in GNU mode
set beam::compiler::cpp::predefined_macro(__const)      {  const}
set beam::compiler::cpp::predefined_macro(__const__)    {  const  }
set beam::compiler::cpp::predefined_macro(__inline)     {  inline}
set beam::compiler::cpp::predefined_macro(__inline__)   {  inline  }
set beam::compiler::cpp::predefined_macro(__restrict)   {  restrict}
set beam::compiler::cpp::predefined_macro(__restrict__) {  restrict  }
set beam::compiler::cpp::predefined_macro(__signed)     {  signed}
set beam::compiler::cpp::predefined_macro(__signed__)   {  signed  }
set beam::compiler::cpp::predefined_macro(__volatile__) {  volatile  }
set beam::compiler::cpp::predefined_macro(__attribute(x))   {}
set beam::compiler::cpp::predefined_macro(__attribute__(x)) {}

#-----------------------------------------------------------
# Predefined functions; FIXME: clarify that they have no parameters
#-----------------------------------------------------------
set beam::compiler::cpp::predefined_macro(__StartVeryInfrequent())      {                       }
set beam::compiler::cpp::predefined_macro(__FinishVeryInfrequent())     {                        }
set beam::compiler::cpp::predefined_macro(__StartInfrequent())          {                   }
set beam::compiler::cpp::predefined_macro(__FinishInfrequent())         {                    }
set beam::compiler::cpp::predefined_macro(__StartSomewhatInfrequent())  {                           }
set beam::compiler::cpp::predefined_macro(__FinishSomewhatInfrequent()) {                            }

#-----------------------------------------------------------
# Miscellaneous junk
#-----------------------------------------------------------
set beam::compiler::cpp::predefined_macro(__atomic)      {        }
set beam::compiler::cpp::predefined_macro(_cdecl)        {      }
set beam::compiler::cpp::predefined_macro(__cdecl)       {       }
set beam::compiler::cpp::predefined_macro(__export)      {        }
set beam::compiler::cpp::predefined_macro(__far)         {     }
set beam::compiler::cpp::predefined_macro(__far16)       {       }
set beam::compiler::cpp::predefined_macro(__far32)       {       }
set beam::compiler::cpp::predefined_macro(__fastcall)    {          }
set beam::compiler::cpp::predefined_macro(_fastcall)     {         }
set beam::compiler::cpp::predefined_macro(__fence)       {       }
set beam::compiler::cpp::predefined_macro(__import)      {        }
set beam::compiler::cpp::predefined_macro(__optlink)     {         }
set beam::compiler::cpp::predefined_macro(_pascal)       {       }
set beam::compiler::cpp::predefined_macro(__pascal)      {        }
set beam::compiler::cpp::predefined_macro(_stdcall)      {        }
set beam::compiler::cpp::predefined_macro(__stdcall)     {         }
set beam::compiler::cpp::predefined_macro(__seg16)       {       }
set beam::compiler::cpp::predefined_macro(__system)      {        }

set beam::compiler::cpp::predefined_macro(_Cdecl)        {      }
set beam::compiler::cpp::predefined_macro(_Export)       {       }
set beam::compiler::cpp::predefined_macro(_Import)       {       }
set beam::compiler::cpp::predefined_macro(_Far16)        {      }
set beam::compiler::cpp::predefined_macro(_Far32)        {      }
set beam::compiler::cpp::predefined_macro(_Fastcall)     {         }
set beam::compiler::cpp::predefined_macro(_Optlink)      {        }
set beam::compiler::cpp::predefined_macro(_Packed)       {       }
set beam::compiler::cpp::predefined_macro(_Pascal)       {       }
set beam::compiler::cpp::predefined_macro(_Seg16)        {      }
set beam::compiler::cpp::predefined_macro(_System)       {       }

set beam::compiler::cpp::predefined_macro(_declspec(...))  {}
set beam::compiler::cpp::predefined_macro(__declspec(...)) {}
set beam::compiler::cpp::predefined_macro(__align(x)) {}
set beam::compiler::cpp::predefined_macro(__offsetof(type,field))          {(size_t)&(((type *)0)->field)}
set beam::compiler::cpp::predefined_macro(__builtin_offsetof(type,field))  {(size_t)&(((type *)0)->field)}


############################################################
# More bloody hacks
############################################################

# The combination of AssertConst and __offsetof is lethal. It's
# used pervasively and causes trouble because __offsetof does not
# evaluate to a constant in BEAM. Since AssertConst is a compile
# time assert it does not provide interesting info to BEAM and
# so we will just bypass all this. This is fragile but so be it.
# 
# Step #1:  make sure AssertConst.H is not read.
set beam::compiler::cpp::predefined_macro(_ASSERTCONST_H) 1

# Step #2: redefine AssertConst
set beam::compiler::cpp::predefined_macro(AssertConst(x)) {(void)0}

# sCC allows access to private class members in sizeof context.
# Here we define "private" to mean "public" which is not a general cure
# because "private" is default in a class... But it helps.
# FIXME: this thould be reverted before releasing but it is still being
# discussed.
set beam::compiler::cpp::predefined_macro(private) {public}

############################################################
# Section 5: Miscellaneous options
############################################################

#-----------------------------------------------------------
# Linkage strings (from interface/Linkage.h)
#-----------------------------------------------------------

set beam::compiler::cpp::extern(c)             "C++"
set beam::compiler::cpp::extern(c++)           "C++"
set beam::compiler::cpp::extern(builtin)       "C++"
set beam::compiler::cpp::extern(cdecl)         "C++"
set beam::compiler::cpp::extern(optlink)       "C++"
set beam::compiler::cpp::extern(system)        "C++"
set beam::compiler::cpp::extern(java)          "C++"
set beam::compiler::cpp::extern(fortran)       "C++"
set beam::compiler::cpp::extern(C\ nowiden)    "C++"
set beam::compiler::cpp::extern(RPG)           "C++"
set beam::compiler::cpp::extern(COBOL)         "C++"
set beam::compiler::cpp::extern(CL)            "C++"
set beam::compiler::cpp::extern(OS\ nowiden)   "C++"
set beam::compiler::cpp::extern(OS\ by_value)  "C++"
set beam::compiler::cpp::extern(OS)            "C++"
set beam::compiler::cpp::extern(ILE)           "C++"
set beam::compiler::cpp::extern(VREF)          "C++"
set beam::compiler::cpp::extern(ILE\ nowiden)  "C++"
set beam::compiler::cpp::extern(VREF\ nowiden) "C++"
set beam::compiler::cpp::extern(PLI)           "C++"
set beam::compiler::cpp::extern(XPLINK)        "C++"
set beam::compiler::cpp::extern(OS_UPSTACK)    "C++"
set beam::compiler::cpp::extern(OS_DOWNSTACK)  "C++"
set beam::compiler::cpp::extern(OS_NOSTACK)    "C++"
set beam::compiler::cpp::extern(OS31_NOSTACK)  "C++"
set beam::compiler::cpp::extern(REFERENCE)     "C++"
set beam::compiler::cpp::extern(OS64_NOSTACK)  "C++"
set beam::compiler::cpp::extern(BII)           "C++"
set beam::compiler::cpp::extern(PLMP)          "C++"

#-----------------------------------------------------------
# __func__ and friends
#-----------------------------------------------------------

set beam::compiler::cpp::function_name_is_string_literal(__func__)      0
set beam::compiler::cpp::function_name_is_string_literal(__FUNCTION__)  1

# __PRETTY_FUNCTION__ is not defined
# __FUNCDNAME__       is not defined
