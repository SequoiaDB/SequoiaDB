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

   Source File Name = dpsLogFileMgr.cpp

   Descriptive Name = Data Protection Service Log File Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains log file manager.


   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "stdio.h"
#include "dpsLogFileMgr.hpp"
#include "pd.hpp"
#include "dpsLogPage.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsLogDef.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "utilStr.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

namespace engine
{

   #define LOG_FILE( a ) ( _files.at((a)%_logFileNum) )

   #define WORK_FILE()  ( _files.at( _work ) )

   #define UPDATE_SUB( a ) { a = ++a % _files.size();}

   // constructor
   _dpsLogFileMgr::_dpsLogFileMgr( class _dpsReplicaLogMgr *replMgr ):_work(0),
   _logicalWork(0)
   {
      SDB_ASSERT ( replMgr, "replMgr can't be NULL" ) ;
      _replMgr    = replMgr ;
      _logFileSz  = PMD_DFT_LOG_FILE_SZ * DPS_LOG_FILE_SIZE_UNIT ;
      _logFileNum = PMD_DFT_LOG_FILE_NUM ;

      _begin = 0 ;
      _rollFlag = FALSE ;
   }

   // destructor
   _dpsLogFileMgr::~_dpsLogFileMgr()
   {
      fini() ;
   }

   void _dpsLogFileMgr::fini()
   {
      _clear() ;
   }

   void _dpsLogFileMgr::_clear()
   {
      for ( UINT32 i = 0; i < _files.size() ; i++ )
      {
         if ( _files[ i ] )
         {
            SDB_OSS_DEL _files[ i ] ;
         }
      }
      _files.clear();
   }

