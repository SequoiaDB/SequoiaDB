#! @SHELL@
# ifnames - print the identifiers used in C preprocessor conditionals
# Copyright (C) 1994, 1995 Free Software Foundation, Inc.

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

# Reads from stdin if no files are given.
# Writes to stdout.

# Written by David MacKenzie <djm@gnu.ai.mit.edu>
# and Paul Eggert <eggert@twinsun.com>.

usage="\
Usage: ifnames [-h] [--help] [-m dir] [--macrodir=dir] [--version] [file...]"
show_version=no

: ${AC_MACRODIR=@datadir@}

while test $# -gt 0; do
  case "$1" in 
  -h | --help | --h* )
    echo "$usage"; exit 0 ;;
  --macrodir=* | --m*=* )
    AC_MACRODIR="`echo \"$1\" | sed -e 's/^[^=]*=//'`"
    shift ;;
  -m | --macrodir | --m* )
    shift
    test $# -eq 0 && { echo "$usage" 1>&2; exit 1; }
    AC_MACRODIR="$1"
    shift ;;
  --version | --versio | --versi | --vers)
    show_version=yes; shift ;;
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

@AWK@ '
  # Record that sym was found in FILENAME.
  function file_sym(sym,  i, fs) {
    if (sym ~ /^[A-Za-z_]/) {
      if (!found[sym,FILENAME]) {
	found[sym,FILENAME] = 1

	# Insert FILENAME into files[sym], keeping the list sorted.
	i = 1
	fs = files[sym]
 	while (match(substr(fs, i), /^ [^ ]*/) \
 	       && substr(fs, i + 1, RLENGTH - 1) < FILENAME) {
 	  i += RLENGTH
	}
	files[sym] = substr(fs, 1, i - 1) " " FILENAME substr(fs, i)
      }
    }
  }

  /^[\t ]*#/ {
    if (sub(/^[\t ]*#[\t ]*ifn?def[\t ]+/, "", $0)) {
      sub(/[^A-Za-z_0-9].*/, "", $0)
      file_sym($0)
    }
    if (sub(/^[\t ]*#[\t ]*(el)?if[\t ]+/, "", $0)) {
      # Remove comments.  Not perfect, but close enough.
      gsub(/\/\*[^\/]*(\*\/)?/, "", $0)

      for (i = split($0, field, /[^A-Za-z_0-9]+/);  1 <= i;  i--) {
	if (field[i] != "defined") {
	  file_sym(field[i])
	}
      }
    }
  }

  END {
    for (sym in files) {
      print sym files[sym]
    }
  }
' ${1+"$@"} | sort
