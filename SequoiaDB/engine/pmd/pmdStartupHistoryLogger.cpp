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

   Source File Name = pmdStartupHistoryLogger.cpp

   Descriptive Name = pmd start-up history logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          01/01/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdStartupHistoryLogger.hpp"
#include "pmdTrace.hpp"
#include "pmd.hpp"
#include "ossProc.hpp"
#include "utilStr.hpp"
#include "pmdDef.hpp"
#include "ossVer.hpp"

using namespace std ;

namespace engine
{
   _pmdStartupLog::_pmdStartupLog()
   :_pid( OSS_INVALID_PID ), _type( SDB_START_NORMAL )
   {
      ossGetSimpleVersion( _dbVersion, 32 ) ;
   }

   _pmdStartupLog::_pmdStartupLog( OSSPID pid,
                                   ossTimestamp time,
                                   SDB_START_TYPE type )
   :_pid( pid ), _time( time ), _type( type )
   {
      ossGetSimpleVersion( _dbVersion, 32 ) ;
   }

   BOOLEAN pmdStr2StartupLog( const string& str, pmdStartupLog& log )
   {
      BOOLEAN isOk = TRUE ;

      vector<string> fields = utilStrSplit( str, "," ) ;
      if( PMD_STARTUP_LOG_FIELD_NUM != fields.size() )
      {
         isOk = FALSE ;
      }
      else
      {
         ossSscanf( fields.at(0).c_str(), "%d", &log._pid ) ;
         ossStringToTimestamp( fields.at(1).c_str(), log._time ) ;
         log._type = pmdStr2StartType( fields.at(2).c_str() ) ;
      }

      return isOk ;
   }

   _pmdStartupHistoryLogger::_pmdStartupHistoryLogger ()
   : _initOk( FALSE )
   {
      ossMemset( _fileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
   }

   _pmdStartupHistoryLogger::~_pmdStartupHistoryLogger ()
   {
      _file.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG_INIT, "_pmdStartupHistoryLogger::init" )
   INT32 _pmdStartupHistoryLogger::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG_INIT ) ;
      BOOLEAN isExist = FALSE ;
      BOOLEAN doCreate = FALSE ;
      INT32 mode = OSS_READWRITE | OSS_CREATE ;
      INT32 permission = OSS_DEFAULTFILE ;

      ossScopedLock _lock( &_loggerLock, EXCLUSIVE ) ;

