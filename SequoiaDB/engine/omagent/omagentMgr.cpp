/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = omagentMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/


#include "omagentMgr.hpp"
#include "omagentSession.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "omagentUtil.hpp"

using namespace bson ;

namespace engine
{
   /*
      LOCAL DEFINE
   */
   #define OMAGENT_WAIT_CB_ATTACH_TIMEOUT             ( 300 * OSS_ONE_SEC )
   #define OMAGENT_ALONE_ALIVE_TIMEOUT_DFT            ( 300 ) // secs
   #define OMAGENT_IMMEDIATELY_TIMEOUT                ( 1 )

   /*
      _omAgentOptions implement
   */
   _omAgentOptions::_omAgentOptions()
   {
      ossMemset( _dftSvcName, 0, sizeof( _dftSvcName ) ) ;
      ossMemset( _cmServiceName, 0, sizeof( _cmServiceName ) ) ;
      _restartCount        = -1 ;
      _restartInterval     = 0 ;
      _autoStart           = FALSE ;
      _isGeneralAgent      = FALSE ;
      _enableWatch         = TRUE ;
      _diagLevel           = PDWARNING ;

      ossMemset( _cfgFileName, 0, sizeof( _cfgFileName ) ) ;
      ossMemset( _localCfgPath, 0, sizeof( _localCfgPath ) ) ;
      ossMemset( _scriptPath, 0, sizeof( _scriptPath ) ) ;
      ossMemset( _startProcFile, 0, sizeof( _startProcFile ) ) ;
      ossMemset( _stopProcFile, 0, sizeof( _stopProcFile ) ) ;
      ossMemset( _omAddress, 0, sizeof( _omAddress ) ) ;

      _localPort           = 0 ;

      // defaut service name
      ossSnprintf( _dftSvcName, OSS_MAX_SERVICENAME, "%u",
                   SDBCM_DFT_PORT ) ;
      ossStrcpy( _cmServiceName, _dftSvcName ) ;

      _useCurUser = FALSE ;
      _useStandAlone = FALSE ;
      _aliveTimeout = 0 ;
   }

   _omAgentOptions::~_omAgentOptions()
   {
   }

   PDLEVEL _omAgentOptions::getDiagLevel() const
   {
      PDLEVEL level = PDWARNING ;
      if ( _diagLevel < PDSEVERE )
      {
         level = PDSEVERE ;
      }
      else if ( _diagLevel > PDDEBUG )
      {
         level = PDDEBUG ;
      }
      else
      {
         level= ( PDLEVEL )_diagLevel ;
      }
      return level ;
   }

