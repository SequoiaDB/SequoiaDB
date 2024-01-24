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

   Source File Name = clsMgr.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsMgr.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "clsShardSession.hpp"
#include "clsReplSession.hpp"
#include "clsFSDstSession.hpp"
#include "clsFSSrcSession.hpp"
#include "clsStorageCheckJob.hpp"
#include "../bson/bson.h"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "dpsOp2Record.hpp"
#include "pmdStartup.hpp"
#include "clsRegAssit.hpp"
#include "schedTaskContainer.hpp"
#include "schedTaskAdapter.hpp"
#include "schedPrepareJob.hpp"
#include "schedDispatchJob.hpp"
#include "clsUniqueIDCheckJob.hpp"
#include "coordResource.hpp"
#include "coordRemoteSession.hpp"
#include "pmdController.hpp"
#include "clsResourceContainer.hpp"
#include "clsIndexJob.hpp"

using namespace bson ;

namespace engine
{

   //The max del session deque size
   #define MAX_SHD_SESSION_CATCH_DEQ_SIZE          (2000)

   #define CLS_WAIT_CB_ATTACH_TIMEOUT              ( 300 * OSS_ONE_SEC )

   /*
      _clsShardSessionMgr implement
   */
   _clsShardSessionMgr::_clsShardSessionMgr( _clsMgr *pClsMgr )
   {
      _pClsMgr    = pClsMgr ;
      _unShardSessionTimer = NET_INVALID_TIMER_ID ;
   }

   _clsShardSessionMgr::~_clsShardSessionMgr()
   {
      _pClsMgr    = NULL ;
   }

   schedTaskInfo* _clsShardSessionMgr::getTaskInfo()
   {
      return &_taskInfo ;
   }

   void _clsShardSessionMgr::onConfigChange()
   {
      pmdOptionsCB *optionsCB = pmdGetKRCB()->getOptionCB() ;

      _taskInfo.setTaskLimit( optionsCB->getSvcMaxConcurrency() ) ;
   }

   BOOLEAN _clsShardSessionMgr::isUnShardTimerStarted() const
   {
      return NET_INVALID_TIMER_ID == _unShardSessionTimer ?
             FALSE : TRUE ;
   }

   void _clsShardSessionMgr::startUnShardTimer( UINT32 interval )
   {
      if ( _pRTAgent && _pTimerHandle && !isUnShardTimerStarted() )
      {
         _pRTAgent->addTimer( interval, _pTimerHandle,
                              _unShardSessionTimer ) ;
      }
   }

   void _clsShardSessionMgr::stopUnShardTimer()
   {
      if ( _pRTAgent && _pTimerHandle && isUnShardTimerStarted() )
      {
         _pRTAgent->removeTimer( _unShardSessionTimer ) ;
         _unShardSessionTimer = NET_INVALID_TIMER_ID ;
      }
   }

   UINT64 _clsShardSessionMgr::makeSessionID( const NET_HANDLE &handle,
                                              const MsgHeader *header )
   {
      UINT64 sessionID = ossPack32To64( header->routeID.columns.nodeID,
                                        header->TID ) ;
      if ( header->routeID.columns.nodeID < DATA_NODE_ID_BEGIN ||
           header->routeID.columns.groupID < DATA_GROUP_ID_BEGIN )
      {
         sessionID = ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
      }

      return sessionID ;
   }

   BOOLEAN _clsShardSessionMgr::_isSplitSessionMsg( UINT32 opCode )
   {
      if ( 0 == opCode )
      {
         return TRUE ;
      }

      return isSplitSessionMsg( opCode ) ;
   }

   SDB_SESSION_TYPE _clsShardSessionMgr::_prepareCreate( UINT64 sessionID,
                                                         INT32 startType,
                                                         INT32 opCode )
   {
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;

      if ( _isSplitSessionMsg( (UINT32)opCode ) )
      {
         if ( PMD_SESSION_ACTIVE == startType )
         {
            // If it's proactive request, that means it's split destination
            // During split the destination part "asks" for the data from source
            sessionType = SDB_SESSION_SPLIT_DST ;
         }
         else
         {
            // otherwise it's the source, which receives the split request
            sessionType = SDB_SESSION_SPLIT_SRC ;
         }
      }
      else
      {
         sessionType = SDB_SESSION_SHARD ;
      }

      return sessionType ;
   }

