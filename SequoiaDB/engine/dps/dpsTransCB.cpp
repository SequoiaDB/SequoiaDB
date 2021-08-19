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

   Source File Name = dpsTransCB.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dpsTransCB.hpp"
#include "dpsTransLockMgr.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "pmdDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsTransLockCallback.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdStartup.hpp"

namespace engine
{

   dpsTransCB::dpsTransCB()
   :_TransIDL48Cur( 1 ) ,
    _MapMutex( MON_LATCH_DPSTRANSCB_MAPMUTEX ),
    _CBMapMutex( MON_LATCH_DPSTRANSCB_CBMAPMUTEX ),
    _lsnMapMutex( MON_LATCH_DPSTRANSCB_LSNMAPMUTEX ),
    _hisMutex( MON_LATCH_DPSTRANSCB_HISMUTEX ),
    _maxFileSizeMutex( MON_LATCH_DPSTRANSCB_MAXFILESIZEMUTEX ),
    _reservedRBSpace( 0 ) ,
    _reservedSpace( 0 )
   {
      _TransIDH16          = 0;
      _isOn                = FALSE ;
      _doRollback          = FALSE ;
      _isNeedSyncTrans     = TRUE ;
      _logFileTotalSize    = 0 ;
      _transLockMgr        = NULL ;
      _indexLockMgr        = NULL ;
      _oldVCB              = NULL;
      _maxLRSize1          = 0 ;
      _maxLRSize2          = 0 ;
      _maxLRLSN1           = DPS_INVALID_LSN_OFFSET ;
      _maxLRLSN2           = DPS_INVALID_LSN_OFFSET ;

      _pEventHandler       = NULL ;
   }

   dpsTransCB::~dpsTransCB()
   {
   }

   SDB_CB_TYPE dpsTransCB::cbType () const
   {
      return SDB_CB_TRANS ;
   }

   const CHAR* dpsTransCB::cbName () const
   {
      return "TRANSCB" ;
   }

