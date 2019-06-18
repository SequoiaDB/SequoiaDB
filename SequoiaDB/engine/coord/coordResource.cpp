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

   Source File Name = coordResource.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordResource.hpp"
#include "pmdEDU.hpp"
#include "msgCatalog.hpp"
#include "msgMessageFormat.hpp"
#include "msgMessage.hpp"
#include "coordRemoteHandle.hpp"
#include "coordRemoteSession.hpp"
#include "coordCommon.hpp"
#include "coordFactory.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "coordOmProxy.hpp"
#include "../bson/bson.h"
#include "utilArray.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_SOCKET_OPR_DFT_TIME         ( 5000 )
   #define COORD_SOCKET_FORCE_TIMEOUT        ( 600000 )

   typedef _utilArray< UINT64, CLS_REPLSET_MAX_NODE_SIZE >     NODE_ARRAY ;

   /*
      cmpNodeInfo define and implement
   */
   struct cmpNodeInfo
   {
      bool operator()( const clsNodeItem &left,
                       const clsNodeItem &right )
      {
         INT32 comp = ossStrcmp( left._host, right._host ) ;
         if ( 0 == comp )
         {
            comp = ossStrcmp( left._service[MSG_ROUTE_CAT_SERVICE].c_str(),
                              right._service[MSG_ROUTE_CAT_SERVICE].c_str() ) ;
         }
         return comp < 0 ? true : false ;
      }
   } ;

   /*
      _coordResource implement
   */
   _coordResource::_coordResource()
   {
      _upGrpIndentify = 1 ;
      _cataAddrChanged = FALSE ;
      _pAgent = NULL ;
      _pOptionsCB = NULL ;
      _pOmProxy = NULL ;
      _pOmStrategyAgent = NULL ;
   }

   _coordResource::~_coordResource()
   {
   }

   INT32 _coordResource::init( _netRouteAgent *pAgent,
                               pmdOptionsCB *pOptionsCB )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfo *pCataGroup = NULL ;
      coordOmProxy *pOmProxy = NULL ;

      if ( !pAgent || !pOptionsCB )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Agent or option is NULL" ) ;
         goto error ;
      }
      _pAgent = pAgent ;
      _pOptionsCB = pOptionsCB ;

      pCataGroup = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
      if ( !pCataGroup )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate catalog group info failed" ) ;
         goto error ;
      }
      _cataGroupInfo = CoordGroupInfoPtr( pCataGroup ) ;
      pCataGroup = NULL ;
      _emptyGroupPtr = _cataGroupInfo ;
      _omGroupInfo = _emptyGroupPtr ;

      _initAddressFromPair( _pOptionsCB->catAddrs(),
                            MSG_ROUTE_CAT_SERVICE,
                            _cataNodeAddrList ) ;

      _initAddressFromPair( _pOptionsCB->omAddrs(),
                            MSG_ROUTE_OM_SERVICE,
                            _omNodeAddrList ) ;

      rc = _buildOmGroupInfo() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build OM group info failed, rc: %d", rc ) ;
         goto error ;
      }

      pOmProxy = SDB_OSS_NEW coordOmProxy() ;
      if ( !pOmProxy )
      {
         PD_LOG( PDERROR, "Allocate om proxy failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pOmProxy = pOmProxy ;

      rc = pOmProxy->init( this ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om proxy failed, rc: %d", rc ) ;
         goto error ;
      }

      _pOmStrategyAgent = SDB_OSS_NEW coordOmStrategyAgent() ;
      if ( !_pOmStrategyAgent )
      {
         PD_LOG( PDERROR, "Allocate om strategy agent failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = _pOmStrategyAgent->init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om strategy agent failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( pCataGroup )
      {
         SDB_OSS_DEL pCataGroup ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _coordResource::fini()
   {
      _mapCataInfo.clear() ;
      _mapGroupInfo.clear() ;
      _mapGroupName.clear() ;

      if ( _pOmStrategyAgent )
      {
         _pOmStrategyAgent->fini() ;
         SDB_OSS_DEL _pOmStrategyAgent ;
         _pOmStrategyAgent = NULL ;
      }

      if ( _pOmProxy )
      {
         SDB_OSS_DEL _pOmProxy ;
         _pOmProxy = NULL ;
      }
   }

   _netRouteAgent* _coordResource::getRouteAgent()
   {
      return _pAgent ;
   }

   _IOmProxy* _coordResource::getOmProxy()
   {
      return _pOmProxy ;
   }

   _coordOmStrategyAgent* _coordResource::getOmStrategyAgent()
   {
      return _pOmStrategyAgent ;
   }

   INT32 _coordResource::_buildOmGroupInfo()
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfo *pGroupInfo = NULL ;

      if ( !_omNodeAddrList.empty() )
      {
         try
         {
            BSONObjBuilder builder( 1024 ) ;
            BSONObj objOMGroup ;

            CoordVecNodeInfo::iterator it ;

            builder.append( CAT_GROUPID_NAME, OM_GROUPID ) ;
            builder.append( CAT_GROUPNAME_NAME, OM_GROUPNAME ) ;
            builder.append( CAT_ROLE_NAME, SDB_ROLE_OM ) ;
            builder.append( FIELD_NAME_SECRETID, (INT32)0 ) ;
            builder.append( CAT_VERSION_NAME, (INT32)1 ) ;
            builder.append( CAT_PRIMARY_NAME, (INT32)OM_NODE_ID_BEGIN ) ;
            BSONArrayBuilder arrGroup( builder.subarrayStart( CAT_GROUP_NAME ) ) ;

            for ( it = _omNodeAddrList.begin() ;
                  it != _omNodeAddrList.end() ;
                  ++it )
            {
               clsNodeItem &nodeItem = *it ;

               BSONObjBuilder node( arrGroup.subobjStart() ) ;
               node.append( CAT_NODEID_NAME, (INT32)OM_NODE_ID_BEGIN ) ;
               node.append( CAT_HOST_FIELD_NAME, nodeItem._host ) ;
               node.append( CAT_STATUS_NAME, (INT32)SDB_CAT_GRP_ACTIVE ) ;
               BSONArrayBuilder arrSvc( node.subarrayStart(
                                        CAT_SERVICE_FIELD_NAME ) ) ;
               BSONObjBuilder svc( arrSvc.subobjStart() ) ;
               svc.append( CAT_SERVICE_TYPE_FIELD_NAME,
                           (INT32)MSG_ROUTE_OM_SERVICE ) ;
               svc.append( CAT_SERVICE_NAME_FIELD_NAME,
                           nodeItem._service[ MSG_ROUTE_OM_SERVICE ] ) ;

               svc.done() ;
               arrSvc.done() ;
               node.done() ;
            }
            arrGroup.done() ;

            objOMGroup = builder.obj() ;

            pGroupInfo = SDB_OSS_NEW CoordGroupInfo( OM_GROUPID ) ;
            if ( NULL == pGroupInfo )
            {
               rc = SDB_OOM ;
               PD_LOG ( PDERROR, "Alloc group info failed" ) ;
               goto error ;
            }

            rc = pGroupInfo->updateGroupItem( objOMGroup ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update group info from bson[%s] failed, "
                       "rc: %d", objOMGroup.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }

         _omGroupInfo = CoordGroupInfoPtr( pGroupInfo ) ;
         pGroupInfo = NULL ;

         rc = _updateRouteInfo( _omGroupInfo, MSG_ROUTE_OM_SERVICE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update om group's om serivce route info "
                    "failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( pGroupInfo )
      {
         SDB_OSS_DEL pGroupInfo ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _coordResource::_initAddressFromPair( const vector< pmdAddrPair > &vecAddrPair,
                                              INT32 serviceType,
                                              CoordVecNodeInfo &vecAddr )
   {
      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      for ( UINT32 i = 0 ; i < vecAddrPair.size() ; ++i )
      {
         if ( 0 == vecAddrPair[i]._host[ 0 ] )
         {
            break ;
         }
         _addAddrNode( routeID, vecAddrPair[i]._host,
                       vecAddrPair[i]._service,
                       serviceType,
                       vecAddr ) ;
      }
      std::sort( vecAddr.begin(), vecAddr.end(), cmpNodeInfo() ) ;
   }

   void _coordResource::_addAddrNode( const MsgRouteID &id,
                                      const CHAR *pHostName,
                                      const CHAR *pSvcName,
                                      INT32 serviceType,
                                      CoordVecNodeInfo &vecAddr )
   {
      CoordNodeInfo nodeInfo ;
      nodeInfo._id.value = id.value ;
      ossStrncpy( nodeInfo._host, pHostName, OSS_MAX_HOSTNAME );
      nodeInfo._host[OSS_MAX_HOSTNAME] = 0 ;
      nodeInfo._service[ serviceType ] = pSvcName ;

      vecAddr.push_back( nodeInfo ) ;
   }

   void _coordResource::clearCataNodeAddrList()
   {
      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      if ( !_cataNodeAddrList.empty() )
      {
         _cataAddrChanged = TRUE ;
         _cataNodeAddrList.clear() ;
      }
      _cataGroupInfo = _emptyGroupPtr ;

      _mapGroupInfo.erase( CATALOG_GROUPID ) ;
      _clearGroupName( CATALOG_GROUPID ) ;
   }

   BOOLEAN _coordResource::addCataNodeAddrWhenEmpty( const CHAR *pHostName,
                                                     const CHAR *pSvcName )
   {
      BOOLEAN add = FALSE ;
      MsgRouteID routeID ;

      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      if ( !_cataNodeAddrList.empty() )
      {
         goto done ;
      }

      routeID.value = MSG_INVALID_ROUTEID ;
      _addAddrNode( routeID, pHostName, pSvcName,
                    MSG_ROUTE_CAT_SERVICE,
                    _cataNodeAddrList ) ;

      _cataAddrChanged = TRUE ;
      add = TRUE ;

   done:
      return add ;
   }

   void _coordResource::setCataGroupInfo( CoordGroupInfoPtr &groupPtr )
   {
      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      if ( _cataGroupInfo->groupVersion() != groupPtr->groupVersion() )
      {
         _cataNodeAddrList.clear() ;
         _cataAddrChanged = TRUE ;

         UINT32 pos = 0 ;
         MsgRouteID id ;
         string hostName ;
         string svcName ;
         while ( SDB_OK == groupPtr->getNodeInfo( pos, id, hostName, svcName,
                                                  MSG_ROUTE_CAT_SERVICE ) )
         {
            _addAddrNode( id, hostName.c_str(), svcName.c_str(),
                          MSG_ROUTE_CAT_SERVICE,
                          _cataNodeAddrList ) ;
            ++pos ;
         }
      }
      _cataGroupInfo = groupPtr ;
   }

   INT32 _coordResource::syncAddress2Options( BOOLEAN flush, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      CoordVecNodeInfo vecNodes ;
      BOOLEAN bSame = FALSE ;

      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      if ( !force && !_cataAddrChanged )
      {
         goto done ;
      }

      _cataAddrChanged = FALSE ;
      _initAddressFromPair( _pOptionsCB->catAddrs(),
                            MSG_ROUTE_CAT_SERVICE,
                            vecNodes ) ;
      bSame = coordIsCataAddrSame( vecNodes, _cataNodeAddrList ) ;

      if ( !force && bSame )
      {
         goto done ;
      }

      _pOptionsCB->clearCatAddr() ;
      for ( UINT32 i = 0 ; i < _cataNodeAddrList.size() ; ++i )
      {
         const clsNodeItem &item = _cataNodeAddrList[i] ;
         _pOptionsCB->setCatAddr( item._host,
                                  item._service[MSG_ROUTE_CAT_SERVICE].c_str() ) ;
      }

      if ( flush )
      {
         rc = _pOptionsCB->reflush2File() ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::getGroupInfo( UINT32 groupID,
                                       CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;

      if ( OM_GROUPID == groupID )
      {
         groupPtr = getOmGroupInfo() ;
      }
      else
      {
         MAP_GROUP_INFO_IT it  ;
         ossScopedLock lock( &_nodeMutex, SHARED ) ;

         it = _mapGroupInfo.find( groupID ) ;
         if ( it != _mapGroupInfo.end() )
         {
            groupPtr = it->second ;
         }
         else
         {
            rc = SDB_COOR_NO_NODEGROUP_INFO ;
         }
      }

      return rc ;
   }

   INT32 _coordResource::getGroupInfo( const CHAR *groupName,
                                       CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_COOR_NO_NODEGROUP_INFO ;

      if ( 0 == ossStrcmp( OM_GROUPNAME, groupName ) )
      {
         groupPtr = getOmGroupInfo() ;
         rc = SDB_OK ;
      }
      else
      {
         MAP_GROUP_INFO_IT it  ;
         MAP_GROUP_NAME_IT itName ;

         ossScopedLock lock( &_nodeMutex, SHARED ) ;

         itName = _mapGroupName.find( groupName ) ;
         if ( itName != _mapGroupName.end() )
         {
            it = _mapGroupInfo.find( itName->second ) ;
            if ( it != _mapGroupInfo.end() )
            {
               groupPtr = it->second ;
               rc = SDB_OK ;
            }
         }
      }

      return rc ;
   }

   UINT32 _coordResource::getGroupsInfo( GROUP_VEC &vecGroupPtr,
                                         BOOLEAN exceptCata,
                                         BOOLEAN exceptCoord )
   {
      UINT32 count = 0 ;
      MAP_GROUP_INFO_IT it ;
      ossScopedLock lock( &_nodeMutex, SHARED ) ;

      it = _mapGroupInfo.begin() ;
      while( it != _mapGroupInfo.end() )
      {
         if ( CATALOG_GROUPID == it->first && exceptCata )
         {
            ++it ;
            continue ;
         }
         else if ( COORD_GROUPID == it->first && exceptCoord )
         {
            ++it ;
            continue ;
         }
         vecGroupPtr.push_back( it->second ) ;
         ++count ;
         ++it ;
      }

      return count ;
   }

   UINT32 _coordResource::getGroupList( CoordGroupList &groupList,
                                        BOOLEAN exceptCata,
                                        BOOLEAN exceptCoord )
   {
      UINT32 count = 0 ;
      MAP_GROUP_INFO_IT it ;
      ossScopedLock lock( &_nodeMutex, SHARED ) ;

      it = _mapGroupInfo.begin() ;
      while( it != _mapGroupInfo.end() )
      {
         if ( CATALOG_GROUPID == it->first && exceptCata )
         {
            ++it ;
            continue ;
         }
         else if ( COORD_GROUPID == it->first && exceptCoord )
         {
            ++it ;
            continue ;
         }
         groupList[ it->first ] = it->first ;
         ++count ;
         ++it ;
      }

      return count ;
   }

   INT32 _coordResource::updateGroupInfo( UINT32 groupID,
                                          CoordGroupInfoPtr &groupPtr,
                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( CATALOG_GROUPID == groupID )
      {
         rc = updateCataGroupInfo( groupPtr, cb ) ;
      }
      else if ( OM_GROUPID == groupID )
      {
         goto done ;
      }
      else
      {
         MsgCatGroupReq msgGroupReq ;
         msgGroupReq.id.columns.groupID = groupID ;
         msgGroupReq.id.columns.nodeID = 0 ;
         msgGroupReq.id.columns.serviceID = 0;
         msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq ) ;
         msgGroupReq.header.opCode = MSG_CAT_GRP_REQ ;
         msgGroupReq.header.routeID.value = 0 ;

         rc = _updateGroupInfo( ( MsgHeader* )&msgGroupReq, cb,
                                MSG_ROUTE_CAT_SERVICE, groupPtr ) ;
         if ( SDB_CLS_GRP_NOT_EXIST == rc )
         {
            removeGroupInfo( groupID ) ;
         }
      }
      if ( rc )
      {
         if ( SDB_CAT_NO_ADDR_LIST != rc )
         {
            PD_LOG( PDERROR, "Update group[%u] info failed, rc: %d",
                    groupID, rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::updateGroupInfo( const CHAR *groupName,
                                          CoordGroupInfoPtr &groupPtr,
                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuf = NULL ;

      if ( 0 == ossStrcmp( groupName, CATALOG_GROUPNAME ) )
      {
         rc = updateCataGroupInfo( groupPtr, cb ) ;
      }
      else if ( 0 == ossStrcmp( groupName, OM_GROUPNAME ) )
      {
         goto done ;
      }
      else
      {
         UINT32 nameLen = ossStrlen( groupName ) + 1 ;
         UINT32 msgLen = nameLen +  sizeof(MsgCatGroupReq) ;
         MsgCatGroupReq *msg = NULL ;

         rc = cb->allocBuff( msgLen, &pBuf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc memory[Size:%u] failed, rc: %d",
                    msgLen, rc ) ;
            goto error ;
         }
         msg = ( MsgCatGroupReq * )pBuf ;
         msg->id.value = 0 ;
         msg->header.messageLength = msgLen ;
         msg->header.opCode = MSG_CAT_GRP_REQ ;
         msg->header.routeID.value = 0 ;
         ossMemcpy( pBuf + sizeof(MsgCatGroupReq),
                    groupName, nameLen ) ;

         rc = _updateGroupInfo( ( MsgHeader*)msg, cb,
                                MSG_ROUTE_CAT_SERVICE, groupPtr ) ;
         if ( SDB_CLS_GRP_NOT_EXIST == rc )
         {
            removeGroupInfo( groupName ) ;
         }
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Update group[%s] info failed, rc: %d",
                 groupName, rc ) ;
         goto error ;
      }

   done:
      if ( pBuf )
      {
         cb->releaseBuff( pBuf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::updateGroupsInfo( GROUP_VEC &vecGroupPtr,
                                           _pmdEDUCB *cb,
                                           const BSONObj *pCondObj,
                                           BOOLEAN exceptCata,
                                           BOOLEAN exceptCoord )
   {
      INT32 rc = SDB_OK ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;

      INT64 contextID = -1 ;
      rtnContextBuf buf ;
      CHAR *pBuffer = NULL ;
      INT32 buffSize = 0 ;

      UINT64 identify = 0 ;
      GROUP_VEC vecGroupPtrTmp ;

      if ( !pCondObj || pCondObj->isEmpty() )
      {
         _nodeMutex.get() ;
         identify = ++_upGrpIndentify ;
         _nodeMutex.release() ;
      }

      rc = pFactory->create( CMD_NAME_LIST_GROUPS, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_LIST_GROUPS, rc ) ;
         goto error ;
      }
      rc = pOperator->init( this, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = msgBuildQueryMsg( &pBuffer, &buffSize,
                             CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPS,
                             0, 0, 0, -1,
                             pCondObj, NULL, NULL, NULL,
                             cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Builder query message failed, rc: %d", rc ) ;
         goto error ;
      }
      rc = pOperator->execute( ( MsgHeader* )pBuffer, cb, contextID, &buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = _processGroupContextReply( contextID, vecGroupPtrTmp, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Process groups result failed, rc: %d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < vecGroupPtrTmp.size() ; ++i )
      {
         CoordGroupInfoPtr &groupPtr = vecGroupPtrTmp[ i ] ;

         if ( CATALOG_GROUPID == groupPtr->groupID() )
         {
            if ( !exceptCata )
            {
               vecGroupPtr.push_back( groupPtr ) ;
            }
         }
         else if ( COORD_GROUPID == groupPtr->groupID() )
         {
            if ( !exceptCoord )
            {
               vecGroupPtr.push_back( groupPtr ) ;
            }
         }
         else
         {
            vecGroupPtr.push_back( groupPtr ) ;
         }

         _updateRouteInfo( groupPtr, MSG_ROUTE_SHARD_SERVCIE ) ;
         addGroupInfo( groupPtr ) ;

         if ( CATALOG_GROUPID == groupPtr->groupID() )
         {
            _updateRouteInfo( groupPtr, MSG_ROUTE_CAT_SERVICE ) ;
            setCataGroupInfo( groupPtr ) ;
            syncAddress2Options( TRUE, FALSE) ;
         }
      }

      if ( identify > 0 )
      {
         invalidateGroupInfo( identify ) ;
      }

   done:
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      if ( pBuffer )
      {
         msgReleaseBuffer( pBuffer, cb ) ;
      }
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::updateGroupList( CoordGroupList &groupList,
                                          _pmdEDUCB *cb,
                                          const BSONObj *pCondObj,
                                          BOOLEAN exceptCata,
                                          BOOLEAN exceptCoord,
                                          BOOLEAN useLocalWhenFailed )
   {
      INT32 rc = SDB_OK ;
      GROUP_VEC vecGroupPtr ;

      rc = updateGroupsInfo( vecGroupPtr, cb, pCondObj,
                             exceptCata, exceptCoord ) ;
      if ( rc )
      {
         if ( useLocalWhenFailed )
         {
            getGroupList( groupList, exceptCata, exceptCoord ) ;
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }
      }
      else
      {
         GROUP_VEC::iterator it = vecGroupPtr.begin() ;
         while ( it != vecGroupPtr.end() )
         {
            CoordGroupInfoPtr &ptr = *it ;
            groupList[ ptr->groupID() ] = ptr->groupID() ;
            ++it ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordResource::removeGroupInfo( UINT32 groupID )
   {
      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      _mapGroupInfo.erase( groupID ) ;
      _clearGroupName( groupID ) ;
   }

   void _coordResource::removeGroupInfo( const CHAR *groupName )
   {
      MAP_GROUP_NAME_IT itName ;
      ossScopedLock lock( &_nodeMutex, EXCLUSIVE ) ;

      itName = _mapGroupName.find( groupName ) ;
      if ( itName != _mapGroupName.end() )
      {
         _mapGroupInfo.erase( itName->second ) ;
         _mapGroupName.erase( itName ) ;
      }
   }

   void _coordResource::addGroupInfo( CoordGroupInfoPtr &groupPtr )
   {
      ossScopedLock _lock( &_nodeMutex, EXCLUSIVE ) ;

      groupPtr->setIdentify( ++_upGrpIndentify ) ;
      _mapGroupInfo[groupPtr->groupID()] = groupPtr ;

      _clearGroupName( groupPtr->groupID() ) ;

      _addGroupName( groupPtr->groupName(), groupPtr->groupID() ) ;
   }

   CoordGroupInfoPtr _coordResource::getCataGroupInfo()
   {
      CoordGroupInfoPtr tmpPtr ;

      ossScopedLock lock( &_nodeMutex, SHARED ) ;
      tmpPtr = _cataGroupInfo ;

      return tmpPtr ;
   }

   CoordGroupInfoPtr _coordResource::getOmGroupInfo()
   {
      return _omGroupInfo ;
   }

   INT32 _coordResource::groupID2Name( UINT32 id, std::string &name )
   {
      INT32 rc = SDB_OK ;
      MAP_GROUP_INFO_IT it ;

      ossScopedLock _lock( &_nodeMutex, SHARED ) ;

      it = _mapGroupInfo.find( id ) ;
      if ( it == _mapGroupInfo.end() )
      {
         rc = SDB_COOR_NO_NODEGROUP_INFO ;
      }
      else
      {
         name = it->second->groupName() ;
      }

      return rc ;
   }

   INT32 _coordResource::groupName2ID( const CHAR *name, UINT32 &id )
   {
      INT32 rc = SDB_OK ;
      MAP_GROUP_NAME_IT itName ;

      ossScopedLock _lock( &_nodeMutex, SHARED ) ;

      itName = _mapGroupName.find( name ) ;
      if ( itName == _mapGroupName.end() )
      {
         rc = SDB_COOR_NO_NODEGROUP_INFO ;
      }
      else
      {
         id = itName->second ;
      }

      return rc ;
   }

   void _coordResource::_clearGroupName( UINT32 groupID )
   {
      MAP_GROUP_NAME_IT itName = _mapGroupName.begin() ;
      while( itName != _mapGroupName.end() )
      {
         if ( itName->second == groupID )
         {
            _mapGroupName.erase( itName ) ;
            break ;
         }
         ++itName ;
      }
   }

   void _coordResource::_addGroupName( const std::string &name, UINT32 id )
   {
      _mapGroupName[name] = id ;
   }

   void _coordResource::getCataNodeAddrList( CoordVecNodeInfo &vecCata )
   {
      ossScopedLock lock( &_nodeMutex, SHARED ) ;
      vecCata = _cataNodeAddrList ;
   }

   INT32 _coordResource::_updateCataGroupInfoByAddr( _pmdEDUCB *cb,
                                                     CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;
      MsgCatCatGroupReq msgGroupReq ;
      CoordVecNodeInfo cataNodeAddrList ;
      UINT32 sendPos = 0 ;
      UINT16 port = 0 ;
      MsgHeader *pReply = NULL ;

      getCataNodeAddrList( cataNodeAddrList ) ;
      msgGroupReq.id.columns.groupID = CAT_CATALOG_GROUPID ;
      msgGroupReq.id.columns.nodeID = 0 ;
      msgGroupReq.id.columns.serviceID = 0;
      msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq );
      msgGroupReq.header.opCode = MSG_CAT_GRP_REQ ;
      msgGroupReq.header.routeID.value = 0 ;
      msgGroupReq.header.TID = cb->getTID() ;
      msgGroupReq.header.requestID = cb->incCurRequestID() ;

      if ( cataNodeAddrList.empty() )
      {
         rc = SDB_CAT_NO_ADDR_LIST ;
         goto error ;
      }

      while( sendPos < cataNodeAddrList.size() )
      {
         clsNodeItem &item = cataNodeAddrList[ sendPos++ ] ;
         rc = ossGetPort( item._service[ MSG_ROUTE_CAT_SERVICE ].c_str(),
                          port ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Convert service[%s:%s] to port failed, rc: %d",
                    item._host, item._service[ MSG_ROUTE_CAT_SERVICE ].c_str(),
                    rc ) ;
            goto error ;
         }
         else
         {
            ossSocket sock( item._host, port ) ;
            rc = sock.initSocket() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Init socket[%s:%u] failed, rc: %d",
                       item._host, port, rc ) ;
               continue ;
            }
            rc = sock.connect( COORD_SOCKET_OPR_DFT_TIME ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Connect to %s:%u failed, rc: %d",
                       item._host, port, rc ) ;
               continue ;
            }

            sock.disableNagle() ;
            sock.setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                               OSS_SOCKET_KEEP_INTERVAL,
                               OSS_SOCKET_KEEP_CONTER ) ;

            rc = pmdSyncSendMsg( ( const MsgHeader* )&msgGroupReq, &pReply,
                                 &sock, cb, TRUE, OSS_SOCKET_DFT_TIMEOUT,
                                 COORD_SOCKET_FORCE_TIMEOUT ) ;
            if ( rc )
            {
               if ( SDB_APP_FORCED == rc )
               {
                  goto error ;
               }
               PD_LOG( PDWARNING, "Sync send message to %s:%u failed, rc: %d",
                       item._host, port, rc ) ;
               continue ;
            }

            if ( pReply->opCode != MSG_CAT_GRP_RES )
            {
               PD_LOG( PDERROR, "Recieve unexpect response[%s]",
                       msg2String( pReply ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            rc = _processGroupReply( pReply, groupPtr ) ;
            cb->releaseBuff( ( CHAR* )pReply ) ;
            pReply = NULL ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to process catalog group reply, "
                       "rc: %d", rc ) ;
               continue ;
            }
            else
            {
               break ;
            }
         }
      }

   done:
      if ( pReply )
      {
         cb->releaseBuff( ( CHAR* )pReply ) ;
         pReply = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::_processGroupReply( MsgHeader *pMsg,
                                             CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfo *pGroupInfo = NULL ;
      UINT32 headerLen = MSG_GET_INNER_REPLY_HEADER_LEN( pMsg ) ;

      if ( SDB_OK == MSG_GET_INNER_REPLY_RC( pMsg ) &&
           (UINT32)pMsg->messageLength >= headerLen + 5 )
      {
         try
         {
            BSONObj boGroupInfo( MSG_GET_INNER_REPLY_DATA( pMsg ) ) ;
            BSONElement beGroupID = boGroupInfo.getField( CAT_GROUPID_NAME ) ;
            if ( beGroupID.eoo() || !beGroupID.isNumber() )
            {
               rc = SDB_SYS ;
               PD_LOG ( PDERROR, "Reply object[%s] is invalid",
                        boGroupInfo.toString().c_str() ) ;
               goto error ;
            }
            pGroupInfo = SDB_OSS_NEW CoordGroupInfo( beGroupID.number() ) ;
            if ( NULL == pGroupInfo )
            {
               rc = SDB_OOM ;
               PD_LOG ( PDERROR, "Alloc group info failed" ) ;
               goto error ;
            }

            rc = pGroupInfo->updateGroupItem( boGroupInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update group info from bson[%s] failed, "
                       "rc: %d", boGroupInfo.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }

         groupPtr = CoordGroupInfoPtr( pGroupInfo ) ;
         pGroupInfo = NULL ;
      }
      else
      {
         rc = MSG_GET_INNER_REPLY_RC( pMsg ) ;
      }

   done:
      if ( pGroupInfo )
      {
         SDB_OSS_DEL pGroupInfo ;
         pGroupInfo = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::_processGroupContextReply( INT64 &contextID,
                                                    GROUP_VEC &vecGroupPtr,
                                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      CoordGroupInfo *pGroupInfo = NULL ;
      CoordGroupInfoPtr groupPtr ;

      while ( TRUE )
      {
         rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Get more from context[%lld] failed, rc: %d",
                       contextID ) ;
               goto error ;
            }
            rc = SDB_OK ;
            break ;
         }

         try
         {
            BSONObj boGroupInfo( buffObj.data() ) ;
            BSONElement beGroupID = boGroupInfo.getField( CAT_GROUPID_NAME ) ;
            if ( beGroupID.eoo() || !beGroupID.isNumber() )
            {
               rc = SDB_SYS ;
               PD_LOG ( PDERROR, "Reply object[%s] is invalid",
                        boGroupInfo.toString().c_str() ) ;
               goto error ;
            }
            pGroupInfo = SDB_OSS_NEW CoordGroupInfo( beGroupID.number() ) ;
            if ( NULL == pGroupInfo )
            {
               rc = SDB_OOM ;
               PD_LOG ( PDERROR, "Alloc group info failed" ) ;
               goto error ;
            }
            groupPtr = CoordGroupInfoPtr( pGroupInfo ) ;

            rc = groupPtr->updateGroupItem( boGroupInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update group info from bson[%s] failed, "
                       "rc: %d", boGroupInfo.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }

         vecGroupPtr.push_back( groupPtr ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::_updateCataGroupInfo( _pmdEDUCB *cb,
                                               const CoordGroupInfoPtr &cataGroupPtr,
                                               CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;
      MsgCatCatGroupReq msgGroupReq ;
      pmdRemoteSessionSite *pSite = NULL ;
      pmdRemoteSession *pRSession = NULL ;
      pmdSubSession *pSubSession = NULL ;
      coordRemoteHandlerBase baseHandle ;

      clsGroupItem *groupItem = NULL ;
      NODE_ARRAY nodes ;
      NODE_ARRAY faultNodes ;
      UINT32 normalNodeCount = 0 ;
      UINT32 beginPos = 0 ;
      UINT32 sendTimes = 0 ;
      INT32  status = 0 ;
      MsgRouteID nodeID ;

      msgGroupReq.id.columns.groupID = CAT_CATALOG_GROUPID ;
      msgGroupReq.id.columns.nodeID = 0 ;
      msgGroupReq.id.columns.serviceID = 0;
      msgGroupReq.header.messageLength = sizeof( MsgCatGroupReq ) ;
      msgGroupReq.header.opCode = MSG_CAT_GRP_REQ ;
      msgGroupReq.header.routeID.value = 0 ;

      groupItem = cataGroupPtr.get() ;
      SDB_ASSERT( groupItem && groupItem->nodeCount() > 0,
                  "Group item's node count must grater than zero" ) ;

      UINT32 primaryPos = groupItem->getPrimaryPos() ;
      if ( CLS_RG_NODE_POS_INVALID != primaryPos &&
           SDB_OK == groupItem->getNodeID( primaryPos, nodeID,
                                           MSG_ROUTE_CAT_SERVICE ) &&
           SDB_OK == groupItem->getNodeInfo( primaryPos, status ) &&
           NET_NODE_STAT_NORMAL == status )
      {
         nodes.append( nodeID.value ) ;
      }
      else
      {
         primaryPos = CLS_RG_NODE_POS_INVALID ;
      }
      beginPos = ossRand() % groupItem->nodeCount() ;
      while( sendTimes < groupItem->nodeCount() )
      {
         rc = groupItem->getNodeID( beginPos, nodeID, MSG_ROUTE_CAT_SERVICE ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = groupItem->getNodeInfo( beginPos, status ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( primaryPos == beginPos )
         {
         }
         else if ( NET_NODE_STAT_NORMAL == status )
         {
            nodes.append( nodeID.value ) ;
         }
         else
         {
            faultNodes.append( nodeID.value ) ;
         }
         beginPos = ( beginPos + 1 ) % groupItem->nodeCount() ;
         ++sendTimes ;
      }
      normalNodeCount = nodes.size() ;

      if ( !faultNodes.empty() )
      {
         NODE_ARRAY::iterator it( faultNodes ) ;
         UINT64 tmpID = 0 ;
         while( it.more() )
         {
            it.next( tmpID ) ;
            nodes.append( tmpID ) ;
         }
      }

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Session[%s] is not registered for remote session",
                 cb->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pRSession = pSite->addSession( -1, &baseHandle ) ;

      for ( UINT32 i = 0 ; i < nodes.size() ; ++i )
      {
         nodeID.value = nodes[ i ] ;
         pRSession->clearSubSession() ;
         pSubSession = pRSession->addSubSession( nodeID.value ) ;
         pSubSession->setReqMsg( ( MsgHeader* )&msgGroupReq,
                                 PMD_EDU_MEM_NONE ) ;
         rc = pRSession->sendMsg( pSubSession ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Send message to node[%s] failed, rc: %d",
                    routeID2String( nodeID ).c_str(), rc ) ;
            if ( i < normalNodeCount )
            {
               groupItem->updateNodeStat( nodeID.columns.nodeID,
                                          netResult2Status( rc ) ) ;
            }
            continue ;
         }
         if ( i >= normalNodeCount )
         {
            groupItem->updateNodeStat( nodeID.columns.nodeID,
                                       NET_NODE_STAT_NORMAL ) ;
         }

         rc = pRSession->waitReply( TRUE ) ;
         if ( rc )
         {
            if ( SDB_APP_INTERRUPT != rc )
            {
               PD_LOG( PDERROR, "Wait reply from node[%s] failed, rc: %d",
                       routeID2String( nodeID ).c_str(), rc ) ;
            }
            goto error ;
         }

         rc = _processGroupReply( pSubSession->getRspMsg(), groupPtr ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to process catalog group reply, "
                    "rc: %d", rc ) ;
            if ( SDB_CLS_FULL_SYNC == rc || SDB_RTN_IN_REBUILD == rc )
            {
               groupItem->updateNodeStat( nodeID.columns.nodeID,
                                          netResult2Status( rc ) ) ;
            }
            continue ;
         }
         else
         {
            break ;
         }
      }

   done:
      if ( pRSession )
      {
         pSite->removeSession( pRSession ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::updateCataGroupInfo( CoordGroupInfoPtr &groupPtr,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr cataGroupInfo ;

      cataGroupInfo = getCataGroupInfo() ;

      if ( 0 == cataGroupInfo->nodeCount() )
      {
         rc = _updateCataGroupInfoByAddr( cb, groupPtr ) ;
      }
      else
      {
         rc = _updateCataGroupInfo( cb, cataGroupInfo, groupPtr ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      rc = _updateRouteInfo( groupPtr, MSG_ROUTE_CAT_SERVICE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update catalog group's cata serivce route "
                   "info failed, rc: %d", rc ) ;
      rc = _updateRouteInfo( groupPtr, MSG_ROUTE_SHARD_SERVCIE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update catalog group's shard serivce route "
                   "info failed, rc: %d", rc ) ;

      addGroupInfo( groupPtr ) ;
      setCataGroupInfo( groupPtr ) ;
      syncAddress2Options( TRUE, FALSE) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::_updateRouteInfo( const CoordGroupInfoPtr &groupPtr,
                                           MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;

      string host ;
      string service ;
      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      UINT32 index = 0 ;
      clsGroupItem *groupItem = groupPtr.get() ;
      while ( SDB_OK == groupItem->getNodeInfo( index++, routeID, host,
                                                service, type ) )
      {
         rc = _pAgent->updateRoute( routeID, host.c_str(),
                                    service.c_str() ) ;
         if ( rc != SDB_OK )
         {
            if ( SDB_NET_UPDATE_EXISTING_NODE == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG ( PDERROR, "Update route[%s] failed, rc: %d",
                        routeID2String( routeID ).c_str(), rc ) ;
               break ;
            }
         }
      }

      return rc ;
   }

   INT32 _coordResource::_updateGroupInfo( MsgHeader *pMsg,
                                           _pmdEDUCB *cb,
                                           MSG_ROUTE_SERVICE_TYPE type,
                                           CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;
      coordGroupSession session ;
      pmdSubSession *pSub = NULL ;
      MsgHeader *pReply = NULL ;

      rc = session.init( this, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init coord remote session failed, rc: %d", rc ) ;
         goto error ;
      }
      session.getGroupSel()->setPrimary( TRUE ) ;
      session.getGroupSel()->setServiceType( type ) ;

   retry:
      session.getSession()->clearSubSession() ;
      rc = session.sendMsg( pMsg, CATALOG_GROUPID, NULL, &pSub ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = session.getSession()->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply from catalog group failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      pReply = pSub->getRspMsg() ;
      rc = _processGroupReply( pReply, groupPtr ) ;
      if ( rc )
      {
         coordGroupSessionCtrl *pGroupCtrl = session.getGroupCtrl() ;
         UINT32 primaryID = MSG_GET_INNER_REPLY_STARTFROM( pReply ) ;

         if ( pGroupCtrl->canRetry( rc, pReply->routeID,
                                    primaryID, TRUE, TRUE ) )
         {
            pGroupCtrl->incRetry() ;
            goto retry ;
         }

         PD_LOG( PDERROR, "Failed to process group reply, rc: %d", rc ) ;
         goto error ;
      }

      rc = _updateRouteInfo( groupPtr, MSG_ROUTE_SHARD_SERVCIE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update group[%u]'s shard serivce route "
                   "info failed, rc: %d", groupPtr->groupID(), rc ) ;

      addGroupInfo( groupPtr ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::getCataInfo( const CHAR *collectionName,
                                      CoordCataInfoPtr &cataPtr )
   {
      INT32 rc = SDB_CAT_NO_MATCH_CATALOG ;
      MAP_CATA_INFO_IT it ;

      ossScopedLock lock( &_cataMutex, SHARED ) ;

      if ( !collectionName || !(*collectionName) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      it = _mapCataInfo.find( collectionName ) ;
      if ( it != _mapCataInfo.end() )
      {
         cataPtr = it->second ;
         rc = SDB_OK ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordResource::removeCataInfo( const CHAR *collectionName )
   {
      _cataMutex.get() ;
      _mapCataInfo.erase( collectionName ) ;
      _cataMutex.release() ;
   }

   void _coordResource::removeCataInfoWithMain( const CHAR *collectionName )
   {

      string strSubCLName = collectionName ;
      MAP_CATA_INFO_IT it ;
      clsCatalogSet *pCatSet = NULL ;

      ossScopedLock _lock( &_cataMutex, EXCLUSIVE ) ;

      it = _mapCataInfo.find( collectionName ) ;
      if ( it != _mapCataInfo.end() )
      {
         string mainCL ;
         pCatSet = it->second->getCatalogSet() ;
         mainCL = pCatSet->getMainCLName() ;

         _mapCataInfo.erase( it ) ;
         if ( !mainCL.empty() )
         {
            _mapCataInfo.erase( mainCL.c_str() ) ;
         }
      }
      else
      {
         it = _mapCataInfo.begin() ;
         while( it != _mapCataInfo.end() )
         {
            pCatSet = it->second->getCatalogSet() ;
            if ( !pCatSet || !pCatSet->isMainCL() )
            {
            }
            else if ( pCatSet->isContainSubCL( strSubCLName ) )
            {
               _mapCataInfo.erase( it++ ) ;
               continue ;
            }
            ++it ;
         }
      }
   }

   void _coordResource::removeCataInfoByCS( const CHAR *csName,
                                            vector < string > *pRelatedCLs )
   {
      UINT32 len = 0 ;
      clsCatalogSet *pCatSet = NULL ;
      const CHAR *pCLName = NULL ;
      MAP_CATA_INFO_IT it ;
      BOOLEAN locked = FALSE ;

      if ( !csName || !( *csName ) )
      {
         goto done ;
      }
      len = ossStrlen( csName ) ;

      _cataMutex.get() ;
      locked = TRUE ;

      it = _mapCataInfo.begin() ;

      while( it != _mapCataInfo.end() )
      {
         pCatSet = it->second->getCatalogSet() ;
         pCLName = it->first ;

         if ( pRelatedCLs && len > 0 && pCatSet )
         {
            string strMainCL = pCatSet->getMainCLName() ;

            if ( 0 == ossStrncmp( strMainCL.c_str(), csName, len ) &&
                 '.' == strMainCL.at( len ) )
            {
               pRelatedCLs->push_back( it->first ) ;
            }
         }

         if ( 0 == ossStrncmp( pCLName, csName, len ) &&
              '.' == pCLName[ len ] )
         {
            _mapCataInfo.erase( it++ ) ;
         }
         else
         {
            ++it ;
         }
      }

   done:
      if ( locked )
      {
         _cataMutex.release() ;
      }
      return ;
   }

   void _coordResource::addCataInfo( CoordCataInfoPtr &cataPtr )
   {
      _cataMutex.get() ;
      _mapCataInfo.erase( cataPtr->getName() ) ;
      _mapCataInfo[ cataPtr->getName() ] = cataPtr ;
      _cataMutex.release() ;
   }

   UINT32 _coordResource::checkAndRemoveCataInfoBySub( const CHAR *collectionName )
   {
      string strSubCLName = collectionName ;
      MAP_CATA_INFO_IT it ;
      clsCatalogSet *pCatSet = NULL ;
      UINT32 count = 0 ;

      ossScopedLock _lock( &_cataMutex, EXCLUSIVE ) ;
      it = _mapCataInfo.begin() ;
      while( it != _mapCataInfo.end() )
      {
         pCatSet = it->second->getCatalogSet() ;
         if ( !pCatSet || !pCatSet->isMainCL() )
         {
         }
         else if ( pCatSet->isContainSubCL( strSubCLName ) )
         {
            _mapCataInfo.erase( it++ ) ;
            ++count ;
            continue ;
         }
         ++it ;
      }
      return count ;
   }

   void _coordResource::invalidateCataInfo()
   {
      _cataMutex.get() ;
      _mapCataInfo.clear() ;
      _cataMutex.release() ;
   }

   void _coordResource::invalidateGroupInfo( UINT64 identify )
   {
      ossScopedLock _lock( &_nodeMutex, EXCLUSIVE ) ;

      if ( 0 == identify )
      {
         _mapGroupInfo.clear() ;
         _mapGroupName.clear() ;
      }
      else
      {
         MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
         while( it != _mapGroupInfo.end() )
         {
            if ( it->second->getIdentify() <= identify )
            {
               MAP_GROUP_NAME_IT itName =
                  _mapGroupName.find( it->second->groupName() ) ;
               if ( itName != _mapGroupName.end() )
               {
                  _mapGroupName.erase( itName ) ;
               }
               _mapGroupInfo.erase( it++ ) ;
            }
            else
            {
               ++it ;
            }
         }
      }
   }

   INT32 _coordResource::updateCataInfo( const CHAR *collectionName,
                                         CoordCataInfoPtr &cataPtr,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      CoordCataInfoPtr tmpCataPtr ;

      try
      {
         obj = BSON( CAT_CATALOGNAME_NAME << collectionName ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _updateCataInfo( obj, collectionName, tmpCataPtr, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", collectionName, rc ) ;

         if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_EOC == rc )
         {
            if ( checkAndRemoveCataInfoBySub( collectionName ) > 0 )
            {
               rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
            }
            removeCataInfo( collectionName ) ;
         }
         goto error ;
      }

      if ( tmpCataPtr->isMainCL() && tmpCataPtr->getSubCLCount() > 0 )
      {
         CoordCataInfoPtr tmpPtr ;
         try
         {
            obj = BSON( CAT_MAINCL_NAME << collectionName ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         rc = _updateCataInfo( obj, "", tmpPtr, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update collection[%s]'s all sub-collections "
                    "catalog info failed, rc: %d", collectionName, rc ) ;
            removeCataInfo( collectionName ) ;
            goto error ;
         }
      }

      cataPtr = tmpCataPtr ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::getOrUpdateCataInfo( const CHAR *collectionName,
                                              CoordCataInfoPtr &cataPtr,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = getCataInfo( collectionName, cataPtr ) ;
      if ( rc )
      {
         rc = updateCataInfo( collectionName, cataPtr, cb ) ;
      }
      return rc ;
   }

   INT32 _coordResource::getOrUpdateGroupInfo( UINT32 groupID,
                                               CoordGroupInfoPtr &groupPtr,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = getGroupInfo( groupID, groupPtr ) ;
      if ( rc )
      {
         rc = updateGroupInfo( groupID, groupPtr, cb ) ;
      }
      return rc ;
   }

   INT32 _coordResource::getOrUpdateGroupInfo( const CHAR *groupName,
                                               CoordGroupInfoPtr &groupPtr,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = getGroupInfo( groupName, groupPtr ) ;
      if ( rc )
      {
         rc = updateGroupInfo( groupName, groupPtr, cb ) ;
      }
      return rc ;
   }

   INT32 _coordResource::_updateCataInfo( const BSONObj &obj,
                                          const CHAR *collectionName,
                                          CoordCataInfoPtr &cataPtr,
                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      coordGroupSession session ;
      pmdSubSession *pSub = NULL ;
      MsgHeader *pReply = NULL ;

      CHAR *pBuffer = NULL ;
      INT32 bufferSize = 0 ;

      rc = session.init( this, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init coord remote session failed, rc: %d", rc ) ;
         goto error ;
      }
      session.getGroupSel()->setPrimary( TRUE ) ;
      session.getGroupSel()->setServiceType( MSG_ROUTE_CAT_SERVICE ) ;

      rc = msgBuildQueryCatalogReqMsg ( &pBuffer, &bufferSize,
                                        0, 0, 0, -1, cb->getTID(),
                                        &obj, NULL, NULL, NULL,
                                        cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Build query catalog request failed, rc: %d",
                  rc ) ;
         goto error ;
      }

   retry:
      session.getSession()->clearSubSession() ;
      rc = session.sendMsg( (MsgHeader*)pBuffer, CATALOG_GROUPID,
                            NULL, &pSub ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = session.getSession()->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply from catalog group failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      pReply = pSub->getRspMsg() ;
      rc = _processCatalogReply( pReply, collectionName, cataPtr ) ;
      if ( rc )
      {
         coordGroupSessionCtrl *pGroupCtrl = session.getGroupCtrl() ;
         UINT32 primaryID = ((MsgOpReply*)pReply)->startFrom ;

         if ( pGroupCtrl->canRetry( rc, pReply->routeID,
                                    primaryID, TRUE, TRUE ) )
         {
            pGroupCtrl->incRetry() ;
            goto retry ;
         }

         PD_LOG( PDERROR, "Failed to process catalog info reply, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( pBuffer )
      {
         cb->releaseBuff( pBuffer ) ;
         bufferSize = 0 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordResource::_processCatalogReply( MsgHeader *pMsg,
                                               const CHAR *collectionName,
                                               CoordCataInfoPtr &cataPtr )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pReply = ( MsgOpReply* )pMsg ;
      INT32 offset = 0 ;
      UINT32 objNum = 0 ;
      BOOLEAN isSpec = FALSE ;

      SDB_ASSERT( -1 == pReply->contextID, "ContextID must be -1" ) ;

      if ( collectionName && *collectionName )
      {
         isSpec = TRUE ;
      }

      rc = pReply->flags ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Recieved unexpected reply for catalog info "
                 "request from node[%s], flag: %d",
                 routeID2String( pMsg->routeID ).c_str(), rc ) ;
         goto error ;
      }

      offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
      while ( offset < pMsg->messageLength )
      {
         CoordCataInfoPtr tmpPtr ;
         try
         {
            BSONObj obj( (CHAR*)pMsg + offset ) ;
            offset += ossRoundUpToMultipleX ( obj.objsize(), 4 ) ;
            ++objNum ;

            rc = coordInitCataPtrFromObj( obj, tmpPtr ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Init catalog info from obj[%s] failed, "
                       "rc: %d", obj.toString().c_str(), rc ) ;
               goto error ;
            }
            addCataInfo( tmpPtr ) ;
            if ( isSpec && 
                 0 == ossStrcmp( collectionName, tmpPtr->getName() ) )
            {
               cataPtr = tmpPtr ;
               isSpec = FALSE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      SDB_ASSERT( objNum == (UINT32)pReply->numReturned,
                  "Return number is invalid" ) ;

      if ( objNum < 1 || isSpec )
      {
         PD_LOG( PDERROR, "Recieved unexpect reply for catalog info from "
                 "node[%s]: no spec object in the all objects[%d]",
                 routeID2String( pMsg->routeID ).c_str(), objNum ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

