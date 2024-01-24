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

   Source File Name = seadptMgr.cpp

   Descriptive Name = Search engine adapter manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmd.hpp"
#include "seAdptMgr.hpp"
#include "seAdptAgentSession.hpp"
#include "seAdptIndexSession.hpp"
#include "utilESCltMgr.hpp"

#define DATA_NODE_GRP_ID                        10000
#define DATA_NODE_ID                            10000
#define SEADPT_INIT_TEXT_INDEX_VERSION          (-1)
#define SEADPT_IDX_UPDATE_INTERVAL              ( 5 * OSS_ONE_SEC )
#define SEADPT_PORT_STR_SZ                      10
#define SEADPT_MAX_PORT                         65535
#define SEADPT_SVC_PORT_PLUS                    7

namespace seadapter
{
   UINT64 _seSvcSessionMgr::makeSessionID( const NET_HANDLE &handle,
                                           const MsgHeader *header )
   {
      return ossPack32To64( PMD_BASE_HANDLE_ID, header->TID ) ;
   }

   SDB_SESSION_TYPE _seSvcSessionMgr::_prepareCreate( UINT64 sessionID,
                                                      INT32 startType,
                                                      INT32 opCode )
   {
      return SDB_SESSION_SE_AGENT ;
   }

   INT32 _seSvcSessionMgr::onErrorHanding( INT32 rc,
                                           const MsgHeader *pReq,
                                           const NET_HANDLE &handle,
                                           UINT64 sessionID,
                                           pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      if ( 0 != sessionID )
      {
         // In case of error, let reply the error to the client.
         ret = _reply( handle, rc, pReq ) ;
      }
      else
      {
         ret = rc ;
      }

      return ret ;
   }

