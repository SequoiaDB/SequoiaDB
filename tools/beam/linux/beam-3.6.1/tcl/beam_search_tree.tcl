# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2004-2010 IBM Corporation. All rights reserved.
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
#        Frank Wallingford
#
#    DESCRIPTION:
#
#        This is applicable to source code organized in different directories.
#        You want to place a BEAM parms Tcl file into one directory
#        and make its settings apply to all the subdirectories.
#        Another such Tcl file in a subdirectory might further override
#        those settings.
#
#        This is a script of how to walk all directories between the
#        source file being compiled (in $env(beam_source_file)) and the
#        directory specified by $search_top_dir. In each directory, if a file
#        that matches the name is found, it is added to a list of
#        files to be sourced. When $search_top_dir is reached, the files are
#        sourced in reverse order from how they were found, so that
#        files in sub-directories override settings in parent directories.
#
#
#    USAGE:
#
#        First you need a project specific Tcl file, if you do not have one already.
#        That is the file you specify on the command line using --beam::parms=
#        That file starts with a line of the form
#               source beam_default_parms.tcl
#        Follow that with 
#               source beam_search_tree.tcl
#        
#        By default beam_search_tree.tcl looks for tcl files of the form "beam*.tcl"
#        If you do not like it, before sourcing beam_search_tree.tcl add
#               set    ::beam::search_file_pattern    "beam_local*.tcl"
#
#        By default beam_search_tree.tcl starts at directory specified using 
#        beam::root; if that is not specified the default is "/"
#        If you do not like it, before sourcing beam_search_tree.tcl add
#               set    ::beam::search_top_dir         "/u/jones/src"
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        See ChangeLog for modifications.

source beam_util

# In our namespace

namespace eval ::beam::search {

namespace import ::beam::util::*

# search_file_pattern is the name (or shell-style glob pattern) of the
# files to source if they are found while walking the directories.
#
# This should be set from your project-specific file.
#
# Example:  set ::beam::search_file_pattern "*.tcl"
  
if { ! [ ::info exists ::beam::search_file_pattern ] } {
  set ::beam::search_file_pattern "beam*.tcl"
}

# If search_top_dir is set, the traversal will not proceed above
# this directory.
# Note that this will only work if every source file compiled
# by beam lives under this directory (after all symlinks are
# resolved).
#
# If this directory is not encountered during the traversal,
# it will automatically stop at the root. This may have performance
# consequences if you run BEAM on source files in a networked
# file system.
#
# Also note that if the traversal makes it outside your source
# tree, it may load files that match $search_file_pattern by
# accident.
#
# This may be set from your project specific file.
#
# Example:  set ::beam::search_top_dir "/afs/eda/projects/my_project"

if { ! [ ::info exists ::beam::search_top_dir ] } {
  if { [ ::info exists ::beam::root ] } {
    set ::beam::search_top_dir $::beam::root
  }
}

################################################################
# Anything below here should only be modified to change the
# algorithm. The user-defined settings are above this line.
################################################################

# This is the list of files to source. It starts empty, and is appended to
# as files matching $file_pattern are found.

set files_to_source ""

# Utility to see if a directory is $top_dir
# Since [file normalize] isn't around until Tcl 8.4,
# we'll use our custom ::beam::abspath.
#
# If the dir is the same as the search_top_dir or
# "/", return 1. Otherwise, return 0.

proc dir_is_top { dir } {
  
  # Use beam::abspath to normalize the filenames

  if { [ut_file_exists $dir] } {
    set abs_dir [::beam::abspath $dir]
  } else {
    set abs_dir $dir
  }
  
  if { [ut_file_exists $::beam::search_top_dir] } {
    set abs_top [::beam::abspath $::beam::search_top_dir]    
  } else {
    set abs_top $::beam::search_top_dir
  }
  
  # Test
  
  if { $abs_dir == $abs_top } {
    return 1
  }

  if { $abs_dir == [::beam::abspath "/"] } {
    return 1
  }
  
  return 0
}

# Utility to scan the directory argument for any files
# matching $search_file_pattern, and for any found, they are
# added to $files_to_source. They are inserted in the
# beginning because the files found further up the
# tree should be sourced first.

proc collect_tcl_files { dir } {
  variable files_to_source

  set matches [ glob -nocomplain -types { f l } \
                     -directory $dir $::beam::search_file_pattern ]

  set files_to_source [ concat $matches $files_to_source ]
}


# This is the rest of the logic.

if { ! [ ::info exists ::env(beam_source_file) ] } {
  return ;# In driver; nothing to do
}

# Figure out where the source file lives

set current_dir [ file dirname $::env(beam_source_file) ]

# Walk from $current_dir to $search_top_dir

while { ! [ dir_is_top $current_dir ] } {

  collect_tcl_files $current_dir
  set current_dir [ file join $current_dir ".." ]

}

# The loop doesn't execute for when $current_dir is $search_top_dir,
# but we want to also check $search_top_dir, so we have to check one
# more time
  
collect_tcl_files $current_dir  

# Now, source everything in $files_to_source at the
# global scope.

foreach file $files_to_source {
  beam::gsource $file
}

# Done

return 0

# End of namespace
} 
