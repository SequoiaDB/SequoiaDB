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

   Source File Name = dpsTransLockMgr.cpp

   Descriptive Name = DPS lock manager

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     JT  10/28/2018, locking performance improvement

*******************************************************************************/
#include "dpsDeadlockDetector.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsTransExecutor.hpp"
#include "dpsTransLockCallback.hpp"
#include "dpsTransDef.hpp"
#include "dpsTrace.hpp"
#include "pd.hpp"
#include "dpsTrace.hpp"
#include "pdTrace.hpp"
#include "sdbInterface.hpp"   // IContext
#include "msg.h"              // MsgRouteID

#include <stdio.h>
#if 0
//#ifdef _DEBUG    // once we upgrade the boost, we might want to have this
#include <boost/stacktrace.hpp>
#endif


namespace engine
{

   #define DPS_TRANSLOCK_DUMP_SLICE_SIZE   ( 1000 )

   dpsTransLockManager::dpsTransLockManager( LOCKMGR_TYPE managerType )
   : _LockHdrBkt( NULL ),
     _bktSlotMax( 0 ) ,
     _initialized( FALSE ),
     _lockMgrType( managerType ),
     _autoUpperLockOp( TRUE )
   {
   }

   dpsTransLockManager::~dpsTransLockManager()
   {
      if ( _initialized )
      {
         fini() ;
      }
   }


   //
   // Description: free allocated LRBs, LRB Headers and reset buckets
   // Input:       none
   // Output:      none
   // Return:      none
   // Dependency:  this function is called during system shutdown,
   //              the caller shall make sure no threads are accessing locking
   //              resource.
   void dpsTransLockManager::fini()
   {
      if ( _initialized )
      {
         _initialized = FALSE ;
         if ( _LockHdrBkt )
         {
            SDB_OSS_DEL [] _LockHdrBkt ;
            _LockHdrBkt = NULL ;
         }
      }
   }

   //
   // Description: Initialize lock manager
   //              . initialize bucket
   // Input:
   //    bucketSize             -- hash bucket size, better be a prime number
   //    autoOperateOnUpperLock -- whether automatically operate on upper
   //                              level lock
   // Output:      none
   // Return:      SDB_OK  :  lock manager successefully initialized
   //              SDB_OOM :  failed to initialize lock manager due to
   //                         lack of memory
   // Dependency:  this function is called during system starting up,
   //              the caller shall make sure no thread is trying to access
   //              lock resource before lock manager is fully initialized
   //
   INT32 dpsTransLockManager::init
   (
      UINT32 bucketSize,
      BOOLEAN autoOperateOnUpperLock
   )
   {
      _bktSlotMax      = bucketSize ;
      _autoUpperLockOp = autoOperateOnUpperLock ;

      _LockHdrBkt = SDB_OSS_NEW dpsTransLRBHeaderHash[ _bktSlotMax + 1 ] ;
      if ( NULL == _LockHdrBkt )
      {
         PD_LOG( PDERROR,
                 "Failed to allocate memory for lock bucket, bucket size:%d",
                 _bktSlotMax + 1 ) ;
         return SDB_OOM ;
      }

      // set initialized flag
      _initialized = TRUE ;
      return SDB_OK ;
   }

   //
   //
   // Description: search the LRB Header chain and find the one with same lockId
   // Function:    walk through LRB Header list/chain, find the one with same
   //              lockId.
   // Input:
   //    lockId   -- lock Id
   //    pLRBHdr  -- the first LRB Header object in the chain
   // Output:
   //    pLRBHdr  -- the pointer of first LRB Header object matches
   //                the lockId if it is found. If not, it shall be the
   //                pointer of the last LRB Header object in the list
   //
   // Return:     true  -- found the LRB Header object with same lockId
   //             false -- not found
   //
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_getLRBHdrByLockId
   (
      const dpsTransLockId & lockId,
      dpsTransLRBHeader *  & pLRBHdr
   )
   {
      BOOLEAN found = FALSE ;
      dpsTransLRBHeader * pLocal = pLRBHdr;
      while ( NULL != pLocal )
      {
         pLRBHdr = pLocal;
         if ( lockId == pLocal->lockId )
         {
            found = TRUE ;
            break ;
         }
         pLocal = pLocal->nextLRBHdr ;
      }
      return found ;
   }


   //
   // Description: walk through the LRB list check if the input
   //              lockMode is compatible with others in the queue,
   //              and find the first incompatible
   // Input:
   //    lrbBegin -- the LRB in the queue to start searching
   //    dpsTxExectr --  the request dpsTxExectr
   //    requestLockMode -- the request lock mode
   // Output:
   //    pLRBIncompatible -- the first incompatible LRB
   // Return:
   //    TRUE     -- if an incompatible one is found in the queue
   //    FALSE    -- compatible with owners
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_checkLockModeWithOthers
   (
      const dpsTransLRB *         lrbBegin,
      _dpsTransExecutor        *  dpsTxExectr,
      const DPS_TRANSLOCK_TYPE    requestLockMode,
      dpsTransLRB *            &  pLRBIncompatible
   )
   {
      dpsTransLRB *plrb = (dpsTransLRB *)lrbBegin ;
      BOOLEAN foundIncomp = FALSE ;

      pLRBIncompatible = NULL ;
      if ( NULL == lrbBegin )
      {
         goto exit ;
      }

      while ( plrb )
      {
         if ( ( dpsTxExectr != plrb->dpsTxExectr ) &&
              ( ! dpsIsLockCompatible( plrb->lockMode, requestLockMode ) ) )
         {
            pLRBIncompatible = plrb ;
            foundIncomp = TRUE ;
            break ;
         }
         plrb = plrb->nextLRB ;
      }

   exit :
      return foundIncomp ;
   }

   //
   // Description: walk through the upgrade list check if the request
   //              LRB might be dead-lock with others, and
   //              and find the first incompatible one
   // Input:
   //    lrbBegin -- the LRB in the uprade queue to start searching
   //    pLRBTobeChecked -- the input LRB to be checked with others
   // Output:
   //    pLRBIncompatible -- the first incompatible LRB
   // Return:
   //    TRUE     -- if a potential dead-lock issue is detected`
   //    FALSE    -- no dead-lock deteced
   // Dependency:  the lock bucket latch shall be acquired
   //
   BOOLEAN dpsTransLockManager::_deadlockDetectedWhenUpgrade
   (
      const dpsTransLRB *  lrbBegin,
      const dpsTransLRB *  pLRBTobeChecked,
      dpsTransLRB *     &  pLRBIncompatible
   )
   {
      dpsTransLRB *plrb = (dpsTransLRB *)lrbBegin ;
      BOOLEAN foundIncomp = FALSE ;

      pLRBIncompatible = NULL ;
      if ( ( NULL == pLRBTobeChecked ) || ( NULL == lrbBegin ) )
      {
         goto exit ;
      }

      while ( plrb )
      {
         if ( ( ! dpsIsLockCompatible( plrb->originMode,
                                       pLRBTobeChecked->lockMode ) ) ||
              ( ! dpsIsLockCompatible( plrb->lockMode,
                                       pLRBTobeChecked->originMode ) ) )
         {
            pLRBIncompatible = plrb ;
            foundIncomp = TRUE ;
            break ;
         }
         plrb = plrb->nextLRB ;
      }

   exit :
      return foundIncomp ;
   }


   //
   // Description: add a LRB at the end of the LRB chain/list
   // Function:    walk through LRB list/chain, add the LRB at the end of
   //              list( owner, waiter or upgrade )
   // Input:
   //    lrbBegin -- the first LRB in the chain( owner, waiter or upgrade
   //                queue )
   //    lrbNew   -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToLRBListTail
   (
      dpsTransLRB* & lrbBegin,
      dpsTransLRB*   lrbNew
   )
   {
      if ( lrbBegin )
      {
         dpsTransLRB * plrb = lrbBegin ;

         // find the last LRB
         while ( plrb->nextLRB )
         {
            plrb = plrb->nextLRB ;
         }
         // add this new LRB behind the last LRB
         plrb->nextLRB = lrbNew ;
         if ( lrbNew )
         {
            lrbNew->prevLRB = plrb ;
            lrbNew->nextLRB = NULL ;
         }
      }
      else
      {
         lrbBegin = lrbNew ;
         if ( lrbNew )
         {
            lrbNew->prevLRB = NULL ;
            lrbNew->nextLRB = NULL ;
         }
      }
   }


   //
   // Description: add a LRB at the head of the LRB chain/list
   // Function:    add the LRB at the head of list
   // Input:
   //    lrbBegin -- the first LRB in the chain
   //    lrbNew   -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToLRBListHead
   (
      dpsTransLRB * & lrbBegin,
      dpsTransLRB *   lrbNew
   )
   {
      if ( lrbBegin )
      {
         lrbBegin->prevLRB  = lrbNew ;
      }
      if ( lrbNew )
      {
         lrbNew->nextLRB = lrbBegin ;
         lrbNew->prevLRB = NULL ;
      }
      lrbBegin = lrbNew ;
   }


   //
   // Description: add a LRB to owner list right after a given LRB
   // Function:    the owner list is sorted on lock mode in descending order,
   //              add a LRB right after the given LRB
   // Input:
   //    lrbPos -- the LRB where the new LRB is being inserted after
   //
   //    lrbNew -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToOwnerLRBList
   (
      dpsTransLRB *insPos,
      dpsTransLRB *lrbNew
   )
   {
      if ( insPos && lrbNew )
      {
         lrbNew->nextLRB = insPos->nextLRB ;
         lrbNew->prevLRB = insPos ;
         if ( insPos->nextLRB )
         {
            insPos->nextLRB->prevLRB = lrbNew ;
         }
         insPos->nextLRB = lrbNew ;
      }
   }


   //
   // Description: after add an IX or IS LRB to owner list,
   //              update the newestIXOwnerList or newestISOwnerList in LRBHdr
   // Input:
   //    lrbNew -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_setNewestISIXOwner
   (
      dpsTransLRB *lrbNew
   )
   {
      if ( lrbNew )
      {
         if ( DPS_TRANSLOCK_IS == lrbNew->lockMode )
         {
            lrbNew->lrbHdr->newestISOwner = lrbNew ;
         }
         else if ( DPS_TRANSLOCK_IX == lrbNew->lockMode )
         {
            lrbNew->lrbHdr->newestIXOwner = lrbNew ;
         }
      }
   }


   //
   // Description: search owner LRB list and find the expected LRB
   // Function:    walk through owner LRB list ( it is sorted on lockMode in
   //              descending order ) and find out :
   //              . if the edu is in owner list
   //              . the pointer which the new LRB shall be inserted after
   //              . the pointer of last compatible and pointer of first
   //                incompatible LRB
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   //    lockMode    -- lock mode
   //    lrbBegin    -- the first LRB pointer in the owner list
   // Output:
   //    pLRBToInsert      -- the lrb which the new LRB shall be inserted after
   //    pLRBIncompatible  -- pointer of first incompatible LRB
   //    pLRBOwner         -- the LRB owned by same dpsTxExectr ( eduId )
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired and
   //              all output parameters are initialized as NULL
   void dpsTransLockManager::_searchOwnerLRBList
   (
      const _dpsTransExecutor * dpsTxExectr,
      const DPS_TRANSLOCK_TYPE  lockMode,
      dpsTransLRB             * lrbBegin,
      dpsTransLRB *           & pLRBToInsert,
      dpsTransLRB *           & pLRBIncompatible,
      dpsTransLRB *           & pLRBOwner
   )
   {
#ifdef _DEBUG
      SDB_ASSERT( ( NULL == pLRBToInsert ),     "Invalid pLRBToInsert" ) ;
      SDB_ASSERT( ( NULL == pLRBIncompatible ), "Invalid pLRBIncompatible" ) ;
      SDB_ASSERT( ( NULL == pLRBOwner ),        "Invalid pLRBOwner" ) ;
#endif
      dpsTransLRB *plrb = lrbBegin, *plrbPrev = NULL;
      BOOLEAN foundIns = FALSE ;

      while ( plrb )
      {
         // check if this LRB owned by the requested EDU, i.e., owner LRB
         if ( NULL == pLRBOwner )
         {
            if ( dpsTxExectr == plrb->dpsTxExectr )
            {
               // save the pointer if the given eduId is found in the owner list
               pLRBOwner = plrb;
            }
         }

         // to find the insert position of this LRB
         // the owner list is sorted on lock mode in descending order
         if ( ! foundIns )
         {
            if ( lockMode >= plrb->lockMode )
            {
               // save the LRB pointer where it shall be inserted after
               pLRBToInsert = plrb->prevLRB ;
               foundIns     = TRUE ;
            }
         }

         // check if the requested lock mode is compatible other owners.
         // If not, remember the first incompatible one.
         if ( NULL == pLRBIncompatible )
         {
            if (    ( dpsTxExectr != plrb->dpsTxExectr )
                 && ( ! dpsIsLockCompatible( plrb->lockMode, lockMode ) ) )
            {
               // save the address/pointer of first incompatible LRB
               pLRBIncompatible = plrb ;
            }
         }

         // move to next
         plrbPrev = plrb ;
         plrb = plrb->nextLRB ;
      }

      if ( ( NULL != lrbBegin ) && ( ! foundIns ) )
      {
         // if the request lock mode is smaller than all owners,
         // the insert position is the end of the list
         pLRBToInsert = plrbPrev ;
      }
   }


   //
   // Description: search owner LRB list and find the expected LRB
   // Function:    walk through owner LRB list ( it is sorted on lockMode in
   //              descending order ) and find out :
   //              . the pointer which the new LRB shall be inserted after
   //              . the pointer of last compatible and pointer of first
   //                incompatible LRB
   // Input:
   //    lockMode -- lock mode
   //    lrbBegin -- the first LRB pointer in the owner list
   //    dpsTxExectr -- current requester _dpsTransExecutor pointer
   // Output:
   //    pLRBToInsert      -- the lrb which the new LRB shall be inserted after
   //    pLRBIncompatible  -- pointer of first incompatible LRB
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired and
   //              all output parameters are initialized as NULL
   void dpsTransLockManager::_searchOwnerLRBListForInsertAndIncompatible
   (
      const _dpsTransExecutor * dpsTxExectr,
      const DPS_TRANSLOCK_TYPE  lockMode,
      dpsTransLRB             * lrbBegin,
      dpsTransLRB *           & pLRBToInsert,
      dpsTransLRB *           & pLRBIncompatible
   )
   {
#ifdef _DEBUG
      SDB_ASSERT( ( NULL == pLRBToInsert ),     "Invalid pLRBToInsert" ) ;
      SDB_ASSERT( ( NULL == pLRBIncompatible ), "Invalid pLRBIncompatible" ) ;
#endif
      dpsTransLRB *plrb = lrbBegin, *plrbPrev = NULL;
      BOOLEAN foundIns = FALSE ;

      while ( plrb )
      {
         // to find the insert position of this LRB
         // the owner list is sorted on lock mode in descending order
         if ( ! foundIns )
         {
            // if request mode is IS or IX
            if ( DPS_TRANSLOCK_IX >= lockMode )
            {
               dpsTransLRBHeader * pLRBHdr = plrb->lrbHdr ;

               if ( DPS_TRANSLOCK_IS == lockMode )
               {
                  if ( pLRBHdr->newestISOwner )
                  {
                     // when pLRBHdr->newestISOwner->prevLRB is NULL,
                     // it implies that is the first one in the list.
                     // The later code ( add to the new LRB after
                     // this pLRBToInsert into owner list ) shall
                     // be able to handle this case.
                     pLRBToInsert = pLRBHdr->newestISOwner->prevLRB ;
                     foundIns     = TRUE ;
                  }
               }
               else
               {
                  if ( pLRBHdr->newestIXOwner )
                  {
                     // ditto, when pLRBHdr->newestIXOwner->prevLRB is NULL,
                     // implies that is the first one in the list.
                     pLRBToInsert = pLRBHdr->newestIXOwner->prevLRB ;
                     foundIns     = TRUE ;
                  }
               }
            }
            if ( ( !foundIns ) && ( lockMode >= plrb->lockMode ) )
            {
               // save the LRB pointer where it shall be inserted after
               pLRBToInsert = plrb->prevLRB ;
               foundIns     = TRUE ;
            }
         }

         if ( ( DPS_TRANSLOCK_IX >= lockMode ) && foundIns )
         {
            // as the owner list is sorted on lockMode in
            // descending order, when IS/IX lock request found
            // the position to insert, no need to walk through
            // rest of the list to search for incompatible,
            // beause IX/IS lock is the lowest lock mode value,
            // i.e., stays at the tail end of the owner list,
            // if there is an incompatible one in the list,
            // it must be found already since it has greater
            // lock mode value than IX/IS lock
            break ;
         }

         // check if the requested lock mode is compatible other owners.
         // If not, remember the first incompatible one.
         if ( NULL == pLRBIncompatible )
         {
            if (    ( dpsTxExectr != plrb->dpsTxExectr )
                 && ( ! dpsIsLockCompatible( plrb->lockMode, lockMode ) ) )
            {
               pLRBIncompatible = plrb ;
            }
         }

         // early exit if all jobs are done (insert position, incompatible LRB)
         if ( foundIns && pLRBIncompatible )
         {
            break ;
         }

         // move to next
         plrbPrev = plrb ;
         plrb = plrb->nextLRB ;
      }

      if ( ( NULL != lrbBegin ) && ( ! foundIns ) )
      {
         // if the request lock mode is smaller than all owners,
         // the insert position is the end of the list
         pLRBToInsert = plrbPrev ;
      }
   }