   pmdAsyncSession*
   _seSvcSessionMgr::_createSession( SDB_SESSION_TYPE sessionType,
                                     INT32 startType,
                                     UINT64 sessionID,
                                     void *data )
   {
      pmdAsyncSession* session = NULL ;

      if ( SDB_SESSION_SE_AGENT == sessionType )
      {
         session = SDB_OSS_NEW seAdptAgentSession( sessionID ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid session type[%d]", sessionType ) ;
      }

      return session ;
   }

   _seIndexSessionMgr::_seIndexSessionMgr( _seAdptCB *pAdptCB )
   {
      _pAdptCB = pAdptCB ;
      _optionMgr = NULL ;
      _innerSessionID = 0 ;
   }

   _seIndexSessionMgr::~_seIndexSessionMgr()
   {
   }

   UINT64 _seIndexSessionMgr::makeSessionID( const NET_HANDLE &handle,
                                             const MsgHeader *header )
   {
      return ossPack32To64( PMD_BASE_HANDLE_ID, header->TID ) ;
   }

   INT32 _seIndexSessionMgr::refreshTasks()
   {
      INT32 rc = SDB_OK ;
      vector<UINT16> imIDs ;
      seIdxMetaMgr* idxMetaMgr = _pAdptCB->getIdxMetaMgr() ;

      idxMetaMgr->getCurrentIMIDs( imIDs ) ;

      for ( vector<UINT16>::iterator itr = imIDs.begin(); itr != imIDs.end();
            ++itr )
      {
         // Start new task.
         BOOLEAN startNew = FALSE ;
         seIdxMetaContext *imContext = NULL ;
         rc = idxMetaMgr->getIMContext( &imContext, *itr, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get index meta context failed[%d]",
                      rc ) ;
         seIndexMeta *meta = imContext->meta() ;
         if ( idxMetaMgr->getMetaVersion() == meta->getVersion() &&
              SEADPT_IM_STAT_PENDING == meta->getStat() )
         {
            _activeWorkerLatch.get() ;
            if ( _activeWorkers.find( meta->getID() ) == _activeWorkers.end() )
            {
               _pAdptCB->startInnerSession( SEADPT_SESSION_INDEX,
                                            _newSessionID(), (void *)imContext ) ;
               startNew = TRUE ;
               _activeWorkers.insert( meta->getID() ) ;
            }
            _activeWorkerLatch.release() ;
            // Note: Do not release the imContext. It will be used by the session,
            // and released when the session exists.
         }
         imContext->metaUnlock() ;
         if ( !startNew )
         {
            // If no new task starts for the index, release the index meta
            // context.
            idxMetaMgr->releaseIMContext( imContext ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIndexSessionMgr::stopAllIndexer( const NET_HANDLE &handle )
   {
      // Stopp all indexers which are running currently. Then clean all
      // remainning tasks.
      handleSessionClose( handle ) ;
      _pAdptCB->cleanInnerSession( SEADPT_SESSION_INDEX ) ;
   }

   INT32 _seIndexSessionMgr::setOptionMgr( const seAdptOptionsMgr *optionMgr )
   {
      INT32 rc = SDB_OK ;
      if ( !optionMgr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _optionMgr = optionMgr ;

   done:
      return rc ;
   error:
      goto done ;
   }

   SDB_SESSION_TYPE _seIndexSessionMgr::_prepareCreate( UINT64 sessionID,
                                                        INT32 startType,
                                                        INT32 opCode )
   {
      return SDB_SESSION_SE_INDEX ;
   }

   BOOLEAN _seIndexSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      return FALSE ;
   }

   UINT32 _seIndexSessionMgr::_maxCacheSize() const
   {
      return 0 ;
   }

   INT32 _seIndexSessionMgr::onErrorHanding( INT32 rc,
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

   void _seIndexSessionMgr::onSessionDestoryed( pmdAsyncSession *pSession )
   {
      seIdxMetaContext *imContext =
            ((seAdptIndexSession *)pSession)->idxMetaContext() ;
      SDB_ASSERT( imContext , "Index meta context is NULL" ) ;
      seIndexMeta *meta = imContext->meta() ;
      _activeWorkerLatch.get() ;
      _activeWorkers.erase( meta->getID() ) ;
      _activeWorkerLatch.release() ;
      _pAdptCB->resetIdxVersion() ;
   }

   pmdAsyncSession*
   _seIndexSessionMgr::_createSession( SDB_SESSION_TYPE sessionType,
                                       INT32 startType,
                                       UINT64 sessionID,
                                       void *data )
   {
      pmdAsyncSession *pSession = NULL ;

      if ( !data )
      {
         PD_LOG( PDERROR, "Parameter data for session is NULL" ) ;
      }
      else if ( SDB_SESSION_SE_INDEX == sessionType )
      {
         seIdxMetaContext *imContext = (seIdxMetaContext*)data ;
         pSession = SDB_OSS_NEW seAdptIndexSession( sessionID, imContext ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   BEGIN_OBJ_MSG_MAP( _seAdptCB, _pmdObjBase )
      ON_MSG( MSG_AUTH_VERIFY_RES, _onRegisterRes )
      ON_MSG( MSG_SEADPT_UPDATE_IDXINFO_RES, _onIdxUpdateRes )
      ON_MSG( MSG_COM_REMOTE_DISC, _onRemoteDisconnect )
   END_OBJ_MSG_MAP()

   _seAdptCB::_seAdptCB()
   : _indexMsgHandler( &_idxSessionMgr ),
     _svcMsgHandler( &_svcSessionMgr ),
     _indexTimerHandler( &_idxSessionMgr ),
     _svcTimerHandler( &_svcSessionMgr ),
     _dbAssist( &_indexMsgHandler ),
     _svcRtAgent( &_svcMsgHandler ),
     _idxSessionMgr( this )
   {
      ossMemset( _serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
      _selfRouteID.value = MSG_INVALID_ROUTEID ;
      _peerPrimary = FALSE ;
      _regTimerID = NET_INVALID_TIMER_ID ;
      _idxUpdateTimerID = NET_INVALID_TIMER_ID ;
      _oneSecTimerID = NET_INVALID_TIMER_ID ;
      _esDetectTimerID = NET_INVALID_TIMER_ID ;
      _cleanupConnnTimerID = NET_INVALID_TIMER_ID ;
      _localIdxVer = SEADPT_INIT_TEXT_INDEX_VERSION ;
      _regMsgBuff = NULL ;
      _indexerOn = FALSE ;
   }

   _seAdptCB::~_seAdptCB()
   {
      if ( _regMsgBuff )
      {
         SDB_OSS_FREE( _regMsgBuff ) ;
      }
   }

   SDB_CB_TYPE _seAdptCB::cbType() const
   {
      return SDB_CB_SEADAPTER ;
   }

   const CHAR* _seAdptCB::cbName() const
   {
      return "SEADAPTER" ;
   }

   INT32 _seAdptCB::init()
   {
      INT32 rc = SDB_OK ;
      utilESCltMgr *esCltMgr = utilGetESCltMgr() ;
      CHAR seSvcAddr[ SEADPT_SE_SVCADDR_MAX_SZ + 1 ] = { 0 } ;

      // register config handler to se adapter options.
      _options.setConfigHandler( pmdGetKRCB() ) ;

      rc = _startSvcListener() ;
      PD_RC_CHECK( rc, PDERROR, "Start service listener failed[ %d ]", rc ) ;

      // Init sdb data node address.
      rc = _initSdbAddr() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init sdb data node address failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = _idxSessionMgr.init( _dbAssist.routeAgent(), &_indexTimerHandler,
                                5 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Init index session manager failed[ %d ]",
                   rc ) ;

      rc = _idxSessionMgr.setOptionMgr( &_options ) ;
      PD_RC_CHECK( rc, PDERROR, "Set option manager in index session manager "
                   "failed[ %d ]", rc ) ;

      rc = _svcSessionMgr.init( &_svcRtAgent, &_svcTimerHandler,
                                60 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Init service session manager failed[ %d ]",
                   rc ) ;

      // Initialize search engine client manager.
      ossSnprintf( seSvcAddr, OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2,
                   "%s:%s", _options.getSEHost(), _options.getSEService() ) ;

      // TODO: In future, if we can connect to adapter and change the
      // configuration value, just set the option manager in the factory.

      rc = esCltMgr->init( seSvcAddr, _options.getSEConnLimit(),
                           _options.getSEConnTimeout(),
                           _options.getTimeout() ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize search engine client manager "
                   "failed[%d]", rc ) ;
      esCltMgr->setScrollSize( _options.getSEScrollSize() ) ;
      PD_LOG( PDEVENT, "Search engine client manager init successfully" ) ;

      // Set the business status to not OK. Change to OK after successfully
      // registered on data node.
      pmdGetKRCB()->setBusinessOK( FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::active()
   {
      #define SEADPT_WAIT_CB_ATTACH_TIMEOUT             ( 300 * OSS_ONE_SEC )

      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;

      _attachEvent.reset() ;
      // 1. start se adapter manager edu.
      rc = pEDUMgr->startEDU( EDU_TYPE_SEADPTMGR, (_pmdObjBase*)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start search engine adapter manager "
                   "edu, rc: %d", rc ) ;
      rc = _attachEvent.wait( SEADPT_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait search engine adapter manager edu attach "
                   "failed, rc: %d", rc ) ;

      // 2. start network daemon for indexer reader.
      rc = pEDUMgr->startEDU( EDU_TYPE_SE_INDEXR, _dbAssist.routeAgent(),
                              &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start search engine adapter net, "
                   "rc: %d", rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait indexer reader network daemons active "
                   "failed[ %d ]", rc ) ;

      // 3. start se adapter service.
      rc = pEDUMgr->startEDU( EDU_TYPE_SE_SERVICE, (void *)&_svcRtAgent,
                              &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Start service listener failed[ %d ]", rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait service listener active failed[ %d ]",
                   rc ) ;

      rc = _setTimers() ;
      PD_RC_CHECK( rc, PDERROR, "Set timers failed[ %d ]", rc ) ;

      // Register immediately.
      _sendRegisterMsg() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::deactive()
   {
      _svcRtAgent.shutdownListen() ;
      _dbAssist.routeAgent()->stop() ;
      _svcRtAgent.stop() ;

      _idxSessionMgr.setForced() ;
      _svcSessionMgr.setForced() ;

      return SDB_OK ;
   }

   INT32 _seAdptCB::fini()
   {
      _idxSessionMgr.fini() ;
      _svcSessionMgr.fini() ;
      return SDB_OK ;
   }

   void _seAdptCB::attachCB( pmdEDUCB *cb )
   {
      if ( EDU_TYPE_SEADPTMGR == cb->getType() )
      {
         _svcMsgHandler.attach( cb ) ;
         _indexMsgHandler.attach( cb ) ;
         _svcTimerHandler.attach( cb ) ;
         _indexTimerHandler.attach( cb ) ;
      }
      _attachEvent.signalAll() ;
   }

   void _seAdptCB::detachCB( pmdEDUCB *cb )
   {
      if ( EDU_TYPE_SEADPTMGR == cb->getType() )
      {
         _svcMsgHandler.detach() ;
         _indexMsgHandler.detach() ;
         _svcTimerHandler.detach() ;
         _indexTimerHandler.detach() ;
      }
   }

   void _seAdptCB::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( timerID == _regTimerID )
      {
         _sendRegisterMsg() ;
      }
      else if ( timerID == _esDetectTimerID )
      {
         _detectES() ;
      }
      else if ( timerID == _oneSecTimerID )
      {
         _idxSessionMgr.onTimer( interval ) ;
         _svcSessionMgr.onTimer( interval ) ;
      }
      else if ( timerID == _idxUpdateTimerID )
      {
         // Only send the index update request when adapter is successfully
         // registered, and Elasticsearch is online.
         if ( NET_INVALID_TIMER_ID == _regTimerID
              && NET_INVALID_TIMER_ID == _esDetectTimerID )
         {
            INT32 rc = _sendIdxUpdateReq() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send index update request failed[ %d ]", rc ) ;
            }
         }
      }
      else if ( timerID == _cleanupConnnTimerID )
      {
         _cleanupESClts() ;
      }
      return ;
   }

   seAdptOptionsMgr* _seAdptCB::getOptions()
   {
      return &_options ;
   }

   seSvcSessionMgr* _seAdptCB::getSeAgentMgr()
   {
      return &_svcSessionMgr ;
   }

   seIndexSessionMgr* _seAdptCB::getIdxSessionMgr()
   {
      return &_idxSessionMgr ;
   }

   INT32 _seAdptCB::startInnerSession( SEADPT_SESSION_TYPE type,
                                       UINT64 sessionID, void *data )
   {
      ossScopedLock lock ( &_seLatch, EXCLUSIVE ) ;

      seAdptSessionInfo info ;
      info.type = type ;
      info.startType = PMD_SESSION_ACTIVE ;
      info.data = data ;
      info.sessionID = sessionID ;

      _vecInnerSessionParam.push_back ( info ) ;

      return SDB_OK ;
   }

   void _seAdptCB::cleanInnerSession( INT32 type )
   {
      ossScopedLock lock( &_seLatch, EXCLUSIVE ) ;

      VECINNERPARAM::iterator itr = _vecInnerSessionParam.begin() ;

      while ( itr != _vecInnerSessionParam.end() )
      {
         if ( type == itr->type )
         {
            _vecInnerSessionParam.erase( itr++ ) ;
         }
         else
         {
            ++itr ;
         }
      }
   }

   void _seAdptCB::resetIdxVersion()
   {
      _localIdxVer = SEADPT_INIT_TEXT_INDEX_VERSION ;
   }

   INT32 _seAdptCB::_updateIndexInfo( const NET_HANDLE &handle, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      INT64 peerVersion = -1 ;
      BOOLEAN isPrimary = FALSE ;

      PD_LOG( PDDEBUG, "Index information object: %s",
              obj.toString().c_str() ) ;

      try
      {
         BSONElement ele ;

         ele = obj.getField( FIELD_NAME_IS_PRIMARY ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Get peer node role failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            isPrimary = ele.Bool() ;
            if ( isPrimary )
            {
               if ( !isDataNodePrimary() )
               {
                  PD_LOG( PDEVENT, "Peer node is primary. Search engine adapter"
                          " switch from READONLY mode to READWRITE mode" ) ;
                  setDataNodePrimary( TRUE ) ;
               }
            }
            else
            {
               if ( isDataNodePrimary() )
               {
                  PD_LOG( PDEVENT, "Peer node is not primary. Search engine "
                          "adapter switch from READWRITE mode to READONLY "
                          "mode" ) ;
                  setDataNodePrimary( FALSE ) ;
                  _idxSessionMgr.stopAllIndexer( handle ) ;
                  _indexerOn = FALSE ;
                  // Reset the index version. If the mode change to full again,
                  // new indexing work should be started.
                  resetIdxVersion() ;
               }
            }
         }

         ele = obj.getField( FIELD_NAME_VERSION ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Get peer text index version failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            peerVersion = ele.numberLong() ;
            if ( peerVersion != _localIdxVer )
            {
               PD_LOG( PDDEBUG, "Index version: local[%lld] != peer[%lld]. "
                                "Ready to update local metas",
                       _localIdxVer, peerVersion ) ;
#ifdef _DEBUG
               _idxMetaMgr.printAllIdxMetas() ;
#endif

               BSONElement idxEles = obj.getField( FIELD_NAME_INDEXES ) ;
               if ( EOO == idxEles.type() )
               {
                  PD_LOG( PDERROR, "No index information found" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               {
                  _idxMetaMgr.setMetaVersion( peerVersion ) ;
                  vector<BSONElement> idxElements = idxEles.Array() ;
                  for ( vector<BSONElement>::iterator itr = idxElements.begin();
                        itr != idxElements.end(); ++itr )
                  {
                     BSONObj idxInfoObj = itr->Obj() ;
                     rc = _idxMetaMgr.addIdxMeta( idxInfoObj, NULL, TRUE ) ;
                     PD_RC_CHECK( rc, PDERROR, "Add index metadata failed[%d]. "
                                               "Index metadata: %s",
                                  rc, idxInfoObj.toString().c_str() ) ;
                  }
                  _idxMetaMgr.clearObsoleteMeta() ;
               }
               _localIdxVer = peerVersion ;
#ifdef _DEBUG
               _idxMetaMgr.printAllIdxMetas() ;
#endif
            }
            else
            {
               PD_LOG( PDDEBUG, "Text index version are the same[%lld], no need "
                       "for updating", _localIdxVer ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse index information failed: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_startSvcListener()
   {
      INT32 rc = SDB_OK ;
      MsgRouteID svcRtID = _selfRouteID ;
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      UINT16 sdbPort = 0 ;
      BOOLEAN success = FALSE ;

      // Use a fixed node id and a random service port for the adapter.
      // Try to use port starting from [ sdb_service + 7|8|9 ], skipping the
      // ones end with 0. Find one that can be used, or return error if we
      // can't.
      ossSocket::getPort( _options.getDBService(), sdbPort ) ;

      // Create listener socket. This is for searching and command processing.
      svcRtID.columns.groupID = SEADPT_GRP_ID ;
      svcRtID.columns.nodeID = SEADPT_NODE_ID ;
      svcRtID.columns.serviceID = SEADPT_SVC_ID ;

      INT32 svcPort = sdbPort + SEADPT_SVC_PORT_PLUS ;
      while ( svcPort <= SEADPT_MAX_PORT )
      {
         // Skip the ports end with 0, for they may be used by sdb nodes.
         if ( 0 == svcPort % 10 )
         {
            svcPort += SEADPT_SVC_PORT_PLUS ;
            continue ;
         }

         CHAR svcPortStr[ SEADPT_PORT_STR_SZ ] = { 0 } ;
         ossItoa( svcPort, svcPortStr, SEADPT_PORT_STR_SZ ) ;

         _svcRtAgent.updateRoute( svcRtID, hostName, svcPortStr ) ;

         rc = _svcRtAgent.listen( svcRtID ) ;
         if ( rc )
         {
            _svcRtAgent.delRoute( svcRtID ) ;
            if ( SEADPT_MAX_PORT == svcPort )
            {
               PD_RC_CHECK( rc, PDERROR, "Create listener for adapter service "
                            "failed[ %d ]", rc ) ;
               goto error ;
            }
         }
         else
         {
            _options.setSvcName( svcPortStr ) ;
            success = TRUE ;
            PD_LOG( PDEVENT, "Create search engine adapter listener"
                    "[ServiceName: %s] successfully", _options.getSvcName() ) ;
            break ;
         }
         ++svcPort ;
      }
      if ( !success )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_initSdbAddr()
   {
      // Add the route information of sdb node.
      INT32 rc = SDB_OK ;
      UINT16 port = 0 ;
      CHAR service[ OSS_MAX_SERVICENAME + 1 ] = { 0 } ;
      MsgRouteID dataNodeID ;

      dataNodeID.columns.groupID = DATA_NODE_GRP_ID ;
      dataNodeID.columns.nodeID = DATA_NODE_ID ;

      // capped collection from shard flat.
      dataNodeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
      ossSocket::getPort( _options.getDBService(), port ) ;
      port += MSG_ROUTE_SHARD_SERVCIE ;

      ossSnprintf( service, OSS_MAX_SERVICENAME + 1, "%u", port ) ;

      rc = _dbAssist.updateDataNodeRoute( dataNodeID, _options.getDBHost(),
                                          service ) ;
      PD_RC_CHECK( rc, PDERROR, "Update data node route information failed[%d]",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // Register on the data node. Use the auth msg.
   INT32 _seAdptCB::_sendRegisterMsg()
   {
      INT32 rc = SDB_OK ;
      BSONObj authObj ;
      INT32 msgLen = 0 ;
      MsgAuthentication *authMsg = NULL ;

      try
      {
         // Should send the route information to data node. Then data node is
         // able to send request and reply. Currently we use a fixed node id
         // for the adapter.
         authObj = BSON( FIELD_NAME_HOST << pmdGetKRCB()->getHostName()
                         << FIELD_NAME_SERVICE_NAME << _options.getSvcName()
                         << FIELD_NAME_GROUPID << SEADPT_GRP_ID
                         << FIELD_NAME_NODEID << SEADPT_NODE_ID
                         << FIELD_NAME_SERVICE_TYPE << SEADPT_SVC_ID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      msgLen = sizeof( MsgAuthentication ) +
               ossRoundUpToMultipleX( authObj.objsize(), 4 ) ;

      authMsg = ( MsgAuthentication * )SDB_OSS_MALLOC( msgLen ) ;
      if ( !authMsg )
      {
         PD_LOG( PDERROR, "Allocate memory of size[ %d ] for message failed",
                 msgLen ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      authMsg->header.requestID = 0 ;
      authMsg->header.opCode = MSG_AUTH_VERIFY_REQ ;
      authMsg->header.messageLength = sizeof( MsgAuthentication ) +
                                      authObj.objsize() ;
      authMsg->header.routeID.value = 0 ;
      // Send to the main thread of shard.
      authMsg->header.TID = 0 ;
      ossMemcpy( (CHAR *)authMsg + sizeof( MsgAuthentication ),
                 authObj.objdata(), authObj.objsize() ) ;

      rc = _dbAssist.sendToDataNode( (const MsgHeader *)authMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send register request to data node "
                   "failed[%d]", rc ) ;
      PD_LOG( PDDEBUG, "Send register message to data node successfully. "
              "Information: %s", authObj.toString().c_str() ) ;

   done:
      if ( authMsg )
      {
         SDB_OSS_FREE( authMsg ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_detectES()
   {
      _killTimer( _esDetectTimerID ) ;
      _esDetectTimerID = NET_INVALID_TIMER_ID ;

      if ( NET_INVALID_TIMER_ID == _regTimerID )
      {
         pmdGetKRCB()->setBusinessOK( TRUE ) ;
      }
      return SDB_OK ;
   }

   INT32 _seAdptCB::_resumeRegister()
   {
      INT32 rc = SDB_OK ;

      rc = _dbAssist.routeAgent()->addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                             _regTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register timer failed[ %d ]", rc ) ;

      _sendRegisterMsg() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_onRegisterRes( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> objVec ;
      BSONObj cataInfoObj ;
      const CHAR *peerGrpName = NULL ;

      // Adapter has registered successfully.
      if ( NET_INVALID_TIMER_ID == _regTimerID )
      {
         goto done ;
      }

      try
      {
         // Get the role, group id of the node.
         rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                               &numReturned, objVec ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract register reply failed[ %d ]", rc ) ;
         if ( SDB_OK != flag )
         {
            PD_LOG( PDERROR, "Adapter register on data node failed[%d]",
                    flag ) ;
            goto error ;
         }

         if ( 1 != objVec.size() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Register reply is not as expected. Make sure the "
                             "data node configuration is correct[%d]", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Register reply message: %s",
                 objVec[0].toString(false, true).c_str() ) ;

         _peerPrimary = objVec[0].getBoolField( FIELD_NAME_IS_PRIMARY ) ;
         peerGrpName = objVec[0].getStringField( FIELD_NAME_GROUPNAME ) ;
         if ( 0 == ossStrlen( peerGrpName ) )
         {
            PD_LOG( PDERROR, "Find peer node group name failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _idxMetaMgr.setFixTypeName( peerGrpName ) ;

         cataInfoObj = objVec[0].getObjectField( FIELD_NAME_CATALOGINFO ) ;
         if ( cataInfoObj.isEmpty() )
         {
            PD_LOG( PDERROR, "Find catalog info in object failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _dbAssist.updateGroupInfo( cataInfoObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Update catalog group info failed[%d]",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _killTimer( _regTimerID ) ;
      _regTimerID = NET_INVALID_TIMER_ID ;

      if ( NET_INVALID_TIMER_ID == _esDetectTimerID )
      {
         pmdGetKRCB()->setBusinessOK( TRUE ) ;
      }

      // Registration forcefully triggered when the service is ok. That may
      // happen when the adapter try to update the catalogue information. In
      // that case, the following actions should not be done.
      if ( !_indexerOn )
      {
         PD_LOG( PDEVENT, "Search engine adapter registered on SequoiaDB data "
                          "node successfully" ) ;
         if ( TRUE == _peerPrimary )
         {
            PD_LOG( PDEVENT, "Peer node is primary. Search engine adapter is "
                             "running in READWRITE mode" ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Peer node is not primary. Search engine adapter "
                             "is running in READONLY mode" ) ;
         }

         // New register means new connections. Refresh the index information
         // forcefully.
         _localIdxVer = SEADPT_INIT_TEXT_INDEX_VERSION ;
         // When connect to any node, start the index update timer.
         rc = _dbAssist.routeAgent()->addTimer( SEADPT_IDX_UPDATE_INTERVAL,
                                                &_indexTimerHandler,
                                                _idxUpdateTimerID ) ;
         PD_RC_CHECK( rc, PDERROR, "Register timer failed[ %d ]", rc ) ;
      }
      _dbAssist.setDataNetHandle( handle ) ;
      _registerEvent.signalAll() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_startEDU( INT32 type, EDU_STATUS waitStatus,
                                  void *agrs, BOOLEAN regSys )
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      pmdEDUMgr *pEDUMgr = pKRCB->getEDUMgr () ;

      //Start EDU
      rc = pEDUMgr->startEDU( (EDU_TYPES)type, (void *)agrs, &eduID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create EDU[type:%d(%s)], rc = %d",
                  type, getEDUName( (EDU_TYPES)type ), rc ) ;
         goto error ;
      }

      //Wait edu running
      if ( PMD_EDU_UNKNOW != waitStatus )
      {
         rc = pEDUMgr->waitUntil( (EDU_TYPES)type, waitStatus ) ;
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
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_startInnerSession( INT32 type,
                                        pmdAsycSessionMgr *pSessionMgr )
   {
      INT32 rc = SDB_OK ;
      _pmdAsyncSession *pSession = NULL ;
      ossScopedLock lock( &_seLatch, EXCLUSIVE ) ;

      VECINNERPARAM::iterator itr = _vecInnerSessionParam.begin() ;
      while ( itr != _vecInnerSessionParam.end() )
      {
         seAdptSessionInfo &info = *itr ;
         if ( info.type != type ||
              SDB_OK == pSessionMgr->getSession( info.sessionID,
                                                 FALSE,
                                                 info.startType,
                                                 NET_INVALID_HANDLE,
                                                 FALSE, 0, NULL,
                                                 NULL ) )
         {
            ++itr ;
            continue ;
         }

         rc = pSessionMgr->getSession( info.sessionID, FALSE,
                                       info.startType,
                                       NET_INVALID_HANDLE, TRUE, 0,
                                       info.data,
                                       &pSession ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Create inner session[%s] succeed",
                    pSession->sessionName() ) ;
            itr = _vecInnerSessionParam.erase( itr ) ;
            continue ;
         }

         PD_LOG( PDERROR, "Create inner session[TID:%lld] failed, rc: %d",
                 info.sessionID, rc ) ;
         ++itr ;
      }

      return rc ;
   }

   // Communicate with SDB data node to ensure the local text index information
   // is fresh. If not, get it from SDB and refresh local copy.
   INT32 _seAdptCB::_sendIdxUpdateReq()
   {
      INT32 rc = SDB_OK ;

      // Send index query request to data node. The local index information
      // version number will be passed.
      BSONObj msgBody ;
      INT32 msgLen = 0 ;

      try
      {
         msgBody =
            BSON( FIELD_NAME_VERSION << _localIdxVer ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      msgLen = sizeof( MsgHeader ) +
               ossRoundUpToMultipleX( msgBody.objsize(), 4 ) ;

      if ( !_regMsgBuff )
      {
         _regMsgBuff = ( MsgHeader * )SDB_OSS_MALLOC( msgLen ) ;
         if ( !_regMsgBuff )
         {
            PD_LOG( PDERROR, "Allocate memory of size[ %d ] for message failed",
                    msgLen ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _regMsgBuff->opCode = MSG_SEADPT_UPDATE_IDXINFO_REQ ;
         _regMsgBuff->messageLength = sizeof( MsgHeader ) + msgBody.objsize() ;
         _regMsgBuff->routeID.value = 0 ;
         _regMsgBuff->TID = 0 ;
         _regMsgBuff->requestID = 0 ;
      }

      ossMemcpy( (CHAR *)_regMsgBuff + sizeof( MsgHeader ),
                 msgBody.objdata(), msgBody.objsize() ) ;

      rc = _dbAssist.sendToDataNode( _regMsgBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to data node failed[%d]", rc ) ;
      PD_LOG( PDDEBUG, "Send text index update request to data node. "
              "Message: %s", msgBody.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_onIdxUpdateRes( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> objVec ;
      const INT32 expectReplySz = 1 ;

      rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                            &numReturned, objVec ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Extract index information query reply failed[ %d ]", rc ) ;
      rc = flag ;
      PD_RC_CHECK( rc, PDERROR,
                   "Index information query request failed[ %d ]", rc ) ;

      // All text indices information should be replied in one object.
      if ( ( expectReplySz != numReturned ) ||
           ( expectReplySz != (INT32)objVec.size() ) )
      {
         PD_LOG( PDERROR, "Reply of index information is not as expected" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _updateIndexInfo( handle, objVec[0] ) ;
      PD_RC_CHECK( rc, PDERROR, "Update indices information failed[ %d ]",
                   rc ) ;

      // There are several scenarios we should refresh index tasks:
      // (1) Index information is updated.
      // (2) Peer data node upgrades from slave to primary.
      // (3) No upgrade or update, all index tasks stopped due to the connection
      //     lost with ES. Now it's alive again. Tasks should be restarted.
      if ( _isESOnline() )
      {
         if ( isDataNodePrimary() )
         {
            rc = _idxSessionMgr.refreshTasks() ;
            PD_RC_CHECK( rc, PDERROR, "Update text index information failed[ %d ]",
                         rc ) ;
            rc = _startInnerSession( SEADPT_SESSION_INDEX, &_idxSessionMgr ) ;
            PD_RC_CHECK( rc, PDERROR, "Start inner session failed[ %d ]", rc ) ;
            _indexerOn = TRUE ;
         }
      }
      else
      {
         if ( _indexerOn )
         {
            PD_LOG( PDERROR, "Can't connect to search engine, "
                    "stop all index tasks... " ) ;
            _idxSessionMgr.stopAllIndexer( handle ) ;
            _indexerOn = FALSE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_onRemoteDisconnect( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      _dbAssist.invalidNetHandle( handle ) ;
      if ( _dbAssist.dataNetHandleValid() )
      {
         PD_LOG( PDERROR, "Network broken with catalogue node" ) ;
      }
      else
      {
         // Kill the index update timer, and resume the register.
         if ( NET_INVALID_TIMER_ID != _idxUpdateTimerID )
         {
            _killTimer( _idxUpdateTimerID ) ;
            _idxUpdateTimerID = NET_INVALID_TIMER_ID ;
         }
         PD_LOG( PDERROR, "Network broken with data node. Stop all indexing "
                          "jobs and try to register on data node again..." ) ;
         _idxSessionMgr.stopAllIndexer( handle ) ;
         _indexerOn = FALSE ;
         rc = _resumeRegister() ;
         PD_RC_CHECK( rc, PDERROR, "Resume register failed[%d]", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_setTimers()
   {
      INT32 rc = SDB_OK ;

      // 1. Set the register timer. Before success, try to register every one
      //    second. Once success, the timer will be killed.
      rc = _dbAssist.routeAgent()->addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                             _regTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register registration timer failed[ %d ]",
                   rc ) ;

      // 2. Set the ES detection timer.
      rc = _dbAssist.routeAgent()->addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                             _esDetectTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register ES detect timer failed[ %d ]", rc ) ;

      // 3. Set the one second timer. This is for session managers to check
      //    deleting sessions.
      rc = _dbAssist.routeAgent()->addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                             _oneSecTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register one second timer failed[ %d ]", rc ) ;

      rc = _dbAssist.routeAgent()->addTimer( OSS_ONE_SEC * 60,
                                             &_indexTimerHandler,
                                             _cleanupConnnTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register connection cleanup timer failed[%d]",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seAdptCB::_killTimer( UINT32 timerID )
   {
      _dbAssist.routeAgent()->removeTimer( timerID ) ;
   }

   BOOLEAN _seAdptCB::_isESOnline()
   {
      BOOLEAN active = FALSE ;
      utilESClt *client = NULL ;
      utilESCltMgr *esCltMgr = utilGetESCltMgr() ;
      if ( SDB_OK ==  esCltMgr->getClient( client ) )
      {
         active = client->isActive() ;
         esCltMgr->releaseClient( client ) ;
      }

      return active ;
   }

   INT32 _seAdptCB::_cleanupESClts()
   {
      utilESCltMgr *esCltMgr = utilGetESCltMgr() ;
      esCltMgr->cleanup() ;
      return SDB_OK ;
   }

   seAdptCB* sdbGetSeAdapterCB()
   {
      static seAdptCB s_seAdptMgr ;
      return &s_seAdptMgr ;
   }

   seAdptOptionsMgr* sdbGetSeAdptOptions()
   {
      return sdbGetSeAdapterCB()->getOptions() ;
   }
}

