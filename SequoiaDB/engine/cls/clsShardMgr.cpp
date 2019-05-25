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

   Source File Name = clsShardMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsShardMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "msgAuth.hpp"
#include "msgCatalog.hpp"
#include "pmdController.hpp"
#include "../bson/bson.h"
#include "rtnQueryOptions.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtnExtDataHandler.hpp"

using namespace bson ;

#define CLS_NAME_CAPPED_COLLECTION           "CappedCL"

namespace engine
{

   #define CLS_CATA_RETRY_MAX_TIMES          ( 4 )

   struct _hostAndPort
   {
      std::string _host ;
      std::string _svc ;
      MsgRouteID  _nodeID ;
      INT32       _status ;
      INT32       _result ;

      _hostAndPort( std::string host, std::string svc, MsgRouteID nodeID )
      {
         _host = host ;
         _svc = svc ;
         _nodeID.value = nodeID.value ;
         _status = NET_NODE_STAT_NORMAL ;
         _result = SDB_OK ;
      }
      _hostAndPort()
      {
         _nodeID.value = MSG_INVALID_ROUTEID ;
         _status = NET_NODE_STAT_NORMAL ;
         _result = SDB_OK ;
      }
      BOOLEAN isNormal() const
      {
         return ( NET_NODE_STAT_NORMAL == _status ? TRUE : FALSE ) ;
      }
   } ;

   static INT32 _clsSelectNodes( clsGroupItem *pItem,
                                 BOOLEAN primary,
                                 MSG_ROUTE_SERVICE_TYPE type,
                                 std::vector< _hostAndPort > &hosts )
   {
      _hostAndPort hostItem ;
      INT32 tmpPrimary = pItem->getPrimaryPos() ;

      hosts.clear() ;

      if ( primary &&
           SDB_OK == pItem->getNodeInfo( tmpPrimary,
                                         hostItem._nodeID,
                                         hostItem._host,
                                         hostItem._svc,
                                         type ) &&
           SDB_OK == pItem->getNodeInfo( tmpPrimary,
                                         hostItem._status ) &&
           hostItem.isNormal() )
      {
         hosts.push_back( hostItem ) ;
      }
      else
      {
         for ( UINT32 pos = 0 ; pos < pItem->nodeCount() ; ++pos )
         {
            if ( SDB_OK == pItem->getNodeInfo( pos, hostItem._nodeID,
                                               hostItem._host, hostItem._svc,
                                               type ) &&
                 SDB_OK == pItem->getNodeInfo( pos, hostItem._status ) &&
                 hostItem.isNormal() )
            {
               hosts.push_back( hostItem ) ;
            }
         }
      }

      return hosts.size() > 0 ? SDB_OK : SDB_CLS_NODE_NOT_EXIST ;
   }

   void _clsUpdateNodeStatus( clsGroupItem *pItem,
                              std::vector< _hostAndPort > &hosts )
   {
      for ( UINT32 i = 0 ; i < hosts.size() ; ++i )
      {
         _hostAndPort &info = hosts[ i ] ;
         if ( info._result )
         {
            pItem->updateNodeStat( info._nodeID.columns.nodeID,
                                   netResult2Status( info._result ) ) ;
         }
      }
   }

   /*
      _clsFreezingWindow implement
   */
   _clsFreezingWindow::_clsFreezingWindow()
   {
      _clCount = 0 ;
   }

   _clsFreezingWindow::~_clsFreezingWindow()
   {
   }

   void _clsFreezingWindow::registerCL ( const CHAR * pName,
                                         const CHAR * pMainCLName,
                                         UINT64 & opID )
   {
      _latch.get() ;

      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      if ( NULL != pName && '\0' != pName[0] )
      {
         _registerCLInternal( pName, opID ) ;
      }
      if ( NULL != pMainCLName && '\0' != pMainCLName[0] )
      {
         _registerCLInternal( pMainCLName, opID ) ;
      }

      _latch.release() ;
   }

   void _clsFreezingWindow::_registerCLInternal ( const CHAR * pName,
                                                  UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.find( pName ) ;

      if ( _mapWindow.end() == it )
      {
         ++_clCount ;

         OP_SET newOpSet ;
         newOpSet.insert( opID ) ;

         _mapWindow[ pName ] = newOpSet ;
      }
      else
      {
         it->second.insert( opID ) ;
      }
   }

   void _clsFreezingWindow::unregisterCL ( const CHAR * pName,
                                           const CHAR * pMainCLName,
                                           UINT64 opID )
   {
      _latch.get() ;

      if ( NULL != pName && '\0' != pName[0] )
      {
         _unregisterCLInternal( pName, opID ) ;
      }
      if ( NULL != pMainCLName && '\0' != pMainCLName[0] )
      {
         _unregisterCLInternal( pMainCLName, opID ) ;
      }

      _latch.release() ;
      _event.signalAll() ;
   }

   void _clsFreezingWindow::_unregisterCLInternal ( const CHAR * pName,
                                                    UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.find( pName ) ;

      if ( _mapWindow.end() != it )
      {
         it->second.erase( opID ) ;

         if ( it->second.empty() )
         {
            _mapWindow.erase( pName ) ;
            --_clCount ;
         }
      }
   }

   BOOLEAN _clsFreezingWindow::needBlockOpr( const CHAR *pName,
                                             UINT64 testOpID )
   {
      MAP_WINDOW::iterator it ;
      BOOLEAN needBlock = FALSE ;

      _latch.get() ;

      if ( !_mapWindow.empty() &&
           _mapWindow.end() != ( it = _mapWindow.find( pName ) ) )
      {
         OP_SET::iterator opIt = it->second.begin() ;

         while ( opIt != it->second.end () )
         {
            if ( *opIt == testOpID )
            {
               needBlock = FALSE ;
               break ;
            }
            else if ( *opIt < testOpID )
            {
               needBlock = TRUE ;
            }
            opIt ++ ;
         }
      }

      _latch.release() ;

      return needBlock ;
   }

