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

   Source File Name = pmdAsyncHandler.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_ASYNC_HANDLER_HPP_
#define PMD_ASYNC_HANDLER_HPP_

#include "netTimer.hpp"
#include "netMsgHandler.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _pmdAsyncTimerHandler define
   */
   class _pmdAsyncTimerHandler : public _netTimeoutHandler
   {
      public:
         _pmdAsyncTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_pmdAsyncTimerHandler () ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      public:
         OSS_INLINE void attach ( pmdEDUCB *cb ) { _pMgrCB = cb ; }
         OSS_INLINE void detach () { _pMgrCB = NULL ; }

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;

      protected:
         pmdEDUCB             *_pMgrCB ;
         _pmdAsycSessionMgr   *_pSessionMgr ;

   } ;
   typedef _pmdAsyncTimerHandler pmdAsyncTimerHandler ;

   /*
      _pmdAsyncMsgHandler define
   */
   class _pmdAsyncMsgHandler : public _netMsgHandler
   {
      public:
         _pmdAsyncMsgHandler( _pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_pmdAsyncMsgHandler () ;

         OSS_INLINE void attach( pmdEDUCB *cb ) { _pMgrEDUCB = cb; }
         OSS_INLINE void detach() { _pMgrEDUCB = NULL; }

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg );
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

         virtual void  onStop() ;

      protected:
         void* _copyMsg ( const CHAR* msg, UINT32 length ) ;

         INT32 _handleSessionMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg );

         INT32 _handleMainMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

         INT32 _handleSysInfo( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg ) ;

      protected:
         _pmdAsycSessionMgr   *_pSessionMgr ;
         pmdEDUCB             *_pMgrEDUCB ;

   } ;
   typedef _pmdAsyncMsgHandler pmdAsyncMsgHandler ;

}

#endif //PMD_ASYNC_HANDLER_HPP_