   //
   // Description: add a LRB at the end of the EDU LRB chain, which is the list
   //              of all locks acquired within a session/tx
   // Function:    walk through EDU LRB chain( doubly linked list ),
   //              add the LRB at the end of list.
   //
   //              the dpsTxExectr::_lastLRB is the latest LRB
   //              acquired within the same tx
   // Input:
   //    dpsTxExectr -- _dpsTransExecutor ptr
   //    insLRB      -- the LRB to be added in
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_addToEDULRBListTail
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * insLRB,
      const dpsTransLockId & lockId
   )
   {
      if ( insLRB )
      {
         // get the pointer of last LRB in the EDU LRB chain
         // and add the new LRB into the chain
         dpsTransLRB *plrb = dpsTxExectr->getLastLRB( _lockMgrType ) ;
         if ( plrb )
         {
            plrb->eduLrbNext   = insLRB ;
            insLRB->eduLrbPrev = plrb ;
            insLRB->eduLrbNext = NULL ;
         }
         else
         {
            insLRB->eduLrbPrev = NULL ;
            insLRB->eduLrbNext = NULL ;
         }

         // set this newly inserted lrb as the last LRB
         dpsTxExectr->setLastLRB( insLRB, _lockMgrType ) ;

         // add to executor non-leaf lock map if automatically operating on
         // upper level lock flag is turned on
         if ( _autoUpperLockOp )
         {
            dpsTxExectr->addLock( lockId, insLRB, _lockMgrType ) ;
         }

         // increase the lock count
         dpsTxExectr->incLockCount( _lockMgrType, lockId.isLeafLevel() ) ;

         // clear the wait info in dpsTxExectr
         dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
      }
   }

   //
   // Description: move a LRB to the end of the EDU LRB list
   // Function:    move a LRB to the end of the EDU LRB list
   //
   //              The reason for this is to ensure that during
   //              lock release, it can search the correct LRB
   //              through EDU LRB list in constant time
   // Input:
   //    dpsTxExectr -- _dpsTransExecutor ptr
   //    insLRB      -- the LRB to be moved
   // Output:     None
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //              insLRB is not NULL
   //
   void dpsTransLockManager::_moveToEDULRBListTail
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * insLRB
   )
   {
      dpsTransLRB *plrb = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      if ( plrb == insLRB)
      {
         return ;
      }

      dpsTxExectr->acquireLRBAccessingLock( _lockMgrType ) ;
      dpsTransLRB *accesingLRB = dpsTxExectr->getAccessingLRB( _lockMgrType ) ;
      if ( accesingLRB != NULL && accesingLRB == insLRB )
      {
         // ignore recycling the lrb
         dpsTxExectr->setAccessingLRB( _lockMgrType, insLRB->eduLrbPrev ) ;
      }
      dpsTxExectr->releaseLRBAccessingLock( _lockMgrType ) ;

      if ( insLRB->eduLrbPrev )
      {
         insLRB->eduLrbPrev->eduLrbNext = insLRB->eduLrbNext ;
      }
      if ( insLRB->eduLrbNext )
      {
         insLRB->eduLrbNext->eduLrbPrev = insLRB->eduLrbPrev ;
      }

      plrb->eduLrbNext   = insLRB ;
      insLRB->eduLrbPrev = plrb ;
      insLRB->eduLrbNext = NULL ;

      // set this newly inserted lrb as the last LRB
      dpsTxExectr->setLastLRB( insLRB, _lockMgrType ) ;
   }

   //
   // Description: remove a LRB from a LRB chain
   // Function:    walk through the LRB chain(linked list, it can be lock owner
   //              list, lock waiter list or upgrade list ) and remove the LRB
   //              from the chain. Please note, it doesn't release/return the
   //              LRB to the LRB manager.
   // Input:
   //    beginLRB -- the LRB in the list that begin to search
   //    delLRB   -- the LRB object to be removed
   //
   // Output:
   //    nextLRB  -- the next LRB to the delLRB
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromLRBList
   (
      dpsTransLRB*     & beginLRB,
      dpsTransLRB*       delLRB
   )
   {
      if ( beginLRB && delLRB )
      {
         dpsTransLRBHeader * pLRBHdr = delLRB->lrbHdr ;

         // if remove from owner list
         if ( beginLRB == pLRBHdr->ownerLRB )
         {
            // if delLRB is the newestISOwner or newestIXOwner
            if ( delLRB == pLRBHdr->newestISOwner )
            {
               if ( ( delLRB->nextLRB ) &&
                    ( DPS_TRANSLOCK_IS == delLRB->nextLRB->lockMode ) )
               {
                  pLRBHdr->newestISOwner = delLRB->nextLRB ;
               }
               else
               {
                  pLRBHdr->newestISOwner = NULL ;
               }
            }
            else if ( delLRB == pLRBHdr->newestIXOwner )
            {
               if ( ( delLRB->nextLRB ) &&
                    ( DPS_TRANSLOCK_IX == delLRB->nextLRB->lockMode ) )
               {
                  pLRBHdr->newestIXOwner = delLRB->nextLRB ;
               }
               else
               {
                  pLRBHdr->newestIXOwner = NULL ;
               }
            }
         }

         // if the first one is the one to be removed
         if ( delLRB == beginLRB )
         {
            beginLRB = delLRB->nextLRB ;
            if ( delLRB->nextLRB )
            {
               delLRB->nextLRB->prevLRB = NULL ;
            }
         }
         else
         {
            SDB_ASSERT( delLRB->prevLRB, "Invalid LRB, prevLRB is NULL" ) ;
            {
               delLRB->prevLRB->nextLRB = delLRB->nextLRB ;
            }
            if ( delLRB->nextLRB )
            {
               delLRB->nextLRB->prevLRB = delLRB->prevLRB ;
            }
         }
         delLRB->nextLRB = NULL ;
         delLRB->prevLRB = NULL ;
      }
   }


   //
   // Description: remove waiter from waiter or upgrade queue/list
   // Function:    remove the waiter LRB ( dpsTxExectr->getWaiterLRBIdx() )
   //              from upgrade or waiter queue/list, and wakeup the next
   //              one if necessary
   // REVISIT:
   // Here are two cases :
   // 1. an EDU was waken up ( _waitLock returned SDB_OK )
   //    it checks next waiters lock mode whether compatible with itself
   //    as itself will retry acquire the lock. Only wake up next waiter
   //    if the lock mode is compatbile with current waiter.
   //
   // 2. an EDU was timeout / interrupted from _waitLock
   //    if the owner list is empty, we may wake up the next waiter, as
   //    the current one will not retry acquire the lock.
   //
   //    alternatives :
   //      a. treat this EDU same as been waken up case and retry acquire.
   //      b. do nothing, next waiter(s) can timeout same as this EDU.
   //
   // Input:
   //    dpsTxExectr     -- pointer to _dpsTransExecutor
   //    removeLRBHeader -- whether remove LRB Header when owner, waiter,
   //                       upgrade queue are all empty
   //                       when it is true, it means _waitLock returned
   //                       non SDB_OK value, due to either lock waiting
   //                       timeout elapsed or be interrupted.
   // Output:  none
   // Return:  none
   // Dependency:  the lock bucket latch shall be acquired
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST, "dpsTransLockManager::_removeFromUpgradeOrWaitList" )
   void dpsTransLockManager::_removeFromUpgradeOrWaitList
   (
      _dpsTransExecutor *    dpsTxExectr,
      const dpsTransLockId & lockId,
      const UINT32           bktIdx,
      const BOOLEAN          removeLRBHeader
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST ) ;

      dpsTransLRB *pLRB = dpsTxExectr->getWaiterLRB( _lockMgrType ) ;
      dpsTransLRB *pLRBNext = NULL;
      dpsTransLRB *pLRBIncompatible = NULL ;
      dpsTransLRBHeader * pLRBHdr = NULL ;

#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE6( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_UINT( bktIdx ),
                 PD_PACK_UINT( removeLRBHeader ),
                 PD_PACK_STRING( "LRB to be removed:" ),
                 PD_PACK_RAW( &pLRB, sizeof(&pLRB) ) ) ;
