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

   Source File Name = pmdRemoteMsgEventHandler.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdDef.hpp"
#include "pmdRemoteMsgEventHandler.hpp"
#include "pmdEDU.hpp"
#include "pmdRemoteSession.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "msgMessage.hpp"

namespace engine
{

   /*
      _pmdRemoteMsgHandler implement
   */
   _pmdRemoteMsgHandler::_pmdRemoteMsgHandler( _pmdRemoteSessionMgr *pRSManager )
   {
      _pRSManager       = pRSManager ;
      _pMainCB          = NULL ;
   }

   _pmdRemoteMsgHandler::~_pmdRemoteMsgHandler()
   {
      _pRSManager       = NULL ;
   }

   void _pmdRemoteMsgHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB    = cb ;
   }

   void _pmdRemoteMsgHandler::detach()
   {
      _pMainCB    = NULL ;
   }

   INT32 _pmdRemoteMsgHandler::handleMsg( const NET_HANDLE &handle,
                                          const _MsgHeader *header,
                                          const CHAR *msg )
   {
      INT32 rc = SDB_OK ;

      // sys info request
      if ( (UINT32)MSG_SYSTEM_INFO_LEN == (UINT32)header->messageLength )
      {
         MsgSysInfoReply reply ;
         MsgSysInfoReply *pReply = &reply ;
         INT32 replySize = sizeof(reply) ;

         rc = msgBuildSysInfoReply ( (CHAR**)&pReply, &replySize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build sys info reply, rc: %d", rc ) ;
            rc = SDB_NET_BROKEN_MSG ;
            goto error ;
         }
         else
         {
            rc = _pRSManager->getAgent()->syncSendRaw( handle,
                                                       (const CHAR *)pReply,
                                                       (UINT32)replySize ) ;
            goto done ;
         }
      }

      // main cb msg
      if ( 0 == header->TID || ! IS_REPLY_TYPE( header->opCode ) )
      {
         rc = _postMsg( handle, header, msg ) ;
      }
      // session msg
      else
      {
         SDB_ASSERT( _pRSManager, "Remote Session Manager can't be NULL" ) ;
         rc = _pRSManager->pushMessage( handle, header ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Push message[%s] failed, rc: %d",
                    msg2String( header, MSG_MASK_ALL, 0 ).c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdRemoteMsgHandler::handleClose( const NET_HANDLE &handle,
                                           _MsgRouteID id )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;
      _pRSManager->handleClose( handle, id ) ;

      MsgOpReply msg ;
      msg.contextID = -1 ;
      msg.flags = SDB_NETWORK_CLOSE ;
      msg.header.messageLength = sizeof( MsgOpReply ) ;
      msg.header.opCode = MSG_COM_REMOTE_DISC ;
      msg.header.requestID = 0 ;
      msg.header.routeID.value = id.value ;
      msg.header.TID = 0 ;
      msg.numReturned = 0 ;
      msg.startFrom = 0 ;

      _postMsg( handle, (_MsgHeader *)&msg ) ;
      PD_LOG ( PDDEBUG, "posting event handle close %u", (UINT32)handle ) ;
   }

   void _pmdRemoteMsgHandler::handleConnect( const NET_HANDLE &handle,
                                             _MsgRouteID id,
                                             BOOLEAN isPositive )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;

      _pRSManager->handleConnect( handle, id, isPositive ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDRMTMSGHDL__POSTMSG, "_pmdRemoteMsgHandler::_postMsg" )
   INT32 _pmdRemoteMsgHandler::_postMsg( const NET_HANDLE &handle,
                                         const MsgHeader *header,
                                         const CHAR *msg )
   {
      PD_TRACE_ENTRY ( SDB__PMDRMTMSGHDL__POSTMSG );

      CHAR *pNewMsg = NULL ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _pMainCB, "Main cb can't be NULL" ) ;
      if ( !_pMainCB )
      {
         PD_LOG( PDERROR, "Main cb handler is null when recv "
                 "msg[opCode:%d]", header->opCode ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pNewMsg = (CHAR*)SDB_THREAD_ALLOC( header->messageLength + 1 ) ;
      if ( !pNewMsg )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc memory for msg[opCode: %d, "
                 "len: %d], rc: %d", header->opCode, header->messageLength,
                 rc ) ;
         goto error ;
      }

      // copy msg
      if ( NULL == msg )
      {
         ossMemcpy( (void *)pNewMsg, header, header->messageLength ) ;
      }
      else
      {
         ossMemcpy( pNewMsg, msg, header->messageLength ) ;
      }
      pNewMsg[ header->messageLength ] = 0 ;

      // push event
      _pMainCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                        PMD_EDU_MEM_THREAD,
                                        pNewMsg,
                                        (UINT64)handle ) ) ;
   done:
      PD_TRACE_EXITRC ( SDB__PMDRMTMSGHDL__POSTMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   /*
      _pmdRemoteTimerHandler implement
   */
   _pmdRemoteTimerHandler::_pmdRemoteTimerHandler()
   {
      _pMainCB       = NULL ;
   }

   _pmdRemoteTimerHandler::~_pmdRemoteTimerHandler()
   {
   }

   void _pmdRemoteTimerHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB       = cb ;
   }

   void _pmdRemoteTimerHandler::detach()
   {
      _pMainCB       = NULL ;
   }

   void _pmdRemoteTimerHandler::handleTimeout( const UINT32 &millisec,
                                               const UINT32 &id )
   {
      if ( !_pMainCB )
      {
         return ;
      }
      PMD_EVENT_MESSAGES *eventMsg = (PMD_EVENT_MESSAGES *)
         SDB_THREAD_ALLOC( sizeof (PMD_EVENT_MESSAGES ) ) ;

      if ( NULL == eventMsg )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for PDM timeout Event" ) ;
      }
      else
      {
         ossTimestamp ts ;
         ossGetCurrentTime( ts ) ;

         eventMsg->timeoutMsg.interval = millisec ;
         eventMsg->timeoutMsg.occurTime = ts.time ;
         eventMsg->timeoutMsg.timerID = id ;

         _pMainCB->postEvent( pmdEDUEvent ( PMD_EDU_EVENT_TIMEOUT,
                                            PMD_EDU_MEM_THREAD,
                                            (void*)eventMsg) ) ;
      }
   }

}


