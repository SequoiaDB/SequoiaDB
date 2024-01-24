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

   Source File Name = rawbson2csv.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014   ly  Initial Draft
          01/12/2016  hjw

   Last Changed =

*******************************************************************************/
#ifndef UTIL_BSON_2_CSV_H__
#define UTIL_BSON_2_CSV_H__

#include "core.h"
#include "oss.h"
#include "ossUtil.h"
#include "ossMem.h"

#define CSV_STR_UNDEFINED  "undefined"
#define CSV_STR_MINKEY     "minKey"
#define CSV_STR_MAXKEY     "maxKey"

#define CSV_STR_UNDEFINED_SIZE   ( sizeof( CSV_STR_UNDEFINED ) - 1 )
#define CSV_STR_MINKEY_SIZE      ( sizeof( CSV_STR_MINKEY ) - 1 )
#define CSV_STR_MAXKEY_SIZE      ( sizeof( CSV_STR_MAXKEY ) - 1 )

SDB_EXTERN_C_START

SDB_EXPORT void setCsvPrecision( const CHAR *pFloatFmt ) ;

SDB_EXPORT void setPrintfLog( void (*pFun)( const CHAR *pFunc,
                                            const CHAR *pFile,
                                            UINT32 line,
                                            const CHAR *pFmt,
                                            ... ) ) ;

SDB_EXPORT INT32 getCSVSize( const CHAR *delChar,
                             const CHAR *delField, INT32 delFieldSize,
                             CHAR *pbson, INT32 *pCSVSize,
                             BOOLEAN includeBinary,
                             BOOLEAN includeRegex,
                             BOOLEAN kickNull ) ;
SDB_EXPORT INT32 bson2csv( const CHAR *delChar,
                           const CHAR *delField, INT32 delFieldSize,
                           CHAR *pbson, CHAR **ppBuffer, INT32 *pCSVSize,
                           BOOLEAN includeBinary,
                           BOOLEAN includeRegex,
                           BOOLEAN kickNull ) ;
SDB_EXTERN_C_END

#endif