   BOOLEAN _clsShardSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      if ( SDB_SESSION_SHARD == sessionType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _clsShardSessionMgr::_maxCacheSize() const
   {
      UINT32 maxPool = pmdGetOptionCB()->getMaxPooledEDU() ;
      return maxPool < MAX_SHD_SESSION_CATCH_DEQ_SIZE ?
             maxPool : MAX_SHD_SESSION_CATCH_DEQ_SIZE ;
   }

   // create session request for the manager
   // there are 3 types of sessions for shardsessions
   // 1) split destination
   // 2) split source
   // 3) regular shard session
   pmdAsyncSession* _clsShardSessionMgr::_createSession(
         SDB_SESSION_TYPE sessionType,
         INT32 startType,
         UINT64 sessionID,
         void *data )
   {
      pmdAsyncSession *pSession = NULL ;

      // Based on session type, let's create Async session
      if ( SDB_SESSION_SPLIT_DST == sessionType )
      {
         pSession = SDB_OSS_NEW _clsSplitDstSession ( sessionID, _pRTAgent,
                                                      data ) ;
      }
      else if ( SDB_SESSION_SPLIT_SRC == sessionType )
      {
         pSession = SDB_OSS_NEW _clsSplitSrcSession ( sessionID, _pRTAgent ) ;
      }
      else if ( SDB_SESSION_SHARD == sessionType )
      {
         pSession = SDB_OSS_NEW _clsShdSession ( sessionID, &_taskInfo ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Unknow session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   void _clsShardSessionMgr::_onSessionNew( pmdAsyncSession *pSession )
   {
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         map< UINT64, clsIdentifyInfo >::iterator it ;
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         it = _mapIdentifys.find( pSession->sessionID() ) ;
         if ( it != _mapIdentifys.end() )
         {
            pShdSession->setDelayLogin( it->second ) ;
            _mapIdentifys.erase( it ) ;
         }
      }
   }

   INT32 _clsShardSessionMgr::handleSessionTimeout( UINT32 timerID,
                                                    UINT32 interval )
   {
      INT32 rc = SDB_OK ;

      if ( _unShardSessionTimer == timerID )
      {
         _checkUnShardSessions( interval ) ;

         // start split task
         _pClsMgr->_startInnerSession( CLS_SHARD, this ) ;

         goto done ;
      }
      else if ( _sessionTimerID == timerID )
      {
         ossScopedLock lock( &_metaLatch ) ;
         if ( _mapSession.size() <= MAX_SHD_SESSION_CATCH_DEQ_SIZE )
         {
            goto done ;
         }
      }

      rc = _pmdAsycSessionMgr::handleSessionTimeout( timerID, interval ) ;

   done:
      return rc ;
   }

   // check timeout for the irregular shard sessions ( like split sessions )
   // usually those types of sessions are for communication within between shard
   // like one shard directly send msg to another shard
   void _clsShardSessionMgr::_checkUnShardSessions( UINT32 interval )
   {
      pmdAsyncSession *pSession = NULL ;

      ossScopedLock lock( &_metaLatch ) ;

      MAPSESSION_IT it = _mapSession.begin() ;
      while ( it != _mapSession.end() )
      {
         pSession = it->second ;
         // skip regular shard sessions
         if ( SDB_SESSION_SHARD == pSession->sessionType() )
         {
            ++it ;
            continue ;
         }
         // get rid of timeout sessions
         if ( !pSession->isProcess() &&
              pSession->timeout( interval ) &&
              !pSession->hasHold() &&
              0 == pSession->getPendingMsgNum() )
         {
            PD_LOG ( PDEVENT, "Session[%s] timeout", pSession->sessionName() ) ;
            _releaseSession_i ( pSession, TRUE, TRUE ) ;
            _mapSession.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }
   }

   void _clsShardSessionMgr::onSessionDestoryed( pmdAsyncSession *pSession )
   {
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         if( !pShdSession->isSetLogout() )
         {
            /// save identify info
            clsIdentifyInfo info ;
            info._id = pSession->identifyID() ;
            info._nid = pSession->identifyNID() ;
            info._eduid = pSession->identifyEDUID() ;
            info._tid = pSession->identifyTID() ;
            info._username = pSession->getClient()->getUsername() ;
            pSession->getAuditConfig( info._auditMask,
                                      info._auditConfigMask ) ;
            if ( !info._username.empty() )
            {
               info._passwd = pSession->getClient()->getPassword() ;
            }
            if ( !pSession->getSchedInfo()->isDefault() )
            {
               info._objSchedInfo = pSession->getSchedInfo()->toBSON() ;
            }
            info._source = pShdSession->getSource() ;
            /// save trans conf
            info._transConf = pShdSession->getTransConf() ;
            _mapIdentifys[ pSession->sessionID() ] = info ;
         }
      }
   }

   void _clsShardSessionMgr::onSessionDisconnect( pmdAsyncSession *pSession )
   {
      /// recv the disconnect msg, so need to logout
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         pShdSession->setLogout() ;

         _mapIdentifys.erase( pSession->sessionID() ) ;
      }
   }

   void _clsShardSessionMgr::onNoneSessionDisconnect( UINT64 sessionID )
   {
      _mapIdentifys.erase( sessionID ) ;
   }

   void _clsShardSessionMgr::onSessionHandleClose( pmdAsyncSession *pSession )
   {
      /// when net handle closed, need to logout
      if ( SDB_SESSION_SHARD == pSession->sessionType() )
      {
         _clsShdSession *pShdSession = ( _clsShdSession* )pSession ;
         pShdSession->setLogout() ;
         _mapIdentifys.erase( pSession->sessionID() ) ;
      }
   }

   INT32 _clsShardSessionMgr::onErrorHanding( INT32 rc,
                                              const MsgHeader *pReq,
                                              const NET_HANDLE &handle,
                                              UINT64 sessionID,
                                              pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;
      ossUnpack32From64( sessionID, nodeID, tid ) ;

      if ( nodeID > PMD_BASE_HANDLE_ID )
      {
         /// shard session
         ret = _reply( handle, rc, pReq ) ;
      }
      else if ( 0 == sessionID )
      {
         ret = rc ;
      }

      return ret ;
   }

   /*
      _clsReplSessionMgr implement
   */
   _clsReplSessionMgr::_clsReplSessionMgr( _clsMgr *pClsMgr )
   {
      _pClsMgr = pClsMgr ;
   }

   _clsReplSessionMgr::~_clsReplSessionMgr()
   {
      _pClsMgr = NULL ;
   }

   INT32 _clsReplSessionMgr::handleSessionTimeout( UINT32 timerID,
                                                   UINT32 interval )
   {
      INT32 rc = SDB_OK ;

      rc = _pmdAsycSessionMgr::handleSessionTimeout( timerID, interval ) ;
      if ( SDB_OK == rc )
      {
         // start repl/fs sessions
         _pClsMgr->_startInnerSession( CLS_REPL, this ) ;
      }

      return rc ;
   }

   UINT64 _clsReplSessionMgr::makeSessionID( const NET_HANDLE & handle,
                                             const MsgHeader * header )
   {
      return ossPack32To64( header->routeID.columns.nodeID,
                            header->TID ) ;
   }

   SDB_SESSION_TYPE _clsReplSessionMgr::_prepareCreate( UINT64 sessionID,
                                                        INT32 startType,
                                                        INT32 opCode )
   {
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;
      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;

      ossUnpack32From64( sessionID, nodeID, tid ) ;

      if ( CLS_TID_REPL_SYC == tid )
      {
         sessionType = PMD_SESSION_ACTIVE == startType ?
                       SDB_SESSION_REPL_DST :
                       SDB_SESSION_REPL_SRC ;
      }
      else if ( CLS_TID_REPL_FS_SYC == tid )
      {
         if ( PMD_SESSION_ACTIVE == startType )
         {
            sessionType = SDB_SESSION_FS_DST ;
         }
         else
         {
            sessionType = SDB_SESSION_FS_SRC ;
         }
      }

      return sessionType ;
   }

   BOOLEAN _clsReplSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      return FALSE ;
   }

   UINT32 _clsReplSessionMgr::_maxCacheSize() const
   {
      return 0 ;
   }

   // create replication sessions manager
   // include:
   // 1) replication destination
   // 2) replication source
   // 3) full sync destination
   // 4) full sync source
   pmdAsyncSession* _clsReplSessionMgr::_createSession(
         SDB_SESSION_TYPE sessionType,
         INT32 startType,
         UINT64 sessionID,
         void *data )
   {
      pmdAsyncSession *pSession = NULL ;
      // check session type for replication sessions
      if ( SDB_SESSION_REPL_DST == sessionType )
      {
         // slave node uses dest
         pSession = SDB_OSS_NEW clsReplDstSession( sessionID ) ;
      }
      else if ( SDB_SESSION_REPL_SRC == sessionType )
      {
         // primary node uses src
         UINT32 nodeID = 0 ;
         UINT32 tid = 0 ;
         ossUnpack32From64( sessionID, nodeID, tid ) ;

         // if we find the requested nodeID is not the nodeID for the current
         // node, that means we get something from another node and we are
         // going to create a new replsrc session
         if ( pmdGetNodeID().columns.nodeID != 0 &&
              pmdGetNodeID().columns.nodeID != nodeID )
         {
            pSession = SDB_OSS_NEW clsReplSrcSession( sessionID ) ;
         }
      }
      // FS means full sync
      else if ( SDB_SESSION_FS_DST == sessionType )
      {
         pSession = SDB_OSS_NEW _clsFSDstSession ( sessionID,
                                                   _pRTAgent ) ;
      }
      else if ( SDB_SESSION_FS_SRC == sessionType )
      {
         pSession = SDB_OSS_NEW _clsFSSrcSession ( sessionID,
                                                   _pRTAgent ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Unknow session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   INT32 _clsReplSessionMgr::onErrorHanding( INT32 rc,
                                             const MsgHeader *pReq,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      if ( 0 == sessionID )
      {
         ret = rc ;
      }

      return ret ;
   }

   /*
      _clsMgr implement
   */
   BEGIN_OBJ_MSG_MAP( _clsMgr, _pmdObjBase )
      ON_MSG ( MSG_CAT_REG_RES, _onCatRegisterRes )
      ON_MSG ( MSG_CAT_QUERY_TASK_RSP, _onCatQueryTaskRes )
      ON_EVENT( PMD_EDU_EVENT_STEP_DOWN, _onStepDown )
      ON_EVENT( PMD_EDU_EVENT_STEP_UP, _onStepUp )
      ON_EVENT( PMD_EDU_EVENT_UPDATE_GRPMODE, _onGroupModeUpdate )
      //ON_EVENT FUCTION MAP
   END_OBJ_MSG_MAP()

   _clsMgr::_clsMgr ()
   :_shardSessionMgr( this ),
    _replSessionMgr( this ),
    _shardServiceID ( MSG_ROUTE_SHARD_SERVCIE ),
    _replServiceID ( MSG_ROUTE_REPL_SERVICE ),
    _taskMgr( 0x7FFFFFFF ),
    _requestID ( 0 ),
    _regTimerID ( CLS_INVALID_TIMERID ),
    _regFailedTimes( 0 ),
    _needUpdateNode( FALSE ),
    _oneSecTimerID ( CLS_INVALID_TIMERID ),
    _taskTimerID( CLS_INVALID_TIMERID )
   {
      _replServiceName[0] = 0 ;
      _shdServiceName[0]  = 0 ;
      _selfNodeID.value   = MSG_INVALID_ROUTEID ;

      _pContainerMgr       = NULL ;
      _pShardAdapter       = NULL ;
      _shdMsgHandlerObj    = NULL ;
      _replMsgHandlerObj   = NULL ;
      _shdTimerHandler     = NULL ;
      _replTimerHandler    = NULL ;
      _replNetRtAgent      = NULL ;
      _shardNetRtAgent     = NULL ;
      _shdObj              = NULL ;
      _replObj             = NULL ;
      _pSitePropMgr        = NULL ;
      _pResource           = NULL ;
   }

   _clsMgr::~_clsMgr ()
   {
   }

   SDB_CB_TYPE _clsMgr::cbType () const
   {
      return SDB_CB_CLS ;
   }

   const CHAR* _clsMgr::cbName () const
   {
      return "CLSCB" ;
   }

   INT32 _clsMgr::_initRemoteSession( _netRouteAgent *netRouteAgent )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;

      _pResource = SDB_OSS_NEW _coordResource() ;
      PD_CHECK( NULL != _pResource, SDB_OOM, error, PDERROR,
                "Failed to malloc _coordResource, rc: %d", rc ) ;

      rc = _pResource->init( netRouteAgent, optCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Init resource failed, rc: %d", rc ) ;

      // set userOwnQueue = TRUE to avoid messages posted to EDU directly
      _pSitePropMgr = SDB_OSS_NEW _coordSessionPropMgr( TRUE ) ;
      PD_CHECK( NULL != _pSitePropMgr, SDB_OOM, error, PDERROR,
                "Failed to malloc _coordSessionPropMgr, rc: %d", rc ) ;

      _pSitePropMgr->setInstanceOption( optCB->getPrefInstStr(),
                                        optCB->getPrefInstModeStr(),
                                        optCB->isPreferredStrict(),
                                        optCB->getPreferredPeriod(),
                                        optCB->getPrefConstraint(),
                                        PMD_PREFER_INSTANCE_TYPE_MASTER ) ;
      rc = _remoteSessionMgr.init( netRouteAgent, _pSitePropMgr ) ;
      PD_RC_CHECK ( rc, PDERROR, "Init session manager failed, rc: %d", rc ) ;

      // set remote session manager to pmdController
      sdbGetPMDController()->setRSManager( &_remoteSessionMgr ) ;

      sdbGetResourceContainer()->setResource( _pResource ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::init ()
   {
      INT32 rc = SDB_OK ;
      NodeID nodeID = _selfNodeID ;
      const CHAR* hostName = pmdGetKRCB()->getHostName() ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;

      SCHED_TASK_QUE_TYPE queType = SCHED_TASK_FIFO_QUE ;
      UINT32 svcSchedulerType = optCB->getSvcSchedulerType() ;

      /// create object
      _pContainerMgr = SDB_OSS_NEW schedTaskContanierMgr() ;
      if ( !_pContainerMgr )
      {
         PD_LOG( PDERROR, "Allocate container manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      if ( svcSchedulerType >= SCHED_TYPE_FIFO &&
           svcSchedulerType <= SCHED_TYPE_CONTAINER )
      {
         if ( SCHED_TYPE_FIFO != svcSchedulerType )
         {
            queType = SCHED_TASK_PIRORITY_QUE ;
         }

         if ( SCHED_TYPE_CONTAINER == svcSchedulerType )
         {
            _pShardAdapter = SDB_OSS_NEW schedContainerAdapter( _pContainerMgr ) ;
         }
         else
         {
            _pShardAdapter = SDB_OSS_NEW schedFIFOAdapter() ;
         }

         if ( !_pShardAdapter )
         {
            PD_LOG( PDERROR, "Allocate task adapter failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = _pShardAdapter->init( _shardSessionMgr.getTaskInfo(), queType ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init task adapter failed, rc: %d" ) ;
            goto error ;
         }
      }

      rc = _pContainerMgr->init( queType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init task container failed, rc: %d", rc ) ;
         goto error ;
      }

      _shdMsgHandlerObj = SDB_OSS_NEW _shdMsgHandler( &_shardSessionMgr,
                                                      _pShardAdapter,
                                                      &_remoteSessionMgr ) ;
      if ( !_shdMsgHandlerObj )
      {
         PD_LOG( PDERROR, "Allocate shard message handler failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _replMsgHandlerObj = SDB_OSS_NEW _replMsgHandler( &_replSessionMgr ) ;
      if ( !_replMsgHandlerObj )
      {
         PD_LOG( PDERROR, "Allocate repl message handler failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _shdTimerHandler = SDB_OSS_NEW _clsShardTimerHandler( &_shardSessionMgr ) ;
      if ( !_shdTimerHandler )
      {
         PD_LOG( PDERROR, "Allocate shard timer handler failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _replTimerHandler = SDB_OSS_NEW _clsReplTimerHandler( &_replSessionMgr ) ;
      if ( !_replTimerHandler )
      {
         PD_LOG( PDERROR, "Allocate repl timer handler failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _replNetRtAgent = SDB_OSS_NEW _netRouteAgent( _replMsgHandlerObj ) ;
      if ( !_replNetRtAgent )
      {
         PD_LOG( PDERROR, "Allocate repl netagent failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _shardNetRtAgent = SDB_OSS_NEW _netRouteAgent( _shdMsgHandlerObj ) ;
      if ( !_shardNetRtAgent )
      {
         PD_LOG( PDERROR, "Allocate shard netagent failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _initRemoteSession( _shardNetRtAgent ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init remote session, rc: %d", rc ) ;

      _shdObj = SDB_OSS_NEW _clsShardMgr( _shardNetRtAgent ) ;
      if ( !_shdObj )
      {
         PD_LOG( PDERROR, "Allocate shard manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _replObj = SDB_OSS_NEW _clsReplicateSet( _replNetRtAgent ) ;
      if ( !_replObj )
      {
         PD_LOG( PDERROR, "Allocate repl manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // 1. init param
      ossStrncpy( _shdServiceName, optCB->shardService(),
                  OSS_MAX_SERVICENAME ) ;
      ossStrncpy( _replServiceName, optCB->replService(),
                  OSS_MAX_SERVICENAME ) ;

      _shardSessionMgr.getTaskInfo()->setTaskLimit(
         optCB->getSvcMaxConcurrency() ) ;

      INIT_OBJ_GOTO_ERROR ( getShardCB() ) ;
      INIT_OBJ_GOTO_ERROR ( getReplCB() ) ;

      // 2. create listen socket
      if ( optCB->serviceMask() & PMD_SVC_MASK_REPLICATE )
      {
         PD_LOG( PDEVENT, "Replicate listener is disabled" ) ;
      }
      else
      {
         nodeID.columns.serviceID = _replServiceID ;
         _replNetRtAgent->updateRoute( nodeID, hostName, _replServiceName ) ;
         rc = _replNetRtAgent->listen( nodeID,
                                       ( NET_FRAME_MASK_TCP |
                                         NET_FRAME_MASK_UDP ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Create listen[Hostname:%s, ServiceName:%s]"
                              " failed",
                     hostName, _replServiceName ) ;
            goto error ;
         }
         PD_LOG ( PDEVENT, "Create replicate group listen[ServiceName:%s]"
                           " succeed",
                  _replServiceName ) ;
      }

      if ( optCB->serviceMask() & PMD_SVC_MASK_SHARD )
      {
         PD_LOG( PDEVENT, "Shard listener is disabled" ) ;
      }
      else
      {
         nodeID.columns.serviceID = _shardServiceID ;
         _shardNetRtAgent->updateRoute( nodeID, hostName, _shdServiceName ) ;
         rc = _shardNetRtAgent->listen( nodeID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Create listen[Hostname:%s, ServiceName:%s]"
                              " failed",
                     hostName, _shdServiceName ) ;
            goto error ;
         }
         PD_LOG ( PDEVENT, "Create sharding listen[ServiceName:%s] succeed",
                  _shdServiceName ) ;
      }

      // 3. init session manager
      rc = _shardSessionMgr.init( _shardNetRtAgent, _shdTimerHandler,
                                  60 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init shard session manager, rc: %d",
                   rc ) ;

      rc = _replSessionMgr.init( _replNetRtAgent, _replTimerHandler,
                                 OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init repl session manager, rc: %d",
                   rc ) ;

      rc = _recycleBinMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init recycle bin manager, rc: %d",
                   rc ) ;

      // 4. set bussiness not ok( need wait register to change )
      pmdGetKRCB()->setBusinessOK( FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_ACTIVE, "_clsMgr::active" )
   INT32 _clsMgr::active ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_ACTIVE ) ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() )
      {
         rc = pmdGetKRCB()->getDMSCB()->regHandler( &_recycleBinMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register event handler of "
                      "recycle bin manager to DMS, rc: %d", rc ) ;
      }

      if ( pmdGetStartup().isOK() )
      {
         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         if ( NULL != dmsCB && NULL != dpsCB )
         {
            DPS_LSN_OFFSET maxLSN = DPS_INVALID_LSN_OFFSET ;
            DPS_LSN expectLSN = dpsCB->expectLsn() ;
            if ( 0 == expectLSN.version && 0 == expectLSN.offset )
            {
               rc = dmsCB->getMaxDMSLSN( maxLSN ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get max dms lsn:rc=%d",
                            rc ) ;

               if ( DPS_INVALID_LSN_OFFSET != maxLSN
                    && expectLSN.offset < maxLSN )
               {
                  DPS_LSN newDPSLSN = expectLSN ;
                  // make sure newDPSLSN.offset is 4 byte align
                  newDPSLSN.offset = ossAlign4( maxLSN )
                           + ossAlign4( (UINT32)sizeof( dpsLogRecordHeader ) ) ;
                  if ( DPS_INVALID_LSN_VERSION == newDPSLSN.version )
                  {
                     newDPSLSN.version = DPS_INVALID_LSN_VERSION + 1 ;
                  }

                  /// clear transinfo
                  sdbGetTransCB()->clearTransInfo() ;
                  /// then move to new dps lsn
                  rc = dpsCB->move( newDPSLSN.offset, newDPSLSN.version ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to move(%lld:%lld)",
                               newDPSLSN.version, newDPSLSN.offset ) ;

                  PD_LOG( PDEVENT, "Move new lsn(%lld:%lld) succeed",
                          newDPSLSN.version, newDPSLSN.offset ) ;
               }
            }
         }
      }

      // 1. start cls edu and shard edu
      _attachEvent.reset() ;
      rc = _startEDU ( EDU_TYPE_CLUSTER, PMD_EDU_UNKNOW,
                       (_pmdObjBase*)this, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _attachEvent.wait( CLS_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait cluster edu attach failed, rc: %d", rc ) ;

      _attachEvent.reset() ;
      rc = _startEDU ( EDU_TYPE_CLUSTERSHARD, PMD_EDU_UNKNOW,
                       (_pmdObjBase*)getShardCB(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _attachEvent.wait( CLS_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait cluster-shard attach failed, rc: %d",
                   rc ) ;

      // Start log notify
      rc = _startEDU( EDU_TYPE_CLSLOGNTY, PMD_EDU_UNKNOW,
                      (_pmdObjBase*)getReplCB(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      // 2. start network daemons for shard/repl reader
      rc = _startEDU ( EDU_TYPE_SHARDR, PMD_EDU_RUNNING,
                       (netRouteAgent*)getShardRouteAgent(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _startEDU ( EDU_TYPE_REPR, PMD_EDU_RUNNING,
                       (netRouteAgent*)getReplRouteAgent(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      /// start task background
      if ( _pShardAdapter )
      {
         rc = schedStartPrepareJob( _pShardAdapter, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Start task-prepare job failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = schedStartDispatchJob( _pShardAdapter,
                                     &_shardSessionMgr,
                                     NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Start task-dispatch job failed, rc: %d", rc ) ;
            goto error ;
         }
      }

      // 3. set timer
      _oneSecTimerID = setTimer( CLS_REPL, OSS_ONE_SEC ) ;

      if ( CLS_INVALID_TIMERID == _oneSecTimerID )
      {
         PD_LOG ( PDERROR, "Register one seccond timer failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _taskTimerID = setTimer( CLS_REPL, 0xFFFFFFFF ) ;

      if ( CLS_INVALID_TIMERID == _taskTimerID )
      {
         PD_LOG ( PDERROR, "Register task timer failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _regTimerID = setTimer( CLS_SHARD, OSS_ONE_SEC ) ;

      if ( CLS_INVALID_TIMERID == _regTimerID )
      {
         PD_LOG ( PDERROR, "Register timer failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // 4. send register msg
      _sendRegisterMsg () ;

      // Start storage check job only for data nodes
      if ( SDB_ROLE_DATA == pmdGetKRCB()->getDBRole() )
      {
         rc = startStorageCheckJob( NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Start storage checking job thread failed, rc: %d",
                      rc ) ;

         rc = _recycleBinMgr.startBGJob() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start background job for "
                      "recycle bin manager, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_ACTIVE, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::deactive ()
   {
      // 1. members to deactive
      if ( _replObj )
      {
         _replObj->deactive() ;
      }
      if ( _shdObj )
      {
         _shdObj->deactive() ;
      }

      // 2. stop listen
      if ( _replNetRtAgent )
      {
         _replNetRtAgent->shutdownListen() ;
      }
      if ( _shardNetRtAgent )
      {
         _shardNetRtAgent->shutdownListen() ;
      }

      // 3. stop io
      if ( _replNetRtAgent )
      {
         _replNetRtAgent->stop() ;
      }
      if ( _shardNetRtAgent )
      {
         _shardNetRtAgent->stop() ;
      }

      _shardSessionMgr.setForced() ;
      _replSessionMgr.setForced() ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() )
      {
         pmdGetKRCB()->getDMSCB()->unregHandler( &_recycleBinMgr ) ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_FINAL, "_clsMgr::fini" )
   INT32 _clsMgr::fini ()
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_FINAL ) ;

      _shardSessionMgr.fini() ;
      _replSessionMgr.fini() ;

      if ( _shdObj )
      {
         _shdObj->final() ;
      }
      if ( _replObj )
      {
         _replObj->final() ;
      }

      _remoteSessionMgr.fini() ;

      if ( NULL != _pResource )
      {
         _pResource->fini() ;
      }

      if ( _pShardAdapter )
      {
         _pShardAdapter->fini() ;
      }
      if ( _pContainerMgr )
      {
         _pContainerMgr->fini() ;
      }

      /// release object
      SAFE_OSS_DELETE( _replObj ) ;
      SAFE_OSS_DELETE( _shdObj ) ;
      SAFE_OSS_DELETE( _shardNetRtAgent ) ;
      SAFE_OSS_DELETE( _pSitePropMgr ) ;
      SAFE_OSS_DELETE( _pResource ) ;
      SAFE_OSS_DELETE( _replNetRtAgent ) ;
      SAFE_OSS_DELETE( _replTimerHandler ) ;
      SAFE_OSS_DELETE( _shdTimerHandler ) ;
      SAFE_OSS_DELETE( _replMsgHandlerObj ) ;
      SAFE_OSS_DELETE( _shdMsgHandlerObj ) ;
      SAFE_OSS_DELETE( _pShardAdapter ) ;
      SAFE_OSS_DELETE( _pContainerMgr ) ;

      _mapTaskQuery.clear() ;
      _mapTaskID.clear() ;
      _vecInnerSessionParam.clear() ;

      _recycleBinMgr.fini() ;

      PD_TRACE_EXIT ( SDB__CLSMGR_FINAL );
      return SDB_OK ;
   }

   void _clsMgr::onConfigChange ()
   {
      _shardSessionMgr.onConfigChange() ;

      _shdObj->onConfigChange() ;
      _replObj->onConfigChange() ;
   }

   void* _clsMgr::queryInterface( SDB_INTERFACE_TYPE type )
   {
      if ( SDB_IF_CLS == type && _replObj )
      {
         return dynamic_cast<ICluster*>( _replObj ) ;
      }
      return IControlBlock::queryInterface( type ) ;
   }

   void _clsMgr::attachCB ( pmdEDUCB *pMainCB )
   {
      if ( EDU_TYPE_CLUSTER == pMainCB->getType() )
      {
         //Set MsgHandler EDU
         _shdMsgHandlerObj->attach ( pMainCB ) ;
         _replMsgHandlerObj->attach ( pMainCB ) ;

         //Set TimerHandler EDU
         _shdTimerHandler->attach ( pMainCB ) ;
         _replTimerHandler->attach ( pMainCB ) ;
      }
      else if ( EDU_TYPE_CLUSTERSHARD == pMainCB->getType() )
      {
         _shdMsgHandlerObj->attachShardCB( pMainCB ) ;
      }

      _attachEvent.signalAll() ;
   }

   void _clsMgr::detachCB( pmdEDUCB *pMainCB )
   {
      if ( EDU_TYPE_CLUSTER == pMainCB->getType() )
      {
         //Set MsgHandler EDU
         _shdMsgHandlerObj->detach() ;
         _replMsgHandlerObj->detach () ;

         //Set TimerHandler EDU
         _shdTimerHandler->detach () ;
         _replTimerHandler->detach () ;
      }
      else if ( EDU_TYPE_CLUSTERSHARD == pMainCB->getType() )
      {
         _shdMsgHandlerObj->detachShardCB() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__STRATEDU, "_clsMgr::_startEDU" )
   INT32 _clsMgr::_startEDU ( INT32 type, EDU_STATUS waitStatus,
                              void *agrs, BOOLEAN regSys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__STRATEDU );
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      pmdEDUMgr *pEDUMgr = pKRCB->getEDUMgr () ;

      //Start EDU
      rc = pEDUMgr->startEDU( (EDU_TYPES)type, (void *)agrs, &eduID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create EDU[type:%d(%s)], rc = %d",
                  type, getEDUName( (EDU_TYPES)type ), rc );
         goto error ;
      }

      //Wait edu running
      if ( PMD_EDU_UNKNOW != waitStatus )
      {
         rc = pEDUMgr->waitUntil( eduID, waitStatus ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to wait EDU[type:%d(%s)] to "
                    "status[%d(%s)], rc: %d", type,
                    getEDUName( (EDU_TYPES)type ), waitStatus,
                    getEDUStatusDesp( waitStatus ), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR__STRATEDU, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONPRMCHG, "_clsMgr::ntyPrimaryChange" )
   void _clsMgr::ntyPrimaryChange( BOOLEAN primary,
                                   SDB_EVENT_OCCUR_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONPRMCHG );
      SDB_DMSCB* pDmsCB = pmdGetKRCB()->getDMSCB() ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         PD_LOG ( PDEVENT, "Node change to [%s]",
                  primary ? "Primary" : "Secondary" ) ;
      }

      // let's ignore the event if the node is still starting up
      if ( !pmdGetStartup().isOK() ||
           _shdObj->getDCMgr()->getDCBaseInfo()->isReadonly() ||
           !_shdObj->getDCMgr()->getDCBaseInfo()->isActivated() )
      {
         return ;
      }

      // if we are switching to primary, let's increase log version BEFORE
      // it actually happen
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         // inc dps log version
         if ( getReplCB()->isInCriticalMode() && getReplCB()->isInEnforcedGrpMode() )
         {
            sdbGetDPSCB()->incVersion( DPS_INC_VER_CRITICAL ) ;
         }
         else
         {
            sdbGetDPSCB()->incVersion( DPS_INC_VER_DFT ) ;
         }

         // start namecheck
         if ( SDB_ROLE_DATA == pmdGetDBRole() )
         {
            clsStartRenameCheckJobs() ;
         }
      }
      // if we are switching to slave, let's interrupt all EDUs that doing write
      // or transaction
      else if ( !primary )
      {
         // interrupt both before and after
         sdbGetDPSCB()->cancelIncVersion() ;

         // interrupt writing and transaction EDUs
         pmdGetKRCB()->getEDUMgr()->interruptWritingAndTransEDUs(
                                                      SDB_CLS_NOT_PRIMARY ) ;
      }

      // notify sub members
      getShardCB()->ntyPrimaryChange( primary, type ) ;
      getReplCB()->ntyPrimaryChange( primary, type ) ;

      // for "post trigger" event
      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         if ( primary )
         {
            if ( SDB_ROLE_DATA == pmdGetDBRole() )
            {
               if ( pDmsCB->nullCSUniqueIDCnt() > 0 )
               {
                  startUniqueIDCheckJob() ;
               }
               // set configure invalid, so the recycle bin manager
               // will update configure from CATALOG later
               _recycleBinMgr.setConfInvalid() ;

               // start query task
               startAllTaskCheck() ;
            }

            clsVoteMachine *vote = getReplCB()->voteMachine() ;
            // Start critical mode monitor
            if ( CLS_GROUP_MODE_CRITICAL == getReplCB()->getGrpMode() &&
                 ! vote->isTmpGrpMode() )
            {
               vote->startCriticalModeMonitor() ;
            }
            else if ( CLS_GROUP_MODE_MAINTENANCE == getReplCB()->getGrpMode() )
            {
               vote->startMaintenanceModeMonitor() ;
            }
         }
         else
         {
            // clean up all query task
            ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
            _mapTaskQuery.clear () ;
         }
      }

      // call other handler
      pmdGetKRCB()->callPrimaryChangeHandler( primary, type ) ;

      PD_TRACE_EXIT ( SDB__CLSMGR__ONPRMCHG );
   }

   const CHAR *_clsMgr::getShardServiceName () const
   {
      return _shdServiceName ;
   }
   const CHAR *_clsMgr::getReplServiceName () const
   {
      return _replServiceName ;
   }
   NodeID _clsMgr::getNodeID () const
   {
      return _selfNodeID ;
   }
   UINT16 _clsMgr::getShardServiceID () const
   {
      return _shardServiceID ;
   }
   UINT16 _clsMgr::getReplServiceID () const
   {
      return _replServiceID ;
   }

   _netRouteAgent *_clsMgr::getShardRouteAgent ()
   {
      return _shardNetRtAgent ;
   }
   _netRouteAgent *_clsMgr::getReplRouteAgent ()
   {
      return _replNetRtAgent ;
   }
   shardCB *_clsMgr::getShardCB ()
   {
      return _shdObj ;
   }
   replCB *_clsMgr::getReplCB ()
   {
      return _replObj ;
   }
   catAgent *_clsMgr::getCatAgent ()
   {
      return _shdObj->getCataAgent() ;
   }
   nodeMgrAgent* _clsMgr::getNodeMgrAgent ()
   {
      return _shdObj->getNodeMgrAgent() ;
   }
   shdMsgHandler* _clsMgr::getShardMsgHandle()
   {
      return _shdMsgHandlerObj ;
   }
   _clsTaskMgr* _clsMgr::getTaskMgr()
   {
      return &_taskMgr ;
   }
   ossEvent* _clsMgr::getTaskEvent()
   {
      return &_taskEvent ;
   }
   BOOLEAN _clsMgr::isPrimary ()
   {
      return _replObj->primaryIsMe () ;
   }
   INT32 _clsMgr::clearAllData ()
   {
      return _shdObj->clearAllData () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_INVALIDCACHE, "_clsMgr::invalidateCache" )
   INT32 _clsMgr::invalidateCache ( const CHAR * name, UINT8 type )
   {
      INT32 rc = SDB_CLS_NOT_PRIMARY ;

      PD_TRACE_ENTRY ( SDB__CLSMGR_INVALIDCACHE );

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      UINT32 logRecSize = 0 ;

      if ( isPrimary() )
      {
         /// write sync cata info log
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET, NULL ) ;
         dpsLogRecord &record = info.getMergeBlock().record() ;
         rc = dpsInvalidCata2Record( type, name, NULL, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build invalid-cata log:%d",rc ) ;
            goto error ;
         }

         logRecSize = record.alignedLen() ;
         rc = transCB->reservedLogSpace( logRecSize, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space for "
                    "invalid-cata log, rc: %d", rc ) ;
            logRecSize = 0 ;
            goto error ;
         }

         rc = dpsCB->prepare(info ) ;
         if ( SDB_OK == rc )
         {
            dpsCB->writeData( info ) ;
         }
      }

   done:
      if ( 0 != logRecSize )
      {
         transCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR_INVALIDCACHE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_INVDATACAT, "_clsMgr::invalidateCata" )
   INT32 _clsMgr::invalidateCata( const CHAR * name )
   {
      INT32 rc = SDB_CLS_NOT_PRIMARY ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_INVDATACAT );

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      UINT32 logRecSize = 0 ;

      if ( isPrimary() )
      {
         /// write sync cata info log
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET, NULL ) ;
         dpsLogRecord &record = info.getMergeBlock().record() ;
         UINT8 type = DPS_LOG_INVALIDCATA_TYPE_CATA ;
         rc = dpsInvalidCata2Record( type, name, NULL, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build invalid-cata log:%d",rc ) ;
            goto error ;
         }

         logRecSize = record.alignedLen() ;
         rc = transCB->reservedLogSpace( logRecSize, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space for "
                    "invalid-cata log, rc: %d", rc ) ;
            logRecSize = 0 ;
            goto error ;
         }

         rc = dpsCB->prepare(info ) ;
         if ( SDB_OK == rc )
         {
            dpsCB->writeData( info ) ;
         }
      }

   done:
      if ( 0 != logRecSize )
      {
         transCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR_INVDATACAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_INVALIDSTAT, "_clsMgr::invalidateStatistics" )
   INT32 _clsMgr::invalidateStatistics ()
   {
      INT32 rc = SDB_CLS_NOT_PRIMARY ;

      PD_TRACE_ENTRY( SDB__CLSMGR_INVALIDSTAT ) ;

      if ( isPrimary() &&
           SDB_ROLE_DATA == pmdGetDBRole() )
      {
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         rc = rtnAnalyzeDpsLog( NULL, NULL, NULL, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write analyze log, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSMGR_INVALIDSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_INVALIDPLAN, "_clsMgr::invalidatePlan" )
   INT32 _clsMgr::invalidatePlan( const CHAR * name )
   {
      INT32 rc = SDB_CLS_NOT_PRIMARY ;

      PD_TRACE_ENTRY ( SDB__CLSMGR_INVALIDPLAN );

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      UINT32 logRecSize = 0 ;

      if ( isPrimary() )
      {
         /// write sync cata info log
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET, NULL ) ;
         dpsLogRecord &record = info.getMergeBlock().record() ;
         UINT8 type = DPS_LOG_INVALIDCATA_TYPE_PLAN ;
         rc = dpsInvalidCata2Record( type, name, NULL, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build invalid-cata log:%d",rc ) ;
            goto error ;
         }

         logRecSize = record.alignedLen() ;
         rc = transCB->reservedLogSpace( logRecSize, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space for "
                    "invalid-cata log, rc: %d", rc ) ;
            logRecSize = 0 ;
            goto error ;
         }

         rc = dpsCB->prepare(info ) ;
         if ( SDB_OK == rc )
         {
            dpsCB->writeData( info ) ;
         }
      }

   done:
      if ( 0 != logRecSize )
      {
         transCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR_INVALIDPLAN, rc );
      return rc ;

   error:
      goto done ;
   }

   void _clsMgr::dumpSchedInfo( BSONObjBuilder &builder )
   {
      if ( _pShardAdapter )
      {
         _pShardAdapter->dump( builder ) ;
      }
      else
      {
         builder.append( FIELD_NAME_SCHDLR_TYPE, ( INT32 )SCHED_TYPE_NONE ) ;
         builder.append( FIELD_NAME_SCHDLR_TYPE_DESP,
                         schedType2String( SCHED_TYPE_NONE ) ) ;
         builder.append( FIELD_NAME_RUN,
                        (INT32)_shardSessionMgr.getTaskInfo()->getRunTaskNum() ) ;
         builder.append( FIELD_NAME_WAIT, 0 ) ;
         builder.append( FIELD_NAME_SCHDLR_MGR_EVT_NUM, 0 ) ;
         builder.append( FIELD_NAME_SCHDLR_TIMES, (INT64)0 ) ;
      }
   }

   void _clsMgr::resetDumpSchedInfo()
   {
      if ( _pShardAdapter )
      {
         _pShardAdapter->resetDump() ;
      }
   }

   // Register async internal sessions
   // The function itself doesn't start session. Instead the function place
   // a request in _vecInnerSessionParam vector so that another daemon will
   // create a background inernal sessions afterwards
   // By default the daemon will be triggered every single seconds to detect
   // if the queue is empty or not
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STARTINSN, "_clsMgr::startInnerSession" )
   INT32 _clsMgr::startInnerSession ( INT32 type, INT32 innerTID, void *data )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_STARTINSN );
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;

      _innerSessionInfo info ;
      info.type = type ;
      info.startType = PMD_SESSION_ACTIVE ;
      info.innerTid = innerTID ;
      info.data = data ;
      info.sessionID = ossPack32To64 ( _selfNodeID.columns.nodeID, innerTID ) ;

      try
      {
         _vecInnerSessionParam.push_back ( info ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSMGR_STARTINSN );
      return rc ;
   }

   // Start a background task check request
   // Another daemon will be triggered every second, it will send the check
   // request to CATALOG to check for task collection
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STARTTSKCHK, "_clsMgr::startTaskCheck" )
   INT32 _clsMgr::startTaskCheck( const BSONObj &match, BOOLEAN quickPull )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_STARTTSKCHK );
      if ( !isPrimary() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
      }
      else
      {
         ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
         try
         {
            BSONObjBuilder builder ;
            builder.appendElements( match ) ;
            // ignore finished task
            builder.append( FIELD_NAME_STATUS,
                            BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;
            // ignore main task, because main task is logical task
            builder.append( FIELD_NAME_IS_MAINTASK,
                            BSON( "$exists" << 0 ) ) ;
            _mapTaskQuery[++_requestID] = builder.obj() ;

            if ( quickPull )
            {
               _postTimeoutEvent( _taskTimerID ) ;
            }
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSMGR_STARTTSKCHK );
      return rc ;
   }

   INT32 _clsMgr::startTaskCheck( UINT64 taskID, BOOLEAN isMainTask )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         // group name
         BSONArrayBuilder arrBuilder( builder.subarrayStart( "$or" ) ) ;
         arrBuilder << BSON( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME <<
                             pmdGetKRCB()->getGroupName() ) ;
         arrBuilder << BSON( FIELD_NAME_TARGETID <<
                             _selfNodeID.columns.groupID ) ;
         arrBuilder.done() ;
         // task id
         if ( isMainTask )
         {
            builder.append( FIELD_NAME_MAIN_TASKID, (INT64)taskID ) ;
         }
         else
         {
            builder.append( FIELD_NAME_TASKID, (INT64)taskID ) ;
         }

         rc = startTaskCheck( builder.done() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsMgr::startIdxTaskCheck( UINT64 taskID, BOOLEAN isMainTask,
                                     BOOLEAN quickPull )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         if ( isMainTask )
         {
            builder.append( FIELD_NAME_MAIN_TASKID, (INT64)taskID ) ;
         }
         else
         {
            builder.append( FIELD_NAME_TASKID, (INT64)taskID ) ;
         }
         builder.append( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME,
                         pmdGetKRCB()->getGroupName() ) ;

         rc = startTaskCheck( builder.done(), quickPull ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsMgr::startIdxTaskCheckByCL( utilCLUniqueID clUniqID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_UNIQUEID, (INT64)clUniqID ) ;
         builder.append( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME,
                         pmdGetKRCB()->getGroupName() ) ;

         rc = startTaskCheck( builder.done() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsMgr::startIdxTaskCheckByCS( utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME,
                         pmdGetKRCB()->getGroupName() ) ;

         rc = utilGetCSBounds( FIELD_NAME_UNIQUEID, csUniqueID, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection space bound "
                      "[%u], rc: %d", csUniqueID, rc ) ;

         rc = startTaskCheck( builder.done() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsMgr::startAllSplitTaskCheck()
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj match1 = BSON( CAT_TARGETID_NAME <<
                                _selfNodeID.columns.groupID ) ;

         rc = startTaskCheck( match1 ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsMgr::startAllIdxTaskCheck()
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj match = BSON( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME <<
                               pmdGetKRCB()->getGroupName() ) ;

         rc = startTaskCheck( match ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::startAllTaskCheck()
   {
      // pull all tasks related to this node from catalog
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj match1 = BSON( CAT_TARGETID_NAME <<
                                _selfNodeID.columns.groupID ) ;
         BSONObj match2 = BSON( FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME <<
                                pmdGetKRCB()->getGroupName() ) ;

         rc = startTaskCheck( BSON( "$or" << BSON_ARRAY( match1 << match2 ) ) ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsMgr::stopTask( UINT64 taskID )
   {
      ossScopedLock lock ( &_clsLatch, SHARED ) ;
      map< UINT64, UINT32 >::iterator it = _mapTaskID.find( taskID ) ;
      if ( it != _mapTaskID.end() )
      {
         _taskMgr.stopTask( it->second ) ;
      }
      return SDB_OK ;
   }

   INT32 _clsMgr::addTask( UINT64 taskID, UINT32 locationID )
   {
      INT32 rc = SDB_OK ;

      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;

      try
      {
         _mapTaskID[ taskID ] = locationID ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   // remove the task from local
   INT32 _clsMgr::removeTask( UINT64 taskID )
   {
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
      map< UINT64, UINT32 >::iterator it = _mapTaskID.find( taskID ) ;
      if ( it != _mapTaskID.end() )
      {
         _mapTaskID.erase( it ) ;
      }
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__DFTMSGFUNC, "_clsMgr::_defaultMsgFunc" )
   INT32 _clsMgr::_defaultMsgFunc( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__DFTMSGFUNC );
      // the msg is not mine, dispatch to sub object
      // restore the type
      INT32 type = (INT32) msg->TID ;
      INT32 opCode = msg->opCode ;
      msg->TID = 0 ;

      if ( CLS_REPL == type ||
           MSG_CAT_GRP_RES == opCode ||
           MSG_CAT_PAIMARY_CHANGE_RES == opCode ||
           MSG_CLS_GINFO_UPDATED == opCode )
      {
         rc = _replObj->dispatchMsg( handle, msg ) ;
      }
      else
      {
         rc = _shdObj->dispatchMsg ( handle, msg ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__DFTMSGFUNC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_ONTMR, "_clsMgr::onTimer" )
   void _clsMgr::onTimer ( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_ONTMR );
      //Judge the timer is myself, if not my self will dispatch to sub object
      if ( timerID == _regTimerID )
      {
         _sendRegisterMsg () ;
      }
      // if we hit one second
      else if ( timerID == _oneSecTimerID )
      {
         //Check _deqShdDeletingSessions
         _shardSessionMgr.onTimer( interval ) ;
         _replSessionMgr.onTimer( interval ) ;

         //prepare task
         _prepareTask () ;

         // if we have one or more pending tasks, and if the unshard timer
         // not started yet, let's start one
         if ( _taskMgr.taskCount() > 0 &&
              !_shardSessionMgr.isUnShardTimerStarted() )
         {
            _shardSessionMgr.startUnShardTimer( OSS_ONE_SEC ) ;
         }
         // if unshard time is already started but pending tasks are 0, let's
         // stop it
         else if ( _shardSessionMgr.isUnShardTimerStarted() &&
                   0 == _taskMgr.taskCount() )
         {
            _shardSessionMgr.stopUnShardTimer() ;
         }
      }
      else if ( timerID == _taskTimerID )
      {
         _prepareTask () ;
      }
      else
      {
         // otherwise let's extract the type from timerID, and call onTimer
         // call back functions based on the request type
         // For now we only have 2 possible types, shard or repl
         UINT32 type = 0 ;
         UINT32 netTimerID = 0 ;
         ossUnpack32From64 ( timerID, type, netTimerID ) ;
         _pmdObjBase *pSubObj = _shdObj ;
         if ( CLS_REPL == (INT32)type )
         {
            pSubObj = _replObj ;
         }

         pSubObj->onTimer ( timerID, interval ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSMGR_ONTMR );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__STARTINSN, "_clsMgr::_startInnerSession" )
   INT32 _clsMgr::_startInnerSession ( INT32 type,
                                       pmdAsycSessionMgr *pSessionMgr )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__STARTINSN );
      _pmdAsyncSession *pSession = NULL ;
      ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;

      VECINNERPARAM::iterator it = _vecInnerSessionParam.begin() ;
      // iterate for all pending internal session requests
      while ( it != _vecInnerSessionParam.end() )
      {
         _innerSessionInfo &info = *it ;
         // skip for any unmatch types or existing sessions
         // if the session already started, we simply ignore the request in
         // the list and wait for start next time
         if ( info.type != type ||
              SDB_OK == pSessionMgr->getSession( info.sessionID,
                                                 FALSE,
                                                 info.startType,
                                                 NET_INVALID_HANDLE,
                                                 FALSE, 0, NULL,
                                                 NULL ) )
         {
            ++it ;
            continue ;
         }
         // let's start the session
         rc = pSessionMgr->getSession ( info.sessionID, FALSE,
                                        info.startType,
                                        NET_INVALID_HANDLE, TRUE, 0,
                                        info.data,
                                        &pSession ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG ( PDEVENT, "Create inner session[%s] succeed",
                     pSession->sessionName() ) ;
            it = _vecInnerSessionParam.erase ( it ) ;
            continue ;
         }
         // if we get here, that means something wrong and we can't start
         // the session
         PD_LOG ( PDERROR, "Create inner session[TID:%d] failed, rc: %d",
                  info.innerTid, rc ) ;
         ++it ;
      }

      PD_TRACE_EXITRC ( SDB__CLSMGR__STARTINSN, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__PREPTASK, "_clsMgr::_prepareTask" )
   INT32 _clsMgr::_prepareTask ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__PREPTASK );
      ossScopedLock lock ( &_clsLatch, SHARED ) ;
      MAPTASKQUERY::iterator it = _mapTaskQuery.begin () ;
      while ( it != _mapTaskQuery.end() )
      {
         // send query msg to catalog
         rc = _sendQueryTaskReq ( it->first, "CAT", &(it->second) ) ;
         if ( SDB_OK != rc )
         {
            break ;
         }
         ++it ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__PREPTASK, rc );
      return rc ;
   }

   void _clsMgr::_postTimeoutEvent( UINT64 timerID )
   {
      UINT32 type = 0 ;
      UINT32 netTimerID = 0 ;

      ossUnpack32From64( timerID, type, netTimerID ) ;

      if ( CLS_SHARD == type )
      {
         _shdTimerHandler->handleTimeout( 0, netTimerID ) ;
      }
      else if ( CLS_REPL == type )
      {
         _replTimerHandler->handleTimeout( 0, netTimerID ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Invalid timerID" ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STARTTSKTH, "_clsMgr::startTaskThread" )
   INT32 _clsMgr::startTaskThread ( const BSONObj &taskObj, UINT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_STARTTSKTH ) ;

      _clsTask *pTask = NULL ;
      BOOLEAN alreadyExist = FALSE ;
      UINT32 locationID = CLS_INVALID_LOCATIONID ;
      BOOLEAN addTaskDone1 = FALSE ;
      BOOLEAN addTaskDone2 = FALSE ;
      const CHAR* groupName = pmdGetKRCB()->getGroupName() ;

      // new clsTask
      rc = clsNewTask( taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task, rc: %d",
                   rc ) ;
      taskID = pTask->taskID() ;

      if ( CLS_TASK_STATUS_FINISH == pTask->status() )
      {
         PD_LOG( PDINFO,
                 "Task[%llu] has been finished, do not start thread",
                 taskID ) ;
         // release memory in goto error
         goto error ;
      }
      else if ( CLS_TASK_STATUS_FINISH ==
                pTask->getTaskStatusByGroup( groupName ) )
      {
         PD_LOG( PDINFO, "Task[%llu] on group[%s] has been finished, "
                 "do not start thread", taskID, groupName ) ;
         // release memory in goto error
         goto error ;
      }

      // add to clsTaskMgr and clsMgr
      locationID = _taskMgr.getLocationID() ;

      rc = _taskMgr.addTask( pTask, locationID, &alreadyExist ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add task[%llu] to manager, location ID: %u, "
                   "rc: %d", taskID, locationID, rc ) ;
      addTaskDone1 = TRUE ;

      PD_CHECK( !alreadyExist, SDB_CLS_MUTEX_TASK_EXIST, error, PDERROR,
                "Failed to add task[%llu] to manager, location ID: %u, rc: %d",
                taskID, locationID, rc ) ;

      rc = addTask( taskID, locationID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add task[%llu] to map, location ID: %u, rc: %d",
                   taskID, locationID, rc ) ;
      addTaskDone2 = TRUE ;

      // start session
      switch ( pTask->taskType() )
      {
         case CLS_TASK_SPLIT :
         {
            rc = startInnerSession ( CLS_SHARD, locationID, (void *)pTask ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to start inner session for task[%llu], rc: %d",
                         taskID, rc ) ;
            break ;
         }
         case CLS_TASK_CREATE_IDX :
         {
            rc = clsStartIndexJob( RTN_JOB_CREATE_INDEX, locationID,
                                   (clsIdxTask*)pTask ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to start index job for task[%llu], rc: %d",
                         taskID, rc ) ;
            break ;
         }
         case CLS_TASK_DROP_IDX :
         {
            rc = clsStartIndexJob( RTN_JOB_DROP_INDEX, locationID,
                                   (clsIdxTask*)pTask ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to start index job for task[%llu], rc: %d",
                         taskID, rc ) ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Unknown task type[%d]", pTask->taskType() ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_STARTTSKTH, rc );
      return rc ;
   error:
      {
         if ( addTaskDone2 )
         {
            removeTask( taskID ) ;
         }
         BOOLEAN hasReleased = FALSE ;
         if ( addTaskDone1 )
         {
            _taskMgr.removeTask( locationID, &hasReleased ) ;
         }
         if ( pTask && FALSE == hasReleased )
         {
            clsReleaseTask( pTask ) ;
         }
      }
      goto done ;
   }

   BOOLEAN _clsMgr::_findAndCheckTaskStatus( UINT64 taskID,
                                             dmsTaskStatusPtr &statusPtr,
                                             BOOLEAN &needRollback )
   {
      dmsTaskStatusMgr *pStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;
      needRollback = FALSE ;

      BOOLEAN foundOut = pStatMgr->findItem( taskID, statusPtr ) ;
      if ( foundOut )
      {
         if ( DMS_TASK_STATUS_ROLLBACK == statusPtr->status() ||
              DMS_TASK_STATUS_CANCELED == statusPtr->status() )
         {
            needRollback = FALSE ;
         }
         else if ( DMS_TASK_STATUS_FINISH == statusPtr->status() )
         {
            // If resultCode is OK, it need rollback
            if ( SDB_OK == statusPtr->resultCode() )
            {
               needRollback = TRUE ;
            }
            else
            {
               needRollback = FALSE ;
            }
         }
         else if ( DMS_TASK_STATUS_RUN   == statusPtr->status() ||
                   DMS_TASK_STATUS_READY == statusPtr->status() )
         {
            // If local task is ready, maybe group status of catalog task is
            // run, so we also need to rollback.
            needRollback = TRUE ;
         }
         else
         {
            needRollback = FALSE ;
         }
      }

      return foundOut ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_STRBTH, "_clsMgr::startRollbackTaskThread" )
   INT32 _clsMgr::startRollbackTaskThread ( UINT64 taskID,
                                            BOOLEAN isCancel )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_STRBTH );

      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;
      _clsTask *pTask = NULL ;
      BOOLEAN alreadyExist = FALSE ;
      BSONObj taskObj ;
      BOOLEAN addTaskDone = FALSE ;
      UINT32 locationID = CLS_INVALID_LOCATIONID ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      BOOLEAN foundOut = FALSE ;
      BOOLEAN needRollback = FALSE ;

      PD_LOG( PDINFO, "Start rollback thread for task[%llu]", taskID ) ;

      // find out idxTaskStatus
      dmsTaskStatusPtr statusPtr ;
      foundOut = _findAndCheckTaskStatus( taskID, statusPtr, needRollback ) ;
      PD_CHECK( foundOut, SDB_SYS, error, PDERROR,
                "Failed to find task status[%llu]", taskID ) ;
      if ( !needRollback )
      {
         goto done ;
      }

      // we cann't rollback drop index, only can rollback create index
      PD_CHECK( statusPtr->taskType() == DMS_TASK_CREATE_IDX,
                SDB_SYS, error, PDERROR,
                "Invalid task type[%d]", statusPtr->taskType() ) ;
      taskType = CLS_TASK_DROP_IDX ;

      {
      dmsIdxTaskStatusPtr idxStatPtr =
                     boost::dynamic_pointer_cast<dmsIdxTaskStatus>(statusPtr) ;
      PD_CHECK( idxStatPtr, SDB_SYS, error, PDERROR,
                "Failed to convert task status pointer" ) ;

      // new task
      while( !idxStatPtr->isInitialized() )
      {
         if ( cb && cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
         ossSleep( OSS_ONE_SEC ) ;
      }
      taskObj = idxStatPtr->toBSON() ;

      rc = clsNewTask( taskType, taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task[type: %d], rc: %d",
                   taskType, rc ) ;

      // add to taskMgr, NOT need to add to _mapTaskID
      locationID = idxStatPtr->locationID() ;

      rc = _taskMgr.addTask( pTask, locationID, &alreadyExist ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add task[%llu] to manager, location ID: %u, "
                   "rc: %d", taskID, locationID, rc ) ;
      addTaskDone = TRUE ;

      if ( alreadyExist )
      {
         // pTask is NOT added to map, it is useless, so release it
         clsReleaseTask( pTask ) ;
      }

      // start session
      rc = clsStartRollbackIndexJob( RTN_JOB_DROP_INDEX, idxStatPtr, isCancel ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start rollback thread[%s], rc: %d",
                 taskObj.toString().c_str(), rc ) ;
         goto error ;
      }
      PD_LOG( PDINFO, "Start rollback thread, taskID[%llu] taskObj[%s]",
              taskID, taskObj.toString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_STRBTH, rc );
      return rc ;
   error:
      {
         BOOLEAN hasReleased = FALSE ;
         if ( addTaskDone )
         {
            _taskMgr.removeTask( locationID, &hasReleased ) ;
         }
         if ( pTask && FALSE == hasReleased )
         {
            clsReleaseTask( pTask ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_RESTRTH, "_clsMgr::restartTaskThread" )
   INT32 _clsMgr::restartTaskThread ( UINT64 taskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR_RESTRTH );

      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;
      _clsTask *pTask = NULL ;
      BOOLEAN alreadyExist = FALSE ;
      BSONObj taskObj ;
      BOOLEAN addTaskDone = FALSE ;
      UINT32 locationID = CLS_INVALID_LOCATIONID ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      BOOLEAN foundOut = FALSE ;
      dmsTaskStatusMgr *pStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;
      RTN_JOB_TYPE jobType = RTN_JOB_MAX ;

      PD_LOG( PDINFO, "Restart thread for task[%llu]", taskID ) ;

      // find out idxTaskStatus
      dmsTaskStatusPtr statusPtr ;
      foundOut = pStatMgr->findItem( taskID, statusPtr ) ;
      PD_CHECK( foundOut, SDB_SYS, error, PDERROR,
                "Failed to find task status[%llu]", taskID ) ;

      PD_CHECK( statusPtr->taskType() == DMS_TASK_CREATE_IDX ||
                statusPtr->taskType() == DMS_TASK_DROP_IDX,
                SDB_SYS, error, PDERROR,
                "Invalid task type[%d]", statusPtr->taskType() ) ;

      if ( DMS_TASK_CREATE_IDX == statusPtr->taskType() )
      {
         jobType = RTN_JOB_CREATE_INDEX ;
         taskType = CLS_TASK_CREATE_IDX ;
      }
      else if ( DMS_TASK_DROP_IDX == statusPtr->taskType() )
      {
         jobType = RTN_JOB_DROP_INDEX ;
         taskType = CLS_TASK_DROP_IDX ; ;
      }

      {
      dmsIdxTaskStatusPtr idxStatPtr =
                     boost::dynamic_pointer_cast<dmsIdxTaskStatus>(statusPtr) ;
      PD_CHECK( idxStatPtr, SDB_SYS, error, PDERROR,
                "Failed to convert task status pointer" ) ;

      // new task
      while( !idxStatPtr->isInitialized() )
      {
         if ( cb && cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
         ossSleep( OSS_ONE_SEC ) ;
      }
      taskObj = idxStatPtr->toBSON() ;

      rc = clsNewTask( taskType, taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task[type: %d], rc: %d",
                   taskType, rc ) ;

      // add to taskMgr, NOT need to add to _mapTaskID
      locationID = idxStatPtr->locationID() ;

      rc = _taskMgr.addTask( pTask, locationID, &alreadyExist ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add task[%llu] to manager, location ID: %u, "
                   "rc: %d", taskID, locationID, rc ) ;
      addTaskDone = TRUE ;

      if ( alreadyExist )
      {
         // pTask is NOT added to map, it is useless, so release it
         clsReleaseTask( pTask ) ;
      }

      // start session
      rc = clsRestartIndexJob( jobType, idxStatPtr ) ;
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to restart thread for task[%llu], taskObj[%s], rc: %d",
                 taskID, taskObj.toString().c_str(), rc ) ;
         goto error ;
      }
      PD_LOG( PDINFO, "Restart thread for task[%llu], taskObj[%s]",
              taskID, taskObj.toString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR_RESTRTH, rc );
      return rc ;
   error:
      {
         BOOLEAN hasReleased = FALSE ;
         if ( addTaskDone )
         {
            _taskMgr.removeTask( locationID, &hasReleased ) ;
         }
         if ( pTask && FALSE == hasReleased )
         {
            clsReleaseTask( pTask ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_SETTMR, "_clsMgr::setTimer" )
   UINT64 _clsMgr::setTimer ( CLS_MEMBER_TYPE type, UINT32 milliSec )
   {
      UINT64 rc;
      PD_TRACE_ENTRY ( SDB__CLSMGR_SETTMR );
      UINT32 timeID = 0 ;
      _netTimeoutHandler * pHandler = _shdTimerHandler ;
      _netRouteAgent * pRtAgent = _shardNetRtAgent ;

      if ( CLS_REPL == type )
      {
         pHandler = _replTimerHandler ;
         pRtAgent = _replNetRtAgent ;
      }

      if ( pRtAgent->addTimer( milliSec, pHandler, timeID ) == SDB_OK )
      {
         rc = ossPack32To64( (UINT32)type, timeID ) ;
      }
      else
      {
         rc = CLS_INVALID_TIMERID ;
      }
      PD_TRACE1( SDB__CLSMGR_SETTMR, PD_PACK_ULONG(rc) );
      PD_TRACE_EXIT ( SDB__CLSMGR_SETTMR );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR_KILLTMR, "_clsMgr::killTimer" )
   void _clsMgr::killTimer( UINT64 timerID )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR_KILLTMR );
      UINT32 type = 0 ;
      UINT32 netTimerID = 0 ;

      ossUnpack32From64 ( timerID, type, netTimerID ) ;

      _netRouteAgent * pRtAgent = _shardNetRtAgent ;

      if ( CLS_REPL == (INT32)type )
      {
         pRtAgent = _replNetRtAgent ;
      }

      pRtAgent->removeTimer( netTimerID ) ;
      PD_TRACE_EXIT ( SDB__CLSMGR_KILLTMR );
   }

   INT32 _clsMgr::sendToCatlog ( MsgHeader * msg )
   {
      return _shdObj->sendToCatlog ( msg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__SNDREGMSG, "_clsMgr::_sendRegisterMsg" )
   INT32 _clsMgr::_sendRegisterMsg ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__SNDREGMSG );
      clsRegAssit regAssit ;
      UINT32 length = 0 ;
      CHAR *buff = NULL ;
      MsgCatRegisterReq *pReq = NULL ;
      BSONObj regObj ;

      rc = regAssit.buildRequestObj( regObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build request obj, rc: %d", rc ) ;

      length = regObj.objsize () + sizeof ( MsgCatRegisterReq ) ;

      // free by end of the function
      buff = (CHAR *)SDB_THREAD_ALLOC( length ) ;
      if ( buff == NULL )
      {
         PD_LOG ( PDERROR, "Failed to allocate memroy for register req" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pReq = (MsgCatRegisterReq*)buff ;
      pReq->header.messageLength = length ;
      pReq->header.opCode = MSG_CAT_REG_REQ ;
      pReq->header.requestID = 0 ;
      pReq->header.TID = 0 ;
      pReq->header.routeID.value = 0 ;
      ossMemcpy( pReq->data, regObj.objdata(), regObj.objsize() ) ;

      rc = sendToCatlog( (MsgHeader *) pReq ) ;
      PD_LOG ( PDDEBUG, "Send node register[rc: %d]", rc ) ;

   done:
      if ( buff )
      {
         SDB_THREAD_FREE ( buff ) ;
         buff = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__SNDREGMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__SNDQTSKREQ, "_clsMgr::_sendQueryTaskReq" )
   INT32 _clsMgr::_sendQueryTaskReq ( UINT64 requestID, const CHAR * clFullName,
                                      const BSONObj* match )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__SNDQTSKREQ );
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *msg = NULL ;
      INT32 rc = SDB_OK ;

      rc = msgBuildQueryMsg ( &pBuff, &buffSize, clFullName, 0, requestID,
                              0, -1, match, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      msg = ( MsgHeader* )pBuff ;
      msg->opCode = MSG_CAT_QUERY_TASK_REQ ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;

      // send msg
      rc = sendToCatlog( msg ) ;
      PD_LOG ( PDDEBUG,
               "Send MSG_CAT_QUERY_TASK_REQ[%s] [requestID:%llu] to catalog"
               "[rc:%d]", match->toString().c_str(), requestID, rc ) ;
   done:
      if ( pBuff )
      {
         SDB_OSS_FREE ( pBuff ) ;
         pBuff = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSMGR__SNDQTSKREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::updateCatGroup ( INT64 millisec )
   {
      return _shdObj->updateCatGroup ( millisec ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__UPDATEDCINFO, "_clsMgr::_updateDCInfo" )
   INT32 _clsMgr::_updateDCInfo( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSMGR__UPDATEDCINFO ) ;

      try
      {
         /// update dc base info
         BSONObj msgObject ( MSG_GET_INNER_REPLY_DATA( msg ) ) ;
         if ( msgIsInnerOpReply( msg ) &&
              msg->messageLength > (INT32)sizeof( MsgOpReply ) +
              msgObject.objsize() + 5 )
         {
            MsgOpReply *pReply = ( MsgOpReply* )msg ;
            if ( pReply->numReturned > 1 )
            {
               clsDCBaseInfo *pInfo = _shdObj->getDCMgr()->getDCBaseInfo() ;
               BSONObj objDCInfo( ( const CHAR* )msg + sizeof( MsgOpReply ) +
                                  ossAlign4( (UINT32)msgObject.objsize() ) ) ;
               _shdObj->getDCMgr()->updateDCBaseInfo( objDCInfo ) ;

               _recycleBinMgr.setConf( pInfo->getRecycleBinConf() ) ;

               pmdGetKRCB()->setDBReadonly( pInfo->isReadonly() ) ;
               pmdGetKRCB()->setDBDeactivated( !pInfo->isActivated() ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse DC info, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSMGR__UPDATEDCINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   //message function
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONCATREGRES, "_clsMgr::_onCatRegisterRes" )
   INT32 _clsMgr::_onCatRegisterRes ( NET_HANDLE handle, MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONCATREGRES );
      UINT32 groupID = INVALID_GROUPID ;
      UINT16 nodeID = INVALID_NODEID ;
      const CHAR *hostname = NULL ;
      NodeID routeID ;
      clsRegAssit regAssit ;
      MsgCatRegisterRsp *rsp = (MsgCatRegisterRsp *)msg ;

      // have register succeed
      if ( _regTimerID == CLS_INVALID_TIMERID )
      {
         goto done ;
      }

      rc = MSG_GET_INNER_REPLY_RC( msg ) ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         updateCatGroup ( 100 ) ;
         goto error ;
      }
      else if ( rc != SDB_OK )
      {
         PD_LOG ( PDSEVERE, "Node register failed[Respone:%d]", rc ) ;
         goto error ;
      }

      //get nodeid
      rc = regAssit.extractResponseMsg ( msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Node register response error, rc: %d", rc ) ;
      groupID = regAssit.getGroupID () ;
      nodeID = regAssit.getNodeID () ;
      hostname = regAssit.getHostname () ;

      rc = _updateDCInfo( msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update DC info, rc: %d", rc ) ;

      if ( _needUpdateNode )
      {
         if ( 0 == rsp->startFrom )
         {
            // update node done
            //Kill register timer
            killTimer ( _regTimerID ) ;
            _regTimerID = CLS_INVALID_TIMERID ;
            _regFailedTimes = 0 ;

            _needUpdateNode = FALSE ;

            PD_LOG ( PDEVENT, "Update node succeed, groupID:%u, nodeID:%u",
                     _selfNodeID.columns.groupID,
                     _selfNodeID.columns.nodeID ) ;
         }

         if ( 0 == rsp->startFrom )
         {
            if ( SDB_OK != _shdObj->updatePrimary( msg->routeID, TRUE ) )
            {
               _shdObj->updateCatGroup () ;
            }
         }
         else if ( -1 != rsp->startFrom )
         {
            if ( SDB_OK != _shdObj->updatePrimaryByReply( msg ) )
            {
               _shdObj->updateCatGroup() ;
            }
         }
         else
         {
            // primary is unknown
            _shdObj->updateCatGroup() ;
         }

         goto done ;
      }
      else if ( SDB_ROLE_CATALOG == pmdGetKRCB()->getDBRole() &&
                0 != rsp->startFrom )
      {
         // for CATALOG, need a secondary register message to update
         // node information to the primary node
         _needUpdateNode = TRUE ;
         _regFailedTimes = 0 ;
      }
      else
      {
         //Kill register timer
         killTimer ( _regTimerID ) ;
         _regTimerID = CLS_INVALID_TIMERID ;
         _regFailedTimes = 0 ;
      }

      //Update the net route agent the local id
      _selfNodeID.columns.groupID = groupID ;
      _selfNodeID.columns.nodeID = nodeID ;
      _shdObj->setNodeID( _selfNodeID ) ;
      PD_LOG ( PDEVENT, "Register succeed, groupID:%u, nodeID:%u",
               _selfNodeID.columns.groupID,
               _selfNodeID.columns.nodeID ) ;

      /*
       * The node can be created by hostname or ip,
       * so the actual 'HostName' maybe current host's name or ip address.
       * Here we ensure the KRCB's HostName is consistent with catalog.
       */
      pmdGetKRCB()->setHostName( hostname ) ;

      routeID.value = _selfNodeID.value ;
      routeID.columns.serviceID = _replServiceID ;
      _replNetRtAgent->setLocalID ( routeID ) ;
      routeID.columns.serviceID = _shardServiceID ;
      _shardNetRtAgent->setLocalID ( routeID ) ;

      // set global id
      pmdSetNodeID( _selfNodeID ) ;

      pmdGetKRCB()->callRegisterEventHandler( _selfNodeID ) ;
      pmdGetKRCB()->setBusinessOK( TRUE ) ;

      //Update the primary catlog node
      if ( 0 == rsp->startFrom )
      {
         if ( SDB_OK != _shdObj->updatePrimary( msg->routeID, TRUE ) )
         {
            _shdObj->updateCatGroup () ;
         }
      }
      else if ( -1 != rsp->startFrom )
      {
         if ( SDB_OK != _shdObj->updatePrimaryByReply( msg ) )
         {
            _shdObj->updateCatGroup() ;
         }
      }
      else
      {
         // primary is unknown
         _shdObj->updateCatGroup() ;
      }

      //Active the shard and repl CBs
      rc = _shdObj->active () ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "active shardCB failed[rc:%d]", rc ) ;
         goto error ;
      }

      rc = _replObj->active () ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "active replCB failed[rc:%d]", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC (SDB__CLSMGR__ONCATREGRES, rc );
      return rc ;
   error:
      //Need to shutdown
      if ( rc == SDB_CAT_AUTH_FAILED )
      {
         ++_regFailedTimes ;
         if ( SDB_ROLE_CATALOG != pmdGetDBRole() ||
              _regFailedTimes >= CLS_REPLSET_MAX_NODE_SIZE )
         {
            PD_LOG ( PDSEVERE, "Catlog auth the db node failed, shutdown..." ) ;
            PMD_SHUTDOWN_DB( SDB_CAT_AUTH_FAILED ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSMGR__ONCATQTSKRES, "_clsMgr::_onCatQueryTaskRes" )
   INT32 _clsMgr::_onCatQueryTaskRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSMGR__ONCATQTSKRES );
      MsgCatQueryTaskRes *res = ( MsgCatQueryTaskRes* )msg ;
      PD_LOG ( PDDEBUG, "Recieve catalog query task response[requestID:%lld, "
               "flag: %d]", msg->requestID, res->flags ) ;

      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> objList ;
      MAPTASKQUERY::iterator it ;

      // need to update catalog group
      if ( SDB_CLS_NOT_PRIMARY == res->flags )
      {
         if ( SDB_OK != _shdObj->updatePrimaryByReply( msg ) )
         {
            updateCatGroup() ;
         }
      }
      // need to clear the query task
      else if ( SDB_DMS_EOC == res->flags ||
                SDB_CAT_TASK_NOTFOUND == res->flags )
      {
         _clsLatch.get() ;
         /// if is the last query and not { TargetID : groupID }, need to
         /// query all( by { TargetID : groupID } )
         it = _mapTaskQuery.find ( msg->requestID ) ;
         if ( it != _mapTaskQuery.end() )
         {
            if ( 1 == _mapTaskQuery.size() )
            {
               BSONObj queryAllSplit = BSON( FIELD_NAME_TARGETID <<
                                             _selfNodeID.columns.groupID <<
                                             FIELD_NAME_STATUS <<
                                             BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;
               if ( 0 != queryAllSplit.woCompare( it->second ) )
               {
                  _mapTaskQuery[ ++_requestID ] = queryAllSplit ;
               }
            }
         }
         _mapTaskQuery.erase ( msg->requestID ) ;
         _clsLatch.release() ;
         PD_LOG ( PDINFO, "The query task[%lld] has 0 jobs", msg->requestID ) ;
      }
      else if ( SDB_OK != res->flags )
      {
         PD_LOG ( PDERROR, "Query task[%lld] failed[rc=%d]",
                  msg->requestID, res->flags ) ;
         goto error ;
      }
      else
      {
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         // find the task query map, and remove it
         {
            ossScopedLock lock ( &_clsLatch, EXCLUSIVE ) ;
            it = _mapTaskQuery.find ( msg->requestID ) ;
            if ( it == _mapTaskQuery.end() )
            {
               PD_LOG ( PDWARNING, "The query task response[%lld] is not exist",
                        msg->requestID ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            //remove the query task
            _mapTaskQuery.erase ( it ) ;
         }

         PD_LOG ( PDINFO, "The query task[requestID:%lld] has %d jobs",
                  msg->requestID, numReturned ) ;

         // start task thread
         for ( UINT32 i = 0 ; i < objList.size() ; i++ )
         {
            UINT64 taskID = CLS_INVALID_TASKID ;
            rc = startTaskThread ( objList[i], taskID ) ;

            if ( rc && SDB_CLS_MUTEX_TASK_EXIST != rc )
            {
               startTaskCheck( taskID ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSMGR__ONCATQTSKRES, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsMgr::_onStepDown( pmdEDUEvent *event )
   {
      return sdbGetReplCB()->dispatchEvent( event ) ;
   }

   INT32 _clsMgr::_onStepUp( pmdEDUEvent *event )
   {
      return sdbGetReplCB()->dispatchEvent( event ) ;
   }

   INT32 _clsMgr::_onGroupModeUpdate( pmdEDUEvent *event )
   {
      return sdbGetReplCB()->dispatchEvent( event ) ;
   }

   /*
      get global cls cb
   */
   clsCB* sdbGetClsCB ()
   {
      static clsCB s_clsCB ;
      return &s_clsCB ;
   }
   shardCB* sdbGetShardCB ()
   {
      return sdbGetClsCB()->getShardCB() ;
   }
   replCB* sdbGetReplCB ()
   {
      return sdbGetClsCB()->getReplCB() ;
   }

}

