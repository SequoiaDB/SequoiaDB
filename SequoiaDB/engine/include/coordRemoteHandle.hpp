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
#include "rtnSessionProperty.hpp"

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #pragma pack(4)

   /*
      _coordRemoteHandleStatus define
   */
   struct _coordRemoteHandleStatus
   {
      UINT16               _initFinished ;
      UINT8                _initTrans ;
      UINT64               _nodeVer ;
      UINT64               _nodeID ;

      _coordRemoteHandleStatus()
      {
         SDB_ASSERT( sizeof( *this ) <= PMD_SUBSESSION_UDF_DATA_LEN,
                     "Size must <= PMD_SUBSESSION_UDF_DATA_LEN" ) ;
         init() ;
      }
      void init()
      {
         _initFinished = TRUE ;
         _initTrans = FALSE ;
         _nodeVer = 0 ;
         _nodeID = 0 ;
      }
   } ;
   typedef _coordRemoteHandleStatus coordRemoteHandleStatus ;

   #pragma pack()

   /*
      Tool functions
   */
   INT32 coordBuildPacketMsg( _pmdSubSession *pSub,
                              MsgHeader *pHeader,
                              _pmdEDUCB *cb ) ;

   INT32 coordBuildPacketMsg( _pmdRemoteSession *pSession,
                              _pmdSubSession *pSub,
                              MsgHeader *pHeader ) ;

   /*
      _coordRemoteHandlerBase define
   */
   class _coordRemoteHandlerBase : public _IRemoteSessionHandler
   {
      public:
         enum SESSION_INIT_TYPE
         {
            INIT_V0,
            INIT_V1
         } ;
      public:
         _coordRemoteHandlerBase() ;
         virtual ~_coordRemoteHandlerBase() ;

         BOOLEAN  isVersion0() const ;

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

         virtual INT32  onExpiredReply ( _pmdRemoteSessionSite *pSite,
                                         const MsgHeader *pReply ) ;

         /*
            if return SDB_OK, will continue
            else, send failed
         */
         virtual INT32  onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst ) ;

         virtual INT32  onSend( _pmdRemoteSession *pSession,
                                _pmdSubSession *pSub ) ;

         virtual void   onHandleClose( _pmdRemoteSessionSite *pSite,
                                       NET_HANDLE handle,
                                       const MsgHeader *pReply ) ;

         virtual void   setUserData( UINT64 data ) ;

         virtual BOOLEAN canReconnect ( _pmdRemoteSession * session,
                                        _pmdSubSession * subSession ) ;

      protected:

         INT32          _sessionInit( _pmdRemoteSession *pSession,
                                      const MsgRouteID &nodeID,
                                      _pmdEDUCB *cb ) ;

         INT32          _buildPacketWithUpdateSched( _pmdRemoteSession *pSession,
                                                     _pmdSubSession *pSub,
                                                     const BSONObj &objSched ) ;

         INT32          _buildPacketWithSessionInit( _pmdRemoteSession *pSession,
                                                     _pmdSubSession *pSub,
                                                     BOOLEAN isUpdate ) ;

         INT32          _onSendConnectOld( _pmdSubSession *pSub ) ;

      private:
         BSONObj        _buildSessionInitObj( _pmdEDUCB *cb ) ;

         INT32          _checkSessionTransaction( _pmdRemoteSession *pSession,
                                                  _pmdSubSession *pSub,
                                                  _pmdEDUCB *cb,
                                                  coordRemoteHandleStatus *pStatus ) ;

         INT32          _checkSessionSchedInfo( _pmdRemoteSession *pSession,
                                                _pmdSubSession *pSub,
                                                _pmdEDUCB *cb,
                                                UINT32 &nodeSiteVer ) ;

         INT32          _checkSessionAttr( _pmdRemoteSession *pSession,
                                           _pmdSubSession *pSub,
                                           _pmdEDUCB *cb,
                                           UINT32 &nodeSiteVer ) ;

         INT32          _checkDSSessionAttr( _pmdSubSession *pSub,
                                             _pmdEDUCB *cb,
                                             UINT32 &nodeSiteVer ) ;

         INT32          _setSessionAttr( _pmdSubSession *pSub,
                                         const rtnSessionProperty &property,
                                         _pmdEDUCB *cb,
                                         BOOLEAN compatibleMode = TRUE ) ;

         INT32          _setSessionAttr( _pmdSubSession *pSub,
                                         const BSONObj &attrObj,
                                         _pmdEDUCB *cb ) ;

         BOOLEAN        _isSupportedDSSessoinAttr( const CHAR *attrName ) ;

      private:
         SESSION_INIT_TYPE    _initType ;

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

      protected:
         BOOLEAN        _interruptWhenFailed ;
         SET_RC         _ignoreRC ;

   } ;
   typedef _coordRemoteHandler coordRemoteHandler ;

}

#endif // COORD_REMOTE_HANDLE_HPP__
