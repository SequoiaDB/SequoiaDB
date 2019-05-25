/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

using namespace std ;

namespace engine
{
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
      INT32 permission = OSS_RU | OSS_WU | OSS_RG | OSS_RO ;

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

      if ( fileSize < PMD_STARTUP_LOGSIZE_MAX )
      {
         goto done ;
      }

      if ( fileSize > PMD_STARTUP_FILESIZE_LIMIT )
      {
         rc = _file.truncate( PMD_STARTUP_LOG_HEADER_SIZE );
         PD_RC_CHECK ( rc, PDERROR, "Failed to pop logs from file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;
         goto done ;
      }

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

      log._type = pmdGetStartup().getStartType() ;
      log._pid = ossGetCurrentProcessID() ;
      ossGetCurrentTime( log._time ) ;
      strLog = log.toString() ;

      rc = _file.seek( 0, OSS_SEEK_END ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to write seek file[%s], ",
                    "rc: %d", _file.getPath().c_str(), rc ) ;
      rc = _file.writeN( strLog.c_str(), strLog.length() ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to write start info into file[%s], "
                    "rc: %d", _file.getPath().c_str(), rc ) ;

      _buffer.push_back( log ) ;

   done :
      PD_TRACE_EXITRC( SDB__PMDSTARTHSTLOG__LOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTHSTLOG_GETLATEST, "_pmdStartupHistoryLogger::getLatestLogs" )
   INT32 _pmdStartupHistoryLogger::getLatestLogs( UINT32 num,
                                              vector<pmdStartupLog> &vecLogs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTARTHSTLOG_GETLATEST ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      for( vector<pmdStartupLog>::reverse_iterator it = _buffer.rbegin();
           it != _buffer.rend() && vecLogs.size() < num ;
           it++ )
      {
         vecLogs.push_back(*it) ;
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


