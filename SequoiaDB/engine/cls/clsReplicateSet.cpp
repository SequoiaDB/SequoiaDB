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
#include "pmdController.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   BEGIN_OBJ_MSG_MAP( _clsReplicateSet, _pmdObjBase )
      //ON_MSG ( )
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

   #define CLS_SYNCCTRL_BASE_TIME               (10)
   #define CLS_STOP_WAIT_HEARTBEAT_TIMEOUT      (20*OSS_ONE_SEC)
   #define CLS_FORMART_STR_128                  (128)

   /*
      _clsReplicateSet define
   */
   _clsReplicateSet::_clsReplicateSet( _netRouteAgent *agent )
   : _agent( agent ),
     _vote( &_info, _agent),
     _logger( NULL ),
     _pFTMgr( NULL ),
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
      _lastConsultTick = 0 ;
      memset( _sizethreshold, 0, sizeof( _sizethreshold ) ) ;
      memset( _timeThreshold, 0, sizeof( _timeThreshold ) ) ;

      _faultEvent.reset() ;
      _syncEmptyEvent.signal() ;

      _syncwaitTimeout = 0 ;
      _shutdownWaitTimeout = 0 ;
      _fusingTimeout = 0 ;
      _isAllNodeFatal = FALSE ;
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

   UINT64 _clsReplicateSet::completeLsn( BOOLEAN doFast, UINT32 *pVer )
   {
      DPS_LSN lsn ;

      if ( !primaryIsMe() && _replBucket.maxReplSync() > 0 )
      {
         if ( doFast )
         {
            lsn = _replBucket.fastCompleteLSN() ;
         }
         else
         {
            lsn = _replBucket.completeLSN() ;
         }
      }

      if ( pVer )
      {
         *pVer = lsn.version ;
      }
      return lsn.offset ;
   }

   UINT32 _clsReplicateSet::lsnQueSize()
   {
      return getBucket()->bucketSize() ;
   }

   BOOLEAN _clsReplicateSet::primaryLsn( UINT64 &lsn, UINT32 *pVer )
   {
      BOOLEAN got = FALSE ;
      _clsSharingStatus tmpStatus ;

      if ( primaryIsMe() )
      {
         got = TRUE ;
         lsn = DPS_INVALID_LSN_OFFSET ;
         if ( pVer )
         {
            *pVer = DPS_INVALID_LSN_VERSION ;
         }
      }
      else if ( getPrimaryInfo( tmpStatus ) )
      {
         got = TRUE ;
         lsn = tmpStatus.beat.endLsn.offset ;
         if ( pVer )
         {
            *pVer = tmpStatus.beat.endLsn.version ;
         }
      }

      return got ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPPSET_NOTIFY2SESSION, "_clsReplicateSet::notify2Session" )
   void _clsReplicateSet::notify2Session( UINT32 suLID, UINT32 clLID,
                                          dmsExtentID extLID,
                                          const DPS_LSN_OFFSET & offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPPSET_NOTIFY2SESSION );
      // the src session is not empty, should notify every one
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

      _netFrame *pNetFrame = NULL ;

      if ( !_agent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // init start shift time
      g_startShiftTime = (INT32)pmdGetOptionCB()->startShiftTime() ;

      _logger = pmdGetKRCB()->getDPSCB() ;
      _pFTMgr = pmdGetKRCB()->getFTMgr() ;
      _clsCB = pmdGetKRCB()->getClsCB() ;
      SDB_ASSERT( NULL != _logger && NULL != _pFTMgr,
                  "logger should not be NULL" ) ;

      // register dps log event handler
      _logger->regEventHandler( this ) ;

      rc = _replBucket.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init repl bucket failed, rc: %d", rc ) ;

      pNetFrame = _agent->getFrame() ;
      /// register repl net agent to net monitor for connections
      pNetFrame->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;

      sdbGetPMDController()->registerNet( pNetFrame,
                                          MSG_ROUTE_REPL_SERVICE ) ;

      _totalLogSize = (UINT64)pmdGetOptionCB()->getReplLogFileSz()*
                      (UINT64)pmdGetOptionCB()->getReplLogFileNum() ;
      // init sync control param
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

      _syncwaitTimeout = pmdGetOptionCB()->syncwaitTimeout() * OSS_ONE_SEC ;
      _fusingTimeout = pmdGetOptionCB()->ftFusingTimeout() * OSS_ONE_SEC ;
      _shutdownWaitTimeout = pmdGetOptionCB()->shutdownWaitTimeout() *
                             OSS_ONE_SEC ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplicateSet::deactive ()
   {
      SDB_ASSERT( PMD_IS_DB_DOWN(), "DB must be down" ) ;
      UINT32 timeout = 0 ;

      /// disconnect al shard agent
      if ( _clsCB )
      {
         _clsCB->getShardRouteAgent()->disconnectAll() ;
      }

      /// wait sync replay packet
      _syncEmptyEvent.wait() ;
      /// wait sync bucket
      if ( _replBucket.maxReplSync() > 0 )
      {
         // wait all repl-sync log processed
         PD_LOG( PDEVENT, "Begin to wait repl bucket empty[bucket size: %d, "
                 "all size: %d, agent number: %d]", _replBucket.bucketSize(),
                 _replBucket.size(), _replBucket.curAgentNum() ) ;

         pmdSetDoing( "Waiting repl bucket to replay empty..." ) ;
         _replBucket.waitEmpty() ;
         pmdCleanDoing() ;

         PD_LOG( PDEVENT, "Wait repl bucket empty completed" ) ;
      }

      /// wait send stop heartbeat to other nodes
      if ( _active )
      {
         PD_LOG( PDEVENT, "Begin to wait broadcast stop-heartbeat..." ) ;
         _heartbeatEvent.reset() ;
         _beatTime = CLS_SHARING_BETA_INTERVAL ;
         if ( SDB_OK != _heartbeatEvent.wait( CLS_STOP_WAIT_HEARTBEAT_TIMEOUT ) )
         {
            PD_LOG( PDWARNING, "Wait broadcast stop-heartbeat failed" ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Wait broadcast stop-heartbeat succeed" ) ;
         }

         if ( _shutdownWaitTimeout > 0 &&
              _info.groupSize() > 1 &&
              _vote.primaryIsMe() )
         {
            UINT32 nodeCnt = 0 ;
            UINT32 aliveCnt = 0 ;
            UINT32 falutCnt = 0 ;
            UINT32 ssCnt = 0 ;
            INT32 indoubtErr = 0 ;
            UINT16 indoubtNodeID = 0 ;

            PD_LOG( PDEVENT, "Begin to wait data consistent..." ) ;
            pmdSetDoing( "Waiting data consistent..." ) ;
            /// When i'm primary, wait other node keep the data consistence
            while ( _vote.primaryIsMe() && timeout < _shutdownWaitTimeout )
            {
               if ( _logger->getCurrentLsn().invalid() ||
                    _sync.atLeastOne( _logger->getCurrentLsn().offset ) )
               {
                  PD_LOG( PDEVENT,
                          "Wait other node keep data consistent succeed" ) ;
                  break ;
               }
               /// check active node
               getDetailInfo( nodeCnt, aliveCnt, falutCnt, ssCnt,
                              indoubtErr, indoubtNodeID ) ;
               if ( 1 == aliveCnt )
               {
                  /// All node stoped
                  break ;
               }

               ossSleep( OSS_ONE_SEC ) ;
               timeout += OSS_ONE_SEC ;
            }
         }
         pmdCleanDoing() ;

         /// force secondary
         _vote.force( CLS_ELECTION_STATUS_SEC, OSS_SINT32_MAX ) ;
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
      if ( _agent )
      {
         sdbGetPMDController()->unregNet( _agent->getFrame() ) ;
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
      _syncwaitTimeout = pmdGetOptionCB()->syncwaitTimeout() * OSS_ONE_SEC ;
      _fusingTimeout = pmdGetOptionCB()->ftFusingTimeout() * OSS_ONE_SEC ;
      _shutdownWaitTimeout = pmdGetOptionCB()->shutdownWaitTimeout() *
                             OSS_ONE_SEC ;
   }

   void _clsReplicateSet::ntyPrimaryChange( BOOLEAN primary,
                                            SDB_EVENT_OCCUR_TYPE type )
   {
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         _replBucket.reset() ;
         setLastConsultTick( 0 ) ;
      }
      else if ( !primary && SDB_EVT_OCCUR_AFTER == type )
      {
         /// when we are not primary any more, we should clear
         /// waiting list.
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

      /// update new nodes, include the node with
      /// same id but different address
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
              /// when has overed the start shift time, need ignore
              /// the nodes there are not actived
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
           /// we alive the changed node here. if it is unnormal,
           /// break it out later.
           _alive( itr->second._id, FALSE ) ;
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

      /// remove deleted nodes
      itr2 = _info.info.begin() ;
      for ( ; itr2 != _info.info.end(); )
      {
         itr = nodes.find( itr2->first ) ;
         if ( nodes.end() == itr || FALSE == itr->second._isActive )
         {
            /// if primary is deleted, set primary invalid
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

   // The function is caller by any thread, so need to use lock
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
         /// avoid out-of-data's timeout event
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
         /// reset the timer tick
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

         /// When self is primary and NOSPC, should force to secondary
         if ( _vote.primaryIsMe() &&
              _info.groupSize() > 1 &&
              OSS_BIT_TEST( _pFTMgr->getConfirmedStat(), PMD_FT_MASK_NOSPC ) )
         {
            DPS_LSN lsn = _logger->getCurrentLsn() ;
            if ( lsn.invalid() || _sync.atLeastOne( lsn.offset ) )
            {
               PD_LOG( PDEVENT, "Force to secondary due to no disk space" ) ;
               _vote.force( CLS_ELECTION_STATUS_SEC, 5 * OSS_ONE_SEC ) ;
               _vote.setShadowWeight( CLS_ELECTION_WEIGHT_MIN ) ;
            }
         }
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
            rc = _handleSharingBeatRes( handle,
                                        ( const _MsgClsBeatRes *)msg ) ;
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
      /// set hash code
      _info.setHashCode( grpHashCode ) ;

      if ( !changeStatus )
      {
         _cata.remove( &(msg->header), MSG_GET_INNER_REPLY_RC(pHeader) ) ;
      }

      pmdGetKRCB()->setGroupName ( groupName.c_str() ) ;
      if ( !_active )
      {
         PD_LOG( PDEVENT, "download group info successfully" ) ;

         //start repl sync session
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

   // The function is called by cls mgr thread with the same change thread,
   // so don't need to use lock
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
         msg.beat.ftConfirmStat = _pFTMgr->getConfirmedStat() ;
         msg.beat.indoubtErr = _pFTMgr->getIndoubtErr() ;
         if ( _pFTMgr->isStop() )
         {
            msg.beat.nodeRunStat = (UINT8)CLS_NODE_STOP ;
         }
         else if ( _pFTMgr->isCatchup() )
         {
            msg.beat.nodeRunStat = (UINT8)CLS_NODE_CATCHUP ;
         }

         map<UINT64, _clsSharingStatus>::iterator itr = _info.info.begin() ;
         for ( ; itr != _info.info.end(); itr++ )
         {
            _clsSharingStatus &status = itr->second ;
 
            /// decrease dead time for heartbeat
            if ( status.deadtime >= pmdGetOptionCB()->sharingBreakTime() &&
                 status.deadtime >= _beatTime )
            {
               status.deadtime -= _beatTime ;
               continue ;
            }
            msg.beat.syncStatus = clsSyncWindow( status.beat.endLsn,
                                                 fBegin, mBegin, expectLSN ) ;

            rc = _sendSharingBeat( status, &msg ) ;
            if ( SDB_OK == rc )
            {
               status.sendFailedTimes = 0 ;
            }
            else
            {
               INT32 sysErr = SOCKET_GETLASTERROR ;

               if ( sysErr == CLS_CONNREFUSED )
               {
                  ++( status.sendFailedTimes ) ;
               }

               /// if send heartbeat msg failed, and the node is not in active,
               /// nead to reset dead time to decrease heartbeat msg
               if ( _info.alives.find( itr->first ) == _info.alives.end() )
               {
                  UINT32 resetTimeout = 0 ;
                  status.deadtime = pmdGetOptionCB()->sharingBreakTime() - 1 ;
                  if ( sysErr == CLS_CONNREFUSED )
                  {
                     resetTimeout = 1800 * OSS_ONE_SEC ;
                  }
                  else
                  {
                     resetTimeout = 120 * OSS_ONE_SEC ;
                  }
                  status.deadtime += resetTimeout ;

                  PD_LOG( PDEVENT, "Reset node[%d] sharing-beat time to %u(sec)",
                          status.beat.identity.columns.nodeID,
                          resetTimeout / OSS_ONE_SEC ) ;
               }
               /// When the node is alive, but run stat is CLS_NODE_STOP, and
               /// send heart-beat failed, should set timeout
               else if ( CLS_NODE_STOP == status.beat.nodeRunStat )
               {
                  status.timeout = pmdGetOptionCB()->sharingBreakTime() ;
               }
            }
         }
      }

   done:
      _heartbeatEvent.signalAll() ;
      PD_TRACE_EXIT ( SDB__CLSREPSET__SHRBEAT );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__SENDSHARINGBEAT, "_clsReplicateSet::_sendSharingBeat" )
   INT32 _clsReplicateSet::_sendSharingBeat( _clsSharingStatus &status,
                                             MsgClsBeat *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSREPSET__SENDSHARINGBEAT ) ;

      /// use UDP to send message, but we need to test whether remote
      /// supports UDP for backwards compatibility
      if ( status.isUDPSupported() )
      {
         // UDP is marked supported
         rc = _agent->syncSendUDP( status.beat.identity, message ) ;
      }
      else if ( status.isUDPUnavailable() )
      {
         // UDP is marked unavailable, use TCP directly
         rc = _agent->syncSend( status.beat.identity, message ) ;
      }
      else
      {
         // UDP status is unknown, test UDP first, and then send with TCP
         INT32 tmpRC = _agent->syncSendUDP( status.beat.identity, message ) ;
         if ( SDB_OK != tmpRC )
         {
            status.setUDPUnavailable() ;
         }
         else
         {
            status.increaseUDPTest() ;
         }

         rc = _agent->syncSend( status.beat.identity, message ) ;
      }

      PD_TRACE_EXITRC( SDB__CLSREPSET__SENDSHARINGBEAT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__CHKBRK, "_clsReplicateSet::_checkBreak" )
   void _clsReplicateSet::_checkBreak( const UINT32 &millisec )
   {
      /// avoid the use of w lock. only find item need to be
      /// erase, we lock w. here we think that no need to lock
      /// w when change value
      PD_TRACE_ENTRY ( SDB__CLSREPSET__CHKBRK );

      BOOLEAN needErase = FALSE ;
      map<UINT64, _clsSharingStatus *>::iterator itr ;
      map< UINT64, _clsSharingStatus>::iterator itrInfo ;
      _clsSharingStatus *pStatus = NULL ;
      BOOLEAN isAllNodeFatal = TRUE ;

      for ( itr = _info.alives.begin() ; itr != _info.alives.end() ; itr++ )
      {
         pStatus = itr->second ;
         pStatus->timeout += millisec ;

         if ( isAllNodeFatal &&
              !PMD_FT_IS_FATAL_FAULT( pStatus->beat.ftConfirmStat ) )
         {
            isAllNodeFatal = FALSE ;
         }

         if ( pmdGetOptionCB()->sharingBreakTime() <= pStatus->timeout )
         {
            needErase = TRUE ;
         }
      }

      /// update _isAllNodeFatal
      _isAllNodeFatal = isAllNodeFatal ;

      // increase break node's break time
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
         pStatus = itr->second ;
         if ( pmdGetOptionCB()->sharingBreakTime() <= pStatus->timeout )
         {
            if ( itr->first == _info.primary.value )
            {
               PD_LOG( PDERROR, "vote: primary [node:%d] alive break(%s)",
                       _info.primary.columns.nodeID,
                       ( CLS_NODE_STOP == pStatus->beat.nodeRunStat ?
                         "shutdown" : "unknown" ) ) ;
               _info.primary.value = MSG_INVALID_ROUTEID ;
            }
            else
            {
               PD_LOG( PDERROR, "vote: [node:%d] alive break(%s)",
                       pStatus->beat.identity.columns.nodeID,
                       ( CLS_NODE_STOP == pStatus->beat.nodeRunStat ?
                         "shutdown" : "unknown" ) ) ;
            }
            pStatus->beat.beatID = CLS_BEATID_INVALID ;
            pStatus->beat.serviceStatus = SERVICE_UNKNOWN ;
            pStatus->beat.ftConfirmStat = 0 ;
            pStatus->beat.indoubtErr = SDB_OK ;

            // alive break, reset UDP support
            pStatus->resetUDP() ;

            _sync.updateNodeStatus( pStatus->beat.identity, FALSE ) ;

            _info.alives.erase( itr++ ) ;
         }
         else
         {
            ++itr ;
         }
      }
      _info.mtx.release_w() ;
      /// cutting when down to secandary is in _clsVSPrimary.
      if ( _vote.primaryIsMe() )
      {
         _sync.cut( _info.alives.size(),
                    FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() ? TRUE : FALSE ) ;
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
      PD_TRACE_ENTRY ( SDB__CLSREPSET__HNDSHRBEAT ) ;

      const _clsGroupBeat &beat = msg->beat ;
      map<UINT64, _clsSharingStatus>::iterator itr ;

      itr = _info.info.find( beat.identity.value ) ;
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
         //download ;
         _MsgCatGroupReq msg ;
         msg.id = _info.local ;
         _cata.call( (MsgHeader *)(&msg) ) ;
      }
      else if ( itr != _info.info.end() )
      {
         _clsSharingStatus &statusItem = itr->second ;

         /// FT confirm stat changed
         if ( statusItem.beat.ftConfirmStat != beat.getFTConfirmStat() )
         {
            CHAR oldStatStr[ CLS_FORMART_STR_128 + 1 ] = { 0 } ;
            CHAR newStatStr[ CLS_FORMART_STR_128 + 1 ] = { 0 } ;

            utilFTMaskToStr( statusItem.beat.ftConfirmStat,
                             oldStatStr, CLS_FORMART_STR_128 ) ;
            utilFTMaskToStr( beat.getFTConfirmStat(),
                             newStatStr, CLS_FORMART_STR_128 ) ;
            PD_LOG( PDEVENT, "Node[%d]'s fault-tolerance confirm stat "
                    "changed: 0x%08x(%s) => 0x%08x(%s), indoubt error: %d",
                    beat.identity.columns.nodeID,
                    statusItem.beat.ftConfirmStat,
                    oldStatStr,
                    beat.getFTConfirmStat(),
                    newStatStr,
                    beat.getIndoubtErr() ) ;
         }
         /// Node start/stop changed
         if ( statusItem.beat.nodeRunStat != beat.nodeRunStat )
         {
            PD_LOG( PDEVENT, "Node[%d]'s run stat changed: %d(%s) => %d(%s)",
                    beat.identity.columns.nodeID,
                    statusItem.beat.nodeRunStat,
                    clsNodeRunStat2String( statusItem.beat.nodeRunStat ),
                    beat.nodeRunStat,
                    clsNodeRunStat2String( beat.nodeRunStat ) ) ;
         }

         statusItem.beat = beat ;

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

               /// when self is in slice, force to secondary
               if ( _vote.isStatus( CLS_ELECTION_STATUS_SILENCE ) )
               {
                  _vote.force( CLS_ELECTION_STATUS_SEC ) ;
               }
            }

            // if find new primary node, should to wake up reelection
            if ( CLS_ELECTION_WEIGHT_USR_MIN != _vote.getShadowWeight() &&
                 _vote.isShadowTimeout() )
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
         _alive( beat.identity, _isUDPHandle( handle ) ) ;
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

   INT32 _clsReplicateSet::_handleSharingBeatRes( NET_HANDLE handle,
                                                  const _MsgClsBeatRes *msg )
   {
      SDB_ASSERT( NULL != msg, "msg should not be NULL" ) ;
      return _alive( msg->identity, _isUDPHandle( handle ) ) ;
   }

   void _clsReplicateSet::setLastConsultTick( UINT64 tick )
   {
      _lastConsultTick = tick ;
   }

   UINT64 _clsReplicateSet::getLastConsultTick() const
   {
      return _lastConsultTick ;
   }

   INT32 _clsReplicateSet::aliveNode( const MsgRouteID &id )
   {
      INT32 rc = SDB_OK ;

      /// wait for 100 mili-secs
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

   BOOLEAN _clsReplicateSet::_isUDPHandle( NET_HANDLE handle )
   {
      return ( NET_EVENT_HANDLER_UDP ==
                           _agent->getFrame()->getEventHandleType( handle ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__ALIVE, "_clsReplicateSet::_alive" )
   INT32 _clsReplicateSet::_alive( const _MsgRouteID &id, BOOLEAN fromUDP )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__ALIVE );
      map<UINT64, _clsSharingStatus>::iterator itr ;

      itr = _info.info.find( id.value ) ;
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

         PD_LOG( PDEVENT, "vote: [node:%d] aliving from %s",
                 status.beat.identity.columns.nodeID,
                 ( CLS_NODE_STOP == status.beat.nodeRunStat ?
                   "shutdown" : "break" ) ) ;
      }
      itr->second.timeout = 0 ;
      itr->second.breakTime = 0 ;
      itr->second.deadtime = 0 ;
      itr->second.sendFailedTimes = 0 ;

      if ( fromUDP )
      {
         itr->second.setUDPSupported() ;
      }

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
      BOOLEAN hasBlock = FALSE ;

      while ( SDB_OK == rc && PMD_IS_DB_AVAILABLE() )
      {
         offset = _sync.getSyncCtrlArbitLSN() ;
         if ( DPS_INVALID_LSN_OFFSET == offset )
         {
            break ;
         }

         expectLSN = _logger->expectLsn() ;
         // when log file number == 1, make sure all other nodes has uped
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

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_SYNCCONTROL,
                             "Waiting for sync control" ) ;
               hasBlock = TRUE ;
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
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
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
                                    pmdEDUCB *cb,
                                    UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_REELECT ) ;
      if ( 1 == groupSize() )
      {
         goto done ;
      }

      rc = _reelection.run( lvl, seconds, cb, destID ) ;
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
         /// do nothing.
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET_PRIMARYCHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET_REPLSZCHECK, "_clsReplicateSet::replSizeCheck" )
   INT32 _clsReplicateSet::replSizeCheck( INT16 w, INT16 &finalW,
                                          _pmdEDUCB *cb,
                                          BOOLEAN isAfterData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_REPLSZCHECK ) ;

      UINT32 nodeCnt = 0 ;
      UINT32 aliveCnt = 0 ;
      UINT32 faultCnt = 0 ;
      UINT32 ssCnt = 0 ;
      INT32 indoubtErr = SDB_OK ;
      UINT16 indoubtNodeID = 0 ;
      INT16 adjW = 0 ;
      UINT32 timeout = 0 ;
      BOOLEAN hasBlock = FALSE ;

      /// check valid
      if ( w < -1 || w > CLS_REPLSET_MAX_NODE_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDWARNING, "Invalid replsize: %d", w ) ;
         goto error ;
      }

      cb->setOrgReplSize( w ) ;

      if ( 1 == w && ( isAfterData || !_isAllNodeFatal ) )
      {
         finalW = w ;
         goto done ;
      }

      while( TRUE )
      {
         adjW = 0 ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         getDetailInfo( nodeCnt, aliveCnt, faultCnt, ssCnt,
                        indoubtErr, indoubtNodeID ) ;

         /// One node in the group
         if ( 1 == nodeCnt )
         {
            finalW = 1 ;
            break ;
         }
         else if ( 1 == w )
         {
            if ( isAfterData || !_isAllNodeFatal )
            {
               finalW = 1 ;
               break ;
            }

            if ( timeout >= _fusingTimeout )
            {
               goto error ;
            }

            ossSleep( OSS_ONE_SEC ) ;
            timeout += OSS_ONE_SEC ;
            continue ;
         }
         else if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
         {
            nodeCnt = aliveCnt ;
         }

         /// When exist fault node
         if ( faultCnt > 0 )
         {
            switch ( _pFTMgr->getFTLevel() )
            {
               case FT_LEVEL_FUSING :
                  break ;
               case FT_LEVEL_SEMI :
                  if ( -1 == w )
                  {
                     adjW = faultCnt ;
                  }
                  break ;
               case FT_LEVEL_WHOLE :
                  adjW = faultCnt ;
                  break ;
               default:
                  break ;
            }
         }

         if ( 0 == w || w > (INT16)nodeCnt )
         {
            finalW = nodeCnt ;

            if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
            {
               adjW += ssCnt ;
            }
            else
            {
               ssCnt = 0 ;
            }
         }
         else if ( -1 == w )
         {
            finalW = aliveCnt ;
            adjW += ssCnt ;
         }
         else
         {
            finalW = w ;

            if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
            {
               adjW += ssCnt ;
            }
            else
            {
               ssCnt = 0 ;
            }
         }

         if ( finalW > (INT16)aliveCnt )
         {
            if ( isAfterData )
            {
               rc = SDB_CLS_WAIT_SYNC_FAILED ;
            }
            else
            {
               PD_LOG( PDERROR, "Alive num[%d] can not meet need[%d]",
                       aliveCnt, finalW ) ;
               rc = SDB_CLS_NODE_NOT_ENOUGH ;
            }
            goto error ;
         }
         else if ( finalW <= (INT16)( aliveCnt - faultCnt - ssCnt ) )
         {
            break ;
         }
         /// down level
         else if ( aliveCnt - faultCnt >= 2 && adjW > 0 )
         {
            finalW = (INT16)( aliveCnt - faultCnt - ssCnt ) ;
            if ( finalW < 2 )
            {
               finalW = 2 ;
            }
            break ;
         }

         if ( isAfterData || timeout >= _fusingTimeout )
         {
            goto error ;
         }

         if ( !hasBlock )
         {
            hasBlock = TRUE ;
            cb->setBlock( EDU_BLOCK_FT, "Waiting for fault-tolerant" ) ;
         }

         ossSleep( OSS_ONE_SEC ) ;
         timeout += OSS_ONE_SEC ;
         continue ;
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      PD_TRACE_EXITRC( SDB__CLSREPSET_REPLSZCHECK, rc ) ;
      return rc ;
   error:
      if ( SDB_OK == rc )
      {
         if ( isAfterData )
         {
            rc = SDB_CLS_WAIT_SYNC_FAILED ;
         }
         else
         {
            rc = SDB_CLS_NODE_NOT_ENOUGH ;
            if ( indoubtErr )
            {
               /// Can't set rc = indoubtErr.
               /// Because onOPMsg() will retry for some error
               PD_LOG_MSG( PDERROR, "Fusing operation by indoubt error(%d) "
                           "from node(%u)", indoubtErr, indoubtNodeID ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Fusing operation by error(%d)",
                           rc ) ;
            }
         }
      }
      goto done ;
   }

}

