# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2007-2010 IBM Corporation. All rights reserved.
#
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp
#
# The source code for this program is not published or otherwise divested
# of its trade secrets, irrespective of what has been deposited within
# the U.S. Copyright Office.
#
#  AUTHOR:
#
#     Francis Wallingford
#
#  DESCRIPTION:
#
#     Deal with file names in a cross-platform manner.
#
#     File names are internally handled as "native" file names.
#
#     Native means that in Unix, internal file names are unix-y.
#
#       /they/look/like/this
#
#     On MSYS, internal file names are also unix-y.
#
#     On Windows (ActiveState Perl), internal file names are Windows-y.
#
#       c:/They/look/like/this
#
#     Now, when interacting with external programs and files, the paths
#     used must be in "system" format, or "external" format. For unix,
#     there is no change. For MSYS, paths are changed from "/c/foo" to
#     "C:\foo" because the external format is Windows paths. For ActiveState
#     with Windows paths, C:/foo is changed to C:\foo for external use.
#
#     All of the fn_* routines handle paths names in native format, except
#     for fn_path_to_int() and fn_path_to_ext(), which do conversions.
#
#     Paths are handled this way so that internal paths can be passed to
#     other Perl routines (Cwd.pm, open, etc) and work well. (MSYS's Cwd.pm,
#     for example, doesn't handle Windows paths well). Also, external
#     paths are needed because system utilities run under Windows may not
#     understand MSYS paths at all.
#    
#  MODIFICATIONS:
#
#      Date      UserID   Remark (newest to oldest)
#      --------  -------  ----------------------------------------------------
#      See ChangeLog for recent modifications.

package BEAM::FileNames;


#------------------------------------------------------------------------------
# AFS Hack. Compressed.
#------------------------------------------------------------------------------

BEGIN {
  require Cwd;
  if ( $^O !~ /msys/i && $^O !~ /mswin32/i ) {
    my $alias = sub { my ( $old, $new ) = @_;
                      local *oldproc = $old;
                      if ( defined &oldproc ) {
                        local $SIG{'__WARN__'} = sub { };
                        *oldproc = $new;
                      } };
    $alias->(*Cwd::getcwd,       \&Cwd::cwd);
    $alias->(*Cwd::fastcwd,      \&Cwd::cwd);
    $alias->(*Cwd::fastgetcwd,   \&Cwd::cwd);
    $alias->(*Cwd::abs_path,     \&Cwd::fast_abs_path);
    $alias->(*Cwd::realpath,     \&Cwd::fast_abs_path);
    $alias->(*Cwd::realpath,     \&Cwd::fast_abs_path);
    $alias->(*Cwd::fast_realpath,\&Cwd::fast_abs_path);
    $alias->(*Cwd::fast_realpath,\&Cwd::fast_abs_path);
  }
  import Cwd qw(getcwd abs_path);
};


#------------------------------------------------------------------------------
# Package stuff
#------------------------------------------------------------------------------

use vars qw(@public_vars @public_funcs);

BEGIN {
  @public_vars = qw(
                     $fn_pathsep_re
                     $fn_dirsep_re
                     $fn_no_dirsep_re
                     $fn_javasep_re
                     $fn_drivespec_re
                     $fn_default_temp_dir
                   );

  @public_funcs = qw(
                      fn_set_unix
                      fn_set_windows
                      fn_set_msys
                      fn_set_int
                      fn_set_ext
                      fn_sys_temp_dir
                      fn_full_pathify
                      fn_path_to_int
                      fn_path_to_ext
                      fn_find_existing_dir
                      fn_is_bare
                      fn_is_absolute
                      fn_dirname
                      fn_split
                      fn_join
                    );
};

use Exporter;

use vars qw(@ISA @EXPORT_OK %EXPORT_TAGS);
use vars ( @public_vars );

@ISA = qw(Exporter);

@EXPORT_OK = ( @public_vars, @public_funcs, 'fn_run_tests' );

%EXPORT_TAGS = ( all => [ @public_vars, @public_funcs ] );


#------------------------------------------------------------------------------
# Modules
#------------------------------------------------------------------------------

# Require perl 5.004
use 5.004;

# For mkpath()
use File::Path;


#------------------------------------------------------------------------------
# Define routines that are called during initialization below
#------------------------------------------------------------------------------

