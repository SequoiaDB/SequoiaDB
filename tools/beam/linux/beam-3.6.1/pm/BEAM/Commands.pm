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
#     Deal with command execution and capture in a cross-platform
#     fashion. These commands also provide safety and correctness
#     by avoiding shell quoting except where explicitly requested.
#
#  MODIFICATIONS:
#
#      Date      UserID   Remark (newest to oldest)
#      --------  -------  ----------------------------------------------------
#      See ChangeLog for recent modifications.

package BEAM::Commands;


#------------------------------------------------------------------------------
# Package stuff
#------------------------------------------------------------------------------

use Exporter;

use vars qw(@ISA @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT_OK = qw( cmd_capture );


#------------------------------------------------------------------------------
# Modules
#------------------------------------------------------------------------------

use 5.004;

use Symbol; # gensym
use POSIX qw(_exit);
use BEAM::Temporary qw(tmp_file);

# Try to find the signal number for SIGINT. On old perls, default to '2'.

use vars qw($SIGINT);

$SIGINT = 2;

BEGIN {
  eval {
    require POSIX;
    import POSIX ();
    $SIGINT = POSIX::SIGINT;
  };
}


#------------------------------------------------------------------------------
# Runs a command given by a list of arguments.
#
# The command and all arguments are passed through without
# additional shell evaluation.
#
# This returns a list of ( $rc, $stdout, $stderr, $signal ).
#
# This raises SIGINT if the command died from SIGINT. Otherwise,
# $signal will be non-zero if the child died from any other signal.
# If $signal is non-zero, $rc is guaranteed to be non-zero as well.
#
# This dies if the command can not be started.
#------------------------------------------------------------------------------

sub cmd_capture
{
  my ( @cmd ) = @_;

  if ( $^O =~ /msys/i || $^O =~ /mswin32/i ) {
    # The Windows way
    return cmd_capture_win( @cmd );
  } else {
    # The Unix way
    return cmd_capture_unix( @cmd );
  }
}

#------------------------------------------------------------------------------
# This is the cmd_capture implementation for Unix systems.
#
# Redirect stdout and stderr to a file, and run the command.
# Return the return code, standard output, standard error,
# and signal, if any.
#
# This would be best done with IPC::Open3, select, and sysread
# on stdout and stderr when data was present, but for some reason
# on certain 2.4 kernels when running an IBM Java process with
# its output hooked up to a pipe, the waitpid() would fail.
# This doesn't happen if we redirect to a file instead of a pipe.
# This was fixed in later kernels but since we frequently run
# IBM Java on 2.4 kernels, we have to work around the bug.
#
# So we fork, redirect stdout and stderr in the child to temp
# files, exec, and let the parent read the files when the child
# finishes.
#------------------------------------------------------------------------------

sub cmd_capture_unix
{
  my ( @cmd ) = @_;
  
  # Create temporary files
  my ( $fout, $foutname ) = tmp_file("stdout");
  my ( $ferr, $ferrname ) = tmp_file("stderr");
  
  # Close the temp handles and use the file names instead.
  # Old perls don't let you dup an open descriptor easily.
  
  close( $fout ); # Ignore rc
  close( $ferr ); # Ignore rc

  # Fork
  
  my $child = fork();
  
  if ( ! defined $child ) {
    die "Can't fork: $!\n";
  }
  
  if ( $child == 0 ) {
    # Child process
    
    # Redirect stdout and stderr to the files, then run the command.
  
    if ( ! open( STDOUT, ">$foutname" ) ) {
      print STDERR "Child: Can't redirect stdout: $!\n";
      _exit(1);
    }
  
    if ( ! open( STDERR, ">$ferrname" ) ) {
      print STDERR "Child: Can't redirect stderr: $!\n";
      _exit(1);
    }
  
    # This exec() syntax with an indirect block
    # object forces Perl to NOT pass the @cmd
    # through '/bin/sh -c'. Perl would have
    # if @cmd had only one element.
  
    { exec( { $cmd[0] } @cmd ); }

    # Error will go to file, parent will read it
    print STDERR "Child: Can't exec $cmd[0]: $!\n";
    _exit(1);
  }
  
  # Parent process
  
  # Wait on the child for the return code, which ends up in $?
  waitpid( $child, 0 );

  my $status = $?;
  
  my $rc = ( $status >> 8 );
  my $sig = ( $status & 127 );

  if ( $sig != 0 && $rc == 0 ) {
    $rc = -1;
  }

  if ( $sig == $SIGINT ) {
    # If the child was interrupted, interrupt ourselves
    kill( $SIGINT, $$ );
  }

  # Read the output files
  
  my $stdout = read_file( $foutname );
  my $stderr = read_file( $ferrname );

  unlink( $foutname );
  unlink( $ferrname );
  
  return ( $rc, $stdout, $stderr, $sig );  
}


#------------------------------------------------------------------------------
# This is the cmd_capture implementation for Windows systems.
#
# Redirect stdout and stderr to a file, and run the command.
# Return the return code, standard output, standard error,
# and signal, if any.
#
# This would be best done with IPC::Open3, select, and sysread
# on stdout and stderr when data was present, but Windows doesn't
# support select on a file handle.
#
# The redirection would be better done in fork()/exec() style
# so the parent's file descriptors weren't played with, but
# Windows doesn't reliably support fork().
#------------------------------------------------------------------------------

sub cmd_capture_win
{
  my ( @cmd ) = @_;

  my ( $fout, $foutname ) = tmp_file("stdout");
  my ( $ferr, $ferrname ) = tmp_file("stderr");
  
  # Close the temp handles and use the file names instead.
  
  close( $fout ); # Ignore rc
  close( $ferr ); # Ignore rc
  
  # Save stdout and stderr for restoring later

  my $origout = 'ORIGOUT'; # Must be strings for ">&$ORIGOUT" below
  my $origerr = 'ORIGERR';
  
  if ( ! open( $origout, '>&STDOUT' ) ) {
    die "Can't save stdout: $!\n";
  }
  
  if ( ! open( $origerr, '>&STDERR' ) ) {
    close( $origout ); # Ignore rc
    die "Can't save stderr: $!\n";
  }
  
  # Redirect stdout and stderr to the files, then run the command.
  
  if ( ! open( STDOUT, ">$foutname" ) ) {
    close( $origout ); # Ignore rc
    close( $origerr ); # Ignore rc
    die "Can't redirect stdout: $!\n";
  }
  
  if ( ! open( STDERR, ">$ferrname" ) ) {
    # Restore STDOUT
    open( STDOUT, ">&$origout" ); # Ignore rc
    close( $origout ); # Ignore rc
    close( $origerr ); # Ignore rc
    die "Can't redirect stderr: $!\n";
  }
  
  # This system() syntax with an indirect block
  # object forces Perl to NOT pass the @cmd
  # through '/bin/sh -c'. Perl would have
  # if @cmd had only one element.
  
  my $sysrc = system( { $cmd[0] } @cmd );

  my $status = $?;
  
  # Restore the original stdout and stderr
  
  open( STDOUT, ">&$origout" ); # Ignore rc
  open( STDERR, ">&$origerr" ); # Ignore rc
  close( $origout ); # Ignore rc
  close( $origerr ); # Ignore rc
  
  if ( $sysrc == -1 ) {
    die "Can't run $cmd[0]: $!\n";
  }
  
  my $rc = ( $status >> 8 );
  my $sig = ( $status & 127 );

  if ( $sig != 0 && $rc == 0 ) {
    $rc = -1;
  }

  if ( $sig == $SIGINT ) {
    # If the child was interrupted, interrupt ourselves
    kill( $SIGINT, $$ );
  }
  
  my $stdout = read_file( $foutname );
  my $stderr = read_file( $ferrname );
  
  unlink( $foutname );
  unlink( $ferrname );

  return ( $rc, $stdout, $stderr, $sig );
}


#------------------------------------------------------------------------------
# Helpers
#------------------------------------------------------------------------------

sub read_file
{
  my ( $file ) = @_;
  
  # Slurp in a whole file at once and return the contents.
  
  my $fh = gensym();
  
  open( $fh, $file ) or die "Can't read '$file': $!\n";
  
  local $/ = undef;
  my $contents = <$fh>;
  
  close( $fh ) or die "Can't close '$file': $!\n";
  
  return $contents;
}
