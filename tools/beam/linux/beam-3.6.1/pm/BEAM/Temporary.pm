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
#     Deal with temporary files and directories easily.
#
#     All temp files and directories created via this
#     module are cleaned up when the process exits.
#
#     This includes calling 'die', 'exit', or exiting
#     from any signal that interrupts the process.
#
#     To keep the files around for debugging, set
#     $BEAM::Temporary::do_cleanup to 0 before
#     the process exits.
#    
#  MODIFICATIONS:
#
#      Date      UserID   Remark (newest to oldest)
#      --------  -------  ----------------------------------------------------
#      See ChangeLog for recent modifications.

package BEAM::Temporary;


#------------------------------------------------------------------------------
# Package stuff
#------------------------------------------------------------------------------

use Exporter;

use vars qw(@ISA @EXPORT_OK);

@ISA = qw(Exporter);

@EXPORT_OK = qw( tmp_file tmp_file_in tmp_dir );


#------------------------------------------------------------------------------
# Modules
#------------------------------------------------------------------------------

use 5.004;

use Symbol; # gensym()
use Fcntl qw(:DEFAULT); # O_WRONLY, etc

use BEAM::FileNames qw( fn_sys_temp_dir );


#------------------------------------------------------------------------------
# Globals
#------------------------------------------------------------------------------

use vars qw($do_cleanup @to_cleanup);

# Set BEAM::Temporary::do_cleanup to 0 before exit time to
# suppress the removal of temporary files. The default is 1,
# which enables cleanup processing at exit time.

BEGIN {
  $do_cleanup = 1;
}


#------------------------------------------------------------------------------
# Routines
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Create and return an open file handle and the name of a temporary
# file. This file will be deleted when the process exits unless
# BEAM::Temporary::do_cleanup is set to 0.
#
# The file will exist in the system temp directory, as defined
# by fn_sys_temp_dir().
#
# The extension is optional, and defaults to no extension. If one
# is given, a period and the given extension will be appended to the
# temporary file name.
#------------------------------------------------------------------------------

sub tmp_file
{
  my ( $ext ) = @_;

  return tmp_file_in( fn_sys_temp_dir(), $ext );
}

#------------------------------------------------------------------------------
# This is just like tmp_file(), but the file will be created in the directory
# given. The extension is optional.
#------------------------------------------------------------------------------

sub tmp_file_in
{
  my ( $dir, $ext ) = @_;

  if ( defined $ext ) {
    $ext = ".$ext";
  } else {
    $ext = "";
  }

  while (1) {
    my $i = int(rand 1000000);

    my $tmpnam = "$dir/temp.$<.$i$ext";
    my $fh = gensym();

    if ( sysopen( $fh, $tmpnam, O_WRONLY|O_CREAT|O_EXCL, 0600 ) ) {
      # Save this file for unlinking and return it
      push @to_cleanup, $tmpnam;
      return ( $fh, $tmpnam );
    } else {
      # Opening failed - if it was because of EEXIST, continue trying
      next if ( defined( %! ) && $!{'EEXIST'} );
      next if ( $! =~ /file exists/i ); # Old perls don't have %!

      die "Error: Can't create temporary file '$tmpnam': $!\n";
    }
  }
  
  # Not reached
  return undef;
}


#------------------------------------------------------------------------------
# Create and return the name of a temporary directory. This directory
# and all of its contents will be deleted when the process exits unless
# BEAM::Temporary::do_cleanup is set to 0.
#------------------------------------------------------------------------------

sub tmp_dir
{
  my $tmproot = fn_sys_temp_dir();

  while (1) {
    my $tmpdir = $tmproot . "/beam_temp.$$." . int(rand 1000000);
    
    if ( mkdir( $tmpdir, 0700 ) ) {
      # We are good. Save the directory name and return it.
      push @to_cleanup, $tmpdir;
      return $tmpdir;
    } else {
      # Mkdir failed... if it was because of EEXIST, keep trying
      next if ( defined( %! ) && $!{'EEXIST'} );
      next if ( $! =~ /file exists/i ); # Old perls don't have %!

      die "Error: Can't create temporary directory '$tmpdir': $!\n";
    }
  }
  
  # Not reached
  return undef;
}


#------------------------------------------------------------------------------
# Helpers
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# This cleans up the temporary files at exit time.
#------------------------------------------------------------------------------

sub dirlist
{
  my ( $dir ) = @_;
  
  # Return a list of files in a directory.
  # Filter out "." and "..".
  
  my $dh = gensym();
  
  opendir( $dh, $dir ) or die "Can't read '$dir': $!\n";
  
  my @ls = readdir( $dh );
  
  closedir( $dh ) or die "Can't read '$dir': $!\n";
  
  @ls = grep { $_ ne "." && $_ ne ".." } @ls;
  
  return @ls;
}

