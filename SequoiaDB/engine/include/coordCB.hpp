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

   Source File Name = coordCB.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORDCB_HPP__
#define COORDCB_HPP__

#include "netRouteAgent.hpp"
#include "ossUtil.h"
#include "coordRemoteSession.hpp"
#include "pmdRemoteMsgEventHandler.hpp"
#include "sdbInterface.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "dpsLogWrapper.hpp"
#include "msgCatalog.hpp"
#include "pmdObjBase.hpp"
#include "rtn.hpp"
#include "clsRegAssit.hpp"

using namespace std ;

namespace engine
{
   /*
      _CoordCB define
   */
   class _CoordCB : public _pmdObjBase, public _IControlBlock
   {
      DECLARE_OBJ_MSG_MAP()

      typedef _utilMap< SINT64, UINT64, 20 >    CONTEXT_LIST ;

      public:
         _CoordCB() ;
         virtual ~_CoordCB() ;

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_COORD ; }
         virtual const CHAR* cbName() const { return "COORDCB" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         UINT32         setTimer( UINT32 milliSec ) ;
         void           killTimer( UINT32 timerID ) ;

         coordResource* getResource() ;
         netRouteAgent* getRouteAgent() ;
         pmdRemoteSessionMgr* getRSManager() ;

      protected:
         virtual void onTimer ( UINT64 timerID, UINT32 interval ) ;
         INT32 _reply ( const NET_HANDLE &handle,
                        MsgOpReply *pReply,
                        const CHAR *pReplyData = NULL,
                        UINT32 replyDataLen = 0 ) ;
         INT32 _sendRegisterMsg () ;
         INT32 _onCatRegisterRes ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
         INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *pMsg ) ;
         void _onMsgBegin( MsgHeader *pMsg ) ;
         void _onMsgEnd() ;
         INT32 _processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
         INT32 _processGetMoreMsg ( MsgHeader *pMsg,
                                    rtnContextBuf &buffObj,
                                    INT64 &contextID ) ;
         INT32 _processKillContext( MsgHeader *pMsg ) ;
         INT32 _processRemoteDisc( const NET_HANDLE &handle,
                                   MsgHeader *pMsg ) ;
         INT32 _processMsgReq( MsgHeader *pMsg ) ;
         INT32 _filterQueryCmd( _rtnCommand *pCommand,
                                MsgHeader *pMsg ) ;
         INT32 _processQueryMsg( MsgHeader *pMsg,
                                 rtnContextBuf &buffObj,
                                 INT64 &contextID ) ;
         INT32 _processSessionInit( MsgHeader *pMsg ) ;
         INT32 _processInterruptMsg( const NET_HANDLE & handle,
                                     MsgHeader * header ) ;
         INT32 _processDisconnectMsg( const NET_HANDLE & handle,
                                      MsgHeader * header ) ;
         void _delContextByHandle( const UINT32 &handle ) ;
         void _delContext( const UINT32 &handle, UINT32 tid ) ;
         void _delContextByID( INT64 contextID, BOOLEAN rtnDel ) ;
         void _addContext( const UINT32 &handle, UINT32 tid, INT64 contextID ) ;
         INT32 _sendToCatlog ( MsgHeader *pMsg, NET_HANDLE *pHandle = NULL ) ;
         INT32 _processUpdateGrpInfo () ;
         INT32 _processCatGrpChgNty () ;
      private:

         coordResource                 _resource ;
         pmdRemoteSessionMgr           _remoteSessionMgr ;
         coordSessionPropMgr           _sitePropMgr ;

         pmdRemoteMsgHandler           *_pMsgHandler ;
         pmdRemoteTimerHandler         *_pTimerHandler ;
         netRouteAgent                 *_pAgent ;

         UINT16                        _shardServiceID ;
         CHAR                          _shdServiceName[OSS_MAX_SERVICENAME+1] ;
         _MsgRouteID                   _selfNodeID ;

         UINT64                        _regTimerID ;

         ossEvent                      _attachEvent ;

         const CHAR                    *_pCollectionName ;
         std::string                   _cmdCollectionName ;

         _SDB_DMSCB                    *_pDmsCB;
         _dpsLogWrapper                *_pDpsCB;
         _SDB_RTNCB                    *_pRtnCB;
         pmdEDUCB                      *_pEDUCB;

         ossSpinXLatch                 _contextLatch ;
         CONTEXT_LIST                  _contextLst;

         MsgOpReply                    _replyHeader ;
         BOOLEAN                       _needReply ;
         BSONObj                       _errorInfo ;
   } ;
   typedef _CoordCB CoordCB ;

   /*
      get global coord cb
   */
   CoordCB* sdbGetCoordCB() ;

}

#endif // COORDCB_HPP__

