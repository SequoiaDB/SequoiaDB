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

   #define LOG_LOOP_BEGIN( a ) \
           for ( UINT32 i = 0; i < a; i++ ) {\

   #define LOG_LOOP_FILE ( _files.at( i ) )

   #define LOG_LOOP_END }

   #define EACH i

   #define LOG_FILE( a ) ( _files.at((a)%_logFileNum) )

   #define WORK_FILE()  ( _files.at( _work ) )

   #define UPDATE_SUB( a ) { a = ++a % _files.size();}

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

   _dpsLogFileMgr::~_dpsLogFileMgr()
   {
      LOG_LOOP_BEGIN ( _files.size() )
      {
         if ( LOG_LOOP_FILE )
            SDB_OSS_DEL LOG_LOOP_FILE ;
      }
      LOG_LOOP_END
      _files.clear();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_INIT, "_dpsLogFileMgr::init" )
   INT32 _dpsLogFileMgr::init( const CHAR *path )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_INIT );
      SDB_ASSERT( path, "path can not be NULL!") ;
      CHAR fileFullPath[ OSS_MAX_PATHSIZE+1 ] = {0} ;
      CHAR tmp[11] = { 0 } ;

      if ( ossStrlen ( path ) + ossStrlen ( DPS_LOG_FILE_PREFIX ) +
           sizeof(tmp) + 2 > OSS_MAX_PATHSIZE )
      {
         PD_LOG ( PDERROR, "Log Path is too long" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      LOG_LOOP_BEGIN( _logFileNum )
         _dpsLogFile *file = SDB_OSS_NEW _dpsLogFile();
         if ( NULL == file )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "Memory can't be allocated for dpsLogFile");
            goto error;
         }
         _files.push_back( file ) ;

         utilBuildFullPath( path, DPS_LOG_FILE_PREFIX,
                            OSS_MAX_PATHSIZE, fileFullPath ) ;
         ossSnprintf ( tmp, sizeof(tmp), "%d", i ) ;
         ossStrncat( fileFullPath, tmp, ossStrlen( tmp ) ) ;
         rc = file->init( fileFullPath, _logFileSz, _logFileNum );
         if ( rc )
         {
            PD_LOG ( PDERROR,"Failed to init log file for %d, rc = %d", i, rc ) ;
            goto error;
         }
      LOG_LOOP_END

      _analysis () ;

   done:
      PD_TRACE_EXITRC ( SDB__DPSLGFILEMGR_INIT, rc );
      return rc;
   error:
      LOG_LOOP_BEGIN ( _files.size() )
         SDB_OSS_DEL LOG_LOOP_FILE ;
      LOG_LOOP_END
      _files.clear();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR__ANLYS, "_dpsLogFileMgr::_analysis" )
   void _dpsLogFileMgr::_analysis ()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR__ANLYS );
      _dpsLogFile *file = NULL ;
      UINT32 i = 0 ;
      UINT32 beginLogID = DPS_INVALID_LOG_FILE_ID ;

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

      UINT32 tmpWork = _begin ;
      i = 0 ;

      while ( _files[tmpWork]->getIdleSize() == 0 && i < _files.size() )
      {
         _work = tmpWork ;
         tmpWork = _incFileID ( tmpWork ) ;
         ++i ;
      }

      if ( i < _files.size() &&
           _files[tmpWork]->header()._logID != DPS_INVALID_LOG_FILE_ID )
      {
         _work = tmpWork ;
         tmpWork = _incFileID ( tmpWork ) ;
         ++i ;
      }

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

      if ( _files[_work]->header()._logID != DPS_INVALID_LOG_FILE_ID )
      {
         _logicalWork = _files[_work]->header()._logID ;
      }
      _rollFlag = ( _begin == _work ? FALSE : TRUE ) ;

      PD_LOG( PDEVENT, "Analysis dps logs[begin: %u, work: %u, "
              "logicalWork: %u]", _begin, _work, _logicalWork ) ;

      PD_TRACE_EXIT ( SDB__DPSLGFILEMGR__ANLYS ) ;
   }

   DPS_LSN _dpsLogFileMgr::getStartLSN ( BOOLEAN mustExist )
   {
      DPS_LSN lsn =  LOG_FILE ( _work + 1 )->getFirstLSN ( mustExist ) ;
      if ( !lsn.invalid() )
         return lsn ;
      else
         return LOG_FILE ( _begin )->getFirstLSN ( mustExist ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGFILEMGR_FLUSH, "_dpsLogFileMgr::flush" )
   INT32 _dpsLogFileMgr::flush( _dpsMessageBlock *mb,
                                const DPS_LSN &beginLsn,
                                BOOLEAN shutdown )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLGFILEMGR_FLUSH );

      dpsLogFile *pWork = WORK_FILE() ;

      SDB_ASSERT ( shutdown || mb->length() == DPS_DEFAULT_PAGE_SIZE,
                   "mb length must be DPS_DEFAULT_PAGE_SIZE unless it's "
                   "shutdown" ) ;
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

      if ( pWork->getIdleSize() == 0 ||
           pWork->getIdleSize() == pWork->size() )
      {
         pWork->reset( _logicalWork, beginLsn.offset, beginLsn.version ) ;
      }

      rc = pWork->write ( mb->startPtr(), DPS_DEFAULT_PAGE_SIZE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write %d bytes into file, rc = %d",
                 DPS_DEFAULT_PAGE_SIZE, rc ) ;
         goto error ;
      }

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
      rc = LOG_FILE( sub )->read( lsn.offset, sizeof(dpsLogRecordHeader),
                                  (CHAR*)&head ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read log file %d, rc = %d", sub, rc ) ;
         goto error ;
      }
      if ( lsn.offset != head._lsn )
      {
         PD_LOG ( PDERROR, "Invalid LSN is read from log file, expect %lld, %d,"
                  "actual %lld, %d", lsn.offset, lsn.version, head._lsn,
                  head._version ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
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

      if ( mb->idleSize() < len )
      {
         rc = mb->extend ( len - mb->idleSize() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to extend mb, rc = %d", rc ) ;
            goto error ;
         }
      }
      ossMemcpy( mb->writePtr(), &head, sizeof(dpsLogRecordHeader ) ) ;
      mb->writePtr( mb->length() + sizeof( dpsLogRecordHeader ) ) ;
      if ( onlyHeader )
      {
         goto done ;
      }

      rc = LOG_FILE( sub )->read ( lsn.offset + sizeof(dpsLogRecordHeader ),
                                   head._length - sizeof( dpsLogRecordHeader ),
                                   mb->writePtr() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read from log file %d for %d bytes, "
                  "rc = %d", sub, head._length-sizeof(dpsLogRecordHeader),
                  rc ) ;
         mb->writePtr( mb->length() - sizeof( dpsLogRecordHeader ) ) ;
         goto error ;
      }
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
            if ( file != _work )
            {
               SDB_ASSERT( 0 == fileOffset, "File offset must be 0" ) ;
               SDB_ASSERT( file == _incFileID( _work ), "File must work + 1" ) ;
               SDB_ASSERT( 0 == _files[_work]->getIdleSize(),
                           "Idle size must be 0" ) ;
            }
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