   INT32 dpsTransCB::init ()
   {
      INT32 rc = SDB_OK ;
      DPS_LSN_OFFSET startLsnOffset = DPS_INVALID_LSN_OFFSET ;

      _isOn = pmdGetOptionCB()->transactionOn() ;
      _rollbackEvent.signal() ;

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      // only enable/setup old version CB if transaction is turned on
      // Each session decide when to setup/use the old copy based on
      // isolation level (TRANS_ISOLATION_RC)
      if( _isOn )
      {
         _TransIDL48Cur.init( ossRand() ) ;

         // create trans lock manager
         _transLockMgr = SDB_OSS_NEW dpsTransLockManager( LOCKMGR_TRANS_LOCK ) ;

         if ( !_transLockMgr )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         // Initialize trans lock manager
         rc = _transLockMgr->init( DPS_TRANS_LOCKBUCKET_SLOTS_MAX, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to initialize lock manager, rc: %d",
                    rc ) ;
            goto error ;
         }
        
         _oldVCB = SDB_OSS_NEW oldVersionCB() ;
         if ( !_oldVCB )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         rc = _oldVCB->init() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init OldVersionCB, rc: %d", rc ) ;
            goto error ;
         }
      }

      // if enabled dps
      if ( pmdGetKRCB()->isCBValue( SDB_CB_DPS ) &&
           !pmdGetKRCB()->isRestore() )
      {
         UINT64 logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
         UINT32 logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
         _logFileTotalSize = logFileSize * logFileNum ;

         startLsnOffset = sdbGetDPSCB()->readOldestBeginLsnOffset() ;
         if ( _isOn && startLsnOffset != DPS_INVALID_LSN_OFFSET &&
              SDB_ROLE_STANDALONE != pmdGetDBRole() )
         {
            rc = syncTransInfoFromLocal( startLsnOffset ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to sync trans info from local, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         setIsNeedSyncTrans( FALSE ) ;

         // if have trans info, need log
         if ( getTransMap()->size() > 0 )
         {
            PD_LOG( PDEVENT, "Restored trans info, have %d trans not "
                    "be complete, the oldest lsn offset is %lld",
                    getTransMap()->size(), getOldestBeginLsn() ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsTransCB::active ()
   {
      return SDB_OK ;
   }

   INT32 dpsTransCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 dpsTransCB::fini ()
   {
      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      if ( _transLockMgr )
      {
         _transLockMgr->fini() ;
         SDB_OSS_DEL _transLockMgr ;
         _transLockMgr = NULL ;
      }

      if ( _indexLockMgr )
      {
         _indexLockMgr->fini() ;
         SDB_OSS_DEL _indexLockMgr ;
         _indexLockMgr = NULL ;
      }

      if ( _oldVCB )
      {
         _oldVCB->fini() ;
         SDB_OSS_DEL _oldVCB ;
         _oldVCB = NULL ;
      }

      clearTransInfo() ;

      return SDB_OK ;
   }

   void dpsTransCB::onConfigChange()
   {
   }

   DPS_TRANS_ID dpsTransCB::allocTransID( BOOLEAN isAutoCommit )
   {
      DPS_TRANS_ID temp = 0 ;

      do {
         temp = DPS_TRANS_GET_SN ( _TransIDL48Cur.inc() ) ;
      } while( 0 == temp ) ;

      temp = temp | _TransIDH16 | DPS_TRANSID_FIRSTOP_BIT ;

      if ( isAutoCommit )
      {
         DPS_TRANS_SET_AUTOCOMMIT( temp ) ;
      }

      return temp ;
   }

   void dpsTransCB::onRegistered( const MsgRouteID &nodeID )
   {
      _TransIDH16 = (DPS_TRANS_ID)nodeID.columns.nodeID << 48 ;
   }

   void dpsTransCB::onPrimaryChange( BOOLEAN primary,
                                     SDB_EVENT_OCCUR_TYPE occurType )
   {
      // change to primary, start trans rollback
      if ( primary )
      {
         if ( SDB_EVT_OCCUR_BEFORE == occurType )
         {
            _doRollback = TRUE ;
            _rollbackEvent.reset() ;
         }
         else
         {
            startRollbackTask() ;
         }
      }
      // change to secondary, stop trans rollback
      else
      {
         if ( SDB_EVT_OCCUR_AFTER == occurType )
         {
            stopRollbackTask() ;
            termAllTrans() ;
         }
      }
   }

   void dpsTransCB::setEventHandler( dpsTransEvent *pEventHandler )
   {
      _pEventHandler = pEventHandler ;
   }

   dpsTransEvent* dpsTransCB::getEventHandler()
   {
      return _pEventHandler ;
   }

   DPS_TRANS_ID dpsTransCB::getRollbackID( DPS_TRANS_ID transID )
   {
      return transID | DPS_TRANSID_ROLLBACKTAG_BIT ;
   }

   DPS_TRANS_ID dpsTransCB::getTransID( DPS_TRANS_ID rollbackID )
   {
      return DPS_TRANS_GET_ID( rollbackID ) ;
   }

   //TODO:  implement
   // In order to find the system lowTran, we just need to use the smaller
   // head of _TransMap and _hisTransStatus
   DPS_TRANS_ID dpsTransCB::getLowTran( )
   {
      return DPS_INVALID_TRANS_ID ;
   }

   // FIXME: Guomin to implement the proper one
   BOOLEAN dpsTransCB::transIDLessThan( DPS_TRANS_ID tidL, DPS_TRANS_ID tidR )
   {
      return ( DPS_TRANS_GET_SN(tidL) < DPS_TRANS_GET_SN(tidR) ) ;
   }

   BOOLEAN dpsTransCB::isRollback( DPS_TRANS_ID transID )
   {
      if ( transID & DPS_TRANSID_ROLLBACKTAG_BIT )
      {
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN dpsTransCB::isFirstOp( DPS_TRANS_ID transID )
   {
      return DPS_TRANS_IS_FIRSTOP( transID ) ? TRUE : FALSE ;
   }

   void dpsTransCB::clearFirstOpTag( DPS_TRANS_ID &transID )
   {
      DPS_TRANS_CLEAR_FIRSTOP( transID ) ;
   }

   BOOLEAN dpsTransCB::isRBPending( DPS_TRANS_ID transID )
   {
      return DPS_TRANS_IS_RBPENDING( transID ) ? TRUE : FALSE ;
   }

   BOOLEAN dpsTransCB::hasRBPendingTrans()
   {
      ossScopedLock _lock( &_MapMutex ) ;
      for ( TRANS_MAP::iterator iter = _TransMap.begin() ;
            iter != _TransMap.end() ;
            ++ iter )
      {
         if ( DPS_INVALID_LSN_OFFSET != iter->second._curLSNWithRBPending )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   INT32 dpsTransCB::startRollbackTask()
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *pEduMgr = NULL ;
      _isNeedSyncTrans = FALSE ;
      _doRollback = TRUE ;
      _rollbackEvent.reset() ;
      pEduMgr = pmdGetKRCB()->getEDUMgr() ;
      eduID = pEduMgr->getSystemEDU( EDU_TYPE_DPSROLLBACK ) ;
      if ( PMD_INVALID_EDUID != eduID )
      {
         rc = pEduMgr->postEDUPost( eduID, PMD_EDU_EVENT_ACTIVE,
                                    PMD_EDU_MEM_NONE, NULL ) ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      if ( rc )
      {
         _doRollback = FALSE ;
         _rollbackEvent.signalAll() ;
      }
      return rc ;
   }

   INT32 dpsTransCB::stopRollbackTask()
   {
      _doRollback = FALSE ;
      _rollbackEvent.signalAll() ;
      return SDB_OK ;
   }

   INT32 dpsTransCB::waitRollback( UINT64 millicSec )
   {
      return _rollbackEvent.wait( millicSec ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SVTRANSINFO, "dpsTransCB::updateTransInfo" )
   void dpsTransCB::updateTransInfo( DPS_TRANS_ID transID,
                                     DPS_LSN_OFFSET lsnOffset,
                                     INT32 status )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSCB_SVTRANSINFO ) ;

      if ( DPS_INVALID_TRANS_ID != transID &&
           !sdbGetDPSCB()->isInRestore() )
      {
         DPS_LSN_OFFSET lastLsn = DPS_INVALID_LSN_OFFSET ;
         BOOLEAN rbPending = isRBPending( transID ) ;
         transID = getTransID( transID ) ;
         TRANS_MAP::iterator it ;

         ossScopedLock _lock( &_MapMutex ) ;

         it = _TransMap.find( transID ) ;

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            // invalid-lsn means the transaction is complete
            if ( it != _TransMap.end() )
            {
               lastLsn = it->second._lsn ;
               if ( rbPending )
               {
                  // just check the tag, current pending LSN may not reset
                  // ( it should be reset by the tag )
                  PD_LOG( PDWARNING, "Transaction [%llu] is still rollback "
                          "pending", transID ) ;
                  SDB_ASSERT( FALSE, "transaction is rollback pending" ) ;
               }
               _TransMap.erase( it ) ;
            }
         }
         else
         {
            if ( it != _TransMap.end() )
            {
               updateTransInfo( it->second, status, lsnOffset, rbPending ) ;
            }
            else
            {
               SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;
               _TransMap[ transID ] = dpsTransBackInfo( lsnOffset, status ) ;
            }
         }

         /// add to his trans
         if ( DPS_INVALID_LSN_OFFSET != lastLsn )
         {
            addHisTrans( transID, status, lastLsn ) ;
         }
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SVTRANSINFO ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSINFO, "dpsTransCB::addTransInfo" )
   void dpsTransCB::addTransInfo( DPS_TRANS_ID transID,
                                  DPS_LSN_OFFSET lsnOffset,
                                  INT32 status )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSINFO ) ;

      TRANS_MAP::iterator it ;

      BOOLEAN rbPending = isRBPending( transID ) ;
      transID = getTransID( transID ) ;

      ossScopedLock _lock( &_MapMutex ) ;

      it = _TransMap.find( transID ) ;
      if ( it == _TransMap.end() )
      {
         SDB_ASSERT( !rbPending, "should not be rollback pending" ) ;
         // it is means transaction is synchronous by log if transID is exist
         _TransMap[ transID ] = dpsTransBackInfo( lsnOffset, status ) ;
      }
      else
      {
         updateTransInfo( it->second, status, lsnOffset, rbPending ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_ADDTRANSINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO, "dpsTransCB::updateTransInfo" )
   void dpsTransCB::updateTransInfo( dpsTransBackInfo &transInfo,
                                     INT32 status,
                                     DPS_LSN_OFFSET lsn,
                                     BOOLEAN rbPending )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO ) ;

      if ( rbPending )
      {
         // if it is rollback pending, save current relatedLSN as pendingLSN
         // and do not move transaction's LSN ( it is the last related LSN
         // which is not pending )
         // NOTE: in consult rollback mode, actually we don't know the last non
         // pending LSN to reset the transaction's LSN, but the consult
         // rollback will rollback to a non pending LSN, so it is safe to reset
         transInfo._curLSNWithRBPending = lsn ;
         transInfo._curNonPendingLSN.insert( lsn ) ;
      }
      else
      {
         // if it is not rollback pending, reset current pending LSN if needed
         transInfo._lsn = lsn ;
         if ( DPS_INVALID_LSN_OFFSET != transInfo._curLSNWithRBPending )
         {
            transInfo._curLSNWithRBPending = DPS_INVALID_LSN_OFFSET ;
            transInfo._curNonPendingLSN.clear() ;
         }
      }
      transInfo._status = status ;

      PD_TRACE_EXIT( SDB_DPSTRANSCB_UPDATETRANSINFO_INFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_UPDATETRANSSTATUS, "dpsTransCB::updateTransStatus" )
   void dpsTransCB::updateTransStatus( DPS_TRANS_ID transID,
                                       INT32 status )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_UPDATETRANSSTATUS ) ;

      transID = getTransID( transID ) ;

      ossScopedLock _lock( &_MapMutex ) ;

      TRANS_MAP::iterator iter = _TransMap.find( transID ) ;
      if ( _TransMap.end() != iter )
      {
         iter->second._status = status ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSCB_UPDATETRANSSTATUS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSCB, "dpsTransCB::addTransCB" )
   BOOLEAN dpsTransCB::addTransCB( DPS_TRANS_ID transID, _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      BOOLEAN hasInsert = FALSE ;
      {
         transID = getTransID( transID ) ;
         ossScopedLock _lock( &_CBMapMutex ) ;
         hasInsert = _cbMap.insert( std::make_pair( transID, eduCB ) ).second ;
      }
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      return hasInsert ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_DELTRANSCB, "dpsTransCB::delTransCB" )
   void dpsTransCB::delTransCB( DPS_TRANS_ID transID )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_DELTRANSCB ) ;

      TRANS_CB_MAP::iterator it ;
      transID = getTransID( transID ) ;

      ossScopedLock _lock( &_CBMapMutex ) ;
      it = _cbMap.find( transID ) ;
      if ( it != _cbMap.end() )
      {
         _cbMap.erase( it ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_DELTRANSCB ) ;
   }

   void dpsTransCB::dumpTransEDUList( TRANS_EDU_LIST & eduList )
   {
      ossScopedLock _lock( &_CBMapMutex ) ;
      TRANS_CB_MAP::iterator iter = _cbMap.begin() ;
      while( iter != _cbMap.end() )
      {
         eduList.push( iter->second->getID() ) ;
         ++iter ;
      }
   }

   TRANS_MAP *dpsTransCB::getTransMap()
   {
      return &_TransMap;
   }

   void dpsTransCB::cloneTransMap( TRANS_MAP &result )
   {
      TRANS_MAP::iterator it = _TransMap.begin() ;
      while ( it != _TransMap.end() )
      {
         result[ it->first ] = it->second ;
         ++it ;
      }
   }

   UINT32 dpsTransCB::getTransCBSize ()
   {
      ossScopedLock _lock( &_CBMapMutex ) ;
      return _cbMap.size() ;
   }

   void dpsTransCB::clearTransInfo()
   {
      _TransMap.clear();
      _cbMap.clear();
      _beginLsnIdMap.clear();
      _idBeginLsnMap.clear();

      clearHisTrans() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ROLLBACKTRANSINFO, "dpsTransCB::rollbackTransInfoFromLog" )
   INT32 dpsTransCB::rollbackTransInfoFromLog( _dpsLogWrapper *dpsCB,
                                               const DPS_LSN &expectLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ROLLBACKTRANSINFO ) ;

      DPS_LSN currentLSN ;

      if ( !isTransOn() ||
           isNeedSyncTrans() ||
           NULL == dpsCB ||
           expectLSN.invalid() )
      {
         goto done ;
      }

      currentLSN.offset = dpsCB->getCurrentLsn().offset ;
      if ( DPS_INVALID_LSN_OFFSET == currentLSN.offset )
      {
         goto done ;
      }

      // reverse loop LSN until meets expected LSN
      while ( currentLSN.compareOffset( expectLSN ) >= 0 &&
              !isNeedSyncTrans() )
      {
         dpsMessageBlock mb ;
         dpsLogRecord record ;

         rc = dpsCB->search( currentLSN, &mb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to search LSN [ offset: %llu ], "
                      "rc: %d", expectLSN.offset, rc ) ;
         record.load( mb.startPtr() ) ;

         if ( !rollbackTransInfoFromLog( record ) )
         {
            setIsNeedSyncTrans( TRUE ) ;
         }

         currentLSN.offset = record.head()._preLsn ;
      }

      PD_LOG( PDEVENT, "Finished rollback trans info, current LSN [%llu], "
              "expect LSN [%llu]", currentLSN.offset, expectLSN.offset ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSCB_ROLLBACKTRANSINFO, rc ) ;
      return rc ;

   error:
      setIsNeedSyncTrans( TRUE ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC, "dpsTransCB::rollbackTransInfoFromLog" )
   BOOLEAN dpsTransCB::rollbackTransInfoFromLog( const dpsLogRecord &record )
   {
      BOOLEAN ret = TRUE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC ) ;

      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      INT32 transStatus = DPS_TRANS_DOING ;
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_TRANSID ) ;
      if ( !itr.valid() )
      {
         goto done ;
      }

      transID = *( (DPS_TRANS_ID*)itr.value() ) ;
      if ( transID != DPS_INVALID_TRANS_ID )
      {
         itr = record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
         if ( !itr.valid() )
         {
            lsnOffset = DPS_INVALID_LSN_OFFSET ;
         }
         else
         {
            lsnOffset = *((DPS_LSN_OFFSET *)itr.value()) ;
         }

         if ( LOG_TYPE_TS_COMMIT == record.head()._type )
         {
            DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;
            UINT8 attr = 0 ;

            itr = record.find( DPS_LOG_PUBLIC_FIRSTTRANS ) ;

            if ( DPS_INVALID_LSN_OFFSET == lsnOffset ||
                 !itr.valid() )
            {
               // In the old version, commit log have not pre trans lsn and
               // first trans lsn. So, we can't rollback trans info
               ret = FALSE ;
               goto done ;
            }
            firstLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;

            itr = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
            if ( itr.valid() )
            {
               attr = *(UINT8*)itr.value() ;
            }

            if ( DPS_TS_COMMIT_ATTR_PRE != attr )
            {
               addBeginLsn( firstLsn, transID ) ;
               transStatus = DPS_TS_COMMIT_ATTR_SND == attr ?
                             DPS_TRANS_WAIT_COMMIT : DPS_TRANS_DOING ;
               delHisTrans( transID ) ;
            }
         }
         else if ( !isRollback( transID ) )
         {
            if ( isFirstOp( transID ) )
            {
               delBeginLsn( transID ) ;
            }
         }
         else // is rollback
         {
            DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
            itr = record.find( DPS_LOG_PUBLIC_RELATED_TRANS ) ;
            if ( !itr.valid() )
            {
               // In the old version, rollback log have not related trans lsn,
               // so, we can't rollback trans info correctly
               ret = FALSE ;
               goto done ;
            }
            relatedLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;
            if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
            {
               // In the last rollback trans lsn, pre trans lsn is invalid
               addBeginLsn( relatedLsn, transID ) ;
               transStatus = DPS_TRANS_ROLLBACK ;
               delHisTrans( transID ) ;
            }
            lsnOffset = relatedLsn ;
         }

         updateTransInfo( transID, lsnOffset, transStatus ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_DPSTRANSCB_ROLLBACKTRANSINFOFROMLOG_REC ) ;

      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG, "dpsTransCB::saveTransInfoFromLog" )
   void dpsTransCB::saveTransInfoFromLog( const dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;

      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID;
      INT32 transStatus = DPS_TRANS_DOING ;
      dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_TRANSID ) ;
      if ( !itr.valid() )
      {
         goto done ;
      }
      transID = *( (DPS_TRANS_ID *)itr.value() ) ;
      if ( transID != DPS_INVALID_TRANS_ID )
      {
         if ( isRollback( transID ) )
         {
            transStatus = DPS_TRANS_ROLLBACK ;
            itr = record.find( DPS_LOG_PUBLIC_PRETRANS ) ;
            if ( !itr.valid() )
            {
               lsnOffset = DPS_INVALID_LSN_OFFSET ;
            }
            else
            {
               lsnOffset = *((DPS_LSN_OFFSET *)itr.value()) ;
            }
         }
         else if ( LOG_TYPE_TS_COMMIT == record.head()._type )
         {
            itr = record.find( DPS_LOG_TSCOMMIT_ATTR ) ;
            if ( !itr.valid() ||
                 DPS_TS_COMMIT_ATTR_PRE != *(UINT8*)itr.value() )
            {
               lsnOffset = DPS_INVALID_LSN_OFFSET ;
               transStatus = DPS_TRANS_COMMIT ;
            }
            else
            {
               lsnOffset = record.head()._lsn ;
               transStatus = DPS_TRANS_WAIT_COMMIT ;
            }
         }
         else
         {
            lsnOffset = record.head()._lsn ;
            if ( isFirstOp( transID ) )
            {
               addBeginLsn( lsnOffset, transID ) ;
            }
         }

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            delBeginLsn( transID ) ;
         }
         updateTransInfo( transID, lsnOffset, transStatus ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;
      return ;
   }

   void dpsTransCB::addHisTrans( DPS_TRANS_ID transID,
                                 INT32 status,
                                 DPS_LSN_OFFSET lsn )
   {
      /// when status is DPS_TRANS_COMMIT and
      /// auto transaction don't need add to history list
      if ( DPS_TRANS_COMMIT != status &&
           !DPS_TRANS_IS_AUTOCOMMIT( transID ) )
      {
         transID = DPS_TRANS_GET_ID( transID ) ;

         ossScopedLock lock( &_hisMutex ) ;
         _hisTransStatus[ transID ] = dpsHisTransStatus( status, lsn ) ;
         _hisLsnTrans[ lsn ] = transID ;
      }
   }

   void dpsTransCB::delHisTrans( DPS_TRANS_ID transID )
   {
      transID = DPS_TRANS_GET_ID( transID ) ;

      ossScopedLock lock( &_hisMutex ) ;
      TRANS_ID_2_STATUS::iterator it = _hisTransStatus.find( transID ) ;
      if ( it != _hisTransStatus.end() )
      {
         _hisLsnTrans.erase( it->second._lsn ) ;
         _hisTransStatus.erase( it ) ;
      }
   }

   void dpsTransCB::clearHisTrans()
   {
      ossScopedLock lock( &_hisMutex ) ;

      _hisLsnTrans.clear() ;
      _hisTransStatus.clear() ;
   }

   void dpsTransCB::clearOutDateHisTrans( DPS_LSN_OFFSET lsn )
   {
      TRANS_LSN_ID_MAP::iterator it ;

      if ( DPS_INVALID_LSN_OFFSET != lsn )
      {
         ossScopedLock lock( &_hisMutex ) ;

         it = _hisLsnTrans.begin() ;
         while( it != _hisLsnTrans.end() )
         {
            if ( it->first < lsn )
            {
               _hisTransStatus.erase( it->second ) ;
               _hisLsnTrans.erase( it++ ) ;
               continue ;
            }
            break ;
         }
      }
   }

   INT32 dpsTransCB::checkTransStatus( DPS_TRANS_ID transID,
                                       DPS_LSN_OFFSET & lsn )
   {
      /// first check in-line trans, then check history trans
      INT32 transStatus = DPS_TRANS_UNKNOWN ;
      transID = DPS_TRANS_GET_ID( transID ) ;
      lsn = DPS_INVALID_LSN_OFFSET ;

      {
         ossScopedLock _lock( &_MapMutex ) ;
         TRANS_MAP::iterator it = _TransMap.find( transID ) ;
         if ( it != _TransMap.end() )
         {
            transStatus = it->second._status ;
            lsn = it->second._lsn ;
            goto done ;
         }
      }

      {
         ossScopedLock lock( &_hisMutex ) ;
         TRANS_ID_2_STATUS::iterator it = _hisTransStatus.find( transID ) ;
         if ( it != _hisTransStatus.end() )
         {
            transStatus = it->second._status ;
            lsn = it->second._lsn ;
            goto done ;
         }
      }

   done:
      return transStatus ;
   }

   void dpsTransCB::addBeginLsn( DPS_LSN_OFFSET beginLsn, DPS_TRANS_ID transID )
   {
      SDB_ASSERT( beginLsn != DPS_INVALID_LSN_OFFSET, "invalid begin-lsn" ) ;
      SDB_ASSERT( transID != DPS_INVALID_TRANS_ID, "invalid transaction-ID" ) ;
      transID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex );
      _beginLsnIdMap[ beginLsn ] = transID;
      _idBeginLsnMap[ transID ] = beginLsn;
   }

   void dpsTransCB::delBeginLsn( DPS_TRANS_ID transID )
   {
      transID = getTransID( transID );
      ossScopedLock _lock( &_lsnMapMutex );
      DPS_LSN_OFFSET beginLsn;
      TRANS_ID_LSN_MAP::iterator iter = _idBeginLsnMap.find( transID ) ;
      if ( iter != _idBeginLsnMap.end() )
      {
         beginLsn = iter->second;
         _beginLsnIdMap.erase( beginLsn );
         _idBeginLsnMap.erase( transID );
      }
   }

   DPS_LSN_OFFSET dpsTransCB::getBeginLsn( DPS_TRANS_ID transID )
   {
      transID = getTransID( transID ) ;
      ossScopedLock _lock( &_lsnMapMutex ) ;
      TRANS_ID_LSN_MAP::iterator iter = _idBeginLsnMap.find( transID ) ;
      if ( iter != _idBeginLsnMap.end() )
      {
         return iter->second ;
      }
      return DPS_INVALID_LSN_OFFSET ;
   }

   DPS_LSN_OFFSET dpsTransCB::getOldestBeginLsn()
   {
      ossScopedLock _lock( &_lsnMapMutex );
      if ( _beginLsnIdMap.size() > 0 )
      {
         return _beginLsnIdMap.begin()->first;
      }
      return DPS_INVALID_LSN_OFFSET;
   }

   BOOLEAN dpsTransCB::isNeedSyncTrans()
   {
      return _isNeedSyncTrans;
   }

   void dpsTransCB::setIsNeedSyncTrans( BOOLEAN isNeed )
   {
      _isNeedSyncTrans = isNeed;
   }

   INT32 dpsTransCB::syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN curLsn ;
      curLsn.offset = beginLsn ;
      _dpsMessageBlock mb(DPS_MSG_BLOCK_DEF_LEN) ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      if ( !_isNeedSyncTrans || DPS_INVALID_LSN_OFFSET == beginLsn )
      {
         goto done;
      }
      clearTransInfo() ;
      while ( curLsn.offset!= DPS_INVALID_LSN_OFFSET &&
              curLsn.compareOffset( dpsCB->expectLsn().offset ) < 0 )
      {
         mb.clear();
         rc = dpsCB->search( curLsn, &mb );
         PD_RC_CHECK( rc, PDERROR, "Failed to search %lld in dpsCB, rc=%d",
                      curLsn.offset, rc );
         _dpsLogRecord record ;
         rc = record.load( mb.readPtr() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load log record, rc=%d", rc );
         saveTransInfoFromLog( record ) ;
         curLsn.offset += record.head()._length;
      }
   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_TERMALLTRANS, "dpsTransCB::termAllTrans" )
   void dpsTransCB::termAllTrans()
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_TERMALLTRANS ) ;

      ossScopedLock _lock( &_CBMapMutex );
      for ( TRANS_CB_MAP::iterator iterMap = _cbMap.begin() ;
            iterMap != _cbMap.end() ;
            ++ iterMap )
      {
         iterMap->second->postEvent( pmdEDUEvent(
                                     PMD_EDU_EVENT_TRANS_STOP ) ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_TERMALLTRANS );
   }


   INT32 dpsTransCB::transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      rc =  _transLockMgr->acquire( eduCB->getTransExecutor(),
                                    lockId, DPS_TRANSLOCK_X,
                                    pContext, pdpsTxResInfo,
                                    callback );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }

      return rc ;
   }


   INT32 dpsTransCB::transLockGetU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_U,
                                   pContext, pdpsTxResInfo,
                                   callback ) ;

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }


   INT32 dpsTransCB::transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    _IContext *pContext,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_S,
                                   pContext, pdpsTxResInfo,
                                   callback );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }


   INT32 dpsTransCB::transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     _IContext *pContext,
                                     dpsTransRetInfo * pdpsTxResInfo )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_IX,
                                   pContext, pdpsTxResInfo );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }


   INT32 dpsTransCB::transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     _IContext *pContext,
                                     dpsTransRetInfo * pdpsTxResInfo )
   {
      INT32 rc = SDB_OK ;
      if ( !_isOn )
      {
         return rc ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      rc = _transLockMgr->acquire( eduCB->getTransExecutor(),
                                   lockId, DPS_TRANSLOCK_IS,
                                   pContext, pdpsTxResInfo );

      if ( eduCB->getTransExecutor()->hasLockWait() )
      {
         eduCB->getTransExecutor()->finishLockWait() ;

         if ( eduCB->getMonQueryCB() )
         {
            eduCB->getMonQueryCB()->lockWaitTime += eduCB->
                                                     getTransExecutor()->
                                                     getLockWaitTime() ;
         }
      }
      return rc ;
   }


   void dpsTransCB::transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      _dpsITransLockCallback * callback )
   {
      if ( 0 == eduCB->getTransExecutor()->getLockCount( LOCKMGR_TRANS_LOCK ) )
      {
         return ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );

      return _transLockMgr->release( eduCB->getTransExecutor(),
                                     lockId, FALSE, callback );
   }

   void dpsTransCB::transLockReleaseAll( _pmdEDUCB *eduCB,
                                         _dpsITransLockCallback * callback )
   {
      if ( 0 == eduCB->getTransExecutor()->getLockCount( LOCKMGR_TRANS_LOCK ) )
      {
         return ;
      }
      else
      {
         _transLockMgr->releaseAll( eduCB->getTransExecutor(), callback );
      }
   }

   BOOLEAN dpsTransCB::isTransOn() const
   {
      return _isOn ;
   }

   INT32 dpsTransCB::transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_S,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback );
   }

   INT32 dpsTransCB::transLockTestSPreempt( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                            UINT16 collectionID,
                                            const dmsRecordID *recordID,
                                            dpsTransRetInfo * pdpsTxResInfo,
                                            _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_S,
                                        TRUE, // preemptively test
                                        pdpsTxResInfo,
                                        callback );
   }

   INT32 dpsTransCB::transLockTestIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      dpsTransRetInfo * pdpsTxResInfo )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_IS,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo );
   }

   INT32 dpsTransCB::transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_X,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback ) ;
   }


   INT32 dpsTransCB::transLockTestU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID,
                                     dpsTransRetInfo * pdpsTxResInfo,
                                     _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_U,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo,
                                        callback ) ;
   }


   INT32 dpsTransCB::transLockTestIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      dpsTransRetInfo * pdpsTxResInfo )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _transLockMgr->testAcquire( eduCB->getTransExecutor(),
                                        lockId, DPS_TRANSLOCK_IX,
                                        FALSE,  // not preemptively test
                                        pdpsTxResInfo );
   }


   INT32 dpsTransCB::transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_X,
                                       pdpsTxResInfo, callback );
   }

   INT32 dpsTransCB::transLockTryU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_U,
                                       pdpsTxResInfo,
                                       callback );
   }


   INT32 dpsTransCB::transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    dpsTransRetInfo * pdpsTxResInfo,
                                    _dpsITransLockCallback * callback )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->tryAcquire( eduCB->getTransExecutor(),
                                       lockId, DPS_TRANSLOCK_S,
                                       pdpsTxResInfo,
                                       callback ) ;
   }


   BOOLEAN dpsTransCB::hasWait( UINT32 logicCSID, UINT16 collectionID,
                                const dmsRecordID *recordID)
   {
      if ( !_isOn )
      {
         return FALSE ;
      }
      SDB_ASSERT( collectionID!=DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _transLockMgr->hasWait( lockId );
   }

   // Agorithm to decide if we have sufficent log space for new LR:
   // Support archive logging for transaction is the ultimate goal, meanwhile,
   // we will use following method if circular logging is in use:
   // 1. we will track two largest record size on each node.  These two number
   // has the LR lsn associated with it.  We can guarantee the larger one is
   // always greater or equal than the largest uncommitted record within the
   // system by only reduce it after a full cycle of all logs
   // 2. we use a _reservedRBSpace to count all outstanding LR space required
   // including potential rollback LRs.  This can be implemented as each
   // transaction keep track of it's reservedSpace, and only release those
   // space at commit or rollbacktime.  non transactional LR reserved space
   // can be released after the LR is written
   // 3. we use a _reservedRBSpace to count all unwritten LR space
   INT32 dpsTransCB::reservedLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      // undo log size has extra field, need to add the delta
      UINT32 rblength = length + DPS_TRANS_LOG_UNDO_DELTA ;

      if ( !_isOn || ( cb && cb->isInTransRollback() ) )
      {
         goto done ;
      }

      {
         _reservedSpace.add( length ) ;
         _reservedRBSpace.add( rblength ) ;
      }

      if ( remainLogSpace() == 0 )
      {
#ifdef _DEBUG
         UINT64 logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
         UINT32 logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
         DPS_LSN_OFFSET curLsnOffset =
                             pmdGetKRCB()->getDPSCB()->expectLsn().offset;
         DPS_LSN_OFFSET beginLsnOffset = getOldestBeginLsn();

         PD_LOG( PDWARNING, "Running out of log space, please commit existing "
                 "transactions. (length=%u, usedSize=%u, maxLRSize=%u, "
                 "beginlsn=%u, curlsn=%u, logFileSize=%u, logFileNum=%u, "
                 "remainLogSpace=%u) " ,
                 length, usedLogSpace(), getMaxLRSize(), beginLsnOffset,
                 curLsnOffset, logFileSize, logFileNum, remainLogSpace() ) ;
         printCounters( ) ;
#endif

         rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE ;
         _reservedRBSpace.sub( rblength ) ;
         _reservedSpace.sub( length ) ;
         goto error ;
      }
      cb->addReservedSpace( rblength ) ;
   done:
      return rc;
   error:
      goto done;
   }

   void dpsTransCB::releaseLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      if ( !_isOn || ( cb && cb->isInTransRollback() ) )
      {
         return ;
      }

      _reservedSpace.sub( length ) ;

      // for non-transactional LR, we can release the reserved space
      if ( !cb->isTransaction() )
      {
         releaseRBLogSpace( cb ) ;
      }
   }

   void dpsTransCB::releaseRBLogSpace( _pmdEDUCB *cb )
   {
      _reservedRBSpace.sub( cb->getReservedSpace() ) ;
      cb->resetLogSpace() ;
   }

   UINT64 dpsTransCB::usedLogSpace()
   {
      DPS_LSN_OFFSET beginLsnOffset;
      DPS_LSN_OFFSET curLsnOffset;
      DPS_LSN curLsn;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB();
      UINT64 usedSize = 0;

      beginLsnOffset = getOldestBeginLsn();
      if ( DPS_INVALID_LSN_OFFSET == beginLsnOffset )
      {
         goto done;
      }
      curLsn = dpsCB->expectLsn() ;
      curLsnOffset = curLsn.offset ;
      if ( DPS_INVALID_LSN_OFFSET == curLsnOffset )
      {
         goto done;
      }

      beginLsnOffset = beginLsnOffset % _logFileTotalSize ;
      curLsnOffset = curLsnOffset % _logFileTotalSize ;
      usedSize = ( curLsnOffset + _logFileTotalSize - beginLsnOffset ) %
                   _logFileTotalSize ;
   done:
      return usedSize ;
   }

   // Calculate remaining log space based on current usage and predicted
   // waste space
   UINT64 dpsTransCB::remainLogSpace()
   {
      UINT64 remainSize = _logFileTotalSize ;
      UINT64 totalSpace = 0 ;
      UINT64 usedSize = 0 ;
      UINT64 logFileSize = 0 ;
      UINT32 logFileNum = 0 ;
      UINT32 adjust = 0 ;

      if ( !_isOn )
      {
         goto done ;
      }

      usedSize = usedLogSpace() ;
      if ( 0 == usedSize )
      {
         // no transaction
         goto done ;
      }

      // culation: based on current logspace usage to decide how much
      // space left. The rule of thumb: we can't overwrite
      // any logs from the oldest uncommitted transaction (lowtran).
      // usedSize: include LRs from uncommitted transaction, LRs from committed
      // transactions but held by lowtran, also LRs from nontransaction
      // _reservedRBSpace: contain space for undo LRs(rollback purpose) for all
      // the uncommitted transaction
      // _reservedSpace: contain space for all unwritten LRs
      // logFileSize: leave one log file as the beginLSN could be in the middle
      // of a log file, but we can't reuse the whole file
      // For the unused file, we should expect waste space for each of them,
      // the waste space is predicated base on current max LR size.
      // Free log space logic:
      // _logFileTotalSize - ( usedSize + logfilesize + length +
      //                      _reservedSpace + _reservedRBSpace +
      //                      (#totalfile - usedfile -1 )*max_record_size )
      logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
      logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
      // need a round down to guarantee enough waste space
      adjust = ( usedSize > logFileSize ) ?
               ( usedSize - logFileSize ) / logFileSize : 0 ;
      totalSpace =  usedSize + logFileSize + _reservedRBSpace.peek() +
                    _reservedSpace.peek() +
                    ( logFileNum - adjust - 1 ) * getMaxLRSize() ;

      if ( totalSpace < _logFileTotalSize )
      {
         remainSize = _logFileTotalSize - totalSpace ;
      }
      else
      {
         remainSize = 0 ;
      }

   done:
      return remainSize ;
   }

   UINT32 dpsTransCB::getMaxLRSize()
   {
      return ( 0 != _maxLRSize1 ) ? _maxLRSize1 : DMS_RECORD_MAX_SZ ;
   }

   // based on the input log record size and lsn, update the stored
   // maxLRSize as needed. Caller need to guarantee serialization.
   void  dpsTransCB::updateMaxLRSize( UINT32 recordSize,
                                      DPS_LSN_OFFSET recordLSN )
   {
      if ( _maxLRSize1 < recordSize )
      {
         _maxLRSize2 = 0 ;
         _maxLRLSN2 = DPS_INVALID_LSN_OFFSET ;

         _maxLRSize1 = recordSize ;
         _maxLRLSN1 = recordLSN ;
      }
      else
      {
         if ( _maxLRSize2 < recordSize )
         {
            _maxLRSize2 = recordSize ;
            _maxLRLSN2 = recordLSN ;
         }

         // when the recordLSN wrapped whole log files, we guarantee safe to
         // update the _maxLRSize1, reduce it to _maxLRSize2, and set its lsn
         // to the recordLSN
         if ( ( recordLSN - _maxLRLSN1 ) > _logFileTotalSize )
         {
#if SDB_INTERNAL_DEBUG
            PD_LOG( PDDEBUG, "Going to reduce maxLRSize1 size, "
                    "_maxLRLSN1=%u, recordLSN=%u ",
                    _maxLRLSN1, recordLSN );
            printCounters( ) ;
#endif
            _maxLRSize1 = _maxLRSize2 ;
            // this LR LSN is very close to curLSN
            _maxLRLSN1 = recordLSN ;
            _maxLRSize2 = 0 ;
            _maxLRLSN2 = DPS_INVALID_LSN_OFFSET ;
         }
      }
   }

   void  dpsTransCB::printCounters()
   {
#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Dumping dpsTransCB Log related counters: "
                       "_logFileTotalSize=%u, _reservedRBSpace=%u, "
                       "_reservedSpace=%u, "
                       "_maxLRSize1=%u, _maxLRLSN1=%u, "
                       "_maxLRSize2=%u, _maxLRLSN2=%u, ",
                       _logFileTotalSize,
                       _reservedRBSpace.peek(), _reservedSpace.peek(),
                       _maxLRSize1, _maxLRLSN1, _maxLRSize2, _maxLRLSN2 ) ;
