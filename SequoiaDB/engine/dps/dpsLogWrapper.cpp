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

   Source File Name = dpsLogWrapper.cpp

   Descriptive Name = Data Protection Service Log Wrapper

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log wrapper,
   which is also called DPS Control Block

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsLogWrapper.hpp"
#include "dpsLogDef.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "pd.hpp"
#include "dpsMergeBlock.hpp"
#include "dpsOp2Record.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsLogRecordDef.hpp"
#include "pmd.hpp"
#include "dpsUtil.hpp"

namespace engine
{
   #define DPS_NO_WRITE_TIME                 ( 5000 )   // 5 seconds

   /*
      _dpsLogWrapper implement
   */
   _dpsLogWrapper::_dpsLogWrapper()
   {
      _initialized   = FALSE ;
      _dpslocal      = FALSE ;

      _syncInterval  = 0 ;
      _syncRecordNum = 0 ;
      _writeReordNum = 0 ;
      _lastWriteTick = 0 ;
      _lastSyncTime  = 0 ;
   }
   _dpsLogWrapper::~_dpsLogWrapper()
   {
      SDB_ASSERT( _vecEventHandler.size() == 0,
                  "Event handler size is not 0" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_INIT, "_dpsLogWrapper::init" )
   INT32 _dpsLogWrapper::init ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_INIT ) ;

      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;

      _dpslocal = optCB->isDpsLocal() ;
      _buf.setLogFileSz( optCB->getReplLogFileSz() ) ;
      _buf.setLogFileNum( optCB->getReplLogFileNum() ) ;

      rc = _buf.init( optCB->getReplLogPath(),
                      optCB->getReplLogBuffSize(),
                      sdbGetTransCB() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _syncInterval = optCB->getSyncInterval() ;
      _syncRecordNum = optCB->getSyncRecordNum() ;
      dpsSetTimeonFlag( optCB->logTimeOn() ) ;

      pmdGetSyncMgr()->setLogAccess( this ) ;
      pmdGetSyncMgr()->setMainUnit( this ) ;

      if ( optCB->archiveOn() )
      {
         rc = _archiver.init( this, optCB->getArchivePath() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_REGEVENTHANDLER, "_dpsLogWrapper::regEventHandler" )
   void _dpsLogWrapper::regEventHandler( dpsEventHandler *pHandler )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_REGEVENTHANDLER ) ;

      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must register in main thread" ) ;
      for ( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
      {
         SDB_ASSERT( pHandler != _vecEventHandler[ i ],
                     "Handle can't be same" ) ;
      }
      _vecEventHandler.push_back( pHandler ) ;
      _buf.regEventHandler( pHandler ) ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_REGEVENTHANDLER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_UNREGEVENTHANDLER, "_dpsLogWrapper::unregEventHandler" )
   void _dpsLogWrapper::unregEventHandler( dpsEventHandler *pHandler )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_UNREGEVENTHANDLER ) ;

      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must unregister in main thread" ) ;

