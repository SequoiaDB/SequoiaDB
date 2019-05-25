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

   Source File Name = utilTimestampParse.cpp

   Descriptive Name = parse timestamp(ISO8601)

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

#include "timestamp.h"

#define MIN_SEC (-62135596800LL)
#define MAX_SEC (253402300799LL)

BOOLEAN timestampValid( const sdbTimestamp *pTime )
{
    const INT64 sec = pTime->sec + pTime->offset * 60 ;
    if( sec < MIN_SEC || sec > MAX_SEC ||
        pTime->nsec < 0 || pTime->nsec > 999999999 ||
        pTime->offset < -1439 || pTime->offset > 1439 )
    {
        return FALSE ;
    }
    return TRUE ;
}

