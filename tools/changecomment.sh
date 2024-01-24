#!/bin/bash
if [ $# != 1 ]; then
   echo "Syntax: $0 <path>"
   exit 0
fi
find $1 -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.hpp" -o -name "*.h" \) -not -path ".svn" -exec dos2unix -n {} {}.bak \; -exec cp {}.bak {} \; -exec rm {}.bak \; -exec sed -i '/^[ ]*OCO SOURCE MATERIALS/{$!{N;N;N;N;N;N;N;N;N;N;s/[ ]*OCO SOURCE MATERIALS\n\n[ ]*SEQUOIADB CONFIDENTIAL (SEQUOIADB CONFIDENTIAL-RESTRICTED when combined\n[ ]*with the Aggregated OCO Source Modules for this Program)\n\n[ ]*COPYRIGHT: xxxxx (C) Copyright SequoiaDB Inc. 2012\n[ ]*Licensed Materials - Program Property of SequoiaDB Inc.\n\n[ ]*The source code for this program is not published or otherwise divested of\n[ ]*its trade secrets, irrespective of what has been deposited with the Copyright\n[ ]*Protection Center of China/\n   Copyright (C) 2011-2014 SequoiaDB Ltd.\n\n   This program is free software: you can redistribute it and\/or modify\n   it under the term of the GNU Affero General Public License, version 3,\n   as published by the Free Software Foundation.\n\n   This program is distributed in the hope that it will be useful,\n   but WITHOUT ANY WARRANTY; without even the implied warrenty of\n   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n   GNU Affero General Public License for more details.\n\n   You should have received a copy of the GNU Affero General Public License\n   along with this program. If not, see <http:\/\/www.gnu.org\/license\/>./;ty;P;D;:y}}' {} \;

