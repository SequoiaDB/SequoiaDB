#! @SHELL@
# autoheader -- create `config.h.in' from `configure.in'
# Copyright (C) 1992, 1993, 1994, 1996, 1998 Free Software Foundation, Inc.

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

# Written by Roland McGrath.

# If given no args, create `config.h.in' from template file `configure.in'.
# With one arg, create a header file on standard output from
# the given template file.

usage="\
Usage: autoheader [-h] [--help] [-m dir] [--macrodir=dir]
       [-l dir] [--localdir=dir] [--version] [template-file]"

# NLS nuisances.
# Only set these to C if already set.  These must not be set unconditionally
# because not all systems understand e.g. LANG=C (notably SCO).
# Fixing LC_MESSAGES prevents Solaris sh from translating var values in `set'!
# Non-C LC_CTYPE values break the ctype check.
if test "${LANG+set}"   = set; then LANG=C;   export LANG;   fi
if test "${LC_ALL+set}" = set; then LC_ALL=C; export LC_ALL; fi
if test "${LC_MESSAGES+set}" = set; then LC_MESSAGES=C; export LC_MESSAGES; fi
if test "${LC_CTYPE+set}"    = set; then LC_CTYPE=C;    export LC_CTYPE;    fi

test -z "${AC_MACRODIR}" && AC_MACRODIR=@datadir@
test -z "${M4}" && M4=@M4@
case "${M4}" in
/*) # Handle the case that m4 has moved since we were configured.
    # It may have been found originally in a build directory.
    test -f "${M4}" || M4=m4 ;;
esac

localdir=.
show_version=no

while test $# -gt 0 ; do
   case "${1}" in
      -h | --help | --h* )
         echo "${usage}"; exit 0 ;;
      --localdir=* | --l*=* )
         localdir="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
         shift ;;
      -l | --localdir | --l*)
         shift
         test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
         localdir="${1}"
         shift ;;
      --macrodir=* | --m*=* )
         AC_MACRODIR="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
         shift ;;
      -m | --macrodir | --m* )
         shift
         test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
         AC_MACRODIR="${1}"
         shift ;;
      --version | --v* )
         show_version=yes; shift ;;
      -- )     # Stop option processing
        shift; break ;;
      - )	# Use stdin as input.
        break ;;
      -* )
        echo "${usage}" 1>&2; exit 1 ;;
      * )
        break ;;
   esac
done

if test $show_version = yes; then
  version=`sed -n 's/define.AC_ACVERSION.[ 	]*\([0-9.]*\).*/\1/p' \
    $AC_MACRODIR/acgeneral.m4`
  echo "Autoconf version $version"
  exit 0
fi

TEMPLATES="${AC_MACRODIR}/acconfig.h"
test -r $localdir/acconfig.h && TEMPLATES="${TEMPLATES} $localdir/acconfig.h"

case $# in
  0) infile=configure.in ;;
  1) infile=$1 ;;
  *) echo "$usage" >&2; exit 1 ;;
esac

config_h=config.h
syms=
types=
funcs=
headers=
libs=

if test "$localdir" != .; then
  use_localdir="-I$localdir -DAC_LOCALDIR=$localdir"
else
  use_localdir=
fi

# Use the frozen version of Autoconf if available.
r= f=
# Some non-GNU m4's don't reject the --help option, so give them /dev/null.
case `$M4 --help < /dev/null 2>&1` in
*reload-state*) test -r $AC_MACRODIR/autoheader.m4f && { r=--reload f=f; } ;;
*traditional*) ;;
*) echo Autoconf requires GNU m4 1.1 or later >&2; exit 1 ;;
esac

# Extract assignments of SYMS, TYPES, FUNCS, HEADERS, and LIBS from the
# modified autoconf processing of the input file.  The sed hair is
# necessary to win for multi-line macro invocations.
eval "`$M4 -I$AC_MACRODIR $use_localdir $r autoheader.m4$f $infile |
       sed -n -e '
		: again
		/^@@@.*@@@$/s/^@@@\(.*\)@@@$/\1/p
		/^@@@/{
			s/^@@@//p
			n
			s/^/@@@/
			b again
		}'`"

# Make SYMS newline-separated rather than blank-separated, and remove dups.
# Start each symbol with a blank (to match the blank after "#undef")
# to reduce the possibility of mistakenly matching another symbol that
# is a substring of it.
syms="`for sym in $syms; do echo $sym; done | sort | uniq | sed 's@^@ @'`"

if test $# -eq 0; then
  tmpout=autoh$$
  trap "rm -f $tmpout; exit 1" 1 2 15
  exec > $tmpout
fi

# Support "outfile[:infile]", defaulting infile="outfile.in".
case "$config_h" in
*:*) config_h_in=`echo "$config_h"|sed 's%.*:%%'`
     config_h=`echo "$config_h"|sed 's%:.*%%'` ;;
