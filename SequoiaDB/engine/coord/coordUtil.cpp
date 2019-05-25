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
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

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
               objBD.append( FIELD_NAME_GROUPNAME, strGroupName ) ;
               objBD.append( FIELD_NAME_RCFLAG, iter->second._rc ) ;
               objBD.append( FIELD_NAME_ERROR_IINFO, iter->second._obj ) ;
               objBD.done() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDWARNING, "Build error object occur exception: %s",
                       e.what() ) ;
            }
            ++iter ;
         }

         arrayBD.done() ;
      }
   }

   BSONObj coordBuildErrorObj( coordResource *pResource,
                               INT32 &flag,
                               pmdEDUCB *cb,
                               ROUTE_RC_MAP *pFailedNodes )
   {
      BSONObjBuilder builder ;
      const CHAR *pDetail = "" ;

      if ( SDB_OK == flag && pFailedNodes && pFailedNodes->size() > 0 )
      {
         flag = SDB_COORD_NOT_ALL_DONE ;
      }

      if ( cb && cb->getInfo( EDU_INFO_ERROR ) )
      {
         pDetail = cb->getInfo( EDU_INFO_ERROR ) ;
      }

      builder.append( OP_ERRNOFIELD, flag ) ;
      builder.append( OP_ERRDESP_FIELD, getErrDesp( flag ) ) ;
      builder.append( OP_ERR_DETAIL, pDetail ? pDetail : "" ) ;
      if ( pFailedNodes && pFailedNodes->size() > 0 )
      {
         coordBuildFailedNodeReply( pResource, *pFailedNodes, builder ) ;
      }

      return builder.obj() ;
   }

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement beGroupArr = obj.getField( CAT_GROUP_NAME ) ;
         if ( beGroupArr.eoo() || beGroupArr.type() != Array )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Failed to get the field(%s) from obj[%s]",
                     CAT_GROUP_NAME, obj.toString().c_str() ) ;
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

            groupLst[ beTmp.numberInt() ] = beTmp.numberInt() ;
            PD_LOG( PDDEBUG, "Get group[%d] into list", beTmp.numberInt() ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Parse catalog reply object occur exception: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
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

      for ( i = 0 ; i < tmpVecInt.size() ; ++i )
      {
         groupList[(UINT32)tmpVecInt[i]] = (UINT32)tmpVecInt[i] ;
      }

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
      BOOLEAN emptyFilter = TRUE ;

      nodes.clear() ;

      rc = coordGroupList2GroupPtr( pResource, cb, groupList, groupPtrs ) ;
      PD_RC_CHECK( rc, PDERROR, "Group ids to group info failed, rc: %d", rc ) ;

      rc = coordParseNodesInfo( filterObj, vecNodeID, vecHostName,
                                vecSvcName, pNewObj, strictCheck ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes info failed, rc: %d",
                   filterObj.toString().c_str(), rc ) ;

      if ( vecNodeID.size() > 0 || vecHostName.size() > 0 ||
           vecSvcName.size() > 0 )
      {
         emptyFilter = FALSE ;
      }

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

}

