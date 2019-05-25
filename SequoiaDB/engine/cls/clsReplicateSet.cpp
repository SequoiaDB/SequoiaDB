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

   Source File Name = clsReplicateSet.hpp

   Descriptive Name = Replication Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsReplicateSet.hpp"
#include "netRouteAgent.hpp"
#include "clsUtil.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "clsMgr.hpp"
#include "clsFSSrcSession.hpp"
#include "pmdStartup.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   BEGIN_OBJ_MSG_MAP( _clsReplicateSet, _pmdObjBase )
      ON_MSG( MSG_CAT_GRP_RES, handleMsg )
      ON_MSG( MSG_CLS_BEAT, handleMsg )
      ON_MSG( MSG_CLS_BEAT_RES, handleMsg )
      ON_MSG( MSG_CLS_BALLOT, handleMsg )
      ON_MSG( MSG_CLS_BALLOT_RES, handleMsg )
      ON_MSG( MSG_CAT_PAIMARY_CHANGE_RES, handleMsg )
      ON_MSG( MSG_CLS_GINFO_UPDATED, handleMsg )
      ON_MSG( MSG_CLS_NODE_STATUS_NOTIFY, handleMsg )
      ON_EVENT( PMD_EDU_EVENT_STEP_DOWN, handleEvent )
      ON_EVENT( PMD_EDU_EVENT_STEP_UP, handleEvent )
   END_OBJ_MSG_MAP ()

   const UINT32 CLS_REPL_SEC_TIME = 1000 ;

   #define CLS_REPL_ACTIVE_CHECK( rc ) \
            do { \
               if ( !_active ) \
               { \
                  rc = SDB_REPL_GROUP_NOT_ACTIVE ;\
                  goto error ;\
               } \
            } while( 0 )

#if defined (_WINDOWS)
   #define CLS_CONNREFUSED    WSAECONNREFUSED
#else
   #define CLS_CONNREFUSED    ECONNREFUSED