#endif

      if ( pLRB )
      {
#ifdef _DEBUG
         SDB_ASSERT( pLRB->lrbHdr != NULL, "Invalid LRB Header." );
#endif
         pLRBHdr  = pLRB->lrbHdr ;
         pLRBNext = pLRB->nextLRB ;

         // sanity check, panic if fails.
         if ( ! ( pLRBHdr->lockId == lockId ) )
         {
            PD_LOG( PDSEVERE,
                    "Fatal error, requested lockId doesn't match LRB Header."
                    "Requested lockId:%s, lockId in LRB Header %s",
                    lockId.toString().c_str(),
                    pLRBHdr->lockId.toString().c_str() ) ;
            ossPanic() ;
         }

         if ( DPS_QUE_UPGRADE == dpsTxExectr->getWaiterQueType( _lockMgrType ) )
         {
            // remove from upgrade list
            _removeFromLRBList( pLRBHdr->upgradeLRB, pLRB ) ;
         }
         else if ( DPS_QUE_WAITER ==
                   dpsTxExectr->getWaiterQueType( _lockMgrType ) )
         {
            // remove from waiter list
            _removeFromLRBList( pLRBHdr->waiterLRB, pLRB ) ;
         }

         // no need to clear the status of current pLRB
         // since it will be released
         // OSS_BIT_CLEAR( pLRB->status, DPS_LRB_STATUS_AWAKE ) ;

         // clear the wait info in dpsTxExectr
         dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;

         // get the waiter LRB pointer
         if ( pLRBHdr->upgradeLRB )
         {
            pLRBNext = pLRBHdr->upgradeLRB ;
         }
         else if ( pLRBHdr->waiterLRB  )
         {
            pLRBNext = pLRBHdr->waiterLRB  ;
         }

         // wake up the next waiting one if necessary.
         // Here are two cases :
         // 1. an EDU was waken up ( _waitLock returned SDB_OK )
         //    it checks next waiters lock mode whether compatible with itself
         //    as itself will retry acquire the lock. Only wake up next waiter
         //    if the lock mode is compatbile with current waiter.
         //
         // REVISIT
         //
         // 2. an EDU was timeout / interrupted from _waitLock
         //    if the owner list is empty, we may wake up the next waiter, as
         //    the current one will not retry acquire the lock.
         //
         //    another approach is treat this EDU same as been waken up case
         //    and retry acquire. Or, do nothing let other waiters timeout.
         //
         if ( pLRBNext &&
              ( ! OSS_BIT_TEST( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ) )
         {
#ifdef _DEBUG
            SDB_ASSERT( pLRB->lrbHdr == pLRBNext->lrbHdr, "Invalid LRB" ) ;
#endif
            // the EDU was waken up ( _waitLock returned SDB_OK )
            if ( FALSE == removeLRBHeader )
            {
               if ( dpsIsLockCompatible( pLRB->lockMode, pLRBNext->lockMode ) )
               {
                  // wake up next waiter if the lock mode is compatible
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
            }
            else
            {
               // the _waitLock() returned timeout or interrupted

               if ( ! pLRBHdr->ownerLRB )
               {
                  // wake up next waiter if owner list is empty
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
               else if ( ! _checkLockModeWithOthers( pLRBHdr->ownerLRB,
                                                     pLRBNext->dpsTxExectr,
                                                     pLRBNext->lockMode,
                                                     pLRBIncompatible ) )
               {
                  // wake up next waiter if it is compatible with all owners :
                  //  . A is holding U lock
                  //  . B requests U and is in waiter queue
                  //  . C requests S and is put in waiter queue
                  //  B timed out, it shall try to wake up C if C is compatible
                  //  with current owners.
                  OSS_BIT_SET( pLRBNext->status, DPS_LRB_STATUS_AWAKE ) ;
                  _wakeUp( pLRBNext->dpsTxExectr ) ;
               }
            }
         }

         // release the waiter LRB
         _releaseLRB( pLRB ) ;
         pLRB = NULL ;

         // when LRB Header is empty, release it if necessary
#ifdef _DEBUG
         SDB_ASSERT( ( pLRBHdr ), "Invalid LRB Header." ) ;
#endif
         // if extData hasn't been setup/initialized,
         // canRelease() will return TRUE
         if (    removeLRBHeader
              && ( !  pLRBHdr->ownerLRB   )
              && ( !  pLRBHdr->upgradeLRB )
              && ( !  pLRBHdr->waiterLRB  )
              && ( pLRBHdr->extData.canRelease() ) )
         {
#ifdef _DEBUG
            PD_TRACE2( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST,
                          PD_PACK_STRING( "LRB Header to be removed:" ),
                          PD_PACK_RAW( &pLRBHdr, sizeof(&pLRBHdr) ) ) ;
#endif
            // remove the LRB Header from the list
            _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
            // release the LRB Header
            //
            _releaseLRBHdr( pLRBHdr ) ;
            pLRBHdr = NULL ;
         }
      }
      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__REMOVEFROMUPGRADEORWAITLIST ) ;
   }


   //
   // Description: remove a LRB Header from a LRB Header chain
   // Function:    walk through the LRB Header chain( linked list ) and remove
   //              the LRB Header from the chain. Please note, it doesn't
   //              release/return the LRB Header to the LRB Header manager.
   // Input:
   //    lrbBegin -- the first LRB Header in the list
   //    lrbDel   -- the LRB Header object to be removed
   //
   // Output:
   //    lrbBegin -- if the lrbBegin is same as lrbDel, it will be updated with
   //                the LRB Header next to lrbDel
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromLRBHeaderList
   (
      dpsTransLRBHeader* & lrbBegin,
      dpsTransLRBHeader*   lrbDel
   )
   {
      if ( ( NULL != lrbBegin ) && ( NULL != lrbDel ) )
      {
         dpsTransLRBHeader *plrbHdr = lrbBegin;

         // if the first one is the one to be removed
         if ( lrbDel == lrbBegin )
         {
            lrbBegin = lrbDel->nextLRBHdr ;
         }
         else
         {
            while ( NULL != plrbHdr )
            {
               if ( lrbDel == plrbHdr->nextLRBHdr )
               {
                  plrbHdr->nextLRBHdr = lrbDel->nextLRBHdr ;
                  break ;
               }
               plrbHdr = plrbHdr->nextLRBHdr ;
            }
         }
         lrbDel->nextLRBHdr = NULL ;
      }
   }


   //
   // Description: remove a LRB from the EDU LRB chain
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and remove the LRB Header from the chain.
   //              Please note, it doesn't release/return the LRB to
   //              the LRB Header manager.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    delLrb       -- the LRB object to be removed
   //
   // Output:
   //    dpsTxExectr->_lastLRB -- the last LRB object
   //                             in the EDU LRB chain.
   //                             If it is equal to delLrb, it will be
   //                             updated with the LRB previous
   //                             to delLrb
   // Return:     None
   // Dependency:  the lock bucket latch shall be acquired
   //
   void dpsTransLockManager::_removeFromEDULRBList
   (
      _dpsTransExecutor    * dpsTxExectr,
      dpsTransLRB          * delLRB,
      const dpsTransLockId & lockId
   )
   {
      if ( dpsTxExectr && delLRB && dpsTxExectr->getLastLRB( _lockMgrType ) )
      {
         dpsTxExectr->acquireLRBAccessingLock( _lockMgrType ) ;
         dpsTransLRB *accesingLRB = dpsTxExectr->getAccessingLRB(
                                                                _lockMgrType ) ;
         if ( accesingLRB != NULL && accesingLRB == delLRB )
         {
            dpsTxExectr->setAccessingLRB( _lockMgrType,
                                          delLRB->eduLrbPrev ) ;
         }
         dpsTxExectr->releaseLRBAccessingLock( _lockMgrType ) ;

         if ( delLRB->eduLrbPrev )
         {
            delLRB->eduLrbPrev->eduLrbNext = delLRB->eduLrbNext ;
         }
         if ( delLRB->eduLrbNext )
         {
            delLRB->eduLrbNext->eduLrbPrev = delLRB->eduLrbPrev ;
         }
         // set new last LRB if I am the last one
         if ( dpsTxExectr->getLastLRB( _lockMgrType ) == delLRB )
         {
            dpsTxExectr->setLastLRB( delLRB->eduLrbPrev, _lockMgrType ) ;
         }
         // remove it from executor non-leaf lock map
         // if automatically operating on upper level lock flag is turned on
         if ( _autoUpperLockOp )
         {
            dpsTxExectr->removeLock( lockId, _lockMgrType ) ;
         }

         // decrease the lock count
         dpsTxExectr->decLockCount( _lockMgrType, lockId.isLeafLevel() ) ;
         delLRB->eduLrbPrev = NULL ;
         delLRB->eduLrbNext = NULL ;
      }
   }


   //
   // Description: acquire, try or test to get a lock with given mode
   // Function: core of acquire, try or test operation, behaviour varies
   //   depending operation mode :
   //   1 DPS_TRANSLOCK_OP_MODE_ACQUIRE
   //     return SDB_OK
   //       . lock acquired, new LRB is added in owner list
   //       . if holing higher level lock, no need to add new LRB in owner list
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_APPEND_TO_WAIT
   //       . need to upgrade, new LRB is added to upgrade list
   //       . need to wait, new LRB is added to waiter list
   //   2 DPS_TRANSLOCK_OP_MODE_TRY
   //     try mode will not add LRB to waiter or upgrade list
   //     return SDB_OK
   //       . lock acquired, new LRB is added in owner list
   //       . holing higher level lock, no need to add new LRB in owner list
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_LOCK_INCOMPATIBLE
   //       . request lock mode can't be acquired
   //   3 DPS_TRANSLOCK_OP_MODE_TEST, DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT
   //     return SDB_OK
   //       . request lock can be acquired
   //     return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST
   //       . can't upgrade to requested lock mode
   //     return SDB_DPS_TRANS_LOCK_INCOMPATIBLE
   //       . request lock mode can't be acquired
   //
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    opMode          -- try     ( DPS_TRANSLOCK_OP_MODE_TRY )
   //                       acquire ( DPS_TRANSLOCK_OP_MODE_ACQUIRE )
   //                       test    ( DPS_TRANSLOCK_OP_MODE_TEST )
   //                       testPreempt( DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT )
   //    bktIdx          -- bucket index
   //    bktLatched      -- if bucket is already latched
   //
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_APPEND_TO_WAIT,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST, "dpsTransLockManager::_tryAcquireOrTest" )
   INT32 dpsTransLockManager::_tryAcquireOrTest
   (
      _dpsTransExecutor                * dpsTxExectr,
      const dpsTransLockId             & lockId,
      const DPS_TRANSLOCK_TYPE           requestLockMode,
      const DPS_TRANSLOCK_OP_MODE_TYPE   opMode,
      UINT32                             bktIdx,
      const BOOLEAN                      bktLatched,
      dpsTransRetInfo                  * pdpsTxResInfo,
      _dpsITransLockCallback           * callback,
      DPS_TRANSLOCK_TYPE               * ownedLockMode
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      INT32 rc    = SDB_OK ;

      // copy lock mode
      DPS_TRANSLOCK_TYPE lockMode   = requestLockMode ;

      dpsTransLRB *pLRBNew          = NULL ,
                  *pLRBIncompatible = NULL ,
                  *pLRBDeadlock     = NULL ,
                  *pLRBToInsert     = NULL ,
                  *pLRBOwner        = NULL ,
                  *pLRB             = NULL ;

      dpsTransLRBHeader *pLRBHdrNew = NULL ,
                        *pLRBHdr    = NULL ;

      BOOLEAN bFreeLRB       = FALSE ,
              bFreeLRBHeader = FALSE ,
              bLatched       = FALSE ;

      BOOLEAN testMode = ( DPS_TRANSLOCK_OP_MODE_TEST == opMode ||
                           DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT == opMode ) ;

#ifdef _DEBUG
      EDUID eduId    = dpsTxExectr->getEDUID() ;
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;

      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE8( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_BYTE( lockMode ),
                 PD_PACK_BYTE( opMode ),
                 PD_PACK_UINT( bktIdx ),
                 PD_PACK_UINT( bktLatched ),
                 PD_PACK_ULONG( pdpsTxResInfo ),
                 PD_PACK_ULONG( eduId ) ) ;

      SDB_ASSERT( _initialized, "dpsTransLockManager is not initialized." ) ;
#endif
      if ( bktLatched )
      {
         bLatched = TRUE ;
      }

      // short cut for non-leaf lock ( CS, CL ),
      // lookup the executor _mapLockID map, if it is found and current
      // lock mode covers the requesting mode then increase refCounter,
      // and job is done. Otherwise, still need to go through the normal
      // routine.
      // we actually don't need bkt latch for looking up CS,CL lock
      // in the executor _mapLockID map
      //
      // findLock works for non-leaf lock only
      if ( _autoUpperLockOp &&
           ( dpsTxExectr->findLock( lockId, pLRB, _lockMgrType ) ) )
      {
         if ( pLRB )
         {
            pLRBOwner = pLRB ;

            if ( dpsLockCoverage( pLRB->lockMode, lockMode ) )
            {
               if ( !testMode )
               {
                  pLRB->refCounter++ ;

                  // clear the wait info in dpsTxExectr
                  dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
               }

               pLRBHdr = pLRB->lrbHdr ;

               goto done ;
            }
         }
      }

      // normal lock acquire/try get/test routine

      if ( bktIdx == DPS_LOCK_INVALID_BUCKET_SLOT )
      {
         bktIdx = _getBucketNo( lockId );
      }

      // latch bucket
      // for test mode, we could quickly test the header
      // if header is empty, it means no one has lock any records in this
      // bucket, so it should be safe to pass the test lock request
      // NOTE: this is based on the rule that before test locks we have
      // acquired the mblatch of collection
      if ( testMode && NULL == _LockHdrBkt[ bktIdx ].lrbHdr )
      {
         goto done ;
      }
      else if ( ! bktLatched )
      {
         _acquireOpLatch( bktIdx ) ;
      }
      bLatched = TRUE ;

      // if no LRB Header
      if ( NULL == _LockHdrBkt[ bktIdx ].lrbHdr )
      {
         if ( !testMode )
         {
            // allocate LRB Header and LRB
            rc = _prepareNewLRBAndHeader( dpsTxExectr, lockId, lockMode,
                                          bktIdx, pLRBHdrNew, pLRBNew ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            bFreeLRB       = TRUE ;
            bFreeLRBHeader = TRUE ;

            // add new LRB header to the link
            _LockHdrBkt[ bktIdx ].lrbHdr = pLRBHdrNew;

            // sample tick before adding to edulist or setting
            // waiter info to make sure snapshot trans is correct.
            pLRBNew->beginTick.sample() ;

            // add new LRB to EDU LRB list
            _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

            // mark the new LRB and LRB Header are used
            bFreeLRB       = FALSE ;
            bFreeLRBHeader = FALSE ;
            pLRBHdr        = pLRBHdrNew ;
            pLRB           = pLRBNew ;
         }
         // job done
         goto done;
      }

      // LRB header exists,
      // lookup the LRB header list and find the one with same lockId
      pLRBHdr = _LockHdrBkt[ bktIdx ].lrbHdr ;
      if ( ! _getLRBHdrByLockId( lockId, pLRBHdr ) )
      {
         // no LRB header with same lockId is found,
         // add the new LRB Header in the lrb header list
         if ( !testMode )
         {
            // allocate LRB Header and LRB
            rc = _prepareNewLRBAndHeader( dpsTxExectr, lockId, lockMode,
                                          bktIdx, pLRBHdrNew, pLRBNew ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            bFreeLRB       = TRUE ;
            bFreeLRBHeader = TRUE ;

            // at this time, pLRBHdr shall be the tail of LRB header list.
            // add the new LRB header to LRB Header list ;
            pLRBHdr->nextLRBHdr = pLRBHdrNew ;

            // sample tick before adding to edulist or setting
            // waiter info to make sure snapshot trans is correct.
            pLRBNew->beginTick.sample() ;

            // add the new LRB to EDU LRB list
            _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

            // mark the new LRB and new LRB Header are used
            bFreeLRB       = FALSE ;
            bFreeLRBHeader = FALSE ;
            pLRBHdr        = pLRBHdrNew ;
            pLRB           = pLRBNew ;
         }
         else
         {
            pLRBHdr = NULL ;
         }
         // job done
         goto done ;
      }
#ifdef _DEBUG
      SDB_ASSERT( ( NULL != pLRBHdr ), "Invalid LRB Header" ) ;
#endif
      // found the LRB header with same lockId

      // leaf level lock ( e.g., record lock or index page lock )
      if (  lockId.isLeafLevel() || ( FALSE == _autoUpperLockOp ) )
      {
         // search owner LRB list, which is sorted on lock mode
         // in descending order, to find
         //  . if the edu is in owner list
         //  . the pointer which the new LRB shall be inserted after
         //  . the pointer of first incompatible LRB
         //
         // pLRBToInsert     -- lrb to insert after
         // pLRBIncompatible -- lrb of first incompatible
         // pLRBOwner        -- lrb owned by same EDU
         _searchOwnerLRBList( dpsTxExectr,
                              lockMode,
                              pLRBHdr->ownerLRB,
                              pLRBToInsert,
                              pLRBIncompatible,
                              pLRBOwner ) ;
      }
      // non-leaf Lock
      else
      {
         // the owner LRB should already checked by findLock, result is saved
         // in pLRBOwner. now search owner LRB list to find
         //  . the position where the new LRB shall be inserted after
         //  . the pointer of first incompatible LRB
         //
         // pLRBToInsert     -- lrb to insert after
         // pLRBIncompatible -- lrb of first incompatible
         _searchOwnerLRBListForInsertAndIncompatible( dpsTxExectr,
                                                      lockMode,
                                                      pLRBHdr->ownerLRB,
                                                      pLRBToInsert,
                                                      pLRBIncompatible ) ;
      }

      if ( pLRBOwner )
      {
         //
         // in owner list
         //
         pLRB = pLRBOwner ;
#ifdef _DEBUG
         SDB_ASSERT( pLRB && ( pLRB->lrbHdr == pLRBHdr ),
                     "Invalid LRB or the lrbHdr doesn't match "
                     "the LRB Header" ) ;
#endif

         // if current holding lock mode covers the requesting mode,
         // then job is done
         if ( dpsLockCoverage( pLRB->lockMode, lockMode ) )
         {
            if ( !testMode )
            {
               pLRB->refCounter ++ ;

               _moveToEDULRBListTail( dpsTxExectr, pLRB ) ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;
            }

            goto done ;
         }

         // if dpsUpgradeCheck is OK
         rc = dpsUpgradeCheck( pLRB->lockMode, lockMode ) ;
         if ( SDB_OK != rc )
         {
            // can't do upgrade, job done with error rc set

            // constrct conflict lock info
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRB->lockMode ;
               pdpsTxResInfo->_eduID    = pLRB->dpsTxExectr->getEDUID();
               pdpsTxResInfo->_tid      = pLRB->dpsTxExectr->getTID() ;
            }
            goto done ;
         }

         // try to do upgrade
         //
         // check if the requested mode is compatible with other owners
         if ( NULL != pLRBIncompatible )
         {
            // valid pLRBIncompatible means an incompatible LRB is found,
            // i.e., not compatible with others

            if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
            {
               // allocate LRB
               rc = _prepareNewLRB( dpsTxExectr, lockMode,
                                    pLRBHdr, pLRBNew ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               bFreeLRB       = TRUE ;

               // save current owining lock mode
               pLRBNew->originMode = pLRBOwner->lockMode ;

               // do dead-lock detection before put in upgrade list.
               // For example, when two S owners both want to upgrade,
               // if allow both of them go in upgrade queue, there will
               // be a dead-lock, and eventually timed out.
               // scenarios:
               //  Owner          Upgrade
               //  1. S     --->  X
               //  2. S     --->  X <--- dead lock with owner 1
               //
               //  Owner          Uprade
               //  1. S     --->  U
               //  2. S     --->  U <--- compatible, no dead-lock
               //  3. U     --->  X <--- dead lock with owner 1 and 2
               if ( FALSE == _deadlockDetectedWhenUpgrade( pLRBHdr->upgradeLRB,
                                                           pLRBNew,
                                                           pLRBDeadlock ) )
               {
                  // add the new LRB to upgrade list
                  _addToLRBListTail( pLRBHdr->upgradeLRB, pLRBNew ) ;

                  // sample tick before adding to edulist or setting
                  // waiter info to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // set the wait info in dpsTxExectr
                  dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_UPGRADE,
                                              _lockMgrType ) ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;

                  // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
                  rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;
               }
               else
               {
                  // may cause dead-lock, can't do upgrade
                  pLRBIncompatible = pLRBDeadlock ;
                  rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
               }
            }
            else
            {
               // for try or test mode
               // . set return code to SDB_DPS_TRANS_LOCK_INCOMPATIBLE
               // . no need to add to upgrade/waiter list
               rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
            }

            // construct conflcit lock info ( representative )
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRBIncompatible->lockMode ;
               pdpsTxResInfo->_eduID    =
                  pLRBIncompatible->dpsTxExectr->getEDUID() ;
               pdpsTxResInfo->_tid      =
                  pLRBIncompatible->dpsTxExectr->getTID() ;
            }

            // job done
            goto done ;
         }
         else
         {
            // compatible with all others
            if ( !testMode )
            {
               // upgrade/convert to request mode.
               //   when upgrade, it implies the request mode is greater
               //   than current mode. Since the owner list is sorted on
               //   the lock mode in acsent order, we will need to move
               //   the owner LRB to the right place by following two steps :
               //     . remove from current place
               //     . move it to the place right after the pLRBToInsert

               // remove from current place
               _removeFromLRBList( pLRBHdr->ownerLRB, pLRB ) ;

               // update current lock mode to the request mode
               pLRB->lockMode = lockMode ;

               // insert it to the new position
               if ( pLRBToInsert )
               {
                  _addToOwnerLRBList( pLRBToInsert, pLRB ) ;
               }
               else
               {
                  // add it at the beginning of owner list
                  _addToLRBListHead( pLRBHdr->ownerLRB, pLRB ) ;
               }

               // set newestISOwner or newestIXOwner in lrbhdr
               _setNewestISIXOwner( pLRB ) ;

               _moveToEDULRBListTail( dpsTxExectr, pLRB ) ;

               // clear the wait info in dpsTxExectr
               dpsTxExectr->clearWaiterInfo( _lockMgrType ) ;

               pLRB->refCounter ++ ;
            }
            // job done
            goto done ;
         }
      }
      else
      {
         // not in owner list

         if ( !testMode )
         {
            // allocate LRB
            rc = _prepareNewLRB( dpsTxExectr, lockMode,
                                 pLRBHdr, pLRBNew ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            bFreeLRB       = TRUE ;
         }

         // check if lock is compatible with all owners
         if ( NULL != pLRBIncompatible )
         {
            // found an incompatible one, i.e., not compatible with others

            if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
            {
               // add it at the end of waiter list
               _addToLRBListTail( pLRBHdr->waiterLRB, pLRBNew ) ;

               // sample tick before adding to edulist or setting
               // waiter info to make sure snapshot trans is correct.
               pLRBNew->beginTick.sample() ;

               // set the wait info in dpsTxExectr
               dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_WAITER,
                                           _lockMgrType ) ;

               // mark the new LRB is used
               bFreeLRB = FALSE ;

               // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
               rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;
            }
            else
            {
               rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
            }

            // construct the conflict lock info
            if ( pdpsTxResInfo )
            {
               pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
               pdpsTxResInfo->_lockType = pLRBIncompatible->lockMode ;
               pdpsTxResInfo->_eduID    =
                  pLRBIncompatible->dpsTxExectr->getEDUID() ;
               pdpsTxResInfo->_tid      =
                  pLRBIncompatible->dpsTxExectr->getTID() ;
            }

            // job done
            goto done ;

         }
         else
         {
            //
            // no incompatible found, i.e., compatible with other owners

            // add to owner list when satisfy following conditions :
            // a. if both upgrade and waiter list are empty
            // b. if it is doing retry-acquire after been woken up.( when input
            //    parameter, bktLatched, is true, means retry acquiring )
            if (    ( ( ! pLRBHdr->upgradeLRB ) && ( ! pLRBHdr->waiterLRB ) )
                 || ( bktLatched ) )
            {
               if ( !testMode )
               {
                  // add the owner list
                  if ( pLRBToInsert )
                  {
                     _addToOwnerLRBList( pLRBToInsert, pLRBNew ) ;
                  }
                  else
                  {
                     _addToLRBListHead( pLRBHdr->ownerLRB, pLRBNew ) ;
                  }

                  // set newestISOwner or newestIXOwner in lrbhdr
                  _setNewestISIXOwner( pLRBNew ) ;

                  // sample tick before adding to edulist or setting
                  // waiter info to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // add the new LRB to EDU LRB list
                  _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;
                  pLRB     = pLRBNew ;
               }

               // job done
               goto done ;
            }
            else
            {
               // when test lock with preemptive mode, return succeess
               // if there is no conflict with current owners regardless
               // the upgrade or wait list.
               if ( DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT == opMode )
               {
                  goto done ;
               }

               // if the requested locked mode is compabile with all members
               // in both upgrade and waiter list, then add it into owner list
               if ( FALSE == _checkLockModeWithOthers( pLRBHdr->upgradeLRB,
                                                       dpsTxExectr,
                                                       lockMode,
                                                       pLRBIncompatible ) )
               {
                  if ( FALSE == _checkLockModeWithOthers( pLRBHdr->waiterLRB,
                                                          dpsTxExectr,
                                                          lockMode,
                                                          pLRBIncompatible ) )
                  {
                     // add to owner list
                     if ( DPS_TRANSLOCK_OP_MODE_TEST != opMode )
                     {
                        if ( pLRBToInsert )
                        {
                           _addToOwnerLRBList( pLRBToInsert, pLRBNew ) ;
                        }
                        else
                        {
                           _addToLRBListHead( pLRBHdr->ownerLRB, pLRBNew ) ;
                        }

                        // set newestISOwner or newestIXOwner in lrbhdr
                        _setNewestISIXOwner( pLRBNew ) ;

                        // sample tick before adding to edulist or setting
                        // waiter info to make sure snapshot trans is correct.
                        pLRBNew->beginTick.sample() ;

                        // add the new LRB to EDU LRB list
                        _addToEDULRBListTail( dpsTxExectr, pLRBNew, lockId ) ;

                        // mark the new LRB is used
                        bFreeLRB = FALSE ;
                        pLRB     = pLRBNew ;
                     }
                     // job done
                     goto done ;
                  }
               }

               // if the requested lock mode is not compabile with all
               // members in upgrade and waiter list, add it to waiter list
               if ( DPS_TRANSLOCK_OP_MODE_ACQUIRE == opMode )
               {
                  // add to the end of waiter list
                  _addToLRBListTail( pLRBHdr->waiterLRB, pLRBNew ) ;

                  // sample tick before adding to edulist or setting waiter info
                  // to make sure snapshot trans is correct.
                  pLRBNew->beginTick.sample() ;

                  // set the wait info in dpsTxExectr
                  dpsTxExectr->setWaiterInfo( pLRBNew, DPS_QUE_WAITER,
                                              _lockMgrType ) ;

                  // set return code to SDB_DPS_TRANS_APPEND_TO_WAIT
                  rc = SDB_DPS_TRANS_APPEND_TO_WAIT ;

                  // mark the new LRB is used
                  bFreeLRB = FALSE ;
               }
               else
               {
                  rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
               }

               // construct the conflict lock info ( representative )
               if ( pdpsTxResInfo )
               {
                  pLRB = pLRBIncompatible ;

                  pdpsTxResInfo->_lockID   = pLRBHdr->lockId ;
                  pdpsTxResInfo->_lockType = pLRB->lockMode ;
                  pdpsTxResInfo->_eduID    = pLRB->dpsTxExectr->getEDUID() ;
                  pdpsTxResInfo->_tid      = pLRB->dpsTxExectr->getTID() ;
               }

               // job done
               goto done ;

            }  // if both upgrade and waiter queue/list are empty
         }  // if request lock mode is compatible with other owners
      }  // if in owner list
   done:
      {
         if( callback )
         {
            // need to call this under bktlatch to make sure we are safe to
            // lookup information in LRBHdr
            callback->afterLockAcquire(
                         lockId, rc,
                         lockMode,
                         pLRB ? pLRB->refCounter : 0,
                         ( ( testMode )
                           ? DPS_TRANSLOCK_OP_MODE_TEST : opMode ),
                         pLRBHdr,
                         pLRBHdr ? &(pLRBHdr->extData) : NULL ) ;
         }
         // there is a scenario, using testX to clean up 'old version'
         // hanging off LRB header. Release LRB header if it is possible
         // when test opreation succeeded.
         if ( ( DPS_TRANSLOCK_OP_MODE_TEST == opMode ) &&
              ( DPS_TRANSLOCK_X == lockMode ) &&
              ( SDB_OK == rc ) )
         {
            if (    ( NULL != pLRBHdr )
                 && ( NULL == pLRBHdr->ownerLRB )
                 && ( NULL == pLRBHdr->upgradeLRB )
                 && ( NULL == pLRBHdr->waiterLRB )
                 && ( pLRBHdr->extData.canRelease() ) )
            {
               _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
               _releaseLRBHdr( pLRBHdr ) ;
            }
         }
      }

      if ( SDB_OK == rc && ownedLockMode )
      {
         // lock is now acquired, or owned, or tested
         *ownedLockMode = pLRB ? pLRB->lockMode : lockMode ;
      }

      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( bFreeLRB )
      {
         _releaseLRB( pLRBNew ) ;
         bFreeLRB = FALSE ;
      }
      else
      {
         // sample lock owning( first time ) or waiting timestamp ( ossTick )
         if ( !testMode && pLRBNew )
         {
            if ( !(BOOLEAN) (pLRBNew->beginTick) )
            {
               pLRBNew->beginTick.sample() ;
            }
         }
      }
      if ( bFreeLRBHeader )
      {
         _releaseLRBHdr( pLRBHdrNew ) ;
         bFreeLRBHeader = FALSE ;
      }

      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__TRYACQUIREORTEST, rc ) ;
      return rc;
   error:
      goto done ;
   }

   BOOLEAN dpsTransLockManager::_addRefIfOwned
   (
         _dpsTransExecutor *        dpsTxExectr,
         const dpsTransLockId &     lockID
   )
   {
      BOOLEAN foundLock = FALSE ;

      UINT32 bktIdx  = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRBHeader *pLRBHdr = NULL ;

      if ( !lockID.isValid() )
      {
         PD_LOG( PDERROR, "Invalid lockId:%s", lockID.toString().c_str() ) ;
         goto error ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockID ) ;

      // latch the LRB Header list
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockID, pLRBHdr ) )
      {
         SDB_ASSERT( pLRBHdr, "Invalid LRB Header" ) ;
         dpsTransLRB *pLRB = pLRBHdr->ownerLRB ;
         while ( NULL != pLRB )
         {
            if ( dpsTxExectr == pLRB->dpsTxExectr )
            {
               ++ pLRB->refCounter ;
               foundLock = TRUE ;
               break ;
            }
            pLRB = pLRB->nextLRB ;
         }
      }

      // free LRB Header list latch
      _releaseOpLatch( bktIdx ) ;

   done:
      return foundLock ;

   error:
      goto done ;
   }

   //
   // Description: acquire and setup a new LRB header and a new LRB
   // Function: acquire a new LRB Header and a new LRB, and initialize these
   //           two new object with given input parameters.
   //           the new LRB will be linked to the new LRB Header
   // Input:
   //    eduId           -- edu Id
   //    lockId          -- lock id
   //    requestLockMode -- requested lock mode
   // Output:
   //    pLRBHdrNew      -- pointer of the new LRB header object
   //    pLRBNew         -- pointer of the new LRB object
   // Return:  SDB_OK or any error returned from _utilSegmentManager::acquire
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER, "dpsTransLockManager::_prepareNewLRBAndHeader" )
   INT32 dpsTransLockManager::_prepareNewLRBAndHeader
   (
      _dpsTransExecutor *        dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      const UINT32               bktIdx,
      dpsTransLRBHeader *      & pLRBHdrNew,
      dpsTransLRB       *      & pLRBNew
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER ) ;

      INT32   rc             = SDB_OK ;

      // acquire a free LRB Header
      pLRBHdrNew = SDB_OSS_NEW dpsTransLRBHeader( lockId, bktIdx ) ;
      if ( !pLRBHdrNew )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc a LRBHeader (rc=%d)", rc ) ;
         goto error ;
      }

      /// acquire a lrb
      pLRBNew = SDB_OSS_NEW dpsTransLRB( dpsTxExectr,
                                         requestLockMode,
                                         pLRBHdrNew ) ;
      if ( !pLRBNew )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc a LRB (rc=%d)", rc ) ;
         goto error ;
      }

      pLRBHdrNew->ownerLRB   = pLRBNew;

      _setNewestISIXOwner( pLRBNew ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRBANDHEADER, rc ) ;
      return rc ;
   error :
      if( pLRBNew )
      {
         _releaseLRB( pLRBNew ) ;
      }
      if( pLRBHdrNew )
      {
         _releaseLRBHdr( pLRBHdrNew ) ;
      }
      goto done;
   }

   //
   // Description: acquire and setup a new LRB
   // Function: acquire a new LRB, and initialize the
   //           new object with given input parameters.
   // Input:
   //    _dpsTransExecutor -- trans executor
   //    requestLockMode   -- requested lock mode
   //    pLRBHdr           -- the LRB header object
   // Output:
   //    pLRBNew           -- pointer of the new LRB object
   // Return:  SDB_OK or any error returned from _utilSegmentManager::acquire
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRB, "dpsTransLockManager::_prepareNewLRB" )
   INT32 dpsTransLockManager::_prepareNewLRB
   (
      _dpsTransExecutor *        dpsTxExectr,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      const dpsTransLRBHeader *  pLRBHdr,
      dpsTransLRB       *      & pLRBNew
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRB ) ;

      INT32   rc             = SDB_OK ;

      if ( !pLRBHdr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// acquire a lrb
      pLRBNew = SDB_OSS_NEW dpsTransLRB( dpsTxExectr,
                                         requestLockMode,
                                         (dpsTransLRBHeader*)pLRBHdr ) ;
      if ( !pLRBNew )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc a LRB (rc=%d)", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_PREPARENEWLRB, rc ) ;
      return rc ;
   error :
      if( pLRBNew )
      {
         _releaseLRB( pLRBNew ) ;
      }
      goto done;
   }

   //
   // Description: acquire a lock with given mode
   // Function:    acquire a lock with requested mode
   //              . if the request is fulfilled, LRB is added to owner list
   //                and EDU LRB chain ( all locks in same TX ).
   //              . if the lock is record lock, intent lock on collection
   //                and collection space will be also acquired.
   //              . if the lock is collection lock, an intention lock on
   //                collection space will be also acquired.
   //              . if lock is not applicable at that time, the LRB will be
   //                added to waiter or upgrade list and wait.
   //              . while lock waiting it could be either woken up or
   //                lock waiting time out ( return an error )
   //              . when it is woken up from lock waiting,
   //                it will try to acquire the lock again
   // Input:
   //    dpsTxExectr     -- dpsTxExectr ( per EDU, similar to eduCB,
   //                                     isolate pmd from dps )
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    pContext        -- pointer to context :
   //                         dmsTBTransContext
   //                         dmsIXTransContext
   //    callback        -- pointer to trans lock callback
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo, a structure used to
   //                       save the conflict lock info
   // Return:
   //     SDB_OK,                                 -- lock is acquired
   //     SDB_DPS_TRANS_APPEND_TO_WAIT,           -- put on wait/upgrade list
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,   -- invalid upgrade request
   //     SDB_INTERRUPT,                          -- lock wait interrrupted
   //     SDB_TIMEOUT,                            -- lock wait timeout elapsed
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_ACQUIRE, "dpsTransLockManager::acquire" )
   INT32 dpsTransLockManager::acquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      IContext                 * pContext,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback,
      DPS_TRANSLOCK_TYPE       * ownedLockMode,
      BOOLEAN                    useEscalation
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_ACQUIRE ) ;
#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE5( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_BYTE( requestLockMode ),
                 PD_PACK_ULONG( pContext ),
                 PD_PACK_ULONG( pdpsTxResInfo ) ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif

      INT32 rc  = SDB_OK ,
            rc2 = SDB_OK ; // context pause or resume return code
      dpsTransLockId     iLockId ;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      BOOLEAN isIntentLockAcquired = FALSE ;

      UINT32  bktIdx   = DPS_LOCK_INVALID_BUCKET_SLOT ;
      BOOLEAN bLatched = FALSE ;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // for trans lock, get intent lock at first
      if ( _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId   = lockId.upOneLevel() ;
         BOOLEAN needEscalate = FALSE ;
         DPS_TRANSLOCK_TYPE iOwnedLockMode = DPS_TRANSLOCK_MAX ;

         // check if escalation is triggered
         if ( useEscalation && iLockId.isSupportEscalation() )
         {
            rc = dpsTxExectr->checkLockEscalation( _lockMgrType,
                                                   iLockId,
                                                   needEscalate ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check lock escalation, "
                         "rc: %d", rc ) ;
         }

         // get intent lock mode
         iLockMode = dpsIntentLockMode( requestLockMode, needEscalate ) ;
#ifdef _DEBUG
         ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                      "%s", iLockId.toString().c_str() ) ;
         PD_TRACE3( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING( "Acquiring intent lock:" ),
                    PD_PACK_STRING( lockIdStr ),
                    PD_PACK_BYTE( iLockMode )  ) ;
