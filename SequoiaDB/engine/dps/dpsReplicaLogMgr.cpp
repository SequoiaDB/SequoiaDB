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

   Source File Name = dpsReplicaLogMgr.cpp

   Descriptive Name = Data Protection Service Replica Log Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log manager,
   which is the driving logic to insert log records into log buffer

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsReplicaLogMgr.hpp"
#include "dpsMergeBlock.hpp"
#include "dpsLogDef.hpp"
#include "dpsLogRecordDef.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsTransCB.hpp"

namespace engine
{
   #define WORK_PAGE ( &( _pages[_work] ) )

   #define PAGE( n ) (&( _pages [ ( DPS_SUB_BIT ? (n)&DPS_SUB_BIT : (n)%_pageNum ) ] ))

   #define SHARED_LOCK_NODES( meta )\
            {for ( UINT32 i = 0;\
                   i < (meta).pageNum; \
                   i++ ) \
            {(PAGE((meta).beginSub + i))->lockShared();}}

   #define SHARED_UNLOCK_NODES( meta ) \
           {for ( UINT32 i = 0;\
                   i < (meta).pageNum; \
                   i++ ) \
           {(PAGE((meta).beginSub + i))->unlockShared();}}

   static UINT32 DPS_SUB_BIT = 0 ;

   _dpsReplicaLogMgr::_dpsReplicaLogMgr()
   :_logger(this), _pages(NULL), _idleSize(0), _totalSize(0),
    _work(0), _pageNum(0), _queSize(0)
   {
      _begin = 0 ;
      _rollFlag = FALSE ;
      _lsn.offset = 0 ;
      _lsn.version = 0 ;
      _restoreFlag = FALSE ;

      _transCB = NULL ;
      _incVersion = FALSE ;
   }

   _dpsReplicaLogMgr::~_dpsReplicaLogMgr()
   {
      if ( NULL != _pages )
      {
         SDB_OSS_DEL []_pages;
         _pages = NULL;
      }
   }

   void _dpsReplicaLogMgr::regEventHandler( dpsEventHandler *pEventHandler )
   {
      _vecEventHandler.push_back( pEventHandler ) ;
   }

   void _dpsReplicaLogMgr::unregEventHandler( dpsEventHandler *pEventHandler )
   {
      vector< dpsEventHandler* >::iterator it = _vecEventHandler.begin() ;
      while ( it != _vecEventHandler.end() )
      {
         if ( *it == pEventHandler )
         {
            _vecEventHandler.erase( it ) ;
            break ;
         }
         ++it ;
         continue ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_INIT, "_dpsReplicaLogMgr::init" )
   INT32 _dpsReplicaLogMgr::init ( const CHAR *path, UINT32 pageNum,
                                   dpsTransCB *pTransCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_INIT );
      SDB_ASSERT ( path, "path can't be NULL" ) ;

      _transCB = pTransCB ;

      _pages = SDB_OSS_NEW _dpsLogPage[pageNum];
      if ( NULL == _pages )
      {
         rc = SDB_OOM;
         PD_LOG (PDERROR, "new _dpsLogPage[pageNum] failed!" );
         goto error;
      }
      for ( UINT32 i = 0; i < pageNum; i++ )
      {
         _pages[i].setNumber( i );
      }
      _totalSize = DPS_DEFAULT_PAGE_SIZE * pageNum;
      _idleSize.init( _totalSize );
      _pageNum = pageNum ;
      if ( ossIsPowerOf2( pageNum ) )
      {
         DPS_SUB_BIT = ~pageNum ;
      }

      rc = _logger.init( path );
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to initial log files, rc = %d", rc ) ;
         goto error;
      }

