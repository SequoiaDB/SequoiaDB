# Makefile to generate build tools
#
# Standard usage:
#        make -f build.mk
#
# Written by Sascha Schumann
#
# $Id: build.mk 242949 2007-09-26 15:44:16Z cvs2svn $ 


LT_TARGETS = ltmain.sh ltconfig

config_h_in = zend_config.h.in

makefile_am_files = Makefile.am
makefile_in_files = $(makefile_am_files:.am=.in)
makefile_files    = $(makefile_am_files:e.am=e)

targets = $(makefile_in_files) $(LT_TARGETS) configure $(config_h_in)

all: $(targets)

clean:
	rm -f $(targets)

$(LT_TARGETS):
	rm -f $(LT_TARGETS)
	libtoolize --automake $(AMFLAGS) -f

$(makefile_in_files): $(makefile_am_files)
	automake -a -i $(AMFLAGS) $(makefile_files)

aclocal.m4: configure.in acinclude.m4
	aclocal

$(config_h_in): configure.in acconfig.h
# explicitly remove target since autoheader does not seem to work 
# correctly otherwise (timestamps are not updated)
	@rm -f $@
	autoheader

configure: aclocal.m4 configure.in
	autoconf
