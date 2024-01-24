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

   Source File Name = pmdAsyncSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_ASYNC_SESSION_HPP_
#define PMD_ASYNC_SESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdObjBase.hpp"
#include "pmdEDU.hpp"
#include "netRouteAgent.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"
#include "pmdSessionBase.hpp"
#include "pmdInnerClient.hpp"
#include "schedTaskMgr.hpp"

#include "ossMemPool.hpp"
#include <deque>

namespace engine
{

   /*
      Global Define
   */
   #define INVLIAD_SESSION_ID       (0)
   #define SESSION_NAME_LEN         (100)

   #define MAX_BUFFER_ARRAY_SIZE    (20)

   #define PMD_BASE_HANDLE_ID       ( DATA_NODE_ID_END + 10000 )

   #define PMD_BUFF_INVALID         (0)
   #define PMD_BUFF_ALLOC           (1)
   #define PMD_BUFF_USING           (2)
   #define PMD_BUFF_FREE            (3)

   enum PMD_SESSION_START_TYPE
   {
      PMD_SESSION_ACTIVE   = 1,  //active
      PMD_SESSION_PASSIVE  = 2   //passive
   };

   #define PMD_SESSION_MSG_INPOOL   ( 0 )
   #define PMD_SESSION_MSG_UNPOOL   ( 1 )

   #define PMD_MAKE_SESSION_USERDATA( netHandle, poolType ) \
      ( ( UINT64 )( netHandle ) | ( (UINT64)(poolType) << 63 ) )

   #define PMD_UNMAKE_SESSION_USERDATA( userData, netHandle, poolType ) \
      do { \
         netHandle = ( UINT64 )( userData ) & ~( (UINT64)1 << 63 ) ; \
         poolType = ( UINT64 )( userData ) >> 63 ; \
      } while( 0 )

   class _pmdAsycSessionMgr ;
   class _pmdSessionScopedHold ;

   /*
      _pmdBuffInfo define
   */
   class _pmdBuffInfo : public SDBObject
   {
   public :
      CHAR     *pBuffer ;
      UINT32   size ;
      INT32    useFlag ;
      UINT64   addTime ;   /// uses

      BOOLEAN isAlloc () { return useFlag == PMD_BUFF_ALLOC ? TRUE : FALSE ; }
      BOOLEAN isUsing () { return useFlag == PMD_BUFF_USING ? TRUE : FALSE ; }
      BOOLEAN isFree () { return useFlag == PMD_BUFF_FREE ? TRUE : FALSE ; }
      BOOLEAN isInvalid () { return useFlag == PMD_BUFF_INVALID ? TRUE : FALSE ; }

      void setFree () { useFlag = PMD_BUFF_FREE ; }
   } ;
   typedef class _pmdBuffInfo pmdBuffInfo ;

   /*
      _pmdSessionMeta define
   */
   class _pmdSessionMeta : public utilPooledObject
   {
      public:
         _pmdSessionMeta( const NET_HANDLE handle ) ;
         virtual ~_pmdSessionMeta() ;

         ossSpinXLatch* getLatch() { return &_Latch ; }
         UINT32         getBasedHandleNum()
         {
            return _basedHandleNum.fetch() ;
         }
         NET_HANDLE     getHandle() const { return _netHandle ; }

         void          incBaseHandleNum()
         {
            _basedHandleNum.inc() ;
         }
         void          decBaseHandleNum()
         {
            _basedHandleNum.dec() ;
         }

      private:
         ossSpinXLatch        _Latch ;
         ossAtomic32          _basedHandleNum ;
         NET_HANDLE           _netHandle ;

   } ;
   typedef _pmdSessionMeta pmdSessionMeta ;

   /*
      _pmdAsyncSession define
   */
   class _pmdAsyncSession : public _pmdObjBase, public _pmdSessionBase
   {
      friend class _pmdAsycSessionMgr ;
      friend class _pmdSessionScopedHold ;
      DECLARE_OBJ_MSG_MAP()

