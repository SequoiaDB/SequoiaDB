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
                                  const CHAR **ppPass,
                                  BSONObj *pOptions,
                                  rtnContextBuf *pBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHBASE_FORWARD ) ;

      BSONObj authObj ;
      BSONElement user, pass, eOptions ;
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
      eOptions = authObj.getField( FIELD_NAME_OPTIONS ) ;

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
      if ( pOptions )
      {
         if ( Object == eOptions.type() )
         {
            *pOptions = eOptions.embeddedObject() ;
         }
         else
         {
            *pOptions = BSONObj() ;
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

      /// wait reply
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

         // get error reply
         _extractReply( (const MsgOpReply*)pReply, pBuf ) ;

         goto error ;
      }

      if ( msgIsInnerOpReply( pReply ) )
      {
         _onSucReply( (const MsgOpReply*)pReply ) ;
      }

      rc = _extractReply( (const MsgOpReply*)pReply, pBuf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract reply, rc: %d", rc ) ;

    done:
      _groupSession.resetSubSession() ;
      PD_TRACE_EXITRC ( COORD_AUTHBASE_FORWARD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordAuthBase::_onSucReply( const MsgOpReply *pReply )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHBASE_EXTREPLY, "_coordAuthBase::_extractReply" )
   INT32 _coordAuthBase::_extractReply( const MsgOpReply *pReply,
                                        rtnContextBuf *pBuf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_AUTHBASE_EXTREPLY ) ;

      if ( ( NULL != pBuf ) &&
           ( NULL != pReply ) &&
           ( pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) ) )
      {
         try
         {
            BSONObj obj( (const CHAR*)pReply + sizeof( MsgOpReply ) ) ;
            *pBuf = rtnContextBuf( obj ) ;
            rc = pBuf->getOwned() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to build user info buf, rc: %d", rc ) ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_AUTHBASE_EXTREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _coordAuthBase::updateSessionByOptions( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      UINT32 mask = 0 ;
      UINT32 configMask = 0 ;

      try
      {
         BSONElement e = options.getField( FIELD_NAME_AUDIT_MASK ) ;
         if ( String == e.type() )
         {
            rc = pdString2AuditMask( e.valuestr(), mask, TRUE, &configMask ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "User's audit config[%s] is invalid, rc: %d",
                       e.valuestr(), rc ) ;
               /// ignore
            }
            else
            {
               pdUpdateCurAuditMask( AUDIT_LEVEL_USER, mask, configMask ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         /// ignore
      }
   }

}

