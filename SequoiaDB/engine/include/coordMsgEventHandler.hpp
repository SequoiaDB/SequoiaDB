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

   Source File Name = coordMsgEventHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_MSG_EVENT_HANDLER_HPP__
#define COORD_MSG_EVENT_HANDLER_HPP__

#include "netMsgHandler.hpp"
#include "netTimer.hpp"

namespace engine
{
   class _pmdRemoteSessionMgr ;
   class _pmdEDUCB ;

   /*
      _omMsgHandler define
   */
   class _coordMsgHandler : public _netMsgHandler
   {
      public:
         _coordMsgHandler( _pmdRemoteSessionMgr *pRSManager ) ;
         virtual ~_coordMsgHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;
         virtual void  handleConnect( const NET_HANDLE &handle,
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
   typedef _coordMsgHandler coordMsgHandler ;

   /*
      _coordTimerHandler define
   */
   class _coordTimerHandler : public _netTimeoutHandler
   {
      public:
         _coordTimerHandler() ;
         virtual ~_coordTimerHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      private:
         _pmdEDUCB               *_pMainCB ;

   } ;
   typedef _coordTimerHandler coordTimerHandler ;

}

#endif // COORD_MSG_EVENT_HANDLER_HPP__

