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

namespace engine
{
   class _netRouteAgent ;
   class _dpsLogWrapper ;
   class _pmdEDUCB ;

   typedef ossPoolMultiSet<DPS_LSN_OFFSET>   CLS_WAKE_PLAN ;

   /*
      _clsSyncManager define
   */
   class _clsSyncManager : public SDBObject
   {
   public:
      _clsSyncManager( _netRouteAgent *agent,
                       _clsGroupInfo *info ) ;

      ~_clsSyncManager() ;

   public:
      INT32 sync( _clsSyncSession &session,
                  const UINT32 &w,
                  INT64 timeout = -1 ) ;

      void  updateNodeStatus( const MsgRouteID &id, BOOLEAN valid ) ;
      void  notifyFullSync( const MsgRouteID &id ) ;

      INT32 updateNotifyList( BOOLEAN newNodeValid ) ;

      void complete( const MsgRouteID &id,
                     const DPS_LSN &lsn,
                     UINT32 TID ) ;

      void handleTimeout( const UINT32 &interval ) ;

      void notify( const DPS_LSN_OFFSET &offset ) ;

      MsgRouteID getSyncSrc( const set<UINT64> &blacklist ) ;

      MsgRouteID getFullSrc( const set<UINT64> &blacklist ) ;

      OSS_INLINE BOOLEAN isReadyToReplay()
      {
         BOOLEAN rc = _enableSync ;
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

      void  enableSync( BOOLEAN enable )
      {
         _enableSync = enable ;
      }

      void cut( UINT32 alives, BOOLEAN isFTWhole = FALSE ) ;

      DPS_LSN_OFFSET getSyncCtrlArbitLSN() ;

      /// offset is current offset.
      BOOLEAN atLeastOne( const DPS_LSN_OFFSET &offset,
                          UINT16 ensureNodeID = 0 ) ;

   private:
      INT32 _wait( _pmdEDUCB *&cb, UINT32 sub, INT64 timeout = -1 ) ;

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
      _clsSyncMinHeap _syncList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      DPS_LSN_OFFSET  _checkList[CLS_REPLSET_MAX_NODE_SIZE -1] ;
      _ossSpinXLatch _mtxs[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      _clsSyncStatus _notifyList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;

      _netRouteAgent *_agent ;
      _clsGroupInfo *_info ;
      MsgRouteID _syncSrc ;

      /// valid _notifyList size
      UINT32 _validSync ;
      UINT32 _timeout ;
      UINT32 _aliveCount ;

      UINT32   _wakeTimeout ;
      BOOLEAN  _enableSync ;

   } ;
}

#endif