sub nuke
{
  my ( $target ) = @_;
  
  # Remove the file or recursively remove the directory.
  # Warn on things that can't be removed.

  if ( -d $target ) {
    foreach my $file ( dirlist( $target ) ) {
      nuke( "$target/$file" );
    }
    
    if ( ! rmdir( $target ) ) {
      print STDERR "Warning: Couldn't delete temporary directory '$target': $!\n";
      print STDERR "         Please delete it manually.\n";
    }
  } elsif ( -e $target ) {
    if ( unlink( $target ) != 1 ) {
      print STDERR "Warning: Couldn't delete temporary file '$target': $!\n";
      print STDERR "         Please delete it manually.\n";
    }
  }
}

sub tmp_cleanup
{
  # Change directories so we can clean up our temp dir, which is pwd...
  chdir( "/" );
  
  if ( $do_cleanup ) {
    foreach my $f ( @to_cleanup ) {
      nuke( $f );
    }
  } else {
    print STDERR "The following temporary files and directories have been kept:\n";
    foreach my $f ( @to_cleanup ) {
      print STDERR "- $f\n";
    }
  }
  
  @to_cleanup = (); # In case it is called again
}

my $orig_pid = $$;

sub interrupted
{
  my ( $sig ) = @_;
  
  # Set them all to ignore in case someone hits one
  # while a separate one is in here working
  $SIG{'INT'} = 'IGNORE';
  $SIG{'TERM'} = 'IGNORE';
  $SIG{'QUIT'} = 'IGNORE';
  $SIG{'HUP'} = 'IGNORE' if ( defined $SIG{'HUP'} );

  # In parent only - if we forked, we don't want the child doing this
  if ( $$ == $orig_pid ) {
    tmp_cleanup();
  }
  
  # Re-raise
  $SIG{$sig} = 'DEFAULT';
  kill( $sig, $$ );
}

$SIG{'INT'} = \&interrupted;
$SIG{'TERM'} = \&interrupted;
$SIG{'QUIT'} = \&interrupted;
$SIG{'HUP'} = \&interrupted if ( defined $SIG{'HUP'} );

END {
  # Set the signals to ignore so that during the cleanup,
  # they won't fire and try to re-enter the cleanup, during
  # the rare situation that someone hits Ctrl-C right at the end
  # here...
  $SIG{'INT'} = 'IGNORE';
  $SIG{'TERM'} = 'IGNORE';
  $SIG{'QUIT'} = 'IGNORE';
  $SIG{'HUP'} = 'IGNORE' if ( defined $SIG{'HUP'} );

  # In parent only - if we forked, we don't want the child doing this
  if ( $$ == $orig_pid ) {
    tmp_cleanup();
  }
}


#------------------------------------------------------------------------------
# Testing
#------------------------------------------------------------------------------

sub tmp_test
{
  # Create some temp files and temp dirs.

  my ( $f1, $n1 ) = tmp_file();
  close( $f1 );
  
  die "No $n1\n" unless ( -f $n1 );
  
  my ( $f2, $n2 ) = tmp_file('ext');
  close( $f2 );
  
  die "No $n2\n" unless ( -f $n2 );
  die "Bad extension on $n2\n" unless ( $n2 =~ /\.ext$/ );
  
  my $dir = tmp_dir();
  
  my ( $f3, $n3 ) = tmp_file_in($dir);
  close( $f3 );

  die "No $n3\n" unless ( -f $n3 );
  die "Bad location for $n3\n" unless ( $n3 =~ /^\Q$dir\E/ );
  
  my ( $f4, $n4 ) = tmp_file_in($dir, 'foo');
  close( $f4 );

  die "No $n4\n" unless ( -f $n4 );
  die "Bad extension on $n4\n" unless ( $n4 =~ /\.foo$/ );
  die "Bad location for $n4\n" unless ( $n4 =~ /^\Q$dir\E/ );
  
  # Put a directory in $dir as well, with files inside
  mkdir( "$dir/test", 0777 ) or die "Can't create '$dir/test': $!\n";
  if ( ! open( WRITE, ">$dir/test/file" ) ) {
    die "Can't write '$dir/test/file': $!\n";
  }
  print WRITE "Testing\n";
  close( WRITE ) or die "Can't write '$dir/test/file: $!\n";
  
  # Clean up everything
  tmp_cleanup();
  
  # Check the files to ensure cleanup happened
  foreach my $thing ( $n1, $n2, $n3, $n4, $dir ) {
    die "Not cleaned: $thing\n" if ( -e $thing );
  }
}


# Always last.
1;
