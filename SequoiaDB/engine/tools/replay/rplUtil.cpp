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

   Source File Name = rplUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplUtil.hpp"
#include "ossUtil.hpp"
#include "ossTypes.h"
#include "dpsDef.hpp"
#include "pd.hpp"
#include "../bson/bson.hpp"
#include "../bson/bsonDecimal.h"
#include <sstream>

using namespace std ;
using namespace bson ;

namespace replay
{
   void getCurrentTime( string &timeStr )
   {
      ossTimestamp Tm ;
      CHAR szFormat[] = "%04d%02d%02d%02d%02d%02d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec ) ;

      timeStr = szTimestmpStr ;
   }

   void getCurrentDate( string &dateStr )
   {
      ossTimestamp Tm ;
      CHAR szFormat[] = "%04d%02d%02d%02d%02d" ;
      CHAR szDatetmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szDatetmpStr, sizeof( szDatetmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min) ;

      dateStr = szDatetmpStr ;
   }

   BOOLEAN isSameDay( UINT64 microSecondLeft, UINT64 microSecondRight )
   {
      struct tm tmLeft ;
      ossTimestamp ossTMLeft = ossMicrosecondsToTimestamp( microSecondLeft ) ;
      ossLocalTime( ossTMLeft.time, tmLeft ) ;

      struct tm tmRight ;
      ossTimestamp ossTMRight = ossMicrosecondsToTimestamp( microSecondRight ) ;
      ossLocalTime( ossTMRight.time, tmRight ) ;

      if ( tmLeft.tm_year == tmRight.tm_year
           && tmLeft.tm_mon == tmRight.tm_mon
           && tmLeft.tm_mday == tmRight.tm_mday )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   UINT64 replaceAndGetTime( UINT64 currentTime, INT32 newHour, INT32 newMinite,
                             INT32 newSecond )
   {
      struct tm tm ;
      ossTimestamp ossTM = ossMicrosecondsToTimestamp( currentTime ) ;
      ossLocalTime( ossTM.time, tm ) ;

      tm.tm_hour = newHour ;
      tm.tm_min = newMinite ;
      tm.tm_sec = newSecond ;

      ossTM.time = ossMkTime( &tm ) ;

      return ossTimestampToMicroseconds( ossTM ) ;
   }

   void rplTimestampToString( ossTimestamp &timestamp, string &timeStr )
   {
      CHAR szFormat[] = "%04d-%02d-%02d %02d.%02d.%02d.%06d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossLocalTime( timestamp.time, tmpTm ) ;

      if ( timestamp.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         timestamp.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    timestamp.microtm ) ;

      timeStr = szTimestmpStr ;
   }

   void rplDateToString( INT64 millis, string &dateStr )
   {
      CHAR szDateStr[ 16 ] = { 0 } ;
      struct tm tmpTm ;
      time_t timer = (time_t)( millis / 1000 ) ;

      ossLocalTime( timer, tmpTm ) ;
      ossSnprintf( szDateStr, sizeof( szDateStr ),
                   "%04d-%02d-%02d",
                   tmpTm.tm_year + 1900,
                   tmpTm.tm_mon + 1,
                   tmpTm.tm_mday ) ;
      dateStr = szDateStr ;
   }

   INT32 rplCheckFloatFmt( const CHAR *pFloatFmt )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == pFloatFmt )
      {
         goto done ;
      }

      if ( '%' != pFloatFmt[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "floatFmt is invalid, format is %%[+][.Precision]Type" ) ;
         goto error ;
      }

      ++pFloatFmt ;

      if ( '+' == pFloatFmt[0] )
      {
         ++pFloatFmt ;
      }

      if ( '.' == pFloatFmt[0] )
      {
         ++pFloatFmt ;
         for ( ; pFloatFmt[0] >= '0' && pFloatFmt[0] <= '9' ; ++pFloatFmt )
         {
         }
      }

      if ( 'f' != pFloatFmt[0] && 'e' != pFloatFmt[0] && 'E' != pFloatFmt[0]
           && 'g' != pFloatFmt[0] && 'G' != pFloatFmt[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "floatFmt is invalid, type is f|e|E|g|G" ) ;
         goto error ;
      }

      ++pFloatFmt ;

