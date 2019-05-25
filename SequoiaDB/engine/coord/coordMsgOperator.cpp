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

   Source File Name = coordMsgOperator.cpp

   Descriptive Name = Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordMsgOperator.hpp"
#include "rtn.hpp"
#include "coordUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "pmdEnv.hpp"
#include "msg.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordMsgOperator implement
   */
   _coordMsgOperator::_coordMsgOperator()
   {
      const static string s_name( "Msg" ) ;
      setName( s_name ) ;
   }

   _coordMsgOperator::~_coordMsgOperator()
   {
   }

   INT32 _coordMsgOperator::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      contextID    = -1 ;

      CoordGroupList groupLst ;
      SET_ROUTEID sendNodes ;
      MsgRouteID routeID ;
      ROUTE_RC_MAP failedNodes ;

      pmdRemoteSession *pRemote     = _groupSession.getSession() ;
      pmdSubSession *pSub           = NULL ;
      SET_ROUTEID::iterator it ;

      rtnMsg( (MsgOpMsg *)pMsg ) ;

      rc = _pResource->updateGroupList( groupLst, cb, NULL,
                                        FALSE, FALSE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d", rc ) ;

      rc = coordGetGroupNodes( _pResource, cb, BSONObj(), NODE_SEL_ALL,
                               groupLst, sendNodes, NULL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( sendNodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Not found any node" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }

      routeID = pmdGetNodeID() ;
      routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
      sendNodes.erase( routeID.value ) ;

      _groupSession.clear() ;

      it = sendNodes.begin() ;
      while( it != sendNodes.end() )
      {
         pSub = pRemote->addSubSession( *it ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rc = pRemote->sendMsg( pSub ) ;
         if ( rc )
         {
            failedNodes[ *it ] = rc ;
            pRemote->delSubSession( *it ) ;
         }
         ++it ;
      }

      rc = pRemote->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || failedNodes.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &failedNodes ) ) ;
      }
      _groupSession.clear() ;
      return rc ;
   error:
      goto done ;
   }

}

