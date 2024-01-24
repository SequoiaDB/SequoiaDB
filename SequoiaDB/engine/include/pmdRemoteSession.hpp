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

   Source File Name = pmdRemoteSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_REMOTE_SESSION_HPP_
#define PMD_REMOTE_SESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdDef.hpp"
#include "utilPooledObject.hpp"
#include "sdbInterface.hpp"
#include "netRouteAgent.hpp"

#include "ossMemPool.hpp"
#include "../bson/bson.h"
#include "pmdRemoteConnection.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   class _pmdRemoteSession ;
   class _pmdRemoteSessionMgr ;
   class _pmdRemoteSessionSite ;
   class _pmdSubSession ;
   class _pmdEDUCB ;

   /*
      _IRemoteMsgConvertor define
   */
   class _IRemoteMsgConvertor : public SDBObject
   {
      public:
         _IRemoteMsgConvertor() {}
         virtual ~_IRemoteMsgConvertor() {}

      public:
         virtual INT32 filter( _pmdSubSession *pSub,
                               _pmdEDUCB *cb,
                               BOOLEAN &ignore,
                               UINT32 &oprMask ) = 0 ;

         virtual INT32 checkPermission( _pmdSubSession *pSub,
                                        UINT32 oprMask,
                                        _pmdEDUCB *cb ) = 0 ;

         virtual INT32 convert( _pmdSubSession *pSub,
                                _pmdEDUCB *cb ) = 0 ;

         virtual INT32 convertReply( _pmdSubSession *pSub,
                                     _pmdEDUCB *cb,
                                     const pmdEDUEvent &orgEvent,
                                     pmdEDUEvent &newEvent,
                                     BOOLEAN &hasConvert ) = 0 ;
   } ;
   typedef _IRemoteMsgConvertor IRemoteMsgConvertor ;

   /*
      _IRemoteSessionHandler define
   */
   class _IRemoteSessionHandler
   {
      public:
         _IRemoteSessionHandler() {}
         virtual ~_IRemoteSessionHandler() {}

      public:
         virtual INT32  onSendFailed( _pmdRemoteSession *pSession,
                                      _pmdSubSession **ppSub,
                                      INT32 flag ) = 0 ;

         /*
            include disconnect: MSG_BS_DISCONNECT or isDisconnect()
         */
         virtual void   onReply( _pmdRemoteSession *pSession,
                                 _pmdSubSession **ppSub,
                                 const MsgHeader *pReply,
                                 BOOLEAN isPending ) = 0 ;

         /*
            if return SDB_OK, will continue
            else, send failed
         */
         virtual INT32  onSendConnect( _pmdSubSession *pSub,
                                       const MsgHeader *pReq,
                                       BOOLEAN isFirst ) = 0 ;

         virtual INT32  onSend( _pmdRemoteSession *pSession,
                                _pmdSubSession *pSub )
         {
            return SDB_OK ;
         }

         virtual INT32  onExpiredReply ( _pmdRemoteSessionSite *pSite,
                                         const MsgHeader *pReply )
         {
            return SDB_OK ;
         }

         virtual void   onHandleClose( _pmdRemoteSessionSite *pSite,
                                       NET_HANDLE handle,
                                       const MsgHeader *pReply )
         {
         }

         virtual void   setUserData( UINT64 data ) {}

         virtual BOOLEAN canReconnect ( _pmdRemoteSession * session,
                                        _pmdSubSession * subSession )
         {
            return TRUE ;
         }
   } ;
   typedef _IRemoteSessionHandler IRemoteSessionHandler ;

   /*
      _IRemoteSiteHandle define
      User may use self defined message queue, instead of the queue of the
      thread.
   */
   class _IRemoteSiteHandle : public utilPooledObject
   {
      public:
         _IRemoteSiteHandle() {}
         virtual ~_IRemoteSiteHandle() {}

         virtual BOOLEAN   waitEvent( pmdEDUEvent &event, INT64 timeout ) = 0 ;
         virtual void      postEvent( const pmdEDUEvent &event ) = 0 ;
   } ;
   typedef _IRemoteSiteHandle IRemoteSiteHandle ;

   /*
      _IRemoteMsgPreprocessor define
   */
   class _IRemoteMsgPreprocessor
   {
      public:
         _IRemoteMsgPreprocessor() {}
         virtual ~_IRemoteMsgPreprocessor() {}
         virtual BOOLEAN preProcess( pmdEDUEvent &event ) = 0 ;
   } ;
   typedef _IRemoteMsgPreprocessor IRemoteMsgPreprocessor ;

   /*
      _IRemoteMgrHandle define
   */
   class _IRemoteMgrHandle : public SDBObject
   {
      public:
         _IRemoteMgrHandle() {}
         virtual ~_IRemoteMgrHandle() {}

      public:
         virtual void   onRegister( _pmdRemoteSessionSite *pSite,
                                    _pmdEDUCB *cb ) = 0 ;
         virtual void   onUnreg( _pmdRemoteSessionSite *pSite,
                                 _pmdEDUCB *cb ) = 0 ;
   } ;
   typedef _IRemoteMgrHandle IRemoteMgrHandle ;

   /*
      _IRemoteMgrControl define
   */
   class _IRemoteMgrControl : public SDBObject
   {
      public:
         _IRemoteMgrControl() {}
         virtual ~_IRemoteMgrControl() {}

      public:
         virtual void   checkSubSession( const MsgRouteID &routeID,
                                         netRouteAgent **ppAgent,
                                         IRemoteMsgConvertor **ppMsgConvertor,
                                         BOOLEAN &isExternConn ) = 0 ;

         virtual BOOLEAN canErrFilterOut( _pmdSubSession *pSub,
                                          const MsgHeader *pOrgMsg,
                                          INT32 err ) = 0 ;

         virtual BOOLEAN canSwitchOtherNode( INT32 err ) = 0 ;

   } ;
   typedef _IRemoteMgrControl IRemoteMgrControl ;

   #define PMD_SUBSESSION_UDF_DATA_LEN          ( 24 )
   /*
      _pmdSubSession define
   */
   class _pmdSubSession : public SDBObject
   {
      friend class _pmdRemoteSession ;
      friend class _pmdRemoteSessionSite ;

      public:
         _pmdSubSession() ;
         ~_pmdSubSession() ;

         _pmdRemoteSession* parent() { return _parent ; }
         pmdRemoteConnection* getConnection() ;
         // Req Msg
         void        setReqMsg( MsgHeader *pReqMsg,
                                pmdEDUMemTypes memType = PMD_EDU_MEM_NONE ) ;
         void        setReqMsgMemType( pmdEDUMemTypes memType ) { _memType = memType ; }
         MsgHeader*  getReqMsg() { return _pReqMsg ; }
         pmdEDUMemTypes getReqMemType() const { return _memType ; }
         void        clearRequestInfo() ;

         // IOVec
         netIOVec*   getIODatas() { return &_ioDatas ; }
         void        clearIODatas() { _ioDatas.clear() ; }
         void        addIODatas( const netIOVec &ioVec ) ;
         void        addIOData( const netIOV &io ) ;
         UINT32      getIODataLen() ;

         // Reply
         MsgHeader*  getRspMsg() ;
         pmdEDUEvent getOwnedRspMsg() ;
         void        clearReplyInfo() ;

         // Other
         UINT64      getNodeIDUInt() const { return _nodeID.value ; }
         MsgRouteID  getNodeID() const { return _nodeID ; }
         UINT64      getReqID() const { return _reqID ; }

         void        setUserData( UINT64 userData ) { _userData = userData ; }
         UINT64      getUserData() const { return _userData ; }

         CHAR*       getUDFData() { return _udfData ; }

         // Process
         void        setProcessInfo( INT32 processResult ) ;
         BOOLEAN     isProcessed() const { return _isProcessed ; }
         INT32       getProcessRet() const { return _processResult ; }
         void        clearProcessInfo() ;

         // Status
         BOOLEAN     isDisconnect() const { return _isDisconnect ; }
         BOOLEAN     isSend() const { return _isSend ; }
         BOOLEAN     hasReply() const { return _event._Data ? TRUE : FALSE ; }
         BOOLEAN     hasStop() const { return _hasStop ; }
         void        clearSend() { _isSend = FALSE ; }
         NET_HANDLE  getHandle() const { return _connection.getNetHandle() ; }
         INT32       getOrgReqOpCode() const { return _reqOpCode ; }
         INT32       getOrgRspOpCode() const { return _orgRspOpCode ; }

         BOOLEAN     sessionInitRequired() const ;

         INT32       preProcessMsg( _pmdEDUCB *cb, BOOLEAN &isIgnored ) ;

         // control
         void        resetForResend() ;

         BOOLEAN     canErrFilterOut( INT32 returnCode ) ;
         BOOLEAN     canSwitchOtherNode( INT32 returnCode ) ;

      protected:
         void        setParent( _pmdRemoteSession *parent ) { _parent = parent ; }
         void        setNodeID( UINT64 nodeID )
         {
            _nodeID.value = nodeID ;
         }
         void        setReqID( UINT64 reqID ) { _reqID = reqID ; }
         void        setSendResult( BOOLEAN isSend ) ;
         /**
          * Invoked when a reply message is received by the frame. Currently
          * only copy the message into local buffer.
          * @param event
          */
         void        processEvent( pmdEDUEvent &event ) ;
         void        setStop( BOOLEAN isStop ) { _hasStop = isStop ; }

         BOOLEAN     isNeedToDel() const { return _needToDel ; }
         void        setNeedToDel( BOOLEAN needToDel ) { _needToDel = needToDel ; }
         INT32*      getAddPos() { return &_addPos ; }

      protected:
         _pmdRemoteSession          *_parent ;
         MsgRouteID                 _nodeID ;
         UINT64                     _reqID ;
         BOOLEAN                    _isSend ;
         BOOLEAN                    _isDisconnect ;

         MsgHeader                  *_pReqMsg ;
         pmdEDUMemTypes             _memType ;
         netIOVec                   _ioDatas ;
         INT32                      _reqOpCode ;
         INT32                      _orgRspOpCode ;

         pmdEDUEvent                _event ;
         BOOLEAN                    _isProcessed ;
         INT32                      _processResult ;

         UINT64                     _userData ;
         CHAR                       _udfData[ PMD_SUBSESSION_UDF_DATA_LEN ] ;

         BOOLEAN                    _needToDel ;
         BOOLEAN                    _hasStop ;
         NET_HANDLE                 _handle ;
         INT32                      _addPos ;
         pmdRemoteConnection        _connection ;
         IRemoteMsgConvertor        *_pMsgConvertor ;
   } ;
   typedef _pmdSubSession pmdSubSession ;

   // Node ID and sub session mapping
   typedef ossPoolMap< UINT64, pmdSubSession >     MAP_SUB_SESSION ;
   typedef MAP_SUB_SESSION::iterator               MAP_SUB_SESSION_IT ;

   typedef ossPoolMap< UINT64, pmdSubSession* >    MAP_SUB_SESSIONPTR ;
   typedef MAP_SUB_SESSIONPTR::iterator            MAP_SUB_SESSIONPTR_IT ;

   typedef ossPoolVector< pmdSubSession* >         VEC_SUB_SESSIONPTR ;

   typedef ossPoolSet< UINT64 >                    SET_NODEID ;
   typedef ossPoolMap< UINT64, NET_HANDLE >        MAP_NODE2NET ;

   /*
      PMD_SUB_SESSION_FILTER define
   */
   enum PMD_SSITR_FILTER
   {
      PMD_SSITR_ALL           = 0,     // all sub sessions
      PMD_SSITR_UNSENT,                // not send or send failed
      PMD_SSITR_SENT,                  // send req succeed
      PMD_SSITR_UNREPLY,               // send req, but not reply
      PMD_SSITR_REPLY,                 // send req, and recv reply succeed
      PMD_SSITR_UNPROCESSED,           // recv reply, but not processed
      PMD_SSITR_PROCESSED,             // recv reply, and processed
      PMD_SSITR_PROCESS_SUC,           // has process, and result = SDB_OK
      PMD_SSITR_PROCESS_FAIL,          // has process, but result != SDB_OK
      PMD_SSITR_DISCONNECT,            // send, but disconnect
      PMD_SSITR_CONNECT                // has connected
   } ;

   /*
      _pmdSubSessionItr define
   */
   class _pmdSubSessionItr : public SDBObject
   {
      public:
         _pmdSubSessionItr() ;
         _pmdSubSessionItr( MAP_SUB_SESSION *pSessions,
                            PMD_SSITR_FILTER filter = PMD_SSITR_ALL ) ;
         ~_pmdSubSessionItr() ;

         BOOLEAN more() ;
         pmdSubSession* next() ;

      protected:
         void _findPos() ;

      private:
         MAP_SUB_SESSION            *_pSessions ;
         PMD_SSITR_FILTER           _filter ;
         MAP_SUB_SESSION_IT         _curPos ;

   } ;
   typedef _pmdSubSessionItr pmdSubSessionItr ;

   /*
    * Remote session is used to communicate with one or more remote nodes. It
    * may contain one or more sub sessions. It can send message to some or
    * all sub sessions, and get the reply by traverse the sub sessions.
   */
   class _pmdRemoteSession : public SDBObject
   {
      friend class _pmdRemoteSessionSite ;

      public:
         virtual ~_pmdRemoteSession() ;
         _pmdRemoteSession( netRouteAgent *pAgent,
                            UINT64 sessionID,
                            _pmdRemoteSessionSite *pSite,
                            INT64 timeout = -1,
                            IRemoteSessionHandler *pHandle = NULL ) ;
         _pmdRemoteSession() ;

         _pmdEDUCB* getEDUCB() { return _pEDUCB ; }
         UINT64     sessionID() const { return _sessionID ; }
         UINT64     getUserData() const { return _userData ; }

         _pmdRemoteSessionSite* getSite() { return _pSite ; }

         void setTimeout( INT64 timeout ) ;
         void setUserData( UINT64 userData ) { _userData = userData ; }

         pmdSubSessionItr getSubSessionItr( PMD_SSITR_FILTER filter =
                                            PMD_SSITR_ALL ) ;
         pmdSubSession* addSubSession( UINT64 nodeID ) ;
         pmdSubSession* getSubSession( UINT64 nodeID ) ;
         void           delSubSession( UINT64 nodeID ) ;
         void           clearSubSession() ;

         void           reConnectSubSession( UINT64 nodeID ) ;
         void           resetSubSession( UINT64 nodeID ) ;
         void           resetAllSubSession() ;
         /*
            Send MSG_BS_INTERRUPT_SELF to running sub sessions
         */
         void           stopSubSession() ;
         void           stopSubSession( pmdSubSession *pSub ) ;

         UINT32         getSubSessionCount( PMD_SSITR_FILTER filter =
                                            PMD_SSITR_ALL ) ;

         BOOLEAN        isTimeout() const ;
         BOOLEAN        isAllReply() ;

         INT64          getMilliTimeout () const ;
         INT64          getTotalWaitTime() const ;

      public:
         /*
            Send by sub session map and use the pSrcMsg.
            If the sub has sent, will not send.
            if send failed, will to call the handle callback
         */
         INT32    sendMsg( MsgHeader *pSrcMsg,
                           pmdEDUMemTypes memType = PMD_EDU_MEM_NONE,
                           INT32 *pSucNum = NULL,
                           INT32 *pTotalNum = NULL ) ;

         /*
            Send only to the subs and use the pSrcMsg.
            If this subs not exist, will added.
            If the subs has sent, will not send again.
            If failed, will to call the handle callback functions
         */
         INT32    sendMsg( MsgHeader *pSrcMsg,
                           SET_NODEID &subs,
                           pmdEDUMemTypes memType = PMD_EDU_MEM_NONE,
                           INT32 *pSucNum = NULL,
                           INT32 *pTotalNum = NULL ) ;

         /*
            Send by sub session map, if the sub has sent, will not send.
            if send failed, will to call the handle callback
         */
         INT32    sendMsg( INT32 *pSucNum = NULL,
                           INT32 *pTotalNum = NULL ) ;

         /*
            Send to special sub session, if the sub has sent, will not send.
            if send failed, doesn't to call the handle callback
         */
         INT32    sendMsg( UINT64 nodeID ) ;
         INT32    sendMsg( pmdSubSession *pSub ) ;

         INT32    waitReply1( BOOLEAN waitAll = FALSE,
                              MAP_SUB_SESSIONPTR *pSubs = NULL ) ;
         INT32    waitReply( BOOLEAN waitAll = FALSE,
                             VEC_SUB_SESSIONPTR *pSubs = NULL ) ;

         /*
            Post msg to specail sub session by net handle, is sub session
            net handle is invalid will return error,
            the msg will not save to pSub.
            For msg: MSG_BS_INTERRUPT, MSG_BS_INTERRUPT_SELF,
                     MSG_BS_DISCONNECT
         */
         INT32    postMsg( MsgHeader *pMsg, pmdSubSession *pSub ) ;
         INT32    postMsg( MsgHeader *pMsg, UINT64 nodeID ) ;

      protected:
         void     addPending( pmdSubSession *pSubSession ) ;

      private:
         void attachCB( _pmdEDUCB *cb ) { _pEDUCB = cb ; }
         void detachCB() { _pEDUCB = NULL ; }
         void reset( netRouteAgent *pAgent,
                     UINT64 sessionID,
                     _pmdRemoteSessionSite *pSite,
                     INT64 timeout = -1,
                     IRemoteSessionHandler *pHandle = NULL ) ;
         void clear() ;

      protected:
         MAP_SUB_SESSION               _mapSubSession ;
         /*
          * The remote session sites contains one or more remote sessions,
          * and they share the same message queue. When they wait on the same
          * queue, one remote session may find the reply of sub session of
          * another remote session. In that case, the sub session pointer
          * will be append to its real parent remote session's pending list.
          */
         MAP_SUB_SESSIONPTR            _mapPendingSubSession ;

         // Rmote session handler for this remote session.
         IRemoteSessionHandler         *_pHandle ;
         _pmdRemoteSessionSite         *_pSite ;
         netRouteAgent                 *_pAgent ;
         _pmdEDUCB                     *_pEDUCB ;
         // Sub sessions of this remote session have changed. It's used to count
         // sub session number which have not replied yet.
         BOOLEAN                       _sessionChange ;
         UINT64                        _sessionID ;
         INT64                         _milliTimeout ; // ms
         INT64                         _milliTimeoutHard ;  // ms
         INT64                         _totalWaitTime ;  // ms
         UINT64                        _userData ;
   } ;
   typedef _pmdRemoteSession pmdRemoteSession ;

   #define PMD_SITE_NODEID_BUFF_SIZE      ( 128 )
   /*
    * _pmdRemoteSessionSite define
    * One remote session site is for one session, eg: agent session on
    * coordinator. It may contain several remote sessions, if you execute
    * multiple operations in one session. One remote session may contain several
    * sub sessions, if the remote session needs to send messages to multiple
    *  nodes.
   */
   class _pmdRemoteSessionSite : public _IRemoteSite
   {
      friend class _pmdRemoteSessionMgr ;
      friend class _pmdRemoteSession ;

      typedef ossPoolMap< UINT64, pmdRemoteSession >  MAP_REMOTE_SESSION ;
      typedef MAP_REMOTE_SESSION::iterator            MAP_REMOTE_SESSION_IT ;

      /// High(32): SchedVersion + Low(32) : SessionVersion
      typedef ossPoolMap< UINT64, UINT64 >            MAP_NODE_2_VERSION ;
      typedef MAP_NODE_2_VERSION::iterator            MAP_NODE_2_VERSION_IT ;

      typedef ossPoolSet< UINT16 >                    SET_NODES ;

      struct posAndNode
      {
         INT16    _pos ;
         UINT16   _nodeID ;
      } ;

      private:
         void setEduCB( _pmdEDUCB *cb )
         {
            _pEDUCB = cb ;
            _hasImmediateRespEvents = FALSE ;
         }

         void setRouteAgent( netRouteAgent *pAgent ) { _pAgent = pAgent ; }
         void setMgrCtrl( IRemoteMgrControl *pCtrl ) { _pCtrl = pCtrl ; }

         void     handleClose( const NET_HANDLE &handle,
                               const _MsgRouteID &id ) ;

      public:
         virtual  UINT64   getUserData() const { return _userData ; }
         void              setUserData( UINT64 data ) { _userData = data ; }
         IRemoteMgrControl* getMgrControl() { return _pCtrl ; }

      public:
         virtual ~_pmdRemoteSessionSite() ;
         _pmdRemoteSessionSite() ;
         _pmdEDUCB* eduCB() { return _pEDUCB ; }
         void setHandle( IRemoteSiteHandle *pHandle ) { _pHandler = pHandle ; }
         IRemoteSiteHandle* getHandle() { return _pHandler ; }

         void setMsgPreprocessor( IRemoteMsgPreprocessor *pPreProcessor )
         {
            _pPreProcessor = pPreProcessor ;
         }
         IRemoteMsgPreprocessor* getPreprocessor() { return _pPreProcessor ; }

         void     interruptAllSubSession() ;
         void     disconnectAllSubSession() ;

         const MAP_NODE2NET& getAllNodesMap() { return _mapNode2Net ; }
         const MAP_NODE2NET* getAddNodesMapPtr() { return &_mapNode2Net ; }
         UINT32   getAllNodeID( SET_NODEID &setNodes ) ;
         UINT32   getAllNodeIDSize() const ;

         BOOLEAN  getNodeVer( UINT64 nodeID, UINT64 &ver ) const ;
         void     setNodeVer( UINT64 nodeID, UINT64 ver ) ;

         BOOLEAN  existHandle( const NET_HANDLE &handle ) ;

      public:

         pmdRemoteSession* addSession( INT64 timeout = -1, // ms
                                       IRemoteSessionHandler *pHandle = NULL ) ;
         pmdRemoteSession* getSession( UINT64 sessionID ) ;
         void              removeSession( UINT64 sessionID ) ;
         void              removeSession( pmdRemoteSession *pSession ) ;
         UINT32            sessionCount() ;

         INT32    checkImmediateRespEvents( IRemoteSessionHandler *pHandle ) ;

      protected:
         INT32    processEvent( pmdEDUEvent &event,
                                MAP_SUB_SESSION &mapSessions,
                                pmdSubSession **ppSub,
                                IRemoteSessionHandler *pHandle ) ;
         void     addSubSession( pmdSubSession *pSub ) ;
         void     delSubSession( UINT64 reqID ) ;

         void     addNodeNet( UINT64 nodeID, NET_HANDLE handle ) ;
         void     removeNodeNet( UINT64 nodeID ) ;
         NET_HANDLE getNodeNet( UINT64 nodeID ) ;

         INT32    addAssitNode( UINT16 nodeID ) ;
         void     removeAssitNode( INT32 *pos, UINT16 nodeID ) ;
         BOOLEAN  existNode( UINT16 nodeID ) ;

         void     _initRemoteConnection( pmdRemoteConnection *pConnection,
                                         UINT64 nodeID,
                                         NET_HANDLE handle ) ;
      private:
         // RequestID and sub session pointer mapping. By this we can find
         // the sub session by the request ID in the message. It contains all
         // sub sessions of all remote session in this site.
         MAP_SUB_SESSIONPTR   _mapReq2SubSession ;
         // Remote node id and net event handler map. Item will be added when
         // the first connection to that node is created by sending message
         // with the sub session.
         MAP_NODE2NET         _mapNode2Net ;
         _pmdEDUCB            *_pEDUCB ;
         netRouteAgent        *_pAgent ;
         IRemoteSiteHandle    *_pHandler ;
         IRemoteMsgPreprocessor *_pPreProcessor ;

         // last for free entry
         posAndNode           _assitNodeBuff[ PMD_SITE_NODEID_BUFF_SIZE + 1 ] ;
         SET_NODES            _assitNodes ;
         ossSpinSLatch        *_pLatch ;

         UINT32               _sessionHWNum ;
         MAP_REMOTE_SESSION   _mapSession ;
         pmdRemoteSession     _curSession ;
         INT32                _curPos ;

         UINT64               _userData ;

         MAP_NODE_2_VERSION   _mapNode2Ver ;
         IRemoteMgrControl    *_pCtrl ;

         // tell EDUCB to process events which requires immediately response
         volatile BOOLEAN     _hasImmediateRespEvents ;
   } ;
   typedef _pmdRemoteSessionSite pmdRemoteSessionSite ;

   /*
      _pmdRemoteSessionMgr define
   */
   class _pmdRemoteSessionMgr : public SDBObject
   {
      typedef ossPoolMap< UINT32, pmdRemoteSessionSite >    MAP_TID_2_EDU ;
      typedef MAP_TID_2_EDU::iterator                       MAP_TID_2_EDU_IT ;

      public:
         _pmdRemoteSessionMgr() ;
         ~_pmdRemoteSessionMgr() ;

         INT32       init( netRouteAgent *pAgent,
                           IRemoteMgrHandle *pHandle = NULL,
                           IRemoteMgrControl *pCtrl = NULL ) ;
         INT32       fini() ;

         netRouteAgent*          getAgent() ;

         pmdRemoteSessionSite*   registerEDU( _pmdEDUCB *cb ) ;
         void                    unregEUD( _pmdEDUCB *cb ) ;

         void                    setAllSiteSchedVer( INT32 ver ) ;

         pmdRemoteSessionSite*   getSite( _pmdEDUCB *cb ) ;
         pmdRemoteSessionSite*   getSite( UINT32 tid ) ;

         INT32       pushMessage( const NET_HANDLE &handle,
                                  const MsgHeader *pMsg ) ;
         void        handleClose( const NET_HANDLE &handle,
                                  const _MsgRouteID &id ) ;
         void        handleConnect( const NET_HANDLE &handle,
                                    _MsgRouteID id,
                                    BOOLEAN isPositive ) ;

      public:

         pmdRemoteSession* addSession( _pmdEDUCB *cb,
                                       INT64 timeout = -1, // ms
                                       IRemoteSessionHandler *pHandle = NULL ) ;
         pmdRemoteSession* getSession( UINT64 sessionID ) ;
         void              removeSession( UINT64 sessionID ) ;
         void              removeSession( pmdRemoteSession *pSession ) ;
         UINT32            sessionCount() ;

      private:
         IRemoteMgrHandle           *_pHandle ;
         IRemoteMgrControl          *_pCtrl ;
         netRouteAgent              *_pAgent ;
         MAP_TID_2_EDU              _mapTID2EDU ;
         ossSpinSLatch              _edusLatch ;
   } ;
   typedef _pmdRemoteSessionMgr pmdRemoteSessionMgr ;

}

#endif //PMD_REMOTE_SESSION_HPP_

