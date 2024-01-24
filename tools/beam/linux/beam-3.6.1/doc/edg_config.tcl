#
# Sample configuration file for EDG.
#

set beam::compiler::cpp::cc default

############################################################
# Section 1: Source language dialect
############################################################
set beam::compiler::cpp::language_dialect c++

############################################################
# Section 2: Default include paths
############################################################
set beam::compiler::cpp::system_include_path {/usr/include}

############################################################
# Section 3: Target machine configuration
############################################################
set beam::compiler::cpp::target_char_bit 8

set beam::compiler::cpp::target_little_endian 0

#
# Sizes of built-in types
#
set beam::compiler::cpp::target_sizeof_short         2
set beam::compiler::cpp::target_sizeof_int           8
set beam::compiler::cpp::target_sizeof_long          8
set beam::compiler::cpp::target_sizeof_long_long     8
set beam::compiler::cpp::target_sizeof_float         4
set beam::compiler::cpp::target_sizeof_double        8
set beam::compiler::cpp::target_sizeof_long_double   8
set beam::compiler::cpp::target_sizeof_pointer       8
set beam::compiler::cpp::target_sizeof_size_t        8
set beam::compiler::cpp::target_sizeof_wchar_t       2

#
# Alignment of built-in types
#
set beam::compiler::cpp::target_alignof_short       2
set beam::compiler::cpp::target_alignof_int         8
set beam::compiler::cpp::target_alignof_long        8
set beam::compiler::cpp::target_alignof_long_long   8
set beam::compiler::cpp::target_alignof_float       4
set beam::compiler::cpp::target_alignof_double      8
set beam::compiler::cpp::target_alignof_long_double 8
set beam::compiler::cpp::target_alignof_pointer     8
set beam::compiler::cpp::target_alignof_size_t      8
set beam::compiler::cpp::target_alignof_wchar_t     2

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
# Is size_t defined as "unsigned int" or "unsigned long" or what ?
set beam::compiler::cpp::target_size_t_int_kind   {unsigned long}

# Is wchar_t defined as "short int" or "int" or what ?
set beam::compiler::cpp::target_wchar_t_int_kind  {short int}

############################################################
# Section 4: Built-in macros
############################################################
set beam::compiler::cpp::standard_predefined_macros "* - __STDC__ __cplusplus"

# Predefined macros from native compiler
#
set beam::compiler::cpp::predefined_macro(__cplusplus)   1
set beam::compiler::cpp::predefined_macro(__STDC__)      0
set beam::compiler::cpp::predefined_macro(__MATH__)      1
set beam::compiler::cpp::predefined_macro(__STR__)       1
set beam::compiler::cpp::predefined_macro(NOLIBERROR)    1
set beam::compiler::cpp::predefined_macro(__AIXxSLIC__)  1
set beam::compiler::cpp::predefined_macro(__TIMESTAMP__) "some-time"

# Hacks to work around language extensions
set beam::compiler::cpp::predefined_macro(__int32)                      {int}
set beam::compiler::cpp::predefined_macro(__atomic)                     {}
set beam::compiler::cpp::predefined_macro(__StartVeryInfrequent())      {}
set beam::compiler::cpp::predefined_macro(__FinishVeryInfrequent())     {}
set beam::compiler::cpp::predefined_macro(__StartInfrequent())          {}
set beam::compiler::cpp::predefined_macro(__FinishInfrequent())         {}
set beam::compiler::cpp::predefined_macro(__StartSomewhatInfrequent())  {}
set beam::compiler::cpp::predefined_macro(__FinishSomewhatInfrequent()) {}
set beam::compiler::cpp::predefined_macro(__offsetof(type,field))       {(size_t)&(((type *)0)->field)}

############################################################
# Section 5: Miscellaneous options
############################################################
set beam::compiler::cpp::extern(builtin) "C++"