#endif
         // acquire upper lock
         rc = acquire( dpsTxExectr, iLockId, iLockMode,
                       pContext, pdpsTxResInfo, NULL, &iOwnedLockMode,
                       useEscalation ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         isIntentLockAcquired = TRUE ;

         // if lock escalated or lock owned in upper level can cover lock
         // requesting in lower level, no need to request lock in lower level
         if ( ( needEscalate ) ||
              ( dpsIsCoverLowerLock( iOwnedLockMode, requestLockMode ) ) )
         {
            // invoke callback if needed
            if ( NULL != callback )
            {
               rc = callback->afterLockEscalated( lockId,
                                                  DPS_TRANSLOCK_OP_MODE_ACQUIRE ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to invoke lock callback, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
            _addRefIfOwned( dpsTxExectr, lockId ) ;
            // if requesting lock in current level is covered by upper lock,
            // it means we got the same type of lock in current level
            // e.g. IX is requesting in current level, and X is owned in
            // upper level, it means we got X in current level as well
            if ( ownedLockMode )
            {
               *ownedLockMode = iOwnedLockMode ;
            }
            // no need to process lower level locks
            goto done ;
         }
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

   acquireRetry:
      // check if EDU is intrrupted first
      if ( dpsTxExectr->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      //
      // acquire the lock
      //
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              DPS_TRANSLOCK_OP_MODE_ACQUIRE,
                              bktIdx,
                              bLatched,
                              pdpsTxResInfo,
                              callback,
                              ownedLockMode ) ;
      // _tryAcquireOrTest acquires bucket latch by default unless the input
      // parameter, bLatched, is set to TRUE; and it always releases the latch
      // before returns
      bLatched = FALSE ;

      if ( SDB_OK == rc )
      {
         // lock acquired sucessfully, job done
         goto done ;
      }
      else if ( SDB_DPS_TRANS_APPEND_TO_WAIT == rc )
      {
         // checks if need to process lock waiting logic
         goto LockWaiting ;
      }
      else
      {
         // lock request is neither fulfilled nor put on wait
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_ACQUIRE, rc ) ;
      return rc ;

   LockWaiting:
      //
      // Processing lock waiting.
      //   at this time the lock request is put on upgrade or wait queue.
      //

      // init rc2. it is used to mark whether the context has
      // been successfully paused / resumed
      rc2 = SDB_OK ;

      // pause the context before waiting the lock
      if ( pContext )
      {
         rc2 = pContext->pause() ;
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("context pause rc:"),
                    PD_PACK_INT( rc2 ) )  ;
#endif
      }
      if ( SDB_OK != rc2 )
      {
         goto postLockWaiting ;
      }

      // wait for the lock
      rc = _waitLock( dpsTxExectr ) ;
      if ( SDB_OK != rc )
      {
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("waitLock rc:"),
                    PD_PACK_INT( rc ) )  ;
#endif
         CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE + 1 ] = { '\0' } ;
         dpsTxExectr->getExecutor()->resetInfo( EDU_INFO_ERROR ) ;
         dpsTxExectr->getExecutor()->printInfo( EDU_INFO_ERROR,
                     "Acquire transaction lock(%s)(%s) failed",
                     lockId.toString( lockIdStr, DPS_LOCKID_STRING_MAX_SIZE ),
                     lockModeToString( requestLockMode ) ) ;
         goto postLockWaiting ;
      }

      // resume context
      if ( pContext )
      {
         rc2 = pContext->resume() ;
#ifdef _DEBUG
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING("context resume rc:"),
                    PD_PACK_INT( rc2 ) )  ;
