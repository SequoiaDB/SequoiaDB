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

   Source File Name = dpsTransLockMgr.hpp

   Descriptive Name = DPS Lock manager header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/28/2018  JT  Initial Draft, locking performance improvement

   Last Changed =

*******************************************************************************/
#ifndef DPSTRANSLOCKMANAGER_HPP_
#define DPSTRANSLOCKMANAGER_HPP_

#include "dpsTransLRB.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "monDps.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"

namespace engine
{
   class _dpsTransExecutor ;
   class _dpsITransLockCallback ;
   class _IContext ;

   enum LOCKMGR_TYPE
   {
      LOCKMGR_TRANS_LOCK = 0,
      LOCKMGR_INDEX_LOCK
   } ;
   #define LOCKMGR_TYPE_MAX ( 2 )

   // trans lock bucket, 524287 ( a prime number close to 524288 )
   #define DPS_TRANS_LOCKBUCKET_SLOTS_MAX ( (UINT32) 524287 )
   // index lock bucket, 131071 ( a prime number close to 131072 )
   // The size of an index normally is around 1/4 of size of
   // the table, so we take 524288 / 4 = 131072 as the size of
   // the index lock bucket
   #define DPS_INDEX_LOCKBUCKET_SLOTS_MAX ( (UINT32) 131071 )

   #define DPS_LOCK_INVALID_BUCKET_SLOT  ( (UINT32) -1 )

   class dpsTransLockManager : public SDBObject
   {
      friend class _dpsTransExecutor ;
   public:
      dpsTransLockManager( LOCKMGR_TYPE managerType );

      virtual ~dpsTransLockManager();

      // initialization,
      INT32 init( UINT32 bucketSize, BOOLEAN autoOperateOnUpperLock ) ;

      // free allocated LRB and LRB Header segments
      void fini() ;

      // if a lock has any waiters ( upgrade and waiter list )
      BOOLEAN hasWait( const dpsTransLockId &lockId );

      // if lock manager has been initialized
      BOOLEAN isInitialized() { return _initialized ; } ;

      // acquire a lock with given mode, the higher level lock ( CL, CS )
      // will be also acquired respectively
      // If a lock can't be acquired right away, the request will be added
      // to waiter/upgrade queue and wait for the lock till it be waken up,
      // lock waiting timeout elapsed, or be interrupted. When it is woken up,
      // it will try to acquire the lock again.
      INT32 acquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         _IContext                * pContext      = NULL,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // release a lock. The higher level intent lock will be also released
      // respectively. It will also wake up the first one in upgrade or
      // waiter queue if it is necessary.
      void release
      (
         _dpsTransExecutor      * dpsTxExectr,
         const dpsTransLockId   & lockId,
         const BOOLEAN            bForceRelease = FALSE,
         _dpsITransLockCallback * callback = NULL
      ) ;

      // release all locks an executor ( EDU ) holding. The executor ( EDU )
      // walks through its EDU LRB chain ( all locks acquired within a tx ),
      // and release all locks its holding and wake up the first one in waiter
      // or upgrade queue if it is necessary.
      void releaseAll
      (
         _dpsTransExecutor      * dpsTxExectr,
         _dpsITransLockCallback * callback = NULL
      ) ;

      // release all index page locks an EDU is holding
      void releaseAllIndexLock( _dpsTransExecutor * dpsTxExectr ) ;

      // count number of index pages being locked by the calling EDU
      UINT32 countAllLocks
      (
         _dpsTransExecutor * dpsTxExectr,
         BOOLEAN             bPrintLog = FALSE,
         CHAR              * memoStr   = NULL
      ) ;

      // try to acquire a lock with given mode and will try to acquire higher
      // level intent lock respectively.
      // If a lock can not be acquired right away, it will NOT put itself in
      // wait/upgrade queue, simply return error SDB_DPS_TRANS_LOCK_INCOMPATIBLE
      INT32 tryAcquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // test if a lock with give lock mode can be acquired, higher level intent
      // lock will also be tested.  It will not acquire the lock nor wait
      // the lock.
      // If the test fails, error code SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be
      // returned.
      INT32 testAcquire
      (
         _dpsTransExecutor        * dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         const BOOLEAN              isPreemptMode = FALSE,
         dpsTransRetInfo          * pdpsTxResInfo = NULL,
         _dpsITransLockCallback   * callback = NULL
      ) ;

      // dump specific lock info to a file for debugging purpose
      void dumpLockInfo
      (
         const dpsTransLockId & lockId,
         const CHAR           * fileName,
         BOOLEAN                bOutputInPlainMode = FALSE
      ) ;

      // dump all holding locks of an executor( EDU ) to a file for debugging.
      // The caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         _dpsTransExecutor * dpsTxExectr,
         const CHAR        * fileName,
         BOOLEAN             bOutputInPlainMode = FALSE
      ) ;

