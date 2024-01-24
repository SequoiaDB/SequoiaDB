# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2008-2010 IBM Corporation. All rights reserved.
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
#        Francis Wallingford and Daniel Brand
#
#    DESCRIPTION:
#
#        This file contains BEAM option parsing routines.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for recent modifications.

# Guard against multiple sourcing
if { [::info exists ::guard(beam_options)] } return
set ::guard(beam_options) 1

# Load some utils
source beam_util
source beam_prefixcc

namespace eval beam::options {
  
  # Pull in some utils
  namespace import ::beam::util::*
  namespace import ::beam::prefixcc::*
  
  #
  # This contains a list of BEAM variables that can be set with values,
  # one per line, in alphabetical order, not including the beam:: namespace.
  #
  variable options {
    acct
    add_system_include_dirs_to_disabled_files
    allocation_may_return_null
    assume_loops_terminate
    assume_no_overflow
    attributes
    attributes_require_must
    avalanche_prevention
    base_data
    build_root
    cc
    clean_enum
    compiler
    complaint_file
    data
    deadlock_search_max
    debugger
    dirty_enum
    disabled_files_for_parser_warnings
    disabled_files
    disabled_functions
    disabled_functions_by_name
    disabled_macros
    disabled_variables
    disabling_comment
    disabling_comment_capitalization
    disabling_comment_policy
    display_parms
    do_while0_is_noop
    dont_report_caught_runtime_exceptions
    editor
    errors_suffix
    errors1
    expand_kind
    expand_max_lines
    expand_max_evidence
    extended_enum
    function_name_style
    innocent_suffix
    innocent1
    ipa
    ipa_all_include_files
    k_limit
    lock_files
    machine
    machine_for_checks
    machine_description
    mapper
    max_array_elements_for_stride
    max_DF_analysis_time
    max_DF_analysis_time_per_1000_boxes
    max_infered_runtime_exceptions
    max_proc
    max_sat_iterations
    max_sat_time
    max_time_per_kloc_in_sec
    may_add_null
    may_call_null
    may_free_null
    may_read_null
    may_write_null
    multithread_enabled_files
    multithread_enabled_functions
    must_check_out_of_mem
    new_innocent_code_dir
    noreturn_fate
    num_backtracks_to_make_sure
    parms
    parser_file
    post_parms
    preserve_evidence
    proc_sum
    pta_call_chain
    pta_condition_sensitivity
    pta_conditions_on_alias_edges
    pta_display_node_id
    pta_do_heap
    pta_do_kill_computation
    pta_excessive_callee_size
    pta_excessive_offset_edges
    pta_field_sensitive
    pta_lock_hierarchy
    pta_max_alias
    pta_max_alias_ratio
    pta_max_assign_sources
    pta_max_assignments
    pta_max_assigns_to_resolve
    pta_max_nodes
    pta_merge_depth
    pta_merge_fetches
    pta_only_addresses
    pta_order_sensitivity
    pta_stack_limit
    pta_statistics
    pta_statistics
    pta_write_to_disk
    q_limit
    read_disabled_input_files
    references_may_be_null
    regression-mode
    relevance_limit
    root
    search_file_pattern
    search_top_dir
    security_mode
    soundness
    source
    state_file
    stats_file
    steps
    suppressed_complaint_file
    use_evidence_not_on_path
    user_innocent
    xml_complaint_file
  }
  
  #
  # This contains the names of BEAM options that require arguments,
  # one per line, not including the beam:: namespace
  #
  variable value_options {
    parms
    post_parms
    steps
    cc
    compiler
    mapper
    complaint_file
    parser_file
    source
    stats_file
    xml_complaint_file
  }
  
  #
  # This contains the names of BEAM options that take no arguments, one per line
  #
  variable single_options {
    --beam::conservative_dotbeam
    --beam::display_analyzed_files
    --beam::display_parms
    --beam::exit0
    --beam::gcc
    --beam::ipa_check
    --beam::list-all-ids
    --beam::list-all-variables
    --beam::list-ids
    --beam::list-variables
    --beam::new_returns_null
    --beam::nostdattr
    --beam::skip_cmdline_files_if_analyzed
    --beam::skip_dependencies
  }