sub fn_set_unix {
  $fn_pathsep_re       = ':';
  $fn_dirsep_re        = '/';
  $fn_no_dirsep_re     = '[^/]';
  $fn_javasep_re       = ':';
  $fn_drivespec_re     = undef;
  $fn_default_temp_dir = '/tmp';
}


sub fn_set_windows {
  $fn_pathsep_re       = ';';
  $fn_dirsep_re        = '[\\\\/]';   # string: [\\/], when in re: [\/]
  $fn_no_dirsep_re     = '[^\\\\/]';  # string: [^\\/], when in re: [^\/]
  $fn_javasep_re       = ';';
  $fn_drivespec_re     = '[A-Za-z]:';
  $fn_default_temp_dir = 'c:/temp';
}


sub fn_set_msys {
  $fn_pathsep_re       = ':';
  $fn_dirsep_re        = '/';
  $fn_no_dirsep_re     = '[^/]';
  $fn_javasep_re       = ';';
  $fn_drivespec_re     = undef;
  $fn_default_temp_dir = '/tmp';
}

sub fn_set_int {
  if ( $^O =~ /mswin32/i ) {
    fn_set_windows();
  } elsif ( $^O =~ /msys/i ) {
    fn_set_msys();
  } else {
    fn_set_unix();
  }
}

sub fn_set_ext {
  if ( $^O =~ /mswin32/i ) {
    fn_set_windows();
  } elsif ( $^O =~ /msys/i ) {
    fn_set_windows();
  } else {
    fn_set_unix();
  }
}

#------------------------------------------------------------------------------
# Initialize our module
#------------------------------------------------------------------------------

BEGIN {
  fn_set_int();
};


#------------------------------------------------------------------------------
# Exported routines
#------------------------------------------------------------------------------

sub fn_sys_temp_dir
{
  #----------------------------------------------------------------------------
  # Return the temporary directory root for this system.
  # This checks a few environment variables like TMPDIR and TMP,
  # choosing the first that is set and exists on disk.
  # If nothing is found, the fn default is created and returned.
  #----------------------------------------------------------------------------

  # Thanks, Perl. The "foreach" can't directly iterate over ($ENV{}, $ENV{})
  # because a foreach loop actually aliases $try to each list item, and aliasing
  # to non-existent hash keys actually autovivifies the hash key with an
  # undefined value. This causes problems later because this means that
  # there will actually be a TMPDIR and TMP environment variable from now on
  # with an undefined value (the empty string for children). It's ridiculous
  # that iterating over hash elements like this actually creates them with
  # empty values (instead of leaving them out of the hash), but that's Perl.
  my @tries = ( $ENV{'TMPDIR'}, $ENV{'TMP'}, $fn_default_temp_dir );
  foreach my $try ( @tries )
  {
    return fn_full_pathify($try) if (defined $try && -d $try);
  }

  # No go. Make a new temp directory and return that.
  mkpath( $fn_default_temp_dir );

  return $fn_default_temp_dir;
}

  
sub fn_full_pathify {
  my ( $path ) = @_;

  #----------------------------------------------------------------------------
  # Make $path an absolute path if it is relative to '.'.  This tries hard to 
  # work even if $path doesn't exist yet.
  #----------------------------------------------------------------------------

  if ( ! fn_is_absolute( $path ) ) {
    $path = getcwd() . "/$path";
  }

  # Note: realpath() and abs_path() don't work as they should.
  # On Linux, they are aliases and work on files and directories.
  # On MSYS, realpath() fails in odd ways with Windows paths or with things
  #   in /tmp.
  # On AIX and MSYS and I'm sure other systems, abs_path() only works with 
  #   directories. So, we use that everywhere, but on directories only.
  # For the file itself, we use "readlink" until it is resolved.

  # First resolve file links
  while ( -f $path && -l $path ) {
    my $dir = fn_dirname( $path );
    
    $path = readlink( $path );
    
    if ( ! fn_is_absolute( $path ) ) {
      # A relative link is relative to where link lived
      if ( $dir eq "/" ) {
        $path = $dir . $path;
      } else {
        $path = $dir . "/" . $path;
      }
    }
  }

  # Now try to abs_path() the first existing dir we find
  my ( $dir, $rest ) = fn_find_existing_dir( $path );
  $dir = abs_path( $dir );
  $path = $dir;
    
  if ( $rest ne "" ) {
    $path = $path . "/" . $rest;
  }
  
  return $path;
}


