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
#include "dpsTransLock.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pmdDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsMessageBlock.hpp"
#include "dpsLogRecordDef.hpp"
#include "pmdStartup.hpp"

namespace engine
{

   dpsTransCB::dpsTransCB()
   :_TransIDL48Cur( 1 )
   {
      _TransIDH16          = 0;
      _isOn                = FALSE;
      _doRollback          = FALSE;
      _isNeedSyncTrans     = TRUE;
      _maxUsedSize         = 0;
      _logFileTotalSize    = 0;
      _accquiredSpace      = 0;
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

      _isOn = pmdGetOptionCB()->transactionOn() ;
      _rollbackEvent.signal() ;

      pmdGetKRCB()->regEventHandler( this ) ;

      if ( pmdGetKRCB()->isCBValue( SDB_CB_DPS ) &&
           !pmdGetKRCB()->isRestore() )
      {
         UINT64 logFileSize = pmdGetOptionCB()->getReplLogFileSz() ;
         UINT32 logFileNum = pmdGetOptionCB()->getReplLogFileNum() ;
         _logFileTotalSize = logFileSize * logFileNum ;

         if ( logFileNum >= 5 )
         {
            UINT64 reservedSize = logFileSize * 2 ;

            _maxUsedSize = ( _logFileTotalSize - reservedSize
                                 - 2 * DMS_RECORD_MAX_SZ * ( logFileNum - 2 ) ) / 2 ;

            UINT64 temp = ( _logFileTotalSize - reservedSize ) / 3 ;
            if ( _maxUsedSize > temp || 0 == _maxUsedSize )
            {
               _maxUsedSize = temp ;
            }
         }
         else
         {
            _maxUsedSize = 0 ;
         }

         DPS_LSN startLSN = sdbGetDPSCB()->getStartLsn() ;
         if ( _isOn && startLSN.offset != DPS_INVALID_LSN_OFFSET &&
              SDB_ROLE_STANDALONE != pmdGetDBRole() )
         {
            rc = syncTransInfoFromLocal( startLSN.offset ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to sync trans info from local, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         setIsNeedSyncTrans( FALSE ) ;

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
      pmdGetKRCB()->unregEventHandler( this ) ;
      return SDB_OK ;
   }

   void dpsTransCB::onConfigChange()
   {
      dpsLockBucket::setLockTimeout( pmdGetOptionCB()->transTimeout() * 1000 ) ;
   }

   DPS_TRANS_ID dpsTransCB::allocTransID()
   {
      DPS_TRANS_ID temp = 0;
      while ( 0 == temp )
      {
         temp = _TransIDL48Cur.inc();
         temp = ( temp & DPS_TRANSID_SN_BIT ) | _TransIDH16 |
                DPS_TRANSID_FIRSTOP_BIT;
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
      else
      {
         if ( SDB_EVT_OCCUR_AFTER == occurType )
         {
            stopRollbackTask() ;
            termAllTrans() ;
         }
      }
   }

   DPS_TRANS_ID dpsTransCB::getRollbackID( DPS_TRANS_ID transID )
   {
      return transID | DPS_TRANSID_ROLLBACKTAG_BIT ;
   }

   DPS_TRANS_ID dpsTransCB::getTransID( DPS_TRANS_ID rollbackID )
   {
      return rollbackID & DPS_TRANSID_VALID_BIT ;
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
      if ( transID & DPS_TRANSID_FIRSTOP_BIT )
      {
         return TRUE;
      }
      return FALSE;
   }

   void dpsTransCB::clearFirstOpTag( DPS_TRANS_ID &transID )
   {
      transID = transID & ( ~DPS_TRANSID_FIRSTOP_BIT );
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SVTRANSINFO, "dpsTransCB::saveTransInfo" )
   void dpsTransCB::updateTransInfo( DPS_TRANS_ID transID,
                                     DPS_LSN_OFFSET lsnOffset )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSCB_SVTRANSINFO ) ;
      if ( DPS_INVALID_TRANS_ID == transID ||
           sdbGetDPSCB()->isInRestore() )
      {
         goto done ;
      }

      if ( pmdGetKRCB()->isCBValue( SDB_CB_CLS ) &&
           pmdIsPrimary() )
      {
         goto done ;
      }
      else
      {
         transID = getTransID( transID );
         ossScopedLock _lock( &_MapMutex );

         if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
         {
            _TransMap.erase( transID ) ;
         }
         else
         {
            _TransMap[ transID ] = lsnOffset ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SVTRANSINFO ) ;
      return ;
   }

   void dpsTransCB::addTransInfo( DPS_TRANS_ID transID,
                                  DPS_LSN_OFFSET lsnOffset )
   {
      transID = getTransID( transID );
      ossScopedLock _lock( &_MapMutex );
      if ( _TransMap.find( transID ) == _TransMap.end() )
      {
         _TransMap[ transID ] = lsnOffset;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_ADDTRANSCB, "dpsTransCB::addTransCB" )
   void dpsTransCB::addTransCB( DPS_TRANS_ID transID, _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_ADDTRANSCB ) ;
      {
         transID = getTransID( transID );
         ossScopedLock _lock( &_CBMapMutex );
         _cbMap[ transID ] = eduCB;
      }
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_ADDTRANSCB );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_DELTRANSCB, "dpsTransCB::delTransCB" )
   void dpsTransCB::delTransCB( DPS_TRANS_ID transID )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_DELTRANSCB ) ;
      {
         transID = getTransID( transID );
         ossScopedLock _lock( &_CBMapMutex ) ;
         _cbMap.erase( transID );
      }
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_DELTRANSCB );
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
   }

   BOOLEAN dpsTransCB::rollbackTransInfoFromLog( const dpsLogRecord &record )
   {
      BOOLEAN ret = TRUE ;
      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
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
            itr = record.find( DPS_LOG_PUBLIC_FIRSTTRANS ) ;

            if ( DPS_INVALID_LSN_OFFSET == lsnOffset ||
                 !itr.valid() )
            {
               ret = FALSE ;
               goto done ;
            }
            firstLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;

            addBeginLsn( firstLsn, transID ) ;
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
               ret = FALSE ;
               goto done ;
            }
            relatedLsn = *( ( DPS_LSN_OFFSET*)itr.value() ) ;
            if ( DPS_INVALID_LSN_OFFSET == lsnOffset )
            {
               addBeginLsn( relatedLsn, transID ) ;
            }
            lsnOffset = relatedLsn ;
         }

         updateTransInfo( transID, lsnOffset ) ;
      }

   done:
      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG, "dpsTransCB::saveTransInfoFromLog" )
   void dpsTransCB::saveTransInfoFromLog( const dpsLogRecord &record )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;

      DPS_LSN_OFFSET lsnOffset = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID;
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
            lsnOffset = DPS_INVALID_LSN_OFFSET ;
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
         updateTransInfo( transID, lsnOffset ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSCB_SAVETRANSINFOFROMLOG ) ;
      return ;
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
         saveTransInfoFromLog( record );
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
      TRANS_CB_MAP::iterator iterMap = _cbMap.begin();
      while( iterMap != _cbMap.end() )
      {
         iterMap->second->postEvent( pmdEDUEvent(
                                     PMD_EDU_EVENT_TRANS_STOP ) ) ;
         _cbMap.erase( iterMap++ );
      }

      PD_TRACE_EXIT ( SDB_DPSTRANSCB_TERMALLTRANS );
   }

   INT32 dpsTransCB::transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.acquireX( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.acquireS( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _TransLock.acquireIX( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, NULL );
      return _TransLock.acquireIS( eduCB, lockId );
   }

   void dpsTransCB::transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.release( eduCB, lockId );
   }

   void dpsTransCB::transLockReleaseAll( _pmdEDUCB *eduCB )
   {
      if ( !_isOn )
      {
         return ;
      }
      return _TransLock.releaseAll( eduCB );
   }

   BOOLEAN dpsTransCB::isTransOn()
   {
      return _isOn ;
   }

   INT32 dpsTransCB::transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.testS( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                     UINT16 collectionID,
                                     const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.testX( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.tryX( eduCB, lockId );
   }

   INT32 dpsTransCB::transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.tryS( eduCB, lockId );
   }

   INT32 dpsTransCB::tryOrAppendX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                   UINT16 collectionID,
                                   const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      SDB_ASSERT( collectionID!=DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.tryOrAppendX( eduCB, lockId );
   }

   INT32 dpsTransCB::waitLock( _pmdEDUCB * eduCB, UINT32 logicCSID,
                              UINT16 collectionID,
                              const dmsRecordID *recordID )
   {
      if ( !_isOn )
      {
         return SDB_OK ;
      }
      SDB_ASSERT( collectionID!=DMS_INVALID_MBID, "invalid collectionID" ) ;
      SDB_ASSERT( recordID, "recordID can't be NULL" ) ;
      dpsTransLockId lockId( logicCSID, collectionID, recordID );
      return _TransLock.wait( eduCB, lockId );
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
      return _TransLock.hasWait( lockId );
   }

   INT32 dpsTransCB::reservedLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      UINT64 usedSize = 0;
      if ( !_isOn || ( cb && cb->isInRollback() ) )
      {
         goto done ;
      }

      {
         ossScopedLock _lock( &_maxFileSizeMutex );
         _accquiredSpace += length;
      }
      usedSize = usedLogSpace();
      if ( usedSize + _accquiredSpace >= _maxUsedSize )
      {
         rc = SDB_DPS_LOG_FILE_OUT_OF_SIZE ;
         ossScopedLock _lock( &_maxFileSizeMutex );
         if ( _accquiredSpace >= length )
         {
            _accquiredSpace -= length ;
         }
         else
         {
            _accquiredSpace = 0 ;
         }
         goto error ;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void dpsTransCB::releaseLogSpace( UINT32 length, _pmdEDUCB *cb )
   {
      if ( !_isOn || ( cb && cb->isInRollback() ) )
      {
         return ;
      }
      ossScopedLock _lock( &_maxFileSizeMutex );
      if ( _accquiredSpace >= length )
      {
         _accquiredSpace -= length ;
      }
      else
      {
         _accquiredSpace = 0 ;
      }
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
      curLsn = dpsCB->expectLsn();
      curLsnOffset = curLsn.offset;
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

   UINT64 dpsTransCB::remainLogSpace()
   {
      UINT64 remainSize = _logFileTotalSize ;
      UINT64 allocatedSize = 0 ;

      if ( !_isOn )
      {
         goto done ;
      }

      {
      ossScopedLock _lock( &_maxFileSizeMutex ) ;
      allocatedSize = _accquiredSpace + usedLogSpace() ;
      }
      if ( _maxUsedSize > allocatedSize )
      {
         remainSize = _maxUsedSize - allocatedSize ;
      }
      else
      {
         remainSize = 0 ;
      }
   done:
      return remainSize ;
   }

   /*
      get global trans cb
   */
   dpsTransCB* sdbGetTransCB ()
   {
      static dpsTransCB s_transCB ;
      return &s_transCB ;
   }

}