  #
  # check_flags checks the correctness of flags.
  #
  # BEAM flags are of the following forms:
  #
  #   --beam::NAME=VALUE
  #   --beam_beam::NAME=VALUE
  #
  # where NAME can be something like ERROR1::enabling_policy
  #
  proc check_args { argv } {
    variable options
    variable value_options
    variable single_options
  
    foreach arg $argv {
      # Allow our single options
      if { [lsearch -exact $single_options $arg] != -1 } {
        continue
      }
      
      # See if the arg is an assignment. If it is,
      # set $assignment to the assignment. Otherwise,
      # error out or ignore the arg.
      
      switch -glob -- $arg {
        --beam_beam::* {
          regsub -- "^--beam_beam::" $arg "" assignment
        }
        
        --beam_beam:* {
          regsub -- "^--beam_beam:" $arg "--beam_beam::" new_arg
          beam::usage "Instead of \"$arg\" you perhaps meant \"$new_arg\""
        }
  
        --beam::* {
          regsub -- "^--beam::" $arg "" assignment
        }
  
        --beam:* {
          regsub -- "^--beam:" $arg "--beam::" new_arg
          beam::usage "Instead of \"$arg\" you perhaps meant \"$new_arg\""
        }
  
        --beam_*::* {
          regsub -- "^--beam_" $arg "" assignment
        }
  
        --beam_* {
          regsub -- "^--beam_" $arg "--beam::" new_arg
          beam::usage "Instead of \"$arg\" you perhaps meant \"$new_arg\""
        }
        
        -beam* {
          regsub -- "^-beam" $arg "--beam" new_arg
          beam::usage "Instead of \"$arg\" you need to say \"$new_arg\""
        }
        
        default {
          # Ignore everything else
          continue
        }
      }
    
      # Check the assignment in $assignment against known BEAM settings
      
      if { $assignment == "" } {
        beam::usage "Malformed option \"$arg\"; expecting parameter name"
      }
  
      if { [string first = $assignment] == -1 } {
        beam::usage "Malformed option \"$arg\"; expecting assignment"
      }
  
      # Extract var=val and check for errors
      
      set equal_pos [string first = $assignment]
      
      if { $equal_pos == 0 } {
        beam::usage "Assignment contains no variable: \"$arg\""
      }
      
      set var [string range $assignment 0 [expr $equal_pos - 1]]
      set val [string range $assignment [expr $equal_pos + 1] end]
      
      #
      # If this assignment came from something like --beam::var=val 
      # check whether VAR is one of the variables we recognize.
      # Assignments like --beam::XYZ::var=val are ignored.
      #
      
      if { [string first :: $var] == -1 } {
  
        # Check it - it is in the top level BEAM namespace
        
        if { [lsearch -exact $options $var] == -1 } {
          beam::usage "Unrecognized beam variable \"$var\" in \"$arg\""
        }
  
        #
        # Certain beam variables require a value. Make sure there is one
        #
  
        if { $val == "" } {
          if { [lsearch -exact $value_options $var] != -1 } {
            beam::usage "Assignment \"$arg\" requires value"
          }
        }
      }
    }
  }
  
