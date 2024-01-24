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
#        This is the configuration file for the z/os VAC C compiler on USS.
#
#        Note: When running beam_configure --c c89 ... the following
#              environment variable must be defined:
#
#              _C89_ELINES=1 
#
#              Otherwise, #line directives will not be written. This causes
#              beam_configure to not find any system headers which has a
#              detrimental effect on detecting predefined macros.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See cvs log for more recent modifications.
#
#	 12/21/05  florian  Created
#

set beam::compiler::c::cc "zosvac"

############################################################
# Section 1: Source language settings
############################################################
#
# These are from beam_configure
#
set beam::compiler::c::language_allow_dollar_in_id_chars             {0}
set beam::compiler::c::language_allow_spaces_in_include_directive    {1}
set beam::compiler::c::language_end_of_line_comments_allowed         {0}
set beam::compiler::c::language_allow_nonstandard_anonymous_unions   {1}

#
# Terry had them set like this:
#
#set beam::compiler::c::language_string_literals_are_const          {1}
#set beam::compiler::c::language_allow_dollar_in_id_chars           {1}
#set beam::compiler::c::language_end_of_line_comments_allowed       {1}
#set beam::compiler::c::language_allow_spaces_in_include_directive  {0}

set beam::compiler::c::language_trigraphs_allowed                   {1}


############################################################
# Section 2: Default include paths
############################################################
lappend beam::compiler::c::system_include_path {/usr/include}


############################################################
# Section 3: Target machine configuration
############################################################
#
# These are from beam_configure. I added "long long" because that is
# only recognized with -Wc,longlong and setting its size/alignment
# won't hurt
#
set beam::compiler::c::target_alignof_double                         {8}
set beam::compiler::c::target_alignof_float                          {4}
set beam::compiler::c::target_alignof_int                            {4}
set beam::compiler::c::target_alignof_long                           {4}
set beam::compiler::c::target_alignof_long_long                      {8}
set beam::compiler::c::target_alignof_long_double                    {8}
set beam::compiler::c::target_alignof_pointer                        {4}
set beam::compiler::c::target_alignof_short                          {2}
set beam::compiler::c::target_char_bit                               {8}
set beam::compiler::c::target_dbl_max_exp                            {63}
set beam::compiler::c::target_dbl_min_exp                            {-64}
set beam::compiler::c::target_flt_max_exp                            {63}
set beam::compiler::c::target_flt_min_exp                            {-64}
set beam::compiler::c::target_ldbl_max_exp                           {63}
set beam::compiler::c::target_ldbl_min_exp                           {-64}
set beam::compiler::c::target_little_endian                          {0}
set beam::compiler::c::target_plain_char_is_unsigned                 {1}
set beam::compiler::c::target_plain_int_bit_field_is_unsigned        {1}
set beam::compiler::c::target_size_t_int_kind                        {unsigned int}
set beam::compiler::c::target_sizeof_double                          {8}
set beam::compiler::c::target_sizeof_float                           {4}
set beam::compiler::c::target_sizeof_int                             {4}
set beam::compiler::c::target_sizeof_long                            {4}
set beam::compiler::c::target_sizeof_long_long                       {8}
set beam::compiler::c::target_sizeof_long_double                     {16}
set beam::compiler::c::target_sizeof_pointer                         {4}
set beam::compiler::c::target_sizeof_short                           {2}
set beam::compiler::c::target_sizeof_size_t                          {4}
set beam::compiler::c::target_sizeof_wchar_t                         {2}
set beam::compiler::c::target_wchar_t_int_kind                       {unsigned short}
set beam::compiler::c::target_wchar_t_is_unsigned                    {1}
set beam::compiler::c::target_string_literals_are_readonly           {1}

#
# From Terry
#
#set beam::compiler::c::target_dbl_max_exp                            {1024}
#set beam::compiler::c::target_dbl_min_exp                            { -1021 }
#set beam::compiler::c::target_flt_max_exp                            {128}
#set beam::compiler::c::target_flt_min_exp                            { -125 }
#set beam::compiler::c::target_ldbl_max_exp                           {1024}
#set beam::compiler::c::target_ldbl_min_exp                           { -1021 }
#set beam::compiler::c::target_size_t_int_kind                        {unsigned long}
#set beam::compiler::c::target_sizeof_long_double                     {8}

############################################################
# Section 4: Predefined macros
############################################################
# Terry had this very differently. We start with a clean plate
# as per beam_configure / predefined-macros.c and then add gently.

