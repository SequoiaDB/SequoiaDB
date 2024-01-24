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
#include "clsRecycleBinManager.hpp"
#include "ossLatch.hpp"
#include "clsTask.hpp"
#include "ossMemPool.hpp"
#include <deque>
#include <vector>
#include <string>
#include "msg.hpp"

using namespace bson ;

namespace engine
{

   class _clsMgr ;
   class _schedTaskAdapterBase ;
   class _schedTaskContanierMgr ;

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

         schedTaskInfo*       getTaskInfo() ;

         void                 onConfigChange() ;

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
         void                 _checkUnShardSessions( UINT32 interval ) ;

      private:
         BOOLEAN              _isSplitSessionMsg( UINT32 opCode ) ;

      protected:
         _clsMgr                 *_pClsMgr ;
         UINT32                  _unShardSessionTimer ;
         map< UINT64, _clsIdentifyInfo >    _mapIdentifys ;
         schedTaskInfo           _taskInfo ;

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

   class _coordSessionPropMgr ;
   class _coordResource ;

   class _clsMgr : public _pmdObjBase, public _IControlBlock
   {
      friend class _clsShardSessionMgr ;
      friend class _clsReplSessionMgr ;

      DECLARE_OBJ_MSG_MAP()

      typedef std::vector<_innerSessionInfo>    VECINNERPARAM ;
      typedef ossPoolMap<UINT64, BSONObj>       MAPTASKQUERY ;

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

         virtual void* queryInterface( SDB_INTERFACE_TYPE type ) ;

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

         INT32  startTaskThread ( const BSONObj &taskObj, UINT64 &taskID ) ;
         INT32  startRollbackTaskThread ( UINT64 taskID,
                                          BOOLEAN isCancel = FALSE ) ;
         INT32  restartTaskThread ( UINT64 taskID ) ;

         INT32  startTaskCheck( const BSONObj& match,
                                BOOLEAN quickPull = FALSE ) ;
         INT32  startTaskCheck( UINT64 taskID,
                                BOOLEAN isMainTask = FALSE) ;
         INT32  startIdxTaskCheck( UINT64 taskID,
                                   BOOLEAN isMainTask = FALSE,
                                   BOOLEAN quickPull = FALSE ) ;
         INT32  startIdxTaskCheckByCL( utilCLUniqueID clUniqID ) ;
         INT32  startIdxTaskCheckByCS( utilCSUniqueID csUniqueID ) ;
         INT32  startAllSplitTaskCheck() ;
         INT32  startAllIdxTaskCheck() ;
         INT32  startAllTaskCheck() ;

         INT32  stopTask ( UINT64 taskID ) ;
         INT32  addTask( UINT64 taskID, UINT32 locationID ) ;
         INT32  removeTask( UINT64 taskID ) ;

         _netRouteAgent *getShardRouteAgent () ;
         _netRouteAgent *getReplRouteAgent () ;
         shardCB * getShardCB () ;
         replCB * getReplCB () ;
         catAgent * getCatAgent () ;
         nodeMgrAgent* getNodeMgrAgent () ;
         shdMsgHandler* getShardMsgHandle() ;
         _clsTaskMgr*  getTaskMgr () ;
         ossEvent* getTaskEvent() ;
         BOOLEAN  isPrimary () ;
         INT32    clearAllData () ;
         INT32    invalidateCache ( const CHAR *name, UINT8 type ) ;
         INT32    invalidateCata ( const CHAR *name ) ;
         INT32    invalidateStatistics () ;
         INT32    invalidatePlan ( const CHAR *name ) ;

         void     dumpSchedInfo( BSONObjBuilder &builder ) ;
         void     resetDumpSchedInfo() ;

         clsRecycleBinManager *getRecycleBinMgr()
         {
            return &_recycleBinMgr ;
         }

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

         void        _postTimeoutEvent( UINT64 timerID ) ;

         INT32       _initRemoteSession( _netRouteAgent *netRouteAgent ) ;

      //msg and event function
      protected:
         INT32 _onCatRegisterRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatQueryTaskRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onStepDown( pmdEDUEvent *event ) ;
         INT32 _onStepUp( pmdEDUEvent *event ) ;
         INT32 _onGroupModeUpdate( pmdEDUEvent *event ) ;
         BOOLEAN _findAndCheckTaskStatus( UINT64 taskID,
                                          dmsTaskStatusPtr &statusPtr,
                                          BOOLEAN &needRollback ) ;
         INT32 _updateDCInfo( MsgHeader* msg ) ;
      private:
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
         UINT64                        _requestID ;
         map< UINT64, UINT32 >         _mapTaskID ; // < taskID, locationID >
         ossSpinSLatch                 _clsLatch ;
         // update task progress to catalog
         ossEvent                      _taskEvent ;

         UINT64                        _regTimerID ;
         UINT32                        _regFailedTimes ;
         BOOLEAN                       _needUpdateNode ;
         UINT64                        _oneSecTimerID ;
         UINT64                        _taskTimerID ;

         _coordSessionPropMgr          *_pSitePropMgr ;
         _coordResource                *_pResource ;
         _pmdRemoteSessionMgr          _remoteSessionMgr ;

         _schedTaskContanierMgr        *_pContainerMgr ;
         _schedTaskAdapterBase         *_pShardAdapter ;
         _shdMsgHandler                *_shdMsgHandlerObj ;
         _replMsgHandler               *_replMsgHandlerObj ;
         _clsShardTimerHandler         *_shdTimerHandler ;
         _clsReplTimerHandler          *_replTimerHandler ;

         _netRouteAgent                *_replNetRtAgent ;
         _netRouteAgent                *_shardNetRtAgent ;

         _clsShardMgr                  *_shdObj ;
         _clsReplicateSet              *_replObj ;

         clsRecycleBinManager          _recycleBinMgr ;
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


