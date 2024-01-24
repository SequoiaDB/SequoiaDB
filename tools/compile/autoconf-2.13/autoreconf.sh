#! @SHELL@
# autoreconf - remake all Autoconf configure scripts in a directory tree
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

usage="\
Usage: autoreconf [-f] [-h] [--help] [-m dir] [--macrodir=dir]
       [-l dir] [--localdir=dir] [--force] [--verbose] [--version]
       [--cygnus] [--foreign] [--gnits] [--gnu] [-i] [--include-deps]"

localdir=
verbose=no
show_version=no
force=no
automake_mode=--gnu
automake_deps=

test -z "$AC_MACRODIR" && AC_MACRODIR=@datadir@

while test $# -gt 0; do
  case "$1" in 
  -h | --help | --h*)
    echo "$usage"; exit 0 ;;
  --localdir=* | --l*=* )
    localdir="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
    shift ;;
  -l | --localdir | --l*)
    shift
    test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
    localdir="${1}"
    shift ;;
  --macrodir=* | --m*=* )
    AC_MACRODIR="`echo \"$1\" | sed -e 's/^[^=]*=//'`"
    shift ;;
  -m | --macrodir | --m*)
    shift
    test $# -eq 0 && { echo "$usage" 1>&2; exit 1; }
    AC_MACRODIR="$1"
    shift ;;
  --verbose | --verb*)
    verbose=yes; shift ;;
  -f | --force)
    force=yes; shift ;;
  --version | --vers*)
    show_version=yes; shift ;;
  --cygnus | --foreign | --gnits | --gnu)
    automake_mode=$1; shift ;;
  --include-deps | -i)
    automake_deps=$1; shift ;;
  --)     # Stop option processing.
    shift; break ;;
  -*) echo "$usage" 1>&2; exit 1 ;;
  *) break ;;
  esac
done

if test $show_version = yes; then
  version=`sed -n 's/define.AC_ACVERSION.[ 	]*\([0-9.]*\).*/\1/p' \
    $AC_MACRODIR/acgeneral.m4`
  echo "Autoconf version $version"
  exit 0
fi

if test $# -ne 0; then
  echo "$usage" 1>&2; exit 1
fi

# The paths to the autoconf and autoheader scripts, at the top of the tree.
top_autoconf=`echo $0|sed s%autoreconf%autoconf%`
top_autoheader=`echo $0|sed s%autoreconf%autoheader%`

# Make a list of directories to process.
# The xargs grep filters out Cygnus configure.in files.
find . -name configure.in -print |
xargs grep -l AC_OUTPUT |
sed 's%/configure\.in$%%; s%^./%%' |
while read dir; do
  (
  cd $dir || continue

  case "$dir" in
  .) dots= ;;
  *) # A "../" for each directory in /$dir.
     dots=`echo /$dir|sed 's%/[^/]*%../%g'` ;;
  esac

  case "$0" in
  /*)  autoconf=$top_autoconf; autoheader=$top_autoheader ;;
  */*) autoconf=$dots$top_autoconf; autoheader=$dots$top_autoheader ;;
  *)   autoconf=$top_autoconf; autoheader=$top_autoheader ;;
  esac

  case "$AC_MACRODIR" in
  /*)  macrodir_opt="--macrodir=$AC_MACRODIR" ;;
  *)   macrodir_opt="--macrodir=$dots$AC_MACRODIR" ;;
  esac

  case "$localdir" in
  "")  localdir_opt=
       aclocal=aclocal.m4 ;;
  /*)  localdir_opt="--localdir=$localdir"
       aclocal=$localdir/aclocal.m4 ;;
  *)   localdir_opt="--localdir=$dots$localdir"
       aclocal=$dots$localdir/aclocal.m4 ;;
  esac

  # Regenerate aclocal.m4 if necessary.  FIXME: if aclocal searches
  # nonstandard directories, we need to deal with that here.  The
  # easiest way is to move this info into configure.in.
  run_aclocal=no
  if test -f "$aclocal" &&
     grep 'generated automatically by aclocal' $aclocal > /dev/null
  then
     run_aclocal=yes
  else
     if test -f `echo $aclocal | sed 's,/*[^/]*$,,;s,^$,.,'`/acinclude.m4
     then
	run_aclocal=yes
     fi
  fi
  if test $run_aclocal = yes
  then
     if test $force = no &&
        ls -lt configure.in $aclocal \
	       `echo $aclocal | sed 's,/*[^/]*$,,;s,^$,.,'`/acinclude.m4 |
	  sed 1q |
          grep 'aclocal\.m4$' > /dev/null
     then
	:
     else
	test $verbose = yes && echo running aclocal in $dir, creating $aclocal
	aclocal --output=$aclocal -I `echo $aclocal | sed 's,/*[^/]*$,,;s,^$,.,'`
     fi
  fi

  # Re-run automake if required.  Assumes that there is a Makefile.am
  # in the topmost directory.
  if test -f Makefile.am
  then
     amforce=
     test $force = no && amforce=--no-force
     test $verbose = yes && echo running automake`test x"$amforce" = x || echo " ($amforce)"` in $dir
     automake $amforce $automake_mode $automake_deps
  fi

  test ! -f $aclocal && aclocal=

  if test $force = no && test -f configure &&
    ls -lt configure configure.in $aclocal | sed 1q |
      grep 'configure$' > /dev/null
  then
    :
  else
    test $verbose = yes && echo running autoconf in $dir
    $autoconf $macrodir_opt $localdir_opt
  fi

  if grep 'A[CM]_CONFIG_HEADER' configure.in >/dev/null; then
    templates=`sed -n '/A[CM]_CONFIG_HEADER/ {
	s%[^#]*A[CM]_CONFIG_HEADER[ 	]*(\([^)]*\).*%\1%
	p
	q
      }' configure.in`
    tcount=`set -- $templates; echo $#`
    template=`set -- $templates; echo $1 | sed '
	s/.*://
	t colon
	s/$/.in/
	: colon
	s/:.*//
      '`
    stamp=`echo $template | sed 's,/*[^/]*$,,;s,^$,.,'`/stamp-h`test "$tcount" -gt 1 && echo "$tcount"`.in
    if test ! -f "$template" || grep autoheader "$template" >/dev/null; then
      if test $force = no && test -f $template &&
	 ls -lt $template configure.in $aclocal $stamp 2>/dev/null \
	        `echo $localdir_opt | sed -e 's/--localdir=//' \
		                          -e '/./ s%$%/%'`acconfig.h |
	   sed 1q | egrep "$template$|$stamp$" > /dev/null
      then
        :
      else
        test $verbose = yes && echo running autoheader in $dir
        $autoheader $macrodir_opt $localdir_opt && 
        { test $verbose != yes || echo touching $stamp; } &&
        touch $stamp
      fi
    fi
  fi
  )
done
