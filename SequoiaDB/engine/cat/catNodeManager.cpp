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

   Source File Name = catNodeManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#include "catCommon.hpp"
#include "../util/fromjson.hpp"
#include "msgCatalog.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "catNodeManager.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "utilCommon.hpp"

using namespace bson;

#define CAT_PORT_STR_SZ 10

namespace engine
{

   /*
      catNodeManager implement
   */
   catNodeManager::catNodeManager()
   {
      _pDmsCB = NULL ;
      _pDpsCB = NULL ;
      _pRtnCB = NULL ;
      _pCatCB = NULL ;
      _pEduCB = NULL ;
   }

   catNodeManager::~catNodeManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_INIT, "catNodeManager::init" )
   INT32 catNodeManager::init()
   {
      INT32 rc = SDB_OK;
      pmdKRCB *krcb     = pmdGetKRCB() ;
      _pDmsCB           = krcb->getDMSCB();
      _pDpsCB           = krcb->getDPSCB();
      _pRtnCB           = krcb->getRTNCB();
      _pCatCB           = krcb->getCATLOGUECB();
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_INIT ) ;

      // 1. insert self to node collection
      rc = readCataConf() ;

      PD_TRACE_EXITRC ( SDB_CATNODEMGR_INIT, rc ) ;
      return rc ;
   }

   INT32 catNodeManager::fini()
   {
      return SDB_OK ;
   }

   void catNodeManager::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;

      /// load group info and ignore the error info
      _loadGroupInfo() ;
   }

   void catNodeManager::detachCB( pmdEDUCB * cb )
   {
      _pEduCB = NULL ;
   }

   // when the node switch to  primary will call this fun
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_ACTIVE, "catNodeManager::active" )
   INT32 catNodeManager::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATNODEMGR_ACTIVE ) ;

      _pCatCB->clearInfo() ;
      rc = _loadGroupInfo() ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_ACTIVE, rc ) ;
      return rc;
   error :
      PD_LOG( PDSEVERE, "Stop program because of active node manager failed, "
              "rc: %d", rc ) ;
      // need to restart engine
      PMD_RESTART_DB( rc ) ;
      goto done ;
   }

   INT32 catNodeManager::deactive()
   {
      _pCatCB->clearInfo() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_LOADGROUPINFO, "catNodeManager::_loadGroupInfo" )
   INT32 catNodeManager::_loadGroupInfo()
   {
      //get all of the nodes's id
      INT32 rc            = SDB_OK ;
      rtnContextBuf buffObj ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_LOADGROUPINFO ) ;

      BSONObj boEmpty ;
      SINT64 sContextID   = -1 ;
      CHAR szBuf[ OP_MAXNAMELENGTH+1 ] = { 0 } ;
      ossStrncpy( szBuf, CAT_NODE_INFO_COLLECTION, OP_MAXNAMELENGTH ) ;
      // query from collection table
      rc = rtnQuery ( szBuf, boEmpty, boEmpty,
                      boEmpty, boEmpty, 0, _pEduCB, 0, -1, _pDmsCB,
                      _pRtnCB, sContextID ) ;
      if ( rc )
      {
         // rtnQuery supposed to be never failed
         PD_LOG ( PDERROR, "Failed to query %s collection, rc = %d",
                  CAT_NODE_INFO_COLLECTION, rc ) ;
         goto error ;
      }
      while ( TRUE )
      {
         rc = rtnGetMore( sContextID, 1, buffObj, _pEduCB, _pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               // loop until EOC
               rc = SDB_OK ;
               sContextID = -1 ;
               break ;
            }
            PD_LOG ( PDERROR, "Failed to fetch from %s collection, rc = %d",
                     CAT_NODE_INFO_COLLECTION, rc );
            goto error ;
         }

         if ( buffObj.data() != NULL )
         {
            try
            {
               BSONObj bsGrpInfo( buffObj.data() );
               PD_TRACE1 ( SDB_CATNODEMGR_ACTIVE,
                           PD_PACK_BSON ( bsGrpInfo ) ) ;
               rc = parseIDInfo( bsGrpInfo ) ;
               if ( rc )
               {
                  // if we cannot parse it, something wrong
                  if ( SDB_INVALIDARG == rc )
                  {
                     rc = SDB_CAT_CORRUPTION ;
                  }
                  PD_LOG( PDERROR, "Failed to parse node info: %s",
                          bsGrpInfo.toString().c_str() ) ;
                  goto error ;
               }
            }
            catch (std::exception &e)
            {
               PD_LOG ( PDERROR, "Invalid data is read from context buffer: %s",
                        e.what() ) ;
               rc = SDB_CAT_CORRUPTION ;
               goto error ;
            }
         } // if ( pBuffer != NULL )
      } // while ( TRUE );

   done :
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_LOADGROUPINFO, rc ) ;
      return rc;
   error :
      if ( -1 != sContextID )
      {
         _pRtnCB->contextDelete( sContextID, _pEduCB );
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PROCESSMSG, "catNodeManager::processMsg" )
   INT32 catNodeManager::processMsg( const NET_HANDLE &handle,
                                     MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PROCESSMSG ) ;
      PD_TRACE1 ( SDB_CATNODEMGR_PROCESSMSG, PD_PACK_INT ( pMsg->opCode ) ) ;

      switch ( pMsg->opCode )
      {
      case MSG_CAT_REG_REQ:
            rc = processRegReq( handle, pMsg ) ;
            break;

      case MSG_CAT_CATGRP_REQ:
      case MSG_CAT_NODEGRP_REQ:
      case MSG_CAT_GRP_REQ:
            rc = processGrpReq( handle, pMsg ) ;
            break;

      case MSG_CAT_PAIMARY_CHANGE:
            rc = processPrimaryChange( handle, pMsg ) ;
            break;

      // command message entry, should dispatch in the entry function
      case MSG_CAT_CREATE_GROUP_REQ :
      case MSG_CAT_CREATE_NODE_REQ :
      case MSG_CAT_UPDATE_NODE_REQ :
      case MSG_CAT_DEL_NODE_REQ :
      case MSG_CAT_RM_GROUP_REQ :
      case MSG_CAT_ACTIVE_GROUP_REQ :
      case MSG_CAT_SHUTDOWN_GROUP_REQ :
            rc = processCommandMsg( handle, pMsg, TRUE ) ;
            break;

      default:
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG( PDWARNING, "Received unknown message (opCode: [%d]%u )",
                    IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode) ) ;
            break;
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_PROCESSMSG, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PRIMARYCHANGE, "catNodeManager::processPrimaryChange" )
   INT32 catNodeManager::processPrimaryChange( const NET_HANDLE &handle,
                                               MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PRIMARYCHANGE ) ;
      UINT32 w = _majoritySize() ;
      MsgCatPrimaryChange *pRequest = (MsgCatPrimaryChange *)pMsg ;
      UINT32 groupID = pRequest->newPrimary.columns.groupID;
      UINT16 nodeID = pRequest->newPrimary.columns.nodeID;
      PD_TRACE2 ( SDB_CATNODEMGR_PRIMARYCHANGE,
                  PD_PACK_UINT ( groupID ),
                  PD_PACK_USHORT ( nodeID ) ) ;
      MsgCatPrimaryChangeRes replyHeader;

      /// fill reply header
      _fillRspHeader( &replyHeader.header, pMsg ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      replyHeader.header.messageLength = sizeof( MsgCatPrimaryChangeRes ) ;

      // the msg is send by timer, don't use _pCatCB->primaryCheck()
      if ( !pmdIsPrimary() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG ( PDWARNING, "service deactive but received primary-change "
                  "request" );
         goto error ;
      }
      else if ( pmdGetKRCB()->isDBReadonly() ||
                pmdGetKRCB()->isDBDeactivated() )
      {
         goto done ;
      }

      try
      {
         BSONObj boUpdater;
         BSONObj boMatcher;
         if ( SPARE_GROUPID == groupID)
         {
             boMatcher = BSON( CAT_GROUPID_NAME << groupID) ;
             boUpdater = BSON( "$unset" << BSON( CAT_PRIMARY_NAME<<"") ) ;
         }
         else if ( INVALID_GROUPID == groupID || INVALID_NODEID == nodeID )
         {
            boMatcher = BSON( CAT_GROUPID_NAME <<
                              pRequest->oldPrimary.columns.groupID <<
                              CAT_PRIMARY_NAME <<
                              pRequest->oldPrimary.columns.nodeID ) ;
            boUpdater = BSON( "$unset" << BSON( CAT_PRIMARY_NAME << nodeID ) ) ;
         }
         else
         {
            BSONObj objPrimary ;
            /// check primary node is the same
            rc = catGetOneObj( CAT_NODE_INFO_COLLECTION,
                               BSON( CAT_PRIMARY_NAME << 0 ),
                               BSON( CAT_GROUPID_NAME << groupID ),
                               BSONObj(), _pEduCB, objPrimary ) ;
            if ( SDB_OK == rc )
            {
               BSONElement ele = objPrimary.getField( CAT_PRIMARY_NAME ) ;
               if ( ele.isNumber() && (UINT16)ele.numberInt() == nodeID )
               {
                  /// already the same, don't update
                  goto done ;
               }
            }

            if ( pRequest->header.routeID.columns.nodeID == nodeID )
            {
               boMatcher = BSON( CAT_GROUPID_NAME << groupID ) ;
            }
            else
            {
               boMatcher = BSON( CAT_GROUPID_NAME << groupID <<
                                 CAT_PRIMARY_NAME <<
                                 pRequest->header.routeID.columns.nodeID ) ;
            }
            boUpdater = BSON( "$set" << BSON( CAT_PRIMARY_NAME << nodeID ) ) ;
         }

         rc = rtnUpdate ( CAT_NODE_INFO_COLLECTION, boMatcher, boUpdater,
                          BSONObj(), 0, _pEduCB, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set primary-node(rc=%d)",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( SDB_SYS, SDB_SYS, error, PDERROR,
                   "Failed to set primary-node, received unexpected error:%s",
                   e.what() ) ;
      }

   done:
      PD_TRACE1 ( SDB_CATNODEMGR_PRIMARYCHANGE, PD_PACK_INT ( rc ) ) ;
      rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to send response(primary-change)(rc=%d)",
                 rc ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_PRIMARYCHANGE, rc ) ;
      return rc ;
   error:
      replyHeader.flags = rc ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_GRPREQ, "catNodeManager::processGrpReq" )
   INT32 catNodeManager::processGrpReq( const NET_HANDLE &handle,
                                        MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_GRPREQ ) ;
      MsgOpReply replyHeader ;
      SINT32 dataLen = 0 ;
      BSONObj boGroupInfo ;

      MsgCatGroupReq *pGrpReq = (MsgCatGroupReq *)pMsg ;
      UINT32 groupID = pGrpReq->id.columns.groupID ;
      const CHAR *name = NULL ;

      /// fill reply header
      _fillRspHeader( &(replyHeader.header), &(pGrpReq->header) ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;

      if ( 0 == groupID )
      {
         if ( pGrpReq->header.messageLength >
              (SINT32)sizeof(MsgCatGroupReq) )
         {
            name = (CHAR *)(&(pGrpReq->header)) + sizeof(MsgCatGroupReq) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Groupid and groupName are all not assigned" ) ;
            goto error ;
         }
      }

      PD_TRACE1 ( SDB_CATNODEMGR_GRPREQ, PD_PACK_UINT ( groupID ) ) ;

      // primary check, except catalog
      if ( ( 0 != groupID && CATALOG_GROUPID != groupID ) ||
           ( name && 0 != ossStrcmp( name, CATALOG_GROUPNAME ) ) )
      {
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            // not reply
            goto done ;
         }
         else if ( rc )
         {
            if ( 0 != groupID )
            {
               PD_LOG( PDWARNING, "Service deactive but received "
                       "group-info-request(groupID=%u), rc: %d",
                       groupID, rc ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Service deactive but received "
                       "group-info-request(groupID=%s), rc: %d",
                       name, rc ) ;
            }
            goto error ;
         }
      }

      // get group by groupID
      if ( 0 != groupID )
      {
         rc = catGetGroupObj( groupID, boGroupInfo, _pEduCB ) ;
      }
      // get group by groupName
      else
      {
         rc = catGetGroupObj( name, FALSE, boGroupInfo, _pEduCB ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get group info, rc: %d", rc ) ;

      // if catalog group, get primary node from replset
      if ( CATALOG_GROUPID == boGroupInfo.getIntField( CAT_GROUPID_NAME ) )
      {
         UINT16 nodeID = sdbGetReplCB()->getPrimary().columns.nodeID ;
         if ( 0 != nodeID )
         {
            BSONObjBuilder builder ;
            BSONObjIterator it( boGroupInfo ) ;
            while ( it.more() )
            {
               BSONElement e = it.next() ;
               if ( 0 == ossStrcmp( e.fieldName(), CAT_PRIMARY_NAME ) &&
                    0 != nodeID )
               {
                  builder.append( CAT_PRIMARY_NAME, (INT32)nodeID ) ;
                  nodeID = 0 ;
               }
               else
               {
                  builder.append( e ) ;
               }
            }
            // if in collection the primary is not exist and memory primay exist
            if ( 0 != nodeID )
            {
               builder.append( CAT_PRIMARY_NAME, (INT32)nodeID ) ;
            }
            boGroupInfo = builder.obj() ;
         }
      }

      // build the response message
      dataLen = boGroupInfo.objsize() ;

   done:
      // send reply
      if ( !_pCatCB->isDelayed() )
      {
         PD_TRACE1 ( SDB_CATNODEMGR_GRPREQ, PD_PACK_INT ( rc ) ) ;
         if ( 0 == dataLen )
         {
            rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
         }
         else
         {
            replyHeader.header.messageLength += dataLen ;
            replyHeader.numReturned = 1 ;
            rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                     (void *)boGroupInfo.objdata(), dataLen ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_GRPREQ, rc ) ;
      return rc ;
   error:
      replyHeader.flags = rc ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         // primary node id store in startFrom
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done;
   }

   void catNodeManager::_fillRspHeader( MsgHeader * rspMsg,
                                        const MsgHeader * reqMsg )
   {
      rspMsg->opCode = MAKE_REPLY_TYPE( reqMsg->opCode ) ;
      rspMsg->requestID = reqMsg->requestID ;
      rspMsg->routeID.value = 0 ;
      rspMsg->TID = reqMsg->TID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_REGREQ, "catNodeManager::processRegReq" )
   INT32 catNodeManager::processRegReq( const NET_HANDLE &handle,
                                        MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      INT32 nodeRole = SDB_ROLE_DATA ;
      BSONObj boReq ;
      MsgCatRegisterRsp replyHeader ;
      MsgCatRegisterReq *pRegReq = (MsgCatRegisterReq *)pMsg ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_REGREQ ) ;
      BSONObj boNodeInfo ;
      INT32 realRole = SDB_ROLE_DATA ;

      BOOLEAN isExisted = FALSE ;
      BSONObj boDCInfo ;

      UINT32 dataLen = 0 ;
      CHAR *pData = NULL ;
      BOOLEAN ownedData = FALSE ;
      INT32 objNumber = 0 ;

      /// fill reply header
      _fillRspHeader( &replyHeader.header, pMsg ) ;
      replyHeader.header.messageLength = sizeof( MsgCatRegisterRsp ) ;
      replyHeader.flags = SDB_OK ;
      replyHeader.contextID = -1 ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;

      try
      {
         boReq = BSONObj( ( CHAR *)pRegReq + sizeof(MsgCatRegisterReq));
         PD_LOG( PDEVENT, "get register request:%s", boReq.toString().c_str());
         BSONElement beRole = boReq.getField( FIELD_NAME_ROLE );
         PD_CHECK( beRole.isNumber(), SDB_INVALIDARG, error,
                   PDERROR, "failed to get the field(%s)",
                   FIELD_NAME_ROLE ) ;
         nodeRole = beRole.number();
      }
      catch ( std::exception &e )
      {
         PD_CHECK( SDB_SYS, SDB_SYS, error, PDERROR,
                   "Failed to process register-request, received "
                   "unexpected error:%s", e.what() );
      }

      PD_TRACE1 ( SDB_CATNODEMGR_REGREQ,
                  PD_PACK_BSON ( boReq ) ) ;

      // don't use _pCatCB->primaryCheck(), because reg msg will send by
      // on timer
      PD_CHECK( ( pmdIsPrimary() || SDB_ROLE_CATALOG == nodeRole ),
                SDB_CLS_NOT_PRIMARY, error, PDWARNING,
                "service deactive but received register-request:%s",
                boReq.toString().c_str() );

      rc = getNodeInfo( boReq, boNodeInfo, realRole ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get node-info:%s (rc=%d)",
                  boReq.toString().c_str(), rc );
         rc = ( SDB_CLS_NODE_NOT_EXIST == rc ) ?
                    ( pmdIsPrimary() ? SDB_CAT_AUTH_FAILED :
                                       SDB_CLS_NOT_PRIMARY ) :
                    rc ;
         goto error;
      }
      else if ( realRole != nodeRole )
      {
         PD_LOG( PDERROR, "The register node role[%d] is unexpected[%d]",
                 nodeRole, realRole ) ;
         rc = SDB_CAT_AUTH_FAILED ;
         goto error ;
      }

      rc = _checkAndUpdateNodeInfo( boReq, realRole, boNodeInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Check and update node info failed, rc: %d, "
                 "request obj: %s, node obj: %s", rc,
                 boReq.toString().c_str(), boNodeInfo.toString().c_str() ) ;
         rc = ( SDB_CLS_NODE_NOT_EXIST == rc ) ?
                    ( pmdIsPrimary() ? SDB_CAT_AUTH_FAILED :
                                       SDB_CLS_NOT_PRIMARY ) :
                    rc ;
         goto error ;
      }

      /// get dc base info
      rc = catCheckBaseInfoExist( CAT_BASE_TYPE_GLOBAL_STR, isExisted,
                                  boDCInfo, _pEduCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Check base info failed, rc: %d", rc ) ;
         goto error ;
      }

      // build the response message
      dataLen = boNodeInfo.objsize() ;
      pData = ( CHAR* )boNodeInfo.objdata() ;
      ownedData = FALSE ;
      objNumber = 1 ;

      if ( isExisted && !boDCInfo.isEmpty() )
      {
         dataLen = ossAlign4( dataLen ) ;
         dataLen += boDCInfo.objsize() ;

         pData = ( CHAR* )SDB_THREAD_ALLOC( dataLen ) ;
         if ( !pData )
         {
            PD_LOG( PDERROR, "Alloc memory[%u] failed, rc: %d",
                    dataLen, rc ) ;
            goto error ;
         }
         ownedData = TRUE ;
         objNumber = 2 ;

         /// copy data
         ossMemcpy( pData, boNodeInfo.objdata(), boNodeInfo.objsize() ) ;
         ossMemcpy( pData + ossAlign4( (UINT32)boNodeInfo.objsize() ),
                    boDCInfo.objdata(), boDCInfo.objsize() ) ;
      }

   done:
      PD_TRACE1 ( SDB_CATNODEMGR_REGREQ, PD_PACK_INT ( rc ) ) ;
      if ( 0 == dataLen )
      {
         rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
      }
      else
      {
         replyHeader.header.messageLength += dataLen ;
         replyHeader.numReturned = objNumber ;
         rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                  (void *)pData, dataLen ) ;
      }
      if ( ownedData && pData )
      {
         SDB_THREAD_FREE( pData ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_REGREQ, rc ) ;
      return rc;
   error:
      replyHeader.flags = rc ;
      dataLen = 0 ;
      goto done ;
   }

   INT32 catNodeManager::_checkAndUpdateNodeInfo( const BSONObj &reqObj,
                                                  INT32 role,
                                                  const BSONObj &nodeObj )
   {
      INT32 rc = SDB_OK ;

      /// check local, repl, cata service.
      /// shard has alread check in getNodeInfo
      BSONElement beSvcL = reqObj.getField( CAT_SERVICE_FIELD_NAME ) ;
      BSONElement beSvcR = nodeObj.getField( CAT_SERVICE_FIELD_NAME ) ;

      const CHAR *pSvcL = NULL ;
      const CHAR *pSvcR = NULL ;

      /// local
      pSvcL = getServiceName( beSvcL, MSG_ROUTE_LOCAL_SERVICE ) ;
      pSvcR = getServiceName( beSvcR, MSG_ROUTE_LOCAL_SERVICE ) ;
      if ( 0 != ossStrcmp( pSvcL, pSvcR ) )
      {
         PD_LOG( PDERROR, "Local service is not the same" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }

      /// repl
      pSvcL = getServiceName( beSvcL, MSG_ROUTE_REPL_SERVICE ) ;
      pSvcR = getServiceName( beSvcR, MSG_ROUTE_REPL_SERVICE ) ;
      if ( 0 != ossStrcmp( pSvcL, pSvcR ) )
      {
         PD_LOG( PDERROR, "Repl service is not the same" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }

      /// cat
      if ( role == SDB_ROLE_CATALOG )
      {
         pSvcL = getServiceName( beSvcL, MSG_ROUTE_CAT_SERVICE ) ;
         pSvcR = getServiceName( beSvcR, MSG_ROUTE_CAT_SERVICE ) ;
         if ( 0 != ossStrcmp( pSvcL, pSvcR ) )
         {
            PD_LOG( PDERROR, "Cat service is not the same" ) ;
            rc = SDB_CLS_NODE_NOT_EXIST ;
            goto error ;
         }
      }

      /// update dbpath and status
      if ( pmdIsPrimary() )
      {
         BSONObj groupInfo ;
         INT32 nodeID = CAT_INVALID_NODEID ;
         BOOLEAN keepInstanceID = FALSE ;
         BSONObjBuilder updateBuilder ;

         BSONElement beDbpath = reqObj.getField( PMD_OPTION_DBPATH ) ;
         if ( String == beDbpath.type() )
         {
            updateBuilder.append( beDbpath ) ;
         }

         BSONElement beInstanceID = reqObj.getField( PMD_OPTION_INSTANCE_ID ) ;
         if ( beInstanceID.isNumber() &&
              utilCheckInstanceID( beInstanceID.numberInt(), FALSE ) )
         {
            updateBuilder.append( beInstanceID ) ;
            keepInstanceID = TRUE ;
         }

         rc = rtnGetIntElement( nodeObj, FIELD_NAME_NODEID, nodeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_NODEID, rc ) ;

         rc = catGetGroupObj( (UINT16)nodeID, groupInfo, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group info by nodeid[%d], "
                      "rc: %d", nodeID, rc ) ;

         rc = _updateNodeToGrp( groupInfo, updateBuilder.obj(), (UINT16)nodeID,
                                TRUE, TRUE, keepInstanceID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update node to group, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catNodeManager::processCommandMsg( const NET_HANDLE &handle,
                                            MsgHeader *pMsg,
                                            BOOLEAN writable )
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      MsgOpReply replyHeader ;
      rtnContextBuf ctxBuff ;

      INT32 flag = 0 ;
      CHAR *pCMDName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;

      // init reply msg
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      _fillRspHeader( &(replyHeader.header), &(pQueryReq->header) ) ;

      // extract msg
      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG ( PDWARNING, "Service deactive but received command: %s"
                     "opCode: %d, rc: %d", pCMDName,
                     pQueryReq->header.opCode, rc ) ;
            goto error ;
         }
      }

      // the second dispatch msg
      switch ( pQueryReq->header.opCode )
      {
         case MSG_CAT_CREATE_GROUP_REQ :
            rc = processCmdCreateGrp( pQuery ) ;
            break ;
         case MSG_CAT_UPDATE_NODE_REQ :
            rc = processCmdUpdateNode( handle, pQuery, pFieldSelector ) ;
            break ;
         case MSG_CAT_ACTIVE_GROUP_REQ :
         case MSG_CAT_SHUTDOWN_GROUP_REQ :
         case MSG_CAT_RM_GROUP_REQ :
         case MSG_CAT_CREATE_NODE_REQ :
         case MSG_CAT_DEL_NODE_REQ :
         {
            SINT64 contextID = -1;
            catContext *pCatCtx = NULL ;
            rc = catCreateContext ( (MSG_TYPE)pQueryReq->header.opCode,
                                    &pCatCtx, contextID,
                                    _pEduCB ) ;
            if ( SDB_OK == rc )
            {
               rc = pCatCtx->open( handle, pMsg, pQuery, ctxBuff, _pEduCB ) ;
               if ( SDB_OK != rc )
               {
                  catDeleteContext( contextID, _pEduCB ) ;
                  contextID = -1 ;
                  pCatCtx = NULL ;
               }
               else
               {
                  replyHeader.contextID = contextID ;
                  _pCatCB->addContext( handle, replyHeader.header.TID,
                                       contextID ) ;
               }
            }
            break;
         }
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Received unknown command: %s, opCode: %d",
                    pCMDName, pQueryReq->header.opCode ) ;
            break ;
      }

      if ( SDB_LOCK_FAILED == rc )
      {
         goto lock_failed ;
      }
      PD_RC_CHECK( rc, PDERROR, "Process command[%s] failed, opCode: %d, "
                   "rc: %d", pCMDName, pQueryReq->header.opCode, rc ) ;

   done:
      // send reply
      if ( !_pCatCB->isDelayed() )
      {
         if ( 0 == ctxBuff.size() )
         {
            rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
         }
         else
         {
            replyHeader.header.messageLength += ctxBuff.size() ;
            replyHeader.numReturned = ctxBuff.recordNum() ;
            rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                     (void *)ctxBuff.data(), ctxBuff.size() ) ;
         }
      }
      return rc ;

   lock_failed:
      // Lock failed, then try to delay operation
      if ( !_pCatCB->delayCurOperation() )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      else
      {
         // Ignore the lock error
         rc = SDB_OK ;
      }
      goto done ;

   error:
      replyHeader.flags = rc ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         // primary node id store in startFrom
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PCREATEGRP, "catNodeManager::processCmdCreateGrp" )
   INT32 catNodeManager::processCmdCreateGrp( const CHAR *pQuery )
   {
      INT32 rc = SDB_OK ;
      const CHAR *groupName = NULL ;

      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PCREATEGRP ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         rc = rtnGetStringElement( boQuery, CAT_GROUPNAME_NAME, &groupName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the field: %s, rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;

         PD_TRACE1 ( SDB_CATNODEMGR_PCREATEGRP,
                     PD_PACK_STRING ( groupName ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "occured unexpected error:%s", e.what() );
         goto error;
      }

      rc = _createGrp( groupName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process create group request(rc=%d)",
                   rc ) ;

   done:
      PD_TRACE1 ( SDB_CATNODEMGR_PCREATEGRP, PD_PACK_INT ( rc ) ) ;
      PD_TRACE_EXITRC( SDB_CATNODEMGR_PCREATEGRP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_UPDATENODE, "catNodeManager::processCmdUpdateNode" )
   INT32 catNodeManager::processCmdUpdateNode( const NET_HANDLE &handle,
                                               const CHAR *pQuery,
                                               const CHAR *pSelector )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_UPDATENODE ) ;
      BOOLEAN isLocalConnection = FALSE ;

      NET_EH eh = _pCatCB->netWork()->getFrame()->getEventHandle( handle ) ;
      if ( eh.get() )
      {
         isLocalConnection = eh->isLocalConnection() ;
      }
      else
      {
         rc = SDB_NETWORK_CLOSE ;
         goto error ;
      }

      try
      {
         BSONObj query( pQuery ) ;
         BSONObj seletor( pSelector ) ;
         BSONObj groupInfo ;
         INT32 nodeID = CAT_INVALID_NODEID ;

         rc = rtnGetIntElement( query, FIELD_NAME_NODEID, nodeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_NODEID, rc ) ;

         rc = catGetGroupObj( (UINT16)nodeID, groupInfo, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group info by nodeid[%d], "
                      "rc: %d", nodeID, rc ) ;

         rc = _updateNodeToGrp( groupInfo, seletor, (UINT16)nodeID,
                                isLocalConnection, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update node to group, rc: %d",
                      rc ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occured exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_UPDATENODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_READCATACONF, "catNodeManager::readCataConf" )
   INT32 catNodeManager::readCataConf()
   {
      INT32 rc = SDB_OK;
      BOOLEAN isFileOpened = FALSE;
      SINT32 sBufferBegin = 0;
      INT32 iReadReturn = SDB_OK;

      const CHAR *szCatFilePath = pmdGetOptionCB()->getCatFile() ;
      CHAR szBuffer[ READ_BUFFER_SIZE + 1 ] = { 0 } ;
      OSSFILE catFile;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_READCATACONF ) ;
      PD_LOG ( PDINFO, "read catalogue-nodes info from configure file" );

      /// File not exist
      rc = ossAccess( szCatFilePath ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = ossOpen( szCatFilePath, OSS_READONLY, 0, catFile );
      if ( rc )
      {
         PD_LOG( PDINFO, "Failed to open the catalog-configure-file(rc=%d)",
                 rc ) ;
         goto error ;
      }

      isFileOpened = TRUE;

      while( iReadReturn != SDB_EOF )
      {
         SINT64 sReadSize = 0;
         iReadReturn = ossRead ( &catFile, szBuffer + sBufferBegin,
                                 READ_BUFFER_SIZE - sBufferBegin,
                                 &sReadSize ) ;
         if ( iReadReturn != SDB_OK && iReadReturn != SDB_EOF &&
              iReadReturn != SDB_INTERRUPT )
         {
            rc = iReadReturn;
            PD_LOG( PDERROR, "read catalogue-configure-file error: %s, rc: %d",
                    szCatFilePath, rc ) ;
            goto error;
         }
         SINT64 sContentLen = sBufferBegin + sReadSize;
         szBuffer[sContentLen] = '\0';

         SINT64 sParseBytes = 0;
         PD_TRACE1 ( SDB_CATNODEMGR_READCATACONF,
                     PD_PACK_RAW ( szBuffer, sContentLen ) ) ;
         rc = parseCatalogConf( szBuffer, sContentLen + 1, sParseBytes);
         if ( rc != SDB_OK )
         {
            PD_LOG( PDERROR, "parse catalogue-configure-file error: %s, rc: %d",
                    szCatFilePath, rc );
            goto error;
         }

         //get the end of the file, complete parsing
         if ( SDB_EOF == iReadReturn )
         {
            rc = SDB_OK;
            break;
         }

         //copy the incomplete content to the buffer-begine, then merge with
         //the content from file
         if ( sContentLen > sParseBytes )
         {
            ossMemcpy( szBuffer, szBuffer + sParseBytes,
                       sContentLen - sParseBytes );
            sBufferBegin = sContentLen - sParseBytes;
         }
         else
         {
            sBufferBegin = 0;
         }
      }//end of "while( iReadReturn != SDB_EOF)"
      if ( isFileOpened )
      {
         ossClose( catFile );
         isFileOpened = FALSE;
      }
      ossDelete( szCatFilePath );

   done:
      if ( isFileOpened )
      {
         ossClose( catFile );
         isFileOpened = FALSE;
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_READCATACONF, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PARSECATCONF, "catNodeManager::parseCatalogConf" )
   INT32 catNodeManager::parseCatalogConf( CHAR *pData,
                                           const SINT64 sDataSize,
                                           SINT64 &sParseBytes )
   {
      SINT64 sEnd = 0;
      SINT64 sBegin = 0;
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PARSECATCONF ) ;
      while ( sEnd < sDataSize )
      {
         //get a line
         if ( 0x0D != *(pData + sEnd) && 0x0A != *(pData + sEnd) &&
              '\0' != *(pData + sEnd) )
         {
            ++sEnd;
            continue;
         }
         *( pData + sEnd ) = '\0';

         BSONObj obj;
         rc = parseLine( pData+sBegin, obj );
         if ( rc != SDB_OK )
         {
            //an obj should on a line
            //if not then break
            break;
         }
         BSONObj boGroupInfo;
         rc = generateGroupInfo( obj, boGroupInfo );
         if ( rc != SDB_OK )
         {
            break;
         }
         PD_TRACE1 ( SDB_CATNODEMGR_PARSECATCONF,
                     PD_PACK_BSON ( boGroupInfo ) ) ;
         if ( 0 != boGroupInfo.nFields() )
         {
            rc = saveGroupInfo( boGroupInfo, 1 );
         }
         if ( rc != SDB_OK )
         {
            break;
         }
         sParseBytes = sEnd;

         sBegin = sEnd + 1;
         ++sEnd;
      }//end of while ( sEnd < sDataSize )
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_PARSECATCONF, rc ) ;
      return rc;
   }//end of parseCatalogConf()

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_SAVEGRPINFO, "catNodeManager::saveGroupInfo" )
   INT32 catNodeManager::saveGroupInfo ( BSONObj &boGroupInfo,
                                         INT16 w )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_SAVEGRPINFO ) ;
      BSONElement beGrpId = boGroupInfo.getField( FIELD_NAME_GROUPID );
      BSONObjBuilder bobMatcher;
      BSONObjBuilder bobUpdater;
      BSONObj boMatcher;
      BSONObj boUpdater;
      BSONObj boHint ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      PD_CHECK( beGrpId.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to get the field(%s), save group-info failed",
                FIELD_NAME_GROUPID ) ;
      bobMatcher.append( beGrpId ) ;
      boMatcher = bobMatcher.obj() ;

      bobUpdater.append("$set", boGroupInfo );
      boUpdater = bobUpdater.obj();
      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, boMatcher, boUpdater,
                      boHint, FLG_UPDATE_UPSERT, cb, _pDmsCB,
                      _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to update the group(%d) info",
                   beGrpId.number() ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_SAVEGRPINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_GENGROUPINFO, "catNodeManager::generateGroupInfo" )
   INT32 catNodeManager::generateGroupInfo( BSONObj &boConf,
                                            BSONObj &boGroupInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_GENGROUPINFO ) ;
      try
      {
         if ( 0 == boConf.nFields() )
         {
            //it is null-obj(comment-line), return ok
            goto done;
         }
         BSONObjBuilder bobGroupInfo;
         bobGroupInfo.append( FIELD_NAME_GROUPNAME, CATALOG_GROUPNAME );
         bobGroupInfo.append( FIELD_NAME_GROUPID, CATALOG_GROUPID );
         bobGroupInfo.append( FIELD_NAME_ROLE, SDB_ROLE_CATALOG );
         bobGroupInfo.append( FIELD_NAME_VERSION, CAT_VERSION_BEGIN );
         bobGroupInfo.append( FIELD_NAME_GROUP_STATUS, SDB_CAT_GRP_ACTIVE );
         bobGroupInfo.append( FIELD_NAME_SECRETID, (INT32)ossRand() ) ;

         BSONObjBuilder bobNodeInfo;
         rc = _getNodeInfoByConf( boConf, bobNodeInfo );
         PD_RC_CHECK(rc, PDERROR,
                     "failed to get catalog-node info(rc=%d)", rc );
         UINT16 nodeID = _pCatCB->allocSystemNodeID() ;
         PD_TRACE1 ( SDB_CATNODEMGR_GENGROUPINFO,
                     PD_PACK_USHORT ( nodeID ) ) ;
         PD_CHECK( nodeID!=CAT_INVALID_NODEID, SDB_SYS, error,
                  PDERROR, "failed to allocate nodeId" );
         bobNodeInfo.append( FIELD_NAME_NODEID, nodeID );
         bobNodeInfo.append( FIELD_NAME_STATUS, (INT32)SDB_CAT_GRP_ACTIVE ) ;
         BSONObj boNodeInfo = bobNodeInfo.obj();
         BSONArrayBuilder babGroup;
         babGroup.append( boNodeInfo );
         bobGroupInfo.appendArray( FIELD_NAME_GROUP, babGroup.arr() ) ;
         boGroupInfo = bobGroupInfo.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Unexpected exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_GENGROUPINFO, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_GETNODEINFOBYCONF, "catNodeManager::_getNodeInfoByConf" )
   INT32 catNodeManager::_getNodeInfoByConf( BSONObj &boConf,
                                             BSONObjBuilder &bobNodeInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_GETNODEINFOBYCONF ) ;
      PD_TRACE1 ( SDB_CATNODEMGR_GETNODEINFOBYCONF,
                  PD_PACK_BSON ( boConf ) ) ;
      do
      {
         try
         {
            BSONArrayBuilder babSvcArray;
            // get service name
            BSONElement beLocalSvc = boConf.getField( PMD_OPTION_SVCNAME );
            if ( beLocalSvc.eoo() || beLocalSvc.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        PMD_OPTION_SVCNAME );
               break;
            }
            BSONObjBuilder bobLocalSvc;
            // append the local service
            bobLocalSvc.append( FIELD_NAME_SERVICE_TYPE, MSG_ROUTE_LOCAL_SERVICE );
            bobLocalSvc.appendAs( beLocalSvc, FIELD_NAME_NAME );
            babSvcArray.append( bobLocalSvc.obj() );

            UINT16 svcPort = 0;
            std::string strSvc = beLocalSvc.str();
            ossSocket::getPort ( strSvc.c_str(), svcPort ) ;

            BSONObjBuilder bobReplSvc;
            // append the replica service
            bobReplSvc.append( FIELD_NAME_SERVICE_TYPE, MSG_ROUTE_REPL_SERVICE );
            BSONElement beReplSvc = boConf.getField( PMD_OPTION_REPLNAME );
            if ( !beReplSvc.eoo() && beReplSvc.type()==String )
            {
               bobReplSvc.appendAs( beReplSvc, FIELD_NAME_NAME );
            }
            else if ( svcPort != 0 )
            {
               CHAR szPort[CAT_PORT_STR_SZ] = {0};
               UINT16 port = svcPort + MSG_ROUTE_REPL_SERVICE;
               ossItoa( port, &szPort[0], CAT_PORT_STR_SZ );
               bobReplSvc.append( FIELD_NAME_NAME, szPort );
            }
            else
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        PMD_OPTION_REPLNAME );
               break;
            }
            babSvcArray.append( bobReplSvc.obj() );

            BSONObjBuilder bobShardSvc;
            // append shard service
            bobShardSvc.append( FIELD_NAME_SERVICE_TYPE, MSG_ROUTE_SHARD_SERVCIE );
            BSONElement beShardSvc = boConf.getField( PMD_OPTION_SHARDNAME );
            if ( !beShardSvc.eoo() && beShardSvc.type()==String )
            {
               bobShardSvc.appendAs( beShardSvc, FIELD_NAME_NAME );
            }
            else if ( svcPort != 0 )
            {
               CHAR szPort[CAT_PORT_STR_SZ] = {0};
               UINT16 port = svcPort + MSG_ROUTE_SHARD_SERVCIE;
               ossItoa( port, &szPort[0], CAT_PORT_STR_SZ );
               bobShardSvc.append( FIELD_NAME_NAME, szPort );
            }
            else
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        PMD_OPTION_SHARDNAME );
               break;
            }
            babSvcArray.append( bobShardSvc.obj() );

            BSONObjBuilder bobCataSvc;
            // append catalog service
            bobCataSvc.append( FIELD_NAME_SERVICE_TYPE, MSG_ROUTE_CAT_SERVICE );
            BSONElement beCataSvc = boConf.getField( PMD_OPTION_CATANAME );
            if ( !beCataSvc.eoo() && beCataSvc.type() == String &&
                 NULL != beCataSvc.valuestr() &&
                 ossStrcmp( beCataSvc.valuestr(), "" ) != 0 )
            {
               bobCataSvc.appendAs( beCataSvc, FIELD_NAME_NAME );
            }
            else if ( svcPort != 0 )
            {
               CHAR szPort[CAT_PORT_STR_SZ] = {0};
               UINT16 port = svcPort + MSG_ROUTE_CAT_SERVICE;
               ossItoa( port, &szPort[0], CAT_PORT_STR_SZ );
               bobCataSvc.append( FIELD_NAME_NAME, szPort );
            }
            else
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        PMD_OPTION_CATANAME );
               break;
            }
            babSvcArray.append( bobCataSvc.obj() );

            // get hostname
            BSONElement beHostName = boConf.getField( FIELD_NAME_HOST );
            if ( beHostName.eoo() || beHostName.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        FIELD_NAME_HOST );
               break;
            }
            // get database path
            BSONElement beDBPath = boConf.getField( PMD_OPTION_DBPATH );
            if ( beDBPath.eoo() || beDBPath.type()!=String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR,
                        "failed to get the field(%s)",
                        PMD_OPTION_DBPATH );
               break;
            }
            bobNodeInfo.append( beDBPath );
            bobNodeInfo.append( beHostName );
            bobNodeInfo.appendArray( FIELD_NAME_SERVICE, babSvcArray.arr() );
         }
         catch( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR,
                  "occured unexpected error:%s",
                  e.what() );
            break;
         }
      }while ( FALSE );
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_GETNODEINFOBYCONF, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PARSELINE, "catNodeManager::parseLine" )
   INT32 catNodeManager::parseLine( const CHAR *pLine, BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT ( pLine, "line can't be NULL" ) ;
      const CHAR *pBegin = pLine;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PARSELINE ) ;
      while ( '\0' != *pBegin )
      {
         //skip all space and tab
         if ( *pBegin != '\t' && *pBegin !=' ' )
         {
            break;
         }
         ++pBegin;
      }
      if ( '#' != *pBegin )
      {
         rc = fromjson ( pBegin, obj ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to parse line into BSONObj: %s",
                     pBegin ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_PARSELINE, rc ) ;
      return rc;
   error :
      goto done ;
   }//end of parseLine()

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_PARSEIDINFO, "catNodeManager::parseIDInfo" )
   INT32 catNodeManager::parseIDInfo( BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_PARSEIDINFO ) ;
      PD_TRACE1 ( SDB_CATNODEMGR_PARSEIDINFO,
                  PD_PACK_BSON ( obj ) ) ;
      try
      {
         MsgRouteID routeID;
         BOOLEAN isGrpActive = FALSE;
         //get group id
         BSONElement beGrpID = obj.getField ( CAT_GROUPID_NAME ) ;
         BSONElement beGrpName = obj.getField( CAT_GROUPNAME_NAME ) ;
         if ( beGrpID.eoo() || !beGrpID.isNumber() )
         {
            PD_LOG( PDWARNING, "Failed to get the field(%s)",
                    CAT_GROUPID_NAME );
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( beGrpName.eoo() || String != beGrpName.type() )
         {
            PD_LOG( PDWARNING, "Failed to get the field(%s)",
                    CAT_GROUPNAME_NAME );
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            // get group status
            BSONElement beGrpStatus = obj.getField ( CAT_GROUP_STATUS ) ;
            // if it's doesn't exist, or it's not number, or it's
            // deactivated, let's set isGrpActive = FALSE
            if ( !beGrpStatus.isNumber() ||
                 SDB_CAT_GRP_ACTIVE == beGrpStatus.numberInt() )
            {
               isGrpActive = TRUE ;
            }
            _pCatCB->insertGroupID( beGrpID.numberInt(),
                                    beGrpName.valuestr(),
                                    isGrpActive );
            routeID.columns.groupID = beGrpID.numberInt();

            //get node id
            BSONElement beNodes = obj.getField ( CAT_GROUP_NAME );
            if ( beNodes.eoo() || beNodes.type() != Array )
            {
               PD_LOG( PDINFO, "Failed to get the field(%s), usually it "
                       "means one or more replica groups are empty",
                       CAT_GROUP_NAME );
               //the deactive have not nodes-info
               rc = SDB_OK ;
               goto done ;
            }
            // for each elements in the group
            BSONObjIterator i( beNodes.embeddedObject() );
            while ( i.more() )
            {
               BSONElement beTmp = i.next();

               BSONObj boTmp = beTmp.embeddedObject();
               BSONElement beNodeID = boTmp.getField( CAT_NODEID_NAME );
               BSONElement beHost = boTmp.getField( CAT_HOST_FIELD_NAME );
               BSONElement beService = boTmp.getField( CAT_SERVICE_FIELD_NAME );
               // get node id
               if ( beNodeID.eoo() || ! beNodeID.isNumber() )
               {
                  PD_LOG( PDWARNING, "Failed to get the field(%s)",
                          CAT_NODEID_NAME );
                  rc = SDB_INVALIDARG;
                  goto error ;
               }
               _pCatCB->insertNodeID( beNodeID.numberInt() ) ;
               routeID.columns.nodeID = beNodeID.numberInt() ;

               // get host name
               if ( beHost.eoo() || beHost.type() != String )
               {
                  PD_LOG( PDWARNING, "Failed to get the field(%s)",
                          CAT_HOST_FIELD_NAME );
                  rc = SDB_INVALIDARG;
                  goto error ;
               }

               // get service name
               if ( beService.eoo() || beService.type() != Array )
               {
                  PD_LOG( PDWARNING, "Failed to get the field(%s)",
                          CAT_SERVICE_FIELD_NAME );
                  rc = SDB_INVALIDARG;
                  goto error ;
               }
               // loop for each service
               {
                  BSONObjIterator j( beService.embeddedObject() );
                  while ( j.more() )
                  {
                     BSONElement beServiceTmp = j.next();
                     BSONObj boServiceTmp = beServiceTmp.embeddedObject();
                     BSONElement beServiceType = boServiceTmp.getField(
                           CAT_SERVICE_TYPE_FIELD_NAME );
                     BSONElement beServiceName = boServiceTmp.getField(
                           CAT_SERVICE_NAME_FIELD_NAME );
                     // make sure type exists
                     if ( beServiceType.eoo() || !beServiceType.isNumber() )
                     {
                        PD_LOG( PDWARNING, "Failed to get the field(%s)",
                                CAT_NODEID_NAME );
                        rc = SDB_INVALIDARG;
                        goto error ;
                     }
                     routeID.columns.serviceID = beServiceType.numberInt();

                     // make sure the service name exists
                     if ( beServiceName.eoo() ||
                          String != beServiceName.type() )
                     {
                        PD_LOG( PDWARNING, "Failed to get the field(%s)",
                                CAT_NODEID_NAME );
                        rc = SDB_INVALIDARG;
                        goto error ;
                     }
                     //add route info to network
                     rc = _pCatCB->netWork()->updateRoute( routeID,
                           beHost.String().c_str(),
                           beServiceName.String().c_str() ) ;
                     if ( rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
                     {
                        PD_LOG( PDWARNING, "Failed to update route(rc = %d)",
                                rc );
                        goto error ;
                     }
                     rc = SDB_OK;
                  }//end of while ( j.moreWithEOO() )
               }
            }//end of while ( i.moreWithEOO() )
         }
      } // try
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_PARSEIDINFO, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_GETNODEINFO, "catNodeManager::getNodeInfo" )
   INT32 catNodeManager::getNodeInfo( const BSONObj &boReq,
                                      BSONObj &boNodeInfo,
                                      INT32 &role )
   {
      INT32 rc                         = SDB_OK;
      SINT64 sContextID                = -1;
      BSONObj boMatcher ;
      BSONObj boOrderBy;
      BSONObj boHint;
      CHAR szBuf[ OP_MAXNAMELENGTH+1 ] = {0};

      rtnContextBuf buffObj ;
      BOOLEAN found                    = FALSE ;
      const CHAR *strShardServiceName  = NULL ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_GETNODEINFO ) ;
      PD_TRACE1 ( SDB_CATNODEMGR_GETNODEINFO,
                  PD_PACK_BSON ( boReq ) ) ;
      try
      {
         BSONObj boSelector;

         BSONElement beHostName = boReq.getField( CAT_HOST_FIELD_NAME);
         if ( beHostName.type()!=String )
         {
            PD_LOG ( PDERROR, "Failed to get the field: %s",
                     CAT_HOST_FIELD_NAME );
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         BSONElement beServiceName = boReq.getField( CAT_SERVICE_FIELD_NAME );
         if ( beServiceName.type()!=Array )
         {
            PD_LOG( PDERROR, "Failed to get the field: %s",
                    CAT_SERVICE_FIELD_NAME );
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         strShardServiceName = getShardServiceName ( beServiceName ) ;
         if ( !strShardServiceName || strShardServiceName[0] == '\0' )
         {
            PD_LOG( PDERROR, "Failed to get the shard service name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // Hostname is case-insensitive, we need to use $regex to match
         // Hostname contains 'a-z', '0-9', '-' and '.', we need to replace
         // '.' with '\\.' in PCRE.
         StringBuilder regHostName ;
         string hostName = beHostName.valuestr() ;
         boost::replace_all( hostName, ".", "\\." ) ;
         regHostName << "^" << hostName << "$" ;
         BSONElement beHostIP = boReq.getField( CAT_IP_FIELD_NAME );
         if ( beHostIP.type() == Array )
         {
            BSONArrayBuilder hostNameArrayBuilder ;
            hostNameArrayBuilder.append( BSON( CAT_MATCHER_HOST_NAME <<
                                               BSON( "$regex" << regHostName.str() <<
                                                     "$options" << "i" ) ) ) ;
            BSONObjIterator iter( beHostIP.embeddedObject() );
            while ( iter.more() )
            {
               BSONElement ip = iter.next();
               if (ip.type() != String)
               {
                  continue;
               }
               hostNameArrayBuilder.append( BSON( CAT_MATCHER_HOST_NAME <<
                                                  ip.valuestr() ) ) ;
            }
            //{Group:{$elemMatch:{ $or:[
            //           {HostName:{$regex:'^*****$', $options:'i'}},
            //           {HostName:*ip*},***],
            //           Service.Name:***}}}
            boMatcher = BSON( FIELD_NAME_GROUP
                              << BSON("$elemMatch"
                                    << BSON( "$or" << hostNameArrayBuilder.arr()
                                       << CAT_MATCHER_SERVICE_NAME
                                       << strShardServiceName ) ) );
         }
         else
         {
            //{Group:{$elemMatch:{
            //           HostName:{$regex:'^*****$', $options:'i'},
            //           Service.Name:****}}}
            boMatcher = BSON( FIELD_NAME_GROUP
                              << BSON("$elemMatch"
                                    << BSON( CAT_MATCHER_HOST_NAME <<
                                             BSON( "$regex" << regHostName.str() <<
                                                   "$options" << "i" )
                                          << CAT_MATCHER_SERVICE_NAME
                                          << strShardServiceName ) ) ) ;
         }

         // copy collection name into buffer
         ossStrncpy( szBuf, CAT_NODE_INFO_COLLECTION, OP_MAXNAMELENGTH ) ;
         // perform query
         rc = rtnQuery ( szBuf, boSelector, boMatcher,
                         boOrderBy, boHint, 0, _pEduCB, 0, 1, _pDmsCB,
                         _pRtnCB, sContextID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to query from collection %s, rc = %d",
                     CAT_NODE_INFO_COLLECTION, rc ) ;
            goto error ;
         }
         // we only call GetMore ONCE, so we have to manually destroy
         // context id
         rc = rtnGetMore( sContextID, 1, buffObj, _pEduCB, _pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG ( PDERROR, "Failed to get more from collection %s, "
                        "rc = %d", CAT_NODE_INFO_COLLECTION, rc ) ;
            }
            else
            {
               // if we cannot find the given host/service, let's return node
               // not found
               rc = SDB_CLS_NODE_NOT_EXIST ;
            }
            goto error ;
         }

         // when we read at least one record, let's extract the record
         {
            BSONObj boGrpInfo ( buffObj.data() ) ;
            PD_TRACE1 ( SDB_CATNODEMGR_GETNODEINFO,
                        PD_PACK_BSON ( boGrpInfo ) ) ;
            // first let's get all elements in the group
            BSONElement beGroup = boGrpInfo.getField( CAT_GROUP_NAME );
            // make sure it exists and array type
            if ( beGroup.type() == Array )
            {
               BSONObjIterator i( beGroup.embeddedObject() ) ;
               BSONObj boTmp ;
               // loop for each element in group
               while( i.more() )
               {
                  const CHAR *strShardServiceNameTmp = NULL ;
                  BSONElement beTmp = i.next();
                  if ( beTmp.type() != Object )
                  {
                     rc = SDB_CAT_CORRUPTION ;
                     PD_LOG ( PDERROR, "Field is not Object: %s",
                              beTmp.toString().c_str() ) ;
                     goto error ;
                  }
                  {
                     boTmp = beTmp.embeddedObject();
                     // get hostname
                     BSONElement beHostNameTmp =
                        boTmp.getField( CAT_HOST_FIELD_NAME );
                     BSONElement beServiceTmp =
                        boTmp.getField( CAT_SERVICE_FIELD_NAME );
                     // make sure host name exists
                     if ( beHostNameTmp.type() != String )
                     {
                        // if we do not have such field, catalog is corrupted
                        rc = SDB_CAT_CORRUPTION ;
                        PD_LOG( PDERROR, "Failed to get the field: %s",
                                CAT_HOST_FIELD_NAME ) ;
                        goto error ;
                     }
                     // if it's not the host we want to find, let's continue the
                     // loop
                     if ( ossStrcasecmp ( beHostNameTmp.valuestr(),
                                          beHostName.valuestr() ) != 0 )
                     {
                        if ( beHostIP.type() != Array )
                        {
                           continue;
                        }

                        // check if it's IP
                        BOOLEAN isIP = FALSE;
                        BSONObjIterator iter( beHostIP.embeddedObject() );
                        while ( iter.more() )
                        {
                           BSONElement ip = iter.next();
                           if ( ip.type() != String )
                           {
                              continue ;
                           }
                           if (ossStrcmp ( beHostNameTmp.valuestr(),
                                           ip.valuestr() ) == 0 )
                           {
                              isIP = TRUE ;
                              break;
                           }
                        }

                        if ( !isIP )
                        {
                           continue ;
                        }
                     }
                     // make sure the service name is also array
                     if ( beServiceTmp.type() != Array )
                     {
                        rc = SDB_CAT_CORRUPTION ;
                        PD_LOG( PDERROR, "Failed to get the field: %s",
                                CAT_SERVICE_FIELD_NAME );
                        goto error ;
                     }
                     strShardServiceNameTmp = getShardServiceName(beServiceTmp);
                     if ( !strShardServiceNameTmp ||
                          strShardServiceNameTmp[0] == '\0' )
                     {
                        rc = SDB_CAT_CORRUPTION ;
                        PD_LOG( PDERROR,
                                "Failed to get the shard service name" );
                        goto error ;
                     }
                     // if both hostname + service name matches, let's mark
                     // Found
                     if ( ossStrcmp ( strShardServiceName,
                                      strShardServiceNameTmp ) == 0 )
                     {
                        found = TRUE ;
                        break ;
                     }
                  }
               } // while ( i.more() )

               SDB_ASSERT( found, "Found must be TRUE" ) ;
               /// got the node-info
               if ( found )
               {
                  //build response
                  BSONObjBuilder bobNodeInfo;
                  bobNodeInfo.append( boGrpInfo.getField(CAT_GROUPID_NAME) ) ;
                  bobNodeInfo.append( boGrpInfo.getField(CAT_GROUPNAME_NAME) ) ;
                  bobNodeInfo.append( boGrpInfo.getField(CAT_ROLE_NAME) ) ;
                  bobNodeInfo.appendElements( boTmp ) ;
                  boNodeInfo = bobNodeInfo.obj() ;
                  role = boGrpInfo.getField(CAT_ROLE_NAME).numberInt() ;
               }
               else
               {
                  PD_LOG( PDERROR, "Unknow system error" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( !beGroup.eoo() && beGroup.type()==Array )
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_CAT_CORRUPTION ;
      }

   done :
      // make sure to delete context since i didn't loop until EOC
      if ( sContextID != -1 )
      {
         _pRtnCB->contextDelete ( sContextID, _pEduCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_GETNODEINFO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATNODEMGR_CREATEGRP, "catNodeManager::_createGrp" )
   INT32 catNodeManager::_createGrp( const CHAR *groupName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATNODEMGR_CREATEGRP ) ;

      BOOLEAN bExist = FALSE ;
      UINT32  newGroupID = CAT_INVALID_GROUPID ;
      INT32   role = SDB_ROLE_DATA ;
      INT32   status = SDB_CAT_GRP_DEACTIVE ;

      catCtxLockMgr lockMgr ;

      // check name is valid
      if ( 0 == ossStrcmp( groupName, COORD_GROUPNAME ) )
      {
         newGroupID = COORD_GROUPID ;
         role = SDB_ROLE_COORD ;
         status = SDB_CAT_GRP_ACTIVE ;
      }
      else if ( 0 == ossStrcmp( groupName, SPARE_GROUPNAME ) )
      {
         newGroupID = SPARE_GROUPID ;
         role = SDB_ROLE_DATA ;
         status = SDB_CAT_GRP_ACTIVE ;
      }
      else
      {
         rc = catGroupNameValidate( groupName, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Group name[%s] is invalid", groupName ) ;
      }

      // Lock group
      PD_CHECK( lockMgr.tryLockGroup( groupName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock group[%s]",
                groupName ) ;

      // check whether the group is exist or not
      rc = catGroupCheck( groupName, bExist, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check group name[%s] exist failed, rc: %d",
                   groupName, rc ) ;
      PD_CHECK( FALSE == bExist, SDB_CAT_GRP_EXIST, error, PDERROR,
                "Create group failed, the group[%s] existed", groupName ) ;

      // assign group id
      if ( CAT_INVALID_GROUPID == newGroupID )
      {
         newGroupID = _pCatCB->allocGroupID() ;
         PD_CHECK( CAT_INVALID_GROUPID != newGroupID, SDB_SYS, error, PDERROR,
                   "Failed to assign group id, maybe group if full" ) ;
      }

      PD_TRACE1 ( SDB_CATNODEMGR_CREATEGRP, PD_PACK_UINT ( newGroupID ) ) ;

      // construct group object and insert to collection
      try
      {
         BSONObjBuilder bobGroupInfo ;
         bobGroupInfo.append( CAT_GROUPNAME_NAME, groupName ) ;
         bobGroupInfo.append( CAT_GROUPID_NAME, newGroupID ) ;
         bobGroupInfo.append( CAT_ROLE_NAME, role ) ;
         bobGroupInfo.append( CAT_VERSION_NAME, CAT_VERSION_BEGIN ) ;
         bobGroupInfo.append( CAT_GROUP_STATUS, status ) ;
         bobGroupInfo.append( FIELD_NAME_SECRETID, (INT32)ossRand() ) ;
         BSONObjBuilder sub( bobGroupInfo.subarrayStart( CAT_GROUP_NAME ) ) ;
         sub.done() ;
         BSONObj boGroupInfo = bobGroupInfo.obj() ;

         rc = rtnInsert( CAT_NODE_INFO_COLLECTION, boGroupInfo, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert group info[%s] to "
                      "collection, rc: %d", boGroupInfo.toString().c_str(),
                      rc ) ;
         _pCatCB->insertGroupID( newGroupID, groupName, FALSE ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Occured exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATNODEMGR_CREATEGRP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 catNodeManager::_updateNodeToGrp ( BSONObj &boGroupInfo,
                                            const BSONObj &boNodeInfoNew,
                                            UINT16 nodeID,
                                            BOOLEAN isLoalConn,
                                            BOOLEAN setStatus,
                                            BOOLEAN keepInstanceID )
   {
      INT32 rc = SDB_OK ;
      INT32 nodeRole = SDB_ROLE_DATA ;
      const CHAR *groupName = NULL ;
      const CHAR *hostName = NULL ;
      BSONObj oldInfoObj ;
      BSONObj newInfoObj ;
      BSONArrayBuilder newGroupsBuild ;
      BOOLEAN incVer = FALSE ;

      BSONObjBuilder updateBuilder ;
      BSONObj updator, matcher ;
      BSONObj dummyObj ;

      rc = rtnGetIntElement( boGroupInfo, CAT_ROLE_NAME, nodeRole ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   CAT_ROLE_NAME, rc ) ;

      rc = rtnGetStringElement( boGroupInfo, FIELD_NAME_GROUPNAME, &groupName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_GROUPNAME, rc ) ;

      // get removed node
      {
         BSONElement beGroups = boGroupInfo.getField( FIELD_NAME_GROUP ) ;
         if ( beGroups.eoo() || beGroups.type() != Array )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Field[%s] type error, groupInfo: %s",
                    FIELD_NAME_GROUP, boGroupInfo.toString().c_str() ) ;
            goto error ;
         }
         rc = _getRemovedGroupsObj( beGroups.embeddedObject(), nodeID,
                                    oldInfoObj, newGroupsBuild ) ;
         PD_RC_CHECK( rc, PDERROR, "Remove the node[%d] from group info "
                      "failed, rc: %d, groupInfo: %s", nodeID, rc,
                      boGroupInfo.toString().c_str() ) ;
      }

      // get host name
      rc = rtnGetStringElement( oldInfoObj, FIELD_NAME_HOST, &hostName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_HOST, rc ) ;

      // check if 'localhost' or '127.0.0.1' is used
      if ( 0 == ossStrcmp( hostName, OSS_LOCALHOST ) ||
           0 == ossStrcmp( hostName, OSS_LOOPBACK_IP ) )
      {
         // add localhost, coord must be the same with catalog
         if ( !isLoalConn )
         {
            rc = SDB_CAT_NOT_LOCALCONN ;
            goto error ;
         }
      }

      // merge new and old
      {
         BOOLEAN modifyNum = 0 ;
         BSONObjBuilder mergeBuild ;
         mergeBuild.append( FIELD_NAME_HOST, hostName ) ;

         BSONElement beStatus = oldInfoObj.getField( FIELD_NAME_STATUS ) ;
         if ( setStatus && beStatus.numberInt() != SDB_CAT_GRP_ACTIVE )
         {
            mergeBuild.append( FIELD_NAME_STATUS, (INT32)SDB_CAT_GRP_ACTIVE ) ;
            if ( !beStatus.eoo() )
            {
               incVer = TRUE ;
            }
            ++modifyNum ;
         }
         else
         {
            mergeBuild.append( beStatus ) ;
         }

         BSONObjIterator itr( oldInfoObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_HOST ) ||
                 0 == ossStrcmp( e.fieldName(), FIELD_NAME_NODEID ) ||
                 0 == ossStrcmp( e.fieldName(), FIELD_NAME_STATUS ) )
            {
               continue ;
            }
            else if ( !keepInstanceID &&
                      0 == ossStrcmp( e.fieldName(), PMD_OPTION_INSTANCE_ID ) )
            {
               // Instance ID is removed
               ++modifyNum ;
               continue ;
            }

            BSONElement newEle = boNodeInfoNew.getField( e.fieldName() ) ;
            if ( !newEle.eoo() && 0 != newEle.woCompare( e, false ) )
            {
               mergeBuild.append( newEle ) ;
               ++modifyNum ;
               if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_SERVICE ) )
               {
                  incVer = TRUE ;
               }
            }
            else
            {
               mergeBuild.append( e ) ;
            }
         }

         // Set new instance ID
         if ( !oldInfoObj.hasElement( PMD_OPTION_INSTANCE_ID ) &&
              boNodeInfoNew.hasElement( PMD_OPTION_INSTANCE_ID ) )
         {
            mergeBuild.append( boNodeInfoNew.getField( PMD_OPTION_INSTANCE_ID ) ) ;
            ++modifyNum ;
         }

         mergeBuild.append( FIELD_NAME_NODEID, nodeID ) ;
         newInfoObj = mergeBuild.obj() ;

         /// nothing change
         if ( 0 == modifyNum )
         {
            goto done ;
         }
      }

      // append new node info
      newGroupsBuild.append( newInfoObj ) ;

      // update group info
      if ( incVer )
      {
         updateBuilder.append("$inc", BSON( FIELD_NAME_VERSION << 1 ) ) ;
      }
      updateBuilder.append("$set", BSON( FIELD_NAME_GROUP <<
                            newGroupsBuild.arr() ) ) ;

      updator = updateBuilder.obj() ;
      matcher = BSON( FIELD_NAME_GROUPNAME << groupName ) ;

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, matcher, updator, dummyObj,
                      0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update group info[%s], matcher: %s, "
                   "updator: %s, rc: %d", boGroupInfo.toString().c_str(),
                   matcher.toString().c_str(), updator.toString().c_str(),
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catNodeManager::_getRemovedGroupsObj( const BSONObj & srcGroupsObj,
                                               UINT16 removeNode,
                                               BSONObj &removedObj,
                                               BSONArrayBuilder &newObjBuilder )
   {
      INT32 rc = SDB_OK ;
      INT32 nodeID = CAT_INVALID_NODEID ;
      BOOLEAN exist = FALSE ;

      BSONObjIterator i( srcGroupsObj ) ;
      while ( i.more() )
      {
         BSONElement beNode = i.next() ;
         BSONObj boNode = beNode.embeddedObject() ;

         rc = rtnGetIntElement( boNode, FIELD_NAME_NODEID, nodeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] from [%s], rc: %d",
                      FIELD_NAME_NODEID, boNode.toString().c_str(), rc ) ;

         if ( removeNode != (UINT16)nodeID )
         {
            newObjBuilder.append( boNode ) ;
         }
         else
         {
            exist = TRUE ;
            removedObj = boNode.getOwned() ;
         }
      }

      PD_CHECK( exist, SDB_CLS_NODE_NOT_EXIST, error, PDERROR,
                "Remove node[%d] is not exist in group[%s]", removeNode,
                srcGroupsObj.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT16 catNodeManager::_majoritySize()
   {
      return _pCatCB->majoritySize() ;
   }
}

