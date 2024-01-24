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
#        Frank Wallingford and Florian Krohm
#
#    DESCRIPTION:
#
#        This is the BEAM configuration file for IBM's VisualAge C compiler
#        Version 6.x
#
#
#    NOTE:
#
#        This file is no longer maintained and its use is not recommended.
#        Instead you should create your own compiler configuration file by 
#        running beam_configure and load that.
#        See https://w3.eda.ibm.com/beam/beam_configure.html and
#        https://w3.eda.ibm.com/beam/compiler_configuration.html
#
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#
#	 04/27/05  fwalling  Created
#

set beam::compiler::cpp::cc "vac"

############################################################
# Section 1: Source language dialect
############################################################
set beam::compiler::cpp::language_allow_dollar_in_id_chars             {0}
set beam::compiler::cpp::language_restrict_keyword_enabled             {1}
set beam::compiler::cpp::language_trigraphs_allowed                    {1}

  
############################################################
# Section 2: Default include paths
#
# Include directories for C++ headers depend on the compiler's
# install point. Since we do not know the install point, you need 
# to set the include directory in another Tcl file.
# See https://w3.eda.ibm.com/beam/compiler_configuration.html
############################################################
lappend beam::compiler::cpp::system_include_path {/usr/include}

  
############################################################
# Section 3: Target machine configuration
############################################################
set beam::compiler::cpp::target_alignof_double                         {4}
set beam::compiler::cpp::target_alignof_float                          {4}
set beam::compiler::cpp::target_alignof_int                            {4}
set beam::compiler::cpp::target_alignof_long                           {4}
set beam::compiler::cpp::target_alignof_long_double                    {4}
set beam::compiler::cpp::target_alignof_long_long                      {8}
set beam::compiler::cpp::target_alignof_pointer                        {4}
set beam::compiler::cpp::target_alignof_short                          {2}
set beam::compiler::cpp::target_char_bit                               {8}
set beam::compiler::cpp::target_dbl_max_exp                            {1024}
set beam::compiler::cpp::target_dbl_min_exp                            {-1021}
set beam::compiler::cpp::target_enum_bit_fields_are_always_unsigned    {0}
set beam::compiler::cpp::target_flt_max_exp                            {128}
set beam::compiler::cpp::target_flt_min_exp                            {-125}
set beam::compiler::cpp::target_gpp_dependent_name_lookup              {1}
set beam::compiler::cpp::target_ldbl_max_exp                           {1024}
set beam::compiler::cpp::target_ldbl_min_exp                           {-1021}
set beam::compiler::cpp::target_little_endian                          {0}
set beam::compiler::cpp::target_plain_char_is_unsigned                 {1}
set beam::compiler::cpp::target_plain_int_bit_field_is_unsigned        {1}
set beam::compiler::cpp::target_ptrdiff_t_int_kind                     {long}
set beam::compiler::cpp::target_size_t_int_kind                        {unsigned long}
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
set beam::compiler::cpp::standard_predefined_macros "* - __STDC__ __STDC_VERSION__ __STDC_HOSTED__"

set beam::compiler::cpp::predefined_macro(_WCHAR_T)                    {1}
set beam::compiler::cpp::predefined_macro(__ALIGN)                     {1}
set beam::compiler::cpp::predefined_macro(__BOOL__)                    {1}
set beam::compiler::cpp::predefined_macro(__FENCE)                     {1}
set beam::compiler::cpp::predefined_macro(__HHW_BIG_ENDIAN__)          {1}
set beam::compiler::cpp::predefined_macro(__HHW_RS6000__)              {1}
set beam::compiler::cpp::predefined_macro(__HOS_AIX__)                 {1}
set beam::compiler::cpp::predefined_macro(__IBMCPP__)                  {600}
set beam::compiler::cpp::predefined_macro(__MATH__)                    {1}
set beam::compiler::cpp::predefined_macro(__TEMPINC__)                 {1}
set beam::compiler::cpp::predefined_macro(__THW_BIG_ENDIAN__)          {1}
set beam::compiler::cpp::predefined_macro(__THW_RS6000__)              {1}
set beam::compiler::cpp::predefined_macro(__TOS_AIX__)                 {1}
set beam::compiler::cpp::predefined_macro(__XLC121__)                  {1}
set beam::compiler::cpp::predefined_macro(__XLC13__)                   {1}
set beam::compiler::cpp::predefined_macro(__xlC__)                     {0x0600}
set beam::compiler::cpp::predefined_macro(__cplusplus)                 {199711L}

set beam::compiler::cpp::predefined_macro(__offsetof(_t,_m))           {__INTADDR__(&(((_t *)0)->_m))}
set beam::compiler::cpp::predefined_macro(__vastart(_a))               {((va_list)(&(_a)))}
set beam::compiler::cpp::predefined_macro(__cdecl)                     {       }
set beam::compiler::cpp::predefined_macro(__export)                    {        }
set beam::compiler::cpp::predefined_macro(__far16)                     {       }
set beam::compiler::cpp::predefined_macro(__far32)                     {       }
set beam::compiler::cpp::predefined_macro(__fastcall)                  {          }
set beam::compiler::cpp::predefined_macro(__optlink)                   {         }
set beam::compiler::cpp::predefined_macro(__pascal)                    {        }
set beam::compiler::cpp::predefined_macro(__stdcall)                   {         }
set beam::compiler::cpp::predefined_macro(__system)                    {        }
set beam::compiler::cpp::predefined_macro(_Cdecl)                      {      }
set beam::compiler::cpp::predefined_macro(_Export)                     {       }
set beam::compiler::cpp::predefined_macro(_Far16)                      {      }
set beam::compiler::cpp::predefined_macro(_Far32)                      {      }
set beam::compiler::cpp::predefined_macro(_Fastcall)                   {         }
set beam::compiler::cpp::predefined_macro(_Optlink)                    {        }
set beam::compiler::cpp::predefined_macro(_Packed)                     {       }
set beam::compiler::cpp::predefined_macro(_Pascal)                     {       }
set beam::compiler::cpp::predefined_macro(_System)                     {       }


  
############################################################
# Section 5: Miscellaneous options
############################################################
set beam::compiler::cpp::extern(builtin) "C"

set beam::compiler::cpp::function_name_is_string_literal(__func__)     0
set beam::compiler::cpp::function_name_is_string_literal(__FUNCTION__) 1
