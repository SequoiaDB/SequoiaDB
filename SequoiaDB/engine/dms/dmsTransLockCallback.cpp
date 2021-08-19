/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = dmsTransLockCallback.cpp

   Descriptive Name = Data Management Service Lock Callback Functions

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/02/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsTransLockCallback.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "rtnIXScanner.hpp"
#include "utilLightJobBase.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"

using namespace bson ;


namespace engine
{

   /*
      _dmsMemRecordRW implement
   */
   class _dmsMemRecordRW : public _dmsRecordRW
   {
      public:
         _dmsMemRecordRW( dpsOldRecordPtr ptr )
         :_dmsRecordRW()
         {
            if ( ptr.get() )
            {
               _isDirectMem = TRUE ;
               _ptr = ( const dmsRecord* )ptr.get() ;
            }
         }
   } ;
   typedef _dmsMemRecordRW dmsMemRecordRW ;

   /*
      _dmsReleaseLockJob define and implement
   */
   class _dmsReleaseLockJob : public _utilLightJob
   {
      public:
         _dmsReleaseLockJob( const dpsTransLockId &id,
                             oldVersionContainer *oldVer )
         {
            _pOldVer = oldVer ;
         }
         virtual ~_dmsReleaseLockJob()
         {
            SDB_OSS_DEL _pOldVer ;
            _pOldVer = NULL ;
         }

         virtual const CHAR*     name() const
         {
            return "ReleaseLockJob" ;
         }
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      private:
         oldVersionContainer    *_pOldVer ;
   } ;
   typedef _dmsReleaseLockJob dmsReleaseLockJob ;

   static void _dmsRemoveOldVerFromChain( oldVersionContainer *oldVer )
   {
      static dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      static oldVersionCB *oldCB = transCB->getOldVCB() ;

      oldVersionUnitPtr unitPtr ;
      unitPtr = oldCB->getOldVersionUnit( oldVer->getCSID(),
                                          oldVer->getCLID() ) ;
      /// when unitPtr.get() is null, means collection is dropped or truncated
      if ( unitPtr.get() )
      {
         unitPtr->removeFromChain( oldVer ) ;
      }
   }