      if ( !_logger.getStartLSN( FALSE ).invalid() )
      {
         rc = _restore () ;
         if ( SDB_OK == rc )
         {
            _lastCommitted = _currentLsn ;
            PD_LOG ( PDEVENT, "Dps restore succeed, file lsn[%lld], buff "
                     "lsn[%lld], current lsn[%lld], expect lsn[%lld]",
                     _logger.getStartLSN().offset, _getStartLsn().offset,
                     _currentLsn.offset, _lsn.offset ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Dps restore failed[rc:%d]", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_INIT, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__RESTRORE, "_dpsReplicaLogMgr::_restore" )
   INT32 _dpsReplicaLogMgr::_restore ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__RESTRORE );
      _dpsLogFile *file = _logger.getWorkLogFile() ;
      DPS_LSN beginLsn = file->getFirstLSN ( FALSE ) ;
      UINT32 length = file->getLength() ;
      _dpsMessageBlock block ( sizeof ( dpsLogRecordHeader ) ) ;
      _dpsMergeBlock mergeBlock ;
      dpsLogRecordHeader &head = mergeBlock.record().head();

      _restoreFlag = TRUE ;

      if ( beginLsn.invalid() )
      {
         rc = SDB_DPS_INVALID_LSN ;
         goto error ;
      }

      _movePages( beginLsn.offset, beginLsn.version ) ;

      while ( ( beginLsn.offset % file->size() < length ) &&
              ( beginLsn.offset / file->size() % _logger.getLogFileNum() ==
                _logger.getWorkPos() ) )
      {
         rc = _logger.load ( beginLsn, &block ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "restore dps file load failed[rc:%d]", rc ) ;
            goto error ;
         }
         ossMemcpy( &head, block.offset(0), sizeof(dpsLogRecordHeader) ) ;
         mergeBlock.setRow( TRUE );
         mergeBlock.record().push( DPS_LOG_ROW_ROWDATA,
                                   block.length()- sizeof(dpsLogRecordHeader),
                                   block.offset(sizeof(dpsLogRecordHeader))) ;
         rc = merge ( mergeBlock ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "restore merge failed[rc:%d]", rc ) ;
            goto error ;
         }

         beginLsn.offset += head._length ;
         mergeBlock.clear() ;
         block.clear() ;
      }

      {
         UINT32 fileIdleSize = file->getIdleSize() +
                              (&_pages[_work])->getLength() ;
         if ( fileIdleSize % DPS_DEFAULT_PAGE_SIZE != 0 )
         {
            PD_LOG( PDERROR, "File[%s] idle size[%u] is not multi-times of "
                    "page size, cur page info[%u, %s]",
                    file->toString().c_str(), fileIdleSize,
                    _work, (&_pages[_work])->toString().c_str() ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "Idle size error" ) ;
            goto error ;
         }

         file->idleSize ( fileIdleSize ) ;
      }