#endif
      }

   postLockWaiting:
      // remove the LRB from upgrade or waiter list and remove the empty
      // LRB Header if it is needed
      if ( ! bLatched )
      {
         // need latch bucket before remove it from upgrade or waiter list
         _acquireOpLatch( bktIdx ) ;
         bLatched = TRUE ;
      }

      // remove the LRB from upgrade or waiter list and wakeup next waiter
      // if necessary. The empty LRB Header will be removed only when it is
      // needed, i.e., when _waitLock() fails( either timeout duration elapsed
      // or be interrupted )
      // The reason removing the empty LRB Header only when _waitLock() fails
      // is if _waitLock returns success, when retry acquiring the lock,
      // _tryAcquireOrTest(), the LRB Header will be added back again.
      _removeFromUpgradeOrWaitList( dpsTxExectr,
                                    lockId, bktIdx, ( SDB_OK != rc ) ) ;

      // retry acquire the lock when following conditions are satisfied:
      //   . SDB_OK == rc = _waitLock(), it has been woken up
      //   . SDB_OK == rc2, context resumed successfully
      //   . bLatched, holding the bucket latch, to avoid race condition
      //     between itself and the one is woken up
      if ( ( SDB_OK == rc ) && ( SDB_OK == rc2 ) && bLatched )
      {
         goto acquireRetry ;
      }

      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }

      // set recode to rc2 when _waitLock() succeeded but resume()
      // or pause() failed
      if ( ( SDB_OK != rc2 ) && ( SDB_OK == rc ) )
      {
         rc = rc2 ;
      }

   error:
      // when _waitLock() fails ( timeout or be interrupted ), or pause/resume
      // context fails, we will need to release upper level intent lock
      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( isIntentLockAcquired && _autoUpperLockOp )
      {
         release( dpsTxExectr, iLockId, FALSE ) ;
         isIntentLockAcquired = FALSE ;
#ifdef _DEBUG
         ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                      "%s", iLockId.toString().c_str() ) ;
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_ACQUIRE,
                    PD_PACK_STRING( "Error code path, release intent lock:" ),
                    PD_PACK_STRING( lockIdStr ) ) ;
#endif
      }
      goto done ;
   }


   //
   // Description: search EDU LRB chain and find the LRB has same lock ID
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and search for the LRB with the LRB Header it associated to
   //              containing the same lockId.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    lockId       -- the lock ID
   // Output:
   //    None
   // Return:      the LRB pointer when it is found
   //              NULL when it is not found
   // Dependency:  None
   //
   dpsTransLRB * dpsTransLockManager::_getLRBFromEDULRBList
   (
      _dpsTransExecutor    * dpsTxExectr,
      const dpsTransLockId & lockId
   )
   {
      dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      while ( pLRB )
      {
         if ( ( NULL != pLRB->lrbHdr ) && ( lockId == pLRB->lrbHdr->lockId ) )
         {
            break ;
         }
         pLRB = pLRB->eduLrbPrev ;
      }
      return pLRB ;
   }

   //
   // Description: search EDU LRB chain and find the LRB has same lock ID
   // Function:    walk through the EDU LRB chain( doubly linked list ),
   //              and search for the LRB with the LRB Header it associated to
   //              containing the same lockId.
   // Input:
   //    dpsTxExectr  -- dpsTxExectr
   //    lockId       -- the lock ID
   // Output:
   //    owningLockMode -- the lock mode it is owning when found
   //    refCount       -- the lock reference counter when found
   // Return:      TRUE  the caller thread is owning this lock
   //              FALSE the caller thread is not ownning this lock
   // Dependency:  None
   //
   BOOLEAN dpsTransLockManager::isHolding
   (
      _dpsTransExecutor    * dpsTxExectr,
      const dpsTransLockId & lockId,
      INT8                 & owningLockMode,
      UINT32               & refCount
   )
   {
      BOOLEAN found = FALSE ;

      if ( lockId.isLeafLevel() )
      {
         dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;

         while ( pLRB )
         {
            if ( ( NULL != pLRB->lrbHdr ) && ( lockId == pLRB->lrbHdr->lockId ) )
            {
               refCount       = pLRB->refCounter ;
               owningLockMode = pLRB->lockMode ;
               found          = TRUE ;
               break ;
            }
            pLRB = pLRB->eduLrbPrev ;
         }
      }
      else
      {
         dpsTransLRB *pLRB = NULL ;
         if ( dpsTxExectr->findLock( lockId, pLRB, _lockMgrType, FALSE ) )
         {
            SDB_ASSERT( NULL != pLRB, "LRB is invalid" ) ;
            refCount = pLRB->refCounter ;
            owningLockMode = pLRB->lockMode ;
            found = TRUE ;
         }
      }
      return found ;
   }

   //
   // Description: core logic of release a lock
   // Function: decrease lock reference counter, do following when the counter
   //           comes zero :
   //           .  remove from the edu ( caller ) LRB chain
   //           .  remove it from owner list
   //           .  wake up a waiter( upgrade or waiter list ) when necessary,
   //           .  remove the LRB Header if it is empty ( owner, waiter,
   //              upgrade list are all empty )
   // Input:
   //    dpsTxExectr         -- dpsTransExecutor
   //    lockId              -- lock id
   //    bForceRelease       -- force release flag
   //    refCountToDecrease  -- value to be decreased from lock refCounter
   //                           when release CL,CS lock in 'force' mode ;
   //                           if this value is zero, then set lock refCounter
   //                           to zero when release CL,CS lock in 'force'
   //
   //    callback            -- pointer to translock callback
   // Output :
   //    bForceRelease       -- value of lock refCounter if release
   //                           a record lock with 'force' mode
   //
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__RELEASE, "dpsTransLockManager::_release" )
   void dpsTransLockManager::_release
   (
      _dpsTransExecutor       * dpsTxExectr,
      const dpsTransLockId    & lockId,
      dpsTransLRB             * pOwnerLRB,
      const BOOLEAN             bForceRelease,
      UINT32                  & refCountToDecrease,
      _dpsITransLockCallback  * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__RELEASE ) ;

#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE4( SDB_DPSTRANSLOCKMANAGER__RELEASE,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_ULONG( callback ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_UINT( bForceRelease ) ) ;
#endif

      UINT32       bktIdx        = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRB *pMyLRB        = pOwnerLRB,
                  *lrbToRelease  = NULL ,
                  *pWaiterLRB    = NULL ;
      dpsTransLRB *pLRBIncompatible = NULL ;
      dpsTransLRBHeader *pLRBHdr = NULL,
                        *pLRBHdrToRelease = NULL;
      BOOLEAN bLatched = FALSE ;
      BOOLEAN foundIncomp = FALSE ;

      // short cut that we may do without bkt latch.
      // If we have the LRB, not doing "force release" and refCounter > 1,
      // we may simply decrease the refCounter. Otherwise, we shall go through
      // the normal routine.
      if ( ( ! bForceRelease ) && pMyLRB && ( pMyLRB->refCounter > 1 ) )
      {
         pMyLRB->refCounter-- ;
         goto done ;
      }

      // some quick ways to get LRB
      if ( NULL == pMyLRB )
      {
         if ( _autoUpperLockOp )
         {
            // for non-leaf lock ( CS, CL ), lookup the executor _mapLockID map
            if ( ! lockId.isLeafLevel() )
            {
               dpsTxExectr->findLock( lockId, pMyLRB, _lockMgrType ) ;
            }
            else
            {
               // for record lock, find the LRB from the EDU LRB list
               // in a single lock release scenario, the lock ID's LRB should
               // be at the end of the EDU LRB list. This should be guaranteed
               // through _moveToEDULRBListTail and _addToEDULRBListTail
               pMyLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
            }
         }
         else
         {
            pMyLRB = _getLRBFromEDULRBList( dpsTxExectr, lockId ) ;
         }

         if ( ( ! bForceRelease ) && pMyLRB && ( pMyLRB->refCounter > 1) )
         {
            pMyLRB->refCounter-- ;
            goto done ;
         }
      }

      // We must have a LRB by now
      if ( NULL == pMyLRB )
      {
#if defined ( _DEBUG )
         // not found in current level, should be holding a non-intent lock in
         // upper levels
         if ( !lockId.isRootLevel() )
         {
            // check upper levels one by one
            dpsTransLockId iLockId = lockId.upOneLevel() ;
            while ( iLockId.isValid() )
            {
               INT8 holdingILockMode = DPS_TRANSLOCK_MAX ;
               UINT32 refCount = 0 ;

               if ( isHolding( dpsTxExectr,
                               iLockId,
                               holdingILockMode,
                               refCount ) &&
                    DPS_TRANSLOCK_IS != holdingILockMode &&
                    DPS_TRANSLOCK_IX != holdingILockMode )
               {
                  // if holding a non-intent lock, it is OK
                  break ;
               }

               // lock is not holding in this level, check upper
               if ( !iLockId.isRootLevel() )
               {
                  iLockId = iLockId.upOneLevel() ;
                  continue ;
               }

               SDB_ASSERT( FALSE, "should hold non-intent lock in upper "
                           "levels" ) ;
            }
         }
         else
         {
            SDB_ASSERT( FALSE, "should hold root level lock" ) ;
         }
#endif
         goto done ;
      }
      // normal lock release routine

      bktIdx = pMyLRB->lrbHdr->bktIdx ;

      // latch the bucket
      _acquireOpLatch( bktIdx ) ;
      bLatched = TRUE ;

      pLRBHdr = pMyLRB->lrbHdr ;

      SDB_ASSERT( ( NULL != pLRBHdr ),
                  "Trying to release a non-exist lock" ) ;

      if ( bForceRelease )
      {
         if ( _autoUpperLockOp )
         {
            if ( lockId.isLeafLevel() )
            {
               // save the record lock refCounter to output parameter
               // so can we decrease that value for CL,CS lock later
               refCountToDecrease = pMyLRB->refCounter ;
               pMyLRB->refCounter = 0 ;
            }
            else
            {
               if ( 0 == refCountToDecrease )
               {
                  // when release CL, CL lock with 'refoce' mode,
                  // if the refCounter passed in is zero,
                  // then set lock refCounter to zero
                  pMyLRB->refCounter = 0 ;
               }
               else
               {
                  // when release CL, CL lock with 'refoce' mode,
                  // if the refCounter passed in is non-zero value,
                  // then substract that value from current lock refCounter
                  pMyLRB->refCounter -= refCountToDecrease ;
               }
            }
         }
         else
         {
            pMyLRB->refCounter = 0 ;
         }
      }
      else
      {
         SDB_ASSERT( pMyLRB->refCounter > 0, "refCounter is negative");

         pMyLRB->refCounter -- ;
      }

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG, "releasing lock before callback,"
              " lockid=%s,bForceRelease=%d,callback=%x,refcounter=%d",
              lockIdStr, bForceRelease, callback, pMyLRB->refCounter);