sub fn_path_to_int {
  my ( $path ) = @_;
  
  #----------------------------------------------------------------------------  
  # Clean up $path for internal usage.
  # - If we are on Windows, convert all \ to / for consistency
  # - If we are on MSYS, convert all C:/Foo to /c/bar/baz
  # - Make it a full path
  #
  # After this routine, the path is in native format, and all fn_* routines 
  # should work with it.
  #
  # This is used when paths are provided by external applications that don't 
  # use native paths. The only known case of this is when running Windows 
  # applications from MSYS perl. The MSYS perl environment expects paths to be
  # in unix-y format and some utils (like Cwd.pm) have trouble if they are not.
  # It is also convenient to treat all paths in MSYS as unix-y paths in all of
  # the fn_* utilities.
  #
  # On other platforms, this is similar to abs_path().
  #----------------------------------------------------------------------------

  if ( $^O =~ /mswin32/i || $^O =~ /msys/i ) {
    $path =~ s|\\|/|g;
  }
  
  if ( $^O =~ /msys/i ) {
    # Fix C:/Bar => /c/foo/bar
    # MSYS has "/bin/pwd" that converts directories for us.
    # Walk up $path until a directory that exists is found.
    # Convert that directory, then add the rest of the path
    # back on.
    
    # On MSYS, external paths are assumed to be Win32 paths.
    # Temporarily set win mode in the fn utils so that
    # fn_split and fn_join work. At the end, set the fn
    # mode back to msys.

    fn_set_windows();

    my ( $dir, $rest ) = fn_find_existing_dir( $path );

    my $msys_dir = qx(cd "$dir"; /bin/pwd);
        
    if ( ! defined $msys_dir ) {
      print STDERR "Warning: Can't run pwd on MSYS: $!. Path not converted.\n";
    } else {
      chomp( $msys_dir );
        
      $path = $msys_dir;
        
      if ( $rest ne "" ) {
        $path = $path . "/" . $rest;
      }
    }
    
    fn_set_msys();
  }
  
  $path = fn_full_pathify( $path );
  
  return $path;
}


sub fn_path_to_ext {
  my ( $path ) = @_;
  
  #----------------------------------------------------------------------------
  # Clean up $path for external usage.
  # - Make it a full path
  # - If we are on MSYS, convert all "/c/foo/bar" to "c:/baz" and make it 
  #   lowercase
  # - If we are on Windows, convert all / to \ and make it lowercase
  #
  # After this routine, the path is no longer in native system format. The 
  # other fn_* routines will not work unless the proper fn_set_* is used first.
  #
  # For example, on msys, paths are of the form /c/foo/bar, and the fn_* 
  # routines all recognize that. After this routine is used, the resulting path
  # of c:\baz\bar will not be recognized by the other fn_* routines unless 
  # fn_set_windows() is called first.
  #----------------------------------------------------------------------------
  
  $path = fn_full_pathify( $path );
  
  if ( $^O =~ /msys/i ) {
    # Fix /c/foo/bar => C:/Bar.
    # MSYS has "/bin/pwd -W" that converts directories for us.
    # Walk up $path until a directory that exists is found.
    # Convert that directory, then add the rest of the path
    # back on.

    my ( $dir, $rest ) = fn_find_existing_dir( $path );
    
    my $win_dir = qx(cd "$dir"; /bin/pwd -W);
        
    if ( ! defined $win_dir ) {
      print STDERR "Warning: Can't run pwd on MSYS: $!. Path not converted.\n";
    } else {
      chomp( $win_dir );        
        
      $path = $win_dir;
        
      if ( $rest ne "" ) {
        $path = $path . "/" . $rest;
      }
    }
    
    # Make path lowercase
    $path = lc( $path );
  }
  
  if ( $^O =~ /mswin32/i || $^O =~ /msys/i ) {
    $path =~ s|/|\\|g;
    
    # Make path lowercase
    $path = lc( $path );
  }
  
  return $path;
}