      public:
         _pmdAsyncSession( UINT64 sessionID ) ;
         virtual ~_pmdAsyncSession();

         virtual UINT64          identifyID() ;
         virtual MsgRouteID      identifyNID() ;
         virtual UINT32          identifyTID() ;
         virtual UINT64          identifyEDUID() ;

         virtual const CHAR*     sessionName() const ;
         virtual INT32           getServiceType() const ;
         virtual IClient*        getClient() { return &_client ; }
         virtual EDUID           eduID () const ;
         virtual pmdEDUCB*       eduCB () const ;

         virtual void*           getSchedItemPtr() ;
         virtual void            setSchedItemVer( INT32 ver ) ;

         virtual EDU_TYPES       eduType () const = 0 ;
         virtual const CHAR*     className() const = 0 ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         virtual BOOLEAN timeout ( UINT32 interval ) ;

         virtual void    onDispatchMsgBegin( const NET_HANDLE netHandle,
                                             const MsgHeader *pHeader )
         {
         }
         virtual void    onDispatchMsgEnd( INT64 costUsecs )
         {
         }

         virtual void clear() ;
         virtual BOOLEAN canAttachMeta() const { return TRUE ; }

         INT32 waitAttach ( INT64 millisec = -1 ) ;
         INT32 waitDetach ( INT64 millisec = -1 ) ;
         INT32 attachIn ( pmdEDUCB *cb ) ;
         INT32 attachOut () ;

         BOOLEAN isAttached () const ;
         BOOLEAN isDetached () const ;

         BOOLEAN isClosed() const ;
         void    close() ;

         void  getAuditConfig( UINT32 &auditMask, UINT32 &auditConfigMask ) ;
         void  setAuditConfig( UINT32 auditMask, UINT32 auditConfigMask ) ;

      public:
         UINT64      sessionID () const ;
         NET_HANDLE  netHandle () const ;
         BOOLEAN     isStartActive() ;
         pmdSessionMeta* getMeta() { return _pMeta ; }

         UINT32         getPendingMsgNum() ;
         UINT32         incPendingMsgNum() ;
         UINT32         decPendingmsgNum() ;

         BOOLEAN        hasHold() ;

         const schedInfo*  getSchedInfo() const ;

         BOOLEAN        isBufferFull() const ;
         BOOLEAN        isBufferEmpty() const ;

         void           setIdentifyInfo( UINT32 ip, UINT16 port, MsgRouteID nid,
                                         UINT32 tid, UINT64 eduID ) ;

      private:
         void        startType ( INT32 startType ) ;
         void        meta ( pmdSessionMeta * pMeta ) ;
         void        sessionID ( UINT64 sessionID ) ;
         void        setSessionMgr( _pmdAsycSessionMgr *pSessionMgr ) ;

         void        forceBack() ;

         pmdBuffInfo*   frontBuffer () ;
         void           popBuffer () ;
         INT32          pushBuffer ( CHAR *pBuffer, UINT32 size ) ;
         void*          copyMsg ( const CHAR *msg, UINT32 length ) ;

         UINT32         _incBuffPos ( UINT32 pos ) ;
         UINT32         _decBuffPos ( UINT32 pos ) ;

         void           _holdIn() ;
         void           _holdOut() ;

      protected:
         INT32 _lock () ;
         INT32 _unlock () ;
         void  _reset() ;

         netRouteAgent* routeAgent() ;

         virtual void   _makeName () ;

         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      protected:

         UINT64               _sessionID ;
         pmdEDUCB             *_pEDUCB ;
         EDUID                _eduID ;
         NET_HANDLE           _netHandle ;

         CHAR                 _name[SESSION_NAME_LEN+1] ;
         pmdSessionMeta       *_pMeta ;

         ossEvent             _detachEvent ;
         _pmdAsycSessionMgr   *_pSessionMgr ;
         pmdInnerClient       _client ;