#endif

      {
         // invoke call back function before release
         if( callback )
         {
            callback->beforeLockRelease( lockId,
                                         pMyLRB->lockMode,
                                         pMyLRB->refCounter,
                                         pLRBHdr,
                                         pLRBHdr ? &(pLRBHdr->extData) :
                                                   NULL ) ;
         }
         else if ( pLRBHdr && pLRBHdr->extData._onLockReleaseFunc )
         {
            pLRBHdr->extData._onLockReleaseFunc( lockId,
                                                 pMyLRB->lockMode,
                                                 pMyLRB->refCounter,
                                                 &(pLRBHdr->extData),
                                                 -1,
                                                 FALSE ) ;
         }
      }

      if ( 0 == pMyLRB->refCounter )
      {
         // remove it from EDU LRB list
         _removeFromEDULRBList( dpsTxExectr, pMyLRB, lockId ) ;

         // remove it from lock owner list
         _removeFromLRBList( pLRBHdr->ownerLRB, pMyLRB ) ;

         // get the waiter LRB pointer
         if ( pLRBHdr->upgradeLRB )
         {
            pWaiterLRB = pLRBHdr->upgradeLRB ;
         }
         else if ( pLRBHdr->waiterLRB  )
         {
            pWaiterLRB = pLRBHdr->waiterLRB  ;
         }
         if ( pWaiterLRB &&
              ( ! OSS_BIT_TEST( pWaiterLRB->status, DPS_LRB_STATUS_AWAKE ) ) )
         {
            // lookup owner list check if the waiter lockMode is compabile
            // with other owners
            foundIncomp = _checkLockModeWithOthers( pLRBHdr->ownerLRB,
                                                    pWaiterLRB->dpsTxExectr,
                                                    pWaiterLRB->lockMode,
                                                    pLRBIncompatible ) ;

            // if the owner queue is empty ( after remove current owner ),
            // or if the waiter lockMode is compabile with other owners
            // wake it up
            if ( FALSE == foundIncomp )
            {
               // wake up the edu by posting an event
               OSS_BIT_SET( pWaiterLRB->status, DPS_LRB_STATUS_AWAKE ) ;
               _wakeUp( pWaiterLRB->dpsTxExectr ) ;
            }
         }

         // save the pointer of owner LRB to be released
         lrbToRelease = pMyLRB;

         // remove the LRB Header if owner, waiter,
         // and upgrade list are all empty
         if (    ( NULL == pLRBHdr->ownerLRB )
              && ( NULL == pLRBHdr->upgradeLRB )
              && ( NULL == pLRBHdr->waiterLRB )
              && ( pLRBHdr->extData.canRelease() ) )
         {
            _removeFromLRBHeaderList( _LockHdrBkt[bktIdx].lrbHdr, pLRBHdr ) ;
            pLRBHdrToRelease = pLRBHdr;
         }
      }  // end of if pMyLRB->refCounter is zero
   done:
      // release the bucket latch
      if ( bLatched )
      {
         _releaseOpLatch( bktIdx ) ;
         bLatched = FALSE ;
      }
      if ( lrbToRelease )
      {
         _releaseLRB( lrbToRelease ) ;
      }
      if ( pLRBHdrToRelease )
      {
         _releaseLRBHdr( pLRBHdrToRelease ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__RELEASE ) ;
      return ;
   }


   //
   // Description: release a lock
   // Function: release a lock, remove from the edu ( caller ) LRB chain
   //           and remove it from owner list if the refference counter is zero.
   //           Wake up a waiter when necessary, it will also release the upper
   //           level intent lock
   // Input:
   //    dpsTxExectr     -- pointer to dpsTransExecutor
   //    lockId          -- lock id
   //    bForceRelease   -- requested lock mode
   //    callback        -- transaction lock callback
   //    releaseUpperLock -- whether to release upper locks
   // Output:
   //    none
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_RELEASE, "dpsTransLockManager::release" )
   void dpsTransLockManager::release
   (
      _dpsTransExecutor      * dpsTxExectr,
      const dpsTransLockId   & lockId,
      const BOOLEAN            bForceRelease,
      _dpsITransLockCallback * callback,
      BOOLEAN                  releaseUpperLock
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_RELEASE ) ;
#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE4( SDB_DPSTRANSLOCKMANAGER_RELEASE,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_ULONG( callback ),
                 PD_PACK_UINT( bForceRelease ) ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      if ( lockId.isValid() )
      {
         dpsTransLockId myLockId = lockId ;
         BOOLEAN done = FALSE ;
         UINT32  refCountToBeDecreased = 0 ;
         do
         {
#ifdef _DEBUG
            ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                         "%s", myLockId.toString().c_str() ) ;
            PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_RELEASE,
                       PD_PACK_STRING( "releasing lockId:"),
                       PD_PACK_STRING( lockIdStr ) ) ;
#endif
            // main logic of release by lockId
            _release( dpsTxExectr, myLockId,
                      NULL, bForceRelease, refCountToBeDecreased,
                      callback ) ;

            if ( _autoUpperLockOp && releaseUpperLock )
            {
               // release the intent lock
               if ( ! myLockId.isRootLevel() )
               {
                  myLockId = myLockId.upOneLevel() ;
               }
               else
               {
                  done = TRUE ;
                  break ;
               }
            }
            else
            {
               done = TRUE ;
               break ;
            }
         } while ( ! done ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
      }

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER_RELEASE ) ;
      return ;
   }

   //
   // Description: Force release all locks an EDU is holding
   // Function: walk though the EDU LRB chain, force release all locks
   //           an EDU is holding.
   //           . remove them from the edu ( caller ) LRB chain
   //           . remove it from owner list
   //           . wake up a waiter when necessary,
   //           . release the upper level intent lock
   // Input:
   //    dpsTxExectr     -- pointer to _dpsTransExecutor
   //    callback        -- pointer to call back function
   // Output:
   //    none
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_RELEASEALL, "dpsTransLockManager::releaseAll" )
   void dpsTransLockManager::releaseAll
   (
      _dpsTransExecutor      * dpsTxExectr,
      _dpsITransLockCallback * callback
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_RELEASEALL ) ;
#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      dpsTransLRB       * pLRB     = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      dpsTransLRB       * pNextOne = NULL ;
      dpsTransLRBHeader * pLRBHdr  = NULL ;
      dpsTransLockId      lockId ;
      UINT32              refCount = 0 ;

      if ( NULL != pLRB )
      {
         if ( _autoUpperLockOp )
         {
            // walk through EDU LRB list,
            //   first loop, release all record locks with force mode.
            //   second loop, release all CL locks with force mode.
            //   last loopi, release all CS locks
            for ( UINT32 loop = 0 ; loop < 3; loop++ )
            {
               pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
               while ( pLRB )
               {
                  // save the next LRB address, pLRB->eduLrbPrev,
                  // before release
                  pNextOne = pLRB->eduLrbPrev ;

                  // peek LRB Header
                  pLRBHdr = pLRB->lrbHdr ;
#ifdef _DEBUG
                  SDB_ASSERT( pLRBHdr, "LRB Header can't be null" ) ;
#endif
                  {
                     lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
                     SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
                     // first loop, release all record locks
                     if ( ( 0 == loop ) && ( ! lockId.isLeafLevel() ) )
                     {
                        goto nextLock ;
                     }
                     // second loop, release all CL locks
                     if ( ( 1 == loop ) && lockId.isRootLevel() )
                     {
                        goto nextLock ;
                     }
#ifdef _DEBUG
                     ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                                  "%s", lockId.toString().c_str() ) ;
                     PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_RELEASEALL,
                                PD_PACK_STRING( "Releasing lock:" ),
                                PD_PACK_STRING( lockIdStr ) ) ;
#endif
                     refCount = 0 ;
                     _release( dpsTxExectr, lockId, pLRB, TRUE, refCount,
                               callback ) ;
                  }
nextLock:
                  // move to next lock attempt to be released
                  pLRB = pNextOne ;
               } // end while
            }
         }
         // the _autoUpperLockOp flag is off
         else
         {
            // walk through EDU LRB list
            while ( pLRB )
            {
               // save the next LRB address, pLRB->eduLrbPrev,
               // before release, because pLRB may be freed by _release
               pNextOne = pLRB->eduLrbPrev ;

               pLRBHdr = pLRB->lrbHdr ;
#ifdef _DEBUG
               SDB_ASSERT( pLRBHdr, "LRB Header can't be null" ) ;
#endif
               {
                  lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
                  SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
                  refCount = 0 ;
                  _release( dpsTxExectr, lockId, pLRB, TRUE, refCount,
                            callback ) ;
               }
               // move to next LRB
               pLRB = pNextOne;
            }
         }
      }

      dpsTxExectr->resetLockEscalated( _lockMgrType ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER_RELEASEALL ) ;
      return ;
   }


   UINT32 dpsTransLockManager::countAllLocks
   (
      _dpsTransExecutor * dpsTxExectr,
      BOOLEAN             bPrintLog,
      CHAR              * memoStr
   )
   {
      dpsTransLRB * pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
      dpsTransLRBHeader * pLRBHdr ;
      dpsTransLockId      lockId ;
      UINT32              lockCount = 0 ;

      std::stringstream lockInfo ;

      // walk through EDU LRB list,
      while ( pLRB )
      {
         pLRBHdr = pLRB->lrbHdr ;
         // get LRB Header
         if ( NULL != pLRBHdr )
         {
            lockId  = pLRBHdr->lockId ;
#ifdef _DEBUG
            SDB_ASSERT( lockId.isValid(), "Invalid lockId" ) ;
#endif
            lockCount++ ;
            if ( bPrintLog )
            {
               lockInfo << "LockId[" << lockId.toString() << "] "
                        << "lockMode:" << lockModeToString( pLRB->lockMode )
                        << std::endl ;
            }
         }
         // move to next LRB
         pLRB = pLRB->eduLrbPrev ;
      }
      if ( bPrintLog )
      {
         PD_LOG( PDDEBUG,
                 "%s" OSS_NEWLINE
                 "Number of locks:%d" OSS_NEWLINE "%s",
                 ( memoStr ? memoStr : "" ),
                 lockCount,
                 lockInfo.str().c_str() ) ;
      }
      return lockCount ;
   }


   //
   // Description: Wakeup a lock waiting EDU
   // Function: wake up an EDU by post an event
   //
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   // Output:
   //    none
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__WAKEUP, "dpsTransLockManager::_wakeUp" )
   void dpsTransLockManager::_wakeUp( _dpsTransExecutor *dpsTxExectr )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__WAKEUP ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTransExecutor can't be NULL" ) ;

      dpsTxExectr->wakeup( SDB_OK ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__WAKEUP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__KILLWAITER, "dpsTransLockManager::_killWaiter" )
   void dpsTransLockManager::_killWaiter( _dpsTransExecutor *dpsTxExectr,
                                          INT32 errorCode )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__KILLWAITER ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTransExecutor can't be NULL" ) ;

      dpsTxExectr->wakeup( errorCode ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER__KILLWAITER ) ;
   }

   //
   // Description: Wait a lock
   // Function: wait a lock by waiting on an event
   //
   // Input:
   //    dpsTxExectr -- pointer to _dpsTransExecutor
   // Output:
   //    none
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__WAITLOCK, "dpsTransLockManager::_waitLock" )
   INT32 dpsTransLockManager::_waitLock ( _dpsTransExecutor *dpsTxExectr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__WAITLOCK ) ;
#ifdef _DEBUG
      SDB_ASSERT( dpsTxExectr, "dpsTransExecutor can't be NULL" ) ;
#endif
      rc = dpsTxExectr->wait( dpsTxExectr->getTransTimeout() ) ;

      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__WAITLOCK, rc ) ;
      return rc ;
   }


   //
   // Description: try to acquire a lock with given mode
   // Function:    try to acquire a lock with given mode
   //              . if the request is fulfilled, LRB is added to owner list
   //                and EDU LRB chain ( all locks in same TX ).
   //              . if the lock is record lock, intent lock on collection
   //                and collection space will be also acquired.
   //              . if the lock is collection lock, an intention lock on
   //                collection space will be also acquired.
   //              . if lock is not applicable at that time,
   //                the LRB will NOT be put into waiter / upgrade list,
   //                SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be returned.
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE, "dpsTransLockManager::tryAcquire" )
   INT32 dpsTransLockManager::tryAcquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback,
      DPS_TRANSLOCK_TYPE       * ownedLockMode,
      BOOLEAN                    useEscalation
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE ) ;
#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE4( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_BYTE( requestLockMode ),
                 PD_PACK_ULONG( pdpsTxResInfo ) ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif

      INT32 rc = SDB_OK ;
      dpsTransLockId iLockId;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      BOOLEAN isIntentLockAcquired = FALSE;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // get intent lock at first
      // it is not need to get intent lock while lock space
      if ( _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         iLockId = lockId.upOneLevel() ;
         BOOLEAN needEscalate = FALSE ;
         DPS_TRANSLOCK_TYPE iOwnedLockMode = DPS_TRANSLOCK_MAX ;

         // check if escalation is triggered
         if ( useEscalation && iLockId.isSupportEscalation() )
         {
            rc = dpsTxExectr->checkLockEscalation( _lockMgrType,
                                                   iLockId,
                                                   needEscalate ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check lock escalation, "
                         "rc: %d", rc ) ;
         }
         // get intent lock mode
         iLockMode = dpsIntentLockMode( requestLockMode, needEscalate ) ;
#ifdef _DEBUG
         ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                      "%s", iLockId.toString().c_str() ) ;
         PD_TRACE3( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE,
                    PD_PACK_STRING( "Trying intent lock:" ),
                    PD_PACK_STRING( lockIdStr ),
                    PD_PACK_BYTE( iLockMode )  ) ;
#endif
         // acquire upper lock
         rc = tryAcquire( dpsTxExectr, iLockId, iLockMode, pdpsTxResInfo,
                          NULL, &iOwnedLockMode, useEscalation ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         isIntentLockAcquired = TRUE;

         // lock escalation has been triggered
         if ( ( needEscalate ) ||
              ( dpsIsCoverLowerLock( iOwnedLockMode, requestLockMode ) ) )
         {
            // invoke callback if needed
            if ( NULL != callback )
            {
               rc = callback->afterLockEscalated( lockId,
                                                  DPS_TRANSLOCK_OP_MODE_TRY ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to invoke lock callback, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
            _addRefIfOwned( dpsTxExectr, lockId ) ;
            // if requesting lock in current level is covered by upper lock,
            // it means we got the same type of lock in current level
            // e.g. IX is requesting in current level, and X is owned in
            // upper level, it means we got X in current level as well
            if ( ownedLockMode )
            {
               *ownedLockMode = iOwnedLockMode ;
            }
            // no need to process lower level locks
            goto done ;
         }
      }

      // check if EDU is intrrupted first
      if ( dpsTxExectr->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      // try to acquire the lock
      // when tryAcquire will not add LRB to either upgrade or waiter list
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              DPS_TRANSLOCK_OP_MODE_TRY,
                              DPS_LOCK_INVALID_BUCKET_SLOT,
                              FALSE,
                              pdpsTxResInfo,
                              callback,
                              ownedLockMode ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE, rc ) ;
      return rc;

   error:
      if ( ( isIntentLockAcquired ) && _autoUpperLockOp )
      {
         release( dpsTxExectr, iLockId, FALSE ) ;
         isIntentLockAcquired = FALSE ;
#ifdef _DEBUG
         ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                      "%s", iLockId.toString().c_str() ) ;
         PD_TRACE2( SDB_DPSTRANSLOCKMANAGER_TRYACQUIRE,
                    PD_PACK_STRING( "Release intent lock:" ),
                    PD_PACK_STRING( lockIdStr ) ) ;
#endif
      }
      goto done;
   }


   //
   // Description: test whether a lock can be acquired with given mode
   // Function:    test whether a lock can be acquired with given mode
   //              . if the request can be fulfilled, returns SDB_OK,
   //                the request will not be added to either owner list
   //                or EDU LRB chain, as it doesn't really acquire the lock.
   //              . if lock is not applicable at that time,
   //                SDB_DPS_TRANS_LOCK_INCOMPATIBLE will be returned.
   // Input:
   //    dpsTxExectr     -- dpsTxExectr
   //    lockId          -- lock Id
   //    requestLockMode -- lock mode being requested
   //    isPreemptMode   -- if do test with preemptive mode
   //    needUpperLock   -- whether to acquire intent lock in upper level
   //                       WARNING: no need to acquire intent lock only when
   //                       we have acquired earlier
   // Output:
   //    pdpsTxResInfo   -- pointer to dpsTransRetInfo
   // Return:
   //     SDB_OK,
   //     SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST,
   //     SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
   //     or other errors
   // Dependency:  the lock manager must be initialized
   //

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE, "dpsTransLockManager::testAcquire" )
   INT32 dpsTransLockManager::testAcquire
   (
      _dpsTransExecutor        * dpsTxExectr,
      const dpsTransLockId     & lockId,
      const DPS_TRANSLOCK_TYPE   requestLockMode,
      const BOOLEAN              isPreemptMode,
      dpsTransRetInfo          * pdpsTxResInfo,
      _dpsITransLockCallback   * callback,
      BOOLEAN                    needUpperLock,
      DPS_TRANSLOCK_TYPE       * ownedLockMode
   )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE ) ;
