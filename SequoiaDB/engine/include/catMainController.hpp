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

   Source File Name = catMainController.hpp

   Descriptive Name = Process MoDel Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure kernel control block,
   which is the most critical data structure in the engine process.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_MAIN_CONTROLLER_HPP__
#define CAT_MAIN_CONTROLLER_HPP__

#include "pmdObjBase.hpp"
#include "netMsgHandler.hpp"
#include "netTimer.hpp"
#include "msgCatalog.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "ossEvent.hpp"
#include "catCatalogManager.hpp"
#include "catNodeManager.hpp"
#include "catEventHandler.hpp"
#include "utilMap.hpp"

#include <vector>

namespace engine
{

   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _authCB ;
   class sdbCatalogueCB ;

   enum CAT_DELAY_REPLY_TYPE
   {
      CAT_DELAY_REPLY_UNKNOWN = 0,

      CAT_DELAY_REPLY_SYNC,
   } ;

   #define CAT_MAX_DELAY_RETRY_TIMES         ( 100 )
   #define CAT_DEALY_TIME_INTERVAL           ( 100 ) // ms

   /*
      catMainController define
   */
   class catMainController : public _pmdObjBase, public _netMsgHandler,
                             public _netTimeoutHandler,
                             public _catEventHandler
   {
   typedef _utilMap< SINT64, UINT64, 20 >    CONTEXT_LIST ;
   typedef std::vector< pmdEDUEvent >        VEC_EVENT ;



   DECLARE_OBJ_MSG_MAP()

   public:
      catMainController() ;
      virtual ~catMainController() ;

      INT32 init() ;
      INT32 fini() ;

      virtual void   attachCB( _pmdEDUCB *cb ) ;
      virtual void   detachCB( _pmdEDUCB *cb ) ;

      virtual void   onTimer ( UINT64 timerID, UINT32 interval ) ;

      ossEvent* getAttachEvent() { return &_attachEvent ; }
      ossEvent* getChangeEvent() { return &_changeEvent ; }

      BOOLEAN   delayCurOperation ( UINT32 maxRetryTimes =
                                           CAT_MAX_DELAY_RETRY_TIMES ) ;
      BOOLEAN   isDelayed() const { return _isDelayed ; }

      void addContext( const UINT32 &handle, UINT32 tid, INT64 contextID ) ;
      void delContextByID( INT64 contextID, BOOLEAN rtnDel ) ;

   public:
      INT32 handleMsg( const NET_HANDLE &handle,
                       const _MsgHeader *header,
                       const CHAR *msg ) ;
      void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

      void  handleTimeout( const UINT32 &millisec, const UINT32 &id ) ;

   protected:
      virtual INT32 _defaultMsgFunc ( NET_HANDLE handle,
                                      MsgHeader* msg ) ;

      INT32 _processMsg ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      void _delayEvent ( pmdEDUEvent &event ) ;

      BOOLEAN _needCheckDelay () ;

      void _setCheckDelayTick () ;

      void _dispatchDelayedOperation ( BOOLEAN dispatch ) ;

      void _deleteDelayedOperation ( UINT32 handle ) ;

   protected:
      INT32 _onActiveEvent( pmdEDUEvent *event ) ;
      INT32 _onDeactiveEvent( pmdEDUEvent *event ) ;

   protected :
      INT32 _processGetMoreMsg ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processQueryMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processKillContext(const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthenticate( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthCrt( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthDel( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processSessionInit( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processRemoteDisc ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processInterruptMsg( const NET_HANDLE &handle,
                                  MsgHeader *header ) ;
      INT32 _processDisconnectMsg( const NET_HANDLE &handle,
                                   MsgHeader *header ) ;
      INT32 _processQueryRequest ( const NET_HANDLE &handle,
                                   MsgHeader *pMsg,
                                   const CHAR *pCollectionName ) ;
      INT32 _processDelayReply ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

   protected:
      INT32 _postMsg( const NET_HANDLE &handle, const MsgHeader *pHead ) ;
      INT32 _catBuildMsgEvent ( const NET_HANDLE &handle,
                                const MsgHeader *pMsg,
                                pmdEDUEvent &event ) ;
      INT32 _ensureMetadata() ;
      INT32 _createSysIndex ( const CHAR *pCollection,
                              const CHAR *pIndex,
                              pmdEDUCB *cb ) ;
      INT32 _createSysCollection ( const CHAR *pCollection,
                                   pmdEDUCB *cb ) ;
      void _delContextByHandle( const UINT32 &handle ) ;
      void _delContext( const UINT32 &handle, UINT32 tid ) ;

   public :
      virtual const CHAR *getHandlerName () { return "catMainController" ; }

      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) ;

      virtual INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) ;

      virtual INT32 onSendReply ( MsgOpReply *pReply, INT32 result ) ;

   public :
      virtual INT32 waitSync( const NET_HANDLE &handle, MsgOpReply *pReply,
                              void *pReplyData, UINT32 replyDataLen ) ;

   protected :
      INT32 _waitSyncInternal ( const NET_HANDLE &handle, BOOLEAN firstTry,
                                UINT64 syncLsn, INT16 w, MsgOpReply *pReply,
                                void *pReplyData = NULL,
                                UINT32 replyDataLen = 0 ) ;

      INT32 _delaySync ( const NET_HANDLE &handle, BOOLEAN firstTry,
                         UINT64 syncLsn, INT16 w,
                         MsgOpReply *pReply, void *pReplyData,
                         UINT32 replyDataLen ) ;

      INT32 _buildDelaySyncEvent ( CHAR **ppBuffer, INT32 *pBufferSize,
                                   UINT64 waitingLsn, INT16 w,
                                   MsgOpReply *pReply,
                                   void *pReplyData, UINT32 replyDataLen ) ;

      INT32 _buildDelayReplyEvent ( CHAR **ppBuffer, INT32 *pBufferSize,
                                    CAT_DELAY_REPLY_TYPE type,
                                    MsgOpReply *pReply, void *pReplyData,
                                    UINT32 replyDataLen,
                                    const BSONObj &boInfo ) ;

      INT32 _extractDelayReplyEvent ( const MsgOpReply *pDelayedReply,
                                      CAT_DELAY_REPLY_TYPE &type,
                                      MsgOpReply **ppReply, BSONObj &boInfo ) ;

   private :
      pmdEDUMgr         *_pEduMgr;
      sdbCatalogueCB    *_pCatCB;
      _SDB_DMSCB        *_pDmsCB;
      pmdEDUCB          *_pEDUCB;
      _SDB_RTNCB        *_pRtnCB;
      _dpsLogWrapper    *_pDpsCB;
      _authCB           *_pAuthCB;
      CONTEXT_LIST      _contextLst;
      ossSpinXLatch     _contextLatch ;

      ossEvent          _attachEvent ;

      ossEvent          _changeEvent ;

      VEC_EVENT         _vecEvent ;
      UINT32            _checkEventTimerID ;
      pmdEDUEvent       _lastDelayEvent ;
      BOOLEAN           _isDelayed ;
      BOOLEAN           _delayWithoutSync ;
      UINT64            _lastCheckDelayTick ;

   } ;

}

#endif // CAT_MAIN_CONTROLLER_HPP__