sub fn_find_existing_dir {
  my( $path ) = @_;

  #----------------------------------------------------------------------------  
  # Split the path into a portion that exists as a directory and a portion that
  # follows it. The returned values can be joined together with a path 
  # separator to get a path equivalent to the original.
  #
  #   my( $dir, $rest ) = fn_find_existing_dir( $path );
  #
  # $dir will be a non-empty, existing directory. It may be ".". $rest may or 
  # may not be empty. If it is empty, $path was also an existing directory.
  #----------------------------------------------------------------------------
  
  my ( $dir, $rest );
  
  my ( $drive, @list ) = fn_split( $path );
    
  # If $path was not absolute, prepend "." to @list
    
  if ( scalar( @list ) > 0 ) {
    if ( ! $list[0]->{'is_root'} ) {
      unshift @list, { name => "." };
    }
  }
    
  my $end = scalar( @list );
    
  for ( my $i = $end; $i > 0; $i-- ) {
    # @list[0 .. $i - 1] is a list of what we're testing as a directory
    # @list[$i .. $end - 1] is a list of what we're adding back on
      
    my @first = @list[0 .. $i - 1];
    my @last  = @list[$i .. $end - 1];
      
    $dir = fn_join( $drive, @first );
      
    if ( -d $dir ) {
      # We found a prefix of $path that is an existing directory.
      $rest = fn_join( "", @last );
      last;
    }
      
    # If we get to the end of the last loop and no path was found, print a 
    # warning. At least the first entry in the path should have worked.
      
    if ( $i == 1 ) {
      print STDERR "Warning: No existing entries on path. This should not be.\n";
      ( $dir, $rest ) = ( $path, "" );
    }
  }
  
  return ( $dir, $rest );
}


sub fn_is_bare {
  my( $name ) = @_;

  #----------------------------------------------------------------------------  
  # Return true if the file name is a bare name with no directory parts and no
  # drive specs. Note that "." and ".." are not bare names. Return false if the
  # name is not a bare name.
  #----------------------------------------------------------------------------
  
  if ( $name eq "." || $name eq ".." ) {
    return 0;
  }
  
  if ( $name =~ /$fn_dirsep_re/ ) {
    return 0;
  }
  
  if ( defined $fn_drivespec_re ) {
    if ( $name =~ /^$fn_drivespec_re/ ) {
      return 0;
    }
  }
  
  return 1;
}


sub fn_is_absolute {
  my( $name ) = @_;
  
  #----------------------------------------------------------------------------
  # Return true if the file name is an absolute file name, and false otherwise.
  #
  # Note that "C:foo" is a relative name, and "C:/foo" is absolute.
  #----------------------------------------------------------------------------
  
  if ( defined $fn_drivespec_re ) {
    # Strip off the drive spec
    $name =~ s/^$fn_drivespec_re//;
  }
  
  if ( $name =~ /^$fn_dirsep_re/ ) {
    return 1;
  }
  
  return 0;
}


