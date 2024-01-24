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

#include "clsSyncManager.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "netRouteAgent.hpp"
#include "clsBase.hpp"
#include <map>
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmd.hpp"
#include "utilReplSizePlan.hpp"
#include "utilBitmap.hpp"

using namespace std ;

namespace engine
{
   const UINT32 CLS_REPLSE_WRITE_ONE = 1 ;
   const UINT32 CLS_SYNC_SET_NUM = CLS_REPLSET_MAX_NODE_SIZE - 1;

   #define CLS_W_2_SUB( num ) ( (num) - 2 )
   #define CLS_SUB_2_W( sub ) ( (sub) + 2 )

   #define CLS_WAKE_W_TIMEOUT             ( 2000 )

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CLSSYNCMAG, "_clsSyncManager::_clsSyncManager" )
   _clsSyncManager::_clsSyncManager( _netRouteAgent *agent,
                                     _clsGroupInfo *info,
                                     _clsGroupInfo *locationInfo ):
                                     _agent( agent ),
                                     _info( info ),
                                     _locationInfo( locationInfo ),
                                     _validSync( 0 ),
                                     _timeout( 0 ),
                                     _aliveCount( 0 ),
                                     _blockSync( 0 )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
      _syncSrc.value = MSG_INVALID_ROUTEID ;
      _wakeTimeout = 0 ;

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
   }

   _clsSyncManager::~_clsSyncManager()
   {
      SDB_ASSERT( 0 == _blockSync.peek(), "block sync should be 0" ) ;
   }

   // The function is called already in _info->mtx.lock_w(),
   // so can't lock any way
   void _clsSyncManager::updateNodeStatus( const MsgRouteID & id,
                                           BOOLEAN valid )
   {
      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         // find
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

   // The function is called by the full sync(source) thread, so need to
   // use lock
   void _clsSyncManager::notifyFullSync( const MsgRouteID & id )
   {
      _info->mtx.lock_r() ;

      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         // find
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].offset = DPS_INVALID_LSN_OFFSET ;
            break ;
         }
      }

      _info->mtx.release_r() ;
   }

   // The function is called by shard session thread, so need to use lock
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
            if ( CLS_GROUP_MODE_CRITICAL == _info->localGrpMode && 
                 INVALID_NODE_ID == _info->grpMode.grpModeInfo[0].nodeID )
            {
               map<UINT64, _clsSharingStatus *>::const_iterator itr = 
                           _info->alives.find( _notifyList[i].id.value ) ;
               // If group is in critical location mode, ignore the nodes which are not in critical mode
               if ( _info->alives.end() == itr || ! itr->second->isInCriticalMode() )
               {
                  continue ;
               }
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

      /// info's changing is handled in one thread.
      /// no need to require lock.
      map<UINT64, _clsSharingStatus> &group = _info->info ;
      UINT32 removed = 0 ;
      UINT32 prevAlives = 0 ;
      UINT32 aliveRemoved = 0 ;
      _clsSyncStatus status[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      UINT32 valid = 0 ;

      ossScopedRWLock lock( &_info->mtx, EXCLUSIVE ) ;

      _aliveCount = _info->alives.size() ;

      /// find removed nodes
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

      /// clear synclist
      if ( 0 != removed )
      {
         _clearSyncList( removed, aliveRemoved, prevAlives,
                         _validSync, _notifyList ) ;
      }

      UINT32 merge = valid ;
      map<UINT64, _clsSharingStatus>::const_iterator itr =
                                        group.begin() ;
      /// add new nodes
      for ( ; itr != group.end(); itr++ )
      {
         BOOLEAN has = FALSE ;
         for ( UINT32 j = 0; j < valid; j++ )
         {
            if ( itr->first == status[j].id.value )
            {
               has = TRUE ;
               status[j].locationID = itr->second.locationID ;
               status[j].affinitive = itr->second.isAffinitiveLocation ;
               status[j].locationIndex = itr->second.locationIndex ;
               break ;
            }
         }
         if ( !has )
         {
            status[merge].offset = 0 ;
            status[merge].id.value = itr->first ;
            status[merge].valid = newNodeValid ;
            status[merge].locationID = itr->second.locationID ;
            status[merge].affinitive = itr->second.isAffinitiveLocation ;
            status[merge].locationIndex = itr->second.locationIndex ;
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
                                INT64 timeout,
                                BOOLEAN isFTWhole )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_SYNC ) ;
      SDB_ASSERT( w <= CLS_REPLSET_MAX_NODE_SIZE &&
                  CLS_REPLSE_WRITE_ONE <= w,
                 "1 <= sync num <= CLS_REPLSET_MAX_NODE_SIZE" ) ;
      SDB_ASSERT( NULL != session.eduCB, "educb should not be NULL" ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != session.waitPlan.offset,
                  "end lsn should not be valid" ) ;
      INT32 rc = SDB_OK ;
      UINT32 sub = 0;
      BOOLEAN needWait = TRUE ;
      BOOLEAN hasJump = FALSE ;

      /// if w <= 1, return
      if ( CLS_REPLSE_WRITE_ONE >= w )
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
         /// has change to secondary. Why need to check primary again,
         /// because change primary will cut(0), but this thread has not push
         /// to wait queue, so, need to check again
         rc = SDB_CLS_WAIT_SYNC_FAILED ;
         _info->mtx.release_r() ;
         goto error ;
      }
      else if ( _aliveCount < _validSync && w > _aliveCount + 1 )
      {
         // if ReplSize is -1, or ReplSize is valid with FT whole mode,
         // we can degrade the ReplSize for wait sync, report node is down
         // to caller, who can adjust ReplSize if needed
         if ( ( -1 == session.eduCB->getOrgReplSize() ) ||
              ( 1 != session.eduCB->getOrgReplSize() &&
                isFTWhole ) )
         {
            rc = SDB_DATABASE_DOWN ;
         }
         else
         {
            rc = SDB_CLS_WAIT_SYNC_FAILED ;
         }
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
      if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
           session.waitPlan.isPassed( _checkList[sub] ) )
      {
         needWait = FALSE ;
         _mtxs[sub].release() ;
      }
      else if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
                session.waitPlan.offset < _checkList[sub].offset )
      {
         _mtxs[sub].release() ;
         if ( _validSync - 1 == sub )
         {
            // if it's last check list, session will not need to sync
            needWait = FALSE ;
         }
         else
         {
            rc = _jump( session, sub, needWait, hasJump ) ;
         }
      }
      else
      {
         rc = _syncList[sub].push( session ) ;
         _mtxs[sub].release() ;
      }
      _info->mtx.release_r() ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else if ( needWait )
      {
         rc = _wait( session, sub, hasJump, timeout ) ;
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
         _agent->syncSend( primary, (MsgHeader *)&msg ) ;
      }
      else
      {
         /// do noting
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

   // The function is called by repl session(src), so need use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_NOTIFY, "_clsSyncManager::notify" )
   void _clsSyncManager::notify( const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_NOTIFY ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != offset,
                  "offset should not be invalid" ) ;
      _MsgSyncNotify msg ;
      msg.header.TID = CLS_TID_REPL_SYC ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( 0 == _notifyList[i].id.value )
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }
         /// compare the offset of lsn.
         /// the node which request the latest lsn
         /// will be nofitied.
         else if ( offset == _notifyList[i].offset )
         {
            msg.header.routeID = _notifyList[i].id ;
            _agent->syncSend( _notifyList[i].id, (MsgHeader *)&msg ) ;
         }
         else
         {
            /// do nothing
         }
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_NOTIFY ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_PREPAREBLACKLIST, "_clsSyncManager::prepareBlackList" )
   void _clsSyncManager::prepareBlackList( set<UINT64> &blacklist, const CLS_SELECT_RANGE &range )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_PREPAREBLACKLIST ) ;
      map<UINT64, _clsSharingStatus>::iterator iterInfo ;
      map<UINT64, _clsSharingStatus *>::iterator iterAlive ;
      UINT32 localLocationID = pmdGetLocationID() ;

      if ( CLS_SELECT_LOCATION != range && blacklist.empty() )
      {
         goto done ;
      }

      _info->mtx.lock_r() ;

      switch ( range )
      {
         case CLS_SELECT_BEGIN:
            break ;
         case CLS_SELECT_LOCATION:
         {
            // It is possible that someone who is not in the location
            // has restarted and make a wrong choice.
            // therefore, get from all node.
            for ( iterInfo = _info->info.begin(); iterInfo != _info->info.end(); iterInfo++ )
            {
               if ( iterInfo->second.beat.locationID != localLocationID )
               {
                  try
                  {
                     blacklist.insert( iterInfo->first ) ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "Fail to insert black list"
                             "exception occurred: %s", e.what() ) ;
                     _info->mtx.release_r() ;
                     goto error ;
                  }
               }
            }

            break ;
         }
         case CLS_SELECT_AFFINITY_LOCATION:
         {
            for ( iterAlive = _info->alives.begin(); iterAlive != _info->alives.end(); iterAlive++ )
            {
               if ( iterAlive->second->isAffinitiveLocation )
               {
                  blacklist.erase( iterAlive->first ) ;
               }
            }

            break ;
         }
         case CLS_SELECT_GROUP:
         {
            if ( MSG_INVALID_LOCATIONID == localLocationID )
            {
               blacklist.clear() ;
            }
            else
            {
               iterAlive = _info->alives.begin() ;
               for ( ; iterAlive != _info->alives.end(); iterAlive++ )
               {
                  if ( !iterAlive->second->isAffinitiveLocation )
                  {
                     blacklist.erase( iterAlive->first ) ;
                  }
               }
            }

            break ;
         }
         case CLS_SELECT_END:
            break ;
         default:
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
            break ;
         }
      }

      _info->mtx.release_r() ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_PREPAREBLACKLIST ) ;
      return ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETSYNCSRC, "_clsSyncManager::getSyncSrc" )
   MsgRouteID _clsSyncManager::getSyncSrc( const set<UINT64> &blacklist,
                                           BOOLEAN isLocationPreferred,
                                           CLS_GROUP_VERSION &version)
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      MsgRouteID res ;
      res.value = MSG_INVALID_ROUTEID ;
      MsgRouteID priIds[ CLS_SYNC_SET_NUM ] ;
      MsgRouteID secIds[ CLS_SYNC_SET_NUM ] ;
      UINT32 priSub = 0, secSub = 0 ;
      map<UINT64, _clsSharingStatus *>::iterator itr ;

      _info->mtx.lock_r() ;

      // update group info version
      version = _info->version ;

      for ( itr = _info->alives.begin(); itr != _info->alives.end(); itr++ )
      {
         if ( CLS_SYNC_STATUS_PEER == itr->second->beat.syncStatus &&
              0 == blacklist.count( itr->first ) )
         {
            if ( itr->second->isPrimary( isLocationPreferred ) )
            {
               priIds[ priSub++ ].value = itr->first ;
            }
            else
            {
               secIds[ secSub++ ].value = itr->first ;
            }
         }
      }

      if ( 0 != priSub )
      {
         res.value = priIds[ ossRand() % priSub ].value ;
      }
      else if ( 0 != secSub )
      {
          res.value = secIds[ ossRand() % secSub ].value ;
      }
      else
      {
         priSub = 0, secSub = 0 ;
         for ( itr = _info->alives.begin() ; itr != _info->alives.end(); itr++ )
         {
            if ( CLS_SYNC_STATUS_RC == itr->second->beat.syncStatus &&
                 0 == blacklist.count( itr->first ) )
            {
               if ( itr->second->isPrimary( isLocationPreferred ) )
               {
                  priIds[ priSub++ ].value = itr->first ;
               }
               else
               {
                  secIds[ secSub++ ].value = itr->first ;
               }
            }
         }
         if ( 0 != priSub )
         {
            res.value = priIds[ ossRand() % priSub ].value ;
         }
         else if ( 0 != secSub )
         {
            res.value = secIds[ ossRand() % secSub ].value ;
         }
         else
         {
            res.value = _info->primary.value ;
         }
      }
      _info->mtx.release_r() ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETFULLSRC, "_clsSyncManager::getFullSrc" )
   MsgRouteID _clsSyncManager::getFullSrc( const set<UINT64> &blacklist,
                                           BOOLEAN isLocationPreferred,
                                           CLS_GROUP_VERSION &version )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      MsgRouteID id ;
      id.value = MSG_INVALID_ROUTEID ;
      _info->mtx.lock_r() ;
      // update group info version
      version = _info->version ;

      MsgRouteID secIds[ CLS_SYNC_SET_NUM ] ;
      MsgRouteID priIds[ CLS_SYNC_SET_NUM ] ;
      UINT32 secSub = 0, priSub = 0 ;
      map<UINT64, _clsSharingStatus *>::iterator itr = _info->alives.begin() ;
      for ( ; itr != _info->alives.end(); itr++ )
      {
         if ( 0 != blacklist.count( itr->first ) )
         {
            continue ;
         }
         else if ( 0 != itr->second->beat.getFTConfirmStat() ||
                   SERVICE_NORMAL != itr->second->beat.serviceStatus )
         {
            continue ;
         }
         else if ( itr->second->isPrimary( isLocationPreferred ) )
         {
            priIds[ priSub++ ].value = itr->first ;
         }
         else if ( 0 != itr->second->beat.endLsn.offset )
         {
            secIds[ secSub++ ].value = itr->first ;
         }
      }

      if ( 0 != secSub )
      {
         id = secIds[ ossRand() % secSub ] ;
      }
      else if ( 0 != priSub )
      {
         id = priIds[ ossRand() % priSub ] ;
      }
      _info->mtx.release_r() ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_CUT, "_clsSyncManager::cut" )
   void _clsSyncManager::cut( UINT32 alives, BOOLEAN isFTWhole )
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
               if ( -1 == session.eduCB->getOrgReplSize() ||
                    ( 1 != session.eduCB->getOrgReplSize() &&
                      isFTWhole ) )
               {
                  session.eduCB->getEvent().signal( SDB_DATABASE_DOWN ) ;
               }
               else
               {
                  session.eduCB->getEvent().signal ( SDB_CLS_WAIT_SYNC_FAILED ) ;
               }
            }
            _mtxs[i].release() ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_CUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_ATLEASTONE, "_clsSyncManager::atLeastOne" )
   BOOLEAN _clsSyncManager::atLeastOne( const DPS_LSN_OFFSET &offset,
                                        UINT16 ensureNodeID )
   {
      BOOLEAN res = _validSync > 0 ? FALSE : TRUE ;
      PD_TRACE_ENTRY( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      DPS_LSN lsn ;
      lsn.offset = offset ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         /// Found ensureNodeID
         if ( 0 != ensureNodeID &&
              ensureNodeID == _notifyList[i].id.columns.nodeID )
         {
            if ( 0 > lsn.compareOffset( _notifyList[i].offset ) )
            {
               res = TRUE ;
            }
            else
            {
               res = FALSE ;
            }
            break ;
         }
         else if ( 0 > lsn.compareOffset( _notifyList[i].offset ) )
         {
            res = TRUE ;
            if ( 0 == ensureNodeID )
            {
               break ;
            }
         }
      }
      PD_TRACE_EXIT( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      return res ;
   }

   // The function is called by repl session, so need to use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__COMPLETE, "_clsSyncManager::_complete" )
   void _clsSyncManager::_complete( const MsgRouteID &id,
                                    const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__COMPLETE ) ;
      DPS_LSN lsn ;
      lsn.offset = offset ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      /// update notify list
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
         /// wake up agent thread which is waiting
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
      /// eg: we got a plan : { 0, 5, 10 }
      /// begin from w = 2 sync list. we pop all nodes which
      /// lsn is lower than 10. then we erase 10 from plan.
      /// next, in the list which w = 3, we pop all nodes which
      /// lsn is lower than 5. then erase 5 from plan.
      /// at last, in the list which w = 4. we pop all nodes
      /// which lsn is lower than 0. pop 0 from plan.
      /// plan is empty. the waking is done.

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAKE ) ;
      _wakeTimeout = 0 ;
      SDB_ASSERT( plan.size() <= CLS_REPLSET_MAX_NODE_SIZE - 1,
                  "plan size should <= CLS_REPLSET_MAX_NODE_SIZE - 1" ) ;

      _clsSyncSession session ;
      /// begin from w = 2.
      UINT32 sub = 0 ;
      CLS_WAKE_PLAN::reverse_iterator ritr = plan.rbegin();
      while ( ritr != plan.rend() )
      {
         /// get max elemenet
         DPS_LSN lsn ;
         lsn.offset = (*ritr).offset ;
         _mtxs[sub].get() ;
         _checkList[sub] = *ritr ;
         while ( SDB_OK == _syncList[sub].root( session ) )
         {
            if ( 0 < lsn.compareOffset( session.waitPlan.offset ) )
            {
               if ( session.waitPlan.isPassed( _checkList[sub] ) )
               {
                  session.eduCB->getEvent().signal ( SDB_OK ) ;
                  _syncList[sub].pop( session ) ;
               }
               else
               {
                  PD_LOG( PDDEBUG, "Session[LSN:%llu, ID:%lld] need level up",
                          session.waitPlan.offset, session.eduCB->getID() ) ;
                  session.eduCB->getEvent().signal( SDB_OPERATION_RETRY ) ;
                  _syncList[sub].pop( session ) ;
               }
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

      UINT32 fullSyncNodes = 0 ;

      DPS_LSN_OFFSET offsetMin = DPS_INVALID_LSN_OFFSET - 1 ;
      utilReplSizePlan wakePlan ;
      UINT32 selfLocation = pmdGetLocationID() ;
      _utilStackBitmap< CLS_REPLSET_MAX_NODE_SIZE > isMarked ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( DPS_INVALID_LSN_OFFSET == _notifyList[ i ].offset )
         {
            ++ fullSyncNodes ;
         }
         else if ( offsetMin > _notifyList[ i ].offset )
         {
            offsetMin = _notifyList[ i ].offset ;
         }
      }

      if ( fullSyncNodes == _validSync )
      {
         // All nodes are under full sync
         offsetMin = DPS_INVALID_LSN_OFFSET - 1 ;
      }

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         wakePlan.reset() ;
         if ( DPS_INVALID_LSN_OFFSET == _notifyList[i].offset )
         {
            /// DPS_INVALID_LSN_OFFSET is for full sync, so ignore the node.
            /// Save as min offset of other nodes
            wakePlan.offset = offsetMin ;
         }
         else
         {
            wakePlan.offset = _notifyList[i].offset ;
         }

         if ( MSG_INVALID_LOCATIONID != selfLocation &&
              MSG_INVALID_LOCATIONID != _notifyList[i].locationID )
         {
            if ( selfLocation == _notifyList[i].locationID )
            {
               wakePlan.primaryLocationNodes = 1 ;
               wakePlan.affinitiveNodes = 1 ;
               isMarked.setBit( _notifyList[i].locationIndex ) ;
            }
            else if ( !isMarked.testBit( _notifyList[i].locationIndex ) )
            {
               wakePlan.locations = 1 ;
               if ( _notifyList[i].affinitive )
               {
                  wakePlan.affinitiveLocations = 1 ;
                  wakePlan.affinitiveNodes = 1 ;
               }
               isMarked.setBit( _notifyList[i].locationIndex ) ;
            }
            else if ( _notifyList[i].affinitive )
            {
               wakePlan.affinitiveNodes = 1 ;
            }
         }
         plan.insert( wakePlan ) ;
      }

      if ( MSG_INVALID_LOCATIONID != selfLocation &&
           CLS_REPLSET_MAX_NODE_SIZE != isMarked.freeSize() )
      {
         CLS_WAKE_PLAN::reverse_iterator ritr = plan.rbegin() ;
         wakePlan.reset() ;
         wakePlan.affinitiveNodes = 1 ;
         while ( ritr != plan.rend() )
         {
            wakePlan.affinitiveLocations += (*ritr).affinitiveLocations ;
            wakePlan.primaryLocationNodes += (*ritr).primaryLocationNodes ;
            wakePlan.locations += (*ritr).locations ;
            wakePlan.affinitiveNodes += (*ritr).affinitiveNodes ;

            (*ritr).affinitiveLocations = wakePlan.affinitiveLocations ;
            (*ritr).primaryLocationNodes = wakePlan.primaryLocationNodes ;
            (*ritr).locations = wakePlan.locations ;
            (*ritr).affinitiveNodes = wakePlan.affinitiveNodes ;
            ++ritr ;
         }
      }

      SDB_ASSERT( plan.size() == _validSync, "impossible") ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CTWAKEPLAN ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__WAIT, "_clsSyncManager::_wait" )
   INT32 _clsSyncManager::_wait( _clsSyncSession &session, UINT32 sub,
                                 BOOLEAN hasJump, INT64 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAIT ) ;
      PD_LOG( PDDEBUG, "sync: wait [w:%d]", CLS_SUB_2_W( sub ) ) ;
      pmdEDUEvent ev ;
      INT64 tmpTime = 0 ;
      BOOLEAN hasBlock = FALSE ;
      pmdEDUCB *cb = session.eduCB ;

      while ( !cb->isInterrupted() )
      {
         tmpTime = timeout >= 0 ?
                   ( timeout > OSS_ONE_SEC ? OSS_ONE_SEC : timeout ) :
                   OSS_ONE_SEC ;
         /// wait for responses from other nodes.
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

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_SYNCWAIT, "Waiting replicas sync" ) ;
               hasBlock = TRUE ;
            }
            continue ;
         }
         else if ( SDB_OPERATION_RETRY == rc )
         {
            cb->getEvent().reset() ;

            if ( _validSync - 1 == sub )
            {
               // if it's last check list, session will not need to sync
               rc = SDB_OK ;
               goto done ;
            }

            BOOLEAN needWait = TRUE ;
            rc = _jump( session, sub, needWait, hasJump ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
            else if ( !needWait )
            {
               goto done ;
            }
         }
         else if ( SDB_CLS_WAIT_SYNC_FAILED == rc )
         {
            if ( hasJump && pmdIsPrimary() )
            {
               rc = SDB_TIMEOUT ;
            }
            break ;
         }
         else
         {
            goto done ;
         }
      }

      if ( hasBlock )
      {
         cb->unsetBlock() ;
         hasBlock = FALSE ;
      }

      /// interrupted or timeout, clear info.
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
                        heap[i].waitPlan.offset, sub, i ) ;
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
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG__WAIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__JUMP, "_clsSyncManager::_jump" )
   INT32 _clsSyncManager::_jump( _clsSyncSession &session,
                                 UINT32 &sub,
                                 BOOLEAN &needWait,
                                 BOOLEAN &hasJump )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__JUMP ) ;
      INT32 rc = SDB_OK ;

      needWait = FALSE ;

      while ( _validSync > sub )
      {
         _mtxs[++sub].get() ;
         if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
              session.waitPlan.isPassed( _checkList[sub] ) )
         {
            needWait = FALSE ;
            _mtxs[sub].release() ;
            break ;
         }
         else if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
                   session.waitPlan.offset < _checkList[sub].offset )
         {
            _mtxs[sub].release() ;
            // if it's last check list, session will not need to sync
            if ( _validSync - 1 == sub )
            {
               needWait = FALSE ;
               break ;
            }
         }
         else
         {
            PD_LOG( PDDEBUG, "Session[LSN:%llu, ID:%lld] has jumped to "
                    "check list[sub:%d]", session.waitPlan.offset,
                    session.eduCB->getID(), sub ) ;
            rc = _syncList[sub].push( session ) ;
            _mtxs[sub].release() ;
            needWait = TRUE ;
            hasJump = TRUE ;
            break ;
         }
      }

      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG__JUMP, rc ) ;
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

      /// loop every removed synclist
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
            /// compute w's completion
            for ( UINT32 j = 0; j < preSyncNum - 1 ; ++j )
            {
               if ( session.waitPlan.offset < left[j].offset )
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
            /// push the session which is not completed
            /// into the newlist.
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
