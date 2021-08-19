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

   Source File Name = pmdStartupHistoryLogger.hpp

   Descriptive Name = pmd start-up history logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          01/01/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_STARTUPHISTORYLOGGER_HPP_
#define PMD_STARTUPHISTORYLOGGER_HPP_

#include <vector>
#include "ossFile.hpp"
#include "ossUtil.hpp"
#include "ossTypes.hpp"
#include "pmdStartup.hpp"

#define PMD_STARTUPHST_FILE_NAME ".SEQUOIADB_STARTUP_HISTORY"

namespace engine
{
   /*
      _pmdStartupHistoryLogger define
      start-up history logger
   */
   #define PMD_STARTUP_LOG_LEN_MAX     ( 64 )
   #define PMD_STARTUP_LOG_HEADER      "pid, startTime, startType, version"OSS_NEWLINE
   #define PMD_STARTUP_LOG_HEADER_SIZE ( sizeof( PMD_STARTUP_LOG_HEADER ) - 1 )
   #define PMD_STARTUP_LOG_FIELD_NUM   ( 4 )
   #define PMD_STARTUP_FILESIZE_LIMIT  ( 1000 * 1024 )
   #define PMD_STARTUP_LOGSIZE_MAX     ( 100 * 1024 )

   struct _pmdStartupLog
   {
      OSSPID            _pid ;
      ossTimestamp      _time ;
      SDB_START_TYPE    _type ;
      CHAR              _dbVersion[32] ;

      _pmdStartupLog() ;

      _pmdStartupLog( OSSPID pid, ossTimestamp time, SDB_START_TYPE type ) ;

      string toString()
      {
         CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         CHAR strLog[ PMD_STARTUP_LOG_LEN_MAX ] = { 0 } ;

         ossTimestampToString( _time, strTime ) ;

         ossSnprintf( strLog, sizeof( strLog ) - 1, "%d,%s,%s,%s"OSS_NEWLINE,
                      _pid,
                      strTime,
                      pmdGetStartTypeStr( _type ),
                      _dbVersion ) ;
         return strLog ;
      }
   } ;
   typedef _pmdStartupLog pmdStartupLog ;

   BOOLEAN pmdStr2StartupLog( const string& str, pmdStartupLog& log ) ;

   class _pmdStartupHistoryLogger : public SDBObject
   {
      public:
         _pmdStartupHistoryLogger () ;
         ~_pmdStartupHistoryLogger () ;

         INT32 init() ;
         INT32 clearAll() ;
         INT32 getLatestLogs( UINT32 num, vector<pmdStartupLog> &vecLogs );

      protected:
         INT32 _clearEarlyLogs() ;
         INT32 _loadLogs() ;
         INT32 _log() ;

      private:
         ossFile _file ;
         vector<pmdStartupLog> _buffer ;
         BOOLEAN _initOk ;
         CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
   };

   typedef _pmdStartupHistoryLogger pmdStartupHistoryLogger ;

   pmdStartupHistoryLogger* pmdGetStartupHstLogger () ;

}

#endif //PMD_STARTUPHISTORYLOGGER_HPP_

