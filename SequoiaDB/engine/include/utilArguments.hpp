/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilArguments.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_ARGUMENTS_HPP_
#define UTIL_ARGUMENTS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"

namespace engine
{

   #define UTIL_ARG_FIELD_EMPTY              ( 0x00000000 )
   #define UTIL_ARG_FIELD_ALL                ( 0xFFFFFFFF )

   #define UTIL_CL_NAME_FIELD                ( 0x00000001 )
   #define UTIL_CL_SHDKEY_FIELD              ( 0x00000002 )
   #define UTIL_CL_REPLSIZE_FIELD            ( 0x00000004 )
   #define UTIL_CL_ENSURESHDIDX_FIELD        ( 0x00000008 )
   #define UTIL_CL_SHDTYPE_FIELD             ( 0x00000010 )
   #define UTIL_CL_PARTITION_FIELD           ( 0x00000020 )
   #define UTIL_CL_COMPRESSED_FIELD          ( 0x00000040 )
   #define UTIL_CL_ISMAINCL_FIELD            ( 0x00000080 )
   #define UTIL_CL_AUTOSPLIT_FIELD           ( 0x00000100 )
   #define UTIL_CL_AUTOREBALANCE_FIELD       ( 0x00000200 )
   #define UTIL_CL_AUTOIDXID_FIELD           ( 0x00000400 )
   #define UTIL_CL_COMPRESSTYPE_FIELD        ( 0x00000800 )
   #define UTIL_CL_CAPPED_FIELD              ( 0x00001000 )
   #define UTIL_CL_MAXREC_FIELD              ( 0x00002000 )
   #define UTIL_CL_MAXSIZE_FIELD             ( 0x00004000 )
   #define UTIL_CL_OVERWRITE_FIELD           ( 0x00008000 )
   #define UTIL_CL_STRICTDATAMODE_FIELD      ( 0x00010000 )
   #define UTIL_CL_AUTOINC_FIELD             ( 0x00020000 )
   #define UTIL_CL_LOBKEYFORMAT_FIELD        ( 0x00040000 )

   // mask for autoincrement option.
   #define UTIL_CL_AUTOINCREMENT_FIELD       ( 0x00040000 )

   // mask for one field of autoincrement filed attr.
   #define UTIL_CL_AUTOINC_INCREMENT_FIELD   ( 0x00000001 )
   #define UTIL_CL_AUTOINC_STARTVALUE_FIELD  ( 0x00000002 )
   #define UTIL_CL_AUTOINC_MINVALUE_FIELD    ( 0x00000004 )
   #define UTIL_CL_AUTOINC_MAXVALUE_FIELD    ( 0x00000008 )
   #define UTIL_CL_AUTOINC_CACHESIZE_FIELD   ( 0x00000010 )
   #define UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ( 0x00000020 )
   #define UTIL_CL_AUTOINC_CYCLED_FIELD      ( 0x00000040 )
   #define UTIL_CL_AUTOINC_GENERATED_FIELD   ( 0x00000080 )
   #define UTIL_CL_AUTOINC_CURVALUE_FIELD    ( 0x00000100 )
   #define UTIL_CL_AUTOINC_INITIAL_FIELD     ( 0x00000200 )

   #define UTIL_CS_NAME_FIELD                ( 0x00000001 )
   #define UTIL_CS_DOMAIN_FIELD              ( 0x00000002 )
   #define UTIL_CS_PAGESIZE_FIELD            ( 0x00000004 )
   #define UTIL_CS_LOBPAGESIZE_FIELD         ( 0x00000008 )
   #define UTIL_CS_CAPPED_FIELD              ( 0x00000010 )

   #define UTIL_DOMAIN_NAME_FIELD            ( 0x00000001 )
   #define UTIL_DOMAIN_GROUPS_FIELD          ( 0x00000002 )
   #define UTIL_DOMAIN_AUTOSPLIT_FIELD       ( 0x00000004 )
   #define UTIL_DOMAIN_AUTOREBALANCE_FIELD   ( 0x00000008 )

}

#endif // UTIL_ARGUMENTS_HPP_