#endif //_WINDOWS

   #define CLS_SYNCCTRL_BASE_TIME         (10)

   _clsReplicateSet::_clsReplicateSet( _netRouteAgent *agent )
   : _agent( agent ),
     _vote( &_info, _agent),
     _logger( NULL ),
     _sync( _agent, &_info ),
     _reelection( &_vote, &_sync ),
     _clsCB( NULL ),
     _timerID( CLS_INVALID_TIMERID ),
     _beatTime( 0 ),
     _active( FALSE )
   {
      _srcSessionNum = 0 ;
      _ntyLastOffset = DPS_INVALID_LSN_OFFSET ;
      _ntyProcessedOffset = DPS_INVALID_LSN_OFFSET ;

      _totalLogSize = 0 ;
      _inSyncCtrl   = FALSE ;
      _lastTimerTick = 0 ;
      memset( _sizethreshold, 0, sizeof( _sizethreshold ) ) ;
      memset( _timeThreshold, 0, sizeof( _timeThreshold ) ) ;

      _faultEvent.reset() ;
      _syncEmptyEvent.signal() ;
   }

   _clsReplicateSet::~_clsReplicateSet()
   {

   }

   void _clsReplicateSet::onWriteLog( DPS_LSN_OFFSET offset )
   {
      _sync.notify( offset ) ;
   }

   void _clsReplicateSet::onPrepareLog( UINT32 csLID, UINT32 clLID,
                                        INT32 extLID, DPS_LSN_OFFSET offset )
   {
      if ( getNtySessionNum() > 0 )
      {
         _ntyLastOffset = offset ;
         _ntyQue.push( clsLSNNtyInfo( csLID, clLID, extLID ,offset ) ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPPSET_NOTIFY2SESSION, "_clsReplicateSet::notify2Session" )
   void _clsReplicateSet::notify2Session( UINT32 suLID, UINT32 clLID,
                                          dmsExtentID extLID,
                                          const DPS_LSN_OFFSET & offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPPSET_NOTIFY2SESSION );
      if ( _srcSessionNum > 0 )
      {
         UINT32 index = 0 ;
         _vecLatch.lock_r () ;
         while ( index < _srcSessionNum )
         {
            _vecSrcSessions[index]->notifyLSN ( suLID, clLID, extLID, offset ) ;
            ++index ;
         }
         _vecLatch.release_r () ;
      }
      _ntyProcessedOffset = offset ;

      PD_TRACE_EXIT ( SDB__CLSREPPSET_NOTIFY2SESSION );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_REGSN, "_clsReplicateSet::regSession" )
   void _clsReplicateSet::regSession ( _clsDataSrcBaseSession * pSession )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_REGSN );
      SDB_ASSERT ( pSession, "Session can't be null" ) ;

      _vecLatch.lock_w () ;
      _srcSessionNum++ ;
      _vecSrcSessions.push_back ( pSession ) ;
      _vecLatch.release_w () ;
      PD_TRACE_EXIT ( SDB__CLSREPSET_REGSN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_UNREGSN, "_clsReplicateSet::unregSession" )
   void _clsReplicateSet::unregSession ( _clsDataSrcBaseSession * pSession )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_UNREGSN );
      SDB_ASSERT ( pSession, "Session can't be null" ) ;

      _vecLatch.lock_w () ;
      std::vector<_clsDataSrcBaseSession*>::iterator it =
         _vecSrcSessions.begin() ;
      while ( it != _vecSrcSessions.end() )
      {
         if ( *it == pSession )
         {
            _vecSrcSessions.erase ( it ) ;
            _srcSessionNum-- ;
            break ;
         }
         ++it ;
      }
      _vecLatch.release_w () ;
      PD_TRACE_EXIT ( SDB__CLSREPSET_UNREGSN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_INIT, "_clsReplicateSet::initialize" )
   INT32 _clsReplicateSet::initialize ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_INIT ) ;

      if ( !_agent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      g_startShiftTime = (INT32)pmdGetOptionCB()->startShiftTime() ;

      _logger = pmdGetKRCB()->getDPSCB() ;
      _clsCB = pmdGetKRCB()->getClsCB() ;
      SDB_ASSERT( NULL != _logger, "logger should not be NULL" ) ;

      _logger->regEventHandler( this ) ;

      rc = _replBucket.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init repl bucket failed, rc: %d", rc ) ;

      _totalLogSize = (UINT64)pmdGetOptionCB()->getReplLogFileSz()*
                      (UINT64)pmdGetOptionCB()->getReplLogFileNum() ;
      {
         UINT32 rate = 2 ;
         UINT32 timeBase = CLS_SYNCCTRL_BASE_TIME ;

         for ( UINT32 idx = 0 ; idx < CLS_SYNCCTRL_THRESHOLD_SIZE ; ++idx )
         {
            rate = 2 << idx ;
            _sizethreshold[ idx ] = _totalLogSize * ( rate - 1 ) / rate ;
            _timeThreshold[ idx ] = timeBase << idx ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplicateSet::deactive ()
   {
      SDB_ASSERT( PMD_IS_DB_DOWN(), "DB must be down" ) ;

      _syncEmptyEvent.wait() ;

      if ( _replBucket.maxReplSync() > 0 )
      {
         PD_LOG( PDEVENT, "Begin to wait repl bucket empty[bucket size: %d, "
                 "all size: %d, agent number: %d]", _replBucket.bucketSize(),
                 _replBucket.size(), _replBucket.curAgentNum() ) ;

         _replBucket.waitEmpty() ;

         PD_LOG( PDEVENT, "Wait repl bucket empty completed" ) ;
      }

      return SDB_OK ;
   }

   INT32 _clsReplicateSet::final ()
   {
      _replBucket.fini() ;
      if ( _logger )
      {
         _logger->unregEventHandler( this ) ;
      }
      return SDB_OK ;
   }

   void _clsReplicateSet::onConfigChange ()
   {
      if ( pmdGetOptionCB()->maxReplSync() != getBucket()->maxReplSync() )
      {
         _sync.enableSync( FALSE ) ;
         _syncEmptyEvent.wait() ;
         getBucket()->enforceMaxReplSync( pmdGetOptionCB()->maxReplSync() ) ;
         _sync.enableSync( TRUE ) ;
      }
      if ( g_startShiftTime >= 0 )
      {
         g_startShiftTime = (INT32)pmdGetOptionCB()->startShiftTime() ;
      }
   }

   void _clsReplicateSet::ntyPrimaryChange( BOOLEAN primary,
                                            SDB_EVENT_OCCUR_TYPE type )
   {
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         _replBucket.reset() ;
      }
      else if ( !primary && SDB_EVT_OCCUR_AFTER == type )
      {
         _sync.cut( 0 ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_ACTIVE, "_clsReplicateSet::active" )
   INT32 _clsReplicateSet::active()
   {
      INT32 rc      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_ACTIVE );
      if ( _active )
      {
         goto done ;
      }

      {
         _MsgRouteID id = _clsCB->getNodeID() ;
         id.columns.serviceID = _clsCB->getReplServiceID() ;
         setLocalID( id ) ;
         _MsgCatGroupReq msg ;
         msg.id = _info.local ;
         _cata.call( (MsgHeader *)(&msg) ) ;
         _timerID = _clsCB->setTimer( CLS_REPL, CLS_REPL_SEC_TIME ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_ACTIVE, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__SETGPSET, "_clsReplicateSet::_setGroupSet" )
   INT32 _clsReplicateSet::_setGroupSet( const CLS_GROUP_VERSION &version,
                                         map<UINT64, _netRouteNode> &nodes,
                                         BOOLEAN &changeStatus )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET__SETGPSET ) ;
      BOOLEAN hasLocal = FALSE ;
      std::map<UINT64, _netRouteNode>::iterator itr ;
      std::map<UINT64, _clsSharingStatus>::iterator itr2 ;
      changeStatus = FALSE ;

      if ( version <= _info.version )
      {
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
         goto error ;
      }

      if ( CLS_REPLSET_MAX_NODE_SIZE  < nodes.size() )
      {
         rc = SDB_CLS_INVALID_GROUP_NUM ;
         PD_LOG( PDWARNING, "invalid group size : %d",
                 nodes.size() ) ;
         goto error ;
      }

      _info.version = version ;

      if ( SPARE_GROUPID == _info.local.columns.groupID )
      {
         hasLocal = TRUE ;
         nodes.clear() ;
      }

      itr = nodes.begin() ;
      for ( ; itr != nodes.end(); itr++ )
      {
        if ( itr->first == _info.local.value )
        {
           hasLocal = TRUE ;
           continue ;
        }
        else if ( !itr->second._isActive )
        {
           if ( g_startShiftTime < 0 )
           {
              continue ;
           }
           itr->second._isActive = TRUE ;
           changeStatus = TRUE ;
        }
        if ( SDB_OK == _agent->updateRoute( itr->second._id,
                                            itr->second ) )
        {
           _info.mtx.lock_w() ;
           _clsGroupBeat &beat = (_info.info[itr->first]).beat ;
           _info.mtx.release_w() ;
           beat.identity = itr->second._id ;
           beat.beatID = 0 ;
           _alive( itr->second._id ) ;
           PD_LOG( PDEVENT, "add node [%s:%s]",
                   itr->second._host, itr->second._service[0].c_str() ) ;
        }
      } // for ( ; itr != nodes.end(); itr++ )

      if ( !hasLocal )
      {
         PD_LOG( PDERROR, "local node is not in the cluster!" ) ;
         PMD_RESTART_DB( SDB_SYS ) ;
         goto done ;
      }

      itr2 = _info.info.begin() ;
      for ( ; itr2 != _info.info.end(); )
      {
         itr = nodes.find( itr2->first ) ;
         if ( nodes.end() == itr || FALSE == itr->second._isActive )
         {
            if ( itr2->first == _info.primary.value )
            {
               _info.primary.value = 0 ;
            }
            MsgRouteID tmp ;
            tmp.value = itr2->first ;
            PD_LOG( PDEVENT, "erase node[%d,%d]",
                    tmp.columns.groupID, tmp.columns.nodeID ) ;
            _info.mtx.lock_w() ;
            _info.alives.erase( itr2->first ) ;
            _info.info.erase( itr2++ ) ;
            _info.mtx.release_w() ;
         }
         else
         {
            ++itr2 ;
         }
      } // for ( ; itr2 != _info.info.end(); itr2++ )

      _sync.updateNotifyList( TRUE ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET__SETGPSET, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplicateSet::callCatalog( MsgHeader *header, UINT32 times )
   {
      return _cata.call( header, times ) ;
   }

   ossEvent* _clsReplicateSet::getFaultEvent()
   {
      return &_faultEvent ;
   }

   ossEvent* _clsReplicateSet::getSyncEmptyEvent()
   {
      return &_syncEmptyEvent ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_GETPRMY, "_clsReplicateSet::getPrimary" )
   MsgRouteID _clsReplicateSet::getPrimary ()
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_GETPRMY );
      _MsgRouteID primary ;
      _info.mtx.lock_r () ;
      primary = _info.primary ;
      _info.mtx.release_r () ;

      PD_TRACE_EXIT ( SDB__CLSREPSET_GETPRMY );
      return primary ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_ISSENDNORMAL, "_clsReplicateSet::isSendNormal" )
   BOOLEAN _clsReplicateSet::isSendNormal( UINT64 nodeID )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_ISSENDNORMAL );
      _info.mtx.lock_r() ;
      BOOLEAN isNormal = _info.getNodeSendFailedTimes( nodeID ) == 0 ?
                         TRUE : FALSE ;
      _info.mtx.release_r() ;
      PD_TRACE_EXIT ( SDB__CLSREPSET_ISSENDNORMAL );
      return isNormal ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_GETPRIMARYINFO, "_clsReplicateSet::getPrimaryInfo" )
   BOOLEAN _clsReplicateSet::getPrimaryInfo( _clsSharingStatus &primaryInfo )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_GETPRIMARYINFO );
      BOOLEAN isOk = FALSE ;
      _MsgRouteID primary ;

      ossScopedRWLock lock( &_info.mtx, SHARED ) ;

      primary = _info.primary ;
      map<UINT64, _clsSharingStatus>::iterator itr =
                                             _info.info.find( primary.value ) ;
      if ( itr != _info.info.end() )
      {
         primaryInfo = itr->second ;
         isOk = TRUE ;
      }

      PD_TRACE_EXIT ( SDB__CLSREPSET_GETPRIMARYINFO );
      return isOk ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_GETGPINFO, "_clsReplicateSet::getGroupInfo" )
   void _clsReplicateSet::getGroupInfo( _MsgRouteID &primary,
                                        vector<_netRouteNode> &group )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_GETGPINFO ) ;

      ossScopedRWLock lock( &_info.mtx, SHARED ) ;

      map<UINT64, _clsSharingStatus>::const_iterator itr =
                                          _info.info.begin() ;
      INT32 rc = SDB_OK ;
      _netRouteNode node ;
      _MsgRouteID id ;
      primary = _info.primary ;
      for ( ; itr != _info.info.end(); itr++ )
      {
         id.value = itr->first ;
         rc = _agent->route( id, node ) ;
         SDB_ASSERT( SDB_OK == rc, "impossible" ) ;
         if ( SDB_OK == rc )
         {
            group.push_back( node ) ;
         }
         else
         {
            PD_LOG( PDERROR, "group info is not match route table." ) ;
         }
      }
      id = _info.local ;
      rc = _agent->route( id, node ) ;
      if ( SDB_OK == rc )
      {
         group.push_back( node ) ;
      }
      else
      {
         PD_LOG( PDERROR, "group info is not match route table." ) ;
         SDB_ASSERT( false, "impossible" ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSREPSET_GETGPINFO );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_ONTMR, "_clsReplicateSet::onTimer" )
   void _clsReplicateSet::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_ONTMR );
      if ( _timerID == timerID )
      {
         UINT64 timeSpan = pmdGetTickSpanTime( _lastTimerTick ) ;
         if ( timeSpan < interval / 2 )
         {
            goto done ;
         }
         else if ( 0 != _lastTimerTick && timeSpan > 3 * interval )
         {
            PD_LOG( PDWARNING, "The %u milli-seconds's timer has %u "
                    "milli-seconds not called, the cluster's main thread "
                    "maybe blocked in some operations", interval,
                    timeSpan ) ;
         }
         _lastTimerTick = pmdGetDBTick() ;

         _cata.handleTimeout( interval ) ;
         if ( !_active )
         {
            goto done ;
         }

         _beatTime += interval ;
         if ( CLS_SHARING_BETA_INTERVAL <= _beatTime )
         {
            _sharingBeat() ;
            _beatTime = 0 ;
         }

         _checkBreak( interval ) ;

         _vote.handleTimeout( interval ) ;
         _sync.handleTimeout( interval ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSREPSET_ONTMR );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_HNDEVENT, "_clsReplicateSet::handleEvent" )
   INT32 _clsReplicateSet::handleEvent( pmdEDUEvent *event )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_HNDEVENT ) ;
      if ( PMD_EDU_EVENT_STEP_DOWN == event->_eventType )
      {
         rc = _handleStepDown() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to step down:%d", rc ) ;
            goto error ;
         }
      }
      else if ( PMD_EDU_EVENT_STEP_UP == event->_eventType )
      {
         rc = _handleStepUp( event->_userData ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to step up:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "unknown event type:%d", event->_eventType ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET_HNDEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_HNDMSG, "_clsReplicateSet::handleMsg" )
   INT32 _clsReplicateSet::handleMsg( NET_HANDLE handle, MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_HNDMSG ) ;
      switch ( msg->opCode )
      {
         case MSG_CAT_GRP_RES:
         {
            rc = _handleGroupRes( (const MsgCatGroupRes *)msg ) ;
            break ;
         }
         case MSG_CLS_BEAT :
         {
            CLS_REPL_ACTIVE_CHECK( rc ) ;
            rc = _handleSharingBeat( handle, ( const _MsgClsBeat *)msg ) ;
            break ;
         }
         case MSG_CAT_PAIMARY_CHANGE_RES:
         {
            INT32 result = MSG_GET_INNER_REPLY_RC( msg ) ;
            if ( SDB_CLS_NOT_PRIMARY == result )
            {
               shardCB *pShardCB = _clsCB->getShardCB() ;
               if ( SDB_OK != pShardCB->updatePrimaryByReply( msg ) )
               {
                  pShardCB->updateCatGroup () ;
               }
            }
            else if ( SDB_OK == result )
            {
               _cata.remove( msg, result ) ;
            }
            break ;
         }
         case MSG_CLS_BEAT_RES :
         {
            CLS_REPL_ACTIVE_CHECK( rc ) ;
            rc = _handleSharingBeatRes( ( const _MsgClsBeatRes *)msg ) ;
            break ;
         }
         case MSG_CLS_BALLOT :
         {
            CLS_REPL_ACTIVE_CHECK( rc ) ;
            rc = _vote.handleInput( msg ) ;
            break ;
         }
         case MSG_CLS_BALLOT_RES :
         {
            CLS_REPL_ACTIVE_CHECK( rc ) ;
            rc = _vote.handleInput( msg ) ;
            break ;
         }
         case MSG_CLS_GINFO_UPDATED :
         {
            PD_LOG( PDEVENT, "Group info has been updated, download again" ) ;
            MsgCatGroupReq msg ;
            msg.id = _info.local ;
            _cata.call( (MsgHeader *)(&msg) ) ;
            break ;
         }
         case MSG_CLS_NODE_STATUS_NOTIFY :
         {
            MsgClsNodeStatusNotify *pNty = ( MsgClsNodeStatusNotify* )msg ;
            if ( SDB_DB_FULLSYNC == pNty->status )
            {
               _sync.notifyFullSync( msg->routeID ) ;
            }
            break ;
         }
         default :
         {
            PD_LOG( PDWARNING, "unknown msg: %s", msg2String( msg ).c_str() ) ;
            rc = SDB_CLS_UNKNOW_MSG ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_HNDMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__HNDGPRES, "_clsReplicateSet::_handleGroupRes" )
   INT32 _clsReplicateSet::_handleGroupRes( const MsgCatGroupRes *msg )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *pHeader = ( MsgHeader* )msg ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__HNDGPRES );

      CLS_GROUP_VERSION version ;
      map<UINT64, _netRouteNode> group ;
      string groupName ;
      BOOLEAN changeStatus = FALSE ;
      UINT32 grpHashCode = 0 ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(pHeader) )
      {
         if ( SDB_CLS_NOT_PRIMARY == MSG_GET_INNER_REPLY_RC(pHeader) )
         {
            shardCB *pShardCB = _clsCB->getShardCB() ;
            if ( SDB_OK != pShardCB->updatePrimaryByReply( pHeader ) )
            {
               pShardCB->updateCatGroup() ;
            }
         }

         PD_LOG( PDWARNING, "Download group info request was refused from "
                 "node[%s], rc: %d",
                 routeID2String( pHeader->routeID ).c_str(),
                 MSG_GET_INNER_REPLY_RC(pHeader) ) ;
         goto error ;
      }

      rc = msgParseCatGroupRes( msg, version, groupName, group,
                                NULL, &grpHashCode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "parse MsgCatGroupRes err, rc = %d", rc ) ;
         goto error ;
      }

      rc = _setGroupSet( version, group, changeStatus ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_REPL_REMOTE_G_V_EXPIRED != rc )
         {
            PD_LOG( PDWARNING, "set group info failed, rc = %d", rc ) ;
            goto error ;
         }
         rc = SDB_OK ;
      }
      _info.setHashCode( grpHashCode ) ;

      if ( !changeStatus )
      {
         _cata.remove( &(msg->header), MSG_GET_INNER_REPLY_RC(pHeader) ) ;
      }

      pmdGetKRCB()->setGroupName ( groupName.c_str() ) ;
      if ( !_active )
      {
         PD_LOG( PDEVENT, "download group info successfully" ) ;

         _clsCB->startInnerSession ( CLS_REPL, CLS_TID_REPL_SYC ) ;

         _active = TRUE ;
         _vote.init() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSREPSET__HNDGPRES, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__SHRBEAT, "_clsReplicateSet::_sharingBeat" )
   void _clsReplicateSet::_sharingBeat()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__SHRBEAT ) ;

      if ( _info.info.empty() )
      {
         goto done ;
      }
      else
      {
         DPS_LSN fBegin ;
         DPS_LSN mBegin ;
         DPS_LSN end ;
         DPS_LSN expectLSN ;
         _logger->getLsnWindow( fBegin, mBegin, end, &expectLSN, NULL ) ;
         _MsgClsBeat msg ;
         msg.beat.identity = _info.local ;
         msg.beat.endLsn = expectLSN ;
         msg.beat.version = _info.version ;
         *(UINT32*)msg.beat.hashCode = _info.getHashCode() ;
         msg.beat.role = _vote.primaryIsMe() ?
                         CLS_GROUP_ROLE_PRIMARY : CLS_GROUP_ROLE_SECONDARY ;
         msg.beat.beatID = _info.nextBeatID() ;
         msg.header.requestID = msg.beat.beatID ;
         msg.beat.serviceStatus = pmdGetStartup().isOK() ?
                                  SERVICE_NORMAL : SERVICE_ABNORMAL ;
         UINT8 weight = pmdGetOptionCB()->weight() ;
         UINT8 shadowWeight = _vote.getShadowWeight() ;
         msg.beat.weight = CLS_GET_WEIGHT( weight, shadowWeight ) ;

         map<UINT64, _clsSharingStatus>::iterator itr = _info.info.begin() ;
         for ( ; itr != _info.info.end(); itr++ )
         {
            if ( itr->second.deadtime >= pmdGetOptionCB()->sharingBreakTime() &&
                 itr->second.deadtime >= _beatTime )
            {
               itr->second.deadtime -= _beatTime ;
               continue ;
            }
            msg.beat.syncStatus = clsSyncWindow( itr->second.beat.endLsn,
                                                 fBegin, mBegin, expectLSN ) ;

            rc = _agent->syncSend( itr->second.beat.identity, &msg ) ;
            if ( SDB_OK == rc )
            {
               itr->second.sendFailedTimes = 0 ;
            }
            else
            {
               INT32 sysErr = SOCKET_GETLASTERROR ;

               if ( sysErr == CLS_CONNREFUSED )
               {
                  ++( itr->second.sendFailedTimes ) ;
               }

               if ( _info.alives.find( itr->first ) == _info.alives.end() )
               {
                  UINT32 resetTimeout = 0 ;
                  itr->second.deadtime = pmdGetOptionCB()->sharingBreakTime() - 1 ;
                  if ( sysErr == CLS_CONNREFUSED )
                  {
                     resetTimeout = 1800 * OSS_ONE_SEC ;
                  }
                  else
                  {
                     resetTimeout = 120 * OSS_ONE_SEC ;
                  }
                  itr->second.deadtime += resetTimeout ;

                  PD_LOG( PDEVENT, "Reset node[%d] sharing-beat time to %u(sec)",
                          itr->second.beat.identity.columns.nodeID,
                          resetTimeout / OSS_ONE_SEC ) ;
               }
            }
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSREPSET__SHRBEAT );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__CHKBRK, "_clsReplicateSet::_checkBreak" )
   void _clsReplicateSet::_checkBreak( const UINT32 &millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET__CHKBRK );

      BOOLEAN needErase = FALSE ;
      map<UINT64, _clsSharingStatus *>::iterator itr ;
      map< UINT64, _clsSharingStatus>::iterator itrInfo ;

      for ( itr = _info.alives.begin() ; itr != _info.alives.end() ; itr++ )
      {
         itr->second->timeout += millisec ;
         if ( pmdGetOptionCB()->sharingBreakTime() <= itr->second->timeout )
         {
            needErase = TRUE ;
         }
      }

      for ( itrInfo = _info.info.begin() ; itrInfo != _info.info.end() ;
            ++itrInfo )
      {
         if ( _info.alives.find( itrInfo->first ) != _info.alives.end() )
         {
            continue ;
         }
         itrInfo->second.breakTime += millisec ;
      }

      if ( !needErase )
      {
         goto done ;
      }

      _info.mtx.lock_w() ;
      itr = _info.alives.begin() ;
      for ( ; itr != _info.alives.end(); )
      {
         if ( pmdGetOptionCB()->sharingBreakTime() <= itr->second->timeout )
         {
            if ( itr->first == _info.primary.value )
            {
               PD_LOG( PDERROR, "vote: primary [node:%d] break",
                       _info.primary.columns.nodeID ) ;
               _info.primary.value = MSG_INVALID_ROUTEID ;
            }
            PD_LOG( PDERROR, "vote: [node:%d] alive break",
                    itr->second->beat.identity.columns.nodeID ) ;
            itr->second->beat.beatID = CLS_BEATID_INVALID ;
            itr->second->beat.serviceStatus = SERVICE_UNKNOWN ;

            _sync.updateNodeStatus( itr->second->beat.identity, FALSE ) ;

            _info.alives.erase( itr++ ) ;
         }
         else
         {
            ++itr ;
         }
      }
      _info.mtx.release_w() ;
      if ( _vote.primaryIsMe() )
      {
         _sync.cut( _info.alives.size() ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSREPSET__CHKBRK );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__HNDSHRBEAT, "_clsReplicateSet::_handleSharingBeat" )
   INT32 _clsReplicateSet::_handleSharingBeat( NET_HANDLE handle,
                                               const _MsgClsBeat *msg )
   {
      SDB_ASSERT( NULL != msg, "msg should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__HNDSHRBEAT );
      const _clsGroupBeat &beat = msg->beat ;
      map<UINT64, _clsSharingStatus>::iterator itr=
                     _info.info.find( beat.identity.value ) ;
      if ( *(UINT32*)beat.hashCode != _info.getHashCode() ||
          ( _info.info.end() == itr && beat.version <= _info.version ) )
      {
         PD_LOG( PDINFO, "Beat hashCode[%u] is not the same with self[%u] or "
                 "node[%s] is not found in group information",
                 *(UINT32*)beat.hashCode, _info.getHashCode(),
                 routeID2String( beat.identity ).c_str() ) ;
         rc = SDB_REPL_INVALID_GROUP_MEMBER ;
         goto error ;
      }

      if ( beat.version > _info.version )
      {
         rc = SDB_REPL_LOCAL_G_V_EXPIRED ;
         _MsgCatGroupReq msg ;
         msg.id = _info.local ;
         _cata.call( (MsgHeader *)(&msg) ) ;
      }
      else if ( itr != _info.info.end() )
      {
         itr->second.beat = beat ;
         if ( CLS_GROUP_ROLE_PRIMARY == beat.role )
         {
            g_startShiftTime = -1 ; // have primary node

            if ( _vote.primaryIsMe() )
            {
               DPS_LSN lsn  = _logger->expectLsn() ;
               if ( 0 >= lsn.compare( beat.endLsn ) )
               {
                  _info.mtx.lock_w() ;
                  _info.primary = beat.identity ;
                  _info.mtx.release_w() ;
                  _vote.force( CLS_ELECTION_STATUS_SILENCE ) ;
                  PD_LOG( PDEVENT, "vote:remote lsn[%d:%lld]"
                          " higher(or equal) than local lsn[%d:%lld],"
                          " we change to secondary.",
                          beat.endLsn.version, beat.endLsn.offset,
                          lsn.version, lsn.offset ) ;
               }
            }
            else if ( _info.primary.value != beat.identity.value )
            {
               PD_LOG( PDEVENT, "vote: the discovery of new primary[%d]",
                       beat.identity.columns.nodeID ) ;
               _cata.remove( MSG_CAT_PAIMARY_CHANGE_RES ) ;
               _vote.force( CLS_ELECTION_STATUS_SILENCE ) ;
               _info.mtx.lock_w() ;
               _info.primary = beat.identity ;
               _info.mtx.release_w() ;
            }

            if ( CLS_ELECTION_WEIGHT_USR_MIN != _vote.getShadowWeight() )
            {
               reelectionDone() ;
            }
         }
         else
         {
            if ( _info.primary.value == beat.identity.value )
            {
               PD_LOG( PDEVENT, "vote: primary node[%d] is down",
                       beat.identity.columns.nodeID ) ;
               _cata.remove( MSG_CAT_PAIMARY_CHANGE_RES ) ;
               _info.mtx.lock_w() ;
               _info.primary.value = MSG_INVALID_ROUTEID ;
               _info.mtx.release_w() ;
            }
         }
      }
      {
         _alive( beat.identity ) ;
         _MsgClsBeatRes res ;
         res.header.header.requestID = msg->header.requestID ;
         res.identity = _info.local ;
         _agent->syncSend( handle, &res ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET__HNDSHRBEAT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplicateSet::_handleSharingBeatRes( const _MsgClsBeatRes *msg )
   {
      SDB_ASSERT( NULL != msg, "msg should not be NULL" ) ;
      return _alive( msg->identity ) ;
   }

   INT32 _clsReplicateSet::aliveNode( const MsgRouteID &id )
   {
      INT32 rc = SDB_OK ;

      rc = _info.mtx.lock_r( 100 ) ;

      if ( SDB_OK == rc )
      {
         map<UINT64, _clsSharingStatus*>::iterator itr =
            _info.alives.find( id.value ) ;
         if ( itr != _info.alives.end() )
         {
            itr->second->timeout = 0 ;
            itr->second->breakTime = 0 ;
            itr->second->deadtime = 0 ;
            itr->second->sendFailedTimes = 0 ;
         }
         else
         {
            rc = SDB_CLS_NODE_BSFAULT ;
         }
         _info.mtx.release_r() ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__ALIVE, "_clsReplicateSet::_alive" )
   INT32 _clsReplicateSet::_alive( const _MsgRouteID &id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__ALIVE );
      map<UINT64, _clsSharingStatus>::iterator itr=
                     _info.info.find( id.value ) ;
      if ( _info.info.end() == itr )
      {
         rc = SDB_REPL_INVALID_GROUP_MEMBER ;
         goto error ;
      }
      if ( _info.alives.end() == _info.alives.find( itr->first ) )
      {
         _clsSharingStatus &status = itr->second ;
         _info.mtx.lock_w() ;
         _info.alives.insert( make_pair( itr->first, &status ) ) ;
         _sync.updateNodeStatus( status.beat.identity, TRUE ) ;
         _info.mtx.release_w() ;

         PD_LOG( PDEVENT, "vote: [node:%d] aliving from break",
                 status.beat.identity.columns.nodeID ) ;
      }
      itr->second.timeout = 0 ;
      itr->second.breakTime = 0 ;
      itr->second.deadtime = 0 ;
      itr->second.sendFailedTimes = 0 ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET__ALIVE, rc );
      return rc ;
   error:
      goto done ;
   }

   UINT32 _clsReplicateSet::_getThresholdTime( UINT64 diffSize )
   {
      UINT32 i = 0 ;
      UINT32 threshTime = 0 ;

      for ( ; i < CLS_SYNCCTRL_THRESHOLD_SIZE ; ++i )
      {
         if ( diffSize < _sizethreshold[ i ] )
         {
            break ;
         }
      }
      if ( i > 1 || ( 1 == i && _inSyncCtrl ) )
      {
         threshTime = _timeThreshold[ i - 1 ] ;
      }
      return threshTime ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__CANASSIGNLOGPAGE, "_clsReplicateSet::canAssignLogPage" )
   INT32 _clsReplicateSet::canAssignLogPage( UINT32 reqLen, pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET__CANASSIGNLOGPAGE );
      INT32 rc = SDB_OK ;

      UINT32 threshTime = 0 ;
      UINT32 waitTime = 0 ;
      DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN expectLSN ;

      while ( SDB_OK == rc && PMD_IS_DB_AVAILABLE() )
      {
         offset = _sync.getSyncCtrlArbitLSN() ;
         if ( DPS_INVALID_LSN_OFFSET == offset )
         {
            break ;
         }

         expectLSN = _logger->expectLsn() ;
         if ( offset >= expectLSN.offset )
         {
            goto done ;
         }

         threshTime = _getThresholdTime( expectLSN.offset - offset ) ;
         if ( 0 == threshTime )
         {
            goto done ;
         }

         expectLSN.offset += reqLen ;
         if ( ( expectLSN.offset > offset + _logger->getLogFileSz() &&
                _logger->calcFileID( expectLSN.offset ) ==
                _logger->calcFileID( offset ) ) ||
              ( waitTime < threshTime ) )
         {
            if ( !_inSyncCtrl )
            {
               _inSyncCtrl = TRUE ;
               pmdGetKRCB()->setFlowControl( TRUE ) ;
               PD_LOG( PDWARNING, "Begin sync control...[expectLSN: %lld, "
                       "ArbitLSN: %lld, threshTime: %d, reqLen: %d, "
                       "waitTime: %d]", expectLSN.offset, offset,
                       threshTime, reqLen, waitTime ) ;
            }
            ossSleep( CLS_SYNCCTRL_BASE_TIME ) ;
            waitTime += CLS_SYNCCTRL_BASE_TIME ;
         }
         else
         {
            break ;
         }

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
         }
         else if ( !_vote.primaryIsMe() )
         {
            rc = SDB_CLS_NOT_PRIMARY ;
         }
      }

   done:
      if ( 0 == waitTime && _inSyncCtrl )
      {
         _inSyncCtrl = FALSE ;
         pmdGetKRCB()->setFlowControl( FALSE ) ;
         PD_LOG( PDWARNING, "End sync control" ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREPSET__CANASSIGNLOGPAGE, rc );
      return rc ;
   }

   INT64 _clsReplicateSet::netIn()
   {
      return _agent->netIn() ;
   }

   INT64 _clsReplicateSet::netOut()
   {
      return _agent->netOut() ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET_REELECT, "_clsReplicateSet::reelect" )
   INT32 _clsReplicateSet::reelect( CLS_REELECTION_LEVEL lvl,
                                    UINT32 seconds,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_REELECT ) ;
      if ( 1 == groupSize() )
      {
         goto done ;
      }

      rc = _reelection.run( lvl, seconds, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to reelect:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET_REELECT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsReplicateSet::reelectionDone()
   {
      _vote.setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
      _reelection.signal() ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__HANDLESTEPDOWN, "_clsReplicateSet::_handleStepDown" )
   INT32 _clsReplicateSet::_handleStepDown()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET__HANDLESTEPDOWN ) ;
      _vote.setShadowWeight( CLS_ELECTION_WEIGHT_MIN ) ;
      _vote.force( CLS_ELECTION_STATUS_SEC ) ;
      PD_TRACE_EXITRC( SDB__CLSREPSET__HANDLESTEPDOWN, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__HANDLESTEPUP, "_clsReplicateSet::_handleStepUp" )
   INT32 _clsReplicateSet::_handleStepUp( UINT32 seconds )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET__HANDLESTEPUP ) ;
      PD_LOG(PDEVENT, "force to step up, seconds:%d", seconds ) ;
      _vote.force( CLS_ELECTION_STATUS_PRIMARY,
                   seconds * 1000 ) ;
      PD_TRACE_EXITRC( SDB__CLSREPSET__HANDLESTEPUP, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__STEPUP, "_clsReplicateSet::stepUp" )
   INT32 _clsReplicateSet::stepUp( UINT32 seconds,
                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET__STEPUP ) ;
      DPS_LSN lsn ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ) ;

      if ( MSG_INVALID_ROUTEID != getPrimary().value )
      {
         PD_LOG( PDERROR, "can not step up when primary node"
                 " exists" ) ;
         rc = SDB_CLS_CAN_NOT_STEP_UP ;
         goto error ;
      }
      else if ( !_active )
      {
         rc = SDB_CLS_NODE_INFO_EXPIRED ;
         PD_LOG( PDERROR, "can not step up before local's node download group info" ) ;
         goto error ;
      }

      lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
      if ( _sync.atLeastOne( lsn.offset ) )
      {
         PD_LOG( PDERROR, "can not step up when other nodes' lsn"
                 " bigger than local's" ) ;
         rc = SDB_CLS_CAN_NOT_STEP_UP ;
         goto error ;
      }

      rc = eduMgr->postEDUPost( eduID,
                                PMD_EDU_EVENT_STEP_UP,
                                PMD_EDU_MEM_NONE,
                                NULL, seconds ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to post event to repl cb:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET__STEPUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET_PRIMARYCHECK, "_clsReplicateSet::primaryCheck" )
   INT32 _clsReplicateSet::primaryCheck( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_PRIMARYCHECK ) ;
      rc = _reelection.wait( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to wait:%d", rc ) ;
         goto error ;
      }
      else if ( !primaryIsMe () )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }
      else
      {
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET_PRIMARYCHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

