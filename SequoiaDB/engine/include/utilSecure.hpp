/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilSecure.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTILSECURE_HPP_
#define UTILSECURE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

#define UTIL_SECURE_HEAD               "SDBSECURE"
#define UTIL_SECURE_BEGIN_SYMBOL       '('
#define UTIL_SECURE_END_SYMBOL         ')'
#define UTIL_SECURE_END_SYMBOL_STR     ")"
#define UTIL_SECURE_ENCRYPT_ALGORITHM  UTIL_SECURE_SDB_BASE64
#define UTIL_SECURE_ENCRYPT_VERSION    UTIL_SECURE_SDB_BASE64_V0
#define UTIL_SECURE_COMPRESS_ALGORITHM 0 // reserve
#define UTIL_SECURE_COMPRESS_VERSION   0 // reserve

#define UTIL_SECURE_SDB_BASE64     0
#define UTIL_SECURE_SDB_BASE64_V0  0

ossPoolString utilSecureObj( const bson::BSONObj &obj ) ;

ossPoolString utilSecureStr( const CHAR* data, INT32 size ) ;
ossPoolString utilSecureStr( const ossPoolString& str ) ;
ossPoolString utilSecureStr( const string& str ) ;

INT32 utilSecureDecrypt( const ossPoolString& str, ossPoolString& output ) ;

#endif
