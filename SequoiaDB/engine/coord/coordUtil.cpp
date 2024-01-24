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

   Source File Name = coordUtil.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/


#include "coordUtil.hpp"
#include "msgDef.h"
#include "msgCatalogDef.h"
#include "pmdEDU.hpp"
#include "rtnQueryOptions.hpp"
#include "coordDataSource.hpp"
#include "coordCB.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   void coordSetResultInfo( INT32 flag,
                            ROUTE_RC_MAP &failedNodes,
                            utilWriteResult *pResult )
   {
      if ( pResult && pResult->isResultObjEmpty() )
      {
         ROUTE_RC_MAP::iterator iter = failedNodes.begin() ;
         while( iter != failedNodes.end() )
         {
            if ( flag == iter->second._rc &&
                 !iter->second._obj.isEmpty() )
            {
               pResult->setResultObj( iter->second._obj ) ;
               break ;
            }
            ++iter ;
         }
      }
   }

   void coordBuildFailedNodeReply( coordResource *pResource,
                                   ROUTE_RC_MAP &failedNodes,
                                   BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      if ( failedNodes.size() > 0 )
      {
         CoordGroupInfoPtr groupInfo ;
         string strHostName ;
         string strServiceName ;
         string strNodeName ;
         string strGroupName ;
         MsgRouteID routeID ;
         BSONObj errObj ;
         BSONArrayBuilder arrayBD( builder.subarrayStart(
                                   FIELD_NAME_ERROR_NODES ) ) ;
         ROUTE_RC_MAP::iterator iter = failedNodes.begin() ;
         while ( iter != failedNodes.end() )
         {
            strHostName.clear() ;
            strServiceName.clear() ;
            strNodeName.clear() ;
            strGroupName.clear() ;

            routeID.value = iter->first ;
            rc = pResource->getGroupInfo( routeID.columns.groupID, groupInfo ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to get group[%d] info, rc: %d",
                       routeID.columns.groupID, rc ) ;
            }
            else
            {
               strGroupName = groupInfo->groupName() ;

               routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;
               rc = groupInfo->getNodeInfo( routeID, strHostName,
                                            strServiceName ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Failed to get node[%d] info failed, "
                          "rc: %d", routeID.columns.nodeID, rc ) ;
               }
               else
               {
                  strNodeName = strHostName + ":" + strServiceName ;
               }
            }

            try
            {
               BSONObjBuilder objBD( arrayBD.subobjStart() ) ;
               objBD.append( FIELD_NAME_NODE_NAME, strNodeName ) ;
               //objBD.append( FIELD_NAME_HOST, strHostName ) ;
               //objBD.append( FIELD_NAME_SERVICE_NAME, strServiceName ) ;
               objBD.append( FIELD_NAME_GROUPNAME, strGroupName ) ;
               //objBD.append( FIELD_NAME_GROUPID, routeID.columns.groupID ) ;
               //objBD.append( FIELD_NAME_NODEID, (INT32)routeID.columns.nodeID ) ;
               objBD.append( FIELD_NAME_RCFLAG, iter->second._rc ) ;
               objBD.append( FIELD_NAME_ERROR_INFO, iter->second._obj ) ;
               objBD.done() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDWARNING, "Build error object occur exception: %s",
                       e.what() ) ;
               /// then ignored this record
            }
            ++iter ;
         }

         arrayBD.done() ;
      }
   }

   BSONObj coordBuildErrorObj( coordResource *pResource,
                               INT32 &flag,
                               pmdEDUCB *cb,
                               ROUTE_RC_MAP *pFailedNodes,
                               UINT32 sucNum )
   {
      BSONObjBuilder builder ;

      coordBuildErrorObj( pResource, flag, cb, pFailedNodes, builder, sucNum ) ;

      return builder.obj() ;
   }

   void coordBuildErrorObj( coordResource *pResource,
                            INT32 &flag,
                            _pmdEDUCB *cb,
                            ROUTE_RC_MAP *pFailedNodes,
                            BSONObjBuilder &builder,
                            UINT32 sucNum )
   {
      const CHAR *pDetail = "" ;
      ROUTE_RC_MAP::iterator iter ;

      if ( SDB_OK == flag && pFailedNodes && pFailedNodes->size() > 0 )
      {
         if ( 0 == sucNum )
         {
            flag = pFailedNodes->begin()->second._rc ;
         }
         else
         {
            flag = SDB_COORD_NOT_ALL_DONE ;
         }
      }

      if ( cb && cb->getInfo( EDU_INFO_ERROR ) )
      {
         pDetail = cb->getInfo( EDU_INFO_ERROR ) ;
      }

      if ( ( !pDetail || !*pDetail ) && pFailedNodes )
      {
         ROUTE_RC_MAP::iterator iter = pFailedNodes->begin() ;
         while( iter != pFailedNodes->end() )
         {
            if ( flag == iter->second._rc &&
                 !iter->second._obj.isEmpty() )
            {
               pDetail = iter->second._obj.getStringField( OP_ERR_DETAIL ) ;
               if ( *pDetail )
               {
                  break ;
               }
            }
            ++iter ;
         }
      }

      try
      {
         builder.append( OP_ERRNOFIELD, flag ) ;
         builder.append( OP_ERRDESP_FIELD, getErrDesp( flag ) ) ;
         builder.append( OP_ERR_DETAIL, pDetail ? pDetail : "" ) ;
         /// add ErrNodes
         if ( pFailedNodes && pFailedNodes->size() > 0 )
         {
            coordBuildFailedNodeReply( pResource, *pFailedNodes, builder ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Build return object failed: %s", e.what() ) ;
      }
   }

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst,
                                const CHAR* fieldName )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement beGroupArr = obj.getField( fieldName ) ;
         if ( beGroupArr.eoo() || beGroupArr.type() != Array )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Failed to get the field(%s) from obj[%s]",
                     fieldName, obj.toString().c_str() ) ;
            goto error ;
         }
         BSONObjIterator i( beGroupArr.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONObj boGroupInfo ;
            BSONElement beTmp = i.next() ;
            if ( Object != beTmp.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Group info in obj[%s] must be object",
                       obj.toString().c_str() ) ;
               goto error ;
            }
            boGroupInfo = beTmp.embeddedObject() ;
            beTmp = boGroupInfo.getField( CAT_GROUPID_NAME ) ;
            if ( beTmp.eoo() || !beTmp.isNumber() )
            {
               rc = SDB_INVALIDARG;
               PD_LOG ( PDERROR, "Failed to get the field(%s) from obj[%s]",
                        CAT_GROUPID_NAME, obj.toString().c_str() );
               goto error ;
            }

            // add to group list
            groupLst[ beTmp.numberInt() ] = beTmp.numberInt() ;
            PD_LOG( PDDEBUG, "Get group[%d] into list", beTmp.numberInt() ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "Parse catalog reply object occur exception: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst )
   {
      return coordGetGroupsFromObj( obj, groupLst, CAT_GROUP_NAME ) ;
   }

   INT32 coordGetFailedGroupsFromObj( const BSONObj &obj,
                                      CoordGroupList &groupLst )
   {
      return coordGetGroupsFromObj( obj, groupLst, FIELD_NAME_FAILGROUP ) ;
   }

   INT32 coordParseGroupList( coordResource *pResource,
                              pmdEDUCB *cb,
                              const BSONObj &obj,
                              CoordGroupList &groupList,
                              BSONObj *pNewObj,
                              BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr grpPtr ;
      vector< INT32 > tmpVecInt ;
      vector< const CHAR* > tmpVecStr ;
      UINT32 i = 0 ;

      rc = coordParseGroupsInfo( obj, tmpVecInt, tmpVecStr,
                                 pNewObj, strictCheck ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse object[%s] group list failed, rc: %d",
                   obj.toString().c_str(), rc ) ;

      /// group id
      for ( i = 0 ; i < tmpVecInt.size() ; ++i )
      {
         groupList[(UINT32)tmpVecInt[i]] = (UINT32)tmpVecInt[i] ;
      }

      /// group name
      for ( i = 0 ; i < tmpVecStr.size() ; ++i )
      {
         rc = pResource->getGroupInfo( tmpVecStr[i], grpPtr ) ;
         if ( rc )
         {
            rc = pResource->updateGroupInfo( tmpVecStr[i], grpPtr, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Update group[%s] info failed, rc: %d",
                         tmpVecStr[i], rc ) ;
         }
         groupList[ grpPtr->groupID() ] = grpPtr->groupID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordParseGroupList( coordResource *pResource,
                              pmdEDUCB *cb,
                              MsgOpQuery *pMsg,
                              FILTER_BSON_ID filterObjID,
                              CoordGroupList &groupList,
                              BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      rtnQueryOptions queryOption ;
      BSONObj *pFilterObj = NULL ;

      rc = queryOption.fromQueryMsg( (CHAR *)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      pFilterObj = coordGetFilterByID( filterObjID, queryOption ) ;
      try
      {
         if ( !pFilterObj->isEmpty() )
         {
            rc = coordParseGroupList( pResource, cb, *pFilterObj,
                                      groupList, NULL, strictCheck ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  GROUP_VEC & groupPtrs )
   {
      INT32 rc = SDB_OK ;
      groupPtrs.clear() ;
      CoordGroupInfoPtr ptr ;

      CoordGroupList::iterator it = groupList.begin() ;
      while ( it != groupList.end() )
      {
         rc = pResource->getGroupInfo( it->second, ptr ) ;
         if ( rc )
         {
            rc = pResource->updateGroupInfo( it->second, ptr, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Update group[%d] info failed, rc: %d",
                         it->second, rc ) ;
         }
         groupPtrs.push_back( ptr ) ;
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  CoordGroupMap &groupMap,
                                  BOOLEAN reNew )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr ptr ;

      if ( reNew )
      {
         groupMap.clear() ;
      }

      CoordGroupList::iterator it = groupList.begin() ;
      while ( it != groupList.end() )
      {
         if ( !reNew && groupMap.end() != groupMap.find( it->second ) )
         {
            // alredy exist, don't update group info
            ++it ;
            continue ;
         }
         rc = pResource->getOrUpdateGroupInfo( it->second, ptr, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group[%d] info, rc: %d",
                      it->second, rc ) ;
         groupMap[ it->second ] = ptr ;
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void coordGroupPtr2GroupList( GROUP_VEC &groupPtrs,
                                 CoordGroupList &groupList )
   {
      groupList.clear() ;

      GROUP_VEC::iterator it = groupPtrs.begin() ;
      while ( it != groupPtrs.end() )
      {
         CoordGroupInfoPtr &ptr = *it ;
         groupList[ ptr->groupID() ] = ptr->groupID() ;
         ++it ;
      }
   }

   INT32 coordGetGroupNodes( coordResource *pResource,
                             pmdEDUCB *cb,
                             const BSONObj &filterObj,
                             NODE_SEL_STY emptyFilterSel,
                             CoordGroupList &groupList,
                             SET_ROUTEID &nodes,
                             BSONObj *pNewObj,
                             BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfoPtr ptr ;
      MsgRouteID routeID ;
      GROUP_VEC groupPtrs ;
      GROUP_VEC::iterator it ;

      vector< INT32 > vecNodeID ;
      vector< const CHAR* > vecHostName ;
      vector< const CHAR* > vecSvcName ;
      vector< const CHAR* > vecNodeName ;
      vector< INT32 > vecInstanceID ;
      BOOLEAN emptyFilter = TRUE ;

      nodes.clear() ;

      rc = coordGroupList2GroupPtr( pResource, cb, groupList, groupPtrs ) ;
      PD_RC_CHECK( rc, PDERROR, "Group ids to group info failed, rc: %d", rc ) ;

      rc = coordParseNodesInfo( filterObj, vecNodeID, vecHostName,
                                vecSvcName, vecNodeName, vecInstanceID,
                                pNewObj, strictCheck ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes info failed, rc: %d",
                   filterObj.toString().c_str(), rc ) ;

      if ( vecNodeID.size() > 0 || vecHostName.size() > 0 ||
           vecSvcName.size() > 0 || vecNodeName.size() > 0 ||
           vecInstanceID.size() > 0 )
      {
         emptyFilter = FALSE ;
      }

      /// parse nodes
      it = groupPtrs.begin() ;
      while ( it != groupPtrs.end() )
      {
         UINT32 calTimes = 0 ;
         UINT32 randNum = ossRand() ;
         ptr = *it ;

         routeID.value = MSG_INVALID_ROUTEID ;
         clsGroupItem *grp = ptr.get() ;
         if ( grp->nodeCount() > 0 )
         {
            randNum %= grp->nodeCount() ;
         }

         /// calc pos
         while ( calTimes++ < grp->nodeCount() )
         {
            if ( NODE_SEL_SECONDARY == emptyFilterSel &&
                 randNum == grp->getPrimaryPos() )
            {
               randNum = ( randNum + 1 ) % grp->nodeCount() ;
               continue ;
            }
            else if ( NODE_SEL_PRIMARY == emptyFilterSel &&
                      CLS_RG_NODE_POS_INVALID != grp->getPrimaryPos() )
            {
               randNum = grp->getPrimaryPos() ;
            }
            break ;
         }

         routeID.columns.groupID = grp->groupID() ;
         const VEC_NODE_INFO *nodesInfo = grp->getNodes() ;
         for ( VEC_NODE_INFO::const_iterator itrn = nodesInfo->begin() ;
               itrn != nodesInfo->end();
               ++itrn, --randNum )
         {
            if ( FALSE == emptyFilter )
            {
               BOOLEAN findNode = FALSE ;
               UINT32 index = 0 ;
               /// check node id
               for ( index = 0 ; index < vecNodeID.size() ; ++index )
               {
                  if ( (UINT16)vecNodeID[ index ] == itrn->_id.columns.nodeID )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               findNode = FALSE ;
               /// check host name
               for ( index = 0 ; index < vecHostName.size() ; ++index )
               {
                  if ( 0 == ossStrcmp( vecHostName[ index ],
                                       itrn->_host ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               findNode = FALSE ;
               /// check svcname
               for ( index = 0 ; index < vecSvcName.size() ; ++index )
               {
                  if ( 0 == ossStrcmp( vecSvcName[ index ],
                                       itrn->_service[MSG_ROUTE_LOCAL_SERVICE].c_str() ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               findNode = FALSE ;
               /// check node name
               for ( index = 0 ; index < vecNodeName.size() ; ++index )
               {
                  if ( coordMatchNodeName( vecNodeName[ index ],
                                           itrn->_host,
                                           itrn->_service[MSG_ROUTE_LOCAL_SERVICE].c_str() ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }

               /// check instance ID
               findNode = FALSE ;
               for ( index = 0 ; index < vecInstanceID.size() ; ++ index )
               {
                  if ( itrn->_instanceID == (UINT8)( vecInstanceID[ index ] ) )
                  {
                     findNode = TRUE ;
                     break ;
                  }
               }
               if ( index > 0 && !findNode )
               {
                  continue ;
               }
            }
            else if ( NODE_SEL_ALL != emptyFilterSel && 0 != randNum )
            {
               continue ;
            }
            routeID.columns.nodeID = itrn->_id.columns.nodeID ;
            routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
            nodes.insert( routeID.value ) ;
         }
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordGetCLDataSource( const CHAR *collection,
                               pmdEDUCB *cb,
                               coordResource *pResource,
                               BOOLEAN &isDataSourceCL,
                               BOOLEAN &isHighErrLevel )
   {
      INT32 rc = SDB_OK ;
      coordCataSel cataSel ;
      CoordCataInfoPtr cataPtr ;
      UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;

      isDataSourceCL = FALSE ;
      isHighErrLevel = FALSE ;

      rc = cataSel.bind( pResource, collection, cb, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Update collection[%s]'s catalog info failed, rc: %d",
                   collection, rc ) ;

      cataPtr = cataSel.getCataPtr() ;
      dsID = cataPtr->getDataSourceID() ;
      if ( dsID != UTIL_INVALID_DS_UID )
      {
         isDataSourceCL = TRUE ;

         CoordDataSourcePtr dsPtr ;
         coordDataSourceMgr *pDSMgr = sdbGetCoordCB()->getDSManager() ;
         rc = pDSMgr->getOrUpdateDataSource( dsID, dsPtr, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get data source[%u] failed, rc: %d",
                      dsID, rc ) ;

         if ( 0 == ossStrcmp( dsPtr->getErrCtlLevel(), VALUE_NAME_HIGH ) )
         {
            isHighErrLevel = TRUE ;
         }
      }

   done :
      return rc ;

   error :
      goto done ;
   }


   INT32 coordRemoveFailedGroup( CoordGroupList &groupLst,
                                 BOOLEAN &hasFailedGroup,
                                 const vector<BSONObj> &replyObjs )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList failedGroupLst ;

      try
      {
         // The vector has only one obj
         if ( ! replyObjs.empty() )
         {
            const BSONObj &replyObj = replyObjs[0] ;

            rc = coordGetFailedGroupsFromObj( replyObj, failedGroupLst ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get groups from catalog "
                         "reply [%s], rc: %d", replyObj.toPoolString().c_str(), rc ) ;

            if ( 0 != failedGroupLst.size() )
            {
               hasFailedGroup = TRUE ;
               ossPoolStringStream returnStr ;
               returnStr << "Operation failed in the following groups: [" ;

               CoordGroupList::const_iterator itr = failedGroupLst.begin() ;
               while ( failedGroupLst.end() != itr )
               {
                  UINT32 groupID = ( itr++ )->second ;

                  // Remove failed group
                  groupLst.erase( groupID ) ;

                  returnStr << " " << groupID ;
                  if ( failedGroupLst.end() != itr )
                  {
                     returnStr << "," ;
                  }
               }
               returnStr << " ]" ;
               PD_LOG_MSG( PDERROR, returnStr.str().c_str() ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