      if ( 0 != pFloatFmt[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "floatFmt is invalid, format is %%[+][.Precision]Type" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void rplDoubleToString( FLOAT64 val, BOOLEAN fullPrecision,
                           const CHAR *floatFmt, string &outStr )
   {
      CHAR buf[ 512 ] = { 0 } ;
      INT32 len = -1 ;

      if ( fullPrecision )
      {
         len = ossDoubleToFullPrecisionString( val, buf, (INT32)sizeof( buf ) ) ;
      }
      else
      {
         SDB_ASSERT( NULL != floatFmt, "floatFmt should not be NULL" ) ;
         len = (INT32)ossSnprintf( buf, sizeof( buf ), floatFmt, val ) ;
         if ( len < 0 || len >= (INT32)sizeof( buf ) )
         {
            len = -1 ;
         }
      }

      if ( len >= 0 )
      {
         outStr = buf ;
      }
      else
      {
         outStr.clear() ;
      }
   }

   INT32 rplCastNumberToString( const BSONElement &ele, BSONType targetType,
                                string &outStr )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      bsonDecimal decimal ;

      if ( NumberInt != ele.type() && NumberLong != ele.type()
           && NumberDecimal != ele.type() )
      {
         return SDB_INVALIDARG ;
      }

      switch ( targetType )
      {
      case NumberInt :
      {
         INT32 i = 0 ;
         if ( NumberInt == ele.type() )
         {
            i = ele.Int() ;
         }
         else if ( NumberLong == ele.type() )
         {
            INT64 l = ele.Long() ;
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
         }
         else
         {
            INT64 l = 0 ;
            rc = ele.Decimal().toLong( &l ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
         }
         ss << i ;
         outStr = ss.str() ;
         break ;
      }
      case NumberLong :
      {
         INT64 l = 0 ;
         if ( NumberLong == ele.type() )
         {
            l = ele.Long() ;
         }
         else if ( NumberInt == ele.type() )
         {
            l = ( INT64 )ele.Int() ;
         }
         else
         {
            rc = ele.Decimal().toLong( &l ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         ss << l ;
         outStr = ss.str() ;
         break ;
      }
      case NumberDecimal :
      {
         if ( NumberDecimal == ele.type() )
         {
            outStr = ele.Decimal().toString() ;
         }
         else if ( NumberLong == ele.type() )
         {
            rc = decimal.fromLong( ele.Long() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            outStr = decimal.toString() ;
         }
         else
         {
            rc = decimal.fromInt( ele.Int() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            outStr = decimal.toString() ;
         }
         break ;
      }
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

   error:
      return rc ;
   }

   void rplTimeIncToString( time_t &timer, UINT32 inc, string &timeStr )
   {
      CHAR szFormat[] = "%04d-%02d-%02d %02d.%02d.%02d.%06d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossLocalTime( timer, tmpTm ) ;
      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    inc ) ;

      timeStr = szTimestmpStr ;
   }

   CHAR* getOPName(UINT16 type)
   {
      switch(type)
      {
      case LOG_TYPE_DATA_INSERT:
         return RPL_LOG_OP_INSERT;
      case LOG_TYPE_DATA_UPDATE:
         return RPL_LOG_OP_UPDATE;
      case LOG_TYPE_DATA_DELETE:
         return RPL_LOG_OP_DELETE;
      case LOG_TYPE_CL_TRUNC:
         return RPL_LOG_OP_TRUNCATE_CL;
      case LOG_TYPE_CS_CRT:
         return RPL_LOG_OP_CREATE_CS;
      case LOG_TYPE_CS_DELETE:
         return RPL_LOG_OP_DELETE_CS;
      case LOG_TYPE_CL_CRT:
         return RPL_LOG_OP_CREATE_CL;
      case LOG_TYPE_CL_DELETE:
         return RPL_LOG_OP_DELETE_CL;
      case LOG_TYPE_IX_CRT:
         return RPL_LOG_OP_CREATE_IX;
      case LOG_TYPE_IX_DELETE:
         return RPL_LOG_OP_DELETE_IX;
      case LOG_TYPE_LOB_WRITE:
         return RPL_LOG_OP_LOB_WRITE;
      case LOG_TYPE_LOB_REMOVE:
         return RPL_LOG_OP_LOB_REMOVE;
      case LOG_TYPE_LOB_UPDATE:
         return RPL_LOG_OP_LOB_UPDATE;
      case LOG_TYPE_LOB_TRUNCATE:
         return RPL_LOG_OP_LOB_TRUNCATE;
      case LOG_TYPE_DUMMY:
         return RPL_LOG_OP_DUMMY;
      case LOG_TYPE_CL_RENAME:
         return RPL_LOG_OP_CL_RENAME;
      case LOG_TYPE_TS_COMMIT:
         return RPL_LOG_OP_TS_COMMIT;
      case LOG_TYPE_TS_ROLLBACK:
         return RPL_LOG_OP_TS_ROLLBACK;
      case LOG_TYPE_INVALIDATE_CATA:
         return RPL_LOG_OP_INVALIDATE_CATA;
      case LOG_TYPE_CS_RENAME:
         return RPL_LOG_OP_CS_RENAME;
      case LOG_TYPE_DATA_POP:
         return RPL_LOG_OP_POP;
      case LOG_TYPE_RETURN:
         return RPL_LOG_OP_RETURN;
      default:
         return "unknown";
      }
   }
}

