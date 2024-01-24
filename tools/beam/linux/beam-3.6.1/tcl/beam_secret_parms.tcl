# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2002-2010 IBM Corporation. All rights reserved.
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
#        Daniel Brand
#
#    DESCRIPTION:
#
#        This is a collection of parameter settings that a casual user
#        sould not care about.
#
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        07/11/03  brand    Increased limits, especially important max_proc
#        01/16/03  brand    Addded macro_tests_explicitly
#        10/07/02  brand    Created


set beam::disabling_comment_capitalization   "ignore"

set beam::base_data         ""
set beam::innocent_suffix   ""
set beam::errors_suffix     ""
set beam::editor            "emacs"

set beam::attributes_require_must "yes"

set beam::evidence(functions)   "reject"
set beam::evidence(alias)       "to_prove"
#set beam::evidence(alias)       "inadmissible"  ;# this setting may be needed for pointer analysis
set beam::evidence(macro)       "inadmissible"
set beam::evidence(generated)   "admissible"
set beam::evidence(loop0)       "inadmissible"
set beam::evidence(loop1)         "admissible"
set beam::evidence(loop2)         "admissible"
set beam::evidence(default)       "admissible"
set beam::evidence(single_else)   "admissible"
set beam::evidence(multi_else)    "admissible"
set beam::evidence(boundary)      "admissible"
set beam::evidence(exception)     "admissible"; # if a func could throw it will
set beam::evidence(buffer)      "inadmissible"; # buffer assigns maximal strlen

set beam::use_evidence_not_on_path  "";

# 0: no restriction on evidence
# 1: from two independent test of same variable use only one as evidence 
set beam::evidence_restriction "1";

set beam::assume_loops_terminate  "yes";

# used when debugging a complaintand and want to know if a loop
# is assumed to modify something
#beam::enable_option "show_loops_with_side_effect"

# The lower relevance_limit the more detailed path info printed
# for complaints having a path
set beam::relevance_limit   "8"

# Should ipa process even those include files whose results are not saved?
# If "yes" they will be reprocessed every time included,
# which is how it used to be.
set beam::ipa_all_include_files "no"

# Should ipa store in the depository even functions invisible to
# other files?
set beam::ipa_write_external_only "yes"


# If file system allows files to be locked then do so to allow parallel
# compilation
set beam::lock_files  "yes"


# This is a limit on the amount of searching for a deadlock
set beam::deadlock_search_max "1000"


# The higher max_sat_time the longer is the theorem prover allowed to work
set beam::max_sat_time         "100"
set beam::max_sat_iterations   "150"
set beam::max_DF_analysis_time   "1000"
set beam::max_DF_analysis_time_per_1000_boxes   "100"
set beam::num_backtracks_to_make_sure   "1000"

# If two paths leading to error contain evidence needed to prove it,
# should we consider both?
# Answer "yes" will find more bugs, but cost more CPU time
set beam::preserve_evidence "no"

set beam::expand_max_evidence "inadmissible"
set beam::max_proc           "10000";   # maximal size for resulting proc

# When calculating a summary, need to be careful with arrays in a struct.
# Cannot simply use the size of the struct as its stride, because iterating
# through a big array would take too long.
# This is a limit on the largest array to be analyzed exactly.
# By the way, for an application there is no limit, because
# there is no need to converge
set beam::max_array_elements_for_stride "8"


set beam::max_infered_runtime_exceptions "5"

# This is a indication for which functions multithread info
# is to be collected.
# It is to be collected if 
# the function file satisfies the pattern beam::multithread_enabled_files and 
# the function name satisfies the pattern beam::multithread_enabled_functions
set beam::multithread_enabled_files "*"
set beam::multithread_enabled_functions "*"


# Should pre-conditions be derived from required quirk properties?
set beam::pre_conditions "no"

# Do not set user_innocent, should be used only to override beam::data
#set beam::user_innocent     ""


# This setting means that a function that does not return is considered
# to have a side-effect only if it MUST not return unconditionally
set beam::noreturn_fate "must"

# This setting means that a function that does not return is considered
# to have a side-effect even if sometimes it does return
#set beam::noreturn_fate "may"

# The problem with the above setting of "may" is that ipa will propagate
# even MAY noreturn attributes.
# It is very rare that a function does not call some library routine
# containing an assert. Therefore just about every function has the
# potential of aborting, and collecting this information is not useful.
# The most common exception are empty stub functions and complaints about
# those is not useful anyway.
# Therefore this "fake_may" has the same effect as "may" by assuming
# that every function has the potential of aborting, without calculating it.
#set beam::noreturn_fate "fake_may"

