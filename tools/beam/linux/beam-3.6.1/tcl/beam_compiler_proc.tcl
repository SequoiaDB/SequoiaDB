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
#        Francis Wallingford
#
#    DESCRIPTION:
#
#        This script defines helper routines for translating the
#        compiler arguments for compilers to EDG.
#
#        Note: This is used in both the beam_compile and beam_lang
#        executables. Care must be taken to write routines in a way
#        such that this file doesn't depend on frontend, beam_define_proc,
#        $::beam::lang, or other things that don't exist in the
#        beam_compile binary. If these things are used, they must be
#        used in routines that are guaranteed to not be needed in
#        the beam_compile binary.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for more recent modifications.
#

# Guard against multiple sourcing
if { [::info exists ::guard(beam_compiler_proc)] } return
set ::guard(beam_compiler_proc) 1

# For the backwards-compat routines below
source beam_util

namespace eval ::beam::compiler {

############################################################
# List of variables
############################################################

# This is a list of all of the variables that can be set
# in the configuration files. This is used in two places:
#
# 1) At the top of the configuration file,
#    ::beam::compiler::init_settings is called. This
#    routine ensures that all of the variables in this
#    list are bound to the calling namespace so that
#    'set foo bar' may be used instead of 'variable foo bar'.
#
# 2) At the top of map_*_flags, ::beam::compiler::import is
#    called. This routine ensures that the variables are
#    all imported into the proc as local variables so they
#    can be set and read without qualifying them.
#
# Note: See the top of edg_config.c for details on how to
#       add variables to this list (and other places they
#       need to go)

variable c_config_var_list {
  cc

  language_dialect
  c99_mode
  strict_mode
  gnu_mode
  msvc_mode
  
  language_friend_injection_enabled
  language_use_nonstandard_for_init_scope
  language_string_literals_are_const
  language_allow_dollar_in_id_chars
  language_end_of_line_comments_allowed
  language_anachronisms_allowed
  language_exceptions_enabled
  language_rtti_enabled
  language_nonstandard_qualifier_deduction
  language_explicit_keyword_enabled
  language_namespaces_enabled
  language_old_specializations_allowed
  language_guiding_decls_allowed
  language_restrict_keyword_enabled
  language_wchar_t_is_keyword
  language_bool_is_keyword
  language_long_long_is_standard
  language_typename_enabled
  language_do_dependent_name_processing
  language_nonclass_prototype_instantiations
  language_alternative_tokens_allowed
  language_class_name_injection_enabled
  language_arg_dependent_lookup_enabled
  language_nonstandard_using_decl_allowed
  language_designators_allowed
  language_extended_designators_allowed
  language_compound_literals_allowed
  language_ignore_std_namespace
  language_allow_spaces_in_include_directive
  language_custom_int_size_modifier
  language_allow_nonstandard_anonymous_unions
  language_trigraphs_allowed
  language_overload_keyword_enabled

  language_needs_builtin_alloca
  language_needs_builtin_assert1
  language_needs_builtin_assert2
  
  system_include_path
  
  predefined_macro
  standard_predefined_macros

  target_gpp_dependent_name_lookup
  
  target_char_bit
  target_plain_char_is_unsigned
  target_plain_int_bit_field_is_unsigned
  target_enum_bit_fields_are_always_unsigned
  target_little_endian
  target_string_literals_are_readonly
  
  target_sizeof_short
  target_sizeof_int
  target_sizeof_long
  target_sizeof_long_long
  target_sizeof_float
  target_sizeof_double
  target_sizeof_long_double
  target_sizeof_pointer
  target_sizeof_custom_int
  
  target_alignof_short
  target_alignof_int
  target_alignof_long
  target_alignof_long_long
  target_alignof_float
  target_alignof_double
  target_alignof_long_double
  target_alignof_pointer
  target_alignof_custom_int
  
  target_sizeof_size_t
  target_size_t_int_kind

  target_sizeof_ptrdiff_t
  target_ptrdiff_t_int_kind

  target_sizeof_char16_t
  target_sizeof_char32_t
  target_char16_t_int_kind
  target_char32_t_int_kind

  target_sizeof_wchar_t
  target_wchar_t_int_kind
  target_wchar_t_is_unsigned

  target_sizeof_wint_t
  target_wint_t_int_kind
  target_wint_t_is_unsigned
  
  target_flt_mant_dig
  target_flt_max_exp
  target_flt_min_exp
  target_dbl_mant_dig
  target_dbl_max_exp
  target_dbl_min_exp
  target_ldbl_mant_dig
  target_ldbl_max_exp
  target_ldbl_min_exp
  
  target_enum_types_can_be_smaller_than_int
  target_bit_field_container_size
  target_zero_width_bit_field_alignment
  target_zero_width_bit_field_affects_struct_alignment
  target_unnamed_bit_field_affects_struct_alignment
  target_microsoft_bit_field_allocation

  target_char_constant_first_char_most_significant
  target_bool_int_kind
  target_force_one_bit_bit_field_to_be_unsigned
  target_right_shift_is_arithmetic
  target_too_large_shift_count_is_taken_modulo_size
  target_sizeof_ptr_to_data_member
  target_alignof_ptr_to_data_member
  target_sizeof_ptr_to_member_function
  target_alignof_ptr_to_member_function
  target_sizeof_virtual_function_info
  target_alignof_virtual_function_info

  extern  
  function_name_is_string_literal
  suppressed_diagnostics

  testing_c_vars
}

variable java_config_var_list {
  cc
 
  language_source_level
  language_encoding

  system_endorsed_dirs
  system_boot_classpath
  system_ext_dirs
  system_sourcepath
  system_classpath
  system_output_directory

  testing_java_vars
}

# This returns the variable list for the given language

proc get_var_list {lang} {
  switch -exact -- $lang {
    c   -
    cpp {
      return $::beam::compiler::c_config_var_list
    }
    
    java {
      return $::beam::compiler::java_config_var_list
    }
    
    default {
      ::beam::internal_error \
        "get_var_list called with bad language $lang"
    }
  }
  
  return {}
}
  
############################################################
# Miscellaneous utilities
############################################################

# Import all of the variables from the configuration list
# above into the caller's namespace.
# Since this can be called from the frontend, it doesn't
# rely on ::beam::lang. Instead, the language comes from
# the caller's namespace, which must be of the form
# "::beam::compiler::{lang}".
#
# Note that with the new fully-qualified settings, this
# is no longer used from BEAM's Tcl files. However, users
# with old compiler configuration files will still be calling
# this, so it can not go away.

proc init_settings {} {
  set ns [uplevel { namespace current }]
  
  if { [regsub "^::beam::compiler::" $ns {} lang] != 0 } {
    set list [get_var_list $lang]
  
    foreach var $list {
      namespace eval $ns [list variable $var]
    }
  }
}

# Import all of the variables from the configuration list
# above into the caller's scope. Since this is called from
# the backend and since the procedure can exist anywhere
# independent of the language, this relies on ::beam::lang
# and ::beam::compiler_namespace.

proc import {} {
  if { [::info exists ::beam::lang] } {
    set list [get_var_list $::beam::lang]
  
    if { [::info exists ::beam::compiler_namespace] } {
      foreach var $list {
        set qualified_var [set ::beam::compiler_namespace]::[set var]
        uplevel [list upvar $qualified_var $var]
      }
    }
  }
  
  # Also, for backwards compat, inject a few things into the
  # caller's namespace. This is defined below.
  import_backwards_compat
}

# Make sure that the --beam::cc setting and the files
# loaded via --beam::compiler= agree on what the compiler is.
#
# This sets ::beam::cc.

proc check_compiler_config {} {

  # Ensure that all loaded configuration files specify the same
  # compiler in the "cc" variable. It is an error if they don't.
  
  set last_cc ""
  set last_ns ""
  
  foreach this_ns [namespace children ::beam::compiler] {
    if { [::info exists [set this_ns]::cc] } {
      # Check to see if this compiler is consistent
      # with the previous one. If this is the first
      # one, set up the previous one.
      
      set this_cc [set [set this_ns]::cc]
      
      if { $last_cc != "" } {
        if { $this_cc != $last_cc } {
          regsub "^::beam::compiler::" $this_ns {} this_lang
          regsub "^::beam::compiler::" $last_ns {} last_lang

          beam::error \
            "The configurations loaded for `$this_lang' and `$last_lang'"  \
            "have different settings for `cc' (`$this_cc' and `$last_cc'," \
            "respectively). When loading multiple configuration files,"    \
            "they must all be for the same compiler."
        }
      }
      
      set last_cc $this_cc
      set last_ns $this_ns
    }
  }

  # Now that we have one $last_cc variable from the compiler
  # config files, check the old flag --beam::cc= for consistency.
  # Set ::beam::cc to the correct value.
  #
  # The rules are complicated to be backwards-compatible.
  #
  # If $last_cc and ::beam::cc are unset, use "default".
  # If $last_cc is unset, use ::beam::cc.
  # If ::beam::cc is unset, use $last_cc.
  # If they are both set and the same, use the value.
  # If they are both set and different, it's an error unless
  #   $last_cc is "default", in which case ::beam::cc wins
  #   because $last_cc came from one of our default compiler
  #   config files and we want ::beam::cc to override that.

  if { $last_cc == "" && ! [::info exists ::beam::cc] } {
    set ::beam::cc "default"
  } elseif { $last_cc == "" } {
    # Leave ::beam::cc set as-is.
  } elseif { ! [::info exists ::beam::cc] } {
    set ::beam::cc $last_cc
  } elseif { $last_cc == $::beam::cc } {
    # Ok - both are the same.
  } elseif { $last_cc == "default" } {
    # Ok - use beam::cc as an override.
  } else {
    # Not ok - both are set and different.

    beam::error \
      "The option '--beam::cc=$::beam::cc' does not match the" \
      "compiler configuration for '$last_cc'."
  }
}

# This will call the correct map_flags proc. If one
# has already been loaded and set with "set_mapper",
# then it is used. If not, one is loaded based on
# the current compiler. If no compiler is set that
# we recognize, we fall back to BEAM's default
# mapper (with a warning) for the simple stuff.

proc map_flags {flags} {
  load_map_flags_proc
  return [$::beam::compiler::map_flags_proc $flags]
}

# This will load a mapper. First it checks for BEAM mappers
# for the current compiler. Then it falls back to our default
# mapper, which works for simple stuff.
#
# This routine sets ::beam::compiler::map_flags_proc

proc load_map_flags_proc {} {

  # Set up the default name and location
  
  set default_script beam_map_default_flags.tcl
  set default_dir    ${::install_dir}/tcl

  # If there is one loaded, do nothing
  
  if { [::info exists ::beam::compiler::map_flags_proc] } {
    return
  }
  
  # Try to auto-load the mapper based on ::beam::cc
  
  set name "default"
  
  if { [::info exists ::beam::cc] } {
    set name $::beam::cc
  }

  # Try to load "beam_map_$name_flags.tcl" from the current
  # directory and the BEAM install directory.
  
  beam::qsource beam_map_${name}_flags.tcl
  
  # See if it set up a mapper
  
  if { [::info exists ::beam::compiler::map_flags_proc] } {
    return
  }
  
  # Hmm, no mapper for this compiler - print a warning
  # if the user tried to choose a mapper, and fall back to 'default'
  
  beam::warning "Argument mapper for compiler '$name' is not defined" \
                "or has not been loaded. Falling back to" \
                "the default argument mapper..."

  beam::gsource "$default_dir/$default_script"

  # Assert that it set up a mapper
  
  if { ! [::info exists ::beam::compiler::map_flags_proc] } {
    beam::internal_error "$default_script didn't set up a mapper"
  }
}

# This is called from a map_flags.tcl file when it is
# loaded. This sets the loaded map_flags proc as
# the value of $::beam::compiler::map_flags_proc
# unless it has already been set. This is so that
# multiple mappers may be loaded, but the first
# one loaded is the one that is used by default.

proc set_mapper {mapper} {
  if { ! [::info exists ::beam::compiler::map_flags_proc] } {
    set ::beam::compiler::map_flags_proc $mapper
  }
}

# This will call the correct describe_flags proc. If one
# has already been loaded and set with "set_describer",
# then it is used. If not, one is loaded based on
# the current compiler. If no compiler is set that
# we recognize, we fall back to BEAM's default
# describer (with no warning) for the simple stuff.

proc describe_flags {flags} {
  load_describe_flags_proc
  return [$::beam::compiler::describe_flags_proc $flags]
}

# This will load a describer. First it checks for BEAM describers
# for the current compiler. Then it falls back to our default
# describer, which works for simple stuff.
#
# This routine sets ::beam::compiler::describe_flags_proc

proc load_describe_flags_proc {} {

  # Set up the default name and location
  
  set default_script beam_map_default_flags.tcl
  set default_dir    ${::install_dir}/tcl

  # If there is one loaded, do nothing
  
  if { [::info exists ::beam::compiler::describe_flags_proc] } {
    return
  }
  
  # Try to auto-load the describer based on ::beam::cc
  
  set name "default"
  
  if { [::info exists ::beam::cc] } {
    set name $::beam::cc
  }

  # Try to load "beam_map_$name_flags.tcl" from the current
  # directory and the BEAM install directory.
  
  beam::qsource beam_map_${name}_flags.tcl
  
  # See if it set up a describer
  
  if { [::info exists ::beam::compiler::describe_flags_proc] } {
    return
  }
  
  # Hmm, no mapper for this compiler - fall back to 'default'
  
  beam::gsource "$default_dir/$default_script"

  # Assert that it set up a describer
  
  if { ! [::info exists ::beam::compiler::describe_flags_proc] } {
    beam::internal_error "$default_script didn't set up a describer"
  }
}

# This is called from a map_flags.tcl file when it is
# loaded. This sets the loaded describe_flags proc as
# the value of $::beam::compiler::describe_flags_proc
# unless it has already been set. This is so that
# multiple mappers may be loaded, but the first
# one loaded is the one that is used by default.

proc set_describer {describer} {
  if { ! [::info exists ::beam::compiler::describe_flags_proc] } {
    set ::beam::compiler::describe_flags_proc $describer
  }
}

# This will call the correct expand_flags proc. If one
# has already been loaded and set with "set_expander",
# then it is used. If not, one is loaded based on
# the current compiler. If no compiler is set that
# we recognize, we fall back to BEAM's default
# expander (with a warning) for the simple stuff.

proc expand_flags {flags} {
  load_expand_flags_proc
  return [$::beam::compiler::expand_flags_proc $flags]
}

# This will load an expander. First it checks for BEAM
# expanders for the current compiler. Then it falls back
# to our default expander, which works for simple stuff.
#
# This routine sets ::beam::compiler::expand_flags_proc

proc load_expand_flags_proc {} {

  # Set up the default name and location
  
  set default_script beam_map_default_flags.tcl
  set default_dir    ${::install_dir}/tcl

  # If there is one loaded, do nothing
  
  if { [::info exists ::beam::compiler::expand_flags_proc] } {
    return
  }
  
  # Try to auto-load the expander based on ::beam::cc
  
  set name "default"
  
  if { [::info exists ::beam::cc] } {
    set name $::beam::cc
  }

  # Try to load "beam_map_$name_flags.tcl" from the current
  # directory and the BEAM install directory.
  
  beam::qsource beam_map_${name}_flags.tcl
  
  # See if it set up an expander
  
  if { [::info exists ::beam::compiler::expand_flags_proc] } {
    return
  }
  
  # Hmm, no expander for this compiler - fall back to 'default'
  
  beam::gsource "$default_dir/$default_script"

  # Assert that it set up an expander
  
  if { ! [::info exists ::beam::compiler::expand_flags_proc] } {
    beam::internal_error "$default_script didn't set up an expander"
  }
}

# This is called from a map_flags.tcl file when it is
# loaded. This sets the loaded expand_flags proc as
# the value of $::beam::compiler::expand_flags_proc
# unless it has already been set. This is so that
# multiple expanders may be loaded, but the first
# one loaded is the one that is used by default.

proc set_expander {expander} {
  if { ! [::info exists ::beam::compiler::expand_flags_proc] } {
    set ::beam::compiler::expand_flags_proc $expander
  }
}

# This will call the correct config_flags proc. If one
# has already been loaded and set with "set_configurer",
# then it is used. If not, one is loaded based on
# the current compiler. If no compiler is set that
# we recognize, we fall back to BEAM's default
# configurer (with a warning) for the simple stuff.

proc config_flags {flags} {
  load_config_flags_proc
  return [$::beam::compiler::config_flags_proc $flags]
}

# This will load a configurer. First it checks for BEAM
# configurers for the current compiler. Then it falls back
# to our default configurer, which works for simple stuff.
#
# This routine sets ::beam::compiler::config_flags_proc

proc load_config_flags_proc {} {

  # Set up the default name and location
  
  set default_script beam_map_default_flags.tcl
  set default_dir    ${::install_dir}/tcl

  # If there is one loaded, do nothing
  
  if { [::info exists ::beam::compiler::config_flags_proc] } {
    return
  }
  
  # Try to auto-load the configurer based on ::beam::cc
  
  set name "default"
  
  if { [::info exists ::beam::cc] } {
    set name $::beam::cc
  }

  # Try to load "beam_map_$name_flags.tcl" from the current
  # directory and the BEAM install directory.
  
  beam::qsource beam_map_${name}_flags.tcl
  
  # See if it set up a configurer
  
  if { [::info exists ::beam::compiler::config_flags_proc] } {
    return
  }
  
  # Hmm, no configurer for this compiler - fall back to 'default'
  
  beam::gsource "$default_dir/$default_script"

  # Assert that it set up a configurer
  
  if { ! [::info exists ::beam::compiler::config_flags_proc] } {
    beam::internal_error "$default_script didn't set up a configurer"
  }
}

# This is called from a map_flags.tcl file when it is
# loaded. This sets the loaded config_flags proc as
# the value of $::beam::compiler::config_flags_proc
# unless it has already been set. This is so that
# multiple configurers may be loaded, but the first
# one loaded is the one that is used by default.

proc set_configurer {configurer} {
  if { ! [::info exists ::beam::compiler::config_flags_proc] } {
    set ::beam::compiler::config_flags_proc $configurer
  }
}

# This switches languages if the current language isn't the
# destination language. This is responsible for switching
# the global state variables and returning 1 if the language
# was switched, or 0 if the language doesn't need switching.
#
# This may change ::beam::lang and ::beam::compiler_namespace

proc switch_compiler_config {lang} {
  
  # If lang is the current lang, do nothing
  
  if { $lang == $::beam::lang } {
    return 0
  }
  
  set ::beam::lang $lang
  set ::beam::compiler_namespace ::beam::compiler::${::beam::lang}
  
  return 1
}

# Read in the xlc config file using our external binary.
# Use the given file and stanza unless $argv contains
# -F options, which will override the given file and
# stanza. Each -F option may override the file
# (-F/file), the stanza (-F:stanza), or both (-F/file:stanza).

proc get_xlc_config_opts {argv file stanza} {
  
  # Look through $argv for all -F options and
  # update $file and $stanza
  
  set len [llength $argv]
  
  for { set i 0 } { $i < $len } { incr i } {
    
    set arg [lindex $argv $i]
    
    switch -glob -- $arg {
      -F  {
            incr i
            set opt [lindex $argv $i]
          }
              
      -F* {
            set opt [string range $arg 2 end]
          }
      
      default { continue }
    }
    
    # Each branch that gets here sets $opt. Check it
    # for "file", ":stanza", or "file:stanza" and update
    # $file and $stanza accordingly
    
    set colon_pos [string first : $opt]
    
    if { $colon_pos == -1 } {
      # File only
      set file $opt
    } elseif { $colon_pos == 0 } {
      # Stanza only
      set stanza [string range $opt 1 end]
    } else {
      # Both
      set before_colon [expr { $colon_pos - 1 }]
      set after_colon  [expr { $colon_pos + 1 }]
      
      set file         [string range $opt 0 $before_colon]
      set stanza       [string range $opt $after_colon end]
    }
  }
 
  # Load up the config file by running our external binary 

  set args {}
  
  set cmd {
    exec ${::install_dir}/bin/beam_parse_xlc_config \
           ${file}:${stanza}
  }
    
  set rc [catch $cmd output]
    
  if { $rc != 0 } {
    puts $output
    beam::warning "Running beam_parse_xlc_config failed"
  } else {
    set args $output
  }
  
  return $args
}

############################################################
# Backwards compat
############################################################

# We used to document these routines here. Also, these are
# used heavily in the map_flags and friends, which only
# imports from ::beam::compiler. This is for backwards
# compatibility.

namespace import ::beam::util::ut_list_add
rename ut_list_add list_add

namespace import ::beam::util::ut_list_add_front
rename ut_list_add_front list_add_front

namespace import ::beam::util::ut_list_push
rename ut_list_push list_push

namespace import ::beam::util::ut_list_pop
rename ut_list_pop list_pop

namespace import ::beam::util::ut_unescape_arg
rename ut_unescape_arg unescape_arg

# This is called during import, and places the above
# into the caller's namespace
proc import_backwards_compat {} {
  # Two to get out of ::beam::compiler::import
  uplevel 2 {
    if { [namespace current] != "::beam::compiler" } {
      namespace import -force ::beam::compiler::list_add
      namespace import -force ::beam::compiler::list_add_front
      namespace import -force ::beam::compiler::list_push
      namespace import -force ::beam::compiler::list_pop
      namespace import -force ::beam::compiler::unescape_arg
    }
  }
}

# These are the names we used in mappers. Some of them may
# not be used any longer, but users may have copies of mappers
# or custom mappers based on those old files. So these may
# never be removed.

proc ::beam::switch_compiler_config {lang} {
  return [::beam::compiler::switch_compiler_config $lang]
}

proc ::beam::set_mapper {mapper} {
  return [::beam::compiler::set_mapper $mapper]
}

proc ::beam::set_describer {describer} {
  return [::beam::compiler::set_describer $describer]
}

proc ::beam::set_expander {expander} {
  return [::beam::compiler::set_expander $expander]
}

proc ::beam::set_configurer {configurer} {
  return [::beam::compiler::set_configurer $configurer]
}

############################################################
# Export utils that can be imported into other namespaces
############################################################

namespace export *

############################################################
# End of file and end of namespace
############################################################
}
