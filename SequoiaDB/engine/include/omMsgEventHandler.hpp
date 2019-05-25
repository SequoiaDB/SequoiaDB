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

   Source File Name = omMsgEventHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_MSG_EVENT_HANDLER_HPP__
#define OM_MSG_EVENT_HANDLER_HPP__

#include "omDef.hpp"
#include "netMsgHandler.hpp"
#include "netTimer.hpp"

namespace engine
{
   class _pmdRemoteSessionMgr ;
   class _pmdEDUCB ;

   /*
      _omMsgHandler define
   */
   class _omMsgHandler : public _netMsgHandler
   {
      public:
         _omMsgHandler( _pmdRemoteSessionMgr *pRSManager ) ;
         virtual ~_omMsgHandler() ;

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
         _pmdRemoteSessionMgr                *_pRSManager ;
         _pmdEDUCB                           *_pMainCB ;

   } ;
   typedef _omMsgHandler omMsgHandler ;

   /*
      _omTimerHandler define
   */
   class _omTimerHandler : public _netTimeoutHandler
   {
      public:
         _omTimerHandler() ;
         virtual ~_omTimerHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      private:
         _pmdEDUCB               *_pMainCB ;

   } ;
   typedef _omTimerHandler omTimerHandler ;

}

#endif // OM_MSG_EVENT_HANDLER_HPP__

