#!/bin/bash
if [ $# != 1 ]; then
   echo "Syntax: $0 <path>"
   exit 0
fi
find $1 -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.hpp" -o -name "*.h" \) -not -path ".svn" -exec echo -n {} \; -exec head -1 {} \; | grep -v "/\*\*\*" | grep -v gtest | grep -v bson | grep -v mdocml | grep -v pcre  | grep -v ncurses | grep -v ssh2 | grep -v snappy | grep -v auto | grep -v cJSON | grep -v linenoise | grep -v "Trace.hpp"

