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

   Source File Name = omagentMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_MGR_HPP__
#define OMAGENT_MGR_HPP__

#include "omagentDef.hpp"
#include "pmdAsyncSession.hpp"
#include "netRouteAgent.hpp"
#include "pmdAsyncHandler.hpp"
#include "pmdAsyncHandler.hpp"
#include "pmdOptionsMgr.hpp"
#include "sdbInterface.hpp"
#include "ossEvent.hpp"
#include "omagentNodeMgr.hpp"
#include "sptContainer.hpp"
#include "omagentMsgDef.hpp"
#include "omagentTask.hpp"
#include "omagentJob.hpp"

#include <string>
#include <map>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      _omAgentOptions define
   */
   class _omAgentOptions : public _pmdCfgRecord
   {
      public:
         _omAgentOptions() ;
         virtual ~_omAgentOptions() ;

         INT32    init ( const CHAR *pRootPath ) ;
         INT32    save () ;

         const CHAR* getCfgFileName() const { return _cfgFileName ; }
         const CHAR* getLocalCfgPath() const { return _localCfgPath ; }
         const CHAR* getScriptPath() const { return _scriptPath ; }
         const CHAR* getStartProcFile() const { return _startProcFile ; }
         const CHAR* getStopProcFile() const { return _stopProcFile ; }

         const CHAR* getCMServiceName() const { return _cmServiceName ; }
         const CHAR* getOMAddress() const { return _omAddress ; }
         INT32       getRestartCount() const { return _restartCount ; }
         INT32       getRestartInterval() const { return _restartInterval ; }
         BOOLEAN     isAutoStart() const { return _autoStart && !_useStandAlone ; }
         BOOLEAN     isGeneralAgent() const { return _isGeneralAgent && !_useStandAlone ; }
         BOOLEAN     isEnableWatch() const { return _enableWatch ; }
         PDLEVEL     getDiagLevel() const ;

         vector< _pmdAddrPair > omAddrs() const
         {
            return _vecOMAddr ;
         }

         void        addOMAddr( const CHAR *host,
                                const CHAR *service ) ;
         void        delOMAddr( const CHAR *host,
                                const CHAR *service ) ;

         void        setCurUser() { _useCurUser = TRUE ; }
         BOOLEAN     isUseCurUser() const { return _useCurUser ; }

         void        setStandAlone() ;
         BOOLEAN     isStandAlone() const { return _useStandAlone ; }

         void        setAliveTimeout( UINT32 timeout ) ;
         UINT32      getAliveTimeout() const { return _aliveTimeout ; }

         void        setCMServiceName( const CHAR *serviceName ) ;

         void        lock( INT32 type = SHARED ) ;
         void        unLock( INT32 type = SHARED ) ;

      protected:
         virtual INT32 doDataExchange( pmdCfgExchange *pEX ) ;
         virtual INT32 postLoaded( PMD_CFG_STEP step ) ;
         virtual INT32 preSaving() ;

      private:
         string                     _hostKey ;

         CHAR                       _dftSvcName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR                       _cmServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         INT32                      _restartCount ;
         INT32                      _restartInterval ;
         BOOLEAN                    _autoStart ;
         INT32                      _diagLevel ;
         CHAR                       _omAddress[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN                    _isGeneralAgent ;
         BOOLEAN                    _enableWatch ;

         CHAR                       _cfgFileName[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR                       _localCfgPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR                       _scriptPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR                       _startProcFile[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR                       _stopProcFile[ OSS_MAX_PATHSIZE + 1 ] ;

         UINT16                     _localPort ;

         vector < pmdAddrPair >     _vecOMAddr ;

         BOOLEAN                    _useCurUser ;
         BOOLEAN                    _useStandAlone ;
         UINT32                     _aliveTimeout ;

         ossSpinSLatch              _latch ;

   } ;
   typedef _omAgentOptions omAgentOptions ;

   /*
      _omAgentSessionMgr define
   */
   class _omAgentSessionMgr : public _pmdAsycSessionMgr
   {
      public:
         _omAgentSessionMgr() ;
         virtual ~_omAgentSessionMgr() ;

      public:
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

   } ;
   typedef _omAgentSessionMgr omAgentSessionMgr ;

   /*
      _omAgentMgr define
   */
   class _omAgentMgr : public _pmdObjBase, public _IControlBlock
   {
      DECLARE_OBJ_MSG_MAP()

      typedef std::map<UINT64, BSONObj>         MAPTASKQUERY ;
      typedef std::map<UINT64, omaTaskPtr >     MAP_TASKINFO ;
      typedef std::map<UINT64, ossAutoEvent*>   MAP_TASKEVENT ;
      typedef std::map<UINT32, sptScope*>       MAP_SCOPE ;

      public:
         _omAgentMgr() ;
         virtual ~_omAgentMgr() ;

         virtual SDB_CB_TYPE cbType() const ;
         virtual const CHAR* cbName() const ;

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         virtual void   onConfigChange() ;
         virtual void   onTimer ( UINT64 timerID, UINT32 interval ) ;

      public:

         omAgentOptions* getOptions() ;
         omAgentNodeMgr* getNodeMgr() ;
         netRouteAgent*  getRouteAgent() ;
         sptContainer*   getSptScopePool() ;

         sptScope*       getScope() ;
         void            releaseScope( sptScope *pScope ) ;

         sptScope*       getScopeBySession() ;
         void            clearScopeBySession() ;

         void            incSession() ;
         void            decSession() ;
         void            resetNoMsgTimeCounter() ;

         INT32           sendToOM( MsgHeader *msg, INT32 *pSendNum = NULL ) ;

         INT32           startTaskCheck ( const BSONObj& match ) ;
         INT32           startTaskCheckImmediately( const BSONObj &match ) ;

         BOOLEAN         isTaskInfoExist( UINT64 taskID ) ;
         void            registerTaskInfo( UINT64 taskID, omaTaskPtr &taskPtr ) ;
         INT32           getTaskInfo( UINT64 taskID, omaTaskPtr &taskPtr ) ;
         void            submitTaskInfo( UINT64 taskID ) ;
         UINT64          getRequestID() ;
         void            registerTaskEvent( UINT64 reqID, ossAutoEvent *pEvent ) ;
         void            unregisterTaskEvent( UINT64 reqID ) ;
         INT32           sendUpdateTaskReq ( UINT64 requestID,
                                             const BSONObj* obj ) ;

      protected:
         void            _initOMAddr( vector< MsgRouteID > &vecNode ) ;
         INT32           _onOMQueryTaskRes( NET_HANDLE handle,
                                            MsgHeader *msg ) ;
         INT32           _onOMUpdateTaskRes( NET_HANDLE handle,
                                             MsgHeader *msg ) ;
         INT32           _prepareTask() ;
         INT32           _sendQueryTaskReq ( UINT64 requestID,
                                             const CHAR *clFullName,
                                             const BSONObj* match ) ;

      private:
         INT32           _getTaskType( const BSONObj &obj, OMA_TASK_TYPE *type ) ;
         INT32           _startTask( const BSONObj &obj ) ;
         INT32           _runStartPluginTask() ;
         INT32           _runStopPluginTask() ;

      private:
         omAgentOptions             _options ;
         omAgentSessionMgr          _sessionMgr ;
         pmdAsyncMsgHandler         _msgHandler ;
         pmdAsyncTimerHandler       _timerHandler ;
         netRouteAgent              _netAgent ;
         omAgentNodeMgr             _nodeMgr ;
         sptContainer               _sptScopePool ;

         ossEvent                   _attachEvent ;
         UINT32                     _oneSecTimer ;

         ossSpinSLatch              _immediatelyTimerLatch ;
         UINT32                     _immediatelyTimer ;

         UINT32                     _nodeMonitorTimer ;
         UINT32                     _watchAndCleanTimer ;

         UINT32                     _sessionNum ;
         UINT32                     _noMsgTimerCounter ;

         ossSpinSLatch              _mgrLatch ;
         vector< MsgRouteID >       _vecOmNode ;
         INT32                      _primaryPos ;

         UINT64                     _requestID ;
         MAPTASKQUERY               _mapTaskQuery ;
         MAP_TASKINFO               _mapTaskInfo ;
         MAP_TASKEVENT              _mapTaskEvent ;

         MAP_SCOPE                  _mapScopes ;
         ossSpinXLatch              _scopeLatch ;

   } ;

   typedef _omAgentMgr omAgentMgr ;

   /*
      get the global om manager object point
   */
   omAgentMgr *sdbGetOMAgentMgr() ;
   omAgentOptions *sdbGetOMAgentOptions() ;

}

#endif // OMAGENT_MGR_HPP__

