/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = seAdptDBAssist.cpp

   Descriptive Name = Database assistant for search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/14/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptDBAssist.hpp"

namespace seadapter
{
   _seAdptDBAssist::_seAdptDBAssist( _netMsgHandler *msgHandler )
   : _routeAgent( msgHandler ),
     _dataNetHandle( NET_INVALID_HANDLE ),
     _cataNetHandle( NET_INVALID_HANDLE ),
     _cataGrpInfo( CATALOG_GROUPID )
   {
      _dataNodeID.value = MSG_INVALID_ROUTEID ;
   }

   _seAdptDBAssist::~_seAdptDBAssist()
   {
   }

   INT32 _seAdptDBAssist::addTimer( UINT32 millsec, _netTimeoutHandler *handler,
                                    UINT32 &timerID )
   {
      return _routeAgent.addTimer( millsec, handler, timerID ) ;
   }

   INT32 _seAdptDBAssist::removeTimer( UINT32 timerID )
   {
      return _routeAgent.removeTimer( timerID ) ;
   }

   INT32 _seAdptDBAssist::updateDataNodeRoute( const MsgRouteID &id,
                                               const CHAR *host,
                                               const CHAR *service )
   {
      if ( MSG_INVALID_ROUTEID != _dataNodeID.value )
      {
         _routeAgent.delRoute( _dataNodeID ) ;
         _dataNetHandle = NET_INVALID_HANDLE ;
      }

      _dataNodeID = id ;

      return _routeAgent.updateRoute( id, host, service ) ;
   }