         schedItem            _info ;

      private:
         ossEvent             _evtIn ;
         ossEvent             _evtOut ;
         BOOLEAN              _lockFlag ;
         INT32                _startType ;

         pmdBuffInfo          _buffArray[MAX_BUFFER_ARRAY_SIZE] ;
         UINT32               _buffBegin ;
         UINT32               _buffEnd ;
         UINT32               _buffCount ;

         /// identify info
         UINT64               _identifyID ;
         MsgRouteID           _identifyNID ;
         UINT32               _identifyTID ;
         UINT64               _identifyEDUID ;

         /// session audit config
         UINT32               _auditMask ;
         UINT32               _auditConfigMask ;

         ossAtomic32          _pendingMsgNum ;
         ossAtomic32          _holdCount ;
         BOOLEAN              _isClosed ;

   };
   typedef _pmdAsyncSession pmdAsyncSession ;

   /*
      _pmdAsycSessionMgr define
   */
   class _pmdAsycSessionMgr : public SDBObject
   {
      public:
      typedef ossPoolMap<UINT64, _pmdAsyncSession*>  MAPSESSION ;
      typedef MAPSESSION::iterator                         MAPSESSION_IT ;

      typedef ossPoolMap<NET_HANDLE, pmdSessionMeta*>  MAPMETA ;
      typedef MAPMETA::iterator                              MAPMETA_IT ;

      typedef std::deque<_pmdAsyncSession*>           DEQSESSION ;

      public:
         _pmdAsycSessionMgr() ;
         virtual ~_pmdAsycSessionMgr() ;

         virtual INT32        init( netRouteAgent *pRTAgent,
                                    _netTimeoutHandler *pTimerHandle,
                                    UINT32 timerInterval ) ;
         virtual INT32        fini() ;
         void                 setForced() { _quit = TRUE ; }
         netRouteAgent*       getRouteAgent() { return _pRTAgent ; }

         BOOLEAN              forceNotify( UINT64 sessionID,
                                           _pmdEDUCB *cb ) ;

         virtual void         onTimer( UINT32 interval ) ;

         INT32                dispatchMsg( const NET_HANDLE &handle,
                                           const MsgHeader *pMsg,
                                           pmdEDUMemTypes memType,
                                           BOOLEAN decPending,
                                           BOOLEAN *hasDispatched = NULL ) ;

         INT32                getSessionObj( UINT64 sessionID,
                                             BOOLEAN withHold,
                                             BOOLEAN bCreate,
                                             INT32 startType,
                                             const NET_HANDLE &handle,
                                             INT32 opCode,
                                             void *data,
                                             pmdAsyncSession **ppSession,
                                             BOOLEAN *pIsPending = NULL ) ;

         void                 holdOut( pmdAsyncSession *pSession ) ;

         INT32          getSession( UINT64 sessionID,
                                    BOOLEAN withHold,
                                    INT32 startType,
                                    const NET_HANDLE &handle,
                                    BOOLEAN bCreate,
                                    INT32 opCode,
                                    void *data,
                                    pmdAsyncSession **ppSession ) ;

         INT32          releaseSession( pmdAsyncSession *pSession,
                                        BOOLEAN delay = FALSE ) ;

         INT32          handleSessionClose( const NET_HANDLE handle ) ;

         void           handlePrepareStop() ;
         void           handleStop() ;

         virtual INT32  handleSessionTimeout( UINT32 timerID,
                                              UINT32 interval ) ;

      public:
         virtual UINT64       makeSessionID( const NET_HANDLE &handle,
                                             const MsgHeader *header ) = 0 ;

         /*
            Session distory callback functions
            Has hold the metaLatch
         */
         virtual void   onSessionDisconnect( pmdAsyncSession *pSession ) {}
         virtual void   onNoneSessionDisconnect( UINT64 sessionID ) {}
         virtual void   onSessionHandleClose( pmdAsyncSession *pSession ) {}
         virtual void   onSessionDestoryed( pmdAsyncSession *pSession ) {}

