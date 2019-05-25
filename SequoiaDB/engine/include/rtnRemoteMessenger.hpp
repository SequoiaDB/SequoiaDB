/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnRemoteMessenger.hpp

   Descriptive Name = RunTime Remote Messenger

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/12/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_REMOTEMESSENGER_HPP__
#define RTN_REMOTEMESSENGER_HPP__

#include "netMsgHandler.hpp"
#include "pmdRemoteSession.hpp"

namespace engine
{
   class _rtnRSHandler : public IRemoteSessionHandler
   {
      public:
         _rtnRSHandler() ;
         virtual ~_rtnRSHandler() ;
      public:
         virtual INT32  onSendFailed( _pmdRemoteSession *pSession,
                                      _pmdSubSession **ppSub,
                                      INT32 flag ) ;

         virtual void   onReply( _pmdRemoteSession *pSession,
                                 _pmdSubSession **ppSub,
                                 const MsgHeader *pReply,
                                 BOOLEAN isPending ) ;

         virtual INT32  onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst ) ;
   } ;
   typedef _rtnRSHandler rtnRSHandler ;

   class _rtnMsgHandler : public _netMsgHandler
   {
      public:
         _rtnMsgHandler( pmdRemoteSessionMgr *pRSManager ) ;
         virtual ~_rtnMsgHandler() ;

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;
         virtual void  handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive ) ;

      protected:
         _pmdRemoteSessionMgr    *_pRSManager ;
   } ;
   typedef _rtnMsgHandler rtnMsgHandler ;

   class _rtnRemoteMessenger : public SDBObject
   {
   public:
      _rtnRemoteMessenger() ;
      ~_rtnRemoteMessenger() ;

      INT32 init() ;
      INT32 active() ;
      INT32 setTarget( const _MsgRouteID &id, const CHAR *host,
                       const CHAR *service ) ;
      INT32 setLocalID( const MsgRouteID &id ) ;

      INT32 prepareSession( pmdEDUCB *cb, UINT64 &sessionID ) ;
      INT32 send( UINT64 sessionID, const MsgHeader *msg, pmdEDUCB *cb ) ;
      INT32 receive( UINT64 sessionID, pmdEDUCB *cb, MsgOpReply *&reply ) ;

      INT32 removeSession( UINT64 sessionID, pmdEDUCB *cb ) ;
      void removeSession( pmdEDUCB *cb ) ;
      BOOLEAN isReady() const { return _ready ; }

   private:
      pmdRemoteSessionMgr  _rsMgr ;
      rtnMsgHandler        _msgHandler ;
      netRouteAgent        _routeAgent ;
      rtnRSHandler         _rsHandler ;
      BOOLEAN              _ready ;
      UINT64               _targetNodeID ;
   } ;
   typedef _rtnRemoteMessenger rtnRemoteMessenger ;
}

#endif /* RTN_REMOTEMESSENGER_HPP__ */