   INT32 _clsFreezingWindow::waitForOpr( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         BOOLEAN isWrite )
   {
      INT32 rc = SDB_OK ;

      if ( isWrite && _clCount > 0 )
      {
         string clName = pName ;
         BOOLEAN needBlock = TRUE ;
         MAP_WINDOW::iterator it ;
         UINT64 opID = cb->getWritingID() ;

         while( needBlock )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               break ;
            }

            needBlock = needBlockOpr( clName.c_str(), opID ) ;

            if ( needBlock )
            {
               _event.reset() ;
               _event.wait( OSS_ONE_SEC ) ;
            }
         }
      }

      return rc ;
   }

   BEGIN_OBJ_MSG_MAP(_clsShardMgr, _pmdObjBase)
      ON_MSG ( MSG_CAT_CATGRP_RES, _onCatCatGroupRes )
      ON_MSG ( MSG_CAT_NODEGRP_RES, _onCatGroupRes )
      ON_MSG ( MSG_CAT_QUERY_CATALOG_RSP, _onCatalogReqMsg )
      ON_MSG ( MSG_CAT_QUERY_SPACEINFO_RSP, _onQueryCSInfoRsp )
      ON_MSG ( MSG_COM_REMOTE_DISC, _onHandleClose )
      ON_MSG ( MSG_AUTH_VERIFY_REQ, _onAuthReqMsg )
      ON_MSG ( MSG_SEADPT_UPDATE_IDXINFO_REQ, _onTextIdxInfoReqMsg )
   END_OBJ_MSG_MAP()

   _clsShardMgr::_clsShardMgr ( _netRouteAgent *rtAgent )
   :_cataGrpItem( CATALOG_GROUPID )
   {
      _pNetRtAgent = rtAgent ;
      _requestID = 0 ;
      _pCatAgent = NULL ;
      _pNodeMgrAgent = NULL ;
      _pFreezingWindow = NULL ;
      _pDCMgr = NULL ;

      _catVerion = 0 ;
      _nodeID.value = 0 ;
      _seAdptID.value = INVALID_NODE_ID ;
      _upCatHandle = NET_INVALID_HANDLE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_DECONSTRUCTOR, "_clsShardMgr::~_clsShardMgr" )
   _clsShardMgr::~_clsShardMgr()
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_DECONSTRUCTOR );
      _pNetRtAgent = NULL ;

      SAFE_DELETE ( _pCatAgent ) ;
      SAFE_DELETE ( _pNodeMgrAgent ) ;
      SAFE_DELETE ( _pFreezingWindow ) ;
      SAFE_DELETE ( _pDCMgr ) ;

      MAP_CAT_EVENT_IT it = _mapSyncCatEvent.begin () ;
      while ( it != _mapSyncCatEvent.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _mapSyncCatEvent.clear () ;

      MAP_NM_EVENT_IT itNM = _mapSyncNMEvent.begin () ;
      while ( itNM != _mapSyncNMEvent.end() )
      {
         SDB_OSS_DEL itNM->second ;
         ++itNM ;
      }
      _mapSyncNMEvent.clear () ;
      PD_TRACE_EXIT ( SDB__CLSSHDMGR_DECONSTRUCTOR );
   }

   void _clsShardMgr::setNodeID ( const MsgRouteID & nodeID )
   {
      _nodeID = nodeID ;
      _nodeID.columns.serviceID = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_INIT, "_clsShardMgr::initialize" )
   INT32 _clsShardMgr::initialize()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_INIT );
      UINT32 catGID = CATALOG_GROUPID ;
      UINT16 catNID = DATA_NODE_ID_END + 1 ;
      MsgRouteID id ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      vector< _pmdAddrPair > catAddrs = optCB->catAddrs() ;
      _netFrame *pNetFrame = NULL ;

      if ( !_pNetRtAgent )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "network runtime agent can't be NULL, rc = %d",
                  rc ) ;
         goto error ;
      }
      pNetFrame = _pNetRtAgent->getFrame() ;
      pNetFrame->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;
      sdbGetPMDController()->registerNet( pNetFrame,
                                          MSG_ROUTE_CAT_SERVICE ) ;

      for ( UINT32 i = 0 ; i < catAddrs.size() ; ++i )
      {
         if ( 0 == catAddrs[i]._host[ 0 ] )
         {
            break ;
         }
         id.columns.groupID = catGID ;
         id.columns.nodeID = catNID++ ;
         id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         setCatlogInfo( id, catAddrs[i]._host, catAddrs[i]._service ) ;
         _pNetRtAgent->updateRoute( id, catAddrs[i]._host,
                                    catAddrs[i]._service ) ;
      }

      if ( _mapNodes.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Catalog information was not properly configured, "
                  "rc = %d", rc ) ;
         goto error ;
      }

      rc = _cataGrpItem.updateNodes( _mapNodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Update catalog group info failed, rc: %d",
                   rc ) ;

      SAFE_NEW_GOTO_ERROR  ( _pCatAgent, _clsCatalogAgent ) ;
      SAFE_NEW_GOTO_ERROR  ( _pNodeMgrAgent, _clsNodeMgrAgent ) ;
      SAFE_NEW_GOTO_ERROR  ( _pFreezingWindow, _clsFreezingWindow ) ;
      SAFE_NEW_GOTO_ERROR  ( _pDCMgr, _clsDCMgr ) ;

      rc = _pDCMgr->initialize() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init datacenter manager failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::active ()
   {
      return SDB_OK ;
   }

   INT32 _clsShardMgr::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _clsShardMgr::final ()
   {
      if ( _pNetRtAgent )
      {
         sdbGetPMDController()->unregNet( _pNetRtAgent->getFrame() ) ;
      }
      return SDB_OK ;
   }

   void _clsShardMgr::onConfigChange ()
   {
      if ( _pNetRtAgent )
      {
         UINT32 opTimeout = pmdGetOptionCB()->getOprTimeout() ;
         _pNetRtAgent->getFrame()->setBeatInfo( opTimeout ) ;
      }
   }

   void _clsShardMgr::ntyPrimaryChange( BOOLEAN primary,
                                        SDB_EVENT_OCCUR_TYPE type )
   {
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         _pCatAgent->lock_w() ;
         _pCatAgent->clearAll() ;
         _pCatAgent->release_w() ;

         pmdGetKRCB()->getDMSCB()->clearSUCaches( DMS_EVENT_MASK_ALL ) ;
      }
      else if ( primary && SDB_EVT_OCCUR_AFTER == type )
      {
      }
   }

   void _clsShardMgr::attachCB( _pmdEDUCB * cb )
   {
      sdbGetClsCB()->attachCB( cb ) ;
   }

   void _clsShardMgr::detachCB( _pmdEDUCB * cb )
   {
      sdbGetClsCB()->detachCB( cb ) ;
   }

   void _clsShardMgr::onTimer ( UINT32 timerID, UINT32 interval )
   {
   }

   NodeID _clsShardMgr::nodeID () const
   {
      return _nodeID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SETCATINFO, "_clsShardMgr::setCatlogInfo" )
   void _clsShardMgr::setCatlogInfo ( const NodeID & id,
                                      const std::string& host,
                                      const std::string& service )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SETCATINFO );
      _netRouteNode info ;
      info._id.value = id.value ;
      info._id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
      ossStrncpy( info._host, host.c_str(), OSS_MAX_HOSTNAME ) ;
      info._service[ MSG_ROUTE_CAT_SERVICE ] = service ;
      _mapNodes[ info._id.value ] = info ;

      PD_TRACE_EXIT ( SDB__CLSSHDMGR_SETCATINFO );
   }

   catAgent* _clsShardMgr::getCataAgent ()
   {
      return _pCatAgent ;
   }

   nodeMgrAgent* _clsShardMgr::getNodeMgrAgent ()
   {
      return _pNodeMgrAgent ;
   }

   clsFreezingWindow* _clsShardMgr::getFreezingWindow()
   {
      return _pFreezingWindow ;
   }

   clsDCMgr* _clsShardMgr::getDCMgr()
   {
      return _pDCMgr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCSND, "_clsShardMgr::syncSend" )
   INT32 _clsShardMgr::syncSend( MsgHeader * msg, UINT32 groupID,
                                 BOOLEAN primary, MsgHeader **ppRecvMsg,
                                 INT64 millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCSND ) ;
      INT32 rc = SDB_OK ;
      std::vector< _hostAndPort > hosts ;
      BOOLEAN hasUpdateGroup = FALSE ;

   retry:
      if ( CATALOG_GROUPID == groupID )
      {
         ossScopedLock lock ( &_shardLatch, SHARED ) ;
         rc = _clsSelectNodes( &_cataGrpItem, primary,
                               MSG_ROUTE_CAT_SERVICE,
                               hosts ) ;
      }
      else
      {
         clsGroupItem *item = NULL ;
         rc = getAndLockGroupItem( groupID, &item, TRUE, CLS_SHARD_TIMEOUT,
                                   &hasUpdateGroup ) ;
         if ( SDB_OK == rc )
         {
            rc = _clsSelectNodes( item, primary, MSG_ROUTE_SHARD_SERVCIE,
                                  hosts ) ;
            unlockGroupItem( item ) ;
         }
      }

      if ( SDB_CLS_NODE_NOT_EXIST == rc && !hasUpdateGroup )
      {
         goto update_group ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to find nodes for sync send, "
                   "group id = %d, rc = %d", groupID, rc ) ;

      {
         UINT32 msgLength = 0 ;
         INT32 receivedLen = 0 ;
         INT32 sentLen = 0 ;
         CHAR* buff = NULL ;
         UINT16 port = 0 ;
         UINT32 pos = ossRand() % hosts.size() ;

         for ( UINT32 i = 0 ; i < hosts.size() ; ++i )
         {
            _hostAndPort &tmpInfo = hosts[pos] ;
            pos = ( pos + 1 ) % hosts.size() ;
            rc = ossGetPort( tmpInfo._svc.c_str(), port ) ;
            PD_RC_CHECK( rc, PDERROR, "Invalid svcname: %s",
                         tmpInfo._svc.c_str() ) ;

            ossSocket tmpSocket ( tmpInfo._host.c_str(), port, millisec ) ;
            rc = tmpSocket.initSocket() ;
            PD_RC_CHECK( rc, PDERROR, "Init socket %s:%d failed, rc:%d",
                         tmpInfo._host.c_str(), port, rc ) ;

            rc = tmpSocket.connect() ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Connect to %s:%d failed, rc:%d",
                       tmpInfo._host.c_str(), port, rc ) ;
               tmpInfo._result = rc ;
               continue ;
            }

            rc = tmpSocket.send( (const CHAR *)msg, msg->messageLength,
                                 sentLen, millisec ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Send messge to %s:%d failed, rc:%d",
                       tmpInfo._host.c_str(), port, rc ) ;
               tmpInfo._result = rc ;
               continue ;
            }

            rc = tmpSocket.recv( (CHAR*)&msgLength, sizeof(INT32), receivedLen,
                                 millisec ) ;
            PD_RC_CHECK( rc, PDERROR, "Recieve msg length failed, rc: %d",
                         rc ) ;

            if ( msgLength < sizeof(INT32) || msgLength > SDB_MAX_MSG_LENGTH )
            {
               PD_LOG ( PDERROR, "Recieve msg length[%d] error", msgLength ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            buff = (CHAR*)SDB_OSS_MALLOC( msgLength + 1 ) ;
            if ( !buff )
            {
               rc = SDB_OOM ;
               PD_LOG ( PDERROR, "Failed to allocate memory for %d bytes",
                        msgLength + 1 ) ;
               goto error ;
            }
            *(INT32*)buff = msgLength ;
            rc = tmpSocket.recv( &buff[sizeof(INT32)], msgLength-sizeof(INT32),
                                 receivedLen,
                                 millisec ) ;
            if ( rc )
            {
               SDB_OSS_FREE( buff ) ;
               PD_LOG ( PDERROR, "Recieve response message failed, rc: %d", rc ) ;
               goto error ;
            }
            *ppRecvMsg = (MsgHeader*)buff ;
            break ;
         }
      }

      if ( rc && !hasUpdateGroup )
      {
         goto update_group ;
      }

   done:
      if ( CATALOG_GROUPID == groupID )
      {
         ossScopedLock lock ( &_shardLatch, SHARED ) ;
         _clsUpdateNodeStatus( &_cataGrpItem, hosts ) ;
      }
      else
      {
         clsGroupItem *item = NULL ;
         rc = getAndLockGroupItem( groupID, &item, FALSE, CLS_SHARD_TIMEOUT,
                                   NULL ) ;
         if ( SDB_OK == rc )
         {
            _clsUpdateNodeStatus( item, hosts ) ;
            unlockGroupItem( item ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCSND, rc );
      return rc ;
   error:
      goto done ;
   update_group:
      if ( !hasUpdateGroup )
      {
         hasUpdateGroup = TRUE ;
         if ( CATALOG_GROUPID == groupID )
         {
            rc = updateCatGroup( millisec ) ;
         }
         else
         {
            rc = syncUpdateGroupInfo( groupID, millisec ) ;
         }

         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         PD_LOG( PDERROR, "Update group[%d] info failed, rc: %d",
                 groupID, rc ) ;
      }
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SND2CAT, "_clsShardMgr::sendToCatlog" )
   INT32 _clsShardMgr::sendToCatlog ( MsgHeader *msg,
                                      NET_HANDLE *pHandle,
                                      INT64 upCataMillsec,
                                      BOOLEAN canUpCataGrp )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SND2CAT ) ;

      MsgRouteID nodeID ;
      INT32 status = 0 ;
      UINT32 tmpPos = CLS_RG_NODE_POS_INVALID ;
      BOOLEAN hasLock = FALSE ;
      BOOLEAN hasUpdateGrp = FALSE ;

   retry:
      _shardLatch.get_shared() ;
      hasLock = TRUE ;

      if ( !_pNetRtAgent || 0 == _cataGrpItem.nodeCount() )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Either network runtime agent does not exist, "
                  "or catalog list is empty, rc = %d", rc ) ;
         goto error ;
      }

      tmpPos = _cataGrpItem.getPrimaryPos() ;
      if ( SDB_OK == _cataGrpItem.getNodeID( tmpPos, nodeID,
                                             MSG_ROUTE_CAT_SERVICE ) &&
           SDB_OK == _cataGrpItem.getNodeInfo( tmpPos, status ) &&
           NET_NODE_STAT_NORMAL == status )
      {
         rc = _pNetRtAgent->syncSend ( nodeID, (void*)msg, pHandle ) ;
         if ( rc != SDB_OK )
         {
            string hostName ;
            string svcName ;
            _cataGrpItem.getNodeInfo( nodeID, hostName, svcName ) ;
            PD_LOG ( PDWARNING, "Send message to primary catalog[%s:%s, "
                     "NodeID:%u] failed[rc:%d]", hostName.c_str(),
                     svcName.c_str(), nodeID.columns.nodeID, rc ) ;
            _cataGrpItem.updateNodeStat( nodeID.columns.nodeID,
                                         netResult2Status( rc ) ) ;
         }
         else
         {
            goto done ;
         }
      }

      {
         tmpPos = ossRand() % _cataGrpItem.nodeCount() ;
         rc = SDB_CLS_NODE_NOT_EXIST ;

         for ( UINT32 times = 0 ; times < _cataGrpItem.nodeCount() ; ++times )
         {
            if ( SDB_OK == _cataGrpItem.getNodeID( tmpPos, nodeID,
                                                   MSG_ROUTE_CAT_SERVICE ) &&
                 SDB_OK == _cataGrpItem.getNodeInfo( tmpPos, status ) &&
                 NET_NODE_STAT_NORMAL == status )
            {
               rc = _pNetRtAgent->syncSend ( nodeID, (void*)msg, pHandle ) ;
               if ( SDB_OK == rc )
               {
                  goto done ;
               }
               else
               {
                  string hostName ;
                  string svcName ;
                  _cataGrpItem.getNodeInfo( nodeID, hostName, svcName ) ;
                  PD_LOG ( PDWARNING, "Send message to catlog[%s:%s, "
                           "NodeID: %u] failed[rc:%d]", hostName.c_str(),
                           svcName.c_str(), nodeID.columns.nodeID, rc ) ;
                  _cataGrpItem.updateNodeStat( nodeID.columns.nodeID,
                                               netResult2Status( rc ) ) ;
               }
            }
            tmpPos = ( tmpPos + 1 ) % _cataGrpItem.nodeCount() ;
         } /// end for
      }

      _shardLatch.release_shared() ;
      hasLock = FALSE ;
      if ( canUpCataGrp && !hasUpdateGrp )
      {
         hasUpdateGrp = TRUE ;
         rc = updateCatGroup( upCataMillsec ) ;
         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         PD_LOG( PDERROR, "Update catalog group info failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( hasLock )
      {
         _shardLatch.release_shared() ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SND2CAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPDCATGRP, "_clsShardMgr::updateCatGroup" )
   INT32 _clsShardMgr::updateCatGroup ( INT64 millsec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPDCATGRP ) ;
      INT32 rc = SDB_OK ;
      UINT32 times = 0 ;

      _shardLatch.get_shared() ;
      _cataGrpItem.clearNodesStat() ;

      _shardLatch.release_shared() ;

      MsgCatCatGroupReq req ;
      req.header.opCode = MSG_CAT_CATGRP_REQ ;
      req.id.value = 0 ;
      req.id.columns.groupID = CATALOG_GROUPID ;

      if ( millsec > 0 )
      {
         req.header.requestID = (UINT64)ossGetCurrentThreadID() ;
         _upCatEvent.reset() ;
      }

   retry:
      ++times ;
      rc = sendToCatlog( ( MsgHeader*)&req, &_upCatHandle, 0, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to sync send to catalog, rc = %d", rc ) ;
         goto error ;
      }

      if ( millsec > 0 )
      {
         INT32 result = 0 ;
         rc = _upCatEvent.wait( millsec, &result ) ;
         if ( SDB_OK == rc )
         {
            rc = result ;
         }
         _upCatHandle = NET_INVALID_HANDLE ;

         if ( rc )
         {
            if ( SDB_NETWORK_CLOSE == rc &&
                 times < CLS_CATA_RETRY_MAX_TIMES )
            {
               goto retry ;
            }
            PD_LOG( PDERROR, "Update catalog group info failed, rc: %d", rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPDCATGRP, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_CLRALLDATA, "_clsShardMgr::clearAllData" )
   INT32 _clsShardMgr::clearAllData ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_CLRALLDATA );
      _SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      std::set<_monCollectionSpace> csList ;

      PD_LOG ( PDEVENT, "Clear all dms data" ) ;

      dmsCB->dumpInfo( csList, TRUE ) ;
      std::set<_monCollectionSpace>::const_iterator it = csList.begin() ;
      while ( it != csList.end() )
      {
         const _monCollectionSpace &cs = *it ;
         rc = rtnDropCollectionSpaceCommand ( cs._name, NULL, dmsCB, NULL,
                                              TRUE ) ;
         if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG ( PDERROR, "Clear collectionspace[%s] failed[rc:%d]",
               cs._name, rc ) ;
            break ;
         }
         PD_LOG ( PDDEBUG, "Clear collectionspace[%s] succeed", cs._name ) ;
         ++it ;
      }

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_CLRALLDATA, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCUPDCAT, "_clsShardMgr::syncUpdateCatalog" )
   INT32 _clsShardMgr::syncUpdateCatalog ( const CHAR *pCollectionName,
                                           INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCUPDCAT );
      BOOLEAN send = FALSE ;
      clsEventItem *pEventInfo = NULL ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

      if ( !pCollectionName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "collection name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      _catLatch.get() ;
      pEventInfo = _findCatSyncEvent( pCollectionName, TRUE ) ;
      if ( !pEventInfo )
      {
         _catLatch.release () ;
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate memory for event info, "
                  "rc = %d", rc ) ;
         goto error ;
      }

      if ( FALSE == pEventInfo->send )
      {
         rc = _sendCatalogReq ( pCollectionName, 0, &(pEventInfo->netHandle),
                                millsec ) ;
         if ( SDB_OK == rc )
         {
            pEventInfo->send = TRUE ;
            send = TRUE ;
            pEventInfo->waitNum++ ;
            pEventInfo->requestID = _requestID ;
            pEventInfo->event.reset () ;
         }
      }
      else
      {
         pEventInfo->waitNum++ ;
      }
      _catLatch.release () ;

      if ( SDB_OK == rc )
      {
         INT32 result = SDB_OK ;
         rc = pEventInfo->event.wait ( millsec, &result ) ;
         if ( SDB_OK == rc )
         {
            rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
         }

         if ( SDB_NET_CANNOT_CONNECT == rc )
         {
            PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                    "other nodes not aware, sleep %d seconds",
                    NET_NODE_FAULTUP_MIN_TIME ) ;
            ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_NETWORK_CLOSE == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = result > 0 ? TRUE : FALSE ;
         }
         else if ( rc && SDB_DMS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR, "Update catalog[%s] failed, rc: %d",
                    pCollectionName, rc ) ;
         }

         _catLatch.get () ;
         pEventInfo->waitNum-- ;

         if ( send )
         {
            pEventInfo->send = FALSE ;
         }

         if ( 0 == pEventInfo->waitNum )
         {
            pEventInfo->event.reset () ;

            SDB_OSS_DEL pEventInfo ;
            pEventInfo = NULL ;
            _mapSyncCatEvent.erase ( pCollectionName ) ;
         }

         _catLatch.release () ;
      }

      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( millsec ) )
         {
            goto retry ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCUPDCAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCUPDGPINFO, "_clsShardMgr::syncUpdateGroupInfo" )
   INT32 _clsShardMgr::syncUpdateGroupInfo ( UINT32 groupID, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCUPDGPINFO );
      BOOLEAN send = FALSE ;
      clsEventItem *pEventInfo = NULL ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      _catLatch.get() ;

      pEventInfo = _findNMSyncEvent( groupID, TRUE ) ;
      if ( !pEventInfo )
      {
         _catLatch.release () ;
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate event info for group %d, "
                  "rc = %d", groupID, rc ) ;
         goto error ;
      }

      if ( FALSE == pEventInfo->send )
      {
         rc = _sendGroupReq( groupID, 0, &(pEventInfo->netHandle),
                             millsec ) ;
         if ( SDB_OK == rc )
         {
            pEventInfo->send = TRUE ;
            send = TRUE ;
            pEventInfo->waitNum++ ;
            pEventInfo->requestID = _requestID ;
            pEventInfo->event.reset () ;
         }
      }
      else
      {
         pEventInfo->waitNum++ ;
      }

      _catLatch.release () ;

      if ( SDB_OK == rc )
      {
         INT32 result = SDB_OK ;
         rc = pEventInfo->event.wait ( millsec, &result ) ;

         if ( SDB_OK == rc )
         {
            rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
         }

         if ( SDB_NET_CANNOT_CONNECT == rc )
         {
            PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                    "other nodes not aware, sleep %d seconds",
                    NET_NODE_FAULTUP_MIN_TIME ) ;
            ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_NETWORK_CLOSE == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = result > 0 ? TRUE : FALSE ;
         }
         else if ( rc && SDB_CLS_GRP_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "Update group info[%d] failed, rc: %d",
                    groupID, rc ) ;
         }

         _catLatch.get () ;
         pEventInfo->waitNum-- ;

         if ( send )
         {
            pEventInfo->send = FALSE ;
         }

         if ( 0 == pEventInfo->waitNum )
         {
            pEventInfo->event.reset () ;

            SDB_OSS_DEL pEventInfo ;
            pEventInfo = NULL ;
            _mapSyncNMEvent.erase ( groupID ) ;
         }

         _catLatch.release () ;
      }

      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( millsec ) )
         {
            goto retry ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCUPDGPINFO,  rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPDPRM, "_clsShardMgr::updatePrimary" )
   INT32 _clsShardMgr::updatePrimary ( const NodeID & id, BOOLEAN primary )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPDPRM ) ;
      INT32 rc = SDB_OK ;
      NodeID tmpID ;
      tmpID.value = id.value ;
      tmpID.columns.groupID = CATALOG_GROUPID ;

      _shardLatch.get_shared() ;
      rc = _cataGrpItem.updatePrimary( tmpID, primary ) ;
      _shardLatch.release_shared() ;

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPDPRM, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPPRM_BYREPLY, "_clsShardMgr::updatePrimaryByReply" )
   INT32 _clsShardMgr::updatePrimaryByReply( MsgHeader *pMsg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPPRM_BYREPLY ) ;
      INT32 rc = MSG_GET_INNER_REPLY_RC( pMsg ) ;
      UINT32 startFrom = MSG_GET_INNER_REPLY_STARTFROM( pMsg ) ;

      if ( IS_REPLY_TYPE( pMsg->opCode ) &&
           SDB_CLS_NOT_PRIMARY == rc &&
           0 != startFrom )
      {
         INT32 preStat = NET_NODE_STAT_NORMAL ;
         NodeID primaryNode ;
         primaryNode.columns.nodeID = startFrom ;
         primaryNode.columns.groupID = CATALOG_GROUPID ;
         primaryNode.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;

         _shardLatch.get_shared() ;
         rc = _cataGrpItem.updatePrimary( primaryNode, TRUE, &preStat ) ;
         if ( NET_NODE_STAT_NORMAL != preStat )
         {
            _cataGrpItem.cancelPrimary() ;
            rc = SDB_NET_CANNOT_CONNECT ;
         }
         _shardLatch.release_shared() ;

         if ( NET_NODE_STAT_NORMAL != preStat )
         {
            PD_LOG( PDWARNING, "Catalog group primary node[%d] is crashed",
                    startFrom ) ;
         }
         else if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Update catalog group primary node to [%d] "
                    "by reply message", startFrom ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPPRM_BYREPLY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SNDGPREQ, "_clsShardMgr::_sendGroupReq" )
   INT32 _clsShardMgr::_sendGroupReq ( UINT32 groupID, UINT64 requestID,
                                       NET_HANDLE *pHandle, INT64 millsec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SNDGPREQ );
      _MsgCatGroupReq msg ;
      msg.header.opCode = MSG_CAT_NODEGRP_REQ ;

      if ( 0 == requestID )
      {
         requestID = ++_requestID ;
      }
      msg.header.requestID = requestID ;
      msg.id.columns.groupID = groupID ;

      INT32 rc = sendToCatlog( (MsgHeader *)&msg, pHandle, millsec, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send to catalog, rc = %d", rc ) ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "send group req[id: %d, requestID: %lld, rc: %d]",
               groupID, _requestID, rc ) ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SNDGPREQ, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDCATAQUERYREQ, "_clsShardMgr::_sendCataQueryReq" )
   INT32 _clsShardMgr::_sendCataQueryReq( INT32 queryType,
                                          const BSONObj &query,
                                          UINT64 requestID,
                                          NET_HANDLE *pHandle,
                                          INT64 millsec )
   {
      INT32 rc        = SDB_OK ;
      CHAR *pBuffer   = NULL ;
      INT32 buffSize  = 0 ;
      MsgHeader * msg = NULL ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SENDCATAQUERYREQ ) ;

      if ( 0 == requestID )
      {
         requestID = ++_requestID ;
      }

      rc = msgBuildQueryMsg ( &pBuffer, &buffSize, "CAT", 0, requestID, 0,
                              -1, &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to build query msg, rc = %d", rc ) ;
         goto error ;
      }

      msg = (MsgHeader *) pBuffer ;
      msg->opCode = queryType ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;
      rc = sendToCatlog ( msg, pHandle, millsec, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDDEBUG, "Failed to send message to catalog, rc = %d", rc ) ;
         goto error ;
      }

   done:
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
         pBuffer = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SENDCATAQUERYREQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SNDCATREQ, "_clsShardMgr::_sendCatalogReq" )
   INT32 _clsShardMgr::_sendCatalogReq ( const CHAR *pCollectionName,
                                         UINT64 requestID,
                                         NET_HANDLE *pHandle,
                                         INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SNDCATREQ );
      BSONObj query ;
      if ( !pCollectionName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "collection name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

      try
      {
         query = BSON ( CAT_COLLECTION_NAME << pCollectionName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception when creating query: %s",
                  e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sendCataQueryReq( MSG_CAT_QUERY_CATALOG_REQ, query, requestID,
                              pHandle, millsec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDDEBUG, "send catelog req[name: %s, requestID: %lld, "
                  "rc: %d]", pCollectionName, requestID, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SNDCATREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDCSINFOREQ, "_clsShardMgr::_sendCSInfoReq" )
   INT32 _clsShardMgr::_sendCSInfoReq( const CHAR * pCSName, UINT64 requestID,
                                       NET_HANDLE *pHandle, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SENDCSINFOREQ ) ;
      if ( !pCSName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "cs name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

      try
      {
         query = BSON ( CAT_COLLECTION_SPACE_NAME << pCSName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception when creating query: %s",
                  e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sendCataQueryReq( MSG_CAT_QUERY_SPACEINFO_REQ, query, requestID,
                              pHandle, millsec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDDEBUG, "send collection space req[name: %s, requestID: "
                  "%lld, rc: %d]", pCSName, requestID, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SENDCSINFOREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATGPRES, "_clsShardMgr::_onCatCatGroupRes" )
   INT32 _clsShardMgr::_onCatCatGroupRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATGPRES );
      UINT32 version = 0 ;
      MsgRouteID primaryNode ;
      UINT32 primary = 0 ;
      std::string groupName ;
      map<UINT64, _netRouteNode> mapNodes ;
      MsgCatCatGroupRes *res = (MsgCatCatGroupRes*)msg ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(msg) )
      {
         PD_LOG ( PDERROR, "Update catalog group info failed[rc: %d]",
                  MSG_GET_INNER_REPLY_RC(msg) ) ;
         rc = MSG_GET_INNER_REPLY_RC(msg) ;
         goto error ;
      }

      rc = msgParseCatGroupRes ( res, version, groupName, mapNodes, &primary ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Parse MsgCatCatGroupRes failed[rc: %d]", rc ) ;
         goto error ;
      }
      primaryNode.columns.groupID = CATALOG_GROUPID ;

      if ( 0 == version || version != _catVerion )
      {
         _shardLatch.get () ;

         pmdOptionsCB *optCB = pmdGetOptionCB() ;
         string oldCfg, newCfg ;
         optCB->toString( oldCfg ) ;
         MAP_ROUTE_NODE oldCatNodes = _mapNodes ;
         NodeID oldID ;

         _catVerion = version ;
         _mapNodes.clear() ;
         optCB->clearCatAddr() ;
         map<UINT64, _netRouteNode>::iterator it = mapNodes.begin() ;
         while ( it != mapNodes.end() )
         {
            clsNodeItem &nodeItem = it->second ;
            setCatlogInfo( nodeItem._id,
                           nodeItem._host,
                           nodeItem._service[MSG_ROUTE_CAT_SERVICE] ) ;
            optCB->setCatAddr( nodeItem._host,
                               nodeItem._service[
                               MSG_ROUTE_CAT_SERVICE].c_str() ) ;
            ++it ;
         }

         _cataGrpItem.updateNodes( mapNodes ) ;

         it = _mapNodes.begin() ;
         while ( it != _mapNodes.end() )
         {
            clsNodeItem &nodeItem = it->second ;
            if ( SDB_OK == _findCatNodeID ( oldCatNodes, nodeItem._host,
                                            nodeItem._service[
                                            MSG_ROUTE_CAT_SERVICE],
                                            oldID ) )
            {
               if ( oldID.value != nodeItem._id.value )
               {
                  _pNetRtAgent->updateRoute ( oldID, nodeItem._id ) ;
                  PD_LOG ( PDDEBUG, "Update catalog node[%u:%u] to [%u:%u]",
                           oldID.columns.groupID, oldID.columns.nodeID,
                           nodeItem._id.columns.groupID,
                           nodeItem._id.columns.nodeID ) ;
               }
            }
            else
            {
               _pNetRtAgent->updateRoute ( nodeItem._id,
                                           nodeItem._host,
                                           nodeItem._service[
                                           MSG_ROUTE_CAT_SERVICE].c_str() ) ;
               PD_LOG ( PDDEBUG, "Update catalog node[%u:%u] to %s:%s",
                        nodeItem._id.columns.groupID,
                        nodeItem._id.columns.nodeID,
                        nodeItem._host,
                        nodeItem._service[MSG_ROUTE_CAT_SERVICE].c_str() ) ;
            }
            ++it ;
         }
         optCB->toString( newCfg ) ;
         if ( oldCfg != newCfg )
         {
            optCB->reflush2File() ;
         }

         _shardLatch.release () ;
      }

      if ( CATALOG_GROUPID == nodeID().columns.groupID )
      {
         replCB *pRepl = sdbGetReplCB() ;
         primary = pRepl->getPrimary().columns.nodeID ;
      }

      if ( primary != 0 )
      {
         primaryNode.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         primaryNode.columns.nodeID = primary ;
         rc = updatePrimary ( primaryNode, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to update primary, rc = %d", rc ) ;
            goto error ;
         }
         else
         {
            PD_LOG( PDEVENT, "Update catalog group primary node to [%d] by "
                    "group info", primary ) ;
         }
      }

   done:
      _upCatEvent.signalAll( rc ) ;
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__ONCATGPRES, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATGRPRES, "_clsShardMgr::_onCatGroupRes" )
   INT32 _clsShardMgr::_onCatGroupRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATGRPRES );
      clsEventItem *pEventInfo = NULL ;

      PD_LOG ( PDDEBUG, "Recieve catalog group res[requestID:%lld,flag:%d]",
               msg->requestID, MSG_GET_INNER_REPLY_RC(msg) ) ;

      ossScopedLock lock ( &_catLatch ) ;

      pEventInfo = _findNMSyncEvent( msg->requestID ) ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(msg) )
      {
         rc = MSG_GET_INNER_REPLY_RC(msg) ;
         if ( pEventInfo )
         {
            if ( SDB_CLS_GRP_NOT_EXIST == rc ||
                 SDB_DMS_EOC == rc )
            {
               _pNodeMgrAgent->lock_w() ;
               _pNodeMgrAgent->clearGroup( pEventInfo->groupID ) ;
               _pNodeMgrAgent->release_w() ;
               pEventInfo->event.signalAll( SDB_CLS_GRP_NOT_EXIST ) ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               INT32 rcTmp = updatePrimaryByReply( msg ) ;
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  pEventInfo->event.signalAll ( rcTmp ) ;
               }
               else if ( SDB_OK == rcTmp )
               {
                  pEventInfo->event.signalAll ( 1 ) ;
               }
               else
               {
                  pEventInfo->event.signalAll ( rc ) ;
               }
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG ( PDERROR, "Update group[%d] failed[rc:%d]",
                        pEventInfo->groupID, rc ) ;
               pEventInfo->event.signalAll( rc ) ;
            }
         }
      }
      else
      {
         _pNodeMgrAgent->lock_w() ;
         const CHAR* objdata = MSG_GET_INNER_REPLY_DATA(msg) ;
         UINT32 length = msg->messageLength -
                         MSG_GET_INNER_REPLY_HEADER_LEN(msg) ;
         UINT32 groupID = 0 ;

         rc = _pNodeMgrAgent->updateGroupInfo( objdata, length, &groupID ) ;
         PD_LOG ( ( SDB_OK == rc ? PDEVENT : PDERROR ),
                  "Update group[groupID:%u, rc: %d]", groupID, rc ) ;

         clsGroupItem* groupItem = NULL ;
         if ( SDB_OK == rc )
         {
            groupItem = _pNodeMgrAgent->groupItem( groupID ) ;
            if ( !pEventInfo )
            {
               pEventInfo = _findNMSyncEvent( groupID, FALSE ) ;
            }
         }
         if ( groupItem )
         {
            UINT32 indexPos = 0 ;
            MsgRouteID nodeID ;
            std::string hostName ;
            std::string service ;

            while ( SDB_OK == groupItem->getNodeInfo( indexPos, nodeID,
                                                      hostName, service,
                                                    MSG_ROUTE_SHARD_SERVCIE ) )
            {
               _pNetRtAgent->updateRoute( nodeID, hostName.c_str(),
                                          service.c_str() ) ;
               ++indexPos ;
            }
         }
         _pNodeMgrAgent->release_w() ;

         if ( pEventInfo )
         {
            pEventInfo->event.signalAll( rc ) ;
         }
      }

      PD_TRACE_EXITRC (SDB__CLSSHDMGR__ONCATGRPRES, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATREQMSG, "_clsShardMgr::_onCatalogReqMsg" )
   INT32 _clsShardMgr::_onCatalogReqMsg ( NET_HANDLE handle, MsgHeader* msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATREQMSG );
      MsgCatQueryCatRsp *res   = ( MsgCatQueryCatRsp*)msg ;
      PD_LOG ( PDDEBUG, "Recieve catalog response[requestID: %lld, flag: %d]",
               msg->requestID, res->flags ) ;

      INT32 flag               = 0 ;
      INT64 contextID          = -1 ;
      INT32 startFrom          = 0 ;
      INT32 numReturned        = 0 ;
      vector < BSONObj > objList ;
      UINT32 groupID           = nodeID().columns.groupID ;
      INT32 rc                 = SDB_OK ;
      clsEventItem *pEventInfo = NULL ;

      ossScopedLock lock ( &_catLatch ) ;

      if ( SDB_OK != res->flags )
      {
         rc = SDB_CLS_UPDATE_CAT_FAILED ;

         pEventInfo = _findCatSyncEvent ( msg->requestID ) ;
         if ( pEventInfo )
         {
            if ( SDB_DMS_EOC == res->flags ||
                 SDB_DMS_NOTEXIST == res->flags )
            {
               _pCatAgent->lock_w () ;
               rc = _pCatAgent->clear ( pEventInfo->name.c_str() ) ;
               _pCatAgent->release_w () ;
               pEventInfo->event.signalAll ( SDB_DMS_NOTEXIST ) ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == res->flags )
            {
               INT32 rcTmp = updatePrimaryByReply( msg ) ;
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  pEventInfo->event.signalAll ( rcTmp ) ;
               }
               else if ( SDB_OK == rcTmp )
               {
                  pEventInfo->event.signalAll ( 1 ) ;
               }
               else
               {
                  pEventInfo->event.signalAll ( rc ) ;
               }
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG ( PDERROR, "Update catalog[%s] failed[response: %d]",
                        pEventInfo->name.c_str(), res->flags ) ;
               pEventInfo->event.signalAll ( res->flags ) ;
            }
         }
      }
      else
      {
         _clsCatalogSet *catSet = NULL ;
         INT32 version = 0 ;
         UINT32 groupCount = 0 ;
         const CHAR *pCLType = "normal" ;
         string collectionName ;
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to extract reply msg, rc = %d", rc ) ;
            goto error ;
         }

         SDB_ASSERT ( numReturned == 1 && objList.size() == 1,
                      "Collection catalog item num must be 1" ) ;

         _pCatAgent->lock_w () ;
         rc = _pCatAgent->updateCatalog ( 0, groupID, objList[0].objdata(),
                                          objList[0].objsize(), &catSet ) ;
         if ( catSet )
         {
            version = catSet->getVersion() ;
            groupCount = catSet->groupCount() ;
            collectionName = catSet->name() ;
            if ( catSet->isMainCL() )
            {
               pCLType = "main" ;
            }
            else if ( !catSet->getMainCLName().empty() )
            {
               pCLType = "sub" ;
            }
         }
         _pCatAgent->release_w () ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update catalog:%d", rc ) ;
            goto error ;
         }

         PD_LOG ( PDEVENT, "Update catalog[name: %s, version:%u, type: %s, "
                  "group count: %d, rc: %d]", collectionName.c_str(),
                  version, pCLType, groupCount, rc ) ;

         BSONElement ele = objList[0].getField ( CAT_COLLECTION_NAME ) ;
         clsEventItem *pEventInfo = _findCatSyncEvent( ele.str().c_str(),
                                                       FALSE ) ;
         if ( pEventInfo )
         {
            pEventInfo->event.signalAll ( rc ) ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__ONCATREQMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATSYNCEV, "_clsShardMgr::_findCatSyncEvent" )
   clsEventItem *_clsShardMgr::_findCatSyncEvent ( const CHAR * pCollectionName,
                                                   BOOLEAN bCreate )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATSYNCEV );
      SDB_ASSERT ( pCollectionName , "Collection name can't be NULL" ) ;

      clsEventItem *pEventInfo = NULL ;
      MAP_CAT_EVENT_IT it = _mapSyncCatEvent.find ( pCollectionName ) ;
      if ( it != _mapSyncCatEvent.end() )
      {
         pEventInfo = it->second ;
         goto done ;
      }

      if ( !bCreate )
      {
         goto done ;
      }

      pEventInfo = SDB_OSS_NEW _clsEventItem ;
      pEventInfo->name = pCollectionName ;
      _mapSyncCatEvent[pCollectionName] = pEventInfo ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDCATSYNCEV );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATSYNCEVN, "_clsShardMgr::_findCatSyncEvent" )
   clsEventItem *_clsShardMgr::_findCatSyncEvent ( UINT64 requestID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATSYNCEVN );
      clsEventItem *pEventInfo = NULL ;
      MAP_CAT_EVENT_IT it = _mapSyncCatEvent.begin() ;
      while ( it != _mapSyncCatEvent.end() )
      {
         pEventInfo = it->second ;
         if ( pEventInfo->requestID == requestID )
         {
            goto done ;
         }
         ++it ;
      }
      pEventInfo = NULL ;
   done :
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDCATSYNCEVN );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDNMSYNCEV, "_clsShardMgr::_findNMSyncEvent" )
   clsEventItem* _clsShardMgr::_findNMSyncEvent( UINT32 groupID,
                                                 BOOLEAN bCreate )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDNMSYNCEV );
      clsEventItem *pEventInfo = NULL ;
      MAP_NM_EVENT_IT it = _mapSyncNMEvent.find ( groupID ) ;
      if ( it != _mapSyncNMEvent.end() )
      {
         pEventInfo = it->second ;
         goto done ;
      }

      if ( !bCreate )
      {
         goto done ;
      }

      pEventInfo = SDB_OSS_NEW _clsEventItem ;
      pEventInfo->groupID = groupID ;
      _mapSyncNMEvent[groupID] = pEventInfo ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDNMSYNCEV );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDNMSYNCEVN, "_clsShardMgr::_findNMSyncEvent" )
   clsEventItem* _clsShardMgr::_findNMSyncEvent ( UINT64 requestID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDNMSYNCEVN );
      clsEventItem *pEventInfo = NULL ;
      MAP_NM_EVENT_IT it = _mapSyncNMEvent.begin() ;
      while ( it != _mapSyncNMEvent.end() )
      {
         pEventInfo = it->second ;
         if ( pEventInfo->requestID == requestID )
         {
            goto done ;
         }
         ++it ;
      }
      pEventInfo = NULL ;
   done :
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDNMSYNCEVN );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATNODEID, "_clsShardMgr::_findCatNodeID" )
   INT32 _clsShardMgr::_findCatNodeID ( MAP_ROUTE_NODE &catNodes,
                                        const CHAR *hostName,
                                        const std::string & service,
                                        NodeID & id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATNODEID );
      MAP_ROUTE_NODE_IT it = catNodes.begin() ;
      while ( it != catNodes.end() )
      {
         const clsNodeItem &nodeItem = it->second ;
         if ( 0 == ossStrcmp( nodeItem._host, hostName ) &&
              nodeItem._service[ MSG_ROUTE_CAT_SERVICE ] == service )
         {
            id.value = nodeItem._id.value ;
            goto done ;
         }
         ++it ;
      }
      rc = SDB_CLS_NODE_NOT_EXIST ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__FNDCATNODEID, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_GETANDLOCKCATSET, "_clsShardMgr::getAndLockCataSet" )
   INT32 _clsShardMgr::getAndLockCataSet( const CHAR * name,
                                          clsCatalogSet **ppSet,
                                          BOOLEAN noWithUpdate,
                                          INT64 waitMillSec,
                                          BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_GETANDLOCKCATSET );
      SDB_ASSERT ( ppSet && name,
                   "ppSet and name can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pCatAgent->lock_r() ;
         *ppSet = _pCatAgent->collectionSet( name ) ;
         if ( !(*ppSet) && noWithUpdate )
         {
            _pCatAgent->release_r() ;
            rc = syncUpdateCatalog( name, waitMillSec ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to sync update catalog, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            noWithUpdate = FALSE ;
            continue ;
         }
         if ( !(*ppSet) )
         {
            _pCatAgent->release_r() ;
            rc = SDB_CLS_NO_CATALOG_INFO ;
         }
         break ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_GETANDLOCKCATSET, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsShardMgr::unlockCataSet( clsCatalogSet * catSet )
   {
      if ( catSet )
      {
         _pCatAgent->release_r() ;
      }
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_GETNLCKGPITEM, "_clsShardMgr::getAndLockGroupItem" )
   INT32 _clsShardMgr::getAndLockGroupItem( UINT32 id, clsGroupItem **ppItem,
                                            BOOLEAN noWithUpdate,
                                            INT64 waitMillSec,
                                            BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_GETNLCKGPITEM );
      SDB_ASSERT ( ppItem, "ppItem can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pNodeMgrAgent->lock_r() ;
         *ppItem = _pNodeMgrAgent->groupItem( id ) ;
         if ( !(*ppItem) && noWithUpdate )
         {
            _pNodeMgrAgent->release_r() ;
            rc = syncUpdateGroupInfo( id, waitMillSec ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to sync update group info, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            noWithUpdate = FALSE ;
            continue ;
         }
         if ( !(*ppItem) )
         {
            _pNodeMgrAgent->release_r() ;
            rc = SDB_CLS_NO_GROUP_INFO ;
         }
         break ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_GETNLCKGPITEM, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsShardMgr::unlockGroupItem( clsGroupItem * item )
   {
      if ( item )
      {
         _pNodeMgrAgent->release_r() ;
      }
      return SDB_OK ;
   }

   INT32 _clsShardMgr::rGetCSInfo( const CHAR * csName,
                                   UINT32 &pageSize,
                                   UINT32 &lobPageSize,
                                   DMS_STORAGE_TYPE &type,
                                   INT64 waitMillSec )
   {
      INT32 rc = SDB_OK ;
      clsCSEventItem *item = NULL ;
      UINT64 requestID = 0 ;
      INT32 result = 0 ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

      SDB_ASSERT ( csName, "collection space name can't be NULL" ) ;
      if ( !csName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      item = SDB_OSS_NEW clsCSEventItem() ;
      if ( NULL == item )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory failed" ) ;
         goto error ;
      }
      item->csName = csName ;

      _catLatch.get() ;
      requestID = ++_requestID ;
      _mapSyncCSEvent[ requestID ] = item ;
      _catLatch.release() ;

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      rc = _sendCSInfoReq( csName, requestID, &(item->netHandle),
                           waitMillSec ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send cs info request, rc = %d", rc ) ;
         goto error ;
      }

      result = SDB_OK ;
      rc = item->event.wait( waitMillSec, &result ) ;
      if ( SDB_OK == rc )
      {
         rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
      }

      if ( SDB_NET_CANNOT_CONNECT == rc )
      {
         PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                 "other nodes not aware, sleep %d seconds",
                 NET_NODE_FAULTUP_MIN_TIME ) ;
         ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
         needRetry = TRUE ;
         hasUpCataGrp = FALSE ;
      }
      else if ( SDB_NETWORK_CLOSE == rc )
      {
         needRetry = TRUE ;
         hasUpCataGrp = FALSE ;
      }
      else if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         needRetry = TRUE ;
         hasUpCataGrp = result > 0 ? TRUE : FALSE ;
      }

      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( waitMillSec ) )
         {
            goto retry ;
         }
      }

      PD_RC_CHECK( rc, PDWARNING, "Get collection space[%s] info failed, "
                   "rc: %d", csName, rc ) ;

      pageSize = item->pageSize ;
      lobPageSize = item->lobPageSize ;
      type = item->type ;

   done:
      _catLatch.get() ;
      _mapSyncCSEvent.erase( requestID ) ;
      _catLatch.release() ;

      if ( item )
      {
         SDB_OSS_DEL item ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::updateDCBaseInfo()
   {
      INT32 rc = SDB_OK ;
      rtnQueryOptions queryOpt ;
      CHAR *pBuff = NULL ;
      INT32 bufSize = 0 ;
      MsgHeader *pRecvMsg = NULL ;
      MsgOpReply *pReply = NULL ;
      const UINT32 maxRetryTimes = 3 ;
      UINT32 retryTimes = 0 ;

      queryOpt.setCLFullName( CAT_SYSDCBASE_COLLECTION_NAME ) ;
      queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;
      queryOpt.setQuery( BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ) ;

      rc = queryOpt.toQueryMsg( &pBuff, bufSize, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build query message failed, rc: %d", rc ) ;
         goto error ;
      }

      while( retryTimes++ < maxRetryTimes )
      {
         rc = syncSend( ( MsgHeader* )pBuff, CATALOG_GROUPID, TRUE,
                        &pRecvMsg ) ;
         if ( rc )
         {
            rc = syncSend( ( MsgHeader* )pBuff, CATALOG_GROUPID, FALSE,
                           &pRecvMsg ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         pReply = ( MsgOpReply* )pRecvMsg ;
         rc = pReply->flags ;
         SDB_ASSERT( pReply->contextID == -1, "Context id must be -1" ) ;

         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            INT32 rcTmp = SDB_OK ;
            rcTmp = updatePrimaryByReply( pRecvMsg ) ;

            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               PD_LOG( PDWARNING, "Catalog group primary node is crashed "
                       "but other nodes not aware, sleep %d seconds",
                       NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }

            if ( rcTmp )
            {
               updateCatGroup( OSS_ONE_SEC ) ;
            }

            SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
            pRecvMsg = NULL ;
            continue ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Update datacenter base info failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         else if ( 1 == pReply->numReturned &&
                   pReply->header.messageLength >
                   (INT32)sizeof( MsgOpReply ) + 5 )
         {
            clsDCBaseInfo *pInfo = _pDCMgr->getDCBaseInfo() ;
            try
            {
               BSONObj obj( ( CHAR* )pRecvMsg + sizeof( MsgOpReply ) ) ;
               rc = _pDCMgr->updateDCBaseInfo( obj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Update datacenter base info failed, "
                          "rc: %d", rc ) ;
                  goto error ;
               }
               pmdGetKRCB()->setDBReadonly( pInfo->isReadonly() ) ;
               pmdGetKRCB()->setDBDeactivated( !pInfo->isActivated() ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "No data center info" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         break ;
      }

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      if ( pRecvMsg )
      {
         SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::_onQueryCSInfoRsp( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      clsCSEventItem *csItem = NULL ;
      MAP_CS_EVENT_IT it ;
      BSONElement ele ;

      MsgOpReply *res = ( MsgOpReply* )msg ;

      PD_LOG ( PDDEBUG, "Recieve collecton space query response[requestID: "
               "%lld, flag: %d]", msg->requestID, res->flags ) ;

      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector < BSONObj > objList ;

      ossScopedLock lock ( &_catLatch ) ;

      it = _mapSyncCSEvent.find( msg->requestID ) ;
      if ( it == _mapSyncCSEvent.end() )
      {
         goto done ;
      }

      csItem = it->second ;

      if ( SDB_OK != res->flags )
      {
         rc = res->flags ;

         if ( SDB_CLS_NOT_PRIMARY == res->flags )
         {
            INT32 rcTmp = updatePrimaryByReply( msg ) ;
            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               csItem->event.signalAll ( rcTmp ) ;
            }
            else if ( SDB_OK == rcTmp )
            {
               csItem->event.signalAll ( 1 ) ;
            }
            else
            {
               csItem->event.signalAll ( rc ) ;
            }
            rc = SDB_OK ;
            goto done ;
         }
         else
         {
            PD_LOG ( PDERROR, "Query collection space[%s] info failed, rc: %d",
                     csItem->csName.c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         SDB_ASSERT ( numReturned == 1 && objList.size() == 1,
                      "Collection space item num must be 1" ) ;

         ele = objList[0].getField ( CAT_PAGE_SIZE_NAME ) ;
         if ( ele.isNumber() )
         {
            csItem->pageSize = (UINT32)ele.numberInt() ;
         }
         else
         {
            csItem->pageSize = DMS_PAGE_SIZE_DFT ;
         }

         ele = objList[0].getField( CAT_LOB_PAGE_SZ_NAME ) ;
         if ( ele.isNumber() )
         {
            csItem->lobPageSize = (UINT32)ele.numberInt() ;
         }
         else
         {
            csItem->lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         }

         ele = objList[0].getField( CAT_TYPE_NAME ) ;
         if ( ele.isNumber() &&
              (INT32)DMS_STORAGE_CAPPED == ele.numberInt() )
         {
            csItem->type = DMS_STORAGE_CAPPED ;
         }
         else
         {
            csItem->type = DMS_STORAGE_NORMAL ;
         }
      }

      csItem->event.signalAll( rc ) ;

   done:
      return rc ;
   error:
      if ( csItem )
      {
         csItem->event.signalAll( rc ) ;
      }
      goto done ;
   }

   INT32 _clsShardMgr::_onHandleClose( NET_HANDLE handle, MsgHeader *msg )
   {
      ossScopedLock lock( &_catLatch ) ;

      MAP_CAT_EVENT::iterator itCat = _mapSyncCatEvent.begin() ;
      while( itCat != _mapSyncCatEvent.end() )
      {
         clsEventItem *pItem = itCat->second ;
         ++itCat ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      MAP_NM_EVENT::iterator itNode = _mapSyncNMEvent.begin() ;
      while( itNode != _mapSyncNMEvent.end() )
      {
         clsEventItem *pItem = itNode->second ;
         ++itNode ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      MAP_CS_EVENT::iterator itCS = _mapSyncCSEvent.begin() ;
      while( itCS != _mapSyncCSEvent.end() )
      {
         clsCSEventItem *pItem = itCS->second ;
         ++itCS ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      if ( handle == _upCatHandle )
      {
         _upCatEvent.signalAll( SDB_NETWORK_CLOSE ) ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONAUTHREQMSG, "_clsShardMgr::_onAuthReqMsg" )
   INT32 _clsShardMgr::_onAuthReqMsg( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__ONAUTHREQMSG ) ;
      BSONObj bodyObj ;
      BSONElement ele ;
      const CHAR *peerHost = NULL ;
      const CHAR *peerSvc = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;
      BSONObj myInfoObj ;
      BSONObjBuilder builder ;
      MsgAuthReply *reply = NULL ;
      INT32 replySize = 0 ;
      UINT32 tmpPos = CLS_RG_NODE_POS_INVALID ;
      MsgRouteID cataRouteID ;
      string cataHost ;
      string cataSvc ;
      MSG_ROUTE_SERVICE_TYPE svcType = MSG_ROUTE_CAT_SERVICE ;
      CHAR groupName[ OSS_MAX_GROUPNAME_SIZE + 1 ] = { 0 } ;

      SDB_ASSERT( messenger, "Remote messenger should not be NULL" ) ;
      rc = extractAuthMsg( msg, bodyObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract auth message failed[ %d ]", rc ) ;

      try
      {
         BSONElement ele ;
         peerHost = bodyObj.getStringField( FIELD_NAME_HOST ) ;
         peerSvc = bodyObj.getStringField( FIELD_NAME_SERVICE_NAME ) ;

         ele = bodyObj.getField( FIELD_NAME_GROUPID ) ;
         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Group id field type error, type[ %d ]",
                    ele.type() ) ;
            goto error ;
         }
         _seAdptID.columns.groupID = ele.numberLong() ;

         ele = bodyObj.getField( FIELD_NAME_NODEID ) ;
         if ( NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Node id field type error, type[ %d ]",
                    ele.type() ) ;
            goto error ;
         }
         _seAdptID.columns.nodeID = ele.numberInt() ;

         ele = bodyObj.getField( FIELD_NAME_SERVICE_TYPE ) ;
         if ( NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Service type field type error, type[ %d ]",
                    ele.type() ) ;
            goto error ;
         }
         _seAdptID.columns.serviceID = ele.numberInt() ;

         if ( MSG_INVALID_ROUTEID == _seAdptID.value )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Route id is invalid" ) ;
            goto error ;
         }

         rc = messenger->setTarget( _seAdptID, peerHost, peerSvc ) ;
         PD_RC_CHECK( rc, PDERROR, "Add remote target failed[ %d ]", rc ) ;

         rc = messenger->setLocalID( _nodeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Set local id failed[ %d ]", rc ) ;

         rc = _pNetRtAgent->updateRoute( _seAdptID, peerHost, peerSvc ) ;
         if ( rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
         {
            PD_LOG( PDERROR, "Update route failed[ %d ], host[ %s ], "
                    "service[ %s ]", rc, peerHost, peerSvc ) ;
            goto error ;
         }

         builder.appendBool( FIELD_NAME_IS_PRIMARY, pmdIsPrimary() ) ;
         pmdGetKRCB()->getGroupName( groupName, OSS_MAX_GROUPNAME_SIZE + 1 ) ;
         builder.append( FIELD_NAME_GROUPNAME, groupName ) ;

         tmpPos = _cataGrpItem.getPrimaryPos() ;
         rc = _cataGrpItem.getNodeInfo( tmpPos, cataRouteID, cataHost,
                                        cataSvc, svcType ) ;
         PD_RC_CHECK( rc, PDERROR, "Get catalog node info failed[ %d ]", rc ) ;
         {
            BSONObjBuilder subBuilder(
               builder.subobjStart( FIELD_NAME_CATALOGINFO ) ) ;
            subBuilder.append( FIELD_NAME_HOST, cataHost ) ;
            subBuilder.append( FIELD_NAME_SERVICE_NAME, cataSvc ) ;
            subBuilder.append( FIELD_NAME_GROUPID, cataRouteID.columns.groupID ) ;
            subBuilder.append( FIELD_NAME_NODEID, cataRouteID.columns.nodeID ) ;
            subBuilder.append( FIELD_NAME_SERVICE, cataRouteID.columns.serviceID ) ;
            subBuilder.done() ;
         }

         myInfoObj = builder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      replySize = sizeof( MsgAuthReply ) ;
      if ( SDB_OK == rc && !myInfoObj.isEmpty())
      {
         replySize += ossRoundUpToMultipleX( myInfoObj.objsize(), 4 ) ;
      }
      reply = ( MsgAuthReply * )SDB_OSS_MALLOC( replySize ) ;
      if ( !reply )
      {
         PD_LOG( PDERROR, "Allocate memory for reply message failed, size: %d",
                 replySize ) ;
         rc = SDB_OOM ;
      }
      else
      {
         reply->header.messageLength = replySize ;
         reply->header.opCode = MSG_AUTH_VERIFY_RES ;
         reply->header.TID = msg->TID ;
         reply->header.routeID.value = 0 ;
         reply->header.requestID = msg->requestID ;
         reply->flags = rc ;
         reply->startFrom = 0 ;
         reply->numReturned = rc ? -1 : 1 ;
         if ( SDB_OK == rc )
         {
            ossMemcpy( (CHAR *)reply + sizeof( MsgAuthReply ),
                       myInfoObj.objdata(), myInfoObj.objsize() ) ;
         }

         rc = _sendToSeAdpt( handle, (MsgHeader *)reply ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send message to search engine adapter "
                    "failed[ %d ]", rc ) ;
         }
      }

      if ( reply )
      {
         SDB_OSS_FREE( reply ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__ONAUTHREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG, "_clsShardMgr::_onTextIdxInfoReqMsg" )
   INT32 _clsShardMgr::_onTextIdxInfoReqMsg( NET_HANDLE handle,
                                             MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG ) ;
      CHAR *body = NULL ;
      INT64 peerVersion = -1 ;
      BSONObj textIdxInfo ;
      MsgOpReply *reply = NULL ;
      INT32 replySize = 0 ;
      INT64 localVersion = -1 ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      body = (CHAR *)msg + sizeof( MsgHeader ) ;
      try
      {
         BSONObj bodyObj( body ) ;
         BSONElement verEle = bodyObj.getField( FIELD_NAME_VERSION ) ;
         if ( verEle.eoo() )
         {
            PD_LOG( PDERROR, "Text index version dose not exist" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( NumberLong != verEle.type() )
         {
            PD_LOG( PDERROR, "Text index version type is wrong" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            peerVersion = verEle.numberLong() ;
         }
      }
      catch (std::exception &e)
      {
         PD_LOG( PDERROR, "unexpected err:%s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      localVersion = rtnCB->getTextIdxVersion() ;
      if ( RTN_INIT_TEXT_INDEX_VERSION == localVersion )
      {
         if ( FALSE == rtnCB->updateTextIdxVersion( RTN_INIT_TEXT_INDEX_VERSION,
                                                    peerVersion + 1 ) )
         {
            PD_LOG( PDERROR, "Update text index version failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         localVersion = rtnCB->getTextIdxVersion() ;
      }

      rc = _dumpTextIdxInfo( localVersion, textIdxInfo,
                             (localVersion == peerVersion) ) ;
      PD_RC_CHECK( rc, PDERROR, "Dump text indices information failed[ %d ]",
                   rc ) ;

   done:
      replySize = sizeof( MsgOpReply ) ;
      if ( SDB_OK == rc && !textIdxInfo.isEmpty() )
      {
         replySize += textIdxInfo.objsize() ;
      }
      reply = (MsgOpReply *)SDB_OSS_MALLOC( replySize ) ;
      if ( !reply )
      {
         PD_LOG( PDERROR, "Allocate memory for reply message failed, size: %d",
                 replySize ) ;
         rc = SDB_OOM ;
      }
      else
      {
         reply->header.messageLength = sizeof( MsgOpReply )
                                       + textIdxInfo.objsize()  ;
         reply->header.opCode = MSG_SEADPT_UPDATE_IDXINFO_RES ;
         reply->header.TID = msg->TID ;
         reply->header.routeID.value = 0 ;
         reply->header.requestID = msg->requestID ;
         reply->flags = SDB_OK ;
         reply->startFrom = 0 ;
         reply->numReturned = rc ? -1 : 1 ;
         if ( SDB_OK == rc )
         {
            ossMemcpy( (CHAR *)reply + sizeof( MsgOpReply ),
                       textIdxInfo.objdata(), textIdxInfo.objsize() ) ;
         }

         rc = _sendToSeAdpt( handle, (MsgHeader *)reply ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send message to search engine adapter "
                    "failed[ %d ]", rc ) ;
         }
      }
      if ( reply )
      {
         SDB_OSS_FREE( reply ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__BUILDTEXTIDXOBJ, "_clsShardMgr::_buildTextIdxObj" )
   INT32 _clsShardMgr::_buildTextIdxObj( const monCSSimple *csInfo,
                                         const monCLSimple *clInfo,
                                         const monIndex *idxInfo,
                                         BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__BUILDTEXTIDXOBJ ) ;

      CHAR cappedCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      rtnExtDataProcessor::getExtDataNames( csInfo->_name, clInfo->_clname,
                                            idxInfo->getIndexName(), NULL, 0,
                                            cappedCLName,
                                            DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;

      try
      {
         builder.append( FIELD_NAME_COLLECTION, clInfo->_name ) ;
         builder.append( CLS_NAME_CAPPED_COLLECTION, cappedCLName ) ;
         builder.appendObject( FIELD_NAME_INDEX,
                               idxInfo->_indexDef.objdata(),
                               idxInfo->_indexDef.objsize() ) ;

         BSONArrayBuilder lidObjs( builder.subarrayStart( FIELD_NAME_LOGICAL_ID ) ) ;
         lidObjs.append( csInfo->_logicalID ) ;
         lidObjs.append( clInfo->_logicalID ) ;
         lidObjs.append( idxInfo->_indexLID ) ;
         lidObjs.done() ;

         builder.done() ;

#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Text index info: %s",
                 builder.done().toString().c_str() ) ;
#endif /* _DEBUG */
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__BUILDTEXTIDXOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
     Dump all text indices information, together with the node role and index
     text version.
     The text indices information will only be dumped when the node is primary.
     So the adapter connectiong to slavery node will get none text index
     information, and no indexing job will be started.

    The result object is as follows:

    {
       "IsPrimary" : TRUE | FALSE,
       "Version" : version_number,
       "Indexes" : [
          {
             "Collection" : collection_full_name,
             "CappedCL"   : capped_collection_full_name,
             "Index"      :
                {
                   index_definition
                },
             "LogicalID"  : [
                cl_logical_id, index_logical_id
             ]
          },
          ...
          {
             "Collection" : collection_full_name,
             "CappedCL"   : capped_collection_full_name,
             "Index"      :
                {
                   index_definition
                },
             "LogicalID"  : [
                cl_logical_id, index_logical_id
             ]
          }
       ]
    }
   *
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__DUMPTEXTIDXINFO, "_clsShardMgr::_dumpTextIdxInfo" )
   INT32 _clsShardMgr::_dumpTextIdxInfo( INT64 localVersion, BSONObj &obj,
                                         BOOLEAN onlyVersion )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__DUMPTEXTIDXINFO ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      MON_CS_SIM_LIST csList ;
      MON_CS_SIM_LIST::iterator csItr ;
      BSONObj infoObj ;
      BSONObjBuilder builder ;
      BOOLEAN isPrimary = pmdIsPrimary() ;

      try
      {
         builder.appendBool( FIELD_NAME_IS_PRIMARY, isPrimary ) ;
         builder.append( FIELD_NAME_VERSION, localVersion ) ;

         if ( !onlyVersion )
         {
            BSONArrayBuilder indexObjs( builder.subarrayStart( FIELD_NAME_INDEXES ) ) ;

            dmsCB->dumpInfo( csList, FALSE, TRUE, TRUE ) ;
            for ( csItr = csList.begin(); csItr != csList.end(); ++csItr )
            {
               for ( MON_CL_SIM_VEC::const_iterator clItr = csItr->_clList.begin();
                     clItr != csItr->_clList.end(); ++clItr )
               {
                  for ( MON_IDX_LIST::const_iterator idxItr = clItr->_idxList.begin();
                        idxItr != clItr->_idxList.end(); ++idxItr )
                  {
                     INT32 rcTmp = SDB_OK ;
                     UINT16 idxType = IXM_EXTENT_TYPE_NONE ;
                     rcTmp = idxItr->getIndexType( idxType ) ;
                     if ( rcTmp )
                     {
                        PD_LOG( PDERROR, "Get type of index failed[ %d ], cs[ %s ],"
                                " cl[ %s ], index[ %s ]", rcTmp, csItr->_name,
                                clItr->_name, idxItr->getIndexName() ) ;
                        continue ;
                     }
                     if ( IXM_INDEX_FLAG_NORMAL == idxItr->_indexFlag &&
                          IXM_EXTENT_TYPE_TEXT == idxType )
                     {
                        BSONObjBuilder subBuilder( indexObjs.subobjStart() ) ;
                        rc = _buildTextIdxObj( &*csItr, &*clItr, &*idxItr,
                                               subBuilder ) ;
                        if ( rc )
                        {
                           PD_LOG( PDERROR, "Build text index object failed[ %d ]",
                                   rc ) ;
                        }
                     }
                  }
               }
            }
            indexObjs.done() ;
         }

         obj = builder.obj() ;
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "All text index info: %s",
                 obj.toString().c_str() ) ;
#endif /* _DEBUG */
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__DUMPTEXTIDXINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDTOSEADPT, "_clsShardMgr::_sendToSeAdpt" )
   INT32 _clsShardMgr::_sendToSeAdpt( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__SENDTOSEADPT ) ;
      BOOLEAN hasLock = FALSE ;

      _shardLatch.get_shared() ;
      hasLock = TRUE ;

      rc = _pNetRtAgent->syncSend( handle, (void *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to search engine adapter "
                   "failed[ %d ]", rc ) ;

   done:
      if ( hasLock )
      {
         _shardLatch.release_shared() ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__SENDTOSEADPT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT64 _clsShardMgr::netIn()
   {
      return _pNetRtAgent->netIn() ;
   }

   INT64 _clsShardMgr::netOut()
   {
      return _pNetRtAgent->netOut() ;
   }

}

