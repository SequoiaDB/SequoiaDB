/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "sptCommon.hpp"
#include "seAdptMgr.hpp"
#include "seAdptAgentSession.hpp"
#include "seAdptIndexSession.hpp"
#include "seAdptDef.hpp"
#include "msgMessage.hpp"

#define DATA_NODE_GRP_ID                        10000
#define DATA_NODE_ID                            10000
#define SEADPT_NAME_CAPPED_COLLECTION           "CappedCL"
#define SEADPT_INIT_TEXT_INDEX_VERSION          -1
#define SEADPT_IDX_UPDATE_INTERVAL              ( 5 * OSS_ONE_SEC )
#define SEADPT_CAT_RETRY_MAX_TIMES              3
#define SEADPT_PORT_STR_SZ                      10
#define SEADPT_MAX_PORT                         65535
#define SEADPT_SVC_PORT_PLUS                    7
#define ES_SYS_PREFIX                           "sys"

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
         ret = _reply( handle, rc, pReq ) ;
      }
      else
      {
         ret = rc  ;
      }

      return ret ;
   }

   pmdAsyncSession* _seSvcSessionMgr::_createSession( SDB_SESSION_TYPE sessionType,
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
      _indexSessionTimer = NET_INVALID_TIMER_ID ;
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

   INT32 _seIndexSessionMgr::refreshTasks( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      seIdxMetaMgr* idxMetaCache = _pAdptCB->getIdxMetaCache() ;

      try
      {
         idxMetaCache->lock( SHARED ) ;
         TASK_SESSION_MAP taskMapTmp ;
         const IDX_META_MAP* metaMap = idxMetaCache->getIdxMetaMap() ;

         for ( IDX_META_MAP_CITR mItr = metaMap->begin();
               mItr != metaMap->end(); ++mItr )
         {
            for ( IDX_META_VEC_CITR vItr = mItr->second.begin();
                  vItr != mItr->second.end(); ++vItr )
            {
               TASK_SESSION_ITEM* taskItem = _findTask( &*vItr ) ;
               if ( !taskItem )
               {
                  UINT64 sessionID = _newSessionID() ;
                  _pAdptCB->startInnerSession( SEADPT_SESSION_INDEX,
                                               sessionID,
                                               (void *)(&*vItr ) ) ;
                  taskMapTmp.insert( TASK_SESSION_ITEM(sessionID, *vItr ) ) ;
               }
               else
               {
                  taskMapTmp.insert( *taskItem ) ;
               }
            }
         }

         _taskSessionMap = taskMapTmp ;
         idxMetaCache->unlock( SHARED ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIndexSessionMgr::stopAllIndexer( const NET_HANDLE &handle )
   {
      handleSessionClose( handle ) ;
      _pAdptCB->cleanInnerSession( SEADPT_SESSION_INDEX ) ;
      _taskSessionMap.clear() ;
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
      _taskSessionMap.erase( pSession->sessionID() ) ;
      _pAdptCB->resetIdxVersion() ;
   }

   pmdAsyncSession* _seIndexSessionMgr::_createSession( SDB_SESSION_TYPE sessionType,
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
         seIndexMeta *idxMeta = (seIndexMeta *)data ;
         if ( idxMeta->valid() )
         {
            pSession = SDB_OSS_NEW seAdptIndexSession( sessionID, idxMeta ) ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Invalid session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   TASK_SESSION_ITEM* _seIndexSessionMgr::_findTask( const seIndexMeta *idxMeta )
   {
      TASK_SESSION_ITEM* item = NULL ;
      for ( TASK_SESSION_MAP_ITR itr = _taskSessionMap.begin() ;
            itr != _taskSessionMap.end(); ++itr )
      {
         if ( *idxMeta == itr->second )
         {
            item = &*itr ;
            break ;
         }
      }

      return item ;
   }

   BEGIN_OBJ_MSG_MAP( _seAdptCB, _pmdObjBase )
      ON_MSG( MSG_AUTH_VERIFY_RES, _onRegisterRes )
      ON_MSG( MSG_CAT_QUERY_CATALOG_RSP, _onCatalogResMsg )
      ON_MSG( MSG_SEADPT_UPDATE_IDXINFO_RES, _onIdxUpdateRes )
      ON_MSG( MSG_COM_REMOTE_DISC, _onRemoteDisconnect )
   END_OBJ_MSG_MAP()

   _seAdptCB::_seAdptCB()
   : _indexMsgHandler( &_idxSessionMgr ),
     _svcMsgHandler( &_svcSessionMgr ),
     _indexTimerHandler( &_idxSessionMgr ),
     _svcTimerHandler( &_svcSessionMgr ),
     _indexNetRtAgent( &_indexMsgHandler ),
     _svcRtAgent( &_svcMsgHandler ),
     _idxSessionMgr( this )
   {
      ossMemset( _serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
      _dataNodeID.value = MSG_INVALID_ROUTEID ;
      _cataNodeID.value = MSG_INVALID_ROUTEID ;
      _selfRouteID.value = MSG_INVALID_ROUTEID ;
      _peerPrimary = FALSE ;
      ossMemset( _peerGroupName, 0, OSS_MAX_GROUPNAME_SIZE + 1 ) ;
      _regTimerID = NET_INVALID_TIMER_ID ;
      _idxUpdateTimerID = NET_INVALID_TIMER_ID ;
      _oneSecTimerID = NET_INVALID_TIMER_ID ;
      _clVersion = -1 ;
      _localIdxVer = SEADPT_INIT_TEXT_INDEX_VERSION ;
      _regMsgBuff = NULL ;
      _esClt = NULL ;
      _indexerOn = FALSE ;
   }

   _seAdptCB::~_seAdptCB()
   {
      if ( _regMsgBuff )
      {
         SDB_OSS_FREE( _regMsgBuff ) ;
      }
      if ( _esClt )
      {
         SDB_OSS_DEL _esClt ;
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
      std::string seSvcPath ;

      _options.setConfigHandler( pmdGetKRCB() ) ;

      rc = _startSvcListener() ;
      PD_RC_CHECK( rc, PDERROR, "Start service listener failed[ %d ]", rc ) ;

      rc = _initSdbAddr() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init sdb data node address failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = _idxSessionMgr.init( &_indexNetRtAgent, &_indexTimerHandler,
                                5 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Init index session manager failed[ %d ]",
                   rc ) ;

      rc = _svcSessionMgr.init( &_svcRtAgent, &_svcTimerHandler,
                                60 * OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Init service session manager failed[ %d ]",
                   rc ) ;

      seSvcPath = std::string( _options.getSeHost() ) + ":"
                  + std::string( _options.getSeService() ) ;
      rc = _seCltFactory.init( seSvcPath, _options.getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init search engine client manager failed[ %d ]",
                 rc ) ;
         goto error ;
      }
      PD_LOG( PDEVENT, "Search engine client manager init successfully" ) ;

      rc = _seCltFactory.create( &_esClt ) ;
      PD_RC_CHECK( rc, PDERROR, "Create search engine client failed[ %d ]",
                   rc ) ;

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
      rc = pEDUMgr->startEDU( EDU_TYPE_SEADPTMGR, (_pmdObjBase*)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start search engine adapter manager "
                   "edu, rc: %d", rc ) ;
      rc = _attachEvent.wait( SEADPT_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait search engine adapter manager edu attach "
                   "failed, rc: %d", rc ) ;

      rc = pEDUMgr->startEDU( EDU_TYPE_SE_INDEXR, &_indexNetRtAgent, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start search engine adapter net, "
                   "rc: %d", rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait indexer reader network daemons active "
                   "failed[ %d ]", rc ) ;

      rc = pEDUMgr->startEDU( EDU_TYPE_SE_SERVICE, (void *)&_svcRtAgent,
                              &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Start service listener failed[ %d ]", rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait service listener active failed[ %d ]",
                   rc ) ;

      rc = _setTimers() ;
      PD_RC_CHECK( rc, PDERROR, "Set timers failed[ %d ]", rc ) ;

      _sendRegisterMsg() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::deactive()
   {
      _svcRtAgent.closeListen() ;

      _indexNetRtAgent.stop() ;
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
      else if ( timerID == _oneSecTimerID )
      {
         _idxSessionMgr.onTimer( interval ) ;
         _svcSessionMgr.onTimer( interval ) ;
      }
      else if ( timerID == _idxUpdateTimerID )
      {
         if ( NET_INVALID_TIMER_ID == _regTimerID )
         {
            INT32 rc = _sendIdxUpdateReq() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send index update request failed[ %d ]", rc ) ;
            }
         }
      }

      return ;
   }

   seAdptOptionsMgr* _seAdptCB::getOptions()
   {
      return &_options ;
   }

   utilESCltFactory* _seAdptCB::getSeCltFactory()
   {
      return &_seCltFactory ;
   }

   seSvcSessionMgr* _seAdptCB::getSeAgentMgr()
   {
      return &_svcSessionMgr ;
   }

   seIndexSessionMgr* _seAdptCB::getIdxSessionMgr()
   {
      return &_idxSessionMgr ;
   }

   netRouteAgent* _seAdptCB::getIdxRouteAgent()
   {
      return &_indexNetRtAgent ;
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
            _vecInnerSessionParam.erase( itr ) ;
         }
         else
         {
            ++itr ;
         }
      }
   }

   INT32 _seAdptCB::sendToDataNode( MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      rc = _indexNetRtAgent.syncSend( _dataNodeID, (void *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to data node failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::syncUpdateCLVersion( const CHAR *collectionName,
                                         INT64 millsec, pmdEDUCB *cb,
                                         INT32 &version )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      BSONObj selector ;
      BOOLEAN needRetry = FALSE ;
      UINT32 retryTimes = 0 ;
      BOOLEAN hasSent = FALSE ;

      try
      {
         query = BSON( FIELD_NAME_NAME << collectionName ) ;
         selector = BSON( FIELD_NAME_VERSION << "" ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred when creating query: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   retry:
      _verUpdateLock.get() ;
      _clVersion = -1 ;
      if ( !hasSent )
      {
         _cataEvent.reset() ;
         rc = _sendCataQueryReq( query, selector, 0, cb ) ;
         if ( SDB_OK == rc )
         {
            hasSent = TRUE ;
         }
      }

      if ( SDB_OK == rc )
      {
         INT32 result = SDB_OK ;
         rc = _cataEvent.wait( millsec, &result ) ;
         if ( SDB_OK == rc )
         {
            rc = result ;
            if ( SDB_DMS_EOC == rc || SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_DMS_NOTEXIST ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               needRetry = TRUE ;
               hasSent = FALSE ;
            }
         }

         if ( SDB_OK == rc )
         {
            version = _clVersion ;
         }
         else if ( SDB_NET_CANNOT_CONNECT == rc ||
                   SDB_NETWORK_CLOSE == rc ||
                   SDB_CLS_NOT_PRIMARY == rc )
         {
            needRetry = TRUE ;
         }
         else if ( rc && SDB_DMS_NOTEXIST != rc )

         {
            needRetry = FALSE ;
         }

      }

      _verUpdateLock.release() ;

      if ( needRetry && retryTimes < SEADPT_CAT_RETRY_MAX_TIMES )
      {
         goto retry ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seAdptCB::resetIdxVersion()
   {
      _localIdxVer = SEADPT_INIT_TEXT_INDEX_VERSION ;
   }

   INT32 _seAdptCB::_onCatalogResMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> docObjs ;
      MsgOpReply *res = (MsgOpReply *)msg ;

      if ( SDB_OK != res->flags )
      {
         if ( SDB_DMS_EOC == res->flags || SDB_DMS_NOTEXIST == res->flags )
         {
            _cataEvent.signalAll( SDB_DMS_NOTEXIST ) ;
         }
         else
         {
            _cataEvent.signalAll( res->flags ) ;
         }
      }
      else
      {
         rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                               &numReturned, docObjs ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to extract query result, rc: %d", rc ) ;
            goto error ;
         }

         rc = flag ;
         PD_RC_CHECK( rc, PDERROR, "Error returned from catalog[ %d ]", rc ) ;

         SDB_ASSERT( 1 == numReturned && 1 == docObjs.size(),
                     "Respond size from catalog is wrong" ) ;
         _clVersion = docObjs[0].getIntField( FIELD_NAME_VERSION ) ;
         _cataEvent.signalAll( SDB_OK ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_sendCataQueryReq( const BSONObj &query,
                                       const BSONObj &selector,
                                       UINT64 requestID,
                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *msg = NULL ;
      INT32 buffSize = 0 ;
      INT32 flag = FLG_QUERY_WITH_RETURNDATA ;

      rc = msgBuildQueryCatalogReqMsg( (CHAR **)&msg, &buffSize, flag, 0, 0,
                                       -1, 0, &query, &selector,
                                       NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query catalog message failed[ %d ]",
                   rc ) ;

      rc = _indexNetRtAgent.syncSend( _cataNodeID, (void *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to cata node failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_updateIndexInfo( const NET_HANDLE &handle, BSONObj &obj,
                                      BOOLEAN &updated, BOOLEAN &upgrade )
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
                  upgrade = TRUE ;
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
               BSONElement idxEles = obj.getField( FIELD_NAME_INDEXES ) ;
               if ( EOO == idxEles.type() )
               {
                  PD_LOG( PDERROR, "No index information found" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               {
                  vector<BSONElement> idxElements = idxEles.Array() ;
                  for ( vector<BSONElement>::iterator itr = idxElements.begin();
                        itr != idxElements.end(); ++itr )
                  {
                     seIndexMeta idxMeta ;
                     seIndexMeta *oldMeta = NULL ;
                     rc = _parseIndexInfo( &*itr, idxMeta ) ;
                     if ( rc )
                     {
                        PD_LOG( PDERROR, "Parse index definition failed[ %d ]",
                                rc ) ;
                        continue ;
                     }

                     idxMeta.setVersion( peerVersion ) ;
                     _idxMetaCache.lock( EXCLUSIVE ) ;
                     oldMeta = _idxMetaCache.getIdxMeta( idxMeta ) ;
                     if ( !oldMeta )
                     {
                        _idxMetaCache.addIdxMeta( idxMeta.getOrigCLName().c_str(),
                                                  idxMeta ) ;
                        PD_LOG( PDDEBUG, "New index metadata added: %s",
                                idxMeta.toString().c_str() ) ;
                     }
                     else
                     {
                        oldMeta->setVersion( peerVersion ) ;
                     }
                     _idxMetaCache.unlock( EXCLUSIVE ) ;
                  }

                  PD_LOG( PDDEBUG, "Change local version from [ %d ] to [ %d ]",
                          _localIdxVer, peerVersion ) ;
                  _localIdxVer = peerVersion ;
                  updated = TRUE ;

                  _idxMetaCache.lock( EXCLUSIVE ) ;
                  _idxMetaCache.cleanByVer( peerVersion ) ;
                  _idxMetaCache.unlock( EXCLUSIVE ) ;
               }
            }
            else
            {
               updated = FALSE ;
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

   INT32 _seAdptCB::_parseIndexInfo( const BSONElement *ele,
                                     seIndexMeta &idxMeta )
   {
      INT32 rc = SDB_OK ;
      const CHAR *clName = NULL ;
      const CHAR *idxName = NULL  ;
      const CHAR *cappedCLName = NULL ;
      BSONObj idxDef ;
      BSONObj key ;
      BSONElement lidEle ;
      UINT32 csLogicalID = 0 ;
      UINT32 clLogicalID = 0 ;
      UINT32 idxLogicalID = 0 ;

      try
      {
         BSONObj idxObj = ele->Obj() ;
         clName = idxObj.getStringField( FIELD_NAME_COLLECTION ) ;
         if ( 0 == ossStrlen( clName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collection name not found in index infor" ) ;
            goto error ;
         }

         cappedCLName = idxObj.getStringField( SEADPT_NAME_CAPPED_COLLECTION ) ;
         if ( 0 == ossStrlen( cappedCLName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Capped collection name not found in index info" ) ;
            goto error ;
         }

         idxDef = idxObj.getObjectField( FIELD_NAME_INDEX ) ;
         if ( idxDef.isEmpty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "No valid index definition in index info" ) ;
            goto error ;
         }

         key = idxDef.getObjectField( IXM_FIELD_NAME_KEY ) ;
         if ( key.isEmpty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "No valid key definition in index info" ) ;
            goto error ;
         }

         idxName = idxDef.getStringField( IXM_FIELD_NAME_NAME ) ;
         if ( 0 == ossStrlen( idxName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get index name from definition failed" ) ;
            goto error ;
         }

         lidEle = idxObj.getField( FIELD_NAME_LOGICAL_ID ) ;
         if ( Array != lidEle.type() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get logical id array from definition failed" ) ;
            goto error ;
         }

         {
            BSONObj lidObj = lidEle.embeddedObject() ;
            if ( 3 != lidObj.nFields() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Logical id field size is not as expected. "
                       "Expected: 2, Actual: %d", lidObj.nFields() ) ;
               goto error ;
            }
            else
            {
               csLogicalID = (UINT32)lidObj.getIntField( "0" ) ;
               clLogicalID = (UINT32)lidObj.getIntField( "1" ) ;
               idxLogicalID = (UINT32)lidObj.getIntField( "2" ) ;
            }
         }

         idxMeta.setCLName( clName ) ;
         idxMeta.setIdxName( idxName ) ;
         idxMeta.setCappedCLName( cappedCLName ) ;
         rc = idxMeta.setIdxDef( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Set index difinition failed[ %d ]", rc ) ;

         idxMeta.setCSLogicalID( csLogicalID ) ;
         idxMeta.setCLLogicalID( clLogicalID ) ;
         idxMeta.setIdxLogicalID( idxLogicalID ) ;

         _genESIdxName( idxMeta ) ;
         idxMeta.setESIdxType( _peerGroupName ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happed: %s", e.what() ) ;
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

      ossSocket::getPort( _options.getDbService(), sdbPort ) ;

      svcRtID.columns.groupID = SDB_SEADPT_GRP_ID ;
      svcRtID.columns.nodeID = SDB_SEADPT_NODE_ID ;
      svcRtID.columns.serviceID = SDB_SEADPT_SVC_ID ;

      INT32 svcPort = sdbPort + SEADPT_SVC_PORT_PLUS;
      while ( svcPort <= SEADPT_MAX_PORT )
      {
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
      INT32 rc = SDB_OK ;
      UINT16 port = 0 ;

      _dataNodeID.columns.groupID = DATA_NODE_GRP_ID ;
      _dataNodeID.columns.nodeID = DATA_NODE_ID ;

      _dataNodeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
      ossSocket::getPort( _options.getDbService(), port ) ;
      port += MSG_ROUTE_SHARD_SERVCIE ;

      std::stringstream portStr ;
      portStr << port ;

      rc = _indexNetRtAgent.updateRoute( _dataNodeID, _options.getDbHost(),
                                         portStr.str().c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update route failed[ %d ]", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_sendRegisterMsg()
   {
      INT32 rc = SDB_OK ;
      BSONObj authObj ;
      INT32 msgLen = 0 ;
      MsgAuthentication *authMsg = NULL ;

      try
      {
         authObj = BSON( FIELD_NAME_HOST << pmdGetKRCB()->getHostName()
                         << FIELD_NAME_SERVICE_NAME << _options.getSvcName()
                         << FIELD_NAME_GROUPID << SDB_SEADPT_GRP_ID
                         << FIELD_NAME_NODEID << SDB_SEADPT_NODE_ID
                         << FIELD_NAME_SERVICE_TYPE << SDB_SEADPT_SVC_ID ) ;
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
      authMsg->header.TID = 0 ;
      ossMemcpy( (CHAR *)authMsg + sizeof( MsgAuthentication ),
                 authObj.objdata(), authObj.objsize() ) ;

      rc = sendToDataNode( (MsgHeader *)authMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send register request to data node "
                   "failed[ %d ]", rc ) ;
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

   INT32 _seAdptCB::_resumeRegister()
   {
      INT32 rc = SDB_OK ;

      rc = _indexNetRtAgent.addTimer( OSS_ONE_SEC, &_indexTimerHandler,
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

      string cataHost ;
      string cataSvc ;
      const CHAR *peerGrpName = NULL ;
      MsgOpReply *res = (MsgOpReply *)msg ;

      if ( NET_INVALID_TIMER_ID == _regTimerID )
      {
         goto done ;
      }

      rc = res->flags ;
      PD_RC_CHECK( rc, PDERROR, "Adapter register on data node failed[ %d ]",
                   rc ) ;

      try
      {
         rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                               &numReturned, objVec ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract register reply failed[ %d ]", rc ) ;
         rc = flag ;
         PD_RC_CHECK( rc, PDERROR, "Error returned from data node[ %d ]", rc ) ;
         if ( 1 != objVec.size() )
         {
            PD_LOG( PDERROR, "Register reply is not as expected" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _peerPrimary = objVec[0].getBoolField( FIELD_NAME_IS_PRIMARY ) ;
         peerGrpName = objVec[0].getStringField( FIELD_NAME_GROUPNAME ) ;
         if ( !peerGrpName )
         {
            PD_LOG( PDERROR, "Find peer node group name failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         ossStrncpy( _peerGroupName, peerGrpName, OSS_MAX_GROUPNAME_SIZE ) ;
         _peerGroupName[ OSS_MAX_GROUPNAME_SIZE ] = '\0' ;

         cataInfoObj = objVec[0].getObjectField( FIELD_NAME_CATALOGINFO ) ;
         if ( cataInfoObj.isEmpty() )
         {
            PD_LOG( PDERROR, "Find catalog info in object failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         cataHost = cataInfoObj.getStringField( FIELD_NAME_HOST ) ;
         cataSvc = cataInfoObj.getStringField( FIELD_NAME_SERVICE_NAME ) ;
         _cataNodeID.columns.groupID =
            cataInfoObj.getIntField( FIELD_NAME_GROUPID ) ;
         _cataNodeID.columns.nodeID =
            cataInfoObj.getIntField( FIELD_NAME_NODEID ) ;
         _cataNodeID.columns.serviceID =
            cataInfoObj.getIntField( FIELD_NAME_SERVICE ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = _indexNetRtAgent.updateRoute( _cataNodeID, cataHost.c_str(),
                                         cataSvc.c_str() ) ;
      if ( rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
      {
         PD_LOG( PDERROR, "Update route failed[ %d ]", rc ) ;
         goto error ;
      }

      _killTimer( _regTimerID ) ;
      _regTimerID = NET_INVALID_TIMER_ID ;

      pmdGetKRCB()->setBusinessOK( TRUE ) ;

      PD_LOG( PDEVENT, "Search engine adapter registered on SequoiaDB data "
              "node successfully" ) ;
      if ( TRUE == _peerPrimary )
      {
         PD_LOG( PDEVENT, "Peer node is primary. Search engine adapter is "
                 "running in READWRITE mode" ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Peer node is not primary. Search engine adapter is "
                 "running in READONLY mode" ) ;
      }

      rc = _indexNetRtAgent.addTimer( SEADPT_IDX_UPDATE_INTERVAL,
                                      &_indexTimerHandler,
                                      _idxUpdateTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register timer failed[ %d ]", rc ) ;

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

      rc = pEDUMgr->startEDU( (EDU_TYPES)type, (void *)agrs, &eduID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create EDU[type:%d(%s)], rc = %d",
                  type, getEDUName( (EDU_TYPES)type ), rc );
         goto error ;
      }

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

   INT32 _seAdptCB::_sendIdxUpdateReq()
   {
      INT32 rc = SDB_OK ;

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

      rc = sendToDataNode( _regMsgBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to data node failed[ %d ]", rc ) ;
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
      BSONElement ele ;
      BOOLEAN updated = FALSE ;
      BOOLEAN upgrade = FALSE ;

      rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                            &numReturned, objVec ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract index update reply failed[ %d ]",
                   rc ) ;
      rc = flag ;
      PD_RC_CHECK( rc, PDERROR, "Index update request failed[ %d ]", rc ) ;

      if ( 1 != objVec.size() )
      {
         PD_LOG( PDERROR, "Register reply is not as expected" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _updateIndexInfo( handle, objVec[0], updated, upgrade ) ;
      PD_RC_CHECK( rc, PDERROR, "Update indices information failed[ %d ]",
                   rc ) ;

      if ( _isESOnline() )
      {
         if ( ( updated && isDataNodePrimary() ) || upgrade || !_indexerOn )
         {
            rc = _idxSessionMgr.refreshTasks( objVec[0] ) ;
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

      if ( NET_INVALID_TIMER_ID != _idxUpdateTimerID )
      {
         _killTimer( _idxUpdateTimerID ) ;
         _idxUpdateTimerID = NET_INVALID_TIMER_ID ;
      }

      _idxMetaCache.lock( EXCLUSIVE ) ;
      _idxMetaCache.clear() ;
      _idxMetaCache.unlock( EXCLUSIVE ) ;

      PD_LOG( PDEVENT, "Network broken with data node. Stop all indexing jobs "
              "and try to register on data node again..." ) ;
      _idxSessionMgr.stopAllIndexer( handle ) ;

      rc = _resumeRegister() ;
      PD_RC_CHECK( rc, PDERROR, "Resume register failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptCB::_setTimers()
   {
      INT32 rc = SDB_OK ;

      rc = _indexNetRtAgent.addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                      _regTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register timer failed[ %d ]", rc ) ;

      rc = _indexNetRtAgent.addTimer( OSS_ONE_SEC, &_indexTimerHandler,
                                      _oneSecTimerID ) ;
      PD_RC_CHECK( rc, PDERROR, "Register timer failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seAdptCB::_killTimer( UINT32 timerID )
   {
      _indexNetRtAgent.removeTimer( timerID ) ;
   }

   void _seAdptCB::_genESIdxName( UINT32 csLID, UINT32 clLID, INT32 idxLID,
                                  CHAR *esIdxName, UINT32 buffSize )
   {
      ossSnprintf( esIdxName, buffSize, ES_SYS_PREFIX"_%u_%u_%d",
                   csLID, clLID, idxLID ) ;
   }

   void _seAdptCB::_genESIdxName( seIndexMeta &idxMeta )
   {
      std::string::size_type pos = idxMeta.getCappedCLName().find('.') ;
      std::string cappedCLName = idxMeta.getCappedCLName().substr( pos + 1 ) ;
      std::string esIdx = cappedCLName + "_" + _peerGroupName ;
      std::transform( esIdx.begin(), esIdx.end(), esIdx.begin(), ::tolower ) ;
      idxMeta.setESIdxName( esIdx.c_str() ) ;
   }

   BOOLEAN _seAdptCB::_isESOnline()
   {
      return _esClt->isActive() ;
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

   seSvcSessionMgr* sdbGetSeAgentCB()
   {
      return sdbGetSeAdapterCB()->getSeAgentMgr() ;
   }

   utilESCltFactory* sdbGetSeCltFactory()
   {
      return sdbGetSeAdapterCB()->getSeCltFactory() ;
   }
}