sub fn_dirname {
  my( $name ) = @_;

  #----------------------------------------------------------------------------  
  # Return the parent directory of the file name.
  #
  # If the file name is a bare name, "." is returned.
  # If the file name is the root directory, it is returned as-is.
  # If the file name is empty, ".." is returned.
  #
  # If the last component of the name is "." or "..", "/.." is appended to name
  # and that is returned.
  #
  # If the file name ends in one or more path separators, they are stripped off 
  # and not counted when looking for the last directory.
  #
  # Examples:
  #
  #    dirname of (empty)      => ..
  #    dirname of .            => ./..
  #    dirname of ..           => ../..
  #    dirname of ./           => ./..
  #    dirname of ../          => ../..
  #    dirname of anything/.   => anything/./..
  #    dirname of anything/..  => anything/../..
  #    dirname of anything/./  => anything/./..
  #    dirname of anything/../ => anything/../..
  #    dirname of /            => /
  #    dirname of ///          => ///
  #    dirname of c:/          => c:/   (windows) or . (unix)
  #    dirname of c:///        => c:/// (windows) or . (unix)
  #    dirname of bare         => .
  #    dirname of bare/        => .
  #    dirname of foo/bar///   => foo
  #    dirname of /foo/bar     => /foo
  #    dirname of c:///foo     => c:///   (windows) or c: (unix)
  #    dirname of c:///foo/    => c:///   (windows) or c: (unix)
  #    dirname of c:           => c:..    (windows) or .  (unix)
  #    dirname of c:foo        => c:      (windows) or .  (unix)
  #    dirname of c:..         => c:../.. (windows) or .  (unix)
  #    dirname of c:.          => c:./..  (windows) or .  (unix)
  #    dirname of c:foo/bar    => c:foo   (both)
  #    dirname of /c:          => /       (both)
  #    dirname of /c:/foo      => /c:     (both)
  #    dirname of c\d\e        => c\d     (windows) or .  (unix)
  #    dirname of c/d\e        => c/d     (windows) or c  (unix)
  #
  # Note: "c:foo" is a relative path that specifies "foo" relative to the 
  #       current working directory of drive "c:". Windows maintains a current
  #       working directory for each drive. A spec is only absolute if it has
  #       a "/" or "\" on the front (ignoring the drivespec).
  #
  # Note: Invalid paths (like "/c:" on windows) are not diagnosed. They are 
  #       treated as if they were legal. You must diagnose them yourself.
  #----------------------------------------------------------------------------

  # Split the path into components.
  
  my ( $drive, @path ) = fn_split( $name );
  
  # If there are no components, return "..".
  
  if ( scalar( @path ) == 0 ) {
    return $drive . ".."
  }
  
  # If the last component is "." or "..", append ".." and return
  
  my $last = $path[-1];
  
  if ( $last->{'name'} eq "." || $last->{'name'} eq ".." ) {
    push @path, { name => ".." };
    return fn_join( $drive, @path );
  }
  
  # If there is only one component, it is either the root
  # (so return it) or a bare name (so return ".")
  
  if ( scalar( @path ) == 1 ) {
    my $only = $path[0];
    
    if ( $only->{'is_root'} ) {
      return $name;
    } else {
      return $drive . ".";
    }
  }
  
  # There is more than one item. Strip off the last one and return
  # the new path.

  pop @path;
  
  return fn_join( $drive, @path );
}


sub fn_split {
  my( $name ) = @_;
  
  #----------------------------------------------------------------------------
  # Take a relative or absolute path and split it into a drive spec and a list
  # of path components. Each component is a hash ref that has one or more of
  # the following keys:
  #
  #   name    => the name of the directory or "" for root
  #   sep     => one or more path separators that follow name
  #   is_root => true if this component is the root dir
  #
  # The root component has at least one character in 'sep'.
  #----------------------------------------------------------------------------

  my $drive = "";
  my @path;
  
  if ( defined $fn_drivespec_re ) {
    if ( $name =~ s/^($fn_drivespec_re)// ) {
      $drive = $1;
    }
  }
  
  if ( $name =~ s/^((?:$fn_dirsep_re)+)// ) {
    push @path, { name => "", sep => $1, is_root => 1 };
  }

  # Eat up components, one by one, with any following
  # path separators

  while ( $name =~ s/^((?:$fn_no_dirsep_re)+)((?:$fn_dirsep_re)*)// ) {
    push @path, { name => $1, sep => $2 };
  }
  
  # Nothing should be left
  
  if ( $name ne "" ) {
    print STDERR "Warning: Unexpected path component after parsing\n";
  }
  
  return ( $drive, @path );
}


sub fn_join {
  my( $drive, @path ) = @_;

  #----------------------------------------------------------------------------
  # Return a string path created from concatenating all of the path components
  # in the list to the end of the drive spec. Components with an empty 'spec'
  # that are not at the end of the list will automatically have one path 
  # separator placed after them. Note that no path separator will appear at the
  # end of the resulting path unless the resulting path is the root path.
  #----------------------------------------------------------------------------
  
  my $name = "";
  
  my $end = scalar( @path );
  
  for ( my $i = 0; $i < $end; $i++ ) {
    my $comp = $path[$i];
    
    $name .= $comp->{'name'};

    # Add on a separator if one is needed. One is need if
    # this is the root component, or if there are more
    # entries in the path.

    my $sep = "/";
    
    # Use the component's separator if one exists
    
    if ( defined $comp->{'sep'} && $comp->{'sep'} ne "" ) {
      $sep = $comp->{'sep'};
    }
    
    # Add it if needed
    
    if ( $i + 1 != $end || $comp->{'is_root'} ) {
      $name .= $sep;
    }
  }
  
  return $drive . $name;
}