   INT32 _dmsReleaseLockJob::doit( IExecutor *pExe,
                                   UTIL_LJOB_DO_RESULT &result,
                                   UINT64 &sleepTime )
   {
      static dpsTransCB *pTransCB = sdbGetTransCB() ;
      static SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;

      INT32 rcTmp = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *pContext = NULL ;
      dpsTransRetInfo transRetInfo ;
      sleepTime = 2000000 ;

      // This backgroud job, _dmsReleaseLockJob, is started by
      // dmsOnTransLockRelease() after it removed this old version
      // container from the chain.
      SDB_ASSERT( ( ! _pOldVer->isOnChain() ),
                  "This old version container must have been "
                  "removed from chain." ) ;

      /// release
      if ( _pOldVer->isRecordDeleted() )
      {
         _pOldVer->releaseRecord() ;
      }

      if ( !_pOldVer->isDiskDeleting() || !pmdGetOptionCB()->recycleRecord() )
      {
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// lock collectionspace
      su = pDmsCB->suLock( _pOldVer->getCSID() ) ;
      if ( !su || su->LogicalCSID() != _pOldVer->getCSLID() )
      {
         /// collectionspace has dropped
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// lock collection
      if ( SDB_OK == su->data()->getMBContext( &pContext,
                                               _pOldVer->getCLID(),
                                               _pOldVer->getCLLID(),
                                               _pOldVer->getCLLID() ) )
      {
         rcTmp = pContext->mbTryLock( EXCLUSIVE ) ;
         if ( SDB_TIMEOUT == rcTmp )
         {
            result = UTIL_LJOB_DO_CONT ;
            goto done ;
         }
         else if ( rcTmp )
         {
            /// collection has dropped or truncated
            result = UTIL_LJOB_DO_FINISH ;
            goto done ;
         }
      }
      else
      {
         /// out-of-memory
         result = UTIL_LJOB_DO_CONT ;
         goto done ;
      }

      rcTmp = pTransCB->transLockTestX( (_pmdEDUCB*)pExe,
                                        _pOldVer->getCSLID(),
                                        _pOldVer->getCLID(),
                                        &_pOldVer->getRecordID(),
                                        &transRetInfo,
                                        NULL ) ;
      if ( SDB_OK == rcTmp )
      {
         result = UTIL_LJOB_DO_FINISH ;

         dmsRecordRW recordRW ;
         const dmsRecord *pRecord = NULL ;
         const dmsRecordID &rid = _pOldVer->getRecordID() ;

         recordRW = su->data()->record2RW( rid,
                                           _pOldVer->getCLID() ) ;
         recordRW.setNothrow( TRUE ) ;
         pRecord = recordRW.readPtr<dmsRecord>() ;
         if ( !pRecord || pRecord->getMyOffset() != rid._offset )
         {
            /// record not exist
            goto done ;
         }

         if ( !pRecord->isDeleting() )
         {
            /// record not deleting
            goto done ;
         }

         /// delete record
         su->data()->deleteRecord( pContext, rid,
                                   (ossValuePtr)pRecord,
                                   (pmdEDUCB*)pExe, NULL, NULL, NULL ) ;
      }
      else if ( DPS_TRANSLOCK_X == transRetInfo._lockType )
      {
         /// other trans locked it, will clear
         result = UTIL_LJOB_DO_FINISH ;
      }
      else
      {
         result = UTIL_LJOB_DO_CONT ;
      }

   done:
      if ( pContext )
      {
         su->data()->releaseMBContext( pContext ) ;
      }
      if ( su )
      {
         pDmsCB->suUnlock( su->CSID(), SHARED ) ;
      }
      return SDB_OK ;
   }

   static INT32 _dmsStartReleaseLockJob( const dpsTransLockId &lockId,
                                         oldVersionContainer *oldVer )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != oldVer, "old version is invalid" ) ;

      // oldVer might be destroyed after job submit
      // copy fields if we want to print into logs later
      INT32 csID = oldVer->getCSID() ;
      UINT16 clID = oldVer->getCLID() ;
      UINT32 csLID = oldVer->getCSLID() ;
      UINT32 clLID = oldVer->getCLLID() ;

      dmsReleaseLockJob *pJob = NULL ;
      pJob = SDB_OSS_NEW dmsReleaseLockJob( lockId, oldVer ) ;
      SDB_ASSERT( pJob, "Job is NULL" ) ;
      if ( pJob )
      {
         UINT64 jobID = 0 ;
         rc = pJob->submit( TRUE, UTIL_LJOB_PRI_MID, UTIL_LJOB_DFT_AVG_COST,
                            &jobID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Submit dmsReleaseLockJob(ID:%llu, CSID:%u, "
                    "CLID:%u, CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) "
                    "failed, rc: %d", jobID, csID, clID, csLID, clLID,
                    lockId.extentID(), lockId.offset(), rc) ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Submit dmsReleaseLockJob(ID:%llu, CSID:%u, "
                    "CLID:%u, CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) "
                    "succeed", jobID, csID, clID, csLID, clLID,
                    lockId.extentID(), lockId.offset() ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Alloc dmsReleaseLockJob(CSID:%u, CLID:%u, "
                 "CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) failed",
                 csID, clID, csLID, clLID,
                 lockId.extentID(), lockId.offset() ) ;
         rc = SDB_OOM ;
      }

      return rc ;
   }

   /*
      Extend data function
   */
   BOOLEAN dmsIsExtendDataValid( const dpsLRBExtData *pExtData )
   {
      return 0 != pExtData->_data ? TRUE : FALSE ;
   }

   BOOLEAN dmsExtendDataReleaseCheck( const dpsLRBExtData *pExtData )
   {
      const oldVersionContainer *pOldVer = NULL ;
      pOldVer = ( const oldVersionContainer * )( pExtData->_data ) ;
      if ( pOldVer &&
           ( !pOldVer->isIndexObjEmpty() || pOldVer->isOnChain() ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void dmsReleaseExtendData( dpsLRBExtData *pExtData )
   {
      oldVersionContainer *pOldVer = NULL ;
      pOldVer = ( oldVersionContainer * )( pExtData->_data ) ;
      if ( pOldVer )
      {
         SDB_OSS_DEL pOldVer ;
         pExtData->_data = 0 ;
      }
   }

   void dmsOnTransLockRelease( const dpsTransLockId &lockId,
                               DPS_TRANSLOCK_TYPE lockMode,
                               UINT32 refCounter,
                               dpsLRBExtData *pExtData,
                               INT32 idxLID,
                               BOOLEAN hasLock )
   {
      oldVersionContainer *oldVer   = NULL ;

      // early exit if this is not record lock OR not in X mode OR
      // there is no old record setup.
      if ( ( !lockId.isLeafLevel() )               ||
           ( DPS_TRANSLOCK_X != lockMode )         ||
           ( 0 != refCounter )                     ||
           ( NULL == pExtData )                    ||
           ( 0 == pExtData->_data ) )
      {
         goto done ;
      }

      oldVer = ( oldVersionContainer* )( pExtData->_data ) ;
      SDB_ASSERT( oldVer->getRecordID() ==
                  dmsRecordID(lockId.extentID(), lockId.offset()),
                  "LockID is not the same" ) ;

      // Previously when creates index, takes old version container
      // from chain first, then inserts into index LID set of that
      // old version container and inserts index tree; while
      // tryReleaseRecord deletes from index tree and index LID set,
      // then removes old version container from chain. In case of
      // both create/rebuild index and releaseRecord running at the
      // same time, we may see unexpect status of index LID set
      // So, we remove old version container from the chain first
      // then releaseRecord when release a lock( dmsOnTransLockRelease,
      // _dmsReleaseLockJob::doit ); when creates an index, walks through
      // the chain and checks if the index LID is in that old version
      // container's index LID set and do insert index LID and index
      // tree ( insertIdxTree and insertWithOldVer ) if it is not there.
      // When traverse the chain, add or remove an old version container
      // to or from the chain, proper latch on oldVersionUnit will apply
      // to protect synchronized accessing.
      if ( oldVer && oldVer->isOnChain() )
      {
         _dmsRemoveOldVerFromChain( oldVer ) ;
      }

      /// try relerase record, because maybe should wait index tree's lock,
      /// so use try
      if ( oldVer && oldVer->tryReleaseRecord( idxLID, hasLock ) )
      {
         PD_LOG( PDDEBUG, "Delete old record for rid[%s] from memory",
                 lockId.toString().c_str() ) ;
         goto done ;
      }
      else
      {
         oldVer->setRecordDeleted() ;
         PD_LOG( PDDEBUG, "Set old record for rid[%s] in memory to deleted",
                 lockId.toString().c_str() ) ;
      }

      /// PUT the rid to backgroud task to recycle
      if ( SDB_OK == _dmsStartReleaseLockJob( lockId, oldVer ) )
      {
         pExtData->_data = 0 ;
      }

   done:
      return ;
   }

   static BOOLEAN _dmsIsKeyUndefined( const BSONObj &keyObj )
   {
      BSONObjIterator itr( keyObj ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( Undefined != e.type() )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   dmsTransLockCallback::dmsTransLockCallback()
   {
      _transCB    = NULL ;
      _oldVer     = NULL ;
      _eduCB      = NULL ;
      _recordRW   = NULL ;

      _csLID      = ~0 ;
      _clLID      = ~0 ;
      _csID       = DMS_INVALID_SUID ;
      _clID       = DMS_INVALID_MBID ;
      _latchedIdxLid = DMS_INVALID_EXTENT ;
      _latchedIdxMode = -1 ;
      _pScanner      = NULL ;

      clearStatus() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK, "dmsTransLockCallback::dmsTransLockCallback" )
   dmsTransLockCallback::dmsTransLockCallback( dpsTransCB *transCB,
                                               _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK ) ;

      _transCB    = transCB ;
      _oldVer     = NULL ;
      _eduCB      = eduCB ;
      _recordRW   = NULL ;

      _csLID      = ~0 ;
      _clLID      = ~0 ;
      _csID       = DMS_INVALID_SUID ;
      _clID       = DMS_INVALID_MBID ;
      _latchedIdxLid = DMS_INVALID_EXTENT ;
      _latchedIdxMode = -1 ;
      _pScanner      = NULL ;

      clearStatus() ;

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_DMSTRANSLOCKCALLBACK );
   }

   dmsTransLockCallback::~dmsTransLockCallback()
   {
   }

   void dmsTransLockCallback::setBaseInfo( dpsTransCB *transCB,
                                           _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( eduCB, "EDUCB can't be NULL" ) ;
      _transCB = transCB ;
      _eduCB   = eduCB ;
   }

   void dmsTransLockCallback::setIDInfo( INT32 csID, UINT16 clID,
                                         UINT32 csLID, UINT32 clLID )
   {
      _csID = csID ;
      _clID = clID ;
      _csLID = csLID ;
      _clLID = clLID ;
   }

   void dmsTransLockCallback::setIXScanner( _rtnIXScanner *pScanner )
   {
      _latchedIdxLid = pScanner->getIdxLID() ;
      _latchedIdxMode = pScanner->getLockModeByType( SCANNER_TYPE_MEM_TREE ) ;
      _pScanner = pScanner ;
   }

   void dmsTransLockCallback::detachRecordRW()
   {
      _recordRW = NULL ;
   }

   void dmsTransLockCallback::attachRecordRW( _dmsRecordRW *recordRW )
   {
      _recordRW = recordRW ;
   }

   void dmsTransLockCallback::clearStatus()
   {
      _oldVer           = NULL ;
      _skipRecord       = FALSE ;
      _result           = SDB_OK ;
      _useOldVersion    = FALSE ;
      _recordPtr        = dpsOldRecordPtr() ;
      _recordInfo.reset() ;
   }

   const dmsTransRecordInfo* dmsTransLockCallback::getTransRecordInfo() const
   {
      return &_recordInfo ;
   }

   // Description:
   //    Function called after lock acquirement. There are two cases to handle:
   //
   // CASE 1: Under RC, TB scanner failed to get record lock in S mode due to
   // incompatibility, we'll try to use the saved old copy if it exist.
   // Note that normally tb scanner or idx scanner will wait on record lock
   // unless transaction isolation level is RC and translockwait is set to NO
   //
   // CASE 2: Update successfully acquire X record lock within a transaction,
   // we'll try to copy the data to lrbHrd if it's not already there
   // Note that we have decided to defer the copy to the actual update time,
   // because there are cases where X lock was acquired, but no update will
   // be made due to other critieras. However, we will do some preparation
   // work at this time.
   //
   // Input:
   //    lockId: lock id to operate on
   //    rc:  rc from the lock acquire
   //    requestLockMode: lock mode requested (IS/IX/S/U/X)
   //    opMode: lock operation mode (TRY/ACQUIRE/TEST)
   // Output:
   //    pdpsTxResInfo: return information about the lock
   // Dependency:
   //    caller must hold lrb bucket latch
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE, "dmsTransLockCallback::afterLockAcquire" )
   void dmsTransLockCallback::afterLockAcquire( const dpsTransLockId &lockId,
                                                INT32 irc,
                                                DPS_TRANSLOCK_TYPE requestLockMode,
                                                UINT32 refCounter,
                                                DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                                const dpsTransLRBHeader *pLRBHeader,
                                                dpsLRBExtData *pExtData )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE ) ;

      BOOLEAN notTransOrRollback = FALSE ;

      /// when not leaf level, do nothing
      if ( !lockId.isLeafLevel() )
      {
         goto done ;
      }

      clearStatus() ;

      /// not in transaction
      if ( DPS_INVALID_TRANS_ID == _eduCB->getTransID() ||
           _eduCB->isInTransRollback() )
      {
         notTransOrRollback = TRUE ;
      }

      // Handle case 1 mentioned above
      // When the update was done by non transaction session, it's
      // possible that the oldRecord does not exist. We do nothing
      // in that case
      if( (SDB_DPS_TRANS_LOCK_INCOMPATIBLE == irc ) &&
          (DPS_TRANSLOCK_S == requestLockMode)     &&
          (DPS_TRANSLOCK_OP_MODE_TEST == opMode ||
           DPS_TRANSLOCK_OP_MODE_TRY == opMode ) )
      {
         if ( !pExtData || 0 == pExtData->_data )
         {
            goto done ;
         }
         else if ( notTransOrRollback )
         {
            goto done ;
         }

         _oldVer = (oldVersionContainer*)(pExtData->_data) ;
         SDB_ASSERT( _oldVer->getRecordID() ==
                     dmsRecordID(lockId.extentID(), lockId.offset()),
                     "LockID is not the same" ) ;

         if ( _oldVer->isRecordDeleted() )
         {
            _oldVer = NULL ;
            goto done ;
         }

         _recordPtr = _oldVer->getRecordPtr() ;

         if ( _oldVer->isRecordNew() )
         {
            _skipRecord = TRUE ;
         }
         else if ( _recordPtr.get() && !_oldVer->isRecordDummy() )
         {
            // We need to re-verify the record with the
            // index again. Here is how this could happen:
            // Session 1 did update, changed index from 1 to 2, paused;
            // session 2 does index scan, searching for record with
            // index 2. It found the index on disk. Did get record lock
            // and ended up using the old version from memory. But
            // the old version record contain the index 1. We must verify
            // and skip this record.
            if ( _pScanner && _latchedIdxLid != DMS_INVALID_EXTENT &&
                 SCANNER_TYPE_DISK == _pScanner->getCurScanType() &&
                 _oldVer->idxLidExist( _latchedIdxLid ) )
            {
               _skipRecord = TRUE ;
               /// remove the duplicate rid
               _pScanner->removeDuplicatRID( _oldVer->getRecordID() ) ;
               goto done ;
            }

#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Use old copy for rid[%s] from memory",
                    lockId.toString().c_str() ) ;
#endif
            // setup the buffer pointer in dmsRecordRW
            *_recordRW = dmsMemRecordRW( _recordPtr ) ;

            // set the return info if we successfully used old version
            _useOldVersion = TRUE ;
         }
      } // end of case 1

      // Handle case 2 mentioned above
      // X record lock request from a transaction need to prepare to set
      // up old copy if the copy is not already there
      else if ( SDB_OK == irc )
      {
         if ( DPS_TRANSLOCK_OP_MODE_TEST != opMode )
         {
            SDB_ASSERT( pExtData, "ExtData is invalid " ) ;
            SDB_ASSERT( refCounter > 0, "Ref count must > 0" ) ;
         }

         _recordInfo._refCount = refCounter ;

         if ( !pExtData )
         {
            goto done ;
         }

         /// from memory tree
         if ( _pScanner && _latchedIdxLid != DMS_INVALID_EXTENT &&
              SCANNER_TYPE_MEM_TREE == _pScanner->getCurScanType() )
         {
            _skipRecord = TRUE ;
            /// remove the duplicate rid
            _pScanner->removeDuplicatRID( dmsRecordID( lockId.extentID(),
                                                       lockId.offset() ) ) ;
            _oldVer = NULL ;
            goto done ;
         }

         /// when pExtData->_data is 0, should create oldver
         if ( 0 == pExtData->_data )
         {
            if ( DPS_TRANSLOCK_X == requestLockMode &&
                 DPS_TRANSLOCK_OP_MODE_TEST != opMode &&
                 !notTransOrRollback )
            {
               dmsRecordID rid( lockId.extentID(), lockId.offset() ) ;
               _oldVer = SDB_OSS_NEW oldVersionContainer( rid, _csID, _clID,
                                                          _csLID, _clLID ) ;
               if ( !_oldVer )
               {
                  _result = SDB_OOM ;
                  PD_LOG( PDERROR, "Alloc oldVersionContainer faild" ) ;
                  goto done ;
               }
               pExtData->_data = (UINT64)_oldVer ;

               /// set callback
               pExtData->setValidFunc(
                  (DPS_EXTDATA_VALID_FUNC)dmsIsExtendDataValid ) ;
               pExtData->setReleaseCheckFunc(
                  (DPS_EXTDATA_RELEASE_CHECK)dmsExtendDataReleaseCheck ) ;
               pExtData->setReleaseFunc(
                  (DPS_EXTDATA_RELEASE)dmsReleaseExtendData ) ;
               pExtData->setOnLockReleaseFunc(
                  (DPS_EXTDATA_ON_LOCKRELEASE)dmsOnTransLockRelease ) ;
            }
         }
         else
         {
            _oldVer = (oldVersionContainer*)pExtData->_data ;
            SDB_ASSERT( _oldVer->getRecordID() ==
                        dmsRecordID(lockId.extentID(), lockId.offset()),
                        "LockID is not the same" ) ;

            /// check the record whether is deleted
            if ( DPS_TRANSLOCK_X == requestLockMode )
            {
               if( _oldVer->isRecordDeleted() )
               {
                  BOOLEAN hasLock = -1 != _latchedIdxMode ? TRUE : FALSE ;
                  _oldVer->releaseRecord( _latchedIdxLid, hasLock ) ;

                  PD_LOG( PDDEBUG, "Delete old record for rid[%s] from memory",
                          lockId.toString().c_str() ) ;
               }
            }

            if ( _oldVer->isRecordNew() &&
                 _oldVer->getOwnnerTID() == _eduCB->getTID() )
            {
               _recordInfo._transInsert = TRUE ;
            }

            if ( notTransOrRollback )
            {
               _oldVer = NULL ;
            }
            // Special case is the new record created within the same
            // transaction, we should not setup oldVer for it because
            // there is no older verion for it.
            else if ( _oldVer->isRecordNew() )
            {
               _oldVer = NULL ;
            }
         }

#ifdef _DEBUG
         if ( _oldVer )
         {
            PD_LOG( PDDEBUG, "Set oldVer[%x] for rid[%s] in memory",
                    _oldVer, lockId.toString().c_str() ) ;
         }
#endif //_DEBUG
      }

   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_AFTERLOCKACQUIRE );
      return ;
   }