      vector< dpsEventHandler* >::iterator it = _vecEventHandler.begin() ;
      while ( it != _vecEventHandler.end() )
      {
         if ( *it == pHandler )
         {
            _vecEventHandler.erase( it ) ;
            break ;
         }
         ++it ;
         continue ;
      }
      _buf.unregEventHandler( pHandler ) ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_UNREGEVENTHANDLER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_ACTIVE, "_dpsLogWrapper::active" )
   INT32 _dpsLogWrapper::active ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_ACTIVE ) ;

      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // dps log writer
      rc = pEDUMgr->startEDU( EDU_TYPE_LOGGW, (void*)this, &eduID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dps log writer failed, rc: %d", rc ) ;
         goto error ;
      }

      // dps trans rollback task
      rc = pEDUMgr->startEDU( EDU_TYPE_DPSROLLBACK, NULL, &eduID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start dps trans rollback failed, rc: %d", rc ) ;
         goto error ;
      }

      // dps log archiving
      if ( pmdGetKRCB()->getOptionCB()->archiveOn() )
      {
         rc = pEDUMgr->startEDU( EDU_TYPE_LOGARCHIVEMGR, (void*)this, &eduID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Start dps log archiving failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_ACTIVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsLogWrapper::deactive ()
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_FINI, "_dpsLogWrapper::fini" )
   INT32 _dpsLogWrapper::fini ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_FINI ) ;

      if ( pmdGetKRCB()->getOptionCB()->archiveOn() )
      {
         rc = _archiver.fini() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _buf.fini() ;

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_FINI, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dpsLogWrapper::onConfigChange ()
   {
      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;
      _dpslocal = optCB->isDpsLocal() ;
      _syncInterval = optCB->getSyncInterval() ;
      _syncRecordNum = optCB->getSyncRecordNum() ;
      dpsSetTimeonFlag( optCB->logTimeOn() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_SEARCH, "_dpsLogWrapper::search" )
   INT32 _dpsLogWrapper::search( const DPS_LSN &minLsn,
                                 _dpsMessageBlock *mb,
                                 UINT8 type,
                                 INT32 maxNum,
                                 INT32 maxTime,
                                 INT32 maxSize )
   {
      SDB_ASSERT ( _initialized, "shouldn't call search without init" ) ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_SEARCH ) ;

      INT32 rc = SDB_OK ;
      UINT32 length = 0 ;
      DPS_LSN searchLsn = minLsn ;
      UINT64 bTime = 0 ;

      if ( maxTime > 0 )
      {
         bTime = (UINT64)time( NULL ) ;
      }

      while( TRUE )
      {
         rc = _buf.search( searchLsn, mb, type, FALSE, &length ) ;
         if ( rc )
         {
            break ;
         }
         searchLsn.offset += length ;

         if ( maxNum > 0 )
         {
            --maxNum ;
         }
         if ( maxSize > 0 )
         {
            maxSize = (UINT32)maxSize > length ? maxSize - length : 0 ;
         }

         /// max num check
         if ( 0 == maxNum )
         {
            break ;
         }
         /// max size check
         if ( 0 == maxSize )
         {
            break ;
         }
         /// max time check
         if ( maxTime > 0 && time( NULL ) - bTime >= (UINT32)maxTime )
         {
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_SEARCH, rc ) ;
      return rc ;
   }

   INT32 _dpsLogWrapper::searchHeader( const DPS_LSN &lsn,
                                       _dpsMessageBlock *mb,
                                       UINT8 type )
   {
      SDB_ASSERT ( _initialized, "shouldn't call search without init" ) ;
      return _buf.search( lsn, mb, type, TRUE ) ;
   }

   DPS_LSN _dpsLogWrapper::getStartLsn ( BOOLEAN logBufOnly )
   {
      if ( !_initialized )
      {
         DPS_LSN lsn ;
         return lsn ;
      }
      return _buf.getStartLsn ( logBufOnly ) ;
   }

   DPS_LSN_OFFSET _dpsLogWrapper::readOldestBeginLsnOffset() const
   {
      return _buf.readOldestBeginLsnOffset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_GETCURRENTLSN, "_dpsLogWrapper::getCurrentLsn" )
   DPS_LSN _dpsLogWrapper::getCurrentLsn()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_GETCURRENTLSN ) ;
      DPS_LSN lsn = _buf.currentLsn() ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_GETCURRENTLSN ) ;
      return lsn ;
   }

   void _dpsLogWrapper::getLsnWindow( DPS_LSN &beginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed )
   {
      if ( _initialized )
      {
         DPS_LSN memLsn ;
         _buf.getLsnWindow( beginLsn, memLsn, endLsn, pExpectLsn, committed ) ;
      }
   }

   void _dpsLogWrapper::getLsnWindow( DPS_LSN &fileBeginLsn,
                                      DPS_LSN &memBeginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed )
   {
      if ( _initialized )
      {
         _buf.getLsnWindow( fileBeginLsn, memBeginLsn, endLsn,
                            pExpectLsn, committed ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_EXPECTLSN, "_dpsLogWrapper::expectLsn" )
   DPS_LSN _dpsLogWrapper::expectLsn()
   {
      DPS_LSN lsn ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_EXPECTLSN ) ;
      if ( _initialized )
      {
         lsn = _buf.expectLsn() ;
      }

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_EXPECTLSN ) ;
      return lsn ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_COMMITLSN, "_dpsLogWrapper::commitLsn" )
   DPS_LSN _dpsLogWrapper::commitLsn()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_COMMITLSN ) ;
      DPS_LSN lsn = _buf.commitLsn() ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_COMMITLSN ) ;
      return lsn ;
   }

   INT32 _dpsLogWrapper::move( const DPS_LSN_OFFSET &offset,
                               const DPS_LSN_VER &version )
   {
      /// make sure the version is correct
      if ( DPS_INVALID_LSN_OFFSET != offset &&
           DPS_INVALID_LSN_VERSION == version )
      {
         return _buf.move( offset, DPS_INVALID_LSN_VERSION + 1 ) ;
      }
      return _buf.move( offset, version ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_WRITEDATA, "_dpsLogWrapper::writeData" )
   void _dpsLogWrapper::writeData ( dpsMergeInfo & info )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_WRITEDATA ) ;

      _lastWriteTick = pmdGetDBTick() ;
      ++_writeReordNum ;

      _buf.writeData( info ) ;

      IExecutor *cb = info.getEDUCB() ;

      /// insert lsn
      if ( cb )
      {
         if ( info.hasDummy() )
         {
            cb->insertLsn( info.getDummyBlock().record().head()._lsn ) ;
         }
         cb->insertLsn( info.getMergeBlock().record().head()._lsn ) ;
      }

      /// notify
      if ( _vecEventHandler.size() > 0 && info.isNeedNotify() )
      {
         DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
         if ( info.hasDummy() )
         {
            offset = info.getDummyBlock().record().head()._lsn ;
            for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
            {
               _vecEventHandler[i]->onWriteLog( offset ) ;
            }
         }
         offset = info.getMergeBlock().record().head()._lsn ;
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            _vecEventHandler[i]->onWriteLog( offset ) ;
         }
      }

      // it is transaction operations
      if ( info.isTransEnabled() && cb &&
           DPS_INVALID_TRANS_ID != cb->getTransID() )
      {
         UINT64 transID = cb->getTransID() ;
         dpsTransCB * transCB = sdbGetTransCB() ;
         cb->setCurTransLsn( info.getMergeBlock().record().head()._lsn ) ;

         if ( transCB->isFirstOp( transID ) )
         {
            transCB->clearFirstOpTag( transID ) ;
            cb->setTransID( transID ) ;
         }
      }

      // reset
      info.resetInfoEx() ;

      PD_TRACE_EXIT( SDB__DPSLGWRAPP_WRITEDATA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_COMPLETEOPR, "_dpsLogWrapper::completeOpr" )
   INT32 _dpsLogWrapper::completeOpr( _pmdEDUCB * cb, INT32 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_COMPLETEOPR ) ;

      if ( w > 1 && cb && 0 != cb->getLsnCount() &&
           _vecEventHandler.size() > 0 )
      {
         for( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
         {
            rc = _vecEventHandler[i]->onCompleteOpr( cb, w ) ;
            if ( rc )
            {
               break ;
            }
         }
         cb->resetLsn() ;
      }

      PD_TRACE_EXITRC ( SDB__DPSLGWRAPP_COMPLETEOPR, rc ) ;
      return rc ;
   }

   // record a row
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_RECDROW, "_dpsLogWrapper::recordRow" )
   INT32 _dpsLogWrapper::recordRow( const CHAR *row, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGWRAPP_RECDROW );
      if ( !_initialized )
      {
         goto done;
      }
      {
         SDB_ASSERT( NULL != row, "row should not be NULL!") ;
         _dpsMergeBlock block ;
         dpsLogRecord &record = block.record();
         dpsLogRecordHeader &header = record.head() ;
         ossMemcpy( &header, row, sizeof(dpsLogRecordHeader) );
         block.setRow( TRUE ) ;
         rc = record.push( DPS_LOG_ROW_ROWDATA,
                           header._length -  sizeof(dpsLogRecordHeader),
                           row + sizeof(dpsLogRecordHeader)) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to push row to record:%d", rc ) ;
            goto error;
         }

         _lastWriteTick = pmdGetDBTick() ;
         ++_writeReordNum ;

         rc = _buf.merge( block );
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSLGWRAPP_RECDROW, rc );
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dpsLogWrapper::isInRestore()
   {
      return _buf.isInRestore() ;
   }

   INT32 _dpsLogWrapper::commit( BOOLEAN deeply, DPS_LSN *committedLsn )
   {
      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      _lastSyncTime = t.time * 1000 + t.microtm / 1000 ;
      /// clear write info
      _writeReordNum = 0 ;

      return _buf.commit( deeply, committedLsn ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_PREPARE, "prepare" )
   INT32 _dpsLogWrapper::prepare( dpsMergeInfo &info )
   {
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_PREPARE ) ;
      INT32 rc = SDB_OK ;
      if ( !_initialized )
      {
         goto done;
      }

      rc = _buf.preparePages( info ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to prepare pages, rc = %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSLGWRAPP_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dpsLogWrapper::isClosed() const
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGWRAPP_CANSYNC, "_dpsLogWrapper::canSync" )
   BOOLEAN _dpsLogWrapper::canSync( BOOLEAN &force ) const
   {
      BOOLEAN needSync = FALSE ;
      force = FALSE ;
      PD_TRACE_ENTRY( SDB__DPSLGWRAPP_CANSYNC ) ;

      if ( !_buf.hasDirty() )
      {
         /// nothing
      }
      else if ( _syncRecordNum > 0 && _writeReordNum >= _syncRecordNum )
      {
         PD_LOG( PDDEBUG, "Write record number[%u] more than threshold[%u]",
                 _writeReordNum, _syncRecordNum ) ;
         force = TRUE ;
         needSync = TRUE ;
      }
      else if ( pmdGetTickSpanTime( _lastWriteTick ) < DPS_NO_WRITE_TIME )
      {
         /// nothing
      }
      else if ( _syncInterval > 0 )
      {
         ossTimestamp tm ;
         ossGetCurrentTime( tm ) ;
         UINT64 curTime = tm.time * 1000 + tm.microtm / 1000 ;

         if ( curTime - _lastSyncTime >= _syncInterval )
         {
            PD_LOG( PDDEBUG, "Time interval threshold tiggered, "
                    "CurTime:%llu, LastSyncTime:%llu, SyncInterval:%u",
                    curTime, _lastSyncTime, _syncInterval ) ;
            needSync = TRUE ;
         }
      }

      PD_TRACE1 ( SDB__DPSLGWRAPP_CANSYNC, PD_PACK_INT( needSync ) ) ;
      PD_TRACE_EXIT( SDB__DPSLGWRAPP_CANSYNC ) ;
      return needSync ;
   }

   INT32 _dpsLogWrapper::sync( BOOLEAN force,
                               BOOLEAN sync,
                               IExecutor *cb )
   {
      return commit( sync, NULL ) ;
   }

   void _dpsLogWrapper::lock()
   {
   }

   void _dpsLogWrapper::unlock()
   {
   }

   /*
      get dps cb
   */
   SDB_DPSCB* sdbGetDPSCB()
   {
      static SDB_DPSCB s_dpscb ;
      return &s_dpscb ;
   }

}

