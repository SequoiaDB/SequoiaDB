#!/usr/bin/env perl
######################################################################
#
# This script can be used with bin/beam_trace to trace
# builds automatically and run BEAM on your source code
# without changing your build at all.
#
# To get started, copy this script to a location that is
# accessible from your build, and edit the settings at
# the top of the file.
#
# For details, see
#
#   https://w3.eda.ibm.com/beam/beam_trace.html
#
# or your local BEAM documentation:
#
#   doc/beam_trace.html
#
######################################################################

#
# FOLLOW ALONG AND SET THESE VARIABLES!
#

# You ought to change "#!/usr/bin/env perl" at the top
# of this file to the actual path to perl (like "#!/usr/bin/perl").
#
# This will make the script more rubust if $PATH is modified during
# the build at all.


# Set this to the full path to 'beam_compile'.

my $BEAM_COMPILE = '/opt/beam-x.y.z/bin/beam_compile';

# Set this to the output file where you want complaints
# to be written. This script can not write STDOUT, so
# you must specify a file or you won't get any BEAM
# output.

my $BEAM_COMPLAINTS = '/tmp/beam-complaints.txt';

# Set these to your compiler configuration files. You
# should have a compiler configuration file for each
# language you are running BEAM over (C, C++, Java).
#
# You only need to include config files here for the
# acutal languages your build will encounter. You
# can remove those that don't apply.

my @C_CONFIG = ('/opt/config/c_config.tcl');
my @CPP_CONFIG = ('/opt/config/cpp_config.tcl');
my @JAVA_CONFIG = ('/opt/config/java_config.tcl');

# Set this to any additional BEAM options you wish to
# pass to each invocation of BEAM. These should all
# be "--beam::..." or "--edg=..." options.
#
# You should add your parms file here.
#
# Example:
#
#   my @BEAM_OPTS = ('--beam::parser_file=/tmp/beam-parser.txt',
#                    '--beam::source=my_parms.tcl',
#                    '--edg=-w');

my @BEAM_OPTS = ();

# If you want to pass --beam::* options to beam_trace itself,
# and use them here, they are in the BEAM_TRACE_OPTS environment
# variable, and they are encoded as a JSON list (see json.org).
#
# To pass them through, install a JSON parser, and use it to
# turn the JSON string into a Perl list. It will contain the
# same --beam::* options that you passed to beam_trace.
#
# Example:
#
#   use JSON;
#   push @BEAM_OPTS, @{ from_json( $ENV{'BEAM_TRACE_OPTS'} ) };

# Set this to be a regular expression that will match
# your compiler.
#
# This default matches 'cc', 'c89', 'gcc', 'g++', and 'javac'.

my $compile_re = qr/\b(cc|c89|gcc|g\+\+|javac)$/;

# Set this to be a regular expression that will match
# any C or C++ source file.
#
# This default matches "c", "cc", "cpp", and "cxx", in
# upper or lower case.

my $c_source_re = qr/\.(c|cc|cpp|cxx)$/i;

# Set this to be a regular expression that will match
# any Java source file.
#
# This default matches "java", in upper or lower case.

my $java_source_re = qr/\.java$/i;

# End of options

######################################################################
# Don't edit anything below this line unless you want to change
# the fundamentals of how this trace script works.
######################################################################

die "Can't find $BEAM_COMPILE: $!\n" unless ( -f $BEAM_COMPILE );

# Check @ARGV for a known binary. If we see one, run BEAM. Otherwise,
# don't.

my $cmd = shift @ARGV;

if ( defined $cmd && $cmd =~ $compile_re ) {
  # Make sure there are source files
  if ( grep { /$c_source_re/ } @ARGV ) {
    # Run BEAM on C or C++ code
    my @flags = ( "--beam::complaint_file=$BEAM_COMPLAINTS",
                  map( { "--beam::compiler=$_" } @C_CONFIG, @CPP_CONFIG ),
                  @BEAM_OPTS );
    
    system( $BEAM_COMPILE, @flags, @ARGV ) == 0
      or die "Can't run $BEAM_COMPILE (rc = $?)\n";
  } elsif ( grep { /$java_source_re/ } @ARGV ) {
    # Run BEAM on Java code
    my @flags = ( "--beam::complaint_file=$BEAM_COMPLAINTS",
                  map( { "--beam::compiler=$_" } @JAVA_CONFIG ),
                  @BEAM_OPTS );
    
    system( $BEAM_COMPILE, @flags, @ARGV ) == 0
      or die "Warning: $BEAM_COMPILE failed: rc = $?\n";
  }
}

exit 0;
