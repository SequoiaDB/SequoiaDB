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

#include "clsSyncManager.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "netRouteAgent.hpp"
#include "clsBase.hpp"
#include <map>
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmd.hpp"

using namespace std ;

namespace engine
{
   const UINT32 CLS_REPLSE_WRITE_ONE = 1 ;
   const UINT32 CLS_SYNC_REQ_INTERVAL = 2000 ;
   const UINT32 CLS_CONSULT_INTERVAL = 5000 ;
   const UINT32 CLS_SYNC_SET_NUM = CLS_REPLSET_MAX_NODE_SIZE - 1;
   const UINT32 CLS_IS_CONSULTING = 0 ;
   const UINT32 CLS_IS_SYNCING = 1 ;

   #define CLS_W_2_SUB( num ) ( (num) - 2 )
   #define CLS_SUB_2_W( sub ) ( (sub) + 2 )

   #define CLS_WAKE_W_TIMEOUT             ( CLS_SYNC_REQ_INTERVAL )

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CLSSYNCMAG, "_clsSyncManager::_clsSyncManager" )
   _clsSyncManager::_clsSyncManager( _netRouteAgent *agent,
                                     _clsGroupInfo *info ):
                                     _agent( agent ),
                                     _info( info ),
                                     _validSync( 0 ),
                                     _timeout( 0 ),
                                     _aliveCount( 0 )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
      _syncSrc.value = MSG_INVALID_ROUTEID ;
      _wakeTimeout = 0 ;

      for ( UINT32 i = 0 ; i < CLS_REPLSET_MAX_NODE_SIZE - 1 ; i++ )
      {
         _checkList[i] = DPS_INVALID_LSN_OFFSET ;
      }

      _enableSync = TRUE ;

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
   }

   _clsSyncManager::~_clsSyncManager()
   {

   }

   void _clsSyncManager::updateNodeStatus( const MsgRouteID & id,
                                           BOOLEAN valid )
   {
      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].valid = valid ;
            if ( valid )
            {
               ++_aliveCount ;
            }
            else
            {
               --_aliveCount ;
            }
            break ;
         }
      }
   }

   void _clsSyncManager::notifyFullSync( const MsgRouteID & id )
   {
      _info->mtx.lock_r() ;

      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].offset = DPS_INVALID_LSN_OFFSET ;
            break ;
         }
      }

      _info->mtx.release_r() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETARBITLSN, "_clsSyncManager::getSyncCtrlArbitLSN" )
   DPS_LSN_OFFSET _clsSyncManager::getSyncCtrlArbitLSN()
   {
      DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
      INT32 syncSty = pmdGetOptionCB()->syncStrategy() ;

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETARBITLSN ) ;

      if ( 0 == _validSync || CLS_SYNC_NONE == syncSty )
      {
         goto done ;
      }
      else
      {
         _info->mtx.lock_r() ;

         for ( UINT32 i = 0 ; i < _validSync ; ++i )
         {
            if ( DPS_INVALID_LSN_OFFSET == _notifyList[i].offset )
            {
               continue ;
            }
            if ( CLS_SYNC_KEEPNORMAL == syncSty &&
                 FALSE == _notifyList[i].isValid() )
            {
               continue ;
            }
            if ( DPS_INVALID_LSN_OFFSET == offset ||
                 _notifyList[i].offset < offset )
            {
               offset = _notifyList[i].offset ;
            }
         }

         _info->mtx.release_r() ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETARBITLSN ) ;
      return offset ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_UPNFYLIST, "_clsSyncManager::updateNotifyList" )
   INT32 _clsSyncManager::updateNotifyList( BOOLEAN newNodeValid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_UPNFYLIST ) ;

      map<UINT64, _clsSharingStatus> &group = _info->info ;
      UINT32 removed = 0 ;
      UINT32 prevAlives = 0 ;
      UINT32 aliveRemoved = 0 ;
      _clsSyncStatus status[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      UINT32 valid = 0 ;

      ossScopedRWLock lock( &_info->mtx, EXCLUSIVE ) ;

      _aliveCount = _info->alives.size() ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( _notifyList[i].valid )
         {
            ++prevAlives ;
         }

         if ( group.end() ==
              group.find( _notifyList[i].id.value ) )
         {
            if ( _notifyList[i].valid )
            {
               ++aliveRemoved ;
            }
            ++removed ;
         }
         else
         {
            status[valid] = _notifyList[i] ;
            ++valid ;
         }
      }

      if ( 0 != removed )
      {
         _clearSyncList( removed, aliveRemoved, prevAlives,
                         _validSync, _notifyList ) ;
      }

      UINT32 merge = valid ;
      map<UINT64, _clsSharingStatus>::const_iterator itr =
                                        group.begin() ;
      for ( ; itr != group.end(); itr++ )
      {
         BOOLEAN has = FALSE ;
         for ( UINT32 j = 0; j < valid; j++ )
         {
            if ( itr->first == status[j].id.value )
            {
               has = TRUE ;
               break ;
            }
         }
         if ( !has )
         {
            status[merge].offset = 0 ; 
            status[merge].id.value = itr->first ;
            status[merge].valid = newNodeValid ;
            ++merge ;
         }
      }

      ossMemcpy( _notifyList, status, merge * sizeof( _clsSyncStatus ) ) ;
      _validSync = merge ;

      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG_UPNFYLIST, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_SYNC, "_clsSyncManager::sync" )
   INT32 _clsSyncManager::sync( _clsSyncSession &session,
                                const UINT32 &w,
                                INT64 timeout )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_SYNC ) ;
      SDB_ASSERT( w <= CLS_REPLSET_MAX_NODE_SIZE &&
                  CLS_REPLSE_WRITE_ONE <= w,
                 "1 <= sync num <= CLS_REPLSET_MAX_NODE_SIZE" ) ;
      SDB_ASSERT( NULL != session.eduCB, "educb should not be NULL" ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != session.endLsn,
                  "end lsn should not be valid" ) ;
      INT32 rc = SDB_OK ;
      UINT32 sub = 0;
      BOOLEAN needWait = TRUE ;

      if ( w <= CLS_REPLSE_WRITE_ONE )
      {
         goto done ;
      }

      _info->mtx.lock_r() ;

      if ( 0 == _validSync )
      {
         _info->mtx.release_r() ;
         goto done ;
      }
      else if ( MSG_INVALID_ROUTEID == _info->primary.value ||
                _info->primary.value != _info->local.value )
      {
         rc = SDB_CLS_WAIT_SYNC_FAILED ;
         _info->mtx.release_r() ;
         goto error ;
      }
      else if ( _aliveCount < _validSync && w > _aliveCount + 1 )
      {
         rc = SDB_CLS_WAIT_SYNC_FAILED ;
         _info->mtx.release_r() ;
         goto error ;
      }
      else if ( w > _validSync + 1 )
      {
         sub = CLS_W_2_SUB( _validSync + 1 ) ;
      }
      else
      {
         sub = CLS_W_2_SUB( w ) ;
      }

      _mtxs[sub].get() ;
      if ( DPS_INVALID_LSN_OFFSET != _checkList[sub] &&
           session.endLsn < _checkList[sub] )
      {
         needWait = FALSE ;
      }
      else
      {
         rc = _syncList[sub].push( session ) ;
      }
      _mtxs[sub].release() ;
      _info->mtx.release_r() ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else if ( needWait )
      {
         rc = _wait( session.eduCB, sub, timeout ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_COMPLETE, "_clsSyncManager::complete" )
   void _clsSyncManager::complete( const MsgRouteID &id,
                                   const DPS_LSN &lsn,
                                   UINT32 TID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_COMPLETE ) ;
      if ( lsn.invalid() || MSG_INVALID_ROUTEID == id.value )
      {
         PD_LOG( PDDEBUG, "sync: invalid complete."
                 "[nodeid:%d] [lsn:%d, %lld]",
                 id.columns.nodeID, lsn.version, lsn.offset ) ;
         goto done ;
      }
      {
      _info->mtx.lock_r() ;
      _MsgRouteID primary = _info->primary ;
      _info->mtx.release_r() ;
      if ( primary.value == _info->local.value &&
           MSG_INVALID_ROUTEID != primary.value )
      {
         _complete( id, lsn.offset ) ;
      }
      else if ( MSG_INVALID_ROUTEID != primary.value )
      {
         _MsgReplVirSyncReq msg ;
         msg.next = lsn ;
         msg.from = id ;
         msg.header.TID = TID ;
         _agent->syncSend( primary, &msg ) ;
      }
      else
      {
      }
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_COMPLETE ) ;
      return ;
   }

   void _clsSyncManager::handleTimeout( const UINT32 &interval )
   {
      _wakeTimeout += interval ;

      if ( _wakeTimeout > CLS_WAKE_W_TIMEOUT )
      {
         ossScopedRWLock lock( &_info->mtx, SHARED ) ;
         CLS_WAKE_PLAN plan ;
         _createWakePlan( plan ) ;
         _wake( plan ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_NOTIFY, "_clsSyncManager::notify" )
   void _clsSyncManager::notify( const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_NOTIFY ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != offset,
                  "offset should not be invalid" ) ;
      _MsgSyncNotify msg ;
      msg.header.TID = CLS_TID_REPL_SYC ;

      _info->mtx.lock_r() ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( 0 == _notifyList[i].id.value )
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }
         else if ( offset == _notifyList[i].offset )
         {
            msg.header.routeID = _notifyList[i].id ;
            _agent->syncSend( _notifyList[i].id, &msg ) ;
         }
         else
         {
         }
      }

      _info->mtx.release_r() ;

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_NOTIFY ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETSYNCSRC, "_clsSyncManager::getSyncSrc" )
   MsgRouteID _clsSyncManager::getSyncSrc( const set<UINT64> &blacklist )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      MsgRouteID res ;
      res.value = MSG_INVALID_ROUTEID ;

      _info->mtx.lock_r() ;
      map<UINT64, _clsSharingStatus *>::iterator itr =
                      _info->alives.find( _info->primary.value ) ;
      if ( _info->alives.end() != itr &&
           CLS_SYNC_STATUS_PEER ==
           itr->second->beat.syncStatus )
      {
         res.value = itr->first ;
         goto done ;
      }
      else
      {
         MsgRouteID ids[CLS_SYNC_SET_NUM] ;
         UINT32 validNum = 0 ;
         itr = _info->alives.begin() ;
         for ( ; itr != _info->alives.end(); itr++ )
         {
            if ( CLS_SYNC_STATUS_PEER == itr->second->beat.syncStatus &&
                 0 == blacklist.count( itr->first ) )
            {
               ids[validNum++].value = itr->first ;
            }
         }
         if ( 0 == validNum )
         {
            itr = _info->alives.begin() ;
            for ( ; itr != _info->alives.end(); itr++ )
            {
               if ( CLS_SYNC_STATUS_RC == itr->second->beat.syncStatus &&
                    itr->first != _info->primary.value &&
                    0 == blacklist.count( itr->first ))
               {
                  ids[validNum++].value = itr->first ;
               }
            }
            if ( 0 != validNum )
            {
               res.value = ids[ossRand() % validNum].value ;
            }
            else
            {
               res.value = _info->primary.value ;
            }
         }
         else
         {
            res.value = ids[ossRand() % validNum].value ;
         }
      }
   done:
      _info->mtx.release_r() ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETFULLSRC, "_clsSyncManager::getFullSrc" )
   MsgRouteID _clsSyncManager::getFullSrc( const set<UINT64> &blacklist )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      MsgRouteID id ;
      id.value = MSG_INVALID_ROUTEID ;
      _info->mtx.lock_r() ;
      /*MsgRouteID ids[CLS_REPLSET_MAX_NODE_SIZE -1 ] ;
      map<UINT64, _clsSharingStatus *>::iterator itr =
                           _info->alives.begin() ;
      UINT16 sub = 0 ;
      for ( ; itr != _info->alives.end(); itr++ )
      {
         if ( itr->first == _info->primary.value )
         {
            continue ;
         }
         else if ( 0 != blacklist.count( itr->first ) )
         {
            continue ;
         }
         else
         {
            ids[sub++].value = itr->first ;
         }
      }

      if ( 0 != sub )
      {
        id = ids[ossRand() % sub] ;
      }
      else
      {
         id = _info->primary ;
      }*/

      if ( _info->primary.columns.nodeID !=
           _info->local.columns.nodeID )
      {
         id = _info->primary ;
      }

      _info->mtx.release_r() ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_CUT, "_clsSyncManager::cut" )
   void _clsSyncManager::cut( UINT32 alives )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_CUT ) ;
      SDB_ASSERT( alives <= _validSync, "impossible" ) ;
      if ( _validSync < alives )
      {
         PD_LOG( PDWARNING, "sync: alives is bigger than valid sync."
                 "[alives:%d][valid:%d]", alives, _validSync ) ;
         goto done ;
      }
      {
         _clsSyncSession session ;
         for ( SINT32 i = (SINT32)_validSync - 1 ; i > (SINT32)alives - 1 ;
               --i )
         {
            _mtxs[i].get() ;
            while ( SDB_OK == _syncList[i].pop( session ) )
            {
               session.eduCB->getEvent().signal ( SDB_CLS_WAIT_SYNC_FAILED ) ;
            }
            _mtxs[i].release() ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_CUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_ATLEASTONE, "_clsSyncManager::atLeastOne" )
   BOOLEAN _clsSyncManager::atLeastOne( const DPS_LSN_OFFSET &offset )
   {
      BOOLEAN res = FALSE ;
      PD_TRACE_ENTRY( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      DPS_LSN lsn ;
      lsn.offset = offset ;
      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( 0 > lsn.compareOffset( _notifyList[i].offset ) )
         {
            res = TRUE ;
            break ;
         }
      }
      PD_TRACE_EXIT( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__COMPLETE, "_clsSyncManager::_complete" )
   void _clsSyncManager::_complete( const MsgRouteID &id,
                                    const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__COMPLETE ) ;
      DPS_LSN lsn ;
      lsn.offset = offset ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( _notifyList[i].id.value == id.value )
         {
            INT32 result = lsn.compareOffset( _notifyList[i].offset ) ;
            if ( 0 == result )
            {
               ++_notifyList[i].sameReqTimes ;
            }
            else
            {
               _notifyList[i].sameReqTimes = 0 ;
            }

            if ( result > 0 )
            {
               _notifyList[i].offset = offset ;
            }
            break ;
         }
      }

      {
         CLS_WAKE_PLAN plan ;
         _createWakePlan( plan ) ;
         _wake( plan ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__COMPLETE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__WAKE, "_clsSyncManager::_wake" )
   void _clsSyncManager::_wake( CLS_WAKE_PLAN &plan )
   {

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAKE ) ;
      _wakeTimeout = 0 ;
      SDB_ASSERT( plan.size() <= CLS_REPLSET_MAX_NODE_SIZE - 1,
                  "plan size should <= CLS_REPLSET_MAX_NODE_SIZE - 1" ) ;

      _clsSyncSession session ;
      UINT32 sub = 0 ;
      CLS_WAKE_PLAN::reverse_iterator ritr = plan.rbegin();
      while ( ritr != plan.rend() )
      {
         DPS_LSN lsn ;
         lsn.offset = *ritr ;
         _mtxs[sub].get() ;
         _checkList[sub] = lsn.offset ;
         while ( SDB_OK == _syncList[sub].root( session ) )
         {
            if ( 0 < lsn.compareOffset( session.endLsn ) )
            {
               session.eduCB->getEvent().signal ( SDB_OK ) ;
               _syncList[sub].pop( session ) ;
            }
            else
            {
               break ;
            }
         }
         _mtxs[sub].release() ;

         ++ritr ;
         ++sub ;
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__WAKE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CTWAKEPLAN, "_clsSyncManager::_createWakePlan" )
   void _clsSyncManager::_createWakePlan( CLS_WAKE_PLAN &plan )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CTWAKEPLAN ) ;

      DPS_LSN_OFFSET offsetTmp = DPS_INVALID_LSN_OFFSET ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( DPS_INVALID_LSN_OFFSET == _notifyList[i].offset )
         {
            plan.insert( DPS_INVALID_LSN_OFFSET - 1 ) ;
         }
         else
         {
            offsetTmp = _notifyList[i].offset ;
            plan.insert( offsetTmp ) ;
         }
      }

      SDB_ASSERT( plan.size() == _validSync, "impossible") ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CTWAKEPLAN ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__WAIT, "_clsSyncManager::_wait" )
   INT32 _clsSyncManager::_wait( _pmdEDUCB *&cb, UINT32 sub, INT64 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAIT ) ;
      PD_LOG( PDDEBUG, "sync: wait [w:%d]", CLS_SUB_2_W( sub ) ) ;
      pmdEDUEvent ev ;
      INT64 tmpTime = 0 ;
      while ( !cb->isInterrupted() )
      {
         tmpTime = timeout >= 0 ?
                   ( timeout > OSS_ONE_SEC ? OSS_ONE_SEC : timeout ) :
                   OSS_ONE_SEC ;
         if ( SDB_OK != cb->getEvent().wait ( tmpTime, &rc ) )
         {
            if ( timeout >= 0 )
            {
               timeout -= tmpTime ;
               if ( timeout <= 0 )
               {
                  rc = SDB_TIMEOUT ;
                  break ;
               }
            }
            continue ;
         }
         else
         {
            goto done ;
         }
      }

      {
         _mtxs[sub].get() ;
         _clsSyncMinHeap &heap = _syncList[sub] ;
         UINT32 i = 0 ;

         while ( i < heap.dataSize() )
         {
            if ( cb == heap[i].eduCB )
            {
               PD_LOG ( PDDEBUG, "Session[ID:%lld, LSN:%lld] interrupt,remove "
                        "from heap[sub:%d, index:%d]", heap[i].eduCB->getID(),
                        heap[i].endLsn, sub, i ) ;
               heap.erase( i ) ;
               break ;
            }
            ++i ;
         }
         _mtxs[sub].release() ;
         if ( SDB_OK == rc )
         {
            rc = SDB_APP_INTERRUPT ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG__WAIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CRSYNCLIST, "_clsSyncManager::_clearSyncList" )
   void _clsSyncManager::_clearSyncList( UINT32 removed, UINT32 removedAlives,
                                         UINT32 preAlives, UINT32 preSyncNum,                           
                                         _clsSyncStatus *left )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CRSYNCLIST ) ;

      UINT32 okW = preSyncNum - removed ; // except self
      UINT32 endRemovedSub = preAlives - removedAlives ;

      UINT32 removedSub = preSyncNum - 1 ;
      for ( ; removedSub >= endRemovedSub ; --removedSub )
      {
         _mtxs[removedSub].get() ;
         if ( endRemovedSub > 0 )
         {
            _mtxs[endRemovedSub-1].get() ;
         }
         _clsSyncSession session ;

         while ( SDB_OK == _syncList[removedSub].pop( session ) )
         {
            UINT32 complete = 0 ;
            for ( UINT32 j = 0; j < preSyncNum - 1 ; ++j )
            {
               if ( session.endLsn < left[j].offset )
               {
                  ++complete ;
               }
            }
            if ( okW <= complete )
            {
               session.eduCB->getEvent().signal ( SDB_OK ) ;
            }
            else if ( removedSub + 1 > endRemovedSub + removed )
            {
               session.eduCB->getEvent().signal ( SDB_CLS_WAIT_SYNC_FAILED ) ;
            }
            else
            {
               _syncList[endRemovedSub-1].push( session ) ;
            }
         }
         _mtxs[removedSub].release() ;
         if ( endRemovedSub > 0 )
         {
            _mtxs[endRemovedSub-1].release() ;
         }
         else if ( 0 == removedSub )
         {
            break ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CRSYNCLIST ) ;
   }

}