sub fn_run_tests {
  my $bare_tests = [
    # String, Unix, Windows
    [ "", 1, 1 ],
    [ "f", 1, 1 ],
    [ "foo", 1, 1 ],
    [ "c:\\", 1, 0 ],
    [ "\\foo\\bar", 1, 0 ],
    [ "/foo/bar", 0, 0 ],
    [ "\\foo/", 0, 0 ],
    [ "d:", 1, 0 ],
    [ "q:blit", 1, 0 ],
    [ ".", 0, 0 ],
    [ "..", 0, 0 ],
  ];

  my $abs_tests = [
    # String, Unix, Windows
    [ "", 0, 0 ],
    [ "z:", 0, 0 ],
    [ "z:/", 0, 1 ],
    [ "z:\\", 0, 1 ],
    [ "/", 1, 1 ],
    [ "\\", 0, 1 ],
    [ "foo/bar", 0, 0 ],
    [ "foo\\bar", 0, 0 ],
    [ "\\foo\\bar", 0, 1 ],
    [ "/foo/bar", 1, 1 ],
    [ "/c:", 1, 1 ],
  ];

  my $dirname_tests = [
    # String, Unix, Windows
    [ "", "..", ".." ],
    [ ".", "./..", "./.." ],
    [ "..", "../..", "../.." ],
    [ "./", "./..", "./.." ],
    [ "../", "../..", "../.." ],
    [ "anything/.", "anything/./..", "anything/./.." ],
    [ "anything/..", "anything/../..", "anything/../.." ],
    [ "anything/./", "anything/./..", "anything/./.." ],
    [ "anything/../", "anything/../..", "anything/../.." ],
    [ "/", "/", "/" ],
    [ "///", "///", "///" ],
    [ "c:/", ".", "c:/" ],
    [ "c:///", ".", "c:///" ],
    [ "bare", ".", "." ],
    [ "bare/", ".", "." ],
    [ "foo/bar///", "foo", "foo" ],
    [ "/foo/bar", "/foo", "/foo" ],
    [ "c:///foo", "c:", "c:///" ],
    [ "c:///foo/", "c:", "c:///" ],
    [ "c:", ".", "c:.." ],
    [ "c:foo", ".", "c:." ],
    [ "c:..", ".", "c:../.." ],
    [ "c:.", ".", "c:./.." ],
    [ "c:foo/bar", "c:foo", "c:foo" ],
    [ "/c:", "/", "/" ],
    [ "/c:/foo", "/c:", "/c:" ],
    [ "c\\d\\e", ".", "c\\d" ],
    [ "c/d\\e", "c", "c/d" ],
  ];

  my $rc = 0;
  
  $rc += fn_run_test( "is_bare", $bare_tests, sub { fn_is_bare( $_[0] ) } );
  $rc += fn_run_test( "is_abs",  $abs_tests, sub { fn_is_absolute( $_[0] ) } );
  $rc += fn_run_test( "dirname", $dirname_tests, sub { fn_dirname( $_[0] ) } );
  
  print "$rc tests failed\n";
  
  return $rc;
}


#------------------------------------------------------------------------------
# Utility routines for internal use only
#------------------------------------------------------------------------------

sub fn_run_test {
  my( $name, $tests, $sub ) = @_;

  #----------------------------------------------------------------------------  
  # For each test, call the sub once with unix path settings and once with 
  # windows path settings. Each test is a list that has the argument, the unix
  # results, and the windows results. If any fail, print a message, Return the 
  # number of failures.
  #----------------------------------------------------------------------------

  my $failures = 0;
  
  foreach my $test ( @$tests ) {
    my $arg = $test->[0];
    my $lin = $test->[1];
    my $win = $test->[2];
    
    my $res;
    
    # Unix test

    fn_set_unix();
    
    $res = $sub->($arg);
    
    if ( $res ne $lin ) {
      print STDERR "** Failure (unix): $name: '$arg', expected '$lin', got '$res'\n";
      $failures++;
    }
    
    # Windows test
  
    fn_set_windows();
    
    $res = $sub->($arg);
  
    if ( $res ne $win ) {
      print STDERR "** Failure (windows): $name: '$arg', expected '$win', got '$res'\n";
      $failures++;
    }
  }
  
  return $failures;
}


# Always last in the file
1;