   // initialize log file manager
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_INIT, "_dpsLogFileMgr::init" )
   INT32 _dpsLogFileMgr::init( const CHAR *path,
                               dpsMetaFileContent &content )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_INIT );
      SDB_ASSERT( path, "path can not be NULL!") ;

      BOOLEAN needRetry = FALSE ;
      INT32 length = -1 ;
      CHAR fileFullPath[ OSS_MAX_PATHSIZE+1 ] = {0} ;
      // temp buffer stores log file sequence up to 0xFFFFFFFF, which is
      // 4294967295 ( 10 bytes )
      CHAR tmp[11] = { 0 } ;

      // make sure path + OSS_FILE_SEP + DPS_LOG_FILE_PREFIX + xxx + 0
      // is less or equal to OSS_MAX_PATHSIZE
      if ( ossStrlen ( path ) + ossStrlen ( DPS_LOG_FILE_PREFIX ) +
           sizeof(tmp) + 2 > OSS_MAX_PATHSIZE )
      {
         PD_LOG ( PDERROR, "Log Path is too long" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   retry:
      for ( UINT32 i = 0; i < _logFileNum ; i++ )
      {
         // memory is free in destructor, or by end of error in this function
         _dpsLogFile *file = SDB_OSS_NEW _dpsLogFile();
         if ( NULL == file )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "Memory can't be allocated for dpsLogFile");
            goto error;
         }
         // push log file to vector array
         _files.push_back( file ) ;

         utilBuildFullPath( path, DPS_LOG_FILE_PREFIX,
                            OSS_MAX_PATHSIZE, fileFullPath ) ;
         ossSnprintf ( tmp, sizeof(tmp), "%d", i ) ;
         ossStrncat( fileFullPath, tmp, ossStrlen( tmp ) ) ;

         /// calc file's valid length
         if ( content.isStatusValid() )
         {
            /// work file
            if ( i == content._workFile )
            {
               length = ( content._curLsnOffset + content._curLsnLength ) %
                        _logFileSz ;
            }
            else if ( content._workFile >= content._beginFile )
            {
               /// [ begin, work ) is full
               if ( i >= content._beginFile && i < content._workFile )
               {
                  length = _logFileSz ;
               }
               /// other is empty
               else
               {
                  length = 0 ;
               }
            }
            else
            {
               /// ( work, begin ) is empty
               if ( i > content._workFile && i < content._beginFile )
               {
                  length = 0 ;
               }
               /// other is full
               else
               {
                  length = _logFileSz ;
               }
            }
         }
         else
         {
            length = -1 ;
         }

         // initialize log file for each newly created one
         // we set readonly to FALSE, so that each log file is opened with
         // WRITEONLY option
         rc = file->init( fileFullPath, _logFileSz, _logFileNum,
                          length, &needRetry ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to init log file for %d, rc: %d",
                     i, rc ) ;
            goto error;
         }
         else if ( needRetry )
         {
            goto check_retry ;
         }
      }

      _analysis( content, needRetry ) ;
      if ( needRetry )
      {
         goto check_retry ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DPSLGFILEMGR_INIT, rc );
      return rc ;
   error:
      _clear() ;
      goto done ;
   check_retry:
      _clear() ;
      content.resetStatus() ;
      needRetry = FALSE ;
      goto retry ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR__ANLYS, "_dpsLogFileMgr::_analysis" )
   void _dpsLogFileMgr::_analysis ( const dpsMetaFileContent &content,
                                    BOOLEAN &needRetry )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR__ANLYS );
      _dpsLogFile *file = NULL ;
      UINT32 i = 0 ;
      UINT32 beginLogID = DPS_INVALID_LOG_FILE_ID ;
      UINT32 tmpWork = 0 ;

      needRetry = FALSE ;

      //find begin
      while ( i < _files.size() )
      {
         file = _files[i] ;
         if ( file->header()._logID == DPS_INVALID_LOG_FILE_ID )
         {
            ++i ;
            continue ;
         }

         if ( beginLogID == DPS_INVALID_LOG_FILE_ID ||
              DPS_FILEID_COMPARE( file->header()._logID, beginLogID ) < 0 )
         {
            beginLogID = file->header()._logID ;
            _begin = i ;
         }

         ++i ;
      }

      /// check _begin is the same with content
      if ( content.isStatusValid() && _begin != content._beginFile )
      {
         PD_LOG( PDWARNING, "Calc begin file(%d) is not the same with "
                            "meta file(%d), will retry restore without "
                            "meta file",
                 _begin, content._beginFile ) ;
         needRetry = TRUE ;
         goto done ;
      }

      //find work
      tmpWork = _begin ;
      i = 0 ;

      // Skip full log files
      while ( _files[tmpWork]->getIdleSize() == 0 && i < _files.size() )
      {
         _work = tmpWork ;
         tmpWork = _incFileID ( tmpWork ) ;
         ++i ;
      }

      // Find the last non-full log file
      // If i == _files.size() means all log file are full, keep the working log
      // file as the last full log file, and let the next flush to move working
      // log file to the next log file
      if ( i < _files.size() &&
           _files[tmpWork]->header()._logID != DPS_INVALID_LOG_FILE_ID )
      {
         _work = tmpWork ;
         tmpWork = _incFileID ( tmpWork ) ;
         ++i ;
      }

      /// check _work is the same with content
      if ( content.isStatusValid() && _work != content._workFile )
      {
         PD_LOG( PDWARNING, "Calc work file(%d) is not the same with "
                            "meta file(%d), will retry restore without "
                            "meta file",
                 _work, content._workFile ) ;
         needRetry = TRUE ;
         goto done ;
      }

      //reset other
      while ( i < _files.size () )
      {
         if ( _files[tmpWork]->header()._logID != DPS_INVALID_LOG_FILE_ID )
         {
            _files[tmpWork]->reset( DPS_INVALID_LOG_FILE_ID,
                                    DPS_INVALID_LSN_OFFSET,
                                    DPS_INVALID_LSN_VERSION ) ;
         }
         tmpWork = _incFileID( tmpWork ) ;
         ++i ;
      }

      //find logical work
      if ( _files[_work]->header()._logID != DPS_INVALID_LOG_FILE_ID )
      {
         _logicalWork = _files[_work]->header()._logID ;
      }
      _rollFlag = ( _begin == _work ? FALSE : TRUE ) ;

      PD_LOG( PDEVENT, "Analysis dps logs[begin: %u, work: %u, "
              "logicalWork: %u]", _begin, _work, _logicalWork ) ;

   done:
      PD_TRACE_EXIT ( SDB__DPSLGFILEMGR__ANLYS ) ;
   }

   // get the first LSN in log file manager
   DPS_LSN _dpsLogFileMgr::getStartLSN ( BOOLEAN mustExist )
   {
      // if the next file got first LSN, that means we have looped at least 1
      // round, so the earliest LSN will be the next one
      DPS_LSN lsn =  LOG_FILE ( _work + 1 )->getFirstLSN ( mustExist ) ;
      if ( !lsn.invalid() )
         return lsn ;
      // otherwise if the LSN for next file is invalid, that means we haven't
      // write into that file yet, so the earliest LSN will be in file 0
      else
         return LOG_FILE ( _begin )->getFirstLSN ( mustExist ) ;
   }

   // write a log buffer into file
   // Note this function will change file pointer so the mb should always be
   // full ( length() = DPS_DEFAULT_PAGE_SIZE ), except during database shutdown
   // period. Otherwise it will cause serious log corruption
   // Again, NEVER flush a partial fulled page into file unless it's during
   // database shutdown
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_FLUSH, "_dpsLogFileMgr::flush" )
   INT32 _dpsLogFileMgr::flush( _dpsMessageBlock *mb,
                                const DPS_LSN &beginLsn,
                                BOOLEAN shutdown )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_FLUSH );

      dpsLogFile *pWork = WORK_FILE() ;

      // first we get the number of bytes need to write into log file
      // by our design, the mb->length() should always be same as
      // DPS_DEFAULT_PAGE_SIZE, except during tearDown phase
      SDB_ASSERT ( shutdown || mb->length() == DPS_DEFAULT_PAGE_SIZE,
                   "mb length must be DPS_DEFAULT_PAGE_SIZE unless it's "
                   "shutdown" ) ;
      // since we always write every dps page, so we shouldn't write out of
      // bound, so we will hit idleSize = 0 when log file is filled up
      if ( pWork->getIdleSize() == 0 )
      {
         _work = _incFileID ( _work ) ;
         pWork = WORK_FILE() ;
         _incLogicalFileID () ;

         if ( !_rollFlag && _begin != _work )
         {
            _rollFlag = TRUE ;
         }
         else if ( _begin == _work && _rollFlag )
         {
            _begin = _incFileID ( _begin ) ;
         }
      }

      // empty file or full file(roll over)
      if ( pWork->getIdleSize() == 0 ||
           pWork->getIdleSize() == pWork->size() )
      {
         pWork->reset( _logicalWork, beginLsn.offset, beginLsn.version ) ;
      }

      // write into log file for page size
      rc = pWork->write ( mb->startPtr(), DPS_DEFAULT_PAGE_SIZE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write %d bytes into file, rc = %d",
                 DPS_DEFAULT_PAGE_SIZE, rc ) ;
         goto error ;
      }

      /// if the page is not whole, reset file pointer to the page begin pos
      if ( DPS_DEFAULT_PAGE_SIZE != mb->length() )
      {
         pWork->idleSize( pWork->getIdleSize() +
                          DPS_DEFAULT_PAGE_SIZE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DPSLGFILEMGR_FLUSH, rc );
      return rc;
   error:
      goto done;
   }

   // retrieve lsn, find the log record and fill up mb
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_LOAD, "_dpsLogFileMgr::load" )
   INT32 _dpsLogFileMgr::load( const DPS_LSN &lsn, _dpsMessageBlock *mb,
                               BOOLEAN onlyHeader,
                               UINT32 *pLength )
   {
      INT32 rc      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_LOAD );
      SDB_ASSERT ( mb, "mb can't be NULL" ) ;
      UINT32 sub    = ( UINT32 )( lsn.offset / _logFileSz  % _files.size() ) ;
      dpsLogRecordHeader head ;
      UINT32 len = 0 ;
      // read log head
      rc = LOG_FILE( sub )->read( lsn.offset, sizeof(dpsLogRecordHeader),
                                  (CHAR*)&head ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read log file %d, rc = %d", sub, rc ) ;
         goto error ;
      }
      // get the lsn
      if ( lsn.offset != head._lsn )
      {
          /// to do , head version is not correct now.
         PD_LOG ( PDERROR, "Invalid LSN is read from log file, expect %lld, %d,"
                  "actual %lld, %d", lsn.offset, lsn.version, head._lsn,
                  head._version ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
      // sanity check, make sure lsn size must be greater than header size
      if ( head._length < sizeof(dpsLogRecordHeader) )
      {
         PD_LOG ( PDERROR, "LSN length[%u] is smaller than dps log head",
                  head._length ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( pLength )
      {
         *pLength = head._length ;
      }

      if ( onlyHeader )
      {
         len = sizeof( dpsLogRecordHeader ) ;
      }
      else
      {
         len = head._length ;
      }

      // make sure we have enough space
      if ( mb->idleSize() < len )
      {
         rc = mb->extend ( len - mb->idleSize() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to extend mb, rc = %d", rc ) ;
            goto error ;
         }
      }
      // copy head info memory
      ossMemcpy( mb->writePtr(), &head, sizeof(dpsLogRecordHeader ) ) ;
      // update write ptr
      mb->writePtr( mb->length() + sizeof( dpsLogRecordHeader ) ) ;
      // if only header, don't read body
      if ( onlyHeader )
      {
         goto done ;
      }

      // read body
      rc = LOG_FILE( sub )->read ( lsn.offset + sizeof(dpsLogRecordHeader ),
                                   head._length - sizeof( dpsLogRecordHeader ),
                                   mb->writePtr() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read from log file %d for %d bytes, "
                  "rc = %d", sub, head._length-sizeof(dpsLogRecordHeader),
                  rc ) ;
         // rollback write ptr
         mb->writePtr( mb->length() - sizeof( dpsLogRecordHeader ) ) ;
         goto error ;
      }
      // update write ptr
      mb->writePtr ( mb->length() + head._length -
                     sizeof( dpsLogRecordHeader ) ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DPSLGFILEMGR_LOAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_MOVE, "_dpsLogFileMgr::move" )
   INT32 _dpsLogFileMgr::move( const DPS_LSN_OFFSET &offset,
                               const DPS_LSN_VER &version )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_MOVE );
      UINT32 i = 0 ;
      UINT32 file = ( offset /_logFileSz ) % _logFileNum ;
      UINT32 fileOffset = offset  % _logFileSz ;
      INT32 signFlag = -1 ;

      if ( _files[_work]->header()._logID != DPS_INVALID_LOG_FILE_ID &&
           offset > _files[_work]->getFirstLSN().offset +
           _files[_work]->getValidLength() )
      {
         signFlag = 1 ;
      }

      while ( i < _logFileNum )
      {
         if ( _files[_work]->header()._logID != DPS_INVALID_LOG_FILE_ID &&
             ( _files[_work]->getFirstLSN().offset <= offset &&
               offset <= _files[_work]->getFirstLSN().offset +
               _files[_work]->getValidLength() ) )
         {
            // at the end of file, need set idle to 0
            if ( file != _work )
            {
               SDB_ASSERT( 0 == fileOffset, "File offset must be 0" ) ;
               SDB_ASSERT( file == _incFileID( _work ), "File must work + 1" ) ;
               SDB_ASSERT( 0 == _files[_work]->getIdleSize(),
                           "Idle size must be 0" ) ;
            }
            // not end of file
            else
            {
               _files[_work]->idleSize ( _logFileSz - fileOffset ) ;
               _files[_work]->invalidateData() ;
            }
            _logicalWork = _files[_work]->header()._logID ;
            break ;
         }

         if ( _files[_work]->header()._logID != DPS_INVALID_LOG_FILE_ID )
         {
            rc = _files[_work]->reset ( DPS_INVALID_LOG_FILE_ID,
                                        DPS_INVALID_LSN_OFFSET,
                                        DPS_INVALID_LSN_VERSION ) ;
         }

         if ( SDB_OK != rc )
         {
            break ;
         }

         _work = signFlag == -1 ? _decFileID ( _work ) : _incFileID ( _work ) ;
         ++i ;
      }

      //out of all dps files lsn range
      if ( i == _logFileNum )
      {
         _begin = file ;
         _work = file ;
         _rollFlag = FALSE ;
         _logicalWork = DPS_LSN_2_FILEID( offset, _logFileSz ) ;
         rc = _files[_work]->reset ( _logicalWork, offset, version ) ;
         _files[_work]->idleSize ( _logFileSz - fileOffset ) ;
      }

      PD_TRACE_EXITRC ( SDB__DPSLGFILEMGR_MOVE, rc );
      return rc ;
   }

   UINT32 _dpsLogFileMgr::_incFileID ( UINT32 fileID )
   {
      ++fileID ;
      if ( fileID >= _logFileNum )
      {
         fileID = 0 ;
      }

      return fileID ;
   }

   UINT32 _dpsLogFileMgr::_decFileID ( UINT32 fileID )
   {
      if ( 0 == fileID )
      {
         fileID = _logFileNum - 1 ;
      }
      else
      {
         --fileID ;
      }

      return fileID ;
   }

   void _dpsLogFileMgr::_incLogicalFileID ()
   {
      ++_logicalWork ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_SYNC, "_dpsLogFileMgr::sync" )
   INT32 _dpsLogFileMgr::sync()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSLGFILEMGR_SYNC ) ;

      /// flush files from the oldest file
      UINT32 j = _work + 1 ;
      for ( UINT32 i = 0 ; i <= _files.size(); ++i, ++j )
      {
         _dpsLogFile *file = LOG_FILE( j ) ;
         if ( !file->isDirty() )
         {
            continue ;
         }
         rc = file->sync() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to sync log file: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGFILEMGR_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