# Should do {...} while(0) be treated as the body only.
# It is a bit more efficient and avoid complaints about
# constant conditions and loop executed only once
set beam::do_while0_is_noop "yes"

# List of macros that contain a test inside, which should be considered
# evidence that it can go both ways
lappend beam::macro_tests_explicitly "feof"  "assert"

# Normally assume enum type to take 4 bytes even if fewer would be sufficient
set beam::enum_compacted  "no"

# Add the directories listed in "system_include_path" in the compiler
# configuration to beam::disabled_files.
set beam::add_system_include_dirs_to_disabled_files "yes"


# proc_sum is a pattern describing what to do with procedure summaries
# set beam::proc_sum "yes";    perform experiments, but no other analysis
# set beam::proc_sum "mod";    mod   info is available
# set beam::proc_sum "alias";  alias info is available
set beam::proc_sum "mod alias"


# This parameter controls whether proc summaries should be written to disk
set beam::pta_write_to_disk "no"


# This is a pattern indicating interest in lock hierarchy.
# If it contains "trace" then the hierarchy will be printed when
# the results of proc summary calculations are printed.
#set beam::pta_lock_hierarchy "trace"
set beam::pta_lock_hierarchy ""

# This is a flag that, if set to true, makes the pointer analysis
# to generate an output file for each function containing statistics
# about its summary graph (number of first class nodes, # of second class, etc
set beam::pta_statistics "no"

# Normally do not include node id when displaying nodes,
# because that would cause too much churn in testcases.
# For debugging set it to "yes"
set beam::pta_display_node_id "no"

# This is a flag that sets the order-sensitivity of the pointer analysis
# "Order-sensitive is what we used to call "flow-aware", and order-insensitive
# is what we use to call "flow-insensitive"
set beam::pta_order_sensitivity "partial_order_2_traversals"
#set beam::pta_order_sensitivity "aware"
#set beam::pta_order_sensitivity "insensitive"

# This is a flag that sets the "soundness" of the pointer analysis in
# case the analysis is not flow-insensitive (in which case it's always
# sound because the analysis doesn't care about edge indices)
#set beam::soundness "unsound"
set beam::soundness "sound"

# Tell the pointer analysis if you want a
# field-sensitive or field-insensitive analysis
set beam::pta_field_sensitive "yes"

# Tell the pointer analysis if you want a call chains to be collected.
# That is needed only if complaints are to be issued based on the pointer graph.
set beam::pta_call_chain "no"

# If a caller has been downgraded treat callees as unknown functions,
# but a callee summary could still be used if it is small enough.
# This parameter controls "small enough"
set beam::pta_excessive_callee_size "10"

# Abandon the computation of a summary if there are more alias edges
# in the graph than this parameter value
set beam::pta_max_alias_summary       "10000"
set beam::pta_max_alias_application   "10000"

# Abandon the computation of a summary if there are more alias edges
# per node than this parameter value
set beam::pta_max_alias_ratio_summary     "100"
set beam::pta_max_alias_ratio_application "100"

# Abandon the computation of a summary if there is a node with more
# assign edges than this parameter value
set beam::pta_max_assignments_summary     "60"
set beam::pta_max_assignments_application "1000"

# Abandon the computation of a summary if there are more nodes than this
set beam::pta_max_nodes_summary     "10000"
set beam::pta_max_nodes_application "10000"

# Abandon the computation of a summary if there is a fetch to be
# resolved against more assign sources than this parameter value
set beam::pta_max_assign_sources_summary     "100"
set beam::pta_max_assign_sources_application "200"
set beam::pta_max_assigns_to_resolve_summary     "2000"
set beam::pta_max_assigns_to_resolve_application "2000"

# Set the k-limit for the pointer analysis. This is related to
# the naming scheme for heap locations and local scope nodes,
# and its value indicates how many occurences of the name of
# the same function can appear in the name of the node
set beam::k_limit 1

# Set the q-limit for the pointer analysis. This is related to
# the "size" of a chain of initial values starting from a
# parameter or global variable
set beam::q_limit 2

# Choose between two modes of pointer analysis: Mode 1) consider in the
# analysis only pins that are to be interpreted as addresses. Mode 2)
# consider any assignment, even things like x = 1; when x is not a pointer
set beam::pta_only_addresses "no"

# A parameter used to merge heap nodes.
# Heap nodes with more than pta_merge_depth identical call frames
# are merged.
set beam::pta_merge_depth 1

