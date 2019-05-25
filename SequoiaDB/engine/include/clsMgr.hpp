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

   Source File Name = clsMgr.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_MGR_HPP_
#define CLS_MGR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdObjBase.hpp"
#include "ossSocket.hpp"
#include "netRouteAgent.hpp"
#include "pmdAsyncSession.hpp"
#include "clsMsgHandler.hpp"
#include "clsTimerHandler.hpp"
#include "pmdMemPool.hpp"
#include "clsShardMgr.hpp"
#include "clsReplicateSet.hpp"
#include "clsCatalogAgent.hpp"
#include "ossLatch.hpp"
#include "clsTask.hpp"
#include <map>
#include <deque>
#include <vector>
#include <string>

namespace engine
{

   class _clsMgr ;

   /*
      _innerSessionInfo define
   */
   struct _innerSessionInfo : public SDBObject
   {
      INT32       type ;
      INT32       startType ;
      INT32       innerTid ;
      UINT64      sessionID ;
      void*       data ;
   } ;
   typedef _innerSessionInfo innerSessionInfo ;

   struct _clsIdentifyInfo ;

   /*
      _clsShardSessionMgr define
   */
   class _clsShardSessionMgr : public _pmdAsycSessionMgr
   {
      public:
         _clsShardSessionMgr( _clsMgr *pClsMgr ) ;
         virtual ~_clsShardSessionMgr() ;

         virtual INT32        handleSessionTimeout( UINT32 timerID,
                                                    UINT32 interval ) ;

         BOOLEAN  isUnShardTimerStarted() const ;
         void     startUnShardTimer( UINT32 interval ) ;
         void     stopUnShardTimer() ;

         virtual UINT64       makeSessionID( const NET_HANDLE &handle,
                                             const MsgHeader *header ) ;

         virtual void         onSessionDisconnect( pmdAsyncSession *pSession ) ;
         virtual void         onNoneSessionDisconnect( UINT64 sessionID ) ;
         virtual void         onSessionHandleClose( pmdAsyncSession *pSession ) ;
         virtual void         onSessionDestoryed( pmdAsyncSession *pSession ) ;

         virtual INT32        onErrorHanding( INT32 rc,
                                              const MsgHeader *pReq,
                                              const NET_HANDLE &handle,
                                              UINT64 sessionID,
                                              pmdAsyncSession *pSession ) ;

      protected:
         /*
            Parse the session type
         */
         virtual SDB_SESSION_TYPE   _prepareCreate( UINT64 sessionID,
                                                    INT32 startType,
                                                    INT32 opCode ) ;

         virtual BOOLEAN      _canReuse( SDB_SESSION_TYPE sessionType ) ;
         virtual UINT32       _maxCacheSize() const ;

         /*
            Create session
         */
         virtual pmdAsyncSession*  _createSession( SDB_SESSION_TYPE sessionType,
                                                   INT32 startType,
                                                   UINT64 sessionID,
                                                   void *data = NULL ) ;

         virtual void         _onSessionNew( pmdAsyncSession *pSession ) ;

      protected:
         void                    _checkUnShardSessions( UINT32 interval ) ;

      protected:
         _clsMgr                 *_pClsMgr ;
         UINT32                  _unShardSessionTimer ;
         map< UINT64, _clsIdentifyInfo >    _mapIdentifys ;

   } ;
   typedef _clsShardSessionMgr clsShardSessionMgr ;

   /*
      _clsReplSessionMgr define
   */
   class _clsReplSessionMgr : public _pmdAsycSessionMgr
   {
      public:
         _clsReplSessionMgr( _clsMgr *pClsMgr ) ;
         virtual ~_clsReplSessionMgr() ;

         virtual INT32        handleSessionTimeout( UINT32 timerID,
                                                    UINT32 interval ) ;

         virtual UINT64       makeSessionID( const NET_HANDLE &handle,
                                             const MsgHeader *header ) ;

         virtual INT32        onErrorHanding( INT32 rc,
                                              const MsgHeader *pReq,
                                              const NET_HANDLE &handle,
                                              UINT64 sessionID,
                                              pmdAsyncSession *pSession ) ;

      protected:
         /*
            Parse the session type
         */
         virtual SDB_SESSION_TYPE   _prepareCreate( UINT64 sessionID,
                                                    INT32 startType,
                                                    INT32 opCode ) ;

         virtual BOOLEAN      _canReuse( SDB_SESSION_TYPE sessionType ) ;
         virtual UINT32       _maxCacheSize() const ;

         /*
            Create session
         */
         virtual pmdAsyncSession*  _createSession( SDB_SESSION_TYPE sessionType,
                                                   INT32 startType,
                                                   UINT64 sessionID,
                                                   void *data = NULL ) ;

      protected:
         _clsMgr                    *_pClsMgr ;

   } ;
   typedef _clsReplSessionMgr clsReplSessionMgr ;

   /*
      _clsMgr define
   */
   class _clsMgr : public _pmdObjBase, public _IControlBlock
   {
      friend class _clsShardSessionMgr ;
      friend class _clsReplSessionMgr ;

