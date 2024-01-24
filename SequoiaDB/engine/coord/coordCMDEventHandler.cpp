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

   Source File Name = coordCMDEventHandler.cpp

   Descriptive Name = Coord Command Event Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCMDEventHandler.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordCommandWithLocation.hpp"
#include "coordCommandRecycleBin.hpp"
#include "coordCommandData.hpp"
#include "msgReplicator.hpp"
#include "coordUtil.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordDataCMDHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_DROPCL, "_coordDataCMDHelper::dropCL" )
   INT32 _coordDataCMDHelper::dropCL( coordResource *resource,
                                      const CHAR *clName,
                                      BOOLEAN skipRecycleBin,
                                      BOOLEAN ignoreLock,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_DROPCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDDropCollection cmdDropCL ;

      rc = cmdDropCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop cl command:rc=%d", rc ) ;

      rc = msgBuildDropCLMsg( &pMsg, &buffSize, clName, skipRecycleBin,
                              ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop cl request:"
                   "cl=%s,rc=%d", clName, rc ) ;

      rc = cmdDropCL.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop cl(%s):rc=%d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_DROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_TRUNCCL, "_coordDataCMDHelper::truncateCL" )
   INT32 _coordDataCMDHelper::truncateCL( coordResource *resource,
                                          const CHAR *clName,
                                          BOOLEAN skipRecycleBin,
                                          BOOLEAN ignoreLock,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_TRUNCCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDTruncate cmdTruncateCL ;

      rc = cmdTruncateCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init truncate collection "
                   "command, rc: %d", rc ) ;

      rc = msgBuildTruncateCLMsg( &pMsg, &buffSize, clName, skipRecycleBin,
                                  ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build truncate collection "
                   "request [%s], rc: %d", clName, rc ) ;

      rc = cmdTruncateCL.execute( (MsgHeader *)pMsg, cb, contextID,
                                  &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s], rc: %d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_TRUNCCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_ALTERCL, "_coordDataCMDHelper::alterCL" )
   INT32 _coordDataCMDHelper::alterCL( coordResource *resource,
                                       const CHAR *clName,
                                       const BSONObj &options,
                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_ALTERCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDAlterCollection cmdAlterCL ;

      rc = cmdAlterCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init alter collection "
                   "command, rc: %d", rc ) ;

      rc = msgBuildAlterCLMsg( &pMsg, &buffSize, clName, options, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build alter collection "
                   "request [%s], rc: %d", clName, rc ) ;

      rc = cmdAlterCL.execute( (MsgHeader *)pMsg, cb, contextID,
                               &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s], rc: %d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_ALTERCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_DROPCS, "_coordDataCMDHelper::dropCS" )
   INT32 _coordDataCMDHelper::dropCS( coordResource *resource,
                                      const CHAR *csName,
                                      BOOLEAN skipRecycleBin,
                                      BOOLEAN ignoreLock,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_DROPCS ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      _coordCMDDropCollectionSpace cmdDropCS ;

      rc = cmdDropCS.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop cs command:rc=%d", rc ) ;

      rc = msgBuildDropCSMsg( &pMsg, &buffSize, csName, skipRecycleBin,
                              ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop cs request:"
                   "cs=%s,rc=%d", csName, rc ) ;

      rc = cmdDropCS.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop cs(%s):rc=%d",
                   csName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_DROPCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordNodeCMDHelper implement
   */

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMDHELPER_NOTIFY2GROUPNODES, "_coordNodeCMDHelper::_notify2GroupNodes" )
   INT32 _coordNodeCMDHelper::_notify2GroupNodes( coordResource *pResource,
                                                  const CoordGroupInfoPtr &groupPtr,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES ) ;

      // Use netRouteAgent instead of pmdRemoteSession, because session will send message by shardsession.
      _netRouteAgent *pAgent = pResource->getRouteAgent() ;

      _MsgClsGInfoUpdated updated ;
      updated.groupID = groupPtr->groupID() ;
      MsgRouteID routeID ;
      UINT32 index = 0 ;

      // If the group is empty, do nothing
      if ( 0 == groupPtr->nodeCount() )
      {
         rc = SDB_CLS_EMPTY_GROUP ;
         PD_LOG( PDWARNING, "There is no node in group, groupID[%u], rc: %d",
                 groupPtr->groupID(), rc ) ;
         goto error ;
      }

      // Send msg to group nodes
      while ( SDB_OK == groupPtr->getNodeID( index++, routeID ) )
      {
         rcTmp = pAgent->syncSend( routeID, ( MsgHeader* )&updated ) ;
         rc = ( rcTmp && !rc ) ? rcTmp : rc ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GEOUPID, "_coordNodeCMDHelper::notify2GroupNodes" )
   INT32 _coordNodeCMDHelper::notify2GroupNodes( coordResource *pResource,
                                                 UINT32 groupID,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GEOUPID ) ;

      CoordGroupInfoPtr groupPtr ;

      // Get group info by groupID, store in groupPtr
      rc = pResource->updateGroupInfo( groupID, groupPtr, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to update group info, "
                   "groupID[%u], rc: %d", groupID, rc ) ;

      // Notify to group nodes
      rc = _notify2GroupNodes( pResource, groupPtr, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to notify group nodes, "
                   "groupID[%u], rc: %d", groupID, rc ) ;

      done:
         PD_TRACE_EXITRC ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GEOUPID, rc ) ;
         return rc ;

      error:
         goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GROUPNAME, "_coordNodeCMDHelper::notify2GroupNodes" )
   INT32 _coordNodeCMDHelper::notify2GroupNodes( coordResource *pResource,
                                                 const CHAR* groupName,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GROUPNAME ) ;

      CoordGroupInfoPtr groupPtr ;

      // Get group info by groupName, store in groupPtr
      rc = pResource->updateGroupInfo( groupName, groupPtr, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to update group info, "
                   "groupName[%s], rc: %d", groupName, rc ) ;

      // Notify to group nodes
      rc = _notify2GroupNodes( pResource, groupPtr, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to notify group nodes, "
                   "groupName[%s], rc: %d", groupName, rc ) ;

   done:
      PD_TRACE_EXITRC ( COORD_NODECMDHELPER_NOTIFY2GROUPNODES_BY_GROUPNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMDHELPER_NOTIFY2NODES_BY_GROUPS, "_coordNodeCMDHelper::notify2NodesByGroups" )
   INT32 _coordNodeCMDHelper::notify2NodesByGroups( coordResource *pResource,
                                                    const CoordGroupList &groupLst,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODECMDHELPER_NOTIFY2NODES_BY_GROUPS ) ;

      CoordGroupList::const_iterator itr = groupLst.begin() ;
      while ( groupLst.end() != itr )
      {
         notify2GroupNodes( pResource, ( itr++ )->second, cb ) ;
      }

      PD_TRACE_EXITRC ( COORD_NODECMDHELPER_NOTIFY2NODES_BY_GROUPS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMDHELPER_NOTIFY2ALLNODES, "_coordNodeCMDHelper::notify2AllNodes" )
   INT32 _coordNodeCMDHelper::notify2AllNodes( coordResource *pResource,
                                               BOOLEAN exceptSelf,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODECMDHELPER_NOTIFY2ALLNODES ) ;

      MsgHeader ntyMsg ;
      CoordGroupList grpLst ;
      SET_ROUTEID nodes ;
      pmdRemoteSessionSite *pSite = NULL ;
      pmdRemoteSession *pSession = NULL ;
      coordRemoteHandlerBase baseHander ;
      pmdSubSession *pSub        = NULL ;
      SET_ROUTEID::iterator it ;

      // Create remote session
      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Remote session is NULL in cb" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pSession = pSite->addSession( -1, &baseHander ) ;
      if ( !pSession )
      {
         PD_LOG( PDERROR, "Create remote session failed in session[%s]",
                 cb->getName() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // Get group list, store in grpLst
      pResource->updateGroupList( grpLst, cb, NULL, FALSE, FALSE, TRUE ) ;

      // Get all nodes, store in nodes
      rc = coordGetGroupNodes( pResource, cb, BSONObj(),
                               NODE_SEL_ALL, grpLst, nodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;

      // Check
      if ( nodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Not found any node" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
      if ( exceptSelf )
      {
         MsgRouteID routeID ;
         routeID = pmdGetNodeID() ;
         routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
         nodes.erase( routeID.value ) ;
      }

      ntyMsg.messageLength = sizeof( MsgHeader ) ;
      ntyMsg.opCode = MSG_CAT_GRP_CHANGE_NTY ;

      // Send msg to group nodes
      for ( it = nodes.begin(); it != nodes.end(); it++ )
      {
         pSub = pSession->addSubSession( *it ) ;
         pSub->setReqMsg( &ntyMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pSession->sendMsg( pSub ) ;
         rc = ( rcTmp && !rc ) ? rcTmp : rc ;
      }

   done:
      if ( pSession )
      {
         pSite->removeSession( pSession->sessionID() ) ;
      }
      PD_TRACE_EXITRC ( COORD_NODECMDHELPER_NOTIFY2ALLNODES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDGlobIdxHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN, "_coordCMDGlobIdxHandler::parseCatReturn" )
   INT32 _coordCMDGlobIdxHandler::parseCatReturn( coordCMDArguments *pArgs,
                                                  const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN ) ;

      BSONObj cataReplyObj ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      /* cataObj:
       * { GlobalIndex:[ { Collection: "GIDX_1.100_a", CLUniqueID: 123 },
       *                 { Collection: "GIDX_2.200_a", CLUniqueID: 456 }, ... ]
       */
      try
      {
         BSONObj gIndexObjs ;
         BOOLEAN haveGlobalIndex = FALSE ;

         rc = rtnGetArrayElement( cataReplyObj, CAT_GLOBAL_INDEX, gIndexObjs ) ;
         if ( SDB_OK == rc )
         {
            haveGlobalIndex = TRUE ;
         }
         else if ( SDB_FIELD_NOT_EXIST == rc )
         {
            haveGlobalIndex = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from obj [%s], "
                      "rc: %d", CAT_GLOBAL_INDEX,
                      cataReplyObj.toPoolString().c_str(), rc ) ;

         if ( haveGlobalIndex )
         {
            BSONElement element ;
            BSONObj gIndexInfo ;
            BSONObjIterator indexIter( gIndexObjs ) ;
            while ( indexIter.more() )
            {
               const CHAR *clName = NULL ;

               element = indexIter.next() ;
               PD_CHECK( Object == element.type(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to get element from field [%s], "
                         "element [%s] should be object",
                         CAT_GLOBAL_INDEX, element.toPoolString().c_str(),
                         rc ) ;

               gIndexInfo = element.embeddedObject() ;
               rc = rtnGetStringElement( gIndexInfo, CAT_COLLECTION,
                                         &clName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from "
                            "index info [%s], rc: %d", CAT_COLLECTION,
                            gIndexInfo.toPoolString().c_str(), rc ) ;

               _globalIndexes.push_back( clName ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get global index name list, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT, "_coordCMDGlobIdxHandler::onBeginEvent" )
   INT32 _coordCMDGlobIdxHandler::onBeginEvent( coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT ) ;

      _globalIndexes.clear() ;

      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT, "_coordCMDGlobIdxHandler::onDataP1Event" )
   INT32 _coordCMDGlobIdxHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                 coordResource *pResource,
                                                 coordCMDArguments *pArgs,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // now collection on data nodes are locked,
         // alter global indexes to enable repair check
         rc = _repairCheckGlobIdxCLs( pResource, TRUE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to enable repair check on global "
                      "index collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS, "_coordCMDGlobIdxHandler::_repairCheckGlobIdxCLs" )
   INT32 _coordCMDGlobIdxHandler::_repairCheckGlobIdxCLs( coordResource *resource,
                                                          BOOLEAN enableRepairCheck,
                                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS ) ;

      BSONObj options ;
      coordDataCMDHelper helper ;

      try
      {
         BSONObjBuilder builder ;
         builder.appendBool( FIELD_NAME_REPARECHECK, enableRepairCheck ) ;
         options = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build alter options, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;

         rc = helper.alterCL( resource, globIdxCLName, options, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s] with "
                      "[%s] : [%s], rc: %d",
                      globIdxCLName, FIELD_NAME_REPARECHECK,
                      enableRepairCheck ? "TRUE" : "FALSE", rc ) ;

         PD_LOG( PDEVENT, "Alter global index collection [%s] to [%s] "
                 "repair check success", globIdxCLName,
                 enableRepairCheck ? "enable" : "disable" ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordDropGlobIdxHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT, "_coordDropGlobIdxHandler::onDataP2Event" )
   INT32 _coordDropGlobIdxHandler::onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                                  coordResource *pResource,
                                                  coordCMDArguments *pArgs,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         rc = _dropGlobIdxCLs( pResource, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop global index collections, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS, "_coordDropGlobIdxHandler::_dropGlobIdxCLs" )
   INT32 _coordDropGlobIdxHandler::_dropGlobIdxCLs( coordResource *resource,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS ) ;

      coordDataCMDHelper helper ;

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;
         // skip recycle bin
         rc = helper.dropCL( resource, globIdxCLName, TRUE, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], "
                      "rc: %d", globIdxCLName, rc ) ;

         PD_LOG( PDEVENT, "Drop global index collection [%s] success",
                 globIdxCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordTruncGlobIdxHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT, "_coordTruncGlobIdxHandler::onDataP2Event" )
   INT32 _coordTruncGlobIdxHandler::onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                                   coordResource *resource,
                                                   coordCMDArguments *arguments,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         rc = _truncGlobIdxCLs( resource, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop global index collections, "
                      "rc: %d", rc ) ;
      }
      else if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // collection is truncated, disable repair check
         rc = _repairCheckGlobIdxCLs( resource, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to disable repair check on global "
                      "index collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS, "_coordTruncGlobIdxHandler::_truncGlobIdxCLs" )
   INT32 _coordTruncGlobIdxHandler::_truncGlobIdxCLs( coordResource *resource,
                                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS ) ;

      coordDataCMDHelper helper ;

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;
         // skip recycle bin
         rc = helper.truncateCL( resource, globIdxCLName, TRUE, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s], "
                      "rc: %d", globIdxCLName, rc ) ;

         PD_LOG( PDEVENT, "Truncate global index collection [%s] success",
                 globIdxCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _cocrdCMDTaskHandler implement
    */
   _coordCMDTaskHandler::_coordCMDTaskHandler()
   {
   }

   _coordCMDTaskHandler::~_coordCMDTaskHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDTASKHANDLER_ONBEGINEVENT, "_coordCMDTaskHandler::onBeginEvent" )
   INT32 _coordCMDTaskHandler::onBeginEvent( coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDTASKHANDLER_ONBEGINEVENT ) ;

      _taskSet.clear() ;

      PD_TRACE_EXITRC( COORD_DATACMDTASKHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATASKHANDLER_PARSECATRETURN, "_coordCMDTaskHandler::parseCatReturn" )
   INT32 _coordCMDTaskHandler::parseCatReturn( coordCMDArguments *pArgs,
                                               const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATASKHANDLER_PARSECATRETURN ) ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      rc = _parseTaskSet( cataObjs[ 0 ] ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse task set, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DATATASKHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATASKHANDLER_PARSECATP2RETURN, "_coordCMDTaskHandler::parseCatP2Return" )
   INT32 _coordCMDTaskHandler::parseCatP2Return( coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATASKHANDLER_PARSECATP2RETURN ) ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      rc = _parseTaskSet( cataObjs[ 0 ] ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse task set, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DATATASKHANDLER_PARSECATP2RETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATASKHANDLER__PARSETASKSET, "_coordCMDTaskHandler::_parseTaskSet" )
   INT32 _coordCMDTaskHandler::_parseTaskSet( const BSONObj &cataObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATASKHANDLER__PARSETASKSET ) ;

      try
      {
         BSONElement ele = cataObj.getField( CAT_TASKID_NAME ) ;

         if ( EOO != ele.type() )
         {
            PD_CHECK( Array == ele.type(), SDB_SYS, error, PDERROR,
                      "Failed to get tasks from CATALOG reply, field [%s] "
                      "is not an array", CAT_TASKID_NAME ) ;

            {
               BSONObjIterator iterTask( ele.embeddedObject() ) ;
               while ( iterTask.more() )
               {
                  BSONElement beTask = iterTask.next() ;
                  PD_CHECK( beTask.isNumber(), SDB_SYS, error, PDERROR,
                            "Failed to get tasks from CATALOG reply, "
                            "element in field [%s] is not a number",
                            CAT_TASKID_NAME ) ;

                  _taskSet.insert( (UINT64)( beTask.numberLong() ) ) ;
               }
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse tasks, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATATASKHANDLER__PARSETASKSET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDTASKHANDLER__WAITTASKS, "_coordCMDTaskHandler::_waitTasks" )
   INT32 _coordCMDTaskHandler::_waitTasks( coordResource *resource,
                                           BOOLEAN ignoreCanceled,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDTASKHANDLER__WAITTASKS ) ;

      for ( ossPoolSet< UINT64 >::iterator iter = _taskSet.begin() ;
            iter != _taskSet.end() ;
            ++ iter )
      {
         UINT64 taskID = *iter ;
         rc = _waitTask( resource, taskID, cb ) ;
         if ( ignoreCanceled && SDB_TASK_HAS_CANCELED == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to wait task [%llu], rc: %d",
                      taskID, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDTASKHANDLER__WAITTASKS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDTASKHANDLER__WAITTASK, "_coordCMDTaskHandler::_waitTask" )
   INT32 _coordCMDTaskHandler::_waitTask( coordResource *resource,
                                          UINT64 taskID,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDTASKHANDLER__WAITTASK ) ;

      coordCmdWaitTask cmd ;

      CHAR *msgBuff = NULL ;
      INT32 msgSize = 0 ;
      BSONObj query ;
      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;

      try
      {
         query = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build cancel task query, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msgBuff, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_WAITTASK,
                             0, 0, 0, -1, &query, NULL, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build cancel task message, rc: %d",
                   rc ) ;

      rc = cmd.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cancel task command, rc: %d",
                   rc ) ;

      rc = cmd.execute( (MsgHeader *)msgBuff, cb, contextID,  &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to cancel task [%llu], rc: %d",
                   taskID, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDTASKHANDLER__WAITTASK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDTASKHANDLER__CANCELTASKS, "_coordCMDTaskHandler::_cancelTasks" )
   INT32 _coordCMDTaskHandler::_cancelTasks( coordResource *resource,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDTASKHANDLER__CANCELTASKS ) ;

      for ( ossPoolSet< UINT64 >::iterator iter = _taskSet.begin() ;
            iter != _taskSet.end() ;
            ++ iter )
      {
         UINT64 taskID = *iter ;

         INT32 tmpRC = _cancelTask( resource, taskID, cb ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to cancel task [%llu], rc: %d",
                    taskID, tmpRC ) ;
         }
      }

      PD_TRACE_EXITRC( COORD_DATACMDTASKHANDLER__CANCELTASKS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDTASKHANDLER__CANCELTASK, "_coordCMDTaskHandler::_cancelTask" )
   INT32 _coordCMDTaskHandler::_cancelTask( coordResource *resource,
                                            UINT64 taskID,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDTASKHANDLER__CANCELTASK ) ;

      coordCmdCancelTask cmd ;

      CHAR *msgBuff = NULL ;
      INT32 msgSize = 0 ;
      BSONObj query ;
      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;

      try
      {
         query = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build cancel task query, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msgBuff, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK,
                             0, 0, 0, -1, &query, NULL, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build cancel task message, rc: %d",
                   rc ) ;

      rc = cmd.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cancel task command, rc: %d",
                   rc ) ;

      rc = cmd.execute( (MsgHeader *)msgBuff, cb, contextID,  &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to cancel task [%llu], rc: %d",
                   taskID, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDTASKHANDLER__CANCELTASK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDRecyTaskHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYTASKHANDLER_ONDATAP1EVENT, "_coordCMDRecyTaskHandler::onDataP1Event" )
   INT32 _coordCMDRecyTaskHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                  coordResource *resource,
                                                  coordCMDArguments *arguments,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYTASKHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         // wait tasks to be finished or cancelled
         rc = _waitTasks( resource, TRUE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait tasks finish, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYTASKHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDRecycleHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_PARSECATRETURN, "_coordCMDRecycleHandler::parseCatReturn" )
   INT32 _coordCMDRecycleHandler::parseCatReturn( coordCMDArguments *pArgs,
                                                  const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_PARSECATRETURN ) ;

      BSONObj cataReplyObj ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      try
      {
         BSONElement element ;

         // get recycle item
         element = cataReplyObj.getField( FIELD_NAME_RECYCLE_ITEM ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not object",
                      FIELD_NAME_RECYCLE_ITEM ) ;

            _recycleOptions = element.embeddedObject().copy() ;

            try
            {
               // as rename, retry when lock failed in data node
               pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;
            }
            catch ( exception  &e )
            {
               PD_LOG( PDERROR, "Failed to save retry return code, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            PD_LOG( PDDEBUG, "Got recycle options [%s]",
                    _recycleOptions.toPoolString().c_str() ) ;
         }
         else
         {
            goto done ;
         }

         // get dropping items
         element = cataReplyObj.getField( FIELD_NAME_DROP_RECYCLE_ITEM ) ;
         if ( Array == element.type() )
         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement subEle = iter.next() ;
               PD_CHECK( String == subEle.type(), SDB_SYS, error, PDERROR,
                         "Failed to get dropping recycle items, "
                         "sub-element should be string" ) ;
               _droppingItems.push_back( subEle.valuestrsafe() ) ;
            }
         }
         else if ( EOO != element.type() )
         {
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to get dropping recycle items, "
                      "field [%s] should be array",
                      FIELD_NAME_DROP_RECYCLE_ITEM ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse catalog return objects, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_REWRITEDATAMSG, "_coordCMDRecycleHandler::rewriteDataMsg" )
   INT32 _coordCMDRecycleHandler::rewriteDataMsg( BSONObjBuilder &queryBuilder,
                                                  BSONObjBuilder &hintBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_REWRITEDATAMSG ) ;

      try
      {
         hintBuilder.append( FIELD_NAME_RECYCLE_ITEM, _recycleOptions ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rewrite data message, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_REWRITEDATAMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_ONBEGINEVENT, "_coordCMDRecycleHandler::onBeginEvent" )
   INT32 _coordCMDRecycleHandler::onBeginEvent( coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_ONBEGINEVENT ) ;

      _recycleOptions = BSONObj() ;
      _droppingItems.clear() ;

      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_ONDATAP1EVENT, "_coordCMDRecycleHandler::onDataP1Event" )
   INT32 _coordCMDRecycleHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                 coordResource *resource,
                                                 coordCMDArguments *arguments,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         // drop old recycle items first to give room for current recycle item
         // if needed
         rc = _dropRecycleItems( resource, TRUE, TRUE, TRUE, TRUE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop dropping recycle items "
                      "from in recycle bin, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER__DROPRECYITEM, "_coordCMDRecycleHandler::_dropRecycleItem" )
   INT32 _coordCMDRecycleHandler::_dropRecycleItem( coordResource *resource,
                                                    const CHAR *recycleName,
                                                    BOOLEAN ignoreIfNotExists,
                                                    BOOLEAN isRecursive,
                                                    BOOLEAN isEnforced,
                                                    BOOLEAN ignoreLock,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER__DROPRECYITEM ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordDropRecycleBinItem command ;

      rc = command.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop recycle bin item command, "
                   "rc: %d", rc ) ;

      rc = msgBuildDropRecyBinItemMsg( &pMsg, &buffSize, recycleName,
                                       isRecursive, isEnforced, ignoreLock,
                                       0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop recycle bin item "
                   "request [%s], rc: %d", recycleName, rc ) ;

      rc = command.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      if ( ignoreIfNotExists && SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle bin item [%s], rc: %d",
                   recycleName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER__DROPRECYITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER__DROPRECYITEMS, "_coordCMDRecycleHandler::_dropRecycleItems" )
   INT32 _coordCMDRecycleHandler::_dropRecycleItems( coordResource *resource,
                                                     BOOLEAN ignoreIfNotExists,
                                                     BOOLEAN isRecursive,
                                                     BOOLEAN isEnforced,
                                                     BOOLEAN ignoreLock,
                                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER__DROPRECYITEMS ) ;

      for ( UTIL_RECY_ITEM_NAME_LIST_CIT iter = _droppingItems.begin() ;
            iter != _droppingItems.end() ;
            ++ iter )
      {
         const CHAR *recycleName = iter->c_str() ;

         rc = _dropRecycleItem( resource, recycleName, ignoreIfNotExists,
                                isRecursive, isEnforced, ignoreLock, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle bin item [%s], "
                      "rc: %d", recycleName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER__DROPRECYITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDRTRNTASKHANDLER_ONCOMMITEVENT, "_coordCMDRtrnTaskHandler::onCommitEvent" )
   INT32 _coordCMDRtrnTaskHandler::onCommitEvent( coordResource *resource,
                                                  coordCMDArguments *pArgs,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDRTRNTASKHANDLER_ONCOMMITEVENT ) ;

      // wait for rebuild index tasks
      _waitTasks( resource, FALSE, cb ) ;

      PD_TRACE_EXITRC( COORD_DATACMDRTRNTASKHANDLER_ONCOMMITEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDRTRNTASKHANDLER_ONROLLBACKEVENT, "_coordCMDRtrnTaskHandler::onRollbackEvent" )
   INT32 _coordCMDRtrnTaskHandler::onRollbackEvent( coordResource *resource,
                                                    coordCMDArguments *pArgs,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDRTRNTASKHANDLER_ONROLLBACKEVENT ) ;

      // cancel rebuild index tasks
      _cancelTasks( resource, cb ) ;

      PD_TRACE_EXITRC( COORD_DATACMDRTRNTASKHANDLER_ONROLLBACKEVENT, rc ) ;

      return rc ;
   }

   /*
      _coordCMDReturnHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARTRNHANDLER_PARSECATRETURN, "_coordCMDReturnHandler::parseCatReturn" )
   INT32 _coordCMDReturnHandler::parseCatReturn( coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARTRNHANDLER_PARSECATRETURN ) ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      _returnOptions = cataObjs[ 0 ].copy() ;
      rc = _returnInfo.fromBSON( _returnOptions, UTIL_RETURN_MASK_ALL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse return info from BSON, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DATARTRNHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARTRNHANDLER_REWRITEDATAMSG, "_coordCMDReturnHandler::rewriteDataMsg" )
   INT32 _coordCMDReturnHandler::rewriteDataMsg( BSONObjBuilder &queryBuilder,
                                                 BSONObjBuilder &hintBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARTRNHANDLER_REWRITEDATAMSG ) ;

      rc = utilRecycleReturnInfo::rebuildBSON( _returnOptions,
                                               hintBuilder,
                                               UTIL_RETURN_MASK_RENAME ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rebuild BSON for return "
                   "info, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DATARTRNHANDLER_REWRITEDATAMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARTRNHANDLER_ONBEGINEVENT, "_coordCMDReturnHandler::onBeginEvent" )
   INT32 _coordCMDReturnHandler::onBeginEvent( coordResource *resource,
                                               coordCMDArguments *arguments,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARTRNHANDLER_ONBEGINEVENT ) ;

      _returnInfo.clear() ;

      PD_TRACE_EXITRC( COORD_DATARTRNHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARTRNHANDLER_ONDATAP1EVENT, "_coordCMDReturnHandler::onDataP1Event" )
   INT32 _coordCMDReturnHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARTRNHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         const UTIL_RETURN_NAME_SET &replaceCS = _returnInfo.getReplaceCS() ;
         const UTIL_RETURN_NAME_SET &replaceCL = _returnInfo.getReplaceCL() ;

         coordDataCMDHelper helper ;

         for ( UTIL_RETURN_NAME_SET_CIT iter = replaceCS.begin() ;
               iter != replaceCS.end() ;
               ++ iter )
         {
            const CHAR *csName = iter->c_str() ;
            // skip recycle bin, and ignore catalog locks
            rc = helper.dropCS( resource, csName, TRUE, TRUE, cb ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop collection space [%s], "
                         "rc: %d", csName, rc ) ;
            PD_LOG( PDDEBUG, "Dropped collection space [%s]", csName ) ;
         }

         for ( UTIL_RETURN_NAME_SET_CIT iter = replaceCL.begin() ;
               iter != replaceCL.end() ;
               ++ iter )
         {
            const CHAR *clName = iter->c_str() ;
            // skip recycle bin, and ignore catalog locks
            rc = helper.dropCL( resource, clName, TRUE, TRUE, cb ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc ||
                 SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], "
                         "rc: %d", clName, rc ) ;
            PD_LOG( PDDEBUG, "Dropped collection [%s]", clName ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARTRNHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
