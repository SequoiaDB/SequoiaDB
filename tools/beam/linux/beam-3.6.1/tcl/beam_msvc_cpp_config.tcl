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
#        beam_configure
#
#    DESCRIPTION:
#
#        This is the C++ configuration file for Microsoft Visual Studio 8.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#

### This tells BEAM which pre-canned settings to load.
### BEAM comes with some function attributes and argument
### mappers for gcc, xlc, and vac. If unsure, set this to
### "default".

set beam::compiler::cpp::cc "msvc"
  
############################################################
# Section 1: Source language dialect
############################################################

set beam::compiler::cpp::msvc_mode 1400

set beam::compiler::cpp::language_friend_injection_enabled             {1}
set beam::compiler::cpp::language_trigraphs_allowed                    {0}

  
############################################################
# Section 2: Default include paths
############################################################

if { [::info exists ::env(INCLUDE)] } {
  set old_include_path {}
  if { [::info exists beam::compiler::cpp::system_include_path] } {
    set old_include_path $beam::compiler::cpp::system_include_path
  }
  set beam::compiler::cpp::system_include_path \
      [concat $old_include_path \
        [split $::env(INCLUDE) $::beam::pathsep]]
}

  
############################################################
# Section 3: Target machine configuration
############################################################

set beam::compiler::cpp::target_alignof_double                         {8}
set beam::compiler::cpp::target_alignof_float                          {4}
set beam::compiler::cpp::target_alignof_int                            {4}
set beam::compiler::cpp::target_alignof_long                           {4}
set beam::compiler::cpp::target_alignof_long_double                    {8}
set beam::compiler::cpp::target_alignof_long_long                      {8}
set beam::compiler::cpp::target_alignof_pointer                        {4}
set beam::compiler::cpp::target_alignof_short                          {2}
set beam::compiler::cpp::target_char_bit                               {8}
set beam::compiler::cpp::target_dbl_max_exp                            {1024}
set beam::compiler::cpp::target_dbl_min_exp                            {-1021}
set beam::compiler::cpp::target_enum_bit_fields_are_always_unsigned    {0}
set beam::compiler::cpp::target_flt_max_exp                            {128}
set beam::compiler::cpp::target_flt_min_exp                            {-125}
set beam::compiler::cpp::target_ldbl_max_exp                           {1024}
set beam::compiler::cpp::target_ldbl_min_exp                           {-1021}
set beam::compiler::cpp::target_little_endian                          {1}
set beam::compiler::cpp::target_plain_char_is_unsigned                 {0}
set beam::compiler::cpp::target_plain_int_bit_field_is_unsigned        {0}
set beam::compiler::cpp::target_ptrdiff_t_int_kind                     {__w64 int}
set beam::compiler::cpp::target_size_t_int_kind                        {__w64 unsigned int}
set beam::compiler::cpp::target_sizeof_double                          {8}
set beam::compiler::cpp::target_sizeof_float                           {4}
set beam::compiler::cpp::target_sizeof_int                             {4}
set beam::compiler::cpp::target_sizeof_long                            {4}
set beam::compiler::cpp::target_sizeof_long_double                     {8}
set beam::compiler::cpp::target_sizeof_long_long                       {8}
set beam::compiler::cpp::target_sizeof_pointer                         {4}
set beam::compiler::cpp::target_sizeof_ptrdiff_t                       {4}
set beam::compiler::cpp::target_sizeof_short                           {2}
set beam::compiler::cpp::target_sizeof_size_t                          {4}
set beam::compiler::cpp::target_sizeof_wchar_t                         {2}
set beam::compiler::cpp::target_wchar_t_is_unsigned                    {1}
set beam::compiler::cpp::target_string_literals_are_readonly           {1}
  

############################################################
# Section 4: Predefined macros
############################################################

set beam::compiler::cpp::predefined_macro(_CPPRTTI)                    {1}
set beam::compiler::cpp::predefined_macro(_INTEGRAL_MAX_BITS)          {64}
set beam::compiler::cpp::predefined_macro(_MSC_EXTENSIONS)             {1}
set beam::compiler::cpp::predefined_macro(_MSC_FULL_VER)               {140050727}
set beam::compiler::cpp::predefined_macro(_MSC_VER)                    {1400}
set beam::compiler::cpp::predefined_macro(_MT)                         {1}
set beam::compiler::cpp::predefined_macro(_M_IX86)                     {600}
set beam::compiler::cpp::predefined_macro(_M_IX86_FP)                  {0}
set beam::compiler::cpp::predefined_macro(_NATIVE_WCHAR_T_DEFINED)     {1}
set beam::compiler::cpp::predefined_macro(_WCHAR_T_DEFINED)            {1}
set beam::compiler::cpp::predefined_macro(_WIN32)                      {1}

set beam::compiler::cpp::standard_predefined_macros "* - __STDC__"
  
############################################################
# Section 5: Miscellaneous options
############################################################

set beam::compiler::cpp::function_name_is_string_literal(__FUNCTION__) 1
set beam::compiler::cpp::function_name_is_string_literal(__FUNCDNAME__) 1
set beam::compiler::cpp::function_name_is_string_literal(__FUNCSIG__) 1