      // 1. open file
      rc = utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                              PMD_STARTUPHST_FILE_NAME,
                              OSS_MAX_PATHSIZE, _fileName ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to build full file path , "
                    "rc: %d", rc ) ;

      rc = ossFile::exists( _fileName, isExist ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to check existence of file[%s], "
                    "rc: %d", _fileName, rc ) ;

      rc = _file.open( _fileName, mode, permission ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to open file[%s], "
                    "rc: %d", _fileName, rc ) ;
      doCreate = ( !isExist ) ? TRUE : FALSE ;

      if ( !isExist )
      {
         CHAR *header = PMD_STARTUP_LOG_HEADER ;
         rc = _file.writeN( header, PMD_STARTUP_LOG_HEADER_SIZE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write header into file[%s], "
                    "rc: %d", _fileName, rc ) ;
      }

      // 2. load logs
      rc = _clearEarlyLogs() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _loadLogs() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // 3. print log
      rc = _log() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _initOk = TRUE ;

   done :
      _file.close() ;
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG_INIT, rc ) ;
      return rc ;
   error :
      if ( doCreate )
      {
         _file.close() ;
         _file.deleteFileIfExists( _fileName ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG__LOADLOG, "_pmdStartupHistoryLogger::_loadLogs" )
   INT32 _pmdStartupHistoryLogger::_loadLogs()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG__LOADLOG ) ;

      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      CHAR* readBuf = NULL ;
      vector<string> records ;

      // if sdb.id too large, it is abnormal
      rc = _file.getFileSize( fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get size of file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

      if ( fileSize > PMD_STARTUP_FILESIZE_LIMIT )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK ( rc, PDERROR, "File size is too large[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;
         goto error ;
      }

      // read all string
      readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !readBuf )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( readBuf, 0, fileSize + 1 ) ;

      rc = _file.seek( 0, OSS_SEEK_SET ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to seek file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;
      rc = _file.readN( readBuf, fileSize, readSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to read file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

      records = utilStrSplit( readBuf, OSS_NEWLINE ) ;
      if ( records.empty() )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      records.erase( records.begin() ) ; // erase header

      // format log
      for ( vector<string>::iterator i = records.begin();
            i != records.end();
            ++i )
      {
         pmdStartupLog log ;
         if ( pmdStr2StartupLog( *i, log ) )
         {
            _buffer.push_back( log ) ;
         }
      }

   done :
      if ( readBuf )
      {
         SDB_OSS_FREE( readBuf ) ;
         readBuf = NULL ;
      }
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG__LOADLOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG__CLREARLY, "_pmdStartupHistoryLogger::_clearEarlyLogs" )
   INT32 _pmdStartupHistoryLogger::_clearEarlyLogs()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG__CLREARLY ) ;

      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      CHAR* readBuf = NULL ;
      vector<string> splited ;
      INT64 popSize = 0 ;
      string writeBuf ;

      rc = _file.getFileSize( fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get size of file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

      /// if file isn't large enough, do nothing
      if ( fileSize < PMD_STARTUP_LOGSIZE_MAX )
      {
         goto done ;
      }

      /// if file is too large, just truncate it
      if ( fileSize > PMD_STARTUP_FILESIZE_LIMIT )
      {
         rc = _file.truncate( PMD_STARTUP_LOG_HEADER_SIZE );
         PD_RC_CHECK ( rc, PDERROR, "Failed to pop logs from file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;
         goto done ;
      }

      /// pop half logs
      readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !readBuf )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( readBuf, 0, fileSize + 1 ) ;
      rc = _file.readN( readBuf, fileSize, readSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to read file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

      splited = utilStrSplit( readBuf, OSS_NEWLINE ) ;
      if ( splited.empty() )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      popSize = splited.size() / 2 ;
      splited.erase( splited.begin(), splited.begin() + 1 + popSize ) ;

      for ( vector<string>::const_iterator i = splited.begin();
            i != splited.end();
            ++i )
      {
         writeBuf += *i ;
         writeBuf += OSS_NEWLINE ;
      }

      rc = _file.truncate( PMD_STARTUP_LOG_HEADER_SIZE );
      PD_RC_CHECK ( rc, PDERROR, "Failed to pop logs from file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

      rc = _file.seek( 0, OSS_SEEK_END ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to seek file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;
      rc = _file.writeN( writeBuf.c_str(), ossStrlen( writeBuf.c_str() ) ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to write start info into file[%s], "
                    "rc: %d", _file.getPath().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG__CLREARLY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG__LOG, "_pmdStartupHistoryLogger::_log" )
   INT32 _pmdStartupHistoryLogger::_log()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG__LOG ) ;
      pmdStartupLog log ;
      string strLog ;

      // build log
      log._type = pmdGetStartup().getStartType() ;
      log._pid = ossGetCurrentProcessID() ;
      ossGetCurrentTime( log._time ) ;
      strLog = log.toString() ;

      // write to file
      rc = _file.seek( 0, OSS_SEEK_END ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to write seek file[%s], "
                    "rc: %d", _file.getPath().c_str(), rc ) ;
      rc = _file.writeN( strLog.c_str(), strLog.length() ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to write start info into file[%s], "
                    "rc: %d", _file.getPath().c_str(), rc ) ;

      // add this log to buffer
      _buffer.push_back( log ) ;

   done :
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG__LOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG_GETLATEST, "_pmdStartupHistoryLogger::getLatestLogs" )
   INT32 _pmdStartupHistoryLogger::getLatestLogs( UINT32 num,
                                                  PMD_STARTUP_LOG_LIST &vecLogs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG_GETLATEST ) ;

      ossScopedLock _lock( &_loggerLock, SHARED ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         for( PMD_STARTUP_LOG_LIST::reverse_iterator it = _buffer.rbegin();
              it != _buffer.rend() && vecLogs.size() < num ;
              it++ )
         {
            vecLogs.push_back(*it) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get startup logs, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG_GETLATEST, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG_CLRALL, "_pmdStartupHistoryLogger::clearAll" )
   INT32 _pmdStartupHistoryLogger::clearAll()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG_CLRALL ) ;

      ossScopedLock _lock( &_loggerLock, EXCLUSIVE ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _buffer.clear() ;

      rc = _file.deleteFileIfExists( _fileName ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to delete file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG_CLRALL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   pmdStartupHistoryLogger* pmdGetStartupHstLogger ()
   {
      static pmdStartupHistoryLogger _startupLogger ;
      return &_startupLogger ;
   }

}