*) config_h_in="${config_h}.in" ;;
esac

# Don't write "do not edit" -- it will get copied into the
# config.h, which it's ok to edit.
cat <<EOF
/* ${config_h_in}.  Generated automatically from $infile by autoheader.  */
EOF

test -r ${config_h}.top && cat ${config_h}.top
test -r $localdir/acconfig.h &&
  grep @TOP@ $localdir/acconfig.h >/dev/null &&
  sed '/@TOP@/,$d' $localdir/acconfig.h

# This puts each template paragraph on its own line, separated by @s.
if test -n "$syms"; then
  # Make sure the boundary of template files is also the boundary
  # of the paragraph.  Extra newlines don't hurt since they will
  # be removed.
  # Undocumented useless feature: stuff outside of @TOP@ and @BOTTOM@ 
  # is ignored in the systemwide acconfig.h too.
  for t in $TEMPLATES; do
    sedscript=""
    grep @TOP@ $t >/dev/null && sedscript="1,/@TOP@/d;"
    grep @BOTTOM@ $t >/dev/null && sedscript="$sedscript /@BOTTOM@/,\$d;"
    # This substitution makes "#undef<TAB>FOO" in acconfig.h work.
    sed -n -e "$sedscript s/	/ /g; p" $t
    echo; echo
  done |
  # The sed script is suboptimal because it has to take care of
  # some broken seds (e.g. AIX) that remove '\n' from the
  # pattern/hold space if the line is empty. (junio@twinsun.com).
  sed -n -e '
	/^[ 	]*$/{
		x
		s/\n/@/g
		p
		s/.*/@/
		x
	}
	H' | sed -e 's/@@*/@/g' |
  # Select each paragraph that refers to a symbol we picked out above.
  # Some fgrep's have limits on the number of lines that can be in the
  # pattern on the command line, so use a temporary file containing the
  # pattern.
  (fgrep_tmp=${TMPDIR-/tmp}/autoh$$
   trap "rm -f $fgrep_tmp; exit 1" 1 2 15
   cat > $fgrep_tmp <<EOF
$syms
EOF
   fgrep -f $fgrep_tmp
   rm -f $fgrep_tmp) |
  tr @ \\012
fi

echo "$types" | tr , \\012 | sort | uniq | while read ctype; do
  test -z "$ctype" && continue
  sym="`echo "${ctype}" | tr 'abcdefghijklmnopqrstuvwxyz *' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_P'`"
  echo "
/* The number of bytes in a ${ctype}.  */
#undef SIZEOF_${sym}"
done

# /bin/sh on the Alpha gives `for' a random value if $funcs is empty.
if test -n "$funcs"; then
  for func in `for x in $funcs; do echo $x; done | sort | uniq`; do
    sym="`echo ${func} | sed 's/[^a-zA-Z0-9_]/_/g' | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`"
    echo "
/* Define if you have the ${func} function.  */
#undef HAVE_${sym}"
  done
fi

if test -n "$headers"; then
  for header in `for x in $headers; do echo $x; done | sort | uniq`; do

    sym="`echo ${header} | sed 's/[^a-zA-Z0-9_]/_/g' | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`"
    echo "
/* Define if you have the <${header}> header file.  */
#undef HAVE_${sym}"
  done
fi

if test -n "$libs"; then
  for lib in `for x in $libs; do echo $x; done | sort | uniq`; do
   sym="`echo ${lib} | sed 's/[^a-zA-Z0-9_]/_/g' | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`"
    echo "
/* Define if you have the ${lib} library (-l${lib}).  */
#undef HAVE_LIB${sym}"
  done
fi

if test -n "$verbatim"; then
  echo "$verbatim"
fi

# Handle the case where @BOTTOM@ is the first line of acconfig.h.
test -r $localdir/acconfig.h &&
  grep @BOTTOM@ $localdir/acconfig.h >/dev/null &&
  sed -n '/@BOTTOM@/,${/@BOTTOM@/!p;}' $localdir/acconfig.h
test -f ${config_h}.bot && cat ${config_h}.bot

status=0

if test -n "$syms"; then
  for sym in $syms; do
    if fgrep $sym $TEMPLATES >/dev/null; then
      : # All is well.
    else
      echo "$0: Symbol \`${sym}' is not covered by $TEMPLATES" >&2
      status=1
    fi
  done
fi

if test $# -eq 0; then
  if test $status -eq 0; then
    if test -f ${config_h_in} && cmp -s $tmpout ${config_h_in}; then
      rm -f $tmpout # File didn't change, so don't update its mod time.
    else
      mv -f $tmpout ${config_h_in}
    fi
  else
    rm -f $tmpout
  fi
fi

exit $status