      // dump all holding locks of an executor ( EDU )
      // the caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         dpsTransLRB       * lastLRB,
         VEC_TRANSLOCKCUR  & vecLocks
      ) ;

      // dump the waiting lock info of an executor ( EDU )
      // the caller shall acquire the monitoring( dump ) latch before
      // calling this function and make sure the dpsTxExectr is still valid
      void dumpLockInfo
      (
         dpsTransLRB     * lrb,
         monTransLockCur & lockCur
      ) ;

      // dump a specific lock info ( waiter, owners, upgrade list etc )
      void dumpLockInfo
      (
         const dpsTransLockId & lockId,
         monTransLockInfo     & monLockInfo
      ) ;

      INT32 dumpEDUTransInfo( _dpsTransExecutor *executor,
                              monTransLockCur &waitLock,
                              VEC_TRANSLOCKCUR  &eduLocks ) ;

      // query if the caller is holding a given lock retrun TRUE if it is,
      // FALSE if it isn't; and output owning lock mode and refCount
      // when returns TRUE.
      BOOLEAN isHolding
      (
         _dpsTransExecutor    * dpsTxExectr,
         const dpsTransLockId & lockId,
         INT8                 & owningLockMode,
         UINT32               & refCount
      ) ;

   private:
      // Latch for normal lock operation ( acquire, tryAcquire,
      // testAcquire, release, releaseAll, hasWait etc on ) :
      //     . latch _rwMutext in shared mode
      //     . latch a bucket slot in exclusively
      OSS_INLINE void _acquireOpLatch ( const UINT32  bucketIndex )
      {
         _LockHdrBkt[ bucketIndex ].hashHdrLatch.get() ;
      }

      // release latches for normal lock operation
      OSS_INLINE void _releaseOpLatch ( const UINT32 bucketIndex )
      {
         _LockHdrBkt[ bucketIndex ].hashHdrLatch.release() ;
      }

      // release/return a LRB Header to LRB Header manager
      OSS_INLINE void _releaseLRBHdr( dpsTransLRBHeader * hdrLRB )
      {
         SDB_OSS_DEL hdrLRB ;
      }

      // release/return a LRB to LRB manager
      OSS_INLINE void _releaseLRB( dpsTransLRB * lrb )
      {
         SDB_OSS_DEL lrb ;
      }

      // search the EDU LRB list and find the LRB associated with same lock Id
      dpsTransLRB * _getLRBFromEDULRBList
      (
         _dpsTransExecutor    * dpsTxExectr,
         const dpsTransLockId & lockId
      ) ;

      // get LRB Header poiner of a given lock from LRB Header bucket
      BOOLEAN _getLRBHdrByLockId
      (
         const dpsTransLockId & lockId,
         dpsTransLRBHeader *  & pLRBHdr
      ) ;

      // walk through the LRB list check if the input
      // lockMode is compatible with others, and find
      // first incompatible
      BOOLEAN _checkLockModeWithOthers
      (
         const dpsTransLRB *       lrbBegin,
         _dpsTransExecutor *       dpsTxExectr,
         const DPS_TRANSLOCK_TYPE  requestLockMode,
         dpsTransLRB     *       & pLRBIncompatible
      ) ;


      // add a LRB at the end of the queue ( waiter or upgrade list )
      void _addToLRBListTail
      (
         dpsTransLRB * & lrbBegin,
         dpsTransLRB *   idxNew
      ) ;

      // add a LRB at the beginning of the queue ( owner list )
      void _addToLRBListHead
      (
         dpsTransLRB * & lrbBegin,
         dpsTransLRB *   lrbNew
      ) ;

      // add a LRB at the given position in owner list ( the owner list is
      // sorted on lock mode in descending order )
      void _addToOwnerLRBList
      (
         dpsTransLRB * insPos,
         dpsTransLRB * idxNew
      ) ;

      // set newestIXOwner or newestIXOwner in LRBHdr after
      // an IS or IX LRB is added into owner list
      void _setNewestISIXOwner
      (
         dpsTransLRB *lrbNew
      ) ;

      // search owner LRB list, and find
      //  . if the edu is in owner list
      //  . the position where the new LRB shall be inserted after
      //  . the pointer of first incompatible LRB
      void _searchOwnerLRBList
      (
         const _dpsTransExecutor * dpsTxExectr,
         const DPS_TRANSLOCK_TYPE  lockMode,
         dpsTransLRB *             lrbBegin,
         dpsTransLRB *           & pLRBToInsert,
         dpsTransLRB *           & pLRBIncompatible,
         dpsTransLRB *           & pLRBOwner
      ) ;

      // search owner LRB list, and find
      //  . the position where the new LRB shall be inserted after
      //  . the pointer of first incompatible LRB
      void _searchOwnerLRBListForInsertAndIncompatible
      (
         const _dpsTransExecutor * dpsTxExectr,
         const DPS_TRANSLOCK_TYPE  lockMode,
         dpsTransLRB             * lrbBegin,
         dpsTransLRB *           & pLRBToInsert,
         dpsTransLRB *           & pLRBIncompatible
      ) ;

      // search upgarde LRB list, and verify / find if the requested LRB
      // . may be dead-lock with others in the upgrade queue
      // . the pointer of first LRB that would be dead-lock with the request one
      BOOLEAN _deadlockDetectedWhenUpgrade
      (
         const dpsTransLRB *  lrbBegin,
         const dpsTransLRB *  pLRBTobeChecked,
         dpsTransLRB *     &  pLRBIncompatible
      ) ;

      void _moveToEDULRBListTail
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * insLRB
      ) ;

      // add a LRB at the end of the EDU LRB chain,
      // which is a list of all locks acquired within a session/tx
      void _addToEDULRBListTail
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * lrb,
         const dpsTransLockId & lockId
      ) ;

      // remove a LRB from a LRB list ( owner, waiter, upgrade list )
      void _removeFromLRBList
      (
         dpsTransLRB * & idxBegin,
         dpsTransLRB *   lrb
      ) ;

      // remove LRB from waiter or upgrade queue/list,
      // and wake up the next one in the queue if necessary
      void _removeFromUpgradeOrWaitList
      (
         _dpsTransExecutor    * dpsTxExectr,
         const dpsTransLockId & lockId,
         const UINT32           bktIdx,
         const BOOLEAN          removeLRBHeader
      ) ;


      // remove a LRB Header from a LRB Header list
      void _removeFromLRBHeaderList
      (
         dpsTransLRBHeader * & lrbBegin,
         dpsTransLRBHeader *   lrbDel
      ) ;


      // remove a LRB from the EDU LRB list
      void _removeFromEDULRBList
      (
         _dpsTransExecutor    * dpsTxExectr,
         dpsTransLRB          * lrb,
         const dpsTransLockId & lockId
      ) ;

      // core logic of acquire, try or test to get a lock with given mode
      INT32 _tryAcquireOrTest
      (
         _dpsTransExecutor                * dpsTxExectr,
         const dpsTransLockId             & lockId,
         const DPS_TRANSLOCK_TYPE           requestLockMode,
         const DPS_TRANSLOCK_OP_MODE_TYPE   opMode,
         UINT32                             bktIdx,
         const BOOLEAN                      bktLatched,
         dpsTransRetInfo                  * pdpsTxResInfo,
         _dpsITransLockCallback           * callback = NULL
      ) ;

      // core logic of release a lock
      void _release
      (
         _dpsTransExecutor       * dpsTxExectr,
         const dpsTransLockId    & lockId,
         dpsTransLRB             * pOwnerLRB,
         const BOOLEAN             bForceRelease,
         UINT32                  & refCountToDecrease,
         _dpsITransLockCallback  * callback = NULL
      ) ;

      // acquire and setup a new LRB header and a new LRB
      INT32 _prepareNewLRBAndHeader
      (
         _dpsTransExecutor *        dpsTxExectr,
         const dpsTransLockId     & lockId,
         const DPS_TRANSLOCK_TYPE   requestLockMode,
         const UINT32               bktIdx,
         dpsTransLRBHeader *      & pLRBHdrNew,
         dpsTransLRB       *      & pLRBNew
      ) ;

      // calculate the index to LRB Header bucket
      UINT32 _getBucketNo( const dpsTransLockId &lockId )
      {
         return (UINT32)( lockId.lockIdHash() % _bktSlotMax ) ;
      }

      // wakeup a lock waiting exectuor ( EDU )
      void _wakeUp( _dpsTransExecutor * dpsTxExectr ) ;

      // wait a lock till be woken up, lock timeout elapsed, or be interrupted
      INT32 _waitLock( _dpsTransExecutor * dpsTxExectr ) ;

      // format LRB to string, flat one line
      CHAR * _LRBToString ( dpsTransLRB * lrb, CHAR * pBuf, UINT32 bufSz ) ;

      // format LRB to string, one field/member per line, with optional prefix
      CHAR * _LRBToString
      (
         dpsTransLRB * lrb,
         CHAR *            pBuf,
         UINT32            bufSz,
         CHAR *            prefix
      ) ;

      // format LRB Header to string, flat one line
      CHAR * _LRBHdrToString
      (
         dpsTransLRBHeader * lrbHdr,
         CHAR              * pBuf,
         UINT32              bufSz
      ) ;

      // format LRB Header, one field/member per line, with optional prefix
      CHAR * _LRBHdrToString
      (
         dpsTransLRBHeader * lrbHdr,
         CHAR              * pBuf,
         UINT32              bufSz,
         CHAR              * prefix
      ) ;

   private:
      dpsTransLRBHeaderHash * _LockHdrBkt ;
      UINT32                 _bktSlotMax ;

      // a flag marks if lock manager has been initialized
      BOOLEAN                _initialized ;

      LOCKMGR_TYPE           _lockMgrType ;

      // a flag tells if autogmatically operate on upper level lock
      BOOLEAN                _autoUpperLockOp ;
   } ;
   typedef class dpsTransLockManager ixmIndexLockManager ;
}

#endif // DPSTRANSLOCKMANAGER_HPP_