   INT32 _omAgentOptions::init ( const CHAR *pRootPath )
   {
      INT32 rc = SDB_OK ;
      CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      po::options_description desc ( "Command options" ) ;
      po::variables_map vm ;

      ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;

      _hostKey = hostName ;
      _hostKey += SDBCM_CONF_PORT ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         ( SDBCM_CONF_DFTPORT, po::value<string>(),
         "sdbcm default listening port" )
         ( _hostKey.c_str(), po::value<string>(),
         "sdbcm specified listening port" )
         ( SDBCM_RESTART_COUNT, po::value<INT32>(),
         "sequoiadb node restart max count" )
         ( SDBCM_RESTART_INTERVAL, po::value<INT32>(),
         "sequoiadb node restart time interval" )
         ( SDBCM_AUTO_START, po::value<string>(),
         "start sequoiadb node automatically when CM start" )
         ( SDBCM_DIALOG_LEVEL, po::value<INT32>(),
         "Dialog level" )
         ( SDBCM_CONF_OMADDR, po::value<string>(),
         "OM address" )
         ( SDBCM_CONF_ISGENERAL, po::value<string>(),
         "Is general agent" )
         ( SDBCM_ENABLE_WATCH, po::value<string>(),
         "restart sequoiadb node when sequoiadb node crash" )
      PMD_ADD_PARAM_OPTIONS_END

      if ( !pRootPath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build 'conf/local' file path
      rc = utilBuildFullPath( pRootPath, SDBCM_LOCAL_PATH, OSS_MAX_PATHSIZE,
                              _localCfgPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Root path is too long: %s", pRootPath ) ;
         goto error ;
      }

      // build 'conf/script' path
      rc = utilBuildFullPath( pRootPath, SDBOMA_SCRIPT_PATH, OSS_MAX_PATHSIZE,
                              _scriptPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Root path is too long: %s", pRootPath ) ;
         goto error ;
      }

      // build sdbstart program file path
      rc = utilBuildFullPath ( pRootPath, SDBSTARTPROG, OSS_MAX_PATHSIZE,
                               _startProcFile ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Root path is too long: %s", pRootPath ) ;
         goto error ;
      }

      // build sdbstop program file path
      rc = utilBuildFullPath ( pRootPath, SDBSTOPPROG, OSS_MAX_PATHSIZE,
                               _stopProcFile ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Root path is too long: %s", pRootPath ) ;
         goto error ;
      }

      // build sdbcm config file path
      rc = utilBuildFullPath( pRootPath, SDBCM_CONF_PATH_FILE,
                              OSS_MAX_PATHSIZE, _cfgFileName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Root path is too long: %s", pRootPath ) ;
         goto error ;
      }

      // read config from file
      rc = utilReadConfigureFile( _cfgFileName, desc, vm ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            // file or dir not exist
            PD_LOG( PDWARNING, "Config[%s] not exist, use default config",
                    _cfgFileName ) ;
            rc = postLoaded( PMD_CFG_STEP_INIT ) ;
            goto done ;
         }
         PD_LOG( PDERROR, "Failed to read config from file[%s], rc: %d",
                 _cfgFileName, rc ) ;
         goto error ;
      }

      rc = pmdCfgRecord::init( &vm, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init config record, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentOptions::save()
   {
      INT32 rc = SDB_OK ;
      std::string line ;

      rc = pmdCfgRecord::toString( line, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the line str:%d", rc ) ;
         goto error ;
      }

      rc = utilWriteConfigFile( _cfgFileName, line.c_str(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write config[%s], rc: %d",
                   _cfgFileName, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentOptions::doDataExchange( pmdCfgExchange * pEX )
   {
      resetResult () ;


      // --defaultPort
      rdxString( pEX, SDBCM_CONF_DFTPORT , _dftSvcName,
                 sizeof( _dftSvcName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 _dftSvcName ) ;
      // --$hostname$_Port
      rdxString( pEX, _hostKey.c_str(), _cmServiceName,
                 sizeof( _cmServiceName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 _dftSvcName ) ;
      // --RestartCount
      rdxInt( pEX, SDBCM_RESTART_COUNT, _restartCount, FALSE, PMD_CFG_CHANGE_RUN,
              _restartCount ) ;
      // --RestartInterval
      rdxInt( pEX, SDBCM_RESTART_INTERVAL, _restartInterval, FALSE, PMD_CFG_CHANGE_RUN,
              _restartInterval ) ;
      // --AutoStart
      rdxBooleanS( pEX, SDBCM_AUTO_START, _autoStart, FALSE, PMD_CFG_CHANGE_RUN,
                   _autoStart ) ;
      // --DiagLevel
      rdxInt( pEX, SDBCM_DIALOG_LEVEL, _diagLevel, FALSE, PMD_CFG_CHANGE_RUN,
              _diagLevel ) ;
      // --OMAddress
      rdxString( pEX, SDBCM_CONF_OMADDR, _omAddress, sizeof( _omAddress ),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", FALSE ) ;
      // --IsGeneral
      rdxBooleanS( pEX, SDBCM_CONF_ISGENERAL, _isGeneralAgent, FALSE,
                   PMD_CFG_CHANGE_FORBIDDEN, FALSE, FALSE ) ;
      // --EnableWatch
      rdxBooleanS( pEX, SDBCM_ENABLE_WATCH, _enableWatch, FALSE, PMD_CFG_CHANGE_RUN,
                   _enableWatch ) ;

      //  end map configs }}

      return getResult () ;
   }

   INT32 _omAgentOptions::postLoaded( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;

      // make sure directory exist
      rc = ossMkdir( getLocalCfgPath() ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Failed to create dir: %s, rc: %d",
                 getLocalCfgPath(), rc ) ;
         goto error ;
      }
      rc = SDB_OK ;

      // parse om address line
      if ( 0 != _omAddress[ 0 ] && 0 == _vecOMAddr.size() )
      {
         rc = parseAddressLine( _omAddress, _vecOMAddr ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse om address[%s] failed, rc: %d",
                      _omAddress, rc ) ;
      }

      rc = ossGetPort( _cmServiceName, _localPort ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get port by service name[%s], rc: %d",
                 _cmServiceName, rc ) ;
         goto error ;
      }
      pmdSetLocalPort( _localPort ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentOptions::preSaving()
   {
      string omAddrLine = makeAddressLine( _vecOMAddr ) ;
      ossStrncpy( _omAddress, omAddrLine.c_str(), OSS_MAX_PATHSIZE ) ;
      _omAddress[ OSS_MAX_PATHSIZE ] = 0 ;
      /// make omaddress to field
      _addToFieldMap( SDBCM_CONF_OMADDR, _omAddress, TRUE, TRUE ) ;

      // make IsGeneralAgent to field
      _addToFieldMap( SDBCM_CONF_ISGENERAL,
                      _isGeneralAgent ? "TRUE" : "FALSE",
                      TRUE, _isGeneralAgent ) ;

      return SDB_OK ;
   }

   void _omAgentOptions::addOMAddr( const CHAR * host, const CHAR * service )
   {
      if ( _vecOMAddr.size() < CLS_REPLSET_MAX_NODE_SIZE )
      {
         pmdAddrPair addr ;
         ossStrncpy( addr._host, host, OSS_MAX_HOSTNAME ) ;
         addr._host[ OSS_MAX_HOSTNAME ] = 0 ;
         ossStrncpy( addr._service, service, OSS_MAX_SERVICENAME ) ;
         addr._service[ OSS_MAX_SERVICENAME ] = 0 ;
         _vecOMAddr.push_back( addr ) ;

         if ( !_isGeneralAgent &&
              0 == ossStrcmp( host, pmdGetKRCB()->getHostName() ) )
         {
            _isGeneralAgent = TRUE ;
         }

         string str = makeAddressLine( _vecOMAddr ) ;
         ossStrncpy( _omAddress, str.c_str(), OSS_MAX_PATHSIZE ) ;
         _omAddress[ OSS_MAX_PATHSIZE ] = 0 ;
      }
   }

   void _omAgentOptions::delOMAddr( const CHAR * host, const CHAR * service )
   {
      BOOLEAN removed = FALSE ;
      _isGeneralAgent = FALSE ;
      vector< pmdAddrPair >::iterator it = _vecOMAddr.begin() ;
      while ( it != _vecOMAddr.end() )
      {
         pmdAddrPair &addr = *it ;
         if ( 0 == ossStrcmp( host, addr._host ) &&
              0 == ossStrcmp( service, addr._service ) )
         {
            it = _vecOMAddr.erase( it ) ;
            removed = TRUE ;
            continue ;
         }
         if ( !_isGeneralAgent &&
              0 == ossStrcmp( addr._host, pmdGetKRCB()->getHostName() ) )
         {
            _isGeneralAgent = TRUE ;
         }
         ++it ;
      }

      if ( removed )
      {
         string str = makeAddressLine( _vecOMAddr ) ;
         ossStrncpy( _omAddress, str.c_str(), OSS_MAX_PATHSIZE ) ;
         _omAddress[ OSS_MAX_PATHSIZE ] = 0 ;
      }
   }

   void _omAgentOptions::setCMServiceName( const CHAR * serviceName )
   {
      if ( serviceName && *serviceName )
      {
         ossStrncpy( _cmServiceName, serviceName, OSS_MAX_SERVICENAME ) ;
         _cmServiceName[ OSS_MAX_SERVICENAME ] = 0 ;
         ossGetPort( _cmServiceName, _localPort ) ;
      }
   }

   void _omAgentOptions::setStandAlone()
   {
      _useStandAlone = TRUE ;
      _aliveTimeout = OMAGENT_ALONE_ALIVE_TIMEOUT_DFT * OSS_ONE_SEC ;
   }

   void _omAgentOptions::setAliveTimeout( UINT32 timeout )
   {
      _aliveTimeout = timeout * OSS_ONE_SEC ;
   }

   void _omAgentOptions::lock( INT32 type )
   {
      if ( SHARED == type )
      {
         _latch.get_shared() ;
      }
      else
      {
         _latch.get() ;
      }
   }

   void _omAgentOptions::unLock( INT32 type )
   {
      if ( SHARED == type )
      {
         _latch.release_shared() ;
      }
      else
      {
         _latch.release() ;
      }
   }

   /*
      _omAgentSessionMgr implement
   */
   _omAgentSessionMgr::_omAgentSessionMgr()
   {
   }

   _omAgentSessionMgr::~_omAgentSessionMgr()
   {
   }

   UINT64 _omAgentSessionMgr::makeSessionID( const NET_HANDLE & handle,
                                             const MsgHeader * header )
   {
      return ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
   }

   SDB_SESSION_TYPE _omAgentSessionMgr::_prepareCreate( UINT64 sessionID,
                                                        INT32 startType,
                                                        INT32 opCode )
   {
      return SDB_SESSION_OMAGENT ;
   }

   BOOLEAN _omAgentSessionMgr::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      return FALSE ;
   }

   UINT32 _omAgentSessionMgr::_maxCacheSize() const
   {
      return 0 ;
   }

   INT32 _omAgentSessionMgr::onErrorHanding( INT32 rc,
                                             const MsgHeader *pReq,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      if ( 0 != sessionID )
      {
         ret = _reply( handle, rc, pReq ) ;
      }
      else
      {
         ret = rc ;
      }
      return ret ;
   }

   pmdAsyncSession* _omAgentSessionMgr::_createSession( SDB_SESSION_TYPE sessionType,
                                                        INT32 startType,
                                                        UINT64 sessionID,
                                                        void * data )
   {
      pmdAsyncSession *pSession = NULL ;

      if ( SDB_SESSION_OMAGENT == sessionType )
      {
         pSession = SDB_OSS_NEW omaSession( sessionID ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid session type[%d]", sessionType ) ;
      }

      return pSession ;
   }

   /*
      omAgentMgr Message MAP
   */
   BEGIN_OBJ_MSG_MAP( _omAgentMgr, _pmdObjBase )
      ON_MSG( MSG_BS_QUERY_RES, _onOMQueryTaskRes )
      ON_MSG( MSG_OM_UPDATE_TASK_RES, _onOMUpdateTaskRes )
   END_OBJ_MSG_MAP()

   /*
      omAgentMgr implement
   */
   _omAgentMgr::_omAgentMgr()
   : _msgHandler( &_sessionMgr ),
     _timerHandler( &_sessionMgr ),
     _netAgent( &_msgHandler )
   {
      _oneSecTimer         = NET_INVALID_TIMER_ID ;
      _nodeMonitorTimer    = NET_INVALID_TIMER_ID ;
      _watchAndCleanTimer  = NET_INVALID_TIMER_ID ;
      _immediatelyTimer    = NET_INVALID_TIMER_ID ;
      _primaryPos          = -1 ;
      _requestID           = 0 ;
      _sessionNum          = 0 ;
      _noMsgTimerCounter   = 0 ;
   }

   _omAgentMgr::~_omAgentMgr()
   {
   }

   SDB_CB_TYPE _omAgentMgr::cbType() const
   {
      return SDB_CB_OMAGT ;
   }

   const CHAR* _omAgentMgr::cbName() const
   {
      return "OMAGENT" ;
   }

   INT32 _omAgentMgr::init()
   {
      INT32 rc = SDB_OK ;
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      const CHAR *cmService = _options.getCMServiceName() ;
      MsgRouteID nodeID ;

      /// register config handle to omagentOptions
      _options.setConfigHandler( pmdGetKRCB() ) ;

      // init om addr
      _initOMAddr( _vecOmNode ) ;
      if ( _vecOmNode.size() > 0 )
      {
         _primaryPos = 0 ;
      }
      else
      {
         _primaryPos = -1 ;
      }

      // if is gerenal agent, need to restore
      if ( _options.isGeneralAgent() )
      {
         BSONObj noFinish ;
         BSONObj noCancel ;
         BSONArrayBuilder arrayBuilder ;
         BSONObj check ;
         pmdGetKRCB()->setBusinessOK( FALSE ) ;
         // {"Status":{$ne:4}}

         noFinish = BSON( "$ne" << OMA_TASK_STATUS_FINISH ) ;
         noCancel = BSON( "$ne" << OMA_TASK_STATUS_CANCEL ) ;

         arrayBuilder.append( BSON( OMA_FIELD_STATUS << noFinish ) ) ;
         arrayBuilder.append( BSON( OMA_FIELD_STATUS << noCancel ) ) ;

         check = BSON( "$and" << arrayBuilder.arr() ) ;
         startTaskCheck( check ) ;
      }

      // 1. create listen
      nodeID.columns.groupID = OMAGENT_GROUPID ;
      nodeID.columns.nodeID = 1 ;
      nodeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;
      _netAgent.updateRoute( nodeID, hostName, cmService ) ;
      rc = _netAgent.listen( nodeID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create listen[ServiceName: %s] failed, rc: %d",
                 cmService, rc ) ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Create listen[ServiceName:%s] succeed",
               cmService ) ;

      // 2. init session manager
      rc = _sessionMgr.init( &_netAgent, &_timerHandler, OSS_ONE_SEC ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init session manager failed, rc: %d", rc ) ;
         goto error ;
      }

      // 3. init node manager
      rc = _nodeMgr.init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init node manager failed, rc: %d", rc ) ;
         goto error ;
      }

      // 4. init scopt container
      rc = _sptScopePool.init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init container failed, rc: %d", rc ) ;
         goto error ;
      }

      // 5. init plugin manager
      rc = _pluginMgr.init( &_options ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init plugin manager, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omAgentMgr::_initOMAddr( vector< MsgRouteID > &vecNode )
   {
      MsgRouteID nodeID ;
      MsgRouteID srcID ;
      UINT16 omNID = 1 ;
      _netRoute *pRoute = _netAgent.getRoute() ;
      INT32 rc = SDB_OK ;

      vector< _pmdAddrPair > omAddrs = _options.omAddrs() ;
      // init om addr
      for ( UINT32 i = 0 ; i < omAddrs.size() ; ++i )
      {
         if ( 0 == omAddrs[i]._host[ 0 ] )
         {
            break ;
         }
         nodeID.columns.groupID = OM_GROUPID ;
         nodeID.columns.nodeID = omNID++ ;
         nodeID.columns.serviceID = MSG_ROUTE_OM_SERVICE ;

         if ( SDB_OK == pRoute->route( omAddrs[ i ]._host,
                                       omAddrs[ i ]._service,
                                       MSG_ROUTE_OM_SERVICE,
                                       srcID ) )
         {
            rc = _netAgent.updateRoute( srcID, nodeID ) ;
         }
         else
         {
            rc = _netAgent.updateRoute( nodeID, omAddrs[ i ]._host,
                                        omAddrs[ i ]._service ) ;
         }

         if ( SDB_OK == rc )
         {
            vecNode.push_back( nodeID ) ;
         }
      }
   }

   INT32 _omAgentMgr::active()
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // set primary
      pmdSetPrimary( TRUE ) ;

      if ( _options.isStandAlone() )
      {
         MsgRouteID id ;
         id.value = 0 ;
         id.columns.groupID = OMAGENT_GROUPID ;
         pmdSetNodeID( id ) ;
      }

      // active node manager
      rc = _nodeMgr.active() ;
      PD_RC_CHECK( rc, PDERROR, "Active node manager failed, rc: %d", rc ) ;

      // 1. start om manager edu
      rc = pEDUMgr->startEDU( EDU_TYPE_OMMGR, (_pmdObjBase*)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start OM Manager edu, rc: %d", rc ) ;
      // wait attach
      rc = _attachEvent.wait( OMAGENT_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait OM Manager edu attach failed, rc: %d",
                   rc ) ;

      // 2. start om net edu
      rc = pEDUMgr->startEDU( EDU_TYPE_OMNET, (netRouteAgent*)&_netAgent,
                              &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start om net, rc: %d", rc ) ;

      // 3. register timer
      rc = _netAgent.addTimer( OSS_ONE_SEC, &_timerHandler, _oneSecTimer ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to set timer, rc: %d", rc ) ;
         goto error ;
      }

      if ( FALSE == _options.isStandAlone() )
      {
         rc = _netAgent.addTimer( 2 * OSS_ONE_SEC, &_timerHandler,
                                  _nodeMonitorTimer ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to set timer, rc: %d", rc ) ;
            goto error ;
         }
         rc = _netAgent.addTimer( 120 * OSS_ONE_SEC, &_timerHandler,
                                  _watchAndCleanTimer ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to set timer, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = _pluginMgr.active() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to active plugin manager, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentMgr::deactive()
   {
      iPmdProc::stop( 0 ) ;

      // 1. kill timer
      if ( NET_INVALID_TIMER_ID != _oneSecTimer )
      {
         _netAgent.removeTimer( _oneSecTimer ) ;
         _oneSecTimer = NET_INVALID_TIMER_ID ;
      }

      // 2. stop listen
      _netAgent.closeListen() ;

      // 3. stop io
      _netAgent.stop() ;

      // 4. set force
      _sessionMgr.setForced() ;

      _pluginMgr.deactive() ;

      return SDB_OK ;
   }

   INT32 _omAgentMgr::fini()
   {
      /// release mapScopes
      SDB_ASSERT( 0 == _mapScopes.size(), "Session scopes must be zero" ) ;
      MAP_SCOPE::iterator it = _mapScopes.begin() ;
      while( it != _mapScopes.end() )
      {
         releaseScope( it->second ) ;
         ++it ;
      }
      _mapScopes.clear() ;

      _nodeMgr.fini() ;
      _sessionMgr.fini() ;
      _sptScopePool.fini() ;

      _pluginMgr.fini() ;

      return SDB_OK ;
   }

   void _omAgentMgr::attachCB( _pmdEDUCB * cb )
   {
      /// register edu exit hook func
      pmdSetEDUHook( (PMD_ON_EDU_EXIT_FUNC)sdbHookFuncOnThreadExit ) ;
      _msgHandler.attach( cb ) ;
      _timerHandler.attach( cb ) ;
      _attachEvent.signalAll() ;
   }

   void _omAgentMgr::detachCB( _pmdEDUCB * cb )
   {
      _msgHandler.detach() ;
      _timerHandler.detach() ;
   }

   void _omAgentMgr::incSession()
   {
      if ( _options.isStandAlone() )
      {
         ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
         ++_sessionNum ;
         _noMsgTimerCounter = 0 ;
      }
   }

   void _omAgentMgr::decSession()
   {
      if ( _options.isStandAlone() )
      {
         ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
         --_sessionNum ;
      }
   }

   void _omAgentMgr::resetNoMsgTimeCounter()
   {
      if ( _options.isStandAlone() )
      {
         ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
         _noMsgTimerCounter = 0 ;
      }
   }

   void _omAgentMgr::onConfigChange()
   {
      vector< MsgRouteID >::iterator it ;
      BOOLEAN bFound = FALSE ;
      vector< MsgRouteID > vecNodes ;
      _initOMAddr( vecNodes ) ;

      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;

      it = _vecOmNode.begin() ;
      while ( it != _vecOmNode.end() )
      {
         // if not found, need to delete
         bFound = FALSE ;
         for ( UINT32 i = 0 ; i < vecNodes.size() ; ++i )
         {
            if ( vecNodes[ i ].value == (*it).value )
            {
               bFound = TRUE ;
               break ;
            }
         }

         if ( !bFound )
         {
            _netAgent.delRoute( *it ) ;
         }
         ++it ;
      }
      _vecOmNode = vecNodes ;
      if ( _vecOmNode.size() > 0 )
      {
         _primaryPos = 0 ;
      }
      else
      {
         _primaryPos = -1 ;
      }

      // effect pd level
      setPDLevel( getOptions()->getDiagLevel() ) ;

      _pluginMgr.onConfigChange() ;
   }

   INT32 _omAgentMgr::_prepareTask()
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock ( &_mgrLatch, SHARED ) ;
      MAPTASKQUERY::iterator it = _mapTaskQuery.begin () ;
      while ( it != _mapTaskQuery.end() )
      {
         // send query msg to catalog
         rc = _sendQueryTaskReq ( it->first, OM_CS_DEPLOY_CL_TASKINFO,
                                  &(it->second) ) ;
         if ( SDB_OK != rc )
         {
            break ;
         }
         ++it ;
      }
      return rc ;
   }

   INT32 _omAgentMgr::_sendQueryTaskReq ( UINT64 requestID,
                                          const CHAR * clFullName,
                                          const BSONObj* match )
   {
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *msg = NULL ;
      INT32 rc = SDB_OK ;

      /// Must user Flag with FLG_QUERY_WITH_RETURNDATA
      rc = msgBuildQueryMsg ( &pBuff, &buffSize, clFullName,
                              FLG_QUERY_WITH_RETURNDATA, requestID,
                              0, -1, match, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      msg = ( MsgHeader* )pBuff ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;

      // send msg
      rc = sendToOM( msg ) ;
      PD_LOG ( PDDEBUG, "Send query[%s] to om[rc:%d]",
               match->toString().c_str(), rc ) ;

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE ( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _omAgentMgr::onTimer( UINT64 timerID, UINT32 interval )
   {
      if ( _oneSecTimer == timerID )
      {
         //Check _deqShdDeletingSessions
         _sessionMgr.onTimer( interval ) ;

         //prepare task
         _prepareTask() ;

         // check standalone mode, the process whether to quit
         if ( _options.isStandAlone() && _options.getAliveTimeout() > 0 &&
              _sessionNum == 0 )
         {
            ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
            if ( _sessionNum == 0 )
            {
               _noMsgTimerCounter += interval ;
               if ( _noMsgTimerCounter > _options.getAliveTimeout() )
               {
                  PD_LOG( PDEVENT, "Has %u secs no recv msg, quit",
                          _noMsgTimerCounter ) ;
                  PMD_SHUTDOWN_DB( SDB_TIMEOUT ) ;
               }
            }
         }
      }
      else if ( _nodeMonitorTimer == timerID )
      {
         _nodeMgr.monitorNodes() ;
      }
      else if ( _watchAndCleanTimer == timerID )
      {
         _nodeMgr.watchManualNodes() ;
         _nodeMgr.cleanDeadNodes() ;
      }
      else if ( _immediatelyTimer == timerID )
      {
         PD_LOG( PDDEBUG, "deal immediately timer:timer=%d",
                 _immediatelyTimer ) ;
         _prepareTask() ;

         {
            // remove it. we do not need a loop timer.
            ossScopedLock lock( &_immediatelyTimerLatch, EXCLUSIVE ) ;
            _netAgent.removeTimer( _immediatelyTimer ) ;
            _immediatelyTimer = NET_INVALID_TIMER_ID ;
         }
      }
   }

   omAgentOptions* _omAgentMgr::getOptions()
   {
      return &_options ;
   }

   omAgentNodeMgr* _omAgentMgr::getNodeMgr()
   {
      return &_nodeMgr ;
   }

   netRouteAgent* _omAgentMgr::getRouteAgent()
   {
      return &_netAgent ;
   }

   sptContainer* _omAgentMgr::getSptScopePool()
   {
      return &_sptScopePool ;
   }

   sptScope* _omAgentMgr::getScope()
   {
      return _sptScopePool.newScope() ;
   }

   void _omAgentMgr::releaseScope( sptScope *pScope )
   {
      _sptScopePool.releaseScope( pScope ) ;
   }

   sptScope* _omAgentMgr::getScopeBySession()
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_ASSERT( cb , "cb can't be NULL" ) ;

      sptScope *pScope = NULL ;
      ossScopedLock lock( &_scopeLatch ) ;

      /// find from mapScopes, if not found, need to alloc new scope
      MAP_SCOPE::iterator it = _mapScopes.find( cb->getTID() ) ;
      if ( it != _mapScopes.end() )
      {
         pScope = it->second ;
      }
      else if ( NULL != ( pScope = getScope() ) )
      {
          _mapScopes[ cb->getTID() ] = pScope ;
      }
      return pScope ;
   }

   void _omAgentMgr::clearScopeBySession()
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_ASSERT( cb , "cb can't be NULL" ) ;

      ossScopedLock lock( &_scopeLatch ) ;

      MAP_SCOPE::iterator it = _mapScopes.find( cb->getTID() ) ;
      if ( it != _mapScopes.end() )
      {
         releaseScope( it->second ) ;
         _mapScopes.erase( it ) ;
      }
   }

   INT32 _omAgentMgr::sendToOM( MsgHeader * msg, INT32 * pSendNum )
   {
      INT32 rc = SDB_OK ;

      if ( pSendNum )
      {
         *pSendNum = 0 ;
      }

      ossScopedLock lock( &_mgrLatch, SHARED ) ;

      INT32 tmpPrimary = _primaryPos ;

      if ( _vecOmNode.size() == 0 )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // primary node exist
      if ( tmpPrimary >= 0 && (UINT32)tmpPrimary < _vecOmNode.size() )
      {
         rc = _netAgent.syncSend ( _vecOmNode[tmpPrimary],
                                   (void*)msg ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDWARNING,
                     "Send message to primary om[%d] failed[rc:%d]",
                     _vecOmNode[tmpPrimary].columns.nodeID,
                     rc ) ;
            _primaryPos = -1 ;
            //will send to all om node
         }
         else
         {
            if ( pSendNum )
            {
               *pSendNum = 1 ;
            }
            goto done ;
         }
      }

      //send to all om node
      {
         UINT32 index = 0 ;
         INT32 rc1 = SDB_OK ;
         rc = SDB_NET_SEND_ERR ;

         while ( index < _vecOmNode.size () )
         {
            rc1 = _netAgent.syncSend ( _vecOmNode[index], (void*)msg ) ;
            if ( rc1 == SDB_OK )
            {
               rc = rc1 ;
               if ( pSendNum )
               {
                  ++(*pSendNum) ;
               }
            }
            else
            {
               PD_LOG ( PDWARNING, "Send message to om[%d] failed[rc:%d]. "
                        "It is possible because the remote service was not "
                        "started yet",
                        _vecOmNode[index].columns.nodeID,
                        rc1 ) ;
            }

            index++ ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentMgr::startTaskCheck( const BSONObj & match )
   {
      ossScopedLock lock ( &_mgrLatch, EXCLUSIVE ) ;
      _mapTaskQuery[++_requestID] = match.copy() ;

      return SDB_OK ;
   }

   INT32 _omAgentMgr::startTaskCheckImmediately( const BSONObj &match )
   {
      INT32 rc = SDB_OK ;
      {
         ossScopedLock lock ( &_mgrLatch, EXCLUSIVE ) ;
         _mapTaskQuery[++_requestID] = match.copy() ;
      }

      {
         ossScopedLock lock( &_immediatelyTimerLatch, EXCLUSIVE ) ;
         // add a immediatelyTimer
         if ( _immediatelyTimer == NET_INVALID_TIMER_ID )
         {
            rc = _netAgent.addTimer( OMAGENT_IMMEDIATELY_TIMEOUT,
                                     &_timerHandler, _immediatelyTimer ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "start check task immediately failed:rc=%d",
                       rc ) ;
               //just log a message here, do not return rc.
               //because we have the one_second_timer to active this task too.
            }

            PD_LOG( PDDEBUG, "add immediately timer:timer=%d",
                    _immediatelyTimer ) ;
         }
      }

      return SDB_OK ;
   }

   INT32 _omAgentMgr::_onOMQueryTaskRes ( NET_HANDLE handle, MsgHeader *msg )
   {
      MsgOpReply *res = ( MsgOpReply* )msg ;
      PD_LOG ( PDDEBUG, "Recieve omsvc query task response[requestID:%lld, "
               "flag: %d]", msg->requestID, res->flags ) ;

      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> objList ;

      // need to clear the query task
      if ( SDB_DMS_EOC == res->flags ||
           SDB_OM_TASK_NOT_EXIST == res->flags ||
           ( SDB_OK == res->flags && 0 == res->numReturned ) )
      {
         _mgrLatch.get() ;
         _mapTaskQuery.erase ( msg->requestID ) ;
         // try to set business to be ok
         if ( _mapTaskInfo.size() == 0 && !pmdGetKRCB()->isBusinessOK() )
         {
            PD_LOG( PDEVENT, "No task need to restore" ) ;
            // restore ok
            pmdGetKRCB()->setBusinessOK( TRUE ) ;
         }
         _mgrLatch.release() ;
         PD_LOG ( PDINFO, "The query task[%lld] has 0 jobs", msg->requestID ) ;
      }
      else if ( SDB_OK != res->flags )
      {
         PD_LOG ( PDERROR, "Query task[%lld] failed[rc=%d]",
                  msg->requestID, res->flags ) ;
         goto error ;
      }
      else
      {
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to extract task infos from omsvc, "
                     "rc = %d", rc ) ;
            goto error ;
         }
         // find the task query map, and remove it
         {
            ossScopedLock lock ( &_mgrLatch, EXCLUSIVE ) ;
            MAPTASKQUERY::iterator it = _mapTaskQuery.find ( msg->requestID ) ;
            if ( it == _mapTaskQuery.end() )
            {
               PD_LOG ( PDWARNING, "The query task response[%lld] does not exist",
                        msg->requestID ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            //remove the query task
            _mapTaskQuery.erase ( it ) ;
         }

         PD_LOG ( PDINFO, "The query[%lld] task has %d jobs", msg->requestID,
                  numReturned ) ;

         // add task inner session
         {
            UINT32 index = 0 ;
            UINT64 taskID = 0 ;
            while ( index < objList.size() )
            {
               BSONObj tmpObj = objList[index].getOwned() ;
               PD_LOG( PDDEBUG, "Task message is: %s",
                       tmpObj.toString().c_str() ) ;
               BSONElement e = tmpObj.getField( OM_TASKINFO_FIELD_TASKID ) ;
               if ( !e.isNumber() )
               {
                  PD_LOG( PDERROR, "Get taskid from obj[%s] failed",
                          tmpObj.toString().c_str() ) ;
                  ++index ;
                  continue ;
               }
               taskID = (UINT64)e.numberLong() ;

               if ( !isTaskInfoExist( taskID ) )
               {
                  INT32 tmpRc = _startTask ( tmpObj ) ;
                  if ( SDB_OK != tmpRc )
                  {
                     PD_LOG( PDERROR, "Failed to start task["OSS_LL_PRINT_FORMAT
                             "]rc = %d", taskID, rc ) ;
                  }
               }
               ++index ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentMgr::_onOMUpdateTaskRes ( NET_HANDLE handle,
                                           MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *res = ( MsgOpReply* )msg ;
      PD_LOG ( PDDEBUG, "Recieve response[requestID:%lld, "
               "flag: %d] about update progress from omsvc",
               msg->requestID, res->flags ) ;

      // check return requestID
      {
         ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
         MAP_TASKEVENT::iterator it = _mapTaskEvent.find( msg->requestID ) ;
         if ( it == _mapTaskEvent.end() )
         {
            PD_LOG ( PDWARNING, "The update task response[%lld] does not exist",
                     msg->requestID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            // notify task go on running
            it->second->signal( res->flags ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   BOOLEAN _omAgentMgr::isTaskInfoExist( UINT64 taskID )
   {
      ossScopedLock lock( &_mgrLatch, SHARED ) ;
      MAP_TASKINFO::iterator it = _mapTaskInfo.find( taskID ) ;
      if ( it == _mapTaskInfo.end() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void _omAgentMgr::registerTaskInfo( UINT64 taskID, omaTaskPtr &taskPtr )
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
      _mapTaskInfo[ taskID ] = taskPtr ;
   }

   INT32 _omAgentMgr::getTaskInfo( UINT64 taskID, omaTaskPtr &taskPtr )
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
      MAP_TASKINFO::iterator it = _mapTaskInfo.find( taskID ) ;
      if ( it != _mapTaskInfo.end() )
      {
         taskPtr = it->second ;
         return SDB_OK ;
      }

      return -1 ;
   }

   void _omAgentMgr::submitTaskInfo( UINT64 taskID )
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;

      MAP_TASKINFO::iterator it = _mapTaskInfo.find( taskID ) ;
      if ( it != _mapTaskInfo.end() )
      {
         _mapTaskInfo.erase( it ) ;
      }

      if ( _mapTaskInfo.size() == 0 && !pmdGetKRCB()->isBusinessOK() )
      {
         // restore ok
         pmdGetKRCB()->setBusinessOK( TRUE ) ;
      }
   }

   UINT64 _omAgentMgr::getRequestID()
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
      return ++_requestID ;
   }

   void _omAgentMgr::registerTaskEvent( UINT64 reqID, ossAutoEvent *pEvent )
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
      _mapTaskEvent[ reqID ] = pEvent ;
   }

   void _omAgentMgr::unregisterTaskEvent( UINT64 reqID )
   {
      ossScopedLock lock( &_mgrLatch, EXCLUSIVE ) ;
      MAP_TASKEVENT::iterator it = _mapTaskEvent.find( reqID ) ;
      if ( it != _mapTaskEvent.end() )
      {
         _mapTaskEvent.erase( it ) ;
      }
   }

   INT32 _omAgentMgr::sendUpdateTaskReq ( UINT64 requestID,
                                          const BSONObj* obj )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *msg = NULL ;
      const CHAR *pCmd = CMD_ADMIN_PREFIX OMA_CMD_UPDATE_TASK ;
      rc = msgBuildQueryMsg ( &pBuff, &buffSize, pCmd, 0, requestID,
                              0, -1, obj, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      msg = ( MsgHeader* )pBuff ;
      msg->opCode = MSG_OM_UPDATE_TASK_REQ ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;

      // send msg
      rc = sendToOM( msg ) ;
      PD_LOG ( PDDEBUG, "Send update task progress[%lld][%s] to om[rc:%d]",
               requestID, obj->toString().c_str(), rc ) ;

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE ( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentMgr::_getTaskType( const BSONObj &obj, OMA_TASK_TYPE *type )
   {
      INT32 rc = SDB_OK ;
      INT32 num = 0 ;
      OMA_TASK_TYPE taskType = OMA_TASK_END ;
      BSONObj infoObj ;
      const CHAR *pBusinessType = NULL ;
      const CHAR *pDeployMode = NULL ;

      // get task type
      rc = omaGetIntElement( obj, OMA_FIELD_TASKTYPE, num ) ;
      PD_CHECK( SDB_OK == rc, rc, error, PDERROR,
                "Get field[%s] failed, rc: %d", OMA_FIELD_TASKTYPE, rc ) ;
      taskType = (OMA_TASK_TYPE)num ;
      // check
      if ( taskType <= OMA_TASK_TYPE_BEGIN || taskType >= OMA_TASK_TYPE_END )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Receive invalid task type[%d], rc = %d",
                     taskType, rc ) ;
         goto error ;
      }

      // get task sub type
      if ( OMA_TASK_ADD_BUS == taskType || OMA_TASK_REMOVE_BUS == taskType ||
           OMA_TASK_EXTEND_BUZ == taskType )
      {
         rc = omaGetObjElement( obj, OMA_FIELD_INFO, infoObj ) ;
         PD_CHECK( SDB_OK == rc, rc, error, PDERROR,
                   "Get field[%s] failed, rc: %d",
                   OMA_FIELD_INFO, rc ) ;

         // businessType
         rc = omaGetStringElement( infoObj, OMA_FIELD_BUSINESSTYPE,
                                   &pBusinessType) ;
         PD_CHECK( SDB_OK == rc, rc, error, PDERROR,
                   "Get field[%s] failed, rc: %d",
                   OMA_FIELD_BUSINESSTYPE, rc ) ;

         // deploy mode
         rc = omaGetStringElement( infoObj, OMA_FIELD_DEPLOYMOD,
                                   &pDeployMode ) ;
         PD_CHECK( SDB_OK == rc, rc, error, PDERROR,
                   "Get field[%s] failed, rc: %d",
                   OMA_FIELD_DEPLOYMOD, rc ) ;

         if ( OMA_TASK_ADD_BUS == taskType )
         {
            if ( string(OMA_BUS_TYPE_SEQUOIADB) == string(pBusinessType) )
            {
               *type = OMA_TASK_INSTALL_DB ;
               goto done ;
            }
            else if ( string(OMA_BUS_TYPE_ZOOKEEPER) == string(pBusinessType) )
            {
               *type = OMA_TASK_INSTALL_ZN ;
               goto done ;
            }
            else if ( string(OMA_BUS_TYPE_SEQUOIASQL_OLAP) ==
                                                         string(pBusinessType) )
            {
               *type = OMA_TASK_INSTALL_SSQL_OLAP ;
               goto done ;
            }
            else
            {
               *type = OMA_TASK_ADD_BUS ;
               goto done ;
            }
         }
         else if( OMA_TASK_REMOVE_BUS == taskType )
         {
            if ( string(OMA_BUS_TYPE_SEQUOIADB) == string(pBusinessType) )
            {
               *type = OMA_TASK_REMOVE_DB ;
               goto done ;
            }
            else if ( string(OMA_BUS_TYPE_ZOOKEEPER) == string(pBusinessType) )
            {
               *type = OMA_TASK_REMOVE_ZN ;
               goto done ;
            }
            else if ( OMA_BUS_TYPE_SEQUOIASQL_OLAP == string(pBusinessType) )
            {
               *type = OMA_TASK_REMOVE_SSQL_OLAP ;
               goto done ;
            }
            else
            {
               *type = OMA_TASK_REMOVE_BUS ;
               goto done ;
            }
         }
         else if( taskType == OMA_TASK_EXTEND_BUZ )
         {
            if( string( pBusinessType ) == OMA_BUS_TYPE_SEQUOIADB )
            {
               *type = OMA_TASK_EXTEND_DB ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Unknow task sub type with name[%s], "
                           "rc = %d", pBusinessType, rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         *type = taskType ;
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentMgr::_startTask( const BSONObj &obj )
   {
      INT32 rc               = SDB_OK ;
      OMA_TASK_TYPE taskType = OMA_TASK_END ;
      INT64 taskID           = 0 ;
      BSONElement ele ;
      BSONObj data ;
      omaTaskPtr taskPtr ;

      // get task type
      rc = _getTaskType( obj, &taskType ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get task type, rc = %d", rc ) ;
         goto error ;
      }
      // get task id
      ele = obj.getField( OM_TASKINFO_FIELD_TASKID ) ;
      if ( !ele.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Get taskid from obj[%s] failed",
                 obj.toString().c_str() ) ;
         goto error ;
      }
      taskID = (INT64)ele.numberLong() ;
      // run task as a background job
      rc = startOmagentJob( taskType, taskID, obj, taskPtr, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start omagent job "
                 "rc = %d", rc ) ;
         goto error ;
      }

      registerTaskInfo( taskID, taskPtr ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      get the global om manager object point
   */
   omAgentMgr *sdbGetOMAgentMgr()
   {
      static omAgentMgr s_omagent ;
      return &s_omagent ;
   }

   omAgentOptions* sdbGetOMAgentOptions()
   {
      return sdbGetOMAgentMgr()->getOptions() ;
   }

}