  #
  # Helper to set beam::cc, depending on which options we were
  # given. There are currently two modes:
  #
  #  1) --beam::prefixcc is given. This is mutually exclusive
  #     with --beam::compiler - if they are given together, an
  #     error results.
  #
  #     The first non-BEAM option on the command-line is the
  #     compiler as it would be invoked, so it has to be found
  #     on $PATH.
  #
  #     If the compiler is supported by the prefixcc mechanism,
  #     then we know what it is, and we can set beam::cc and
  #     be done. The user can also use --beam::cc= to skip
  #     the detection and force a compiler.
  #
  #     The binary is stored in beam::prefixcc so that when
  #     it is auto-configured (after command-line processing),
  #     it can be found.
  #
  #  2) --beam::compiler= is given. This is mutually exclusive
  #     with --beam::prefixcc.
  #
  #     The compiler files are loaded (if given), and the
  #     compiler::cc variables are compared with beam::cc
  #     (if given) and if they are the same, beam::cc is
  #     the result. It is an error if they are different.
  #
  # This sets ::beam::cc and possibly ::beam::prefixcc. It also loads
  # any --beam::compiler= files.
  #
  # This returns a possibly modified argv without --beam::prefixcc or the
  # compiler binary in it.
  #
  proc set_beam_cc { argv } {
    set has_prefixcc 0
    set has_compiler 0

    foreach arg $argv {
      switch -glob -- $arg {
        --beam::prefixcc {
          set has_prefixcc 1
        }
        
        --beam::cc=* {
          regsub -- "^--beam::cc=" $arg {} ::beam::cc
        }
        
        --beam::compiler=* {
          set has_compiler 1
          regsub -- "^--beam::compiler=" $arg {} file
          beam::gsource $file
        }
      }
    }

    set argv [ut_list_filter_out --beam::prefixcc $argv]
    
    if { $has_prefixcc && $has_compiler } {
      beam::error "The --beam::prefixcc option may not be given with " \
                  "the --beam::compiler option."
    }
    
    if { $has_prefixcc } {
      set binary [bpcc_find_binary argv]

      if { $binary == "" } {      
        beam::error "No compiler binary was given with the --beam::prefixcc option."
      }

      if { [ut_value ::beam::cc] == "" } {
        # Discover beam::cc via pattern matching the binary
        set ::beam::cc [bpcc_identify $binary]
      
        if { $::beam::cc == "" } {
          beam::error "The '$binary' compiler given with --beam::prefixcc " \
                      "is not recognized. See the documentation for details."
        }
      }

      # Save the binary for later      
      set ::beam::prefixcc $binary
    } else {
      # Check beam::cc and the compiler config files to ensure they are consistent.
      # This sets a final ::beam::cc value.
      ::beam::compiler::check_compiler_config
    }
    
    return $argv
  }
  
  #
  # Helper to set beam::lang based on the command-line.
  #
  proc set_beam_lang { argv } {
    # If the driver knew the language, use that directly.
    # This is because the driver may process language-switching
    # directives that dictate a language independently of the
    # file extension, which we must honor.
    
    if { [::info exists ::env(beam_language)] } {
      set ::beam::lang $::env(beam_language)
      set ::beam::compiler_namespace ::beam::compiler::${::beam::lang}
      return
    }
    
    set desc [::beam::compiler::describe_flags $argv]    
    set last_family ""
    set ::beam::lang "unset"
    
    ut_foreach_desc type rest pos $argv $desc {
      if { $type != "source" } { continue }
        
      set lang [lindex $rest 0]
      set family $lang
        
      # For mutual-exclusive purposes, asm, c, and c++ are the same
      if { $family == "c" }   { set family c_fam }
      if { $family == "cpp" } { set family c_fam } 
      if { $family == "asm" } { set family c_fam } 
       
      if { $last_family != "" } {
        if { $family != $last_family } {
          beam::usage "Only one language per invocation please"
        }
      }
        
      set ::beam::lang $lang     ;# Last one wins here
      set ::beam::langs($lang) 1 ;# Collects all languages seen
    }
    
    set ::beam::compiler_namespace ::beam::compiler::${::beam::lang}
  }
  
  #
  # Helper to process a "foo=bar" assignment, setting a Tcl variable.
  #
  proc do_assignment { assignment } {
    set equal_pos [string first = $assignment]
    
    if { $equal_pos == -1 } {
      set var $assignment
      set val "yes"
    } else {
      set var [string range $assignment 0 [expr $equal_pos - 1]]
      set val [string range $assignment [expr $equal_pos + 1] end]
    }
    
    namespace eval :: [list set $var $val]
  }

