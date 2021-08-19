/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

//1970-01-01T00:00:00
#define RDN_OFFSET (62135683200LL)

static const UINT16 _dayOffset[13] = {
    0, 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275
} ;

static void rdn2Tm( UINT32 rdn, struct tm *pTmTime )
{
   UINT32 Z = 0 ;
   UINT32 H = 0 ;
   UINT32 A = 0 ;
   UINT32 B = 0 ;
   UINT16 C = 0 ;
   UINT16 y = 0 ;
   UINT16 m = 0 ;
   UINT16 d = 0 ;

   Z = rdn + 306 ;
   H = 100 * Z - 25 ;
   A = H / 3652425 ;
   B = A - ( A >> 2 ) ;
   y = ( 100 * B + H) / 36525 ;
   C = B + Z - ( 1461 * y >> 2 ) ;
   m = ( 535 * C + 48950 ) >> 14 ;
   if( m > 12 )
   {
      d = C - 306 ;
      ++y ;
      m -= 12 ;
   }
   else
   {
      d = C + 59 + ( ( y & 3 ) == 0 && ( y % 100 != 0 || y % 400 == 0 ) ) ;
   }

   //Day of month [1,31]
   pTmTime->tm_mday = C - _dayOffset[m] ;
   //Month of year [0,11]
   pTmTime->tm_mon  = m - 1;
   //Years since 1900
   pTmTime->tm_year = y - 1900 ;
   //Day of week [0,6] (Sunday =0)
   pTmTime->tm_wday = rdn % 7 ;
   //Day of year [0,365]
   pTmTime->tm_yday = d - 1 ;
}

static INT32 timestamp2Tm( const sdbTimestamp *pTime,
                           struct tm *pTmTime,
                           const BOOLEAN local )
{
   INT32 rc = SDB_OK ;
   UINT64 sec = 0 ;
   UINT32 rdn = 0 ;
   UINT32 sod = 0 ;

   if( !timestampValid( pTime ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   sec = pTime->sec + RDN_OFFSET ;
   if( local )
   {
      sec += pTime->offset * 60 ;
   }
   rdn = sec / 86400 ;
   sod = sec % 86400 ;

   rdn2Tm( rdn, pTmTime ) ;
   pTmTime->tm_sec  = sod % 60;
   sod /= 60 ;
   pTmTime->tm_min  = sod % 60;
   sod /= 60 ;
   pTmTime->tm_hour = sod ;
done:
   return rc ;
error:
   goto done ;
}

INT32 timestamp2LocalTm( const sdbTimestamp *pTime, struct tm *pTmTime )
{
   return timestamp2Tm( pTime, pTmTime, TRUE ) ;
}

INT32 timestamp2UtcTm( const sdbTimestamp *pTime, struct tm *pTmTime )
{
   return timestamp2Tm( pTime, pTmTime, FALSE ) ;
}