# A parameter limiting the number of stack frames recorded with each heap node
set beam::pta_stack_limit 1

# During the pointer analysis, by default, merge multiple fetches
# out of the same node into groups based on some common characteristics.
set beam::pta_merge_fetches "no"

# During pointer analysis, by default, do not analyze heap
# (ie., when a call to "malloc" or the like is seen, simply skip it
set beam::pta_do_heap "yes"

# During pointer analysis, we may switch off the resolution of
# pointer increments such as "p++" or "p = p->next" (essentially
# ignoring such statements). The default is to include them by
# not testing for pointer increment (which means default is "no")
set beam::pta_test_ptr_increment "no"

# During pointer analysis, we usually consider kill information
# where a later assigment may kill the points-to facts generated
# by an earlier statement.
set beam::pta_do_kill_computation "yes"

# During pointer analysis, we may want to consider the conditions
# under which fetches and assignments occur in the code. Then a
# fetch only matches an assignment if they occur under conditions
# that are not inconsistent.
#set beam::pta_condition_sensitivity "insensitive"
set beam::pta_condition_sensitivity "sensitive"

# During pointer analysis *with* conditions, we may want to add
# conditions to alias edges in addition to conditions on operation
# edges. To do that, set the following parameter to "yes"
set beam::pta_conditions_on_alias_edges "yes"

# If we are doing field-sensitive analysis, and a node has
# too many outgoing offset edges, we collapse all these
# edges into the node and make the node's stride equals 1
# This parameters sets what it means to be "too many"
set beam::pta_excessive_offset_edges "5"

# Normally want copy propagation even during IPA, but this
# flag can be turned off to try running without it.
set beam::copy_propagation "yes"


######################################################################
# Do not change the settings below. Doing so will
# (a) false positives (invalid complaints)
# (b) false negatives (failure to report valid complaints)
# These flags are meant for delvelopers only!
######################################################################
namespace eval beam::ERROR1 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR2 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR3 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR4 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR5 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR6 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR7 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR8 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR9 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR10 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR11(heap_memory) {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR11(stack_memory) {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR11(file) {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR11 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
# ERROR12 is obsolete
namespace eval beam::ERROR13 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR14 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR15 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR16 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR17 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR18 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR19 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR20 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR21 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR22 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR23 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
# ERROR24 is obsolete
namespace eval beam::ERROR25 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR26 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
# ERROR27 is obsolete
namespace eval beam::ERROR28 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR29 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR30 {
  set enabled_for_compiler_generated  "yes";     # compiler generated calls like constructors
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR31 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR32 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR33 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR34 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR35 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR36 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR37 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR38(stack_memory) {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR38(file) {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR38 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR39 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR40 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::ERROR44 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}

namespace eval beam::MISTAKE1 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE2 {
  set enabled_for_handler             "no"; # no complaint about unreachable handlers
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE3 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE4 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE5 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE6 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE7 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE8 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
# MISTAKE9 does not exist anymore.
namespace eval beam::MISTAKE10 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE11 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE12 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE13 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::MISTAKE14 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
# MISTAKE15 is obsolete
namespace eval beam::MISTAKE16 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE17 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE18 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE19 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE20 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE21 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE22 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE23 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE24 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::MISTAKE25 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval beam::WARNING1 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING2 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING3 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING4 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING5 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING6 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::WARNING7 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING8 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING9 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING10 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING11 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::WARNING12 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING13 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING14 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING15 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING16 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING17 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING18 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING19 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING20 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING21 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING22 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::WARNING23 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::WARNING24 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::PORTABILITY1 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::PORTABILITY2 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::PORTABILITY3 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::PORTABILITY4 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::PORTABILITY5 {
  set disabled_for_user_written       "yes"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::SECURITY1 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::SECURITY2 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::CONCURRENCY1 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::CONCURRENCY2 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "no"
}
namespace eval beam::CONCURRENCY3 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::CONCURRENCY4 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}
namespace eval beam::CONCURRENCY5 {
  set disabled_for_user_written       "no"
  set enabled_for_compiler_generated  "yes"
}

namespace eval beam::JNI1 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval beam::JNI2 {
  set enabled_for_compiler_generated  "no"
  set disabled_for_user_written       "no"
}
namespace eval vim::VIM2 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval vim::VIM3 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
namespace eval vim::VIM4 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
# NUTSH1 no longer exists
namespace eval nutsh::NUTSH1 {
  set enabled_for_compiler_generated  "yes"
  set disabled_for_user_written       "no"
}