         virtual INT32  onErrorHanding( INT32 rc,
                                        const MsgHeader *pReq,
                                        const NET_HANDLE &handle,
                                        UINT64 sessionID,
                                        pmdAsyncSession *pSession ) = 0 ;

      protected:
         /*
            Parse the session type
         */
         virtual SDB_SESSION_TYPE   _prepareCreate( UINT64 sessionID,
                                                    INT32 startType,
                                                    INT32 opCode ) = 0 ;

         virtual BOOLEAN      _canReuse( SDB_SESSION_TYPE sessionType ) = 0 ;
         virtual UINT32       _maxCacheSize() const = 0 ;

         /*
            Create session
         */
         virtual pmdAsyncSession*  _createSession( SDB_SESSION_TYPE sessionType,
                                                   INT32 startType,
                                                   UINT64 sessionID,
                                                   void *data = NULL ) = 0 ;

         virtual void         _onSessionNew( pmdAsyncSession *pSession ) {}

      protected:
         /*
            Caller must hold the metaLatch
         */
         INT32          _attachSessionMeta( pmdAsyncSession *pSession,
                                            const NET_HANDLE handle ) ;

         INT32          _startSessionEDU( pmdAsyncSession * pSession ) ;

         /*
            Caller must hold the metaLatch
         */
         INT32          _releaseSession_i ( pmdAsyncSession *pSession,
                                            BOOLEAN postQuit,
                                            BOOLEAN delay ) ;
         INT32          _releaseSession( pmdAsyncSession *pSession ) ;

         INT32          _reply( const NET_HANDLE &handle, INT32 rc,
                                const MsgHeader *pReqMsg ) ;

         INT32          _pushMessage ( pmdAsyncSession *pSession,
                                       const MsgHeader *header,
                                       pmdEDUMemTypes memType,
                                       const NET_HANDLE &handle ) ;

      protected:
         void           _checkSession( UINT32 interval ) ;
         void           _checkSessionMeta( UINT32 interval ) ;
         void           _checkForceSession( UINT32 interval ) ;

      protected:
         MAPSESSION                 _mapSession ;
         MAPSESSION                 _mapPendingSession ;
         MAPMETA                    _mapMeta ;
         DEQSESSION                 _deqCacheSessions ;
         UINT32                     _cacheSessionNum ;

         ossSpinXLatch              _metaLatch ;

         // Delay delete sessions
         DEQSESSION                 _deqDeletingSessions ;
         ossSpinXLatch              _deqDeletingMutex ;

         netRouteAgent              *_pRTAgent ;
         _netTimeoutHandler         *_pTimerHandle ;

         UINT32                     _handleCloseTimerID ;
         UINT32                     _sessionTimerID ;
         UINT32                     _timerInterval ;

         // for _quit edu by edu mgr, not by session mgr
         UINT32                     _forceChecktimer ;
         std::deque< UINT64 >       _forceSessions ;
         ossSpinXLatch              _forceLatch ;
         BOOLEAN                    _isStop ;

         BOOLEAN                    _quit ;

   } ;
   typedef _pmdAsycSessionMgr pmdAsycSessionMgr ;

   /*
      _pmdSessionScopedHold define
    */
   class _pmdSessionScopedHold
   {
   public:
      _pmdSessionScopedHold()
      : _session( NULL )
      {
      }

      ~_pmdSessionScopedHold()
      {
         holdOut() ;
      }

      void setSession( _pmdAsyncSession *session )
      {
         _session = session ;
      }

      void holdOut()
      {
         if ( NULL != _session )
         {
            _session->_holdOut() ;
            _session = NULL ;
         }
      }

   protected:
      pmdAsyncSession * _session ;
   } ;

   typedef class _pmdSessionScopedHold pmdSessionScopedHold ;

}

#endif //PMD_ASYNC_SESSION_HPP_

