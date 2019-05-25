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

   Source File Name = coordAuthBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordAuthBase.hpp"
#include "msgAuth.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAuthBase impelemnt
   */
   _coordAuthBase::_coordAuthBase()
   {
   }

   _coordAuthBase::~_coordAuthBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHBASE_FORWARD, "_coordAuthBase::forward" )
   INT32 _coordAuthBase::forward( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  BOOLEAN sWhenNoPrimary,
                                  INT64 &contextID,
                                  const CHAR **ppUserName,
                                  const CHAR **ppPass )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHBASE_FORWARD ) ;

      BSONObj authObj ;
      BSONElement user, pass ;
      pmdSubSession *pSub = NULL ;
      coordGroupSel *pSel = _groupSession.getGroupSel() ;
      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pRemote = _groupSession.getSession() ;

      MsgHeader *pReply = NULL ;
      UINT32 primaryID = 0 ;
      MsgRouteID routeID ;

      contextID = -1 ;
      pCtrl->resetRetry() ;

      rc = extractAuthMsg( pMsg, authObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extrace auth msg, rc: %d", rc ) ;

      user = authObj.getField( SDB_AUTH_USER ) ;
      pass = authObj.getField( SDB_AUTH_PASSWD ) ;

      if ( ppUserName )
      {
         if ( String == user.type() )
         {
            *ppUserName = user.valuestr() ;
         }
         else
         {
            *ppUserName = "" ;
         }
      }
      if ( ppPass )
      {
         if ( String == pass.type() )
         {
            *ppPass = pass.valuestr() ;
         }
         else
         {
            *ppPass = "" ;
         }
      }

   retry:
      pSel->setPrimary( TRUE ) ;
      pSel->setServiceType( MSG_ROUTE_CAT_SERVICE ) ;
      _groupSession.resetSubSession() ;

      rc = _groupSession.sendMsg( pMsg, CATALOG_GROUPID, NULL, &pSub ) ;
      if ( SDB_CAT_NO_ADDR_LIST == rc )
      {
         PD_LOG( PDINFO, "There is no catalog address" ) ;
         goto error ;
      }
      else if ( rc && sWhenNoPrimary )
      {
         pSel->setPrimary( FALSE ) ;
         _groupSession.resetSubSession() ;
         rc = _groupSession.sendMsg( pMsg, CATALOG_GROUPID, NULL, &pSub ) ;
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Send message to catalog group failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      rc = pRemote->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply from catalog group failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      pReply = pSub->getRspMsg() ;
      if ( !pReply )
      {
         PD_LOG( PDERROR, "Reply is NULL in sub session" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      primaryID = MSG_GET_INNER_REPLY_STARTFROM( pReply ) ;
      rc = MSG_GET_INNER_REPLY_RC( pReply ) ;
      routeID.value = pReply->routeID.value ;
      if ( rc )
      {
         if ( pCtrl->canRetry( rc, routeID, primaryID, isReadOnly(), TRUE ) )
         {
            pCtrl->incRetry() ;
            goto retry ;
         }
         PD_LOG( PDERROR, "Failed to execute command[%u] on node[%s], rc: %d",
                 pMsg->opCode, routeID2String( routeID ).c_str(), rc ) ;
         goto error ;
      }

    done:
      _groupSession.resetSubSession() ;
      PD_TRACE_EXITRC ( COORD_AUTHBASE_FORWARD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