#ifdef _DEBUG
      CHAR lockIdStr[ DPS_LOCKID_STRING_MAX_SIZE ] = { '\0' } ;
      ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                   "%s", lockId.toString().c_str() ) ;
      PD_TRACE4( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE,
                 PD_PACK_ULONG( dpsTxExectr ),
                 PD_PACK_STRING( lockIdStr ),
                 PD_PACK_BYTE( requestLockMode ),
                 PD_PACK_ULONG( pdpsTxResInfo ) ) ;

      SDB_ASSERT( dpsTxExectr, "dpsTxExectr can't be null" ) ;
#endif
      INT32 rc = SDB_OK;
      dpsTransLockId iLockId;
      DPS_TRANSLOCK_TYPE iLockMode = DPS_TRANSLOCK_MAX ;
      UINT32 bktIdx = DPS_LOCK_INVALID_BUCKET_SLOT ;
      const DPS_TRANSLOCK_OP_MODE_TYPE testOpMode =
               ( isPreemptMode ? DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT
                               : DPS_TRANSLOCK_OP_MODE_TEST ) ;

      if ( ! lockId.isValid() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // get intent lock at first
      // it is not need to get intent lock while lock space
      if ( needUpperLock && _autoUpperLockOp && ( ! lockId.isRootLevel()) )
      {
         DPS_TRANSLOCK_TYPE iOwnedLockMode = DPS_TRANSLOCK_MAX ;
         iLockId = lockId.upOneLevel() ;
         // get intent lock mode
         iLockMode = dpsIntentLockMode( requestLockMode ) ;
#ifdef _DEBUG
         ossSnprintf( lockIdStr, sizeof( lockIdStr ),
                      "%s", iLockId.toString().c_str() ) ;
         PD_TRACE3( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE,
                    PD_PACK_STRING( "Testing intent lock:" ),
                    PD_PACK_STRING( lockIdStr ),
                    PD_PACK_BYTE( iLockMode )  ) ;
#endif
         rc = testAcquire( dpsTxExectr, iLockId, iLockMode,
                           isPreemptMode, pdpsTxResInfo, callback, TRUE,
                           &iOwnedLockMode );
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         // if lock escalated or lock owned in upper level can cover lock
         // requesting in lower level, no need to request lock in lower level
         if ( dpsIsCoverLowerLock( iOwnedLockMode, requestLockMode ) )
         {
            // if requesting lock in current level is covered by upper lock,
            // it means we got the same type of lock in current level
            // e.g. IX is requesting in current level, and X is owned in
            // upper level, it means we got X in current level as well
            if ( ownedLockMode )
            {
               *ownedLockMode = iOwnedLockMode ;
            }

            // already covered
            goto done ;
         }
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // test if the request lock mode can be acquired
      // it will not acquire the lock, the LRB will not be added to
      // owner, upgrade or waiter list
      rc = _tryAcquireOrTest( dpsTxExectr, lockId, requestLockMode,
                              testOpMode,
                              bktIdx,
                              FALSE,
                              pdpsTxResInfo,
                              callback,
                              ownedLockMode ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_TESTACQUIRE, rc ) ;
      return rc;
   error:
      goto done;
   }


   //
   // Description: whether a lock is being waited
   // Function:    test whether a lock is being waited by checking if
   //              the waiter list and upgrade list are empty.
   // Input:
   //    lockId -- lock Id
   // Output:
   //    none
   // Return:
   //    True   -- the lock has waiter(s)
   //    False  -- the lock has no waiter(s)
   // Dependency:  the lock manager must be initialized
   //
   BOOLEAN dpsTransLockManager::hasWait( const dpsTransLockId &lockId )
   {
      BOOLEAN result = FALSE;
      UINT32 bktIdx  = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRBHeader *pLRBHdr = NULL ;

      if ( ! lockId.isValid() )
      {
         PD_LOG( PDERROR, "Invalid lockId:%s", lockId.toString().c_str() ) ;
         goto error ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // latch the LRB Header list
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr  = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ) )
      {
         SDB_ASSERT( pLRBHdr, "Invalid LRB Header" ) ;
         if ( pLRBHdr && ( pLRBHdr->waiterLRB || pLRBHdr->upgradeLRB ) )
         {
            result = TRUE ;
         }
      }

      // free LRB Header list latch
      _releaseOpLatch( bktIdx ) ;

   done:
      return result ;
   error:
      goto done ;
   }


   #define DPS_STRING_LEN_MAX ( 512 )
   //
   // format LRB to string, flat one line
   //
   CHAR * dpsTransLockManager::_LRBToString
   (
      dpsTransLRB *pLRB,
      CHAR * pBuf,
      UINT32 bufSz
   )
   {
      if ( pLRB )
      {
         UINT32 seconds = 0, microseconds = 0 ;
         ossTickConversionFactor factor ;
         ossTick endTick ;
         endTick.sample() ;
         ossTickDelta delta = endTick - pLRB->beginTick ;
         delta.convertToTime( factor, seconds, microseconds ) ;

         ossSnprintf( pBuf, bufSz,
            "LRB: %p, EDU: %llu, dpsTxExectr: %p, "
            "eduLrbNext: %p, eduLrbPrev: %p, "
            "lrbHdr: %p, nextLRB: %p ,prevLRB: %p, "
            "refCounter: %llu, lockMode: %s, duration: %llu",
            pLRB,
            pLRB->dpsTxExectr->getEDUID(), pLRB->dpsTxExectr,
            pLRB->eduLrbNext,
            pLRB->eduLrbPrev,
            pLRB->lrbHdr,
            pLRB->nextLRB,
            pLRB->prevLRB,
            pLRB->refCounter,
            lockModeToString( pLRB->lockMode ),
            (UINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      }
      return pBuf ;
   }



   //
   // format LRB to string, each field/member per line, with optional prefix
   //
   CHAR * dpsTransLockManager::_LRBToString
   (
      dpsTransLRB *pLRB,
      CHAR * pBuf,
      UINT32 bufSz,
      CHAR * prefix
   )
   {
      CHAR * pBuff = pBuf;
      CHAR * pDummy= "" ;
      CHAR * pStr  = ( prefix ? prefix : pDummy ) ;
      if ( pLRB )
      {

         UINT32 seconds = 0, microseconds = 0 ;
         ossTickConversionFactor factor ;
         ossTick endTick ;
         endTick.sample() ;
         ossTickDelta delta = endTick - pLRB->beginTick ;
         delta.convertToTime( factor, seconds, microseconds ) ;

         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sEDU          : %llu" OSS_NEWLINE, pStr,
                               pLRB->dpsTxExectr->getEDUID() ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sLRB          : %p" OSS_NEWLINE, pStr,
                               pLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sdpsTxExectr  : %p" OSS_NEWLINE, pStr,
                               pLRB->dpsTxExectr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%seduLrbNext   : %p" OSS_NEWLINE, pStr,
                               pLRB->eduLrbNext ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%seduLrbPrev   : %p" OSS_NEWLINE, pStr,
                               pLRB->eduLrbPrev ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slrbHdr       : %p" OSS_NEWLINE, pStr,
                               pLRB->lrbHdr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%snextLRB      : %p" OSS_NEWLINE, pStr,
                               pLRB->nextLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sprevLRB      : %p" OSS_NEWLINE, pStr,
                               pLRB->prevLRB ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%srefCounter   : %llu" OSS_NEWLINE, pStr,
                               pLRB->refCounter ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slockMode     : %s" OSS_NEWLINE, pStr,
                               lockModeToString( pLRB->lockMode ) ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sduration     : %llu" OSS_NEWLINE, pStr,
                               (UINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      }
      return pBuf ;
   }


   //
   // format LRB Header to flat one line string
   //
   CHAR * dpsTransLockManager::_LRBHdrToString
   (
      dpsTransLRBHeader *pLRBHdr,
      CHAR * pBuf,
      UINT32 bufSz
   )
   {
      if ( pLRBHdr )
      {
         ossSnprintf( pBuf, bufSz,
          "LRB Header: %p, nextLRBHdr: %p, "
          "ownerLRB: %p, waiterLRB : %p, upgradeLRB: %p, "
          "newestISOwner: %p, newestIXOwner: %p,"
          "lockId: ( %s )",
          pLRBHdr,
          pLRBHdr->nextLRBHdr,
          pLRBHdr->ownerLRB,
          pLRBHdr->waiterLRB,
          pLRBHdr->upgradeLRB,
          pLRBHdr->newestISOwner,
          pLRBHdr->newestIXOwner,
          pLRBHdr->lockId.toString().c_str() ) ;
      }
      return pBuf;
   }


   //
   // format LRB Header to string, one field/member per line, w/ optional prefix
   //
   CHAR * dpsTransLockManager::_LRBHdrToString
   (
      dpsTransLRBHeader *pLRBHdr,
      CHAR * pBuf,
      UINT32 bufSz,
      CHAR * prefix
   )
   {
      CHAR * pBuff = pBuf;
      CHAR * pDummy= "" ;
      CHAR * pStr  = ( prefix ? prefix : pDummy ) ;
      if ( pLRBHdr )
      {
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sLRB Header : %p" OSS_NEWLINE, pStr,
                               pLRBHdr ) ;
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%snextLRBHdr : %p" OSS_NEWLINE, pStr,
                              pLRBHdr->nextLRBHdr );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sownerLRB   : %p" OSS_NEWLINE, pStr,
                               pLRBHdr->ownerLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%swaiterLRB  : %p" OSS_NEWLINE, pStr,
                               pLRBHdr->waiterLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%supgradeLRB : %p" OSS_NEWLINE, pStr,
                               pLRBHdr->upgradeLRB );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sISOwner    : %p" OSS_NEWLINE, pStr,
                               pLRBHdr->newestISOwner );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%sIXOwner    : %p" OSS_NEWLINE, pStr,
                               pLRBHdr->newestIXOwner );
         pBuff += ossSnprintf( pBuff, bufSz - strlen( pBuf ),
                               "%slockId     : ( %s )" OSS_NEWLINE, pStr,
                               pLRBHdr->lockId.toString().c_str() ) ;
      }
      return pBuf;
   }


   //
   // dump all LRB in EDU LRB chain to specified the file ( full path name )
   // for debugging purpose .
   // It walks through EDU LRB chain, the caller shall acquire the monitoring(
   // dump ) latch, acquireMonLatch(), and make sure the executor is still
   // available
   //
   void dpsTransLockManager::dumpLockInfo
   (
      _dpsTransExecutor * dpsTxExectr,
      const CHAR        * fileName,
      BOOLEAN             bOutputInPlainMode
   )
   {
      dpsTransLRB *pLRB  = NULL ;
      CHAR * pStr = NULL ;
      CHAR * prefixStr = (CHAR*)"   " ;
      CHAR szBuffer[ DPS_STRING_LEN_MAX ] = { '\0' } ;
      FILE * fp   = NULL;

      if ( dpsTxExectr )
      {
         // open output file
         if ( NULL == ( fp = fopen( fileName, "ab+" ) ) )
         {
            goto error ;
         }

         pLRB = dpsTxExectr->getLastLRB( _lockMgrType ) ;
         while ( pLRB )
         {
            SDB_ASSERT( pLRB->lrbHdr != NULL ,
                        "Invalid LRB Header." ) ;

            if ( bOutputInPlainMode )
            {
               pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                            sizeof(szBuffer) ) ;
            }
            else
            {
               pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                            sizeof(szBuffer), prefixStr ) ;
            }
            fprintf( fp, "%s" OSS_NEWLINE, pStr ) ;
            pLRB = pLRB->eduLrbPrev ;
         }

         // close output file
         if ( fp )
         {
            fclose( fp ) ;
         }
      }
   error:
      return ;
   }


   //
   // dump LRB Header, owner/waiter/upgrade list info
   // to specified file( full path name ), for debugging purpose only
   // This function doesn't need to acquire monitoring/dump
   // latch ( acquireMonLatch() ), it will get the bucket latch
   void dpsTransLockManager::dumpLockInfo
   (
      const dpsTransLockId & lockId,
      const CHAR           * fileName,
      BOOLEAN                bOutputInPlainMode
   )
   {
      dpsTransLockId iLockId;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;
      UINT32             bktIdx;
      CHAR * pStr = NULL ;
      CHAR * prefixStr = (CHAR*)"   " ;
      CHAR szBuffer[ DPS_STRING_LEN_MAX ] = { '\0' } ;

      FILE * fp = NULL ;


      // dump intent lock at first
      if ( ! lockId.isRootLevel() )
      {
         iLockId = lockId.upOneLevel() ;
         dumpLockInfo( iLockId, fileName, bOutputInPlainMode );
      }

      // open output file
      if ( NULL == ( fp = fopen( fileName, "ab+" ) ) )
      {
         goto error ;
      }

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // latch the bucket
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr  = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ))
      {
         fprintf( fp, "%s",  "LRB Header " ) ;
         if ( lockId.isRootLevel() )
         {
            pStr = "( Container Space Lock )" ;
         }
         else if ( lockId.isLeafLevel() )
         {
            pStr = "( Record Lock )" ;
         }
         else
         {
            pStr = "( Container Lock )" ;
         }

         fprintf( fp, " %s" OSS_NEWLINE, pStr ) ;
         fprintf( fp, "%s", "-------------------------------" OSS_NEWLINE ) ;
         if ( bOutputInPlainMode )
         {
            pStr = (CHAR*) _LRBHdrToString(pLRBHdr, szBuffer, sizeof(szBuffer));
         }
         else
         {
            pStr = (CHAR*) _LRBHdrToString(pLRBHdr, szBuffer, sizeof(szBuffer),
                                           NULL );
         }
         fprintf( fp, "%s" OSS_NEWLINE OSS_NEWLINE, pStr ) ;
         if ( pLRBHdr->ownerLRB )
         {
            if ( bOutputInPlainMode )
            {
               fprintf( fp, "%s", "Owner list:" OSS_NEWLINE ) ;
               fprintf( fp, "%s", "-----------" OSS_NEWLINE ) ;
            }
            else
            {
               fprintf( fp, "%sOwner list:" OSS_NEWLINE, prefixStr ) ;
               fprintf( fp, "%s-----------" OSS_NEWLINE, prefixStr ) ;
            }
            pLRB = pLRBHdr->ownerLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr ) ;
               }
               fprintf( fp, "%s" OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         if ( pLRBHdr->upgradeLRB )
         {
            fprintf( fp, "%sUpgrade list:" OSS_NEWLINE, prefixStr ) ;
            fprintf( fp, "%s-------------" OSS_NEWLINE, prefixStr ) ;
            pLRB = pLRBHdr->upgradeLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr );
               }
               fprintf( fp, "%s" OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         if ( pLRBHdr->waiterLRB != NULL )
         {
            fprintf( fp, "%sWaiter list:" OSS_NEWLINE, prefixStr ) ;
            fprintf( fp, "%s------------" OSS_NEWLINE, prefixStr ) ;
            pLRB = pLRBHdr->waiterLRB ;
            while ( pLRB )
            {
               if ( bOutputInPlainMode )
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer) ) ;
               }
               else
               {
                  pStr = (CHAR*) _LRBToString( pLRB, szBuffer,
                                               sizeof(szBuffer), prefixStr );
               }
               fprintf( fp, "%s" OSS_NEWLINE, pStr ) ;
               pLRB = pLRB->nextLRB ;
            }
            fprintf( fp, "%s", OSS_NEWLINE ) ;
         }
         fprintf( fp, "%s", OSS_NEWLINE ) ;
      }

      // free bucket latch
      _releaseOpLatch( bktIdx ) ;

      // close file
      if ( fp )
      {
         fclose( fp ) ;
      }
   error:
      return ;
   }

   //
   // dump EDU LRB info into VEC_TRANSLOCKCUR
   // It walks through the EDU LRB chain, the caller shall acquire
   // the monitoring( dump ) latch, acquireMonLatch(),
   // and make sure the executor is still available
   //
   void dpsTransLockManager::dumpLockInfo
   (
      dpsTransLRB       * lastLRB,
      VEC_TRANSLOCKCUR  & vecLocks
   )
   {
      monTransLockCur monLock ;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      pLRB = lastLRB ;
      while ( pLRB )
      {
         pLRBHdr  = pLRB->lrbHdr ;

         monLock._id    = pLRBHdr->lockId ;
         monLock._mode  = pLRB->lockMode ;
         monLock._count = pLRB->refCounter ;
         monLock._beginTick = pLRB->beginTick ;

         vecLocks.push_back( monLock ) ;

         pLRB = pLRB->eduLrbPrev ;
      }
   }

   INT32 dpsTransLockManager::dumpEDUTransInfo( _dpsTransExecutor *executor,
                                                monTransLockCur &waitLock,
                                                VEC_TRANSLOCKCUR  &eduLocks )
   {
      SDB_ASSERT( NULL != executor, "Should not be null" ) ;
      INT32 rc = SDB_OK ;
      dpsTransLRB *lrb = NULL ;
      INT32 count = 0 ;

      executor->acquireLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
      lrb = executor->getWaiterLRB( LOCKMGR_TRANS_LOCK ) ;
      if ( NULL != lrb )
      {
         dumpLockInfo( lrb, waitLock ) ;
      }

      SDB_ASSERT( executor->getAccessingLRB( LOCKMGR_TRANS_LOCK ) == NULL,
                  "should be NULL" ) ;
      lrb = executor->getLastLRB( LOCKMGR_TRANS_LOCK ) ;
      while ( NULL != lrb )
      {
         monTransLockCur tmpLock ;
         dumpLockInfo( lrb, tmpLock ) ;

         try
         {
            eduLocks.push_back( tmpLock ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when dump trans info: %s",
                    e.what() ) ;
            executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            goto error ;
         }

         ++count ;
         lrb = lrb->eduLrbPrev ;

         if ( count >= DPS_TRANSLOCK_DUMP_SLICE_SIZE && NULL != lrb )
         {
            executor->setAccessingLRB( LOCKMGR_TRANS_LOCK, lrb ) ;
            // release lock to let user session work
            executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            ossYield() ;
            count = 0 ;
            executor->acquireLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
            lrb = executor->getAccessingLRB( LOCKMGR_TRANS_LOCK ) ;
         }
      }

      executor->setAccessingLRB( LOCKMGR_TRANS_LOCK, NULL ) ;
      executor->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // dump the lock info ( lockId, lockMode, refCounter ) into monTransLockCur
   // via LRB
   void dpsTransLockManager::dumpLockInfo
   (
      dpsTransLRB      *lrb,
      monTransLockCur  &lockCur
   )
   {
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      if ( lrb )
      {
         pLRB    = lrb ;
         pLRBHdr  = pLRB->lrbHdr ;

         lockCur._id = pLRBHdr->lockId ;
         lockCur._mode = pLRB->lockMode ;
         lockCur._count = pLRB->refCounter ;
         lockCur._beginTick = pLRB->beginTick ;
      }
   }


   //
   // dump LRB Header and owner / waiter / upgrade list for a specific lock
   // into monTransLockInfo. This function doesn't need to acquire
   // monitoring/dump latch ( acquireMonLatch() ), it will get the bucket latch
   //
   void dpsTransLockManager::dumpLockInfo
   (
      const dpsTransLockId & lockId,
      monTransLockInfo     & monLockInfo
   )
   {
      UINT32             bktIdx  = DPS_LOCK_INVALID_BUCKET_SLOT ;
      dpsTransLRBHeader *pLRBHdr = NULL ;
      dpsTransLRB       *pLRB    = NULL ;

      monTransLockInfo::lockItem monLockItem ;

      // calculate the hash index by lockId
      bktIdx = _getBucketNo( lockId ) ;

      // acquire bucket latch
      _acquireOpLatch( bktIdx ) ;

      pLRBHdr = _LockHdrBkt[bktIdx].lrbHdr ;
      if ( _getLRBHdrByLockId( lockId, pLRBHdr ) )
      {
         monLockInfo._id = pLRBHdr->lockId ;

         // owner list
         if ( pLRBHdr->ownerLRB )
         {
            pLRB = pLRBHdr->ownerLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecHolder.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
         // upgrade list
         if ( pLRBHdr->upgradeLRB )
         {
            pLRB = pLRBHdr->upgradeLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecWaiter.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
         // waiter list
         if ( pLRBHdr->waiterLRB  )
         {
            pLRB = pLRBHdr->waiterLRB ;
            while ( pLRB )
            {
               monLockItem._eduID = pLRB->dpsTxExectr->getEDUID() ;
               monLockItem._mode  = pLRB->lockMode ;
               monLockItem._count = pLRB->refCounter ;
               monLockItem._beginTick = pLRB->beginTick ;

               monLockInfo._vecWaiter.push_back( monLockItem ) ;

               pLRB = pLRB->nextLRB ;
            }
         }
      }

      // release bucket latch
      _releaseOpLatch( bktIdx ) ;
   }


   // return TRUE if a LRB in waiter or upgrade queue
   BOOLEAN dpsTransLockManager::_isInWaiterOrUpgradeQueue
   (
      const dpsTransLRBHeader * pLRBHdr,
      const dpsTransLRB       * pLRB
   )
   {
      if ( pLRB && pLRBHdr && ( pLRBHdr == pLRB->lrbHdr ) )
      {
         if ( pLRBHdr->upgradeLRB )
         {
            dpsTransLRB *plrb = pLRBHdr->upgradeLRB;
            while ( plrb )
            {
               if ( plrb == pLRB )
               {
                  return TRUE ;
               }
               plrb = plrb->nextLRB ;
            }
         }
         if ( pLRBHdr->waiterLRB )
         {
            dpsTransLRB *plrb = pLRBHdr->waiterLRB;
            while ( plrb )
            {
               if ( plrb == pLRB )
               {
                  return TRUE ;
               }
               plrb = plrb->nextLRB ;
            }
         }
      }
      return FALSE ;
   }


   void dpsTransLockManager::snapWaitInfo
   (
      _dpsTransExecutor       * pExctr,
      dpsTransLRB             * pWaiterLRB,
      const dpsTransLockId    & lockId,
      DPS_TRANS_WAIT_SET      & waitInfoSet
   )
   {
      ossTickConversionFactor factor ;
      ossTick endTick ;

      dpsDBNodeID nodeID ;// will get nodeID in snapshot, no need to get it here
      UINT32 bktIdx = _getBucketNo( lockId ) ;
      BOOLEAN waiterLocked = FALSE ;

      endTick.sample() ;
      _acquireOpLatch( bktIdx ) ;

      if ( pExctr == NULL )
      {
         goto done ;
      }

      pExctr->acquireLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
      waiterLocked = TRUE ;

      if ( pWaiterLRB && ( pWaiterLRB == pExctr->getWaiterLRB(LOCKMGR_TRANS_LOCK ) ) )
      {
         DPS_TRANS_ID  waiterTransId  = pExctr->getNormalizedTransID();
         UINT64            waiterCost = pExctr->getLogSpace() ;
         ISession *        pWaiterSes = pExctr->getExecutor()->getSession() ;
         EDUID        waiterSessionID = pExctr->getEDUID();
         UINT64       waiterRelatedID = pWaiterSes->identifyID();
         MsgRouteID  waiterRelatedNID = pWaiterSes->identifyNID();
         UINT32      waiterRelatedTID = pWaiterSes->identifyTID();
         EDUID waiterRelatedSessionID = pWaiterSes->identifyEDUID();

         dpsTransLRBHeader *pLRBHdr = _LockHdrBkt[bktIdx].lrbHdr ;
         if ( ( DPS_INVALID_TRANS_ID != waiterTransId ) &&
              _getLRBHdrByLockId( lockId, pLRBHdr ) )
         {
            if ( ( pWaiterLRB->lrbHdr == pLRBHdr ) &&
                 ( pLRBHdr->ownerLRB ) &&
                 _isInWaiterOrUpgradeQueue( pLRBHdr, pWaiterLRB ) )
            {
               DPS_TRANS_ID holderTransId = DPS_INVALID_TRANS_ID ;
               ISession    *pHolderSes = NULL ;
               dpsTransLRB *pLRB = pLRBHdr->ownerLRB ;
               while ( pLRB )
               {
                  holderTransId = pLRB->dpsTxExectr->getNormalizedTransID() ;
                  pHolderSes = pLRB->dpsTxExectr->getExecutor()->getSession();
                  if ( ( waiterTransId != holderTransId ) &&
                       pHolderSes &&
                       ( DPS_INVALID_TRANS_ID != holderTransId ) )
                  {
                     UINT32 seconds = 0, microseconds = 0 ;
                     ossTickDelta delta = endTick - pWaiterLRB->beginTick ;
                     delta.convertToTime( factor, seconds, microseconds ) ;
                     UINT64 durationInMicroseconds =
                        (UINT64)( seconds * 1000 + microseconds / 1000 );

                     dpsTransWait waitInfo(
                        waiterTransId, holderTransId, nodeID,
                        durationInMicroseconds,// wait time 
                        waiterCost,            // Cost 
                        pLRB->dpsTxExectr->getLogSpace(),
                        waiterSessionID,       // sessionID
                        pLRB->dpsTxExectr->getEDUID(),
                        waiterRelatedID,       // RelatedID 
                        pHolderSes->identifyID(),
                        waiterRelatedTID,      // RelatedTID
                        pHolderSes->identifyTID(),
                        waiterRelatedSessionID,// Related SessionID
                        pHolderSes->identifyEDUID(),
                        waiterRelatedNID,      // RelatedNID, i.e., MsgRouteID
                        pHolderSes->identifyNID() );
                     try
                     {
                        waitInfoSet.insert( waitInfo ) ;
                     }
                     catch( std::exception &e )
                     {
                        PD_LOG( PDERROR,
                                "Exception captured: %s, "
                                "when dump transaction waiting info",
                                e.what() ) ;
                     }
                  }
                  pLRB = pLRB->nextLRB ;
               }
            }
         }
      }

   done:
      if ( waiterLocked )
      {
         pExctr->releaseLRBAccessingLock( LOCKMGR_TRANS_LOCK ) ;
      }
      _releaseOpLatch( bktIdx ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_GETINCOMPTRANS, "dpsTransLockManager::getIncompTrans" )
   INT32 dpsTransLockManager::getIncompTrans
   (
      const dpsTransLockId &     lockID,
      const DPS_TRANSLOCK_TYPE   lockMode,
      DPS_TRANS_ID_SET &         incompTrans
   )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_GETINCOMPTRANS ) ;

      dpsTransLRBHeader *pLRBHdr = NULL ;
      UINT32 bktIdx = _getBucketNo( lockID ) ;

      _acquireOpLatch( bktIdx ) ;

      pLRBHdr = _LockHdrBkt[ bktIdx ].lrbHdr ;
      if ( !_getLRBHdrByLockId( lockID, pLRBHdr ) )
      {
         goto done ;
      }

      rc = _getIncompTrans( pLRBHdr, lockMode, incompTrans ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions "
                   "for lock [%s], lock mode [%s], rc: %d",
                   lockID.toString().c_str(), lockModeToString( lockMode ),
                   rc ) ;

   done:
      _releaseOpLatch( bktIdx ) ;

      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER_GETINCOMPTRANS, rc ) ;

      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER_KILLWAITERS, "dpsTransLockManager::killWaiters" )
   BOOLEAN dpsTransLockManager::killWaiters( const dpsTransLockId &lockID,
                                             INT32 errorCode )
   {
      BOOLEAN killed = FALSE ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER_KILLWAITERS ) ;

      dpsTransLRBHeader *pLRBHdr = NULL ;
      UINT32 bktIdx = _getBucketNo( lockID ) ;

      _acquireOpLatch( bktIdx ) ;

      pLRBHdr = _LockHdrBkt[ bktIdx ].lrbHdr ;
      if ( _getLRBHdrByLockId( lockID, pLRBHdr ) )
      {
         dpsTransLRB *pLRB = (dpsTransLRB *)pLRBHdr->waiterLRB ;
         while ( NULL != pLRB )
         {
            _killWaiter( pLRB->dpsTxExectr, errorCode ) ;
            killed = TRUE ;
            pLRB = pLRB->nextLRB ;
         }
      }

      _releaseOpLatch( bktIdx ) ;

      PD_TRACE_EXIT( SDB_DPSTRANSLOCKMANAGER_KILLWAITERS ) ;

      return killed ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS_HEADER, "dpsTransLockManager::_getIncompTrans" )
   INT32 dpsTransLockManager::_getIncompTrans
   (
      const dpsTransLRBHeader *  pLRBHdr,
      const DPS_TRANSLOCK_TYPE   lockMode,
      DPS_TRANS_ID_SET &         incompTrans
   )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS_HEADER ) ;

      if ( NULL == pLRBHdr )
      {
         goto done ;
      }

      rc = _getIncompTrans( pLRBHdr->ownerLRB, lockMode, incompTrans ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions "
                   "from owner LRB list, rc: %d", rc ) ;

      rc = _getIncompTrans( pLRBHdr->waiterLRB, lockMode, incompTrans ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions "
                   "from waiter LRB list, rc: %d", rc ) ;

      rc = _getIncompTrans( pLRBHdr->upgradeLRB, lockMode, incompTrans ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions "
                   "from upgrader LRB list, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS_HEADER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS, "dpsTransLockManager::_getIncompTrans" )
   INT32 dpsTransLockManager::_getIncompTrans
   (
      const dpsTransLRB *        pLRBBegin,
      const DPS_TRANSLOCK_TYPE   lockMode,
      DPS_TRANS_ID_SET &         incompTrans
   )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS ) ;

      dpsTransLRB *pLRB = (dpsTransLRB *)pLRBBegin ;
      while ( NULL != pLRB )
      {
         if ( !dpsIsLockCompatible( pLRB->lockMode, lockMode ) )
         {
            DPS_TRANS_ID transID = pLRB->dpsTxExectr->getOrigTransID() ;
            // check if transaction ID is valid, if not, it is only a write
            // operation without transaction
            if ( DPS_INVALID_TRANS_ID != transID )
            {
               try
               {
                  incompTrans.insert( transID ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to save incompatible transaction, "
                          "occur exception %s", e.what() ) ;
                  rc = ossException2RC( &e ) ;
                  goto error ;
               }
            }
         }
         pLRB = pLRB->nextLRB ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DPSTRANSLOCKMANAGER__GETINCOMPTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}  // namespace engine
