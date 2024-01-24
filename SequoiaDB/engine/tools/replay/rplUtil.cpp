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
#include "dpsDef.hpp"
#include "pd.hpp"

using namespace std ;

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

