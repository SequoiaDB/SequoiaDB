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

   Source File Name = pmdRemoteMsgEventHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_REMOTE_MSG_EVENT_HANDLER_HPP__
#define PMD_REMOTE_MSG_EVENT_HANDLER_HPP__

#include "netMsgHandler.hpp"
#include "netTimer.hpp"

namespace engine
{
   class _pmdRemoteSessionMgr ;
   class _pmdEDUCB ;

   /*
      _pmdRemoteMsgHandler define
   */
   class _pmdRemoteMsgHandler : public _netMsgHandler
   {
      public:
         _pmdRemoteMsgHandler( _pmdRemoteSessionMgr *pRSManager ) ;
         virtual ~_pmdRemoteMsgHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;
         virtual INT32 handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive ) ;
      protected:
         INT32 _postMsg( const NET_HANDLE &handle,
                         const MsgHeader *header,
                         const CHAR *msg = NULL ) ;

      protected:
         _pmdRemoteSessionMgr                *_pRSManager ;
         _pmdEDUCB                           *_pMainCB ;

   } ;
   typedef _pmdRemoteMsgHandler pmdRemoteMsgHandler ;

   /*
      _pmdRemoteTimerHandler define
   */
   class _pmdRemoteTimerHandler : public _netTimeoutHandler
   {
      public:
         _pmdRemoteTimerHandler() ;
         virtual ~_pmdRemoteTimerHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      private:
         _pmdEDUCB               *_pMainCB ;

   } ;
   typedef _pmdRemoteTimerHandler pmdRemoteTimerHandler ;

}

#endif // PMD_REMOTE_MSG_EVENT_HANDLER_HPP__

