############################################################
# Invocation of beam_configure:
#
#   Template Configuration File
#
# Location of compiler:
#
#   Template Configuration File
#
############################################################
#
# This is BEAM configuration file that describes a Java
# compilation environment. This was generated with beam_configure
# version "4.0".
#
# This information will help BEAM emulate this compiler's
# Java language level, default classpath, and encoding
# so that BEAM can compile the same source code that the
# original compiler could compile.
#
# The file format is Tcl, so basic Tcl knowledge may be beneficial
# for anything more than the simplest of modifications.
# 
# A quick Tcl primer:
# - Lines starting with "#" or ";#" are comments
# - Things inside balanced curly braces are literal strings {one string literal}
# - Things in square brackets that aren't in curly braces are function calls,
#   and will be expanded inline automatically. This causes the most problems in
#   double-quoted strings: "this is a function call: [some_func]"
#
# This file contains these sections:
#
# 1) Source language dialect
# 2) Default Endorsed Directories
# 3) Default Boot Classpath
# 4) Default Extension Directories
# 5) Default Classpath
#
# Each section has variables that help configure BEAM. They should
# each be commented well. For additional documentation, please
# refer to the local documentation in the install point.
#
# Note that the order of the sections is not important,
# and variables may be set in any order.
#
# All variables are prefixed with name that corresponds to
# which language this configuration is for.
#
# For Java compilers, the prefix is "beam::compiler::java"
#
############################################################

### This is the version of beam_configure that generated this
### configuration file.
  


### This tells BEAM which pre-canned settings to load.
### BEAM comes with some function attributes and argument
### mappers for Sun Javac and IBM Javac.
###
### If unsure, set to 'default'.

# set beam::compiler::java::cc "sun_javac"
# set beam::compiler::java::cc "ibm_javac"

set beam::compiler::java::cc "default"



############################################################
# Section 1: Source language dialect
############################################################
  
### The language_source_level variable selects among the available
### dialects of Java.

# set beam::compiler::java::language_source_level "1.3"
# set beam::compiler::java::language_source_level "1.4"
# set beam::compiler::java::language_source_level "1.5"

set beam::compiler::java::language_source_level 1.4



### The encoding of the Java source files. This should be
### the same as the "file.encoding" System property of
### your Java Virtual Machine when run with no options.

# set beam::compiler::java::language_encoding {ANSI_X3.4-1968}



  
############################################################
# Section 2: Default Endorsed Directories
############################################################

### The system_endorsed_dirs variable is a list of entries
### that will be used for the initial endorsed path. The
### endorsed path is a list of directories where Java
### Endorsed Standards live. Any jar file or zip file
### in these directories will be placed in the classpath
### before the boot classpath entries.
###
### This should be the same as the "java.endorsed.dirs"
### System property of your Java Virtual Machine when run
### with no cross-compilation options.

# lappend beam::compiler::java::system_endorsed_dirs {/path/to/jre/lib/endorsed}




############################################################
# Section 3: Default Boot Classpath
############################################################

### The system_boot_classpath variable is a list of entries
### that will be used for the initial boot classpath. These
### entries normally contain jar files that come with the
### java compiler itself, and contain the basic Java classes
### like java.lang.Object.
###
### This should be the same as the "sun.boot.class.path"
### System property of your Java Virtual Machine when
### run with no cross-compilation options.
  
# lappend beam::compiler::java::system_boot_classpath {/path/to/jre/lib/rt.jar}
# lappend beam::compiler::java::system_boot_classpath {/path/to/jre/lib/i18n.jar}
# lappend beam::compiler::java::system_boot_classpath {/path/to/jre/lib/jce.jar}




############################################################
# Section 4: Default Extension Directories
############################################################

### The system_ext_dirs variable is a list of entries that will
### be used for the initial extension path. The extension
### path is a list of directories where Java Extensions live.
### Any jar file or zip file in these directories will be placed
### in the classpath before the user directories.
###
### This should be the same as the "java.ext.dirs" System
### property of your Java Virtual Machine when run
### with no cross-compilation options.

# lappend beam::compiler::java::system_ext_dirs {/path/to/jre/lib/ext}


  

############################################################
# Section 5: Default Classpath
############################################################

### The system_classpath variable is a list of entries
### that will be used as the default classpath if no
### option is given on the command-line to override it.
### As per the Java documentation, it should default to
### the value of the environment variable $CLASSPATH.
### If the system_classpath variable ends up empty
### after BEAM has parsed all command-line arguments,
### "." will be used as the system_classpath.

if { [::info exists ::env(CLASSPATH)] } {
  set beam::compiler::java::system_classpath [split $::env(CLASSPATH) $::beam::pathsep]
}

