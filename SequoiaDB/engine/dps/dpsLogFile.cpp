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

   Source File Name = dpsLogFile.cpp

   Descriptive Name = Data Protection Service Log File

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains code logic for
   DPS transaction log file basic operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsLogFile.hpp"
#include "dpsLogPage.hpp"
#include "dpsLogFileMgr.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsLogRecord.hpp"
#include "pmdStartup.hpp"

namespace engine
{
   #define DPS_LOGFILE_READ_TIMEOUT          ( 30000 )

   _dpsLogFile::_dpsLogFile()
   {
      _file = NULL ;
      _fileSize = 0 ;
      _fileNum  = 0 ;
      _idleSize = 0 ;
      _inRestore= FALSE ;
      _dirty = FALSE ;
   }

   _dpsLogFile::~_dpsLogFile()
   {
      if ( NULL != _file )
      {
         close();
         SDB_OSS_DEL _file;
      }

      _file = NULL;
   }

   string _dpsLogFile::toString() const
   {
      stringstream ss ;
      ss << "FileSize: " << _fileSize
         << ", IdleSize: " << _idleSize
         << ", LogID: " << _logHeader._logID
         << ", FirstLSNV: " << _logHeader._firstLSN.version
         << ", FirstLSNO: " << _logHeader._firstLSN.offset ;
      return ss.str() ;
   }

