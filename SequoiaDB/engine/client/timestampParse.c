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

static const UINT8 _lastDays[2][13] = {
   {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
} ;

static const UINT32 _Pow10[10] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
} ;

static const UINT16 _dayOffset[13] = {
    0, 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275
} ;

static INT32 isLeapYear( UINT16 year )
{
   return ( ( year & 3 ) == 0 && ( year % 100 != 0 || year % 400 == 0 ) ) ;
}

static UINT8 monthLastDays( UINT16 year, UINT16 month )
{
   return _lastDays[month == 2 && isLeapYear( year )][month] ;
}

static INT32 parse2Number( const UINT8 *const pStrTime,
                           INT32 index,
                           UINT16 *pNumber )
{
   INT32 rc = SDB_OK ;
   UINT8 n1 = pStrTime[index + 0] - '0' ;
   UINT8 n2 = pStrTime[index + 1] - '0' ;
   if( n1 > 9 || n2 > 9 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pNumber = n1 * 10 + n2 ;
done:
   return rc ;
error:
   goto done ;
}

static INT32 parse4Number( const UINT8* const pStrTime,
                           INT32 index,
                           UINT16 *pNumber )
{
   INT32 rc = SDB_OK ;
   UINT8 n1 = pStrTime[index + 0] - '0' ;
   UINT8 n2 = pStrTime[index + 1] - '0' ;
   UINT8 n3 = pStrTime[index + 2] - '0' ;
   UINT8 n4 = pStrTime[index + 3] - '0' ;
   if( n1 > 9 || n2 > 9 || n3 > 9 || n4 > 9 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pNumber = n1 * 1000 + n2 * 100 + n3 * 10 + n4 ;
done:
   return rc ;
error:
   goto done ;
}

INT32 timestampParse( const CHAR *pStr, INT32 len, sdbTimestamp *pTime )
{
   INT32 rc = SDB_OK ;
   UINT8 ch = 0 ;
   UINT16 year = 0 ;
   UINT16 month = 0 ;
   UINT16 day = 0 ;
   UINT16 hour = 0 ;
   UINT16 min = 0 ;
   UINT16 sec = 0 ;
   UINT32 rdn = 0 ;
   UINT32 sod = 0 ;
   UINT32 nsec = 0 ;
   CHAR timeSeparator = ':' ;
   INT16 offset = 0 ;
   const UINT8 *pCur = NULL ;
   const UINT8 *pEnd = NULL ;

   pCur = (const UINT8 *)pStr ;
   if( len < 20 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( pCur[13] == '.' )
   {
      timeSeparator = '.' ;
   }

   if( pCur[4]  != '-' || pCur[7]  != '-' ||
       pCur[13] != timeSeparator || pCur[16] != timeSeparator )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ch = pCur[10] ;
   if( !( ch == 'T' || ch == ' ' || ch == 't') )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( parse4Number( pCur, 0,  &year )  || year  < 1  ||
       parse2Number( pCur, 5,  &month ) || month < 1  || month > 12 ||
       parse2Number( pCur, 8,  &day )   || day   < 1  || day   > 31 ||
       parse2Number( pCur, 11, &hour )  || hour  > 23 ||
       parse2Number( pCur, 14, &min )   || min   > 59 ||
       parse2Number( pCur, 17, &sec )   || sec   > 59 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( day > 28 && day > monthLastDays( year, month ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( month < 3 )
   {
      --year ;
   }

   rdn = ( 1461 * year ) / 4 - year / 100 +
         year / 400 + _dayOffset[month] + day - 306 ;
   sod = hour * 3600 + min * 60 + sec;
   pEnd = pCur + len;
   pCur = pCur + 19;
   offset = nsec = 0;

   ch = *pCur++;
   if( ch == '.' )
   {
      const UINT8 *pStart;
      INT32 ndigits = 0 ;

      pStart = pCur;
      for( ; pCur < pEnd; ++pCur )
      {
         const UINT8 digit = *pCur - '0';
         if( digit > 9 )
         {
            break ;
         }
         nsec = nsec * 10 + digit ;
      }

      ndigits = pCur - pStart;
      if( ndigits < 1 || ndigits > 9 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      nsec *= _Pow10[ 9 - ndigits ] ;

      if( pCur == pEnd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ch = *pCur++;
   }

   if( !( ch == 'Z' || ch == 'z' ) )
   {
      if( pCur + 5 == pEnd && ( ch == '+' || ch == '-' ) &&
          pCur[2] == timeSeparator )
      {
         if( parse2Number( pCur, 0, &hour ) || hour > 23 ||
             parse2Number( pCur, 3, &min )  || min  > 59 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         offset = hour * 60 + min;
         if( ch == '-' )
         {
            offset *= -1 ;
         }
         pCur += 5 ;
      }
      else if( pCur + 4 == pEnd && ( ch == '+' || ch == '-' ) )
      {
         if( parse2Number( pCur, 0, &hour ) || hour > 23 ||
             parse2Number( pCur, 2, &min )  || min  > 59 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         offset = hour * 60 + min;
         if( ch == '-' )
         {
            offset *= -1 ;
         }
         pCur += 4 ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   if( pCur != pEnd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pTime->sec    = ( (INT64)rdn - 719163 ) * 86400 + sod - offset * 60 ;
   pTime->nsec   = nsec ;
   pTime->offset = offset ;
done:
   return rc ;
error:
   goto done ;
}