   // Description:
   //    Function called before lock release(in dpsTransLockManager::_release)
   // Under RC, before X lock is released, the update is responsible
   // to free up the kept old version record and clean up all related indexes
   // from the in-memory index tree. All information were kept in lrbHdr->oldVer
   // The latching protocal has to be:
   // 1. LRB hash bkt latch must be held(X) to tranverse/update lrbHdrs/LRBs
   // 2. preIdxTree latch must be held in X to insert/delete node in the tree,
   //    oldVersionCB(_oldVersionCBLatch) need to be held in S before
   //    accessing individual index tree.
   // 3. Request preIdxTree latch while holding LRB hash bkt latch is forbidden,
   //    But reverse order is OK. Note that the scanner will hold tree latch
   //    and acquire lock.
   //
   // Input:
   //    lockId: lock id to operate on
   //    lockMode: lock mode requested (IS/IX/S/U/X)
   //    refCounter: current reference counter of the lock
   //    oldVer: pointer to oldVersionContainer
   // Output:
   //    oldVer: pointer to oldVersionContainer
   // Dependency:
   //    caller MUST hold the record lock and LRB hash bkt latch in X
   // which protects all the update on oldVer
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE, "dmsTransLockCallback::beforeLockRelease" )
   void dmsTransLockCallback::beforeLockRelease( const dpsTransLockId &lockId,
                                                 DPS_TRANSLOCK_TYPE lockMode,
                                                 UINT32 refCounter,
                                                 const dpsTransLRBHeader *pLRBHeader,
                                                 dpsLRBExtData *pExtData )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE ) ;

      BOOLEAN hasLock = -1 != _latchedIdxMode ? TRUE : FALSE ;
      dmsOnTransLockRelease( lockId, lockMode, refCounter, pExtData,
                             _latchedIdxLid, hasLock ) ;

      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_BEFORELOCKRELEASE );
   }

   // Description
   // Dependency: Caller must hold the mbLcok
   INT32 dmsTransLockCallback::saveOldVersionRecord( const _dmsRecordRW *pRecordRW,
                                                     const dmsRecordID &rid,
                                                     const BSONObj &obj,
                                                     UINT32 ownnerTID )
   {
      INT32  rc      = SDB_OK ;

      // if the oldRecord does not exist, we will create one
      if ( _oldVer && _oldVer->isRecordEmpty() )
      {
         /// when not use rollback segment
         if ( !_eduCB->getTransExecutor()->useRollbackSegment() )
         {
            _oldVer->setRecordDummy( ownnerTID ) ;
         }
         else
         {
            const dmsRecord *pRecord= pRecordRW->readPtr( 0 ) ;

            // 1. get to overflow record if needed
            if ( pRecord->isOvf() )
            {
               dmsRecordID ovfRID = pRecord->getOvfRID() ;
               dmsRecordRW ovfRW = pRecordRW->derive( ovfRID );
               ovfRW.setNothrow( pRecordRW->isNothrow() ) ;
               pRecord = ovfRW.readPtr( 0 ) ;
            }
            // 2. save record
            rc = _oldVer->saveRecord( pRecord, obj, ownnerTID ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         // 3. hang the old version container to the linked list
         if ( !_unitPtr.get() )
         {
            oldVersionCB *oldCB = _transCB->getOldVCB() ;
            rc = oldCB->getOrCreateOldVersionUnit( _csID, _clID, _unitPtr ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         _unitPtr->addToChain( _oldVer ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::_checkInsertIndex( preIdxTreePtr &treePtr,
                                                  _INSERT_CURSOR &insertCursor,
                                                  const ixmIndexCB *indexCB,
                                                  BOOLEAN isUnique,
                                                  BOOLEAN isEnforce,
                                                  const BSONObj &keyObj,
                                                  const dmsRecordID &rid,
                                                  _pmdEDUCB *cb,
                                                  BOOLEAN allowSelfDup,
                                                  utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      // If there is an old verion of the index in our in memory tree, we
      // should not insert the index because the update/delete transaction
      // has not committed yet.
      if ( isUnique && !cb->isInTransRollback() )
      {
         preIdxTreeNodeValue idxValue ;

         if ( !treePtr.get() && _INSERT_NONE == insertCursor )
         {
            oldVersionCB *pOVCB = _transCB->getOldVCB() ;
            globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
            treePtr = pOVCB->getIdxTree( gid, FALSE ) ;
         }
         insertCursor = _INSERT_CHECK ;

         if ( !treePtr.get() )
         {
            goto done ;
         }

         if ( _latchedIdxLid != indexCB->getLogicalID() ||
              -1 == _latchedIdxMode )
         {
            treePtr->lockS() ;
            locked = TRUE ;
         }

         if ( treePtr->isKeyExist( keyObj, idxValue ) )
         {
            // Internally will test X lock on the record to make sure key
            // does not exist and avoid duplicate key false alarm
            // for example:
            //   db.cs.createCL( "c1" )
            //   db.cs.c1.createIndex( "a", {a:1})
            //   db.cs.c1.insert( {a:1, b:1} )
            //   db.transBegin()
            //   db.cs.c1.remove() // or delete { a:1, b:1 }
            //   db.cs.c1.insert({a:1, b:1}) ==> fails due to dupblicate key
            //
            if ( allowSelfDup && idxValue.getOwnnerTID() == cb->getTID() )
            {
               goto done ;
            }
            else if ( !isEnforce && _dmsIsKeyUndefined( keyObj ) )
            {
               goto done ;
            }

            rc = SDB_IXM_DUP_KEY ;
            if ( NULL != pResult )
            {
               INT32 rcTmp = pResult->setIndexErrInfo( indexCB->getName(),
                                                       indexCB->keyPattern(),
                                                       keyObj ) ;
               if ( rcTmp )
               {
                  rc = rcTmp ;
               }
               else
               {
                  pResult->setPeerID( idxValue.getRecordObj() ) ;
               }
            }
            PD_LOG ( PDERROR, "Insert index(%s) key(%s) with rid(%d, %d) "
                     "failed, rc: %d", indexCB->getDef().toString().c_str(),
                     keyObj.toString().c_str(),
                     rid._extent, rid._offset, rc ) ;
            goto error ;
         }
      }

   done:
      if ( locked )
      {
         treePtr->unlockS() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onInsertRecord( _dmsMBContext *context,
                                               const BSONObj &object,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               _pmdEDUCB* cb )
   {
      if ( _oldVer )
      {
         _oldVer->setRecordNew( cb->getTID() ) ;
      }
      return SDB_OK ;
   }

   INT32 dmsTransLockCallback::onDeleteRecord( _dmsMBContext *context,
                                               const BSONObj &object,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               BOOLEAN markDeleting,
                                               _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      rc = saveOldVersionRecord( pRecordRW, rid, object, cb->getTID() ) ;
      if ( SDB_OK == rc && markDeleting && _oldVer )
      {
         _oldVer->setDiskDeleting() ;
      }
      return rc ;
   }

   INT32 dmsTransLockCallback::onUpdateRecord( _dmsMBContext *context,
                                               const BSONObj &orignalObj,
                                               const BSONObj &newObj,
                                               const dmsRecordID &rid,
                                               const _dmsRecordRW *pRecordRW,
                                               _pmdEDUCB *cb )
   {
      return saveOldVersionRecord( pRecordRW, rid, orignalObj, cb->getTID() ) ;
   }

   INT32 dmsTransLockCallback::onInsertIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObjSet &keySet,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB *cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      preIdxTreePtr treePtr ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      for ( BSONObjSet::const_iterator cit = keySet.begin() ;
            cit != keySet.end() ;
            ++cit )
      {
         rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                 isUnique, isEnforce, *cit,
                                 rid, cb, TRUE, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onInsertIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObj &keyObj,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB* cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      preIdxTreePtr treePtr ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      /// create index, don't allow self duplicate
      rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                              isUnique, isEnforce, keyObj,
                              rid, cb, FALSE, pResult ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      PD_LOG ( PDDEBUG, "onInsertIndex with rid(%d, %d) failed, rc=%d  ",
               _transCB, rid._extent, rid._offset, rc ) ;
      goto done ;
   }

   INT32 dmsTransLockCallback::_checkDeleteIndex( preIdxTreePtr &treePtr,
                                                  _DELETE_CURSOR &deleteCursor,
                                                  const ixmIndexCB *indexCB,
                                                  BOOLEAN isUnique,
                                                  const BSONObj &keyObj,
                                                  const dmsRecordID &rid,
                                                  _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLocked = FALSE ;
      globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;

      if ( !_oldVer || cb->isInTransRollback() )
      {
         /// not in transaction, or in trans rollback
         goto done ;
      }
      else if ( _oldVer->isRecordNew() )
      {
         goto done ;
      }
      else if ( _oldVer->isRecordDummy() && !isUnique )
      {
         /// when not use rollback segment
         goto done ;
      }
      else if ( _DELETE_NONE == deleteCursor )
      {
         //    check if oldVer has the index, Note that if it exist in
         //    the set, the index must exist in the tree as well.
         //    This is valid scenario because one transaction can do multiple
         //    update of the same record. But we only need to keep the last
         //    committed version which happened to be the first copy.
         if ( _oldVer->idxLidExist( gid._idxLID ) )
         {
            deleteCursor = _DELETE_IGNORE ;
            goto done ;
         }
         else
         {
            if ( !treePtr.get() )
            {
               oldVersionCB *pVerCB = _transCB->getOldVCB() ;
               rc = pVerCB->getOrCreateIdxTree( gid, indexCB,
                                                treePtr, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Create memory index tree(%s) "
                          "failed, rc: %d", gid.toString().c_str(), rc ) ;
                  goto error ;
               }
            }

            deleteCursor = _DELETE_SAVE ;
            /// insert into oldVer's indexSet
            rc = _oldVer->insertIdxTree( treePtr ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      else if ( _DELETE_IGNORE == deleteCursor )
      {
         goto done ;
      }

      if ( _latchedIdxLid == gid._idxLID && _latchedIdxMode != -1 )
      {
         if ( EXCLUSIVE == _latchedIdxMode )
         {
            hasLocked = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock mode(%d) is not EXCLUSIVE(%d)",
                    _latchedIdxMode, EXCLUSIVE ) ;
            SDB_ASSERT( FALSE, "Lock mode is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      rc = treePtr->insertWithOldVer( &keyObj, rid, _oldVer, hasLocked ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Insert index keys(%s) with rid(%d, %d) "
                  "failed, rc: %d", keyObj.toString().c_str(),
                  rid._extent, rid._offset, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onDeleteIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              const BSONObjSet &keySet,
                                              const dmsRecordID &rid,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
      preIdxTreePtr treePtr ;

      if ( !_transCB || !_transCB->isTransOn() )
      {
         goto done ;
      }

      /// insert key to mem tree
      for ( BSONObjSet::const_iterator cit = keySet.begin() ;
            cit != keySet.end() ;
            ++cit )
      {
         rc = _checkDeleteIndex( treePtr,deleteCursor, indexCB,
                                 isUnique, *cit, rid, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onUpdateIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              BOOLEAN isUnique,
                                              BOOLEAN isEnforce,
                                              const BSONObjSet &oldKeySet,
                                              const BSONObjSet &newKeySet,
                                              const dmsRecordID &rid,
                                              BOOLEAN isRollback,
                                              _pmdEDUCB* cb,
                                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      BSONObjSet::const_iterator itori ;
      BSONObjSet::const_iterator itnew ;
      preIdxTreePtr treePtr ;
      _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
      _INSERT_CURSOR insertCursor = _INSERT_NONE ;
      BOOLEAN hasChanged = FALSE ;
      // check ID index for normal update
      // NOTE: for sequoiadb upgrade, if the old data before upgrade
      //       contains invalid _id field, we could not report error,
      //       we need to allow update if _id field is not changed
      BOOLEAN checkIDIndex = indexCB->isSysIndex() &&
                             !cb->isInTransRollback() &&
                             !cb->isDoRollback() ;

      /// not use transaction
      if ( !_transCB || !_transCB->isTransOn() )
      {
         if ( checkIDIndex )
         {
            // for no transaction, we need to check _id field
            if ( oldKeySet.size() != newKeySet.size() )
            {
               // check length of _id field
               PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to update $id index, "
                         "_id field can't be array or empty" ) ;
            }

            itori = oldKeySet.begin() ;
            itnew = newKeySet.begin() ;
            while ( oldKeySet.end() != itori && newKeySet.end() != itnew )
            {
               if ( 0 != (*itori).woCompare((*itnew), BSONObj(), FALSE ) )
               {
                  PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                            "Failed to update $id index, "
                            "_id field can't be array" ) ;
                  // check _id field
                  rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                               "rc: %d", rc ) ;
               }
               itori++ ;
               itnew++ ;
            }
         }
         goto done ;
      }
      /// rollback
      else if ( isRollback )
      {
         goto done ;
      }

      if ( oldKeySet.size() != newKeySet.size() )
      {
         if ( checkIDIndex )
         {
            // check length of _id field
            PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to update $id index, "
                      "_id field can't be array or empty" ) ;
         }
         hasChanged = TRUE ;
      }

      itori = oldKeySet.begin() ;
      itnew = newKeySet.begin() ;
      while ( oldKeySet.end() != itori && newKeySet.end() != itnew )
      {
         INT32 result = (*itori).woCompare((*itnew), BSONObj(), FALSE ) ;
         if ( 0 == result )
         {
            // new and original are the same, we don't need to change
            // anything in the index
            itori++ ;
            itnew++ ;
         }
         else if ( result < 0 )
         {
            // original smaller than new, that means the original doesn't
            // appear in the new list anymore, let's delete it
            hasChanged = TRUE ;
            itori++ ;
         }
         else
         {
            if ( checkIDIndex )
            {
               PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to update $id index, "
                         "_id field can't be array" ) ;
               // check _id field
               rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                            "rc: %d", rc ) ;
            }

            hasChanged = TRUE ;
            rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                    isUnique, isEnforce, *itnew,
                                    rid, cb, TRUE, pResult ) ;
            if ( rc )
            {
               goto error ;
            }
            itnew++ ;
         }
      }

      // insert rest of itnew
      while ( newKeySet.end() != itnew )
      {
         if ( checkIDIndex )
         {
            PD_CHECK( 1 == newKeySet.size(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to update $id index, "
                      "_id field can't be array" ) ;
            // check _id field
            rc = _checkIDIndexUpdate( rid, (*itnew).firstElement(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update $id index, "
                         "rc: %d", rc ) ;
         }
         rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                 isUnique, isEnforce, *itnew,
                                 rid, cb, TRUE, pResult ) ;
         if ( rc )
         {
            goto error ;
         }
         ++itnew ;
      }

      if ( hasChanged )
      {
         // insert the original index into the in-memory old version tree
         itori = oldKeySet.begin() ;
         while ( oldKeySet.end() != itori )
         {
            rc = _checkDeleteIndex( treePtr, deleteCursor, indexCB,
                                    isUnique, *itori, rid, cb ) ;
            if ( rc )
            {
               goto error ;
            }
            ++itori ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsTransLockCallback::onDropIndex( _dmsMBContext *context,
                                            const ixmIndexCB *indexCB,
                                            _pmdEDUCB *cb )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         pOldVCB->delIdxTree( gid, FALSE ) ;
      }

      return SDB_OK ;
   }

   // when rebuild index, we need to build in memory old version index
   // tree based on all old version record on the collection.
   // Input:
   //    context:  dms MB context
   //    indexCB:  index control block
   //    cb:  edu control block
   // return:
   //    rc:
   // Dependency:
   //    The context and indexCB must be setup.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX, "dmsTransLockCallback::onRebuildIndex" )
   INT32 dmsTransLockCallback::onRebuildIndex( _dmsMBContext *context,
                                               const ixmIndexCB *indexCB,
                                               _pmdEDUCB *cb,
                                               utilWriteResult *pResult )
   {
      INT32   rc         = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX ) ;

      // Input parameter validation ASSERTS
      SDB_ASSERT( context && context->isMBLock(),
                  "Caller should hold mb lock" ) ;
      SDB_ASSERT( indexCB, "indexCB is invalid " ) ;

      if ( _transCB && _transCB->isTransOn() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         oldVersionContainer *oldVer = NULL ;
         preIdxTreePtr treePtr ;
         oldVersionUnitPtr unitPtr ;
         oldVersionUnit::iterator itChain ;

         unitPtr = pOldVCB->getOldVersionUnit( _csID, _clID ) ;
         if ( !unitPtr.get() )
         {
            /// collection chain is not exist
            goto done ;
         }
         itChain = unitPtr->itr() ;

         treePtr = pOldVCB->getIdxTree( gid, FALSE ) ;
         if ( !treePtr.get() || !treePtr->isValid() )
         {
            PD_LOG( PDERROR, "Memory index tree(%s) is not exist",
                    gid.toString().c_str(), rc ) ;
            rc = SDB_DMS_INVALID_INDEXCB ;
            goto error ;
         }
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "RebuildIndex got memtree,gid(%s)",
                 gid.toString().c_str() ) ;
#endif
         // go through the old version link list and insert index to tree
         oldVer = itChain.next() ;
         while ( oldVer )
         {
            if ( oldVer->isRecordDeleted() )
            {
               /// do nothing
            }
            else if ( oldVer->isRecordDummy() )
            {
               if ( indexCB->unique() )
               {
                  /// We can't allow unique index if there are transactions
                  /// with RB segment disable configuration. otherwise
                  /// rollback of those transaction could fail
                  rc = SDB_OPERATION_CONFLICT ;
                  PD_LOG_MSG( PDERROR, "Can't allow create unique index "
                              "when doing some transactions with "
                              "\'transuserbs=false\'" ) ;
                  goto error ;
               }
            }
            else if ( !oldVer->isRecordNew() && oldVer->getRecord() )
            {
               BSONObjSet  keySet ;
               _DELETE_CURSOR deleteCursor = _DELETE_NONE ;
               _INSERT_CURSOR insertCursor = _INSERT_NONE ;
               _oldVer = oldVer ;

               // get the keyset
               rc = indexCB->getKeysFromObject ( oldVer->getRecordObj(),
                                                 keySet ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to get keys from object %s",
                           oldVer->getRecordObj().toString().c_str() ) ;
                  goto error ;
               }

               // We remove old version container from the chain first
               // then releaseRecord when release a lock( dmsOnTransLockRelease,
               // _dmsReleaseLockJob::doit ); when creates an index, walks
               // through the chain and checks if the index LID is in that
               // old version container's index LID set and do insert index
               // LID and index tree ( insertIdxTree and insertWithOldVer )
               // if it is not there.
               // When traverse the chain, add or remove an old version
               // container to or from the chain, proper latch on oldVersionUnit
               // will apply to protect synchronized accessing.

               // insert to the mem tree
               for ( BSONObjSet::const_iterator cit = keySet.begin() ;
                     cit != keySet.end() ;
                     ++cit )
               {
                  rc = _checkInsertIndex( treePtr, insertCursor, indexCB,
                                          indexCB->unique(),
                                          indexCB->enforced(), *cit,
                                          oldVer->getRecordID(), cb,
                                          FALSE, pResult ) ;
                  if ( rc )
                  {
                     goto error ;
                  }

                  rc = _checkDeleteIndex( treePtr,deleteCursor, indexCB,
                                          indexCB->unique(), *cit,
                                          oldVer->getRecordID(), cb ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to insert the key(%s) for obj(%s)",
                              cit->toString().c_str(),
                              oldVer->getRecordObj().toString().c_str() ) ;
                     goto error ;
                  }
               }
#ifdef _DEBUG
               PD_LOG( PDDEBUG, "Inserted key to mem tree for obj(%s)",
                       oldVer->getRecordObj().toString().c_str() ) ;
#endif
            }

            oldVer = itChain.next() ;
         }  // while (oldVer)
         PD_LOG( PDDEBUG, "Rebuild index callback done successfully(%s)",
                 gid.toString().c_str() ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_ONREBUILDINDEX ) ;
      return rc ;

   error :
      PD_LOG( PDERROR, "Rebuild index callback failed, lid=%d, rc=%d",
              indexCB->getLogicalID(), rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX, "dmsTransLockCallback::onCreateIndex" )
   INT32 dmsTransLockCallback::onCreateIndex( _dmsMBContext *context,
                                              const ixmIndexCB *indexCB,
                                              _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX ) ;
      INT32   rc         = SDB_OK ;

      // create in memory tree
      if ( _transCB && _transCB->isTransOn() )
      {
         preIdxTreePtr treePtr ;
         globIdxID gid( _csID, _clID, indexCB->getLogicalID() ) ;
         oldVersionCB *pVerCB = _transCB->getOldVCB() ;

#ifdef _DEBUG
         SDB_ASSERT( !(pVerCB->getIdxTree(gid, FALSE).get()),
                     "Index tree already exist " ) ;
#endif

         rc = pVerCB->addIdxTree( gid, indexCB, treePtr, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create memory index tree(%s) "
                    "failed, rc: %d", gid.toString().c_str(), rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "Create index callback done successfully(%s)",
                 gid.toString().c_str() ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_DMSTRANSLOCKCALLBACK_ONCREATEINDEX ) ;
      return rc ;
   error :
      goto done ;
   }

   void dmsTransLockCallback::onCSClosed( INT32 csID )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         pOldVCB->clearIdxTreeByCSID( csID, FALSE ) ;
         pOldVCB->clearOldVersionUnitByCS( csID ) ;
         PD_LOG( PDDEBUG, "CS close callback done successfully(%d)",
                 csID ) ;
      }
   }

   void dmsTransLockCallback::onCLTruncated( INT32 csID, UINT16 clID )
   {
      if ( _transCB && _transCB->getOldVCB() )
      {
         oldVersionCB *pOldVCB = _transCB->getOldVCB() ;
         pOldVCB->clearIdxTreeByCLID( csID, clID, FALSE ) ;
         pOldVCB->delOldVersionUnit( csID, clID ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE, "dmsTransLockCallback::_checkIDIndexUpdate" )
   INT32 dmsTransLockCallback::_checkIDIndexUpdate( const dmsRecordID &rid,
                                                    const BSONElement &idEle,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE ) ;

      const CHAR *errStr = "" ;

      // check _id field
      if ( !dmsIsRecordIDValid( idEle, FALSE, &errStr ) )
      {
         PD_LOG( PDERROR, "Failed to update $id index, "
                 "_id is error: %s", errStr ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTRANSLOCKCALLBACK__CHKIDIDXUPDATE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}

