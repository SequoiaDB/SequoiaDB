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

   Source File Name = seAdptMsgHandler.cpp

   Descriptive Name = Search Engine Adapter Message Handler.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/

#include "seAdptMsgHandler.hpp"

namespace seadapter
{
   _indexMsgHandler::_indexMsgHandler( _pmdAsycSessionMgr *pSessionMgr )
   : _pmdAsyncMsgHandler( pSessionMgr )
   {
   }

   _indexMsgHandler::~_indexMsgHandler()
   {
   }

   void _indexMsgHandler::handleClose( const NET_HANDLE &handle,
                                       _MsgRouteID id )
   {
      if ( _pMgrEDUCB )
      {
         MsgOpReply *pMsg = NULL ;
         pMsg = ( MsgOpReply* )SDB_OSS_MALLOC( sizeof( MsgOpReply ) ) ;
         if ( !pMsg )
         {
            PD_LOG( PDERROR, "Alloc memory[size: %d] failed",
                    sizeof( MsgOpReply ) ) ;
         }
         else
         {
            pMsg->contextID = -1 ;
            pMsg->flags = SDB_NETWORK_CLOSE ;
            pMsg->header.messageLength = sizeof( MsgOpReply ) ;
            pMsg->header.opCode = MSG_COM_REMOTE_DISC ;
            pMsg->header.requestID = 0 ;
            pMsg->header.routeID.value = id.value ;
            pMsg->header.TID = 0 ;
            pMsg->numReturned = 0 ;
            pMsg->startFrom = 0 ;

            _pMgrEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                                PMD_EDU_MEM_ALLOC,
                                                pMsg, (UINT64)handle ) ) ;
         }
      }
   }
}

