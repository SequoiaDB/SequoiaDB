/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordRemoteMsgHandler.cpp

   Descriptive Name = Remote message handler on coordinator.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordRemoteMsgHandler.hpp"
#include "coordRemoteConnection.hpp"
#include "netRouteAgent.hpp"
#include "coordDataSource.hpp"
#include "msgMessageFormat.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   #define COORD_DS_SOCKET_TIMEOUT           ( 5000 )

   _coordDataSourceMsgHandler::_coordDataSourceMsgHandler( _pmdRemoteSessionMgr *pRSManager,
                                                           _coordDataSourceMgr *pDSMgr )
   : _pmdRemoteMsgHandler( pRSManager ),
     _pDSMgr( pDSMgr )
   {
   }

   _coordDataSourceMsgHandler::~_coordDataSourceMsgHandler()
   {
   }

   // Now the TCP connection to the data source node has been established.
   // As we actually connect to a coordinator in the data source cluster, it
   // is required to send a system info request and do the authentication.
   INT32 _coordDataSourceMsgHandler::handleConnect( const NET_HANDLE& handle,
                                                    _MsgRouteID id,
                                                    BOOLEAN isPositive )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;

      if ( SDB_IS_DSID( id.columns.groupID ) )
      {
         UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;
         dsID = SDB_GROUPID_2_DSID( id.columns.groupID ) ;
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         _netRouteAgent *pDSAgent = NULL ;
         NET_EH eh ;
         SOCKET nativeSocket = -1 ;

         if ( !_pDSMgr )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Data source manager is NULL" ) ;
            goto error ;
         }

         pDSAgent = _pDSMgr->getRouteAgent() ;
         if ( !pDSAgent )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get data source route agent failed" ) ;
            goto error ;
         }

         if ( !cb )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get current EDU failed" ) ;
            goto error ;
         }

         eh = pDSAgent->getFrame()->getEventHandle( handle ) ;
         if ( !eh.get() )
         {
            rc = SDB_NET_INVALID_HANDLE ;
            PD_LOG( PDERROR, "Get net handle[%u] failed, rc: %d",
                    handle, rc ) ;
            goto error ;
         }
         else
         {
            netEventHandler *handler =
                  dynamic_cast<netEventHandler *>( eh.get() ) ;
            nativeSocket = handler->socket().native() ;
            ossSocket tmpSocket( &nativeSocket, COORD_DS_SOCKET_TIMEOUT ) ;
            tmpSocket.closeWhenDestruct( FALSE ) ;
            coordRemoteConnection connection ;
            CoordDataSourcePtr dsPtr ;

            rc = connection.init( &tmpSocket ) ;
            PD_RC_CHECK( rc, PDERROR, "Initialize connection failed[%d]", rc ) ;

            rc = _pDSMgr->getOrUpdateDataSource( dsID, dsPtr, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get datasource[%u] failed, rc: %d",
                       dsID, rc ) ;
               goto error ;
            }

            rc = connection.authenticate( dsPtr->getUser(),
                                          dsPtr->getPassword(),
                                          cb ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }

      _pRSManager->handleConnect( handle, id, isPositive ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordDataSourceMsgHandler::handleClose( const NET_HANDLE &handle,
                                                 _MsgRouteID id )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;
      _pRSManager->handleClose( handle, id ) ;
   }

   INT32 _coordDataSourceMsgHandler::handleMsg( const NET_HANDLE &handle,
                                                const _MsgHeader *header,
                                                const CHAR *msg )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _pRSManager, "Remote Session Manager can't be NULL" ) ;

      rc = _pRSManager->pushMessage( handle, header ) ;
      if ( rc )
      {
         PD_LOG( ( ( SDB_INVALIDARG == rc ) ? PDWARNING : PDERROR ),
                 "Push message[%s] failed, rc: %d",
                 msg2String( header, MSG_MASK_ALL, 0 ).c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( ! ( rc == SDB_INVALIDARG && IS_REPLY_TYPE( header->opCode ) ) )
      {
         rc = SDB_NET_BROKEN_MSG ;
      }
      goto done ;
   }


}