   done:
      _restoreFlag = FALSE ;
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__RESTRORE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_PREPAGES, "_dpsReplicaLogMgr::preparePages" )
   INT32 _dpsReplicaLogMgr::preparePages ( dpsMergeInfo &info )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_PREPAGES );
      info.getDummyBlock().clear() ;
      dpsMergeBlock &block = info.getMergeBlock () ;
      block.pageMeta().clear() ;
      dpsLogRecordHeader &head = block.record().head() ;
      UINT32 logFileSz = _logger.getLogFileSz() ;
      BOOLEAN locked = FALSE ;

      if ( _totalSize < head._length )
      {
         PD_LOG ( PDERROR, "dps total memory size[%d] less than block size[%d]",
                  _totalSize, head._length ) ;
         rc = SDB_SYS ;
         SDB_ASSERT ( 0, "system error" ) ;
         goto error ;
      }

      if ( FALSE == _restoreFlag )
      {
         _writeMutex.get() ;
         while ( _idleSize.peek() < head._length )
         {
            PD_LOG ( PDWARNING, "No space in log buffer for %d bytes, "
                     "currently left %d bytes", head._length,
                     _idleSize.peek() ) ;
            _allocateEvent.wait ( OSS_ONE_SEC ) ;
         }
         _mtx.get();
         locked = TRUE ;
      }

      if ( block.isRow() && _lsn.offset != head._lsn)
      {
         PD_LOG( PDERROR, "lsn[%lld] of row is not equal to lsn[%lld] of local",
                 head._lsn, _lsn.offset) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( DPS_INVALID_LSN_VERSION == _lsn.version || _incVersion )
      {
         ++_lsn.version ;
         _incVersion = FALSE ;
      }

      if ( !block.isRow() )
      {
         if ( ( _lsn.offset / logFileSz ) !=
               ( _lsn.offset + head._length - 1 ) / logFileSz )
         {
            SDB_ASSERT ( !block.isRow(), "replicated log record should never"
                         " hit this part" ) ;
            UINT32 dummyLogSize = logFileSz - ( _lsn.offset % logFileSz ) ;
            SDB_ASSERT ( dummyLogSize >= sizeof ( dpsLogRecordHeader ),
                         "dummy log size is smaller than log head" ) ;
            SDB_ASSERT ( dummyLogSize % sizeof(SINT32) == 0,
                         "dummy log size is not 4 bytes aligned" ) ;

            dpsLogRecordHeader &dummyhead =
                              info.getDummyBlock().record().head() ;
            dummyhead._length = dummyLogSize ;
            dummyhead._type   = LOG_TYPE_DUMMY ;
            _allocate ( dummyhead._length, info.getDummyBlock().pageMeta() ) ;

            SHARED_LOCK_NODES ( info.getDummyBlock().pageMeta()) ;
            _push2SendQueue ( info.getDummyBlock().pageMeta() ) ;
            dummyhead._lsn = _lsn.offset ;
            dummyhead._version = _lsn.version ;
            dummyhead._preLsn = _currentLsn.offset ;
            _currentLsn = _lsn ;
            _lsn.offset += dummyhead._length ;

            if ( info.isNeedNotify() && _vecEventHandler.size() > 0 )
            {
               for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
               {
                  _vecEventHandler[i]->onPrepareLog( info.getCSLID(),
                                                     info.getCLLID(),
                                                     info.getExtentLID(),
                                                     dummyhead._lsn ) ;
               }
            }
         }

         if ( ( (_lsn.offset+head._length) / logFileSz ) !=
              ( (_lsn.offset+head._length+
                 sizeof(dpsLogRecordHeader)) / logFileSz ) )
         {
            SDB_ASSERT ( !block.isRow(), "replicated log record should never"
                         " hit this part" ) ;
            head._length = logFileSz - _lsn.offset % logFileSz ;
         }
      }
      _allocate( head._length, block.pageMeta() );

      SHARED_LOCK_NODES( block.pageMeta() ) ;
      _push2SendQueue( block.pageMeta() ) ;
      if ( !block.isRow() )
      {
         head._lsn = _lsn.offset;
         head._preLsn = _currentLsn.offset ;
         head._version = _lsn.version ;
      }
      else
      {
         SDB_ASSERT ( _lsn.offset == head._lsn, "row lsn error" ) ;
         _lsn.version = head._version ;
      }
      _currentLsn = _lsn ;
      _lsn.offset += head._length ;

      if ( info.isNeedNotify() && _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onPrepareLog( info.getCSLID(),
                                               info.getCLLID(),
                                               info.getExtentLID(),
                                               head._lsn ) ;
         }
      }

   done:
      if ( locked )
      {
         _mtx.release() ;
         _writeMutex.release() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_PREPAGES, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_WRITEDATA, "_dpsReplicaLogMgr::writeData" )
   void _dpsReplicaLogMgr::writeData ( dpsMergeInfo & info )
   {
      SDB_ASSERT ( info.getMergeBlock().pageMeta().valid(),
                   "block not prepared" ) ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_WRITEDATA );

      if ( info.hasDummy() )
      {
         _mergeLogs( info.getDummyBlock(), info.getDummyBlock().pageMeta());
         SHARED_UNLOCK_NODES( info.getDummyBlock().pageMeta() );
      }

      _mergeLogs( info.getMergeBlock(), info.getMergeBlock().pageMeta() );
      SHARED_UNLOCK_NODES( info.getMergeBlock().pageMeta() );

      if ( _transCB && _transCB->isTransOn() && !_restoreFlag )
      {
         if ( info.getMergeBlock().isRow() )
         {
            dpsLogRecord newRecord ;
            newRecord = info.getMergeBlock().record() ;
            newRecord.loadRowBody() ;
            _transCB->saveTransInfoFromLog( newRecord ) ;
         }
         else
         {
            _transCB->saveTransInfoFromLog( info.getMergeBlock().record() ) ;
         }
      }

      PD_TRACE_EXIT ( SDB__DPSRPCMGR_WRITEDATA );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_MERGE, "_dpsReplicaLogMgr::merge" )
   INT32 _dpsReplicaLogMgr::merge( _dpsMergeBlock &block )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_MERGE );
      dpsMergeInfo mergeInfo( block ) ;
      INT32 rc = preparePages ( mergeInfo ) ;
      if ( SDB_OK != rc )
      {
         goto done ;
      }
      writeData ( mergeInfo ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_MERGE, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_GETSTARTLSN, "_dpsReplicaLogMgr::getStartLsn" )
   DPS_LSN _dpsReplicaLogMgr::getStartLsn ( BOOLEAN logBufOnly )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_GETSTARTLSN ) ;
      ossScopedLock lock( &_mtx ) ;

      DPS_LSN lsn ;
      DPS_LSN memBeginLsn = _getStartLsn() ;
      if ( logBufOnly )
      {
         lsn = memBeginLsn ;
      }
      else
      {
         lsn = _logger.getStartLSN() ;
         if ( lsn.invalid() || lsn.offset > memBeginLsn.offset )
         {
            lsn = memBeginLsn ;
         }
      }

      PD_TRACE_EXIT ( SDB__DPSRPCMGR_GETSTARTLSN ) ;
      return lsn ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_GETLSNWIN, "_dpsReplicaLogMgr::getLsnWindow" )
   void _dpsReplicaLogMgr::getLsnWindow( DPS_LSN &fileBeginLsn,
                                         DPS_LSN &memBeginLsn,
                                         DPS_LSN &endLsn,
                                         DPS_LSN *expected,
                                         DPS_LSN *committed )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_GETLSNWIN );
      ossScopedLock lock( &_mtx ) ;
      memBeginLsn = _getStartLsn() ;
      fileBeginLsn = _logger.getStartLSN () ;
      if ( fileBeginLsn.invalid() || fileBeginLsn.offset > memBeginLsn.offset )
      {
         fileBeginLsn = memBeginLsn ;
      }
      endLsn = _currentLsn ;
      if ( NULL != expected)
      {
         *expected = _lsn ;
      }

      if ( NULL != committed )
      {
         *committed = _lastCommitted ;
      }
      PD_TRACE_EXIT ( SDB__DPSRPCMGR_GETLSNWIN );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__MVPAGES, "_dpsReplicaLogMgr::_movePages" )
   INT32 _dpsReplicaLogMgr::_movePages ( const DPS_LSN_OFFSET & offset, 
                                         const DPS_LSN_VER & version )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__MVPAGES );
      UINT32 page = ( offset / DPS_DEFAULT_PAGE_SIZE ) % _pageNum ;
      UINT32 pageOffset = offset % DPS_DEFAULT_PAGE_SIZE ;
      DPS_LSN invalidLsn ;

      if ( _currentLsn.invalid() ||
           offset < _getStartLsn().offset ||
           offset > _lsn.offset )
      {
         UINT32 i = 0 ;
         while ( i < _pageNum )
         {
            (&_pages[i])->setBeginLSN( invalidLsn ) ;
            (&_pages[i])->mb()->writePtr ( 0 ) ;
            ++i ;
         }

         _work = page ;
         _begin = _work ;
         _rollFlag = FALSE ;

         DPS_LSN lastLSN ;
         lastLSN.offset = offset ;
         lastLSN.version = version ;
         (&_pages[_work])->setBeginLSN( lastLSN ) ;

         _currentLsn.offset = DPS_INVALID_LSN_OFFSET ;
         _currentLsn.version = DPS_INVALID_LSN_VERSION ;
      }
      else if ( _lsn.offset > offset )
      {
         while ( _work != page )
         {
            (&_pages[_work])->setBeginLSN ( invalidLsn ) ;

            _work = _decPageID ( _work ) ;
         }

         dpsLogRecordHeader header ;
         rc = _parse ( _work, pageOffset, sizeof (dpsLogRecordHeader),
                      (CHAR*)&header ) ;
         if ( SDB_OK != rc || offset != header._lsn )
         {
            PD_LOG ( PDERROR, "Move pages failed[rc:%d], header[lsn:%lld, "
                     "preLsn:%lld, length:%d, type:%d], offset: %lld",
                     rc, header._lsn, header._preLsn, header._length,
                     header._type, offset ) ;
            if ( SDB_OK == rc )
            {
               rc = SDB_DPS_CORRUPTED_LOG ;
            }
            goto error ;
         }
         _currentLsn.offset = header._preLsn ;
         _currentLsn.version = version ;
      }

      (&_pages[_work])->mb()->writePtr ( pageOffset ) ;
      (&_pages[_work])->mb()->invalidateData() ;
      _idleSize.init ( _totalSize - pageOffset ) ;

      _lsn.offset = offset ;
      _lsn.version = version ;

      _lastCommitted = DPS_LSN() ;

   done:
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__MVPAGES, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_MOVE, "_dpsReplicaLogMgr::move" )
   INT32 _dpsReplicaLogMgr::move( const DPS_LSN_OFFSET &offset,
                                  const DPS_LSN_VER &version )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_MOVE );
      UINT32 tmpWork = 0 ;
      DPS_LSN_OFFSET tmpLsnOffset = 0 ;
      DPS_LSN_OFFSET tmpBeginOffset = 0 ;
      DPS_LSN tmpCurLsn ;
      BOOLEAN doMove = FALSE ;
      BOOLEAN locked = FALSE ;

      if ( DPS_INVALID_LSN_OFFSET == offset )
      {
         rc = SDB_DPS_MOVE_FAILED ;
         PD_LOG( PDERROR, "can not move to a invalid lsn" ) ;
         goto error ;
      }

      _writeMutex.get() ;
      while ( !_queSize.compare( 0 ) )
      {
         ossSleep ( 100 ) ;
      }
      _mtx.get() ;
      locked = TRUE ;

      tmpWork = _work ;
      tmpCurLsn = _currentLsn ;
      tmpLsnOffset = _lsn.offset ;
      tmpBeginOffset = _getStartLsn().offset ;

      doMove = TRUE ;
      if ( _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onMoveLog( offset, version,
                                            _lsn.offset, _lsn.version,
                                            DPS_BEFORE, SDB_OK ) ;
         }
      }

      _idleSize.add ( (&_pages[_work])->getLength() ) ;
      (&_pages[_work])->clear() ;

      rc = _movePages ( offset, version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( offset >= tmpBeginOffset && offset <= tmpLsnOffset
         && tmpWork == _work && !tmpCurLsn.invalid() )
      {
         goto done ;
      }

      rc = _logger.move( offset, version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( !_logger.getStartLSN().invalid() && offset < tmpBeginOffset )
      {
         rc = _restore () ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else //reset file idle size
      {
         _dpsLogFile *file = _logger.getWorkLogFile() ;
         UINT32 fileIdleSize = file->getIdleSize() +
                               (&_pages[_work])->getLength() ;
         if ( fileIdleSize % DPS_DEFAULT_PAGE_SIZE != 0 )
         {
            PD_LOG( PDERROR, "File[%s] idle size[%u] is not multi-times of "
                    "page size, cur page info[%u, %s]",
                    file->toString().c_str(), fileIdleSize,
                    _work, (&_pages[_work])->toString().c_str() ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "Idle size error" ) ;
            goto error ;
         }
         file->idleSize ( fileIdleSize ) ;
      }

   done:
      if ( doMove && _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onMoveLog( offset, version,
                                            _lsn.offset, _lsn.version,
                                            DPS_AFTER, rc ) ;
         }
      }
      if ( locked )
      {
         _mtx.release() ;
         _writeMutex.release() ;
      }
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_MOVE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__GETSTARTLSN, "_dpsReplicaLogMgr::_getStartLsn" )
   DPS_LSN _dpsReplicaLogMgr::_getStartLsn ()
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__GETSTARTLSN );
      UINT32 begin = _begin ;
      DPS_LSN lsn ;

      for ( UINT32 count = 0 ; count < _pageNum; count++ )
      {
         lsn = PAGE(begin)->getBeginLSN () ;
         if ( !lsn.invalid() )
         {
            goto done ;
         }

         if ( begin == _work )
         {
            break ;
         }

         begin = _incPageID ( begin ) ;
      }
   done :
      PD_TRACE_EXIT ( SDB__DPSRPCMGR__GETSTARTLSN );
      return lsn ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__SEARCH, "_dpsReplicaLogMgr::_search" )
   INT32 _dpsReplicaLogMgr::_search ( const DPS_LSN &lsn, _dpsMessageBlock *mb,
                                      BOOLEAN onlyHeader,
                                      UINT32 *pLength )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__SEARCH );
      dpsLogRecordHeader head ;
      UINT32 offset    = 0 ;
      UINT32 len       = 0 ;
      UINT32 pageSub   = 0 ;
      DPS_LSN beginLSN ;
      DPS_LSN lastLSN ;
      BOOLEAN mtxLocked = FALSE ;
      BOOLEAN pageLocked = FALSE ;

      pageSub = ( lsn.offset / DPS_DEFAULT_PAGE_SIZE ) % _pageNum ;
      offset  = lsn.offset % DPS_DEFAULT_PAGE_SIZE ;

      _mtx.get () ;
      mtxLocked = TRUE ;

      beginLSN = _getStartLsn() ;
      if ( beginLSN.invalid() )
      {
         PD_LOG( PDERROR, "begin lsn invalid [offset:%lld] [version:%d], "
                 "begin: %d, work: %d", beginLSN.offset, beginLSN.version,
                 _begin, _work ) ;
         rc = SDB_DPS_LOG_NOT_IN_BUF ;
         goto error ;
      }
      lastLSN = _lsn ;
      if ( 0 > lsn.compareOffset(beginLSN.offset) )
      {
         PD_LOG( PDDEBUG, "lsn %lld is smaller than membegin %lld",
                 lsn.offset, beginLSN.offset ) ;
         rc = SDB_DPS_LOG_NOT_IN_BUF ;
         goto error ;
      }
      else if ( 0 <= lsn.compareOffset(lastLSN.offset) )
      {
         rc = SDB_DPS_LSN_OUTOFRANGE ;
         goto error ;
      }

      (&_pages[pageSub])->lock() ;
      pageLocked = TRUE ;
      _mtx.release() ;
      mtxLocked = FALSE ;

      rc = _parse ( pageSub, offset, sizeof(dpsLogRecordHeader),
                   (CHAR*)&head ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to parse log record, rc = %d", rc ) ;
         goto error ;
      }
      if ( head._lsn != lsn.offset ||
           head._length < sizeof(dpsLogRecordHeader) )
      {
         PD_LOG ( PDERROR, "Unexpected LSN is read, expects %lld, %d,"
                  "actually read %lld, %d",
                  lsn.offset, lsn.version, head._lsn, head._version ) ;
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
         rc = mb->extend ( len - mb->idleSize () ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to extend mb, rc = %d", rc ) ;
            goto error ;
         }
      }

      if ( onlyHeader )
      {
         ossMemcpy( mb->writePtr(), (CHAR*)&head, len ) ;
      }
      else
      {
         rc = _parse ( pageSub, offset, len, mb->writePtr() ) ;
      }

      (&_pages[pageSub])->unlock() ;
      pageLocked = FALSE ;

      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to parse entire log, rc = %d", rc ) ;
         goto error ;
      }
      mb->writePtr ( len + mb->length () ) ;

   done :
      if ( mtxLocked )
      {
         _mtx.release () ;
      }
      if ( pageLocked )
      {
         (&_pages[pageSub])->unlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__SEARCH, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__PARSE, "_dpsReplicaLogMgr::_parse" )
   INT32 _dpsReplicaLogMgr::_parse( UINT32 sub, UINT32 offset,
                                    UINT32 len, CHAR *out )
   {
      INT32 rc            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__PARSE );
      UINT32 localSub     = sub;
      UINT32 localOffset  = offset;
      UINT32 needParseLen = len;
      UINT32 outOffset    = 0;
      SDB_ASSERT ( out, "out can't be NULL" ) ;
      if ( offset + len >= _pageNum*DPS_DEFAULT_PAGE_SIZE )
      {
         PD_LOG ( PDERROR, "offset + len is greater than buffer size, "
                  "offset = %d, len = %d", offset, len ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }
      while ( needParseLen > 0 )
      {
         UINT32 parseLen = ( DPS_DEFAULT_PAGE_SIZE - localOffset ) <
                                 needParseLen ?
                           ( DPS_DEFAULT_PAGE_SIZE - localOffset ) :
                                 needParseLen;
         ossMemcpy( out + outOffset,
                    (PAGE( localSub )->mb())->offset( localOffset ), parseLen );

         localOffset += parseLen;
         outOffset += parseLen;
         needParseLen -= parseLen;

         if ( 0 == ( DPS_DEFAULT_PAGE_SIZE - localOffset ) )
         {
            localSub = _incPageID( localSub ) ;
            localOffset = 0 ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__PARSE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_SEARCH, "_dpsReplicaLogMgr::search" )
   INT32 _dpsReplicaLogMgr::search( const DPS_LSN &minLsn,
                                    _dpsMessageBlock *mb,
                                    UINT8 type,
                                    BOOLEAN onlyHeader,
                                    UINT32 *pLength )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_SEARCH );
      if ( DPS_INVALID_LSN_OFFSET == minLsn.offset )
      {
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
      {
         rc = _search( minLsn, mb, onlyHeader, pLength );
         if ( rc )
         {
            if ( SDB_DPS_LOG_NOT_IN_BUF == rc &&
                 DPS_SEARCH_FILE & type )
            {
               rc = _logger.load( minLsn, mb, onlyHeader, pLength );
               if ( rc )
               {
                  PD_LOG ( PDINFO, "Failed to find [%lld, %d]from memory and "
                           "file, rc = %d", minLsn.offset, minLsn.version,
                           rc ) ;
                  goto error ;
               }
            }
            else
            {
               PD_LOG ( PDDEBUG, "Failed to find [%lld, %d] from memory, "
                        "rc = %d", minLsn.offset, minLsn.version, rc ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_SEARCH, rc );
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__ALLOCATE, "_dpsReplicaLogMgr::_allocate" )
   void _dpsReplicaLogMgr::_allocate( UINT32 len,
                                      dpsPageMeta &allocated )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__ALLOCATE );
      UINT32 needAlloc = len;
      UINT32 pageNum = 0;

      SDB_ASSERT( 0 != len, "can not allocate zero length" ) ;
      SDB_ASSERT ( _totalSize > len,
                  "total memory size must grater than record size" ) ;

      while ( _idleSize.peek() < needAlloc )
      {
         PD_LOG ( PDWARNING, "No space in log buffer for %d bytes, currently "
                  "left %d bytes", needAlloc, _idleSize.peek() ) ;
         _allocateEvent.wait ( OSS_ONE_SEC ) ;
      }
      allocated.offset = WORK_PAGE->getLength();
      allocated.beginSub = _work ;

      do
      {
         pageNum++ ;
         if ( 1 == pageNum )
         {
            DPS_LSN beginLSN = WORK_PAGE->getBeginLSN () ;
            if ( ( beginLSN.invalid() ) ||
                 ( _lsn.offset - beginLSN.offset > DPS_DEFAULT_PAGE_SIZE ) )
            {
               WORK_PAGE->setBeginLSN ( _lsn ) ;
            }
         }
         else
         {
            DPS_LSN invalid ;
            invalid.version = _lsn.version ;
            WORK_PAGE->setBeginLSN ( invalid ) ;
         }
         UINT32 pageIdle = WORK_PAGE->getLastSize();
         INT32 allocLen = pageIdle < needAlloc ? pageIdle : needAlloc;
         WORK_PAGE->allocate( allocLen );
         needAlloc -= allocLen;

         if ( 0 == WORK_PAGE->getLastSize() )
         {
            _work = _incPageID ( _work ) ;

            if ( !_rollFlag && _begin != _work )
            {
               _rollFlag = TRUE ;
            }
            else if ( _begin == _work && _rollFlag )
            {
               _begin = _incPageID ( _begin ) ;
            }
         }
      } while ( needAlloc > 0 ) ;

      _idleSize.sub( len ) ;
      allocated.pageNum = pageNum ;
      allocated.totalLen = len ;
      PD_TRACE_EXIT ( SDB__DPSRPCMGR__ALLOCATE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__PSH2SNDQUEUE, "_dpsReplicaLogMgr::_push2SendQueue" )
   void _dpsReplicaLogMgr::_push2SendQueue( const dpsPageMeta &allocated )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__PSH2SNDQUEUE );

      SDB_ASSERT( allocated.valid(), "impossible" ) ;
      for ( UINT32 i = 0; i < allocated.pageNum; ++i )
      {
         _dpsLogPage *page = PAGE(allocated.beginSub + i) ;
         if ( 0 == page->getLastSize() )
         {
            if ( !_restoreFlag )
            {
               _queue.push ( page ) ;
               _queSize.inc() ;
            }
            else
            {
               _idleSize.add( page->getLength() );
               page->clear() ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__DPSRPCMGR__PSH2SNDQUEUE );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__MRGLOGS, "_dpsReplicaLogMgr::_mergeLogs" )
   void _dpsReplicaLogMgr::_mergeLogs( _dpsMergeBlock &block,
                                       const dpsPageMeta &meta )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__MRGLOGS );
      UINT32 offset = meta.offset ;
      UINT32 work = meta.beginSub ;
      dpsLogRecordHeader &head = block.record().head() ;
      _mergePage( (CHAR *)&head,
                  sizeof( dpsLogRecordHeader ),
                  work,
                  offset);

      dpsLogRecord::iterator itr( &(block.record()) ) ;
      if ( block.isRow() )
      {
         BOOLEAN res = itr.next() ;
         SDB_ASSERT( res, "impossible" ) ;
         const _dpsRecordEle &dataMeta = itr.dataMeta() ;
         _mergePage( itr.value(), dataMeta.len,
                     work, offset ) ;
      }
      else
      {
         UINT32 mergeSize = 0 ;

         if ( LOG_TYPE_DUMMY == head._type )
         {
            goto done ;
         }

         while ( itr.next() )
         {
            const _dpsRecordEle &dataMeta = itr.dataMeta() ;
            SDB_ASSERT( DPS_INVALID_TAG != dataMeta.tag, "impossible" ) ;

            _mergePage( ( CHAR *)(&dataMeta), sizeof( dataMeta ),
                         work, offset ) ;
            mergeSize += sizeof( dataMeta ) ;
            _mergePage( itr.value(), dataMeta.len,
                        work, offset ) ;
            mergeSize += dataMeta.len ;
         }

         if (  mergeSize <= head._length -
                           sizeof(dpsRecordEle) -
                           sizeof(dpsLogRecordHeader))
         {
            CHAR stop[sizeof(dpsRecordEle)] = {0} ;
            _mergePage( stop, sizeof(stop), work, offset ) ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__DPSRPCMGR__MRGLOGS );
      return;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__MRGPAGE, "_dpsReplicaLogMgr::_mergePage" )
   void _dpsReplicaLogMgr::_mergePage( const CHAR *src,
                                       UINT32 len,
                                       UINT32 &workSub,
                                       UINT32 &offset )
   {
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__MRGPAGE ) ;
      UINT32 needcpy = len ;
      UINT32 srcOffset = 0 ;
      UINT32 pageIdle = 0 ;
      UINT32 cpylen = 0 ;

      while ( needcpy > 0 )
      {
         dpsLogPage *page = PAGE(workSub) ;
         pageIdle = page->getBufSize() - offset ;
         cpylen = pageIdle < needcpy ? pageIdle : needcpy ;
         page->fill( offset, src + srcOffset, cpylen ) ;

         needcpy -= cpylen ;
         srcOffset += cpylen ;
         offset += cpylen ;

         if ( offset == page->getBufSize() )
         {
            workSub++ ;
            offset = 0 ;
         }
      }

      PD_TRACE_EXIT ( SDB__DPSRPCMGR__MRGPAGE ) ;
      return;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_RUN, "_dpsReplicaLogMgr::run" )
   INT32 _dpsReplicaLogMgr::run( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_RUN );
      _dpsLogPage *page = NULL;
      if ( _queue.timed_wait_and_pop( page, OSS_ONE_SEC  ) )
      {
         if ( cb )
         {
            cb->incEventCount() ;
         }
         rc = _flushPage ( page ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to flush page, rc = %d", rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_RUN, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _dpsReplicaLogMgr::flushAll()
   {
      INT32 rc = SDB_OK ;
      rc = _flushAll() ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR__FLUSHALL, "_dpsReplicaLogMgr::_flushAll" )
   INT32 _dpsReplicaLogMgr::_flushAll()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__FLUSHALL );
      _dpsLogPage *page = NULL ;
      while ( TRUE )
      {
         if ( !_queue.try_pop( page ) )
         {
            break;
         }
         else
         {
            rc = _flushPage( page );
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to flush page, rc = %d", rc ) ;
               goto error ;
            }
            page = NULL;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__FLUSHALL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRPCMGR_TEARDOWN, "_dpsReplicaLogMgr::tearDown" )
   INT32 _dpsReplicaLogMgr::tearDown()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR_TEARDOWN );
      _dpsLogPage *page = NULL;
      rc = _flushAll() ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to flush all, rc = %d", rc ) ;
         goto error ;
      }

      if ( 0 != WORK_PAGE->getLength() )
      {
         page = WORK_PAGE;
         _work = _incPageID ( _work ) ;
         ossMemset( page->mb()->writePtr(), 0, page->getLastSize() ) ;
         _queSize.inc() ;
         rc = _flushPage( page, TRUE );
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to flush page, rc = %d", rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR_TEARDOWN, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSRPCMGR__FLUSHPAGE, "_dpsReplicaLogMgr::_flushPage" )
   INT32 _dpsReplicaLogMgr::_flushPage( _dpsLogPage *page, BOOLEAN shutdown )
   {
      INT32 rc = SDB_OK ;
      UINT32 preFileId = 0 ;
      UINT32 preLogicalFileId = 0 ;
      UINT32 curFileId = 0 ;
      UINT32 curLogicalFileId = 0 ;
      PD_TRACE_ENTRY ( SDB__DPSRPCMGR__FLUSHPAGE );
      SDB_ASSERT ( page, "page can't be NULL" ) ;
      UINT32 length = 0 ;
      page->lock();
      page->unlock();
      preFileId = _logger.getWorkPos() ;
      preLogicalFileId = _logger.getLogicalWorkPos() ;
      rc = _logger.flush( page->mb(), page->getBeginLSN(), shutdown );
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to flush page, rc = %d", rc ) ;
         goto error ;
      }
      SDB_ASSERT ( shutdown || page->getLength() == DPS_DEFAULT_PAGE_SIZE,
                   "page can't be partial during flush except shutdown" ) ;
      curFileId = _logger.getWorkPos() ;
      curLogicalFileId = _logger.getLogicalWorkPos() ;
      length = page->getLength() ;
      page->clear();
      _idleSize.add( length );
      _queSize.dec() ;
      _allocateEvent.signalAll() ;

      if ( preFileId != curFileId && _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onSwitchLogFile( preLogicalFileId,
                                                  preFileId,
                                                  curLogicalFileId,
                                                  curFileId ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSRPCMGR__FLUSHPAGE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSRPCMGR_CHECKSYNCCONTROL, "_dpsReplicaLogMgr::checkSyncControl" )
   INT32 _dpsReplicaLogMgr::checkSyncControl( UINT32 reqLen, _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSRPCMGR_CHECKSYNCCONTROL ) ;

      if ( _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            rc = _vecEventHandler[i]->canAssignLogPage( reqLen, cb ) ;
            if ( rc )
            {
               break ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DPSRPCMGR_CHECKSYNCCONTROL, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__DPSRPCMGR_COMMIT, "_dpsReplicaLogMgr::commit" )
   INT32 _dpsReplicaLogMgr::commit( BOOLEAN deeply, DPS_LSN *committedLsn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSRPCMGR_COMMIT ) ;
      _dpsLogPage *work = NULL ;

      _writeMutex.get() ;
 
      work = WORK_PAGE ;
      if ( 0 ==_lastCommitted.compare( _currentLsn ) )
      {
         goto done ;
      }

      while ( !_queSize.compare( 0 ) )
      {
         ossSleep ( 1 ) ;
      }

      work->lock() ;
      work->unlock() ;

      if ( 0 != work->getLength() )
      {
         ossMemset( work->mb()->writePtr(), 0, work->getLastSize() ) ;
         rc = _logger.flush( work->mb(), work->getBeginLSN(), TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to flush page, rc = %d", rc ) ;
            goto error ;
         }
      }

      if ( deeply )
      {
         rc = _logger.sync() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to sync log file: %d", rc ) ;
            goto error ;
         }
      }

      _mtx.get() ;
      _lastCommitted = _currentLsn ;
      _mtx.release() ;

   done:
      if ( NULL != committedLsn )
      {
          *committedLsn = _lastCommitted ;
      }
      _writeMutex.release() ;
      PD_TRACE_EXITRC( SDB__DPSRPCMGR_COMMIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