  #
  # Wrapper around bo_parse_opts.
  #
  proc bo_parse_core_opts { argv } {
    return [bo_parse_opts $argv 0]
  }
  
  #
  # Process BEAM's arguments.
  #
  # This will load Tcl files and set up Tcl settings according to the
  # command-line.
  #
  # Since this routine expands @file arguments among other things,
  # this will return a modified $argv.
  #
  proc bo_parse_opts { argv do_expansion } {
    # Initialize
    set ::beam::steps "beam_default_steps"

    # Pass 1: Very early, basic options that exit quickly.
    
    foreach arg $argv {
      switch -exact -- $arg {
        --version { beam::version }
        --help    { beam::help }
      }
    }

    # Pass 2: Find and set up beam::cc.
    
    set argv [set_beam_cc $argv]
    
    # Pass 3: Based on beam::cc, BEAM usually loads a built-in mapper.
    # However, we allow the user to override this with --beam::mapper=.
    # After this point, the mapper is valid and can be used to process
    # the rest of the command-line.
    
    foreach arg $argv {
      switch -glob -- $arg {
        --beam::mapper=* {
          regsub -- "^--beam::mapper=" $arg {} mapper
          beam::gsource $mapper
        }
      }
    }
    
    # Pass 4: Expand @file arguments and environment variables. This
    # happens after beam::cc is set because each compiler expands
    # @file arguments and environment variables differently.
    #
    # This is only done in the driver.
    
    if { $do_expansion } {
      set argv [::beam::compiler::expand_flags $argv]
    }
    
    # Pass 5: Make sure all BEAM related args are OK. This happens after
    # the command-line is expanded so that we see all options.
    
    check_args $argv
    
    # Pass 6: Set ::beam::lang. This happens after the command-line
    # is expanded so that we see all options. This relies on the
    # describer in order to find the source files listed.
    
    set_beam_lang $argv
  
    # Pass 7: Find and load the parms. We need beam::lang to be set for
    # this since that denotes which parameters are loaded.
    set parms "beam_default_parms"

    foreach arg $argv {    
      switch -glob -- $arg {
        --beam::parms=* { regsub -- "^--beam::parms=" $arg {} parms }
      }
    }
    
    beam::gsource $parms
    
    # Pass 8: Process the remaining arguments. Most of these are simple
    #         variable assignments; a few are special.
    
    foreach arg $argv {
      switch -glob -- $arg {
        --beam::mapper=*   -
        --beam::cc=*       -
        --beam::compiler=* -
        --beam::parms=*    { ;# Already done above }

        --beam-trace {
          beam::error "Option `--beam-trace' needs argument (--beam-trace=argument)"
        }
        
        --beam-trace=* {
          # We allow this to be set multiple times. Let's build a list.
          regsub -- "^--beam-trace=" $arg {} to_trace
          lappend ::beam::dev::trace $to_trace
        }
        
        --beam::source=* {
          regsub -- "^--beam::source=" $arg {} to_source
          beam::gsource $to_source
        }
        
        --beam-* {
          # Hidden developer options. We set these in beam::dev::
          regsub -- "^--beam-" $arg {} assignment
          do_assignment "beam::dev::$assignment"
        }
        
        --beam::* {
          # Normal variable assignment. We set these in beam::
          regsub -- "^--" $arg {} assignment
          do_assignment $assignment
        }
        
        --beam_* {
          # Fully-qualified variable assignment.
          regsub -- "^--beam_" $arg {} assignment
          do_assignment $assignment
        }
      }
    }
    
    # Finally, load beam::post_parms, if any.
    
    if { [::info exists ::beam::post_parms] && $::beam::post_parms != "" } {
      beam::gsource $::beam::post_parms
    }
    
    return $argv
  }

  # Export our public utils
  namespace export bo_*
}
