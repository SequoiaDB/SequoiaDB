/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = logProcessor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "logProcessor.hpp"
#include "ossFile.hpp"
#include "ossIO.hpp"
#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "dpsLogFile.hpp"
#include "dpsArchiveFile.hpp"
#include "dpsArchiveFileMgr.hpp"
#include "dpsLogRecord.hpp"
#include "dmsLobDef.hpp"
#include "dpsOp2Record.hpp"

namespace sdbrevert
{
   #define SDB_REVERT_DOC_MAX_CACHE_NUM 1000
   #define SDB_REVERT_LOB_MAX_FILL_SIZE 256 * 1024 // 256KB

   #define SDB_REVERT_IS_INTERRUPT ( _interrupted->fetch() > 0 || ( SDB_OK != _globalInfoMgr->getGlobalRc() && _options->stop() ) )

   logProcessor::logProcessor()
   {
      _conn          = NULL ;
      _connected     = FALSE ;
      _outputCL      = NULL ;
      _options       = NULL ;
      _logFileMgr    = NULL ;
      _lobMetaMgr    = NULL ;
      _lobLockMap    = NULL ;
      _globalInfoMgr = NULL ;
      _buf           = NULL ;
      _bufSize       = 0 ;
      _lobHoleBuf    = NULL ;
      _interrupted   = NULL ;
   }

   logProcessor::~logProcessor()
   {
      if ( _connected && NULL != _conn )
      {
         _conn->disconnect() ;
      }

      SAFE_OSS_DELETE( _conn ) ;
      SAFE_OSS_DELETE( _outputCL ) ;

      if ( _bufSize > 0 )
      {
         SAFE_OSS_FREE( _buf ) ;
         _bufSize = 0 ;
      }

      SAFE_OSS_FREE( _lobHoleBuf ) ;
   }

   INT32 logProcessor::_connect()
   {
      INT32 rc                = SDB_OK ;
      vector<string> hostList = _options->hostList() ;
      INT32 hostNum           = hostList.size() ;
      const CHAR** connAddrs  = NULL ;
      string hostsStr         = "" ;

      _conn = new(std::nothrow) sdb( _options->useSSL() ) ;
      if ( NULL == _conn )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new sdbclient::sdb, rc= %d", rc ) ;
         goto error ;
      }

