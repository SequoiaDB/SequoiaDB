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

   Source File Name = clsMsgHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_MSG_HANDLER_HPP_
#define CLS_MSG_HANDLER_HPP_

#include "clsBase.hpp"
#include "pmdAsyncHandler.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include <vector>

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _shdMsgHandler define
   */
   class _shdMsgHandler : public _pmdAsyncMsgHandler
   {
      typedef ossPoolSet< ossEvent* >              SET_EVENTS ;
      typedef ossPoolMap< NET_HANDLE, SET_EVENTS > MAP_NET_2_EVENTS ;
      typedef MAP_NET_2_EVENTS::iterator           MAP_NET_2_EVENTS_IT ;

      public:
         _shdMsgHandler( _pmdAsycSessionMgr *pSessionMgr,
                         _schedTaskAdapterBase *pTaskAdapter = NULL,
                         _pmdRemoteSessionMgr *pRemoteSessionMgr = NULL ) ;
         virtual ~_shdMsgHandler();

         OSS_INLINE void attachShardCB( pmdEDUCB *cb ) { _pShardCB = cb ; }
         OSS_INLINE void detachShardCB() { _pShardCB = NULL ; }

         virtual void  handleClose( const NET_HANDLE &handle,
                                    _MsgRouteID id ) ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg,
                                    pmdEDUMemTypes memType ) ;

      protected:
         pmdEDUCB             *_pShardCB ;

   } ;
   typedef _shdMsgHandler shdMsgHandler ;

   /*
      _replMsgHandler define
   */
   class _replMsgHandler : public _pmdAsyncMsgHandler
   {
      public:
         _replMsgHandler ( _pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_replMsgHandler () ;

         INT32 type () const ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg,
                                    pmdEDUMemTypes memType ) ;

   } ;
   typedef _replMsgHandler replMsgHandler ;

}

#endif //CLS_MSG_HANDLER_HPP_