   DPS_LSN _dpsLogFile::getFirstLSN ( BOOLEAN mustExist )
   {
      if ( _logHeader._logID != DPS_INVALID_LOG_FILE_ID
           && ( ( mustExist && _logHeader._firstLSN.offset % _fileSize <
                  _fileSize - _idleSize ) || !mustExist ) )
      {
         return _logHeader._firstLSN ;
      }
      return DPS_LSN () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE_INIT, "_dpsLogFile::init" )
   INT32 _dpsLogFile::init( const CHAR *path, UINT32 size, UINT32 fileNum )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE_INIT ) ;
      BOOLEAN created = FALSE ;

      SDB_ASSERT ( 0 == ( _fileSize % DPS_DEFAULT_PAGE_SIZE ),
                   "Size must be multiple of DPS_DEFAULT_PAGE_SIZE bytes" ) ;

      _fileSize = size ;
      _fileNum  = fileNum ;
      _idleSize = _fileSize ;
      _path = string( path ) ;

      _file = SDB_OSS_NEW _OSS_FILE();
      if ( !_file )
      {
         rc = SDB_OOM;
         PD_LOG ( PDERROR, "new _OSS_FILE failed!" );
         goto error;
      }

      if ( SDB_OK == ossAccess( path ) )
      {
         rc = ossOpen ( path, OSS_READWRITE|OSS_SHAREWRITE, OSS_RWXU, *_file ) ;
         if ( rc == SDB_OK )
         {
            rc = _restore () ;
            if ( rc == SDB_OK )
            {
               UINT32 startOffset = 0 ;
               if ( DPS_INVALID_LSN_OFFSET != _logHeader._firstLSN.offset )
               {
                  startOffset = (UINT32)( _logHeader._firstLSN.offset %
                                          _fileSize ) ;
               }
               PD_LOG ( PDEVENT, "Restore dps log file[%s] succeed, "
                        "firstLsn[%lld], idle space: %u, start offset: %d",
                        path, getFirstLSN().offset, getIdleSize(),
                        startOffset ) ;
               goto done ;
            }
            else
            {
               close () ;
               PD_LOG ( PDEVENT, "Restore dps log file[%s] failed[rc:%d]",
                        path, rc ) ;
               goto error ;
            }
         }
      }

      if ( SDB_OK == ossAccess( path ) )
      {
         rc = ossDelete ( path );
         if ( SDB_IO == rc )
         {
            PD_LOG ( PDERROR, "Failed to delete file at %s", path ) ;
            goto error;
         }
      }

      rc = ossOpen( path, OSS_CREATEONLY |OSS_READWRITE | OSS_SHAREWRITE,
                    OSS_RWXU, *_file );

      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to open log file %s, rc = %d", path, rc ) ;
         goto error;
      }

      created = TRUE ;

      rc = ossExtendFile( _file, (SINT64)_fileSize + DPS_LOG_HEAD_LEN );
      if ( rc )
      {
         close() ;
         PD_LOG ( PDERROR, "Failed to extend log file size to %d, rc = %d",
                 size + DPS_LOG_HEAD_LEN, rc ) ;
         goto error;
      }

      _initHead ( DPS_INVALID_LOG_FILE_ID ) ;
      rc = _flushHeader () ;
      if ( rc )
      {
         close () ;
         PD_LOG ( PDERROR, "Failed to flush header, rc = %d", rc ) ;
         goto error ;
      }
      rc = ossSeek ( _file, DPS_LOG_HEAD_LEN, OSS_SEEK_SET ) ;
      if ( rc )
      {
         close() ;
         PD_LOG ( PDERROR, "Failed to seek to %d offset in log file, rc = %d",
                 DPS_LOG_HEAD_LEN, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE_INIT, rc );
      return rc;
   error:
      if ( NULL != _file )
      {
         SDB_OSS_DEL _file;
         _file = NULL ;
      }
      if ( created )
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = ossDelete( path ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to remove new file[%s], rc:%d",
                    path, rc ) ;
         }
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE__RESTRORE, "_dpsLogFile::_restore" )
   INT32 _dpsLogFile::_restore ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE__RESTRORE );
      INT64 fileSize = 0 ;
      UINT64 offSet = 0 ;
      UINT64 baseOffset = 0 ;
      dpsLogRecordHeader lsnHeader ;
      CHAR *lastRecord = NULL ;
      UINT32 lastLen = 0 ;

      _inRestore = TRUE ;

      rc = ossGetFileSize( _file, &fileSize ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( fileSize < (INT64)( _fileSize + sizeof(dpsLogHeader) ) )
      {
         PD_LOG ( PDERROR, "DPS file size[%d] is smaller than config[%d]",
                  fileSize - sizeof(dpsLogHeader), _fileSize ) ;
         rc = SDB_DPS_FILE_SIZE_NOT_SAME ;
         goto error ;
      }

      rc = _readHeader() ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Fail to read dps file header[rc:%d]", rc ) ;
         goto error ;
      }

      if ( ossStrncmp( _logHeader._eyeCatcher, DPS_LOG_HEADER_EYECATCHER,
                       sizeof( _logHeader._eyeCatcher ) ) != 0 )
      {
         PD_LOG( PDERROR, "DPS file eye catcher error" ) ;
         rc = SDB_DPS_FILE_NOT_RECOGNISE ;
         goto error ;
      }
      else if ( _logHeader._fileSize != 0 &&
                _logHeader._fileSize != _fileSize )
      {
         PD_LOG( PDERROR, "DPS file's meta size[%d] is not the same with "
                 "config[%d]", _logHeader._fileSize, _fileSize ) ;
         rc = SDB_DPS_FILE_SIZE_NOT_SAME ;
         goto error ;
      }
      else if ( _logHeader._fileNum != 0 &&
                _logHeader._fileNum != _fileNum )
      {
         PD_LOG( PDERROR, "DPS file's meta file num[%d] is not the same with "
                 "config[%d]", _logHeader._fileNum, _fileNum ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( fileSize > (INT64)( _fileSize + sizeof(dpsLogHeader) ) )
      {
         PD_LOG( PDERROR, "DPS file real size[%d] is not the same with "
                 "config[%d]", fileSize - sizeof(dpsLogHeader),
                 _fileSize ) ;
         if ( !pmdGetStartup().isOK() )
         {
            rc = ossTruncateFile( _file, _fileSize + sizeof(dpsLogHeader) ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Tuncate dps file to config size failed, "
                       "rc: %d", rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Tuncate dps file to config size[%d]",
                    _fileSize ) ;
         }
         else
         {
            goto error ;
         }
      }

      PD_LOG ( PDEVENT, "Header info[first lsn:%d.%lld, logID:%d]",
               _logHeader._firstLSN.version, _logHeader._firstLSN.offset,
               _logHeader._logID ) ;

      if ( _logHeader._version != DPS_LOG_FILE_VERSION1 )
      {
         _logHeader._version = DPS_LOG_FILE_VERSION1 ;
         _logHeader._fileSize = _fileSize ;
         _logHeader._fileNum  = _fileNum ;

         rc = _flushHeader() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to flush header, rc: %d", rc ) ;
      }

      if ( _logHeader._logID == DPS_INVALID_LOG_FILE_ID ||
           _logHeader._firstLSN.invalid() ||
           DPS_LSN_2_FILEID( _logHeader._firstLSN.offset, _fileSize ) !=
           _logHeader._logID )
      {
         _logHeader._logID = DPS_INVALID_LOG_FILE_ID ;
         _logHeader._firstLSN.version = DPS_INVALID_LSN_VERSION ;
         _logHeader._firstLSN.offset = DPS_INVALID_LSN_OFFSET ;
         goto done ;
      }

      offSet = _logHeader._firstLSN.offset % _fileSize ;
      baseOffset = _logHeader._firstLSN.offset - offSet ;

      while ( offSet < _fileSize )
      {
         rc = read ( offSet + baseOffset , sizeof (dpsLogRecordHeader),
                     (CHAR*)&lsnHeader ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to read lsn header[offset:%lld,rc:%d]",
                     offSet, rc ) ;
            goto error ;
         }

         if ( lsnHeader._lsn != offSet + baseOffset )
         {
            PD_LOG ( PDEVENT, "LSN is not the same[%lld!=%lld]",
                     lsnHeader._lsn, offSet + baseOffset ) ;
            break ;
         }
         else if ( offSet + lsnHeader._length > _fileSize )
         {
            PD_LOG ( PDEVENT, "LSN length[%d] is over the file "
                     "size[offSet:%lld]", lsnHeader._length, offSet ) ;
            break ;
         }
         else if ( lsnHeader._length < sizeof (dpsLogRecordHeader) )
         {
            PD_LOG ( PDEVENT, "LSN length[%d] less than min[%d], invalid LSN",
                     lsnHeader._length, sizeof (dpsLogRecordHeader) ) ;
            break ;
         }
         else if ( lsnHeader._length > DPS_RECORD_MAX_LEN )
         {
            PD_LOG( PDEVENT, "LSN length[%d] more than max[%d], invalid LSN",
                    lsnHeader._length, DPS_RECORD_MAX_LEN ) ;
            break ;
         }
         else if ( lsnHeader._length % sizeof( UINT32 ) != 0 )
         {
            PD_LOG( PDEVENT, "LSN length[%d] is not 4 bytes aligned, "
                    "invalid LSN", lsnHeader._length ) ;
            break ;
         }

         offSet += lsnHeader._length ;
         lastLen = lsnHeader._length ;
      }

      if ( 0 < lastLen && 0 < offSet )
      {
         _dpsLogRecord lr ;
         lastRecord = ( CHAR * )SDB_OSS_MALLOC( lastLen ) ;
         if ( NULL == lastRecord )
         {
            PD_LOG( PDERROR, "failed to allocate mem.") ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = read( offSet + baseOffset - lastLen,
                    lastLen,
                    lastRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read dps record[%lld, rc:%d]",
                    offSet, rc ) ;
            goto error ;
         }

         rc = lr.load( lastRecord ) ;
         if ( SDB_DPS_CORRUPTED_LOG == rc )
         {
            offSet -= lastLen ;
            rc = SDB_OK ;
            const dpsLogRecordHeader *corruptedHeader =
                           ( const dpsLogRecordHeader * )lastRecord ;
            PD_LOG( PDEVENT, "last log record(lsn:%lld) is corrupted.",
                    corruptedHeader->_lsn ) ;

            if ( 0 == offSet )
            {
               _logHeader._firstLSN.offset = DPS_INVALID_LSN_OFFSET ;
               _logHeader._firstLSN.version = DPS_INVALID_LSN_VERSION ;
            }
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to load record log:%d", rc ) ;
            goto error ;
         }
      }

      _idleSize = _fileSize - offSet ;

   done:
      _inRestore = FALSE ;
      SAFE_OSS_FREE( lastRecord ) ;
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE__RESTRORE, rc );
      return rc ;
   error:
      goto done ;
   }

   UINT32 _dpsLogFile::getValidLength() const
   {
      if ( DPS_INVALID_LSN_OFFSET == _logHeader._firstLSN.offset )
      {
         return _fileSize - _idleSize ;
      }
      else
      {
         return _fileSize - _idleSize -
               ( _logHeader._firstLSN.offset % _fileSize ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE_RESET, "_dpsLogFile::reset" )
   INT32 _dpsLogFile::reset ( UINT32 logID, const DPS_LSN_OFFSET &offset,
                              const DPS_LSN_VER &version )
   {
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE_RESET );
      if ( DPS_INVALID_LOG_FILE_ID != logID )
      {
         SDB_ASSERT ( DPS_LSN_2_FILEID( offset, _fileSize ) == logID ,
                      "logical log file id error" ) ;
         _logHeader._firstLSN.offset= offset ;
         _logHeader._firstLSN.version = version ;
      }
      else
      {
         _logHeader._firstLSN.version = DPS_INVALID_LSN_VERSION ;
         _logHeader._firstLSN.offset = DPS_INVALID_LSN_OFFSET ;
      }
      _logHeader._logID = logID ;
      _idleSize = _fileSize ;
      _dirty = FALSE ;

      INT32 rc = _flushHeader () ;
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE_RESET, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE__FLUSHHD, "_dpsLogFile::_flushHeader" )
   INT32 _dpsLogFile::_flushHeader ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE__FLUSHHD );
      SINT64 writtenLen = 0;
      UINT32 written = 0 ;
      CHAR *buff = (CHAR*)&_logHeader ;

      while ( written < DPS_LOG_HEAD_LEN )
      {
         rc = ossSeekAndWrite( _file, written, &buff[written],
                               DPS_LOG_HEAD_LEN - written, &writtenLen ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to write into file header, rc = %d", rc ) ;
            goto error;
         }
         written += (UINT32)writtenLen ;
      }

      _dirty = TRUE ;
   done:
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE__FLUSHHD, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE__RDHD, "_dpsLogFile::_readHeader" )
   INT32 _dpsLogFile::_readHeader ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE__RDHD );
      SINT64 readLen = 0 ;
      SINT64 read = 0 ;
      CHAR *buff = (CHAR*)&_logHeader ;

      while ( read < DPS_LOG_HEAD_LEN )
      {
         rc = ossSeekAndRead ( _file, read, &buff[read],
                              DPS_LOG_HEAD_LEN-read, &readLen ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,  "Failed to ossSeekAndRead, rc = %d", rc ) ;
            goto error;
         }
         read += ( UINT32 )readLen;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE__RDHD, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE_WRITE, "_dpsLogFile::write" )
   INT32 _dpsLogFile::write( const CHAR *content, UINT32 len )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE_WRITE );
      _dirty = TRUE ;

      if ( len <= _idleSize && _idleSize <= _fileSize )
      {
         SINT64 writtenLen = 0 ;
         UINT32 written = 0 ;

         while ( written < len )
         {
            rc = ossSeekAndWrite( _file, DPS_LOG_HEAD_LEN + _fileSize -
                                  _idleSize + written, &content[written],
                                  len - written, &writtenLen ) ;
            if ( SDB_OK == rc )
            {
               written += (UINT32)writtenLen ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to write page data into file[%s], "
                        "written: %u, len: %u, rc = %d",
                        toString().c_str(), written, len, rc ) ;
               goto error;
            }
         }
         _idleSize -= len ;
      }
      else if ( len > _idleSize )
      {
         PD_LOG( PDERROR, "Write page data failed, len[%u] grater than "
                 "idle size in log file[%s]", len, toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         PD_LOG( PDERROR, "Wrong idle in log file[%s]", toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      _writeEvent.signalAll() ;

   done:
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE_WRITE, rc );
      return rc;
   error:
      ossPanic() ;
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE_READ, "_dpsLogFile::read" )
   INT32 _dpsLogFile::read ( const DPS_LSN_OFFSET &lOffset, UINT32 len, CHAR *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLOGFILE_READ );
      SINT64 readLen = 0;
      UINT32 read = 0 ;
      UINT32 readTimerCounter = 0 ;
      UINT32 offset = lOffset % _fileSize ;
      SDB_ASSERT ( offset + len <= _fileSize,
                   "Read out of range" ) ;
      if ( lOffset < _logHeader._firstLSN.offset ||
           lOffset > _logHeader._firstLSN.offset + _fileSize )
      {
         PD_LOG ( PDERROR, "Unable to find LSN %lld in the file", lOffset ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
      while ( !_inRestore && offset + len > getLength() )
      {
         if ( SDB_OK == _writeEvent.wait( OSS_ONE_SEC ) )
         {
            continue ;
         }
         readTimerCounter += OSS_ONE_SEC ;
         if ( readTimerCounter >= DPS_LOGFILE_READ_TIMEOUT )
         {
            break ;
         }
      }

      while ( read < len )
      {
         rc = ossSeekAndRead ( _file, offset + DPS_LOG_HEAD_LEN + read,
                               &buf[read], len-read, &readLen ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to ossSeekAndRead, rc = %d", rc ) ;
            goto error;
         }
         read += ( UINT32 )readLen;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DPSLOGFILE_READ, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dpsLogFile::close()
   {
      return ossClose( *_file );
   }

   INT32 _dpsLogFile::invalidateData()
   {
      INT32 rc = SDB_OK ;
      dpsLogRecordHeader header ;
      const CHAR *pData = ( const CHAR* )&header ;
      UINT32 len = sizeof( header ) ;
      UINT32 idleSize = _idleSize ;

      if ( 0 == idleSize )
      {
         goto done ;
      }
      else if ( len <= idleSize && idleSize <= _fileSize )
      {
         _dirty = TRUE ;

         SINT64 writtenLen = 0 ;
         UINT32 written = 0 ;

         while ( written < len )
         {
            rc = ossSeekAndWrite( _file, DPS_LOG_HEAD_LEN + _fileSize -
                                  idleSize + written, &pData[written],
                                  len - written, &writtenLen ) ;
            if ( SDB_OK == rc )
            {
               written += (UINT32)writtenLen ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to write dummy data to file[%s], "
                        "written: %u, len: %u, idleSize: %u , rc = %d",
                        toString().c_str(), written, len, idleSize, rc ) ;
               goto error;
            }
         }
      }
      else
      {
         PD_LOG( PDERROR, "Wrong idle in log file[%s]", toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLOGFILE_SYNC, "_dpsLogFile::sync" )
   INT32 _dpsLogFile::sync()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLOGFILE_SYNC ) ;
      if ( NULL == _file )
      {
         PD_LOG( PDERROR, "file has not been initialized yet" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      rc = ossFsync( _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to sync file, file no:%d, rc:%d",
                 _fileNum, rc ) ;
         goto error ;
      }      
      _dirty = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLOGFILE_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

