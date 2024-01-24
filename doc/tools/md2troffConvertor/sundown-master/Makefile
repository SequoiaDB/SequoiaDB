# Makefile

# Copyright (c) 2009, Natacha Porté
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

DEPDIR=depends

# "Machine-dependant" options
#MFLAGS=-fPIC

#CFLAGS=-c -g -O0 -fPIC -Wall -Werror -Wsign-compare -Isrc -Ihtml
INCLUDE_DIR=../../../../SequoiaDB/engine/include

CFLAGS=-c -g -O0 -fPIC -Wall -Wsign-compare -Isrc -Ihtml -I$(INCLUDE_DIR)
LDFLAGS=-g -O3 -Wall -Werror 
CC=gcc


SUNDOWN_SRC=\
	src/markdown.o \
	src/stack.o \
	src/buffer.o \
	src/autolink.o \
    src/renderer.o \
	src/main.o

all:		md2md

.PHONY:		all clean

# executables

md2md:	$(SUNDOWN_SRC)
	$(CC) $(LDFLAGS) $^ -o $@

# housekeeping
clean:
	rm -f src/*.o html/*.o examples/*.o
	rm -f libmd2md.so libmd2md.so.1 md2md
	rm -f md2md.exe
	rm -rf $(DEPDIR)

# dependencies

include $(wildcard $(DEPDIR)/*.d)


# generic object compilations

%.o:	src/%.c examples/%.c html/%.c
	@mkdir -p $(DEPDIR)
	@$(CC) -MM $< > $(DEPDIR)/$*.d
	$(CC) $(CFLAGS) -o $@ $<

