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
   // Remote session handler. Used by _rtnContextTS.
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

   class _rtnRemoteMessenger ;

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

   class _rtnRemoteSiteHandle : public IRemoteSiteHandle
   {
   public:
      _rtnRemoteSiteHandle() {}
      virtual ~_rtnRemoteSiteHandle() {}

      BOOLEAN waitEvent( pmdEDUEvent &event, INT64 timeout ) ;
      void postEvent( const pmdEDUEvent &event ) ;
   private:
      ossQueue<pmdEDUEvent> _queue ;
   } ;
   typedef _rtnRemoteSiteHandle rtnRemoteSiteHandle ;

   class _rtnRemoteMgrHandle : public IRemoteMgrHandle
   {
   public:
      _rtnRemoteMgrHandle() {}
      virtual ~_rtnRemoteMgrHandle() {}

      void onRegister( _pmdRemoteSessionSite *pSite, _pmdEDUCB *cb ) ;
      void onUnreg( _pmdRemoteSessionSite *pSite, _pmdEDUCB *cb ) ;
   } ;
   typedef _rtnRemoteMgrHandle rtnRemoteMgrHandle ;

   // A messenger for communication with remote target, the search engine
   // adapter, for example.
   class _rtnRemoteMessenger : public SDBObject
   {
   public:
      _rtnRemoteMessenger() ;
      ~_rtnRemoteMessenger() ;

      INT32 init() ;
      INT32 active() ;
      void deactive() ;
      INT32 setTarget( const _MsgRouteID &id, const CHAR *host,
                       const CHAR *service ) ;
      INT32 setLocalID( const MsgRouteID &id ) ;

      INT32 prepareSession( pmdEDUCB *cb, UINT64 &sessionID ) ;
      INT32 removeSession( pmdEDUCB *cb, UINT64 *sessionID = NULL ) ;
      INT32 send( UINT64 sessionID, const MsgHeader *msg, pmdEDUCB *cb ) ;
      INT32 receive( UINT64 sessionID, pmdEDUCB *cb, MsgOpReply *&reply ) ;

      // If disconnect with adapter, disable the messenger.
      void onDisconnect() ;

      OSS_INLINE BOOLEAN isReady()
      {
         ossScopedRWLock lock( &_lock, SHARED ) ;
         return _ready ;
      }

   private:
      rtnRemoteMgrHandle   _rMgrHandle ;
      pmdRemoteSessionMgr  _rsMgr ;
      rtnMsgHandler        _msgHandler ;
      netRouteAgent        _routeAgent ;
      rtnRSHandler         _rsHandler ;
      ossRWMutex           _lock ;
      BOOLEAN              _ready ;
      UINT64               _targetNodeID ;
   } ;
   typedef _rtnRemoteMessenger rtnRemoteMessenger ;
}

#endif /* RTN_REMOTEMESSENGER_HPP__ */

