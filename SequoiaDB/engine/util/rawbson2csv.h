/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

/* Enable (TRUE) or disable (FALSE) full-precision double formatting for CSV.
 * When enabled, finite doubles use ossDoubleToFullPrecisionString; NaN/Inf
 * keep the existing literal-string behavior. Default is FALSE.
 */
SDB_EXPORT void setCsvFullPrecision( BOOLEAN enable ) ;

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
