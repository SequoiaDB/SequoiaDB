#! @SHELL@
# autoupdate - modernize a configure.in
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

# If given no args, update `configure.in';
# With one arg, write on the standard output from the given template file.
#
# Written by David MacKenzie <djm@gnu.ai.mit.edu>

usage="\
Usage: autoupdate [-h] [--help] [-m dir] [--macrodir=dir] 
       [--version] [template-file]" 

sedtmp=/tmp/acups.$$
# For debugging.
#sedtmp=/tmp/acups
show_version=no
test -z "${AC_MACRODIR}" && AC_MACRODIR=@datadir@

while test $# -gt 0 ; do
   case "${1}" in 
      -h | --help | --h* )
         echo "${usage}" 1>&2; exit 0 ;;
      --macrodir=* | --m*=* )
         AC_MACRODIR="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
         shift ;;
      -m | --macrodir | --m* ) 
         shift
         test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
         AC_MACRODIR="${1}"
         shift ;;
      --version | --versio | --versi | --vers)
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

: ${SIMPLE_BACKUP_SUFFIX='~'}

tmpout=acupo.$$
trap 'rm -f $sedtmp $tmpout; exit 1' 1 2 15
case $# in
  0) infile=configure.in; out="> $tmpout"
     # Make sure $infile can be read, and $tmpout has the same permissions.
     cp $infile $tmpout || exit

     # Make sure $infile can be written.
     if test ! -w $infile; then
       rm -f $tmpout
       echo "$0: $infile: cannot write" >&2
       exit 1
     fi
     ;;
  1) infile="$1"; out= ;;
  *) echo "$usage" >&2; exit 1 ;;
esac

# Turn the m4 macro file into a sed script.
# For each old macro name, make one substitution command to replace it
# at the end of a line, and one when followed by ( or whitespace.
# That is easier than splitting the macros up into those that take
# arguments and those that don't.
sed -n -e '
/^AC_DEFUN(/ {
  s//s%/
  s/, *\[indir(\[/$%/
  s/\].*/%/
  p
  s/\$//
  s/%/^/
  s/%/\\([( 	]\\)^/
  s/%/\\1^/
  s/\^/%/g
  p
}' ${AC_MACRODIR}/acoldnames.m4 > $sedtmp
eval sed -f $sedtmp $infile $out

case $# in
  0) mv configure.in configure.in${SIMPLE_BACKUP_SUFFIX} &&
     mv $tmpout configure.in ;;
esac

rm -f $sedtmp $tmpout
exit 0
