/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilTimestamp.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component. This file contains declare of json2rawbson. Note
   this function should NEVER be directly called other than fromjson.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/23/2015  JWH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include "core.h"
#include "oss.h"
#include "ossUtil.h"
#include <time.h>

SDB_EXTERN_C_START

typedef struct {
   INT64 sec ;
   INT32 nsec ;
   INT16 offset;
} sdbTimestamp ;

INT32 timestampParse( const CHAR *str, INT32 len, sdbTimestamp *pTime ) ;

BOOLEAN timestampValid( const sdbTimestamp *pTime ) ;

INT32 timestamp2UtcTm( const sdbTimestamp *pTime, struct tm *pTmTime ) ;

INT32 timestamp2LocalTm( const sdbTimestamp *pTime, struct tm *pTmTime ) ;

SDB_EXTERN_C_END

#endif

