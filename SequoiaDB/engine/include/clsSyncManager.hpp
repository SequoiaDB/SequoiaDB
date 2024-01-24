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

   Source File Name = clsSyncManager.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSSYNCMANAGER_HPP_
#define CLSSYNCMANAGER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsDef.hpp"
#include "ossLatch.hpp"
#include "clsSyncMinHeap.hpp"
#include "msgReplicator.hpp"
#include "ossAtomic.hpp"
#include "ossMemPool.hpp"
#include "utilReplSizePlan.hpp"

namespace engine
{
   class _netRouteAgent ;
   class _dpsLogWrapper ;
   class _pmdEDUCB ;

   struct clsWakePlanCompare
   {
      bool operator() ( const utilReplSizePlan &left,
                        const utilReplSizePlan &right ) const
      {
         BOOLEAN result = FALSE ;
         /* sort in the following order
            1.LSN ( order : sort from small to large )
            2.the location of the primary node ( reverse order )
            3.the affinitive location of the primary node ( reverse order )
            4.location ( reverse order )
          */
         if ( right.offset > left.offset )
         {
            result = TRUE ;
         }
         else if ( right.offset < left.offset )
         {
            /// do nothing
         }
         else if ( right.primaryLocationNodes < left.primaryLocationNodes )
         {
            result = TRUE ;
         }
         else if ( right.affinitiveLocations < left.affinitiveLocations )
         {
            result = TRUE ;
         }
         else if ( right.locations < left.locations )
         {
            result = TRUE ;
         }

         return result ;
      }
   } ;

   typedef ossPoolMultiSet<utilReplSizePlan, clsWakePlanCompare>   CLS_WAKE_PLAN ;

   /*
      _clsSyncManager define
   */
   class _clsSyncManager : public SDBObject
   {
   public:
      _clsSyncManager( _netRouteAgent *agent,
                       _clsGroupInfo *info,
                       _clsGroupInfo *locationInfo = NULL ) ;

      ~_clsSyncManager() ;

   public:
      INT32 sync( _clsSyncSession &session,
                  const UINT32 &w,
                  INT64 timeout = -1,
                  BOOLEAN isFTWhole = FALSE ) ;

      void  updateNodeStatus( const MsgRouteID &id, BOOLEAN valid ) ;
      void  notifyFullSync( const MsgRouteID &id ) ;

      INT32 updateNotifyList( BOOLEAN newNodeValid ) ;

      void complete( const MsgRouteID &id,
                     const DPS_LSN &lsn,
                     UINT32 TID ) ;

      void handleTimeout( const UINT32 &interval ) ;

      void notify( const DPS_LSN_OFFSET &offset ) ;

      MsgRouteID getSyncSrc( const set<UINT64> &blacklist, BOOLEAN isLocationPreferred,
                             CLS_GROUP_VERSION &version ) ;

      MsgRouteID getFullSrc( const set<UINT64> &blacklist, BOOLEAN isLocationPreferred,
                             CLS_GROUP_VERSION &version ) ;

      OSS_INLINE BOOLEAN isReadyToReplay()
      {
         BOOLEAN rc = _blockSync.peek() > 0 ? FALSE : TRUE ;
         if ( rc )
         {
            _info->mtx.lock_r() ;
            rc = ( _info->primary.value == _info->local.value &&
                   _info->primary.value != MSG_INVALID_ROUTEID ) ?
                 FALSE : TRUE ;
            _info->mtx.release_r() ;
         }
         return rc ;
      }

      void  enableSync()
      {
         SDB_ASSERT( _blockSync.peek() > 0, "block sync should be > 0" ) ;
         _blockSync.dec() ;
      }

      void disableSync()
      {
         _blockSync.inc() ;
      }

      void cut( UINT32 alives, BOOLEAN isFTWhole = FALSE ) ;

      DPS_LSN_OFFSET getSyncCtrlArbitLSN() ;

      /// offset is current offset.
      BOOLEAN atLeastOne( const DPS_LSN_OFFSET &offset, UINT16 ensureNodeID = 0 ) ;

      void prepareBlackList( set<UINT64> &blacklist, const CLS_SELECT_RANGE &range ) ;

      BOOLEAN isGroupInfoExpired( CLS_GROUP_VERSION version )
      {
         return version < _info->version ;
      }

   private:
      INT32 _wait( _clsSyncSession &session, UINT32 sub,
                   BOOLEAN hasJump, INT64 timeout = -1 ) ;

      /// compare waitPlan of session whit checkList, and if necessary,
      /// jump to next checklist until it passes checklist.
      INT32 _jump( _clsSyncSession &session, UINT32 &sub, BOOLEAN &needWait, BOOLEAN &hasJump ) ;

      void _createWakePlan( CLS_WAKE_PLAN &plan ) ;

      void _wake( CLS_WAKE_PLAN &plan ) ;

      void _complete( const MsgRouteID &id,
                      const DPS_LSN_OFFSET &offset ) ;


      void _clearSyncList( UINT32 removed, UINT32 removedAlives,
                           UINT32 preAlives, UINT32 preSyncNum,
                           _clsSyncStatus *left ) ;

   private:
      /// sub between <0, CLS_REPLSET_MAX_NODE_SIZE - 2>.
      /// means ( w = 2 ) to ( w = CLS_REPLSET_MAX_NODE_SIZE ).
      _clsSyncMinHeap  _syncList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      utilReplSizePlan _checkList[CLS_REPLSET_MAX_NODE_SIZE -1] ;
      _ossSpinXLatch   _mtxs[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      _clsSyncStatus   _notifyList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;

      _netRouteAgent *_agent ;
      _clsGroupInfo *_info ;
      _clsGroupInfo *_locationInfo ;
      MsgRouteID _syncSrc ;

      /// valid _notifyList size
      UINT32 _validSync ;
      UINT32 _timeout ;
      UINT32 _aliveCount ;

      UINT32   _wakeTimeout ;
      // counts of operations which are blocking repl sync
      ossAtomic32 _blockSync ;
   } ;
}

#endif