      connAddrs = new(std::nothrow) const CHAR*[hostNum] ;
      if ( NULL == connAddrs )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new const CHAR*[], rc= %d", rc ) ;
         goto error ;
      }

      for ( size_t i = 0 ; i < hostList.size() ; ++i ) {
         connAddrs[i] = hostList[i].c_str() ;

         if ( i != 0 )
         {
            hostsStr += "," ;
         }
         hostsStr += hostList[i] ;
      }

      if ( _options->password().empty() && !_options->cipherfile().empty())
      {
         rc = _conn->connect( connAddrs, hostNum,
                            _options->userName().c_str(),
                            _options->token().c_str(),
                            _options->cipherfile().c_str() ) ;
      }
      else
      {
         rc = _conn->connect( connAddrs, hostNum,
                            _options->userName().c_str(),
                            _options->password().c_str() ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to connect sdb, hosts= %s, usessl= %d, rc= %d", 
                 hostsStr.c_str(), _options->useSSL(), rc ) ;
         goto error ;
      }

      _connected = TRUE ;

   done:
      if ( connAddrs )
      {
         delete[] connAddrs ;
      }

      return rc ;
   error:
      SAFE_OSS_DELETE( _conn ) ;
      goto done ;
   }

   INT32 logProcessor::_getCL()
   {
      INT32 rc = SDB_OK ;

      if ( !_connected || NULL == _conn )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _outputCL = new(std::nothrow) sdbCollection() ;
      if ( NULL == _outputCL )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new sdbclient::sdbCollection, rc= %d", rc ) ;
         goto error ;
      }

      rc = _conn->getCollection( _options->outputClName().c_str(), *_outputCL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get output cl[%s], rc= %d", 
                 _options->outputClName().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _outputCL ) ;
      goto done ;
   }

   INT32 logProcessor::_revertLogFile( const string &filePath )
   {
      INT32 rc                                          = SDB_OK ;
      CHAR startTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { '\0' } ;
      CHAR endTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ]   = { '\0' } ;
      CHAR lastWriteTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { '\0' } ;
      time_t lastWriteTime ;

      // check permission
      rc = ossAccess( filePath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access the log file[%s], rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      // check time
      rc = ossFile::getLastWriteTime( filePath, lastWriteTime ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the last modification time of the log file[%s]"
                 "rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      if ( lastWriteTime < _options->startTime() || lastWriteTime >= _options->endTime() )
      {
         utilAscTime( _options->startTime(), startTimeStr, OSS_TIMESTAMP_STRING_LEN ) ;
         utilAscTime( _options->endTime(), endTimeStr, OSS_TIMESTAMP_STRING_LEN ) ;
         utilAscTime( lastWriteTime, lastWriteTimeStr, OSS_TIMESTAMP_STRING_LEN ) ;
         PD_LOG( PDEVENT, "The last modified time[%s] of file[%s], which is no within the"
                 "specified time range[%s, %s)", lastWriteTimeStr, filePath.c_str(),
                 startTimeStr, endTimeStr ) ;
         goto done ;
      }

      // check type
      if ( isArchivelog( filePath ) )
      {
         rc = _revertArchivelogFile( filePath );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if ( isReplicalog( filePath ) )
      {
         rc = _revertReplicalogFile( filePath ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "File[%s] is not archivelog or replicalog, rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_revertArchivelogFile( const string &filePath )
   {
      INT32 rc                        = SDB_OK ;
      dpsArchiveHeader* archiveHeader = NULL ;
      dpsLogHeader* logHeader         = NULL ;
      BOOLEAN hasTmpFile              = FALSE ;
      string archiveFilePath          = filePath ;
      string tmpFile ;
      dpsArchiveFile archiveFile ;
      dpsLogFile logFile ;

      PD_LOG( PDEVENT, "Start reverting archive log file[%s]", filePath.c_str() ) ;

      rc = archiveFile.init( archiveFilePath, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init archive log file[%s], rc= %d",
                 archiveFilePath.c_str(), rc ) ;
         goto error ;
      }

      archiveHeader = archiveFile.getArchiveHeader() ;
      logHeader = archiveFile.getLogHeader() ;

      // uncompress archive file
      if ( archiveHeader->hasFlag( DPS_ARCHIVE_COMPRESSED ) )
      {
         rc = _uncompresLogFile( archiveFilePath, tmpFile, hasTmpFile ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         archiveFile.close() ;
         archiveHeader = NULL ;
         archiveFilePath = tmpFile ;

         rc = archiveFile.init( archiveFilePath, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init archive log file[%s], rc= %d",
                    archiveFilePath.c_str(), rc ) ;
            goto error ;
         }

         archiveHeader = archiveFile.getArchiveHeader() ;
         archiveHeader->unsetFlag( DPS_ARCHIVE_COMPRESSED ) ;
         logHeader = archiveFile.getLogHeader() ;
      }

      rc = logFile.init( archiveFilePath.c_str(), (UINT32)logHeader->_fileSize, logHeader->_fileNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init archive log file[%s], rc= %d",
                 archiveFilePath.c_str(), rc ) ;
         goto error ;
      }

      rc = _revertFileWithLSN( logFile,
                               archiveHeader->startLSN.offset,
                               archiveHeader->endLSN.offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to revert archive log file[%s], rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Finished reverting archive log file[%s]", filePath.c_str() ) ;

   done:
      if ( hasTmpFile )
      {
         ossFile::deleteFile( tmpFile ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_uncompresLogFile( const string &filePath, string &newFilePath, BOOLEAN &create )
   {
      INT32 rc = SDB_OK ;
      string tmpFile ;
      string fileName ;
      dpsArchiveFileMgr archiveFileMgr ;

      if ( _options->tempPath().empty() )
      {
         tmpFile = filePath + SDB_REVERT_TMP_ARCHIVELOG_SUFFIX ;
      }
      else
      {
         fileName = ossFile::getFileName( filePath ) + SDB_REVERT_TMP_ARCHIVELOG_SUFFIX ;
         tmpFile = joinPath( _options->tempPath(), fileName ) ;
      }

      ossFile::deleteFile( tmpFile ) ;
      rc = archiveFileMgr.copyArchiveFile( filePath, tmpFile, DPS_ARCHIVE_COPY_UNCOMPRESS ) ;
      if ( SDB_OK != rc )
      {
         create = FALSE ;
         PD_LOG( PDERROR, "Failed to uncompress archive log file[%s], rc= %d",
                  filePath.c_str(), rc ) ;
         goto error ;
      }

      create = TRUE ;
      newFilePath = tmpFile ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_revertReplicalogFile( const string &filePath )
   {
      INT32 rc                 = SDB_OK ;
      UINT32 fileSize          = 0 ;
      UINT32 fileNum           = 0 ;
      DPS_LSN_OFFSET startLSN  = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET endLSN    = DPS_INVALID_LSN_OFFSET ;
      dpsLogFile logFile ;

      PD_LOG( PDEVENT, "Start reverting replica log file[%s]", filePath.c_str() ) ;

      rc = _getLogFileSizeAndNum( filePath, fileSize, fileNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get infomation form replica log file[%s], rc= %d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      rc = logFile.init( filePath.c_str(), fileSize, fileNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init replica log file[%s], rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      startLSN = logFile.getFirstLSN().offset ;
      endLSN = startLSN + logFile.getValidLength() ;

      if ( DPS_INVALID_LSN_OFFSET == startLSN && DPS_INVALID_LSN_OFFSET == endLSN )
      {
         PD_LOG( PDINFO, "Skip empty replica log file[%s], startLSN[%ld], endLSN[%ld], rc= %d",
                 filePath.c_str(), startLSN, endLSN, rc ) ;
         goto done ;
      }

      rc = _revertFileWithLSN( logFile, startLSN, endLSN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to revert replica log file[%s], rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Finished reverting replica log file[%s]", filePath.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_getLogFileSizeAndNum( const string &filePath, UINT32& fileSize, UINT32& fileNum )
   {
      INT32 rc                 = SDB_OK ;
      _dpsLogHeader* logHeader = NULL ;
      INT64 readSize           = 0 ;
      ossFile file ;

      rc = file.open( filePath, OSS_READONLY, OSS_DEFAULTFILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open log file[%s], rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      logHeader = SDB_OSS_NEW _dpsLogHeader() ;
      if ( NULL == logHeader )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to create new _dpsLogHeader!, rc= %d", rc ) ;
         goto error ;
      }

      rc = file.readN( (CHAR*)logHeader, DPS_LOG_HEAD_LEN, readSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read file header, rc= %d", rc ) ;
         goto error ;
      }

      if ( DPS_LOG_HEAD_LEN != readSize )
      {
         rc = SDB_DPS_FILE_NOT_RECOGNISE ;
         PD_LOG( PDERROR, "DPS file header length error, rc= %d", rc ) ;
         goto error ;
      }

      if ( 0 != ossStrncmp( logHeader->_eyeCatcher,
                            DPS_LOG_HEADER_EYECATCHER,
                            DPS_LOG_HEADER_EYECATCHER_LEN ) )
      {
         rc = SDB_DPS_FILE_NOT_RECOGNISE ;
         PD_LOG( PDERROR, "DPS file eye catcher error, rc= %d", rc ) ;
         goto error ;
      }

      fileSize = logHeader->_fileSize ;
      fileNum = logHeader->_fileNum ;

   done:
      SAFE_OSS_DELETE( logHeader ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_revertFileWithLSN( dpsLogFile& logFile,
                                           DPS_LSN_OFFSET startLSN,
                                           DPS_LSN_OFFSET endLSN )
   {
      INT32 rc                      = SDB_OK ;
      dpsLogRecord log;
      dpsLogRecordHeader& logHeader = log.head();
      DPS_LSN_OFFSET currentLSN ;

      // check lsn
      if ( startLSN >= endLSN )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid LSN, startLSN[%lld] <= endLSN[%lld], filePath= %s",
                 startLSN, endLSN, logFile.path().c_str() ) ;
         goto error ;
      }

      if ( endLSN < _options->startLSN() || startLSN > _options->endLSN() )
      {
         PD_LOG( PDEVENT, "The lsn[%ld, %ld) of file[%s], which is no within the specified lsn "
                 "range[%ld, %ld)", startLSN, endLSN, logFile.path().c_str(),
                 _options->startLSN(), _options->endLSN() ) ;
         goto done ;
      }

      currentLSN = logFile.getFirstLSN().offset;
      if ( DPS_INVALID_LSN_OFFSET == currentLSN || currentLSN < startLSN )
      {
         rc = SDB_DPS_CORRUPTED_LOG ;
         PD_LOG( PDERROR, "Invalid first LSN[%lld] of log file[%s]", currentLSN, logFile.path().c_str() ) ;
         goto error ;
      }

      currentLSN = startLSN >= _options->startLSN() ? startLSN : _options->startLSN() ;
      endLSN = endLSN <= _options->endLSN() ? endLSN : _options->endLSN() ;

      _localInfo.reset() ;
      _localInfo.fileNumInc() ;
      _localInfo.setStartLSN( currentLSN ) ;
      _localInfo.setEndLSN( endLSN ) ;

      if ( currentLSN >= endLSN )
      {
         PD_LOG( PDINFO, "No need to revert, currentLSN[%lld], endLSN[%lld], flie[%s]",
                 currentLSN, endLSN, logFile.path().c_str() ) ;
         goto done ;
      }

      while ( currentLSN < endLSN && !SDB_REVERT_IS_INTERRUPT )
      {
         rc = logFile.read( currentLSN, sizeof( dpsLogRecordHeader ), (CHAR*)&logHeader ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read log file[%s], lsn[%lld], rc= %d",
                    logFile.path().c_str(), currentLSN, rc ) ;
            goto error ;
         }

         if ( logHeader._lsn != currentLSN )
         {
            rc = SDB_DPS_INVALID_LSN ;
            PD_LOG( PDERROR, "Invalid LSN of log file[%s], expect lsn[%lld], real lsn[%lld]",
                    logFile.path().c_str(), currentLSN, logHeader._lsn ) ;
            goto error ;
         }

         rc = _ensureBufSize( logHeader._length ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to ensure buf size, rc= %d", rc ) ;
            goto error ;
         }

         rc = logFile.read( currentLSN, logHeader._length, _buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read log file[%s], lsn[%lld], rc= %d",
                    logFile.path().c_str(), currentLSN, rc ) ;
            goto error ;
         }

         // skip internal data, like split
         if ( OSS_BIT_TEST( logHeader._flags, DPS_FLG_NON_BS_OP ) )
         {
            currentLSN += logHeader._length ;
            continue ;
         }

         rc = _revertLogRecord( _buf, currentLSN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to revert log record, lsn[%lld], rc= %d", logHeader._lsn, rc ) ;
            goto error ;
         }

         _localInfo.parsedLogInc() ;
         currentLSN += logHeader._length ;
      }

      if ( _docBuf.size() > 0 )
      {
         rc = _writeDocs() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _globalInfoMgr->appendResultInfo( _localInfo ) ;
      if ( currentLSN < endLSN && _interrupted->fetch() > 0 )
      {
         _globalInfoMgr->appendFailedLogFile( _logFilePath ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_ensureBufSize( UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( _bufSize >= size )
      {
         goto done ;
      }

      if ( 0 != _bufSize )
      {
         SAFE_OSS_FREE( _buf ) ;
         _bufSize = 0 ;
      }

      SDB_ASSERT( NULL == _buf, "_buf should be NULL" ) ;
      _buf = (CHAR*)SDB_OSS_MALLOC( size ) ;
      if ( NULL == _buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc, size= %u", size ) ;
         goto error ;
      }
      _bufSize = size ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_revertLogRecord( const CHAR* log, DPS_LSN_OFFSET lsn )
   {
      INT32 rc                         = SDB_OK ;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log ;
      UINT16 type                      = header._type ;
      SDB_REVERT_DATA_TYPE dataType    = _options->dataType() ;

      switch( type )
      {
         case LOG_TYPE_DATA_DELETE:
            if ( dataType == SDB_REVERT_DOC || dataType == SDB_REVERT_ALL )
            {
               rc = _revertDocDelete( log, lsn ) ;
            }
            break;
         case LOG_TYPE_LOB_REMOVE:
            if ( dataType == SDB_REVERT_LOB || dataType == SDB_REVERT_ALL )
            {
               rc = _revertLobDelete( log, lsn ) ;
            }
            break ;
         default:
            break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to revert log, type= %u, rc= %d", type, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_revertDocDelete( const CHAR* log, DPS_LSN_OFFSET lsn )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *fullName = NULL ;
      bson::BSONObj originData ;
      bson::BSONObj obj ;

      rc = dpsRecord2Delete( log, &fullName, originData ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to parse log record[%lld], rc= %d", lsn, rc ) ;
         goto error ;
      }

      if ( originData.isEmpty() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Parsed log record is empty, lsn[%lld], rc= %d", lsn, rc ) ;
         goto error ;
      }

      if ( ossStrcmp( _options->targetClName().c_str(), fullName ) != 0 )
      {
         goto done ;
      }

      if ( !match( originData, _options->matcher() ) )
      {
         goto done ;
      }

      obj = _formatDocData( originData, lsn ) ;

      _localInfo.parsedDocInc() ;

      if ( _docBuf.size() >= SDB_REVERT_DOC_MAX_CACHE_NUM )
      {
         rc = _writeDocs() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _docBuf.push_back( obj ) ;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logProcessor::_revertLobDelete( const CHAR* log, DPS_LSN_OFFSET lsn )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *fullName = NULL ;
      const bson::OID *oid = NULL ;
      UINT32 sequence      = 0 ;
      UINT32 offset        = 0 ;
      UINT32 len           = 0 ;
      UINT32 hash          = 0 ;
      const CHAR *data     = NULL ;
      DMS_LOB_PAGEID page  = DMS_LOB_INVALID_PAGEID ;
      UINT32 lobPageSize   = 0 ;
      dpsLogRecordHeader recordHeader ;
      bson::BSONObjBuilder builder ;
      bson::BSONObj obj ;
      bson::OID lobId ;

      rc = dpsRecord2LobRm( log,
                            &fullName, &oid,
                            sequence, offset,
                            len, hash, &data,
                            page, &lobPageSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to parse log record[%lld], rc= %d", lsn, rc ) ;
         goto error ;
      }

      if ( ossStrcmp( _options->targetClName().c_str(), fullName ) != 0 )
      {
         goto done ;
      }

      // before 3.4.2, lobPageSize may be 0, see 6372.
      // there is no need to support versions prior to 3.4.2
      if ( 0 == lobPageSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get error lob page size from log record[%lld], rc= %d", lsn, rc ) ;
         goto error ;
      }

      lobId = *oid ;
      builder.appendOID( SDB_REVERT_MATCH_FIELD_OID, &lobId ) ;
      obj = builder.obj() ;
      if ( !match( obj, _options->matcher() ) )
      {
         goto done ;
      }

      _localInfo.parsedLobPiecesInc() ;

      rc = _writeLobPieces( *oid, sequence, data, offset, len, lsn, lobPageSize) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _localInfo.revertedLobPiecesInc() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   bson::BSONObj logProcessor::_formatDocData( const bson::BSONObj &obj, DPS_LSN_OFFSET lsn )
   {
      bson::BSONObjBuilder builder ;

      builder.appendObject( SDB_REVERT_DOC_DELETE_ENTRY, obj.objdata() ) ;
      builder.append( SDB_REVERT_DOC_DELETE_OPTYPE, SDB_REVERT_OPTYPE_DOC_DELETE ) ;
      builder.append( SDB_REVERT_DOC_DELETE_LABEL, _options->label() ) ;
      builder.append( SDB_REVERT_DOC_DELETE_LSN, (INT64)lsn ) ;
      builder.append( SDB_REVERT_DOC_DELETE_SOURCE, _logFilePath ) ;

      return builder.obj() ;
   }

   INT32 logProcessor::_writeLobPieces( const bson::OID &oid,
                                        UINT32 sequence,
                                        const CHAR *data,
                                        UINT32 offset,
                                        UINT32 len,
                                        DPS_LSN_OFFSET lsn,
                                        UINT32 lobPageSize )
   {
      INT32 rc               = SDB_OK ;
      INT64 pos              = 0 ;
      UINT32 currentSize     = 0 ;
      CHAR *buffer           = NULL ;
      ossSpinXLatch *lobLock = NULL ;
      sdbLob lob ;

      rc = _lobLockMap->getLobLock( oid, lobLock ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob lock, oid= %s, sequence= %d, rc= %d",
                 oid.str().c_str(), sequence, rc ) ;
         goto error ;
      }

      {
         ossScopedLock lock( lobLock ) ;

         if ( !_lobMetaMgr->checkLSN( oid, sequence, lsn ) )
         {
            goto done ;
         }

         rc = _outputCL->openLob( lob, oid, SDB_LOB_WRITE ) ;
         if ( SDB_FNE == rc )
         {
            rc = _outputCL->createLob( lob, &oid ) ;
            if ( SDB_OK != rc && SDB_FE != rc )
            {
               PD_LOG( PDERROR, "Failed to create lob in collection= %s, oid= %s, rc= %d",
                       _options->outputClName().c_str(), oid.str().c_str(), rc ) ;
               goto error ;
            }

            rc = lob.close() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to close lob in collection= %s, oid= %s, rc= %d",
                       _options->outputClName().c_str(), oid.str().c_str(), rc ) ;
               goto error ;
            }

            rc = _outputCL->openLob( lob, oid, SDB_LOB_WRITE ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to create lob in collection= %s, oid= %s, rc= %d",
                    _options->outputClName().c_str(), oid.str().c_str(), rc ) ;
            goto error ;
         }

         // skip lob mate data
         if ( DMS_LOB_META_SEQUENCE == sequence && 0 == offset )
         {
            len = DMS_LOB_META_LENGTH >= len ? 0 : ( len - DMS_LOB_META_LENGTH ) ;
            data = data + DMS_LOB_META_LENGTH ;
            pos = offset + sequence * lobPageSize ;
         }
         else
         {
            pos = offset + sequence * lobPageSize - DMS_LOB_META_LENGTH ;
         }

         // check if lob need to be extended
         // NOTE: lob can be automatically extended, but the meta of sharding
         //       pieces with holes is limited to 320 bytes, to avoid large sharding
         currentSize = lob.getSize() ;
         if ( _options->fillLobHole() && currentSize < pos )
         {
            UINT32 holeLen = pos - currentSize ;
            rc = _fillLobHole( lob, currentSize, holeLen ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to fill lob hole in collection= %s, offset= %d, "
                       "len= %d rc= %d",_options->outputClName().c_str(), currentSize,
                       holeLen, rc ) ;
               goto error ;
            }
         }

         rc = lob.seek( pos, SDB_LOB_SEEK_SET ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to lock and seek lob in collection= %s, oid= %s, "
                    "sequence= %d, offset= %d, len= %d, rc= %d", _options->outputClName().c_str(),
                    oid.str().c_str(), sequence, offset, len, rc ) ;
            goto error ;
         }

         rc = lob.write( data, len ) ;
         if ( SDB_OK != rc )
         {
            if ( !_options->fillLobHole() && SDB_LOB_PIECESINFO_OVERFLOW == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to write lob in collection= %s, oid= %s, sequence= %d, "
                     "offset= %d, len= %d, rc= %d", _options->outputClName().c_str(),
                     oid.str().c_str(), sequence, offset, len, rc ) ;
               goto error ;
            }
         }

         rc = lob.close() ;
         if ( SDB_OK != rc )
         {
            if ( !_options->fillLobHole() && SDB_LOB_PIECESINFO_OVERFLOW == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to close lob in collection= %s, oid= %s, rc= %d",
                        _options->outputClName().c_str(), oid.str().c_str(), rc ) ;
               goto error ;
            }
         }
      }

   done:
      if ( !lob.isClosed() )
      {
         lob.close() ;
      }
      SAFE_OSS_FREE( buffer ) ;

      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_fillLobHole( sdbLob &lob, UINT32 offset, UINT32 len )
   {
      INT32 rc      = SDB_OK ;
      INT32 holeLen = 0 ;

      if ( NULL == _lobHoleBuf )
      {
         holeLen = SDB_REVERT_LOB_MAX_FILL_SIZE ;
         _lobHoleBuf = (CHAR *)SDB_OSS_MALLOC( holeLen ) ;
         if ( NULL == _lobHoleBuf )
         {
            PD_LOG( PDERROR, "Failed to allocate buffer [%d]", holeLen ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( _lobHoleBuf, 0, holeLen ) ;
      }

      while ( len > 0 )
      {
         if ( len > SDB_REVERT_LOB_MAX_FILL_SIZE )
         {
            len -= SDB_REVERT_LOB_MAX_FILL_SIZE ;
            holeLen = SDB_REVERT_LOB_MAX_FILL_SIZE ;
         }
         else
         {
            holeLen = len ;
            len = 0 ;
         }

         rc = lob.seek( offset, SDB_LOB_SEEK_SET ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to seek lob in collection= %s, offset= %d, "
                     "len= %d rc= %d",_options->outputClName().c_str(), offset,
                     holeLen, rc ) ;
            goto error ;
         }

         rc = lob.write( _lobHoleBuf, holeLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write lob in collection= %s, rc=%d",
                     _options->outputClName().c_str(), rc ) ;
            goto error ;
         }

         offset += holeLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::_writeDocs()
   {
      INT32 rc = SDB_OK ;
      
      rc = _outputCL->insert( _docBuf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to revert delete doc record, rc=%d", rc ) ;
         goto error ;
      }
      
      _localInfo.appendRevertedDoc( _docBuf.size() ) ;
      _docBuf.clear() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::init( revertOptions &options,
                             logFileMgr &logFileMgr,
                             lobMetaMgr &lobMetaMgr,
                             lobLockMap &lobLockMap,
                             ossAtomic32 &interrupted,
                             globalInfoMgr &globalInfoMgr )
   {
      INT32 rc = SDB_OK ;

      _options    = &options ;
      _logFileMgr = &logFileMgr ;
      _lobMetaMgr = &lobMetaMgr ;
      _lobLockMap = &lobLockMap ;
      _interrupted = &interrupted ;
      _globalInfoMgr = &globalInfoMgr ;

      rc = _connect() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _getCL() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 logProcessor::run()
   {
      INT32 rc = SDB_OK ;

      _globalInfoMgr->runNumInc() ;
      while ( !_logFileMgr->empty() && !SDB_REVERT_IS_INTERRUPT )
      {
         _logFilePath = _logFileMgr->pop() ;

         rc = _revertLogFile( _logFilePath ) ;
         if ( SDB_OK != rc )
         {
            _globalInfoMgr->setGlobalRc( rc ) ;
            _globalInfoMgr->appendFailedLogFile( _logFilePath ) ;
            goto error ;
         }
      }

   done:
      _globalInfoMgr->runNumDec() ;
      return rc ;
   error:
      goto done ;
   }
}