      DECLARE_OBJ_MSG_MAP()

      typedef std::vector<_innerSessionInfo>    VECINNERPARAM ;
      typedef std::map<UINT64, BSONObj>         MAPTASKQUERY ;

      public:
         _clsMgr() ;
         ~_clsMgr() ;

         virtual SDB_CB_TYPE cbType() const ;
         virtual const CHAR* cbName() const ;

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         virtual INT32 getMaxProcMsgTime() const { return 2 ; }
         virtual INT32 getMaxProcEventTime() const { return 2 ; }

         void     ntyPrimaryChange( BOOLEAN primary,
                                    SDB_EVENT_OCCUR_TYPE type ) ;

      public:
         const CHAR* getShardServiceName() const ;
         const CHAR* getReplServiceName () const ;
         NodeID getNodeID () const ;
         UINT16 getShardServiceID () const ;
         UINT16 getReplServiceID () const ;

      public:
         UINT64 setTimer ( CLS_MEMBER_TYPE type, UINT32 milliSec ) ;
         void   killTimer ( UINT64 timerID ) ;
         INT32  sendToCatlog ( MsgHeader * msg ) ;
         INT32  updateCatGroup ( INT64 millisec = 0 ) ;

         INT32  startInnerSession ( INT32 type, INT32 innerTID,
                                    void *data = NULL ) ;
         INT32  startTaskCheck ( const BSONObj& match ) ;
         INT32  stopTask ( UINT64 taskID ) ;
         INT32  removeTask( UINT64 taskID ) ;

         _netRouteAgent *getShardRouteAgent () ;
         _netRouteAgent *getReplRouteAgent () ;
         shardCB * getShardCB () ;
         replCB * getReplCB () ;
         catAgent * getCatAgent () ;
         nodeMgrAgent* getNodeMgrAgent () ;
         shdMsgHandler* getShardMsgHandle() ;
         _clsTaskMgr*  getTaskMgr () ;
         BOOLEAN  isPrimary () ;
         INT32    clearAllData () ;
         INT32    invalidateCache ( const CHAR *name, UINT8 type ) ;
         INT32    invalidateCata ( const CHAR *name ) ;
         INT32    invalidateStatistics () ;
         INT32    invalidatePlan ( const CHAR *name ) ;

      protected:

         INT32          _startEDU ( INT32 type, EDU_STATUS waitStatus,
                                    void *agrs, BOOLEAN regSys = TRUE ) ;

         INT32 _sendRegisterMsg () ;
         INT32 _sendQueryTaskReq ( UINT64 requestID, const CHAR *clFullName,
                                   const BSONObj* match ) ;

         virtual INT32 _defaultMsgFunc ( NET_HANDLE handle, MsgHeader* msg ) ;
         virtual void  onTimer ( UINT64 timerID, UINT32 interval ) ;

      private:
         INT32       _startInnerSession ( INT32 type,
                                          pmdAsycSessionMgr *pSessionMgr ) ;
         INT32       _prepareTask () ;
         INT32       _addTaskInnerSession ( const CHAR *objdata ) ;

      protected:
         INT32 _onCatRegisterRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatQueryTaskRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onStepDown( pmdEDUEvent *event ) ;
         INT32 _onStepUp( pmdEDUEvent *event ) ;

      private:
         _shdMsgHandler                _shdMsgHandlerObj ;
         _replMsgHandler               _replMsgHandlerObj ;
         _clsShardTimerHandler         _shdTimerHandler ;
         _clsReplTimerHandler          _replTimerHandler ;

         _netRouteAgent                _replNetRtAgent ;
         _netRouteAgent                _shardNetRtAgent ;

         _clsShardMgr                  _shdObj ;
         _clsReplicateSet              _replObj ;

         clsShardSessionMgr            _shardSessionMgr ;
         clsReplSessionMgr             _replSessionMgr ;

         ossEvent                      _attachEvent ;

         UINT16                        _shardServiceID ;
         UINT16                        _replServiceID ;
         CHAR                          _replServiceName[OSS_MAX_SERVICENAME+1] ;
         CHAR                          _shdServiceName[OSS_MAX_SERVICENAME+1] ;

         NodeID                        _selfNodeID ;
         _clsTaskMgr                   _taskMgr ;

         VECINNERPARAM                 _vecInnerSessionParam ;
         MAPTASKQUERY                  _mapTaskQuery ;
         UINT64                        _taskID ;
         map< UINT64, UINT64 >         _mapTaskID ;
         ossSpinSLatch                 _clsLatch ;

         UINT64                        _regTimerID ;
         UINT32                        _regFailedTimes ;
         UINT64                        _oneSecTimerID ;

   };

   typedef _clsMgr  clsCB ;

   /*
      get global cls cb
   */
   clsCB* sdbGetClsCB () ;
   shardCB* sdbGetShardCB () ;
   replCB* sdbGetReplCB () ;

}

#endif //CLS_MGR_HPP_


