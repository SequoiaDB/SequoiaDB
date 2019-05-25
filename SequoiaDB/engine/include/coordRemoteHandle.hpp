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

   Source File Name = coordRemoteHandle.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/17/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_REMOTE_HANDLE_HPP__
#define COORD_REMOTE_HANDLE_HPP__

#include "pmdRemoteSession.hpp"
#include "coordDef.hpp"

namespace engine
{

   /*
      _coordRemoteHandlerBase define
   */
   class _coordRemoteHandlerBase : public _IRemoteSessionHandler
   {
      public:
         _coordRemoteHandlerBase() ;
         virtual ~_coordRemoteHandlerBase() ;

      public:
         virtual INT32  onSendFailed( _pmdRemoteSession *pSession,
                                      _pmdSubSession **ppSub,
                                      INT32 flag ) ;

         /*
            include disconnect: MSG_BS_DISCONNECT or isDisconnect()
         */
         virtual void   onReply( _pmdRemoteSession *pSession,
                                 _pmdSubSession **ppSub,
                                 const MsgHeader *pReply,
                                 BOOLEAN isPending ) ;

         /*
            if return SDB_OK, will continue
            else, send failed
         */
         virtual INT32  onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst ) ;

      protected:

         INT32          _sessionInit( _pmdRemoteSession *pSession,
                                      const MsgRouteID &nodeID,
                                      _pmdEDUCB *cb ) ;
   } ;
   typedef _coordRemoteHandlerBase coordRemoteHandlerBase ;

   /*
      _coordNoSessionInitHandler define
   */
   class _coordNoSessionInitHandler : public _coordRemoteHandlerBase
   {
      public:
         _coordNoSessionInitHandler() ;
         virtual ~_coordNoSessionInitHandler() ;

      public:
         /*
            if return SDB_OK, will continue
            else, send failed
         */
         virtual INT32  onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst ) ;
   } ;
   typedef _coordNoSessionInitHandler coordNoSessionInitHandler ;

   /*
      _coordRemoteHandler define
   */
   class _coordRemoteHandler : public _coordRemoteHandlerBase
   {
      public:
         _coordRemoteHandler() ;
         virtual ~_coordRemoteHandler() ;

         void     enableInterruptWhenFailed( BOOLEAN enable,
                                             const SET_RC *pIgnoreRC = NULL ) ;

      public:

         /*
            include disconnect: MSG_BS_DISCONNECT or isDisconnect()
         */
         virtual void   onReply( _pmdRemoteSession *pSession,
                                 _pmdSubSession **ppSub,
                                 const MsgHeader *pReply,
                                 BOOLEAN isPending ) ;

         virtual INT32  onExpiredReply ( pmdRemoteSessionSite *pSite,
                                         const MsgHeader *pReply ) ;

      protected:
         BOOLEAN        _interruptWhenFailed ;
         SET_RC         _ignoreRC ;

   } ;
   typedef _coordRemoteHandler coordRemoteHandler ;

}

#endif // COORD_REMOTE_HANDLE_HPP__