   INT32 _seAdptDBAssist::updateGroupInfo( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      rc = _cataGrpInfo.updateGroupItem( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Update group info from bson[%s] failed[%d]",
                   obj.toString().c_str(), rc ) ;

      rc = _updateRouteInfo() ;
      PD_RC_CHECK( rc, PDERROR, "Update route infor failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::changeCataPrimary( UINT16 nodeID )
   {
      INT32 rc = SDB_OK ;
      MsgRouteID newPrimary ;
      MsgRouteID currentPrimary =
            _cataGrpInfo.primary( MSG_ROUTE_CAT_SERVICE ) ;
      newPrimary.value = MSG_INVALID_ROUTEID ;

      if ( nodeID == currentPrimary.columns.nodeID )
      {
         goto done ;
      }

      if ( INVALID_NODE_ID == nodeID )
      {
         // If specify an invalid id, just skip to the next node.
         UINT32 newPrimaryPos =
               ( _cataGrpInfo.getPrimaryPos() + 1 ) % _cataGrpInfo.nodeCount() ;
         rc = _cataGrpInfo.getNodeID( newPrimaryPos, newPrimary,
                                      MSG_ROUTE_CAT_SERVICE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get node id failed[%d]", rc ) ;
      }
      else if ( nodeID != currentPrimary.columns.nodeID )
      {
         newPrimary.columns.groupID = CATALOG_GROUPID ;
         newPrimary.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         newPrimary.columns.nodeID = nodeID ;
      }
      _cataNetHandle = NET_INVALID_HANDLE ;
      rc = _cataGrpInfo.updatePrimary( newPrimary, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update catalog primary to node[%u] "
                                "failed[%d]", newPrimary.columns.nodeID, rc ) ;
      PD_LOG( PDDEBUG, "Change catalogue primary node from %u to %u",
              currentPrimary.columns.nodeID, newPrimary.columns.nodeID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seAdptDBAssist::setDataNetHandle( NET_HANDLE handle )
   {
      _dataNetHandle = handle ;
   }

   void _seAdptDBAssist::setCataNetHandle( NET_HANDLE handle )
   {
      _cataNetHandle = handle ;
   }

   void _seAdptDBAssist::invalidNetHandle( NET_HANDLE handle )
   {
      if ( handle == _dataNetHandle )
      {
         _dataNetHandle = NET_INVALID_HANDLE ;
      }
      else if ( handle == _cataNetHandle )
      {
         _cataNetHandle = NET_INVALID_HANDLE ;
      }
   }

   INT32 _seAdptDBAssist::queryOnDataNode( const CHAR *clName,
                                           UINT64 reqID,
                                           UINT32 tid,
                                           INT32 flag,
                                           INT64 numToSkip,
                                           INT64 numToReturn,
                                           const BSONObj *query,
                                           const BSONObj *selector,
                                           const BSONObj *orderBy,
                                           const BSONObj *hint,
                                           IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 bufSize = 0 ;
      CHAR *msg = NULL ;

      rc = msgBuildQueryMsg( &msg, &bufSize, clName, flag,
                             reqID, numToSkip, numToReturn, query,
                             selector, orderBy, hint, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;

      ((MsgHeader *)msg)->TID = tid ;
      rc = sendToDataNode( (const MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send query message to data node failed[%d]",
                   rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::queryOnCataNode( const CHAR *clName,
                                           UINT64 reqID,
                                           UINT32 tid,
                                           INT64 numToSkip,
                                           INT64 numToReturn,
                                           const BSONObj *query,
                                           const BSONObj *selector,
                                           const BSONObj *orderBy,
                                           const BSONObj *hint,
                                           IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 bufSize = 0 ;
      CHAR *msg = NULL ;

      rc = msgBuildQueryMsg( &msg, &bufSize, clName, 0,
                             reqID, numToSkip, numToReturn, query,
                             selector, orderBy, hint, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;

      ((MsgHeader *)msg)->TID = tid ;
      rc = _sendToCataNode( (const MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send query message to catalogue node "
                                "failed[%d]", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::queryCLCataInfo( const CHAR *clName, UINT64 reqID,
                                           UINT32 tid, const BSONObj *selector,
                                           IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 buffSize = 0 ;
      INT32 flag = FLG_QUERY_WITH_RETURNDATA ;
      BSONObj query ;

      SDB_ASSERT( clName, "collection name is NULL" ) ;

      try
      {
         query = BSON( FIELD_NAME_NAME << clName ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = msgBuildQueryCatalogReqMsg( &msg, &buffSize, flag, reqID, 0, -1, tid,
                                       &query, selector, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query catalog message failed[%d]", rc ) ;

      ((MsgHeader *)msg)->TID = tid ;
      rc = _sendToCataNode( (const MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to cata node failed[%d]", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::doCmdOnDataNode( const CHAR *cmd,
                                           const BSONObj &argObj,
                                           UINT64 reqID, UINT32 tid,
                                           IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 buffSize = 0 ;
      BSONObj emptyObj ;

      rc = msgBuildQueryCMDMsg( &msg, &buffSize, cmd, argObj, emptyObj,
                                emptyObj, emptyObj, reqID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build command message failed[%d]", rc ) ;

      ((MsgHeader *)msg)->TID = tid ;
      rc = sendToDataNode( (const MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to data node failed[%d]", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::sendToDataNode( const MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( msg, "Message is NULL" ) ;

      if ( NET_INVALID_HANDLE != _dataNetHandle )
      {
         rc = _routeAgent.syncSend( _dataNetHandle, (void *)msg ) ;
      }
      else
      {
         rc = _routeAgent.syncSend( _dataNodeID, (void *)msg ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Send message to data node failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::sendMsg( const MsgHeader *msg, NET_HANDLE handle )
   {
      INT32 rc = SDB_OK ;

      if ( NET_INVALID_HANDLE == handle )
      {
         rc = SDB_NET_INVALID_HANDLE ;
         PD_LOG( PDERROR, "Net handle is invalid" ) ;
         goto error ;
      }

      rc = _routeAgent.syncSend( handle, (void *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptDBAssist::_updateRouteInfo()
   {
      INT32 rc = SDB_OK ;
      string host ;
      string service ;
      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      UINT32 index = 0 ;
      while ( SDB_OK == _cataGrpInfo.getNodeInfo( index++, routeID, host,
                                   service, MSG_ROUTE_CAT_SERVICE ) )
      {
         rc = _routeAgent.updateRoute( routeID, host.c_str(),
                                       service.c_str() ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_NET_UPDATE_EXISTING_NODE == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Update route[%s] failed[%d]",
                       routeID2String( routeID ).c_str(), rc ) ;
               break ;
            }
         }
      }

      return rc ;
   }

   INT32 _seAdptDBAssist::_sendToCataNode( const MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( msg, "Message is NULL" ) ;

      if ( NET_INVALID_HANDLE != _cataNetHandle )
      {
         rc = _routeAgent.syncSend( _cataNetHandle, (void *)msg ) ;
      }
      else
      {
         rc = _routeAgent.syncSend(
                 _cataGrpInfo.primary( MSG_ROUTE_CAT_SERVICE ), (void *)msg ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Send message to catalogue node failed[%d]",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}
