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

   Source File Name = omMsgEventHandler.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "omMsgEventHandler.hpp"
#include "pmdEDU.hpp"
#include "pmdRemoteSession.hpp"
#include "msgMessageFormat.hpp"

namespace engine
{

   /*
      _omMsgHandler implement
   */
   _omMsgHandler::_omMsgHandler( _pmdRemoteSessionMgr *pRSManager )
   {
      _pRSManager       = pRSManager ;
      _pMainCB          = NULL ;
   }

   _omMsgHandler::~_omMsgHandler()
   {
      _pRSManager       = NULL ;
   }

   void _omMsgHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB    = cb ;
   }

   void _omMsgHandler::detach()
   {
      _pMainCB    = NULL ;
   }

   INT32 _omMsgHandler::handleMsg( const NET_HANDLE &handle,
                                   const _MsgHeader *header,
                                   const CHAR *msg )
   {
      INT32 rc = SDB_OK ;

      if ( header->TID == 0 )
      {
         CHAR *pNewMsg = NULL ;
         SDB_ASSERT( _pMainCB, "Main cb can't be NULL" ) ;
         if ( !_pMainCB )
         {
            PD_LOG( PDERROR, "Main cb handler is null when recv "
                    "msg[opCode:%d]", header->opCode ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         pNewMsg = (CHAR*)SDB_OSS_MALLOC( header->messageLength + 1 ) ;
         if ( !pNewMsg )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc memory for msg[opCode: %d, "
                    "len: %d], rc: %d", header->opCode, header->messageLength,
                    rc ) ;
            goto error ;
         }

         ossMemcpy( pNewMsg, msg, header->messageLength ) ;
         pNewMsg[ header->messageLength ] = 0 ;
         _pMainCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                           PMD_EDU_MEM_ALLOC,
                                           pNewMsg, (UINT64)handle ) ) ;
      }
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

   void _omMsgHandler::handleClose( const NET_HANDLE &handle, _MsgRouteID id )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;

      _pRSManager->handleClose( handle, id ) ;
   }

   void _omMsgHandler::handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;

      _pRSManager->handleConnect( handle, id, isPositive ) ;
   }

   /*
      _omTimerHandler implement
   */
   _omTimerHandler::_omTimerHandler()
   {
      _pMainCB       = NULL ;
   }

   _omTimerHandler::~_omTimerHandler()
   {
   }

   void _omTimerHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB       = cb ;
   }

   void _omTimerHandler::detach()
   {
      _pMainCB       = NULL ;
   }

   void _omTimerHandler::handleTimeout( const UINT32 &millisec,
                                        const UINT32 &id )
   {
      if ( !_pMainCB )
      {
         return ;
      }
      PMD_EVENT_MESSAGES *eventMsg = (PMD_EVENT_MESSAGES *)
         SDB_OSS_MALLOC( sizeof (PMD_EVENT_MESSAGES ) ) ;

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
                                            PMD_EDU_MEM_ALLOC,
                                            (void*)eventMsg) ) ;
      }
   }

}