#endif
   }

   dpsTransLockManager * dpsTransCB::getLockMgrHandle()
   {
      return ( _transLockMgr->isInitialized() ? ( _transLockMgr ) : NULL ) ;
   }

   ixmIndexLockManager * dpsTransCB::getIndexLockMgrHandle()
   {
      return ( _indexLockMgr->isInitialized() ? ( _indexLockMgr ) : NULL ) ;
   }

   /*
      get global trans cb
   */
   dpsTransCB* sdbGetTransCB ()
   {
      static dpsTransCB s_transCB ;
      return &s_transCB ;
   }

   /*
      helper functions for dpsTransPendingKey
    */
   // check pending key and value
   // 1. should have OID
   // 2. should be owned
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSCHECKPENDING, "_dpsCheckTransPending" )
   static INT32 _dpsCheckTransPending( dpsTransPendingKey &pendingKey,
                                       dpsTransPendingValue &pendingValue )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSCHECKPENDING ) ;

      try
      {
         BSONElement element ;

         element = pendingKey._obj.getField( DMS_ID_KEY_NAME ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to check pending key, "
                   "OID is not found in BSON [%s]",
                   pendingKey._obj.toPoolString().c_str() ) ;
         pendingKey._obj = pendingKey._obj.getOwned() ;

         if ( !( pendingValue._obj.isEmpty() ) )
         {
            element = pendingValue._obj.getField( DMS_ID_KEY_NAME ) ;
            PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                      "Failed to check pending value, "
                      "OID is not found in BSON [%s]",
                      pendingValue._obj.toPoolString().c_str() ) ;
         }
         pendingValue._obj = pendingValue._obj.getOwned() ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Failed to check pending key, exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
      }

   done :
      PD_TRACE_EXITRC( SDB__DPSCHECKPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSADDTRANSPENDING, "dpsAddTransPending" )
   INT32 dpsAddTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                             dpsTransPendingKey &pendingKey,
                             dpsTransPendingValue &pendingValue,
                             BOOLEAN &added )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSADDTRANSPENDING ) ;

      MAP_TRANS_PENDING_OBJ::iterator iter ;

      added = FALSE ;

      rc = _dpsCheckTransPending( pendingKey, pendingValue ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check transaction rollback pending "
                   "key [ collection %s, key %s ], value [ %s ], rc: %d",
                   pendingKey._collection.c_str(),
                   pendingKey._obj.toPoolString().c_str(),
                   pendingValue._obj.toPoolString().c_str(), rc ) ;

      iter = pendingMap.find( pendingKey ) ;
      if ( iter != pendingMap.end() )
      {
         PD_LOG( PDDEBUG, "Find duplicated transaction rollback pending "
                 "object [ collection %s, key %s ],"
                 "old value [ %s ], "
                 "new value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 iter->second._obj.toPoolString().c_str(),
                 pendingValue._obj.toPoolString().c_str() ) ;
         // replace
         iter->second = pendingValue ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Insert transaction rollback pending "
                 "object [ collection %s, key %s ], value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 pendingValue._obj.toPoolString().c_str() ) ;
         pendingMap.insert( make_pair( pendingKey, pendingValue ) ) ;
      }

      added = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB__DPSADDTRANSPENDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSRMTRANSPENDING, "dpsRemoveTransPending" )
   INT32 dpsRemoveTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                                const dpsTransPendingKey &pendingKey,
                                BSONObj *oldKey,
                                dpsTransPendingValue *oldValue,
                                BOOLEAN &removed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSRMTRANSPENDING ) ;

      removed = FALSE ;

      MAP_TRANS_PENDING_OBJ::iterator iter =
                                       pendingMap.find( pendingKey ) ;
      if ( iter != pendingMap.end() )
      {
         PD_LOG( PDDEBUG, "Delete transaction rollback pending "
                 "object [ collection %s, key %s ], value [ %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str(),
                 iter->second._obj.toPoolString().c_str() ) ;

         if ( NULL != oldKey )
         {
            (*oldKey) = iter->first._obj ;
         }
         if ( NULL != oldValue )
         {
            oldValue->setValue( iter->second._obj, iter->second._opType ) ;
         }

         pendingMap.erase( iter ) ;
         removed = TRUE ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Failed to find transaction rollback pending "
                 "object for delete [ collection %s, key %s ]",
                 pendingKey._collection.c_str(),
                 pendingKey._obj.toPoolString().c_str() ) ;
      }

      PD_TRACE_EXITRC( SDB__DPSRMTRANSPENDING, rc ) ;

      return rc ;
   }

}