#
# These were detected by beam_configure
#
set beam::compiler::c::predefined_macro(__ANSI__)                    {1}
set beam::compiler::c::predefined_macro(__ARCH__)                    {5}
set beam::compiler::c::predefined_macro(__CHARSET_LIB)               {0}
set beam::compiler::c::predefined_macro(__COMPILER_VER__)            {0x41070000}
set beam::compiler::c::predefined_macro(__DIGRAPHS__)                {1}
set beam::compiler::c::predefined_macro(__FENCE)                     {1}
set beam::compiler::c::predefined_macro(__HHW_BIG_ENDIAN__)          {1}
set beam::compiler::c::predefined_macro(__IBMC__)                    {41070}
set beam::compiler::c::predefined_macro(__LONGDOUBLE128)             {1}
set beam::compiler::c::predefined_macro(__LONGNAME__)                {1}
set beam::compiler::c::predefined_macro(__MATH__)                    {1}
set beam::compiler::c::predefined_macro(__MVS__)                     {1}
set beam::compiler::c::predefined_macro(__OPTIMIZE__)                {0}
set beam::compiler::c::predefined_macro(__PTR32)                     {1}
set beam::compiler::c::predefined_macro(__STDC_VERSION__)            {199409L}
set beam::compiler::c::predefined_macro(__TARGET_LIB__)              {0x41070000}
set beam::compiler::c::predefined_macro(__THW_BIG_ENDIAN__)          {1}
set beam::compiler::c::predefined_macro(__WSIZEOF_OPER__)            {1}
set beam::compiler::c::predefined_macro(_CHAR_UNSIGNED)              {1}
set beam::compiler::c::predefined_macro(_ILP32)                      {1}
set beam::compiler::c::predefined_macro(_OPEN_DEFAULT)               {1}
set beam::compiler::c::predefined_macro(errno)                       {(*__errno())}

#
# In addition, these were detected by predefined-macros.c
#
set beam::compiler::c::predefined_macro(__370__)             {1}
set beam::compiler::c::predefined_macro(__CHAR_UNSIGNED__)   {1}
set beam::compiler::c::predefined_macro(__CODESET__)         {"IBM-1047"}
set beam::compiler::c::predefined_macro(__ENUM_OPT)          {1}
set beam::compiler::c::predefined_macro(__HHW_370__)         {1}
set beam::compiler::c::predefined_macro(__HOS_MVS__)         {1}
set beam::compiler::c::predefined_macro(__LIBREL__)          {0x41070000}
set beam::compiler::c::predefined_macro(__LOCALE__)          {"POSIX"}
set beam::compiler::c::predefined_macro(__SIZE_TYPE__)       {unsigned int}
set beam::compiler::c::predefined_macro(__STR__)             {1}
set beam::compiler::c::predefined_macro(__THW_370__)         {1}
set beam::compiler::c::predefined_macro(__TOS_MVS__)         {1}
set beam::compiler::c::predefined_macro(__TUNE__)            {5}
set beam::compiler::c::predefined_macro(__XPLINK_CALLBACK__) {1}

#
# These following are language extensions that BEAM does not need to
# understand. Therefore, we #define the keywords away by replacing them
# with a sequence of spaces of the same length as the keyword in order to
# preserve column information. We do that whereever possible.
#
# The list was taken from z/OS C/C++ Language Reference, Fourth Edition,
# September 2004 ( cbclr140.pdf ).
#
# FIXME: typeof  same as in GCC
# FIXME: __typeof__  is the same as 'typeof'
# FIXME: __alignof__
set beam::compiler::c::predefined_macro(__ptr32)        {       }
set beam::compiler::c::predefined_macro(__ptr64)        {       }
set beam::compiler::c::predefined_macro(__attribute__(x))  { }
set beam::compiler::c::predefined_macro(__const__)      {         }
set beam::compiler::c::predefined_macro(__extension__)  {             }
set beam::compiler::c::predefined_macro(__inline__)     {          }
set beam::compiler::c::predefined_macro(__label__)      {         }
set beam::compiler::c::predefined_macro(__restrict__)   {            }
set beam::compiler::c::predefined_macro(__signed__)     {          }
set beam::compiler::c::predefined_macro(__volatile__)   {            }

set beam::compiler::c::predefined_macro(_Packed)        {       }
set beam::compiler::c::predefined_macro(__packed)       {        }

# __offsetof appears to be an undocumented language extension
set beam::compiler::c::predefined_macro(__offsetof(type,field)) {(size_t)&(((type *)0)->field)}

############################################################
# Section 5: Miscellaneous options
############################################################

set beam::compiler::c::extern(builtin)  "C"
set beam::compiler::c::extern(OS)       "C"
set beam::compiler::c::extern(os)       "C"
set beam::compiler::c::extern(PLI)      "C"

set beam::compiler::c::function_name_is_string_literal(__FUNCTION__) 1

