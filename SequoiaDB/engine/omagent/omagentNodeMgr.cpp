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

   Source File Name = omagentNodeMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentNodeMgr.hpp"
#include "omagentUtil.hpp"
#include "omagentMgr.hpp"
#include "utilStr.hpp"
#include "utilParam.hpp"
#include "ossProc.hpp"
#include "pmdStartup.hpp"
#include "pmd.hpp"

#include "../bson/bson.h"
#include "pd.hpp"
#include "ossPath.hpp"
#include "omagentNodePathGuard.hpp"

using namespace bson ;

namespace engine
{
   /*
      Local define
   */
   #define W_OK         2

   /*
      _startNodeJob implement
   */
   _startNodeJob::_startNodeJob( const string &svcname,
                                 NODE_START_TYPE startType,
                                 _omAgentNodeMgr *pNodeMgr )
   {
      _svcName       = svcname ;
      _startType     = startType ;
      _pNodeMgr      = pNodeMgr ;

      _jobName = "StartNode[" ;
      _jobName += _svcName ;
      _jobName += "]" ;
   }

   _startNodeJob::~_startNodeJob()
   {
   }

   RTN_JOB_TYPE _startNodeJob::type() const
   {
      return RTN_JOB_STARTNODE ;
   }

   const CHAR* _startNodeJob::name() const
   {
      return _jobName.c_str() ;
   }

   BOOLEAN _startNodeJob::muteXOn( const _rtnBaseJob *pOther )
   {
      BOOLEAN mutex = FALSE ;

      if ( RTN_JOB_STARTNODE == pOther->type() )
      {
         _startNodeJob *pOtherJob = ( _startNodeJob* )pOther ;
         if ( 0 == ossStrcmp( _svcName.c_str(),
                              pOtherJob->getSvcName().c_str() ) )
         {
            mutex = TRUE ;
         }
      }

      return mutex ;
   }

   INT32 _startNodeJob::doit()
   {
      INT32 rc = _pNodeMgr->startANode( _svcName.c_str(), _startType,
                                        TRUE ) ;
      if ( SDB_OK == rc )
      {
         if ( NODE_START_MONITOR == _startType )
         {
            PD_LOG ( PDEVENT, "Successfully to restart SequoiaDB node from "
                     "crash, svcname = %s", _svcName.c_str() ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Start Sequoaidb node[svcname = %s] succeed",
                    _svcName.c_str() ) ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Start Sequoaidb node[svcname = %s] failed, rc: %d",
                 _svcName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _stopNodeJob implement
   */
   _stopNodeJob::_stopNodeJob( const string &svcname,
                               NODE_START_TYPE type,
                               _omAgentNodeMgr *pNodeMgr )
   {
      _svcName    = svcname ;
      _type       = type ;
      _pNodeMgr   = pNodeMgr ;

      _jobName = "StopNode[" ;
      _jobName += _svcName ;
      _jobName += "]" ;
   }

   _stopNodeJob::~_stopNodeJob()
   {
   }

   RTN_JOB_TYPE _stopNodeJob::type() const
   {
      return RTN_JOB_STOPNODE ;
   }

   const CHAR* _stopNodeJob::name() const
   {
      return _jobName.c_str() ;
   }

   BOOLEAN _stopNodeJob::muteXOn( const _rtnBaseJob *pOther )
   {
      BOOLEAN mutex = FALSE ;

      if ( RTN_JOB_STOPNODE == pOther->type() )
      {
         _stopNodeJob *pOtherJob = ( _stopNodeJob* )pOther ;
         if ( 0 == ossStrcmp( _svcName.c_str(),
                              pOtherJob->getSvcName().c_str() ) )
         {
            mutex = TRUE ;
         }
      }

      return mutex ;
   }

   INT32 _stopNodeJob::doit()
   {
      INT32 rc = SDB_OK ;

      rc = _pNodeMgr->stopANode( _svcName.c_str(), _type, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Stop SequoaiDB node[svcname = %s] failed, rc: %d",
                 _svcName.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Stop SequoaiDB node[svcname = %s] succeed",
              _svcName.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 runStartNodeJob( const string &svcname, NODE_START_TYPE startType,
                          _omAgentNodeMgr *pNodeMgr, EDUID * pEDUID,
                          BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;
      startNodeJob *pJob = NULL ;

      pJob = SDB_OSS_NEW startNodeJob( svcname, startType, pNodeMgr ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Failed to alloc start node job" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE, pEDUID,
                                     returnResult ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 runStopNodeJob( const string &svcname, NODE_START_TYPE type,
                         _omAgentNodeMgr *pNodeMgr, EDUID * pEDUID,
                         BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;
      stopNodeJob *pJob = NULL ;

      pJob = SDB_OSS_NEW stopNodeJob( svcname, type, pNodeMgr ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Failed to alloc stop node job" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE, pEDUID,
                                     returnResult ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _cmSyncJob implement
   */
   _cmSyncJob::_cmSyncJob( _omAgentNodeMgr * pNodeMgr )
   {
      _pNodeMgr = pNodeMgr ;
   }

   _cmSyncJob::~_cmSyncJob()
   {
   }

   RTN_JOB_TYPE _cmSyncJob::type() const
   {
      return RTN_JOB_CMSYNC ;
   }

   const CHAR* _cmSyncJob::name() const
   {
      return "CMSYNC" ;
   }

   BOOLEAN _cmSyncJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   INT32 _cmSyncJob::doit()
   {
      _pNodeMgr->syncProcesserInfo() ;
      PMD_SHUTDOWN_DB( 0 ) ;
      return SDB_OK ;
   }

   INT32 startCMSyncJob( _omAgentNodeMgr * pNodeMgr, EDUID * pEDUID,
                         BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;
      cmSyncJob *pJob = NULL ;

      pJob = SDB_OSS_NEW cmSyncJob( pNodeMgr ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Failed to alloc cmSync job" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID,
                                     returnResult ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omAgentNodeMgr implement
   */
   _omAgentNodeMgr::_omAgentNodeMgr()
   {
   }

   _omAgentNodeMgr::~_omAgentNodeMgr()
   {
   }

   INT32 _omAgentNodeMgr::init()
   {
      INT32 rc = SDB_OK ;
      omAgentOptions *option = sdbGetOMAgentOptions() ;
      vector< string > vecSvc ;
      dbProcessInfo dbProcess ;
      BOOLEAN isRunning = FALSE ;

      if ( option->isStandAlone() )
      {
         goto done ;
      }

      rc = cCMService::init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init cm service failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omGetSvcListFromConfig( option->getLocalCfgPath(), vecSvc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get service list from config, "
                   "rc: %d", rc ) ;

      /// init process info
      _mapLatch.get() ;
      for ( UINT32 i = 0 ; i < vecSvc.size() ; ++i )
      {
         omCheckDBProcessBySvc( vecSvc[i].c_str(), isRunning,
                                dbProcess._pid ) ;
         if ( isRunning )
         {
            dbProcess._status = OMNODE_RUNNING ;
            PD_LOG( PDEVENT, "Detect Sequoiadb node[svcname = %s] already "
                    "started, pid: %d", vecSvc[i].c_str(), dbProcess._pid ) ;
         }
         _mapDBProcess[ vecSvc[i] ] = dbProcess ;
         dbProcess.reset() ;
      }
      _mapLatch.release() ;

      /// init node guards
      for ( UINT32 i = 0 ; i < vecSvc.size() ; ++i )
      {
         addNodeGuard( vecSvc[i] ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::active()
   {
      INT32 rc = SDB_OK ;
      omAgentOptions *option = sdbGetOMAgentOptions() ;

      if ( option->isStandAlone() )
      {
         goto done ;
      }

      rc = startCMSyncJob( this, NULL, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start cm sync job failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( option->isAutoStart() )
      {
         startAllNodes( NODE_START_SYSTEM ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::fini()
   {
      cCMService::fini() ;

      _nodeGuards.clear() ;

      return SDB_OK ;
   }

   INT32 _omAgentNodeMgr::startAllNodes( NODE_START_TYPE startType )
   {
      INT32 rc = SDB_OK ;
      dbProcessInfo *pInfo = NULL ;
      const CHAR *pSvcName = NULL ;

      ossScopedLock lock( &_mapLatch, SHARED ) ;

      MAP_DB_PROCESS_IT it = _mapDBProcess.begin() ;
      while ( it != _mapDBProcess.end() )
      {
         pInfo = &(it->second) ;
         pSvcName = it->first.c_str() ;
         ++it ;

         if ( OMNODE_REMOVING == pInfo->_status )
         {
            continue ;
         }
         rc = runStartNodeJob( pSvcName, startType, this, NULL, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Start startNodeJob failed, svcname = %s, rc: %d",
                    pSvcName, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::stopAllNodes()
   {
      return omStopDBNode( sdbGetOMAgentOptions()->getStopProcFile(), "" ) ;
   }

   void _omAgentNodeMgr::cleanDeadNodes()
   {
      vector< string > delList ;
      _mapLatch.get() ;
      MAP_DB_PROCESS_IT it = _mapDBProcess.begin() ;
      while ( it != _mapDBProcess.end() )
      {
         dbProcessInfo &info = it->second ;
         if ( OMNODE_REMOVING == info._status )
         {
            delList.push_back( it->first ) ;
            _mapDBProcess.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }
      _mapLatch.release() ;

      for ( UINT32 i = 0 ; i < delList.size() ; ++i )
      {
         delNodeGuard( delList[i] ) ;
      }
   }

   void _omAgentNodeMgr::watchManualNodes()
   {
      INT32 rc = SDB_OK ;
      vector< string > vecNodes ;

      rc = omGetSvcListFromConfig( sdbGetOMAgentOptions()->getLocalCfgPath(),
                                   vecNodes ) ;
      if ( rc )
      {
         goto done ;
      }

      for ( UINT32 i = 0 ; i < vecNodes.size() ; ++i )
      {
         const string & svcname = vecNodes[ i ] ;

         lockBucket( svcname ) ;

         addNodeGuard( svcname ) ;
         if ( NULL != _getNodeGuard( svcname.c_str() ) )
         {
            addNodeProcessInfo( svcname ) ;
         }

         releaseBucket( svcname ) ;
      }

   done:
      return ;
   }

   void _omAgentNodeMgr::monitorNodes()
   {
      ossScopedLock lock( &_mapLatch, SHARED ) ;

      dbProcessInfo *pInfo    = NULL ;
      const CHAR *pSvcName    = NULL ;
      BOOLEAN isRunning       = FALSE ;
      BOOLEAN isLock          = FALSE ;

      INT32 restartCount      = sdbGetOMAgentOptions()->getRestartCount() ;
      INT32 restartInterval   = sdbGetOMAgentOptions()->getRestartInterval() ;

      MAP_DB_PROCESS_IT it = _mapDBProcess.begin() ;
      while ( it != _mapDBProcess.end() )
      {
         INT32 rc = SDB_OK ;
         CHAR cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         if ( isLock )
         {
            getBucket( pSvcName )->release() ;
            isLock = FALSE ;
         }

         pInfo = &(it->second) ;
         pSvcName = it->first.c_str() ;
         ++it ;

         if ( OMNODE_REMOVING == pInfo->_status )
         {
            continue ;
         }

         // if the bucket is locked by other, skip
         if ( FALSE == getBucket( pSvcName )->try_get() )
         {
            continue ;
         }
         isLock = TRUE ;

         if ( OSS_INVALID_PID != pInfo->_pid &&
              ossIsProcessRunning( pInfo->_pid ) )
         {
            pInfo->_status = OMNODE_RUNNING ;
            pInfo->_startTime.clear() ;
            continue ;
         }

         omCheckDBProcessBySvc( pSvcName, isRunning, pInfo->_pid ) ;
         if ( isRunning )
         {
            // start by manual start
            pInfo->_status = OMNODE_RUNNING ;
            pInfo->_startTime.clear() ;
            PD_LOG( PDEVENT, "Detect Sequoiadb node[svcname = %s ] has been "
                    "started, pid: %d", pSvcName, pInfo->_pid ) ;
            continue ;
         }

         pInfo->_pid = OSS_INVALID_PID ;

         // before we try to restart a node which _status is "restart" or
         // "crash", check whether it's cfg file exist or not, if not,
         // set it's _status to be "removing"
         rc = _getCfgFile( pSvcName, cfgFile, OSS_MAX_PATHSIZE + 1 ) ;
         if ( SDB_FNE == rc )
         {
            PD_LOG ( PDERROR, "Failed to get node[%s]'s cfg file: rc: %d",
                     pSvcName, rc ) ;
            pInfo->_status = OMNODE_REMOVING ;
            continue ;
         }

         if ( 0 == restartCount || ( restartCount > 0 &&
              pInfo->_startTime.size() > (UINT32)restartCount ) )
         {
            // the process has been start many times and all failed.
            continue ;
         }

         if ( restartInterval > 0 && pInfo->_startTime.size() > 0 )
         {
            if ( ( time( NULL ) - pInfo->_startTime.back() ) / 60 <
                 restartInterval )
            {
               // does not meet the interval
               continue ;
            }
         }

         // clear some time-info
         if ( pInfo->_startTime.size() > 0 )
         {
            UINT32 keepCount = 1 ;
            if ( restartCount >= 0 )
            {
               keepCount = restartCount + 1 ;
            }
            while ( pInfo->_startTime.size() > keepCount )
            {
               pInfo->_startTime.pop_front() ;
            }
         }

         if ( OMNODE_RESTART != pInfo->_status )
         {
            pInfo->_status = OMNODE_NORMAL ;
            // check status by startup file
            _checkNodeByStartupFile( pSvcName, pInfo ) ;
         }

         // if crashed, start job
         if ( OMNODE_CRASH == pInfo->_status ||
              OMNODE_RESTART == pInfo->_status )
         {
            // if enableWatch is TRUE, start node
            if ( sdbGetOMAgentOptions()->isEnableWatch() )
            {
               // if enable watch, clear state of _isDetected
               if ( pInfo->_isDetected )
               {
                  pInfo->_isDetected = FALSE ;
               }
               PD_LOG( PDEVENT, "Detect Sequoiadb node[svcname = %s] %s, "
                       "Begin to restart", pSvcName,
                       OMNODE_CRASH == pInfo->_status ?
                       "crashed" : "start failed" ) ;
               runStartNodeJob( pSvcName, NODE_START_MONITOR, this,
                                NULL, FALSE ) ;
            }
            else if( !pInfo->_isDetected )
            {
               // if the process is detected the first time, write log
               pInfo->_isDetected = TRUE ;
               PD_LOG( PDEVENT, "Detect Sequoiadb node[svcname = %s] %s",
                       pSvcName,
                       OMNODE_CRASH == pInfo->_status ?
                       "crashed" : "start failed" ) ;
            }
         }
      }

      if ( isLock )
      {
         getBucket( pSvcName )->release() ;
         isLock = FALSE ;
      }
   }

   INT32 _omAgentNodeMgr::_getCfgFile( const CHAR *pSvcName,
                                       CHAR *pBuffer, INT32 bufSize )
   {
      INT32 rc = SDB_OK ;
      INT32 len = 0 ;
      CHAR cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( NULL == pBuffer || bufSize <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid buffer info for output cfg file path" ) ;
         goto error ;
      }

      rc = utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                              pSvcName, OSS_MAX_PATHSIZE, cfgFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build node[%s] config path failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = utilCatPath( cfgFile, OSS_MAX_PATHSIZE, PMD_DFT_CONF ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build node[%s] config path failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = ossAccess( cfgFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Access node[%s]'s cfg file failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      // get cfg file
      len = ( ossStrlen(cfgFile) + 1 <= (UINT32)bufSize ) ?
            ossStrlen(cfgFile) + 1 : bufSize ;
      ossStrncpy( pBuffer, cfgFile, len - 1 ) ;
      pBuffer[len] = '\0' ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omAgentNodeMgr::_checkNodeByStartupFile( const CHAR *pSvcName,
                                                  dbProcessInfo *pInfo )
   {
      INT32 rc = SDB_OK ;
      CHAR cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      const CHAR *pDBPath = NULL ;

      po::options_description desc ( "Command options" ) ;
      po::variables_map vm ;
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         PMD_COMMANDS_OPTIONS
         PMD_HIDDEN_COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      rc = _getCfgFile( pSvcName, cfgFile, OSS_MAX_PATHSIZE + 1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get node[%s] config path failed, rc: %d",
                 pSvcName, rc ) ;
         pInfo->_status = OMNODE_REMOVING ;
         goto done ;
      }

      rc = utilReadConfigureFile( cfgFile, desc, vm ) ;
      if ( rc )
      {
         if ( pInfo->_errNum != 1 )
         {
            PD_LOG ( PDERROR, "Can not read configure file: %s, rc: %d",
                     cfgFile, rc ) ;
            pInfo->_errNum = 1 ;
         }
         goto done ;
      }
      if ( vm.count( PMD_OPTION_DBPATH ) == 0 )
      {
         if ( pInfo->_errNum != 2 )
         {
            PD_LOG ( PDERROR, "Can not get dbpath in configure file: %s",
                     cfgFile ) ;
            pInfo->_errNum = 2 ;
         }
         goto done ;
      }

      pDBPath = vm[PMD_OPTION_DBPATH].as<string>().c_str() ;
      if ( NULL == pDBPath )
      {
         if ( pInfo->_errNum != 3 )
         {
            PD_LOG ( PDERROR, "Can not read dbpath from configure file: %s",
                     cfgFile ) ;
            pInfo->_errNum = 3 ;
         }
         goto done ;
      }

      {
         pmdStartup startUpFile ;
         rc = startUpFile.init( pDBPath, TRUE ) ;
         if ( rc )
         {
            if ( pInfo->_errNum != 4 )
            {
               PD_LOG ( PDERROR, "Init startup file[%s] failed, rc: %d",
                        cfgFile, rc ) ;
               pInfo->_errNum = 4 ;
            }
            goto done ;
         }

         pInfo->_errNum = 0 ;

         if ( startUpFile.needRestart() )
         {
            pInfo->_status = OMNODE_CRASH ;
         }
      }

   done:
      return ;
   }

   INT32 _omAgentNodeMgr::startANode( const CHAR *svcname,
                                      NODE_START_TYPE type,
                                      BOOLEAN needLock )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      const CHAR *pLocalCfgDir = sdbGetOMAgentOptions()->getLocalCfgPath() ;
      CHAR  cfgPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      dbProcessInfo *pInfo = NULL ;
      time_t now ;
      time( &now ) ;

      rc = utilBuildFullPath( pLocalCfgDir, svcname, OSS_MAX_PATHSIZE,
                              cfgPath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build config path[svcname: %s], "
                   "rc: %d", svcname, rc ) ;

      if ( needLock )
      {
         lockBucket( svcname ) ;
         hasLock = TRUE ;
      }

      pInfo = getNodeProcessInfo( svcname ) ;
      if ( !pInfo )
      {
         rc = SDBCM_NODE_NOTEXISTED ;
         goto error ;
      }

      if ( OMNODE_RUNNING == pInfo->_status )
      {
         if ( OSS_INVALID_PID != pInfo->_pid &&
              ossIsProcessRunning( pInfo->_pid ) )
         {
            rc = SDBCM_SVC_STARTED ;
            goto error ;
         }
         else
         {
            BOOLEAN isRunning = FALSE ;
            // check is running
            omCheckDBProcessBySvc( svcname, isRunning, pInfo->_pid ) ;

            if ( isRunning )
            {
               rc = SDBCM_SVC_STARTED ;
               goto error ;
            }
         }
      }

      if ( NODE_START_MONITOR == type )
      {
         pInfo->_startTime.push_back( now ) ;
      }

      // start node
      rc = omStartDBNode( sdbGetOMAgentOptions()->getStartProcFile(),
                          cfgPath, svcname, pInfo->_pid,
                          sdbGetOMAgentOptions()->isUseCurUser() ) ;
      if ( SDB_OK == rc )
      {
         pInfo->_errNum = 0 ;
         pInfo->_status = OMNODE_RUNNING ;
      }
      else
      {
         if ( NODE_START_SYSTEM == type )
         {
            pInfo->_status = OMNODE_RESTART ;
         }
         PD_LOG( PDERROR, "Start node[%s] failed, rc: %d(%s)", svcname,
                 rc, getErrDesp( rc ) ) ;
         goto error ;
      }

   done:
      if ( hasLock )
      {
         releaseBucket( svcname ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::stopANode( const CHAR * svcname,
                                     NODE_START_TYPE type,
                                     BOOLEAN needLock,
                                     BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      dbProcessInfo *pInfo = NULL ;

      if ( needLock )
      {
         lockBucket( svcname ) ;
         hasLock = TRUE ;
      }

      pInfo = getNodeProcessInfo( svcname ) ;
      /*
         When NULL == pInfo, we can stop other sequoaidb
      */

      rc = omStopDBNode( sdbGetOMAgentOptions()->getStopProcFile(),
                         svcname, force ) ;
      if ( SDB_OK == rc )
      {
         if ( pInfo )
         {
            pInfo->_pid = OSS_INVALID_PID ;
            pInfo->_startTime.clear() ;
            pInfo->_status = OMNODE_NORMAL ;
            pInfo->_errNum = 0 ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Stop sequoaidb node failed, svc = %s, rc: %d",
                 svcname, rc ) ;
         goto error ;
      }

   done:
      if ( hasLock )
      {
         releaseBucket( svcname ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::addNodeProcessInfo( const string &svcname )
   {
      INT32 rc = SDB_OK ;
      _mapLatch.get() ;
      MAP_DB_PROCESS_IT it = _mapDBProcess.find( svcname ) ;
      if ( it != _mapDBProcess.end() )
      {
         rc = SDBCM_NODE_EXISTED ;
      }
      else
      {
         dbProcessInfo info ;
         _mapDBProcess[ svcname ] = info ;
      }
      _mapLatch.release() ;

      return rc ;
   }

   INT32 _omAgentNodeMgr::delNodeProcessInfo( const string &svcname )
   {
      INT32 rc = SDB_OK ;
      _mapLatch.get() ;
      MAP_DB_PROCESS_IT it = _mapDBProcess.find( svcname ) ;
      if ( it == _mapDBProcess.end() )
      {
         rc = SDBCM_NODE_NOTEXISTED ;
      }
      else
      {
         _mapDBProcess.erase( it ) ;
      }

      _mapLatch.release() ;
      return rc ;
   }

   omaNodePathGuard* _omAgentNodeMgr::_getNodeGuard( const CHAR *svcname )
   {
      for ( UINT32 i = 0 ; i < _nodeGuards.size() ; ++i )
      {
         if ( 0 == ossStrcmp( svcname, _nodeGuards[ i ].name() ) )
         {
            return &_nodeGuards[ i ] ;
         }
      }
      return NULL ;
   }

   INT32 _omAgentNodeMgr::addNodeGuard( omaNodePathGuard &nodeGuard )
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock( &_guardLatch, EXCLUSIVE ) ;
      if ( _getNodeGuard( nodeGuard.name() ) )
      {
         rc = SDBCM_NODE_EXISTED ;
      }
      else
      {
         _nodeGuards.push_back( nodeGuard ) ;
      }
      return rc ;
   }

   INT32 _omAgentNodeMgr::addNodeGuard( const string &svcname )
   {
      INT32 rc = SDB_OK ;
      CHAR configFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                         svcname.c_str(), OSS_MAX_PATHSIZE, configFile ) ;
      utilCatPath( configFile, OSS_MAX_PATHSIZE, PMD_DFT_CONF ) ;

      omaNodePathGuard nodeGuard ;
      pmdOptionsCB options ;
      rc = options.initFromFile( configFile, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract node[%s]'s config from file[%s] "
                   "failed, rc: %d", svcname.c_str(), configFile, rc ) ;

      nodeGuard.init( svcname.c_str(), &options ) ;
      rc = addNodeGuard( nodeGuard ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::delNodeGuard( const string &svcname )
   {
      INT32 rc = SDBCM_NODE_EXISTED ;
      ossScopedLock lock( &_guardLatch, EXCLUSIVE ) ;
      vector<_omaNodePathGuard>::iterator it = _nodeGuards.begin() ;
      while( it != _nodeGuards.end() )
      {
         if ( 0 == ossStrcmp( svcname.c_str(), (*it).name() ) )
         {
            _nodeGuards.erase( it ) ;
            break ;
         }
         ++it ;
      }
      return rc ;
   }

   dbProcessInfo* _omAgentNodeMgr::getNodeProcessInfo( const string & svcname )
   {
      dbProcessInfo *pInfo = NULL ;
      ossScopedLock lock( &_mapLatch, SHARED ) ;
      MAP_DB_PROCESS_IT it = _mapDBProcess.find( svcname ) ;
      if ( it != _mapDBProcess.end() )
      {
         pInfo = &(it->second) ;
      }
      return pInfo ;
   }

   void _omAgentNodeMgr::lockBucket( const string &svcname )
   {
      UINT32 id = ossHash( svcname.c_str() ) % OM_NODE_LOCK_BUCKET_SIZE ;
      _lockBucket[ id ].get() ;
   }

   void _omAgentNodeMgr::releaseBucket( const string & svcname )
   {
      UINT32 id = ossHash( svcname.c_str() ) % OM_NODE_LOCK_BUCKET_SIZE ;
      _lockBucket[ id ].release() ;
   }

   ossSpinXLatch* _omAgentNodeMgr::getBucket( const string & svcname )
   {
      UINT32 id = ossHash( svcname.c_str() ) % OM_NODE_LOCK_BUCKET_SIZE ;
      return &_lockBucket[ id ] ;
   }

   INT32 _omAgentNodeMgr::addANode( const CHAR *arg1, const CHAR *arg2,
                                    string *omsvc )
   {
      return _addANode( arg1, arg2, TRUE, FALSE, omsvc ) ;
   }

   const CHAR* _omAgentNodeMgr::_getSvcNameFromArg( const CHAR * arg )
   {
      const CHAR *pSvcName = NULL ;

      try
      {
         BSONObj objArg1( arg ) ;
         BSONElement e = objArg1.getField( PMD_OPTION_SVCNAME ) ;
         if ( e.type() != String )
         {
            PD_LOG( PDERROR, "Param[%s] type[%s] error: %s",
                    PMD_OPTION_SVCNAME, e.type(), e.toString().c_str() ) ;
            goto error ;
         }
         pSvcName = e.valuestrsafe() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocuur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return pSvcName ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::_addANode( const CHAR *arg1, const CHAR *arg2,
                                     BOOLEAN needLock, BOOLEAN isModify,
                                     string *omsvc )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = NULL ;
      const CHAR *pDBPath = NULL ;
      string otherCfg ;
      omaNodePathGuard nodeGuard ;

      CHAR dbPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR cfgPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      BOOLEAN createDBPath    = FALSE ;
      BOOLEAN createCfgPath   = FALSE ;
      BOOLEAN createCfgFile   = FALSE ;
      BOOLEAN hasLock         = FALSE ;

      try
      {
         stringstream ss ;
         BSONObj objArg1( arg1 ) ;
         BSONObjIterator it ( objArg1 ) ;
         while ( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 == ossStrcmp( e.fieldName(), PMD_OPTION_SVCNAME ) )
            {
               if ( e.type() != String )
               {
                  PD_LOG( PDERROR, "Param[%s] type[%d] is not string",
                          PMD_OPTION_SVCNAME, e.type() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               pSvcName = e.valuestrsafe() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), PMD_OPTION_DBPATH ) )
            {
               if ( e.type() != String )
               {
                  PD_LOG( PDERROR, "Param[%s] type[%d] is not string",
                          PMD_OPTION_DBPATH, e.type() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               pDBPath = e.valuestrsafe() ;
            }
            /// ignore PMD_OPTION_CONFPATH
            else if ( 0 != ossStrcmp( e.fieldName(), PMD_OPTION_CONFPATH ) )
            {
               ss << e.fieldName() << "=" ;
               switch( e.type() )
               {
                  case NumberDouble :
                     ss << e.numberDouble () ;
                     break ;
                  case NumberInt :
                     ss << e.numberLong () ;
                     break ;
                  case NumberLong :
                     ss << e.numberInt () ;
                     break ;
                  case String :
                     ss << e.valuestrsafe () ;
                     break ;
                  case Bool :
                     ss << ( e.boolean() ? "TRUE" : "FALSE" ) ;
                     break ;
                  default :
                     PD_LOG ( PDERROR, "Unexpected type[%d] for %s",
                              e.type(), e.toString().c_str() ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
               }
               ss << endl ;
            }
         }
         otherCfg = ss.str() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !pSvcName || !pDBPath )
      {
         PD_LOG( PDERROR, "Param [%s] or [%s] is not config",
                 PMD_OPTION_SVCNAME, PMD_OPTION_DBPATH ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !ossGetRealPath( pDBPath, dbPath, OSS_MAX_PATHSIZE ) )
      {
         PD_LOG( PDERROR, "Invalid db path: %s", pDBPath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( needLock )
      {
         // lock bucket
         lockBucket( pSvcName ) ;
         hasLock = TRUE ;
      }

      if ( isModify && !getNodeProcessInfo( pSvcName ) )
      {
         rc = SDBCM_NODE_NOTEXISTED ;
         goto error ;
      }

      rc = ossAccess( dbPath, W_OK ) ;
      // if we get permission, we can't continue
      if ( SDB_PERM == rc )
      {
         PD_LOG ( PDERROR, "Permission error for path: %s", dbPath ) ;
         goto error ;
      }
      else if ( SDB_FNE == rc )
      {
         rc = ossMkdir ( dbPath ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create config file in path: %s, "
                     "rc: %d", dbPath, rc ) ;
            goto error ;
         }
         createDBPath = TRUE ;
      }
      else if ( rc )
      {
         PD_LOG ( PDERROR, "System error for access path: %s, rc: %d",
                  dbPath, rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                              pSvcName, OSS_MAX_PATHSIZE, cfgPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config path for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      // create path: "conf/local/$svcname"
      rc = ossAccess( cfgPath, W_OK ) ;
      // if we get permission, we can't continue
      if ( SDB_PERM == rc )
      {
         PD_LOG ( PDERROR, "Permission error for path[%s]", cfgPath ) ;
         goto error ;
      }
      // if we can not find the file, then create one
      else if ( SDB_FNE == rc )
      {
         rc = ossMkdir ( cfgPath ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create directory: %s, rc: %d",
                     cfgPath, rc ) ;
            goto error ;
         }
         createCfgPath = TRUE ;
      }
      else if ( rc )
      {
         PD_LOG ( PDERROR, "System error for access path: %s, rc: %d",
                  cfgPath, rc ) ;
         goto error ;
      }

      // make config file
      rc = utilBuildFullPath( cfgPath, PMD_DFT_CONF, OSS_MAX_PATHSIZE,
                              cfgFile ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Build config file for service[%s] failed, rc: %d",
                  pSvcName, rc ) ;
         goto error ;
      }
      else if ( !isModify && ( NULL != getNodeProcessInfo( pSvcName ) ||
                SDB_OK == ossAccess( cfgFile ) ) )
      {
         /// need to return omsvc
         if ( omsvc )
         {
            pmdOptionsCB nodeOptions ;
            nodeOptions.initFromFile( cfgFile ) ;
            *omsvc = nodeOptions.getOMService() ;
         }
         
         PD_LOG ( PDERROR, "service[%s] node existed", pSvcName ) ;
         rc = SDBCM_NODE_EXISTED ;
         goto error ;
      }

      // build full config and write file
      {
         pmdOptionsCB nodeOptions ;
         stringstream ss ;
         ss << PMD_OPTION_SVCNAME << "=" << pSvcName << endl ;
         ss << PMD_OPTION_DBPATH << "=" << dbPath << endl ;
         ss << otherCfg ;

         rc = utilWriteConfigFile( cfgFile, ss.str().c_str(),
                                   isModify ? FALSE : TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write config file[%s] failed, rc: %d",
                    cfgFile, rc ) ;
            goto error ;
         }
         createCfgFile = TRUE ;

         // read and check config
         rc = nodeOptions.initFromFile( cfgFile, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extract node[%s] config failed, rc: %d",
                    pSvcName, rc ) ;
            goto error ;
         }
         else if ( 0 == ossStrcmp( sdbGetOMAgentOptions()->getCMServiceName(),
                                   nodeOptions.getServiceAddr() ) )
         {
            PD_LOG( PDERROR, "Node[%s] svcname[%s] is same with omagent",
                    pSvcName, nodeOptions.getServiceAddr() ) ;
            rc = SDB_CM_CONFIG_CONFLICTS ;
            goto error ;
         }

         if ( omsvc )
         {
            *omsvc = nodeOptions.getOMService() ;
         }

         nodeGuard.init( pSvcName, &nodeOptions ) ;
         /// check config path valid
         rc = nodeGuard.checkValid( &nodeOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Check node[%s] config path valid "
                      "failed, rc: %d", pSvcName, rc ) ;
         /// check config mutex on others
         {
            ossScopedLock lock( &_guardLatch, SHARED ) ;
            for ( UINT32 idx = 0 ; idx < _nodeGuards.size() ; ++idx )
            {
               if ( nodeGuard.muteXOn( &_nodeGuards[ idx ] ) )
               {
                  rc = SDB_CM_CONFIG_CONFLICTS ;
                  goto error ;
               }
            }
         }
      }

      if ( isModify || !arg2 )
      {
         goto done ;
      }

      // the first catalog node, need to write sdb.cat file
      try
      {
         CHAR cataCfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         BSONObj objArg2( arg2 ) ;
         stringstream ss ;
         if ( objArg2.isEmpty() )
         {
            goto done ;
         }
         rc = utilBuildFullPath( cfgPath, PMD_DFT_CAT, OSS_MAX_PATHSIZE,
                                 cataCfgFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build cat config file failed in service[%s], "
                    "rc: %d", pSvcName, rc ) ;
            goto error ;
         }
         ss << objArg2 << endl ;

         // write sdb.cat file
         rc = utilWriteConfigFile( cataCfgFile, ss.str().c_str(), TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write cat file[%s] failed in service[%s], rc: %d",
                    cataCfgFile, pSvcName, rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exeption for extract the second args for "
                 "service[%s]: %s", pSvcName, e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      if ( SDB_OK == rc )
      {
         if ( !isModify )
         {
            addNodeProcessInfo( pSvcName ) ;
            addNodeGuard( nodeGuard ) ;
            PD_LOG( PDEVENT, "Add node[%s] succeed", pSvcName ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Modify node[%s] succeed", pSvcName ) ;
         }
      }
      if ( hasLock )
      {
         releaseBucket( pSvcName ) ;
      }
      return rc ;
   error:
      if ( createCfgFile )
      {
         ossDelete( cfgFile ) ;
      }
      if ( createCfgPath )
      {
         ossDelete( cfgPath ) ;
      }
      if ( createDBPath )
      {
         ossDelete( dbPath ) ;
      }
      goto done ;
   }

   INT32 _omAgentNodeMgr::rmANode( const CHAR *arg1, const CHAR *arg2,
                                   const CHAR *roleStr, string *omsvc )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = _getSvcNameFromArg( arg1 ) ;
      BOOLEAN backupDialog = FALSE ;
      CHAR  cfgPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR  cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      pmdOptionsCB nodeOptions ;
      BOOLEAN hasLock = FALSE ;

      if ( !pSvcName )
      {
         PD_LOG( PDERROR, "Failed to get svc name from arg" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( arg2 )
      {
         try
         {
            BSONObj objArg2( arg2 ) ;
            if ( !objArg2.isEmpty() )
            {
               backupDialog = TRUE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Ocuur exception: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      rc = utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                              pSvcName, OSS_MAX_PATHSIZE, cfgPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config path for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( cfgPath, PMD_DFT_CONF, OSS_MAX_PATHSIZE,
                              cfgFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config file for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      // lock
      lockBucket( pSvcName ) ;
      hasLock = TRUE ;

      // read config
      rc = nodeOptions.initFromFile( cfgFile, FALSE ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = SDBCM_NODE_NOTEXISTED ;
         }
         else
         {
            PD_LOG( PDERROR, "Extract node[%s] config failed, rc: %d",
                    pSvcName, rc ) ;
         }
         goto error ;
      }
      if ( omsvc )
      {
         *omsvc = nodeOptions.getOMService() ;
      }

      if ( roleStr && 0 != *roleStr &&
           0 != ossStrcmp( nodeOptions.dbroleStr(), roleStr ) )
      {
         PD_LOG_MSG( PDERROR, "Role[%s] is not expect[%s]",
                 nodeOptions.dbroleStr(), roleStr ) ;
         rc = SDB_PERM ;
         goto error ;
      }

      // check config whether is matched
      try
      {
         BSONObj configObj( arg1 ) ;
         BSONObjIterator itr( configObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), PMD_OPTION_CLUSTER_NAME ) &&
                 0 != ossStrcmp( e.fieldName(), PMD_OPTION_BUSINESS_NAME ) &&
                 0 != ossStrcmp( e.fieldName(), PMD_OPTION_USERTAG ) )
            {
               continue ;
            }
            string name ;
            nodeOptions.getFieldStr( e.fieldName(), name, "" ) ;
            if ( 0 != ossStrcmp( e.valuestrsafe(), name.c_str() ) )
            {
               // not the special node
               rc = SDBCM_NODE_NOTEXISTED ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace config obj occur exception: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // first to stop the node
      rc = stopANode( pSvcName, NODE_START_CLIENT, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Stop node[%s] failed before remove it, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      // make sure need to backup dialog
      if ( backupDialog )
      {
         CHAR bakPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

         ossStrncpy( bakPath, nodeOptions.getDbPath(),
                     OSS_MAX_PATHSIZE ) ;
         UINT32 bkPathLen = ossStrlen( bakPath ) ;
         if ( bkPathLen > 0 &&
              OSS_FILE_SEP_CHAR == bakPath[ bkPathLen - 1 ] )
         {
            bakPath[ bkPathLen - 1 ] = 0 ;
         }
         ossStrncat( bakPath, "_bak", OSS_MAX_PATHSIZE ) ;

         if ( SDB_OK == ossAccess( bakPath ) )
         {
            ossDelete( bakPath ) ;
         }
         if ( SDB_OK == ( rc = ossRenamePath( nodeOptions.getDiagLogPath(),
                                              bakPath ) ) )
         {
            PD_LOG( PDEVENT, "Move node[%s] dialog[%s] to path[%s]",
                    pSvcName, nodeOptions.getDiagLogPath(), bakPath ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Move node[%s] dialog[%s] to path[%s] failed, "
                    "rc: %d", pSvcName, nodeOptions.getDiagLogPath(),
                    bakPath, rc ) ;
         }
      }

      // remove all dir
      rc = nodeOptions.removeAllDir() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Remove node[%s] directorys failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      // remove config
      rc = ossDelete( cfgPath ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to rm conf path: %s, rc: %d",
                 cfgPath, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Remove node[svcname=%s] succeed.", pSvcName ) ;

      // remove from process info
      delNodeProcessInfo( pSvcName ) ;
      delNodeGuard( pSvcName ) ;

   done:
      if ( hasLock )
      {
         releaseBucket( pSvcName ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::mdyANode( const CHAR * arg1 )
   {
      return _addANode( arg1, NULL, TRUE, TRUE ) ;
   }

   INT32 _omAgentNodeMgr::startANode( const CHAR *arg1 )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = _getSvcNameFromArg( arg1 ) ;

      if ( !pSvcName )
      {
         PD_LOG( PDERROR, "Get svcname form arg failed" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = startANode( pSvcName, NODE_START_CLIENT, TRUE ) ;
      if ( SDBCM_SVC_STARTED == rc )
      {
         PD_LOG( PDERROR, "Node[%s] has already started", pSvcName ) ;
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Start node[%s] failed, rc: %d", pSvcName, rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDEVENT, "Start SequoiaDB node succeed, svcname = %s",
                 pSvcName ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::stopANode( const CHAR * arg1 )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = _getSvcNameFromArg( arg1 ) ;

      if ( !pSvcName )
      {
         PD_LOG( PDERROR, "Get svcname form arg failed" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = stopANode( pSvcName, NODE_START_CLIENT, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Stop sequoiadb node[%s] failed, rc: %d",
                   pSvcName, rc ) ;

      PD_LOG( PDEVENT, "Stop Sequoiadb node succeed, svcname = %s",
              pSvcName ) ;

      /// If detach, need to remove self from cataAddr
      try
      {
         BSONObj argObj( arg1 ) ;
         BSONElement eHost = argObj.getField( FIELD_NAME_HOST ) ;
         BSONElement eDetach = argObj.getField( FIELD_NAME_ONLY_DETACH ) ;
         BSONElement eKeep = argObj.getField( FIELD_NAME_KEEP_DATA ) ;

         if ( eDetach.booleanSafe() && eKeep.booleanSafe() &&
              String == eHost.type() )
         {
            CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
            pmdOptionsCB nodeOptions ;

            utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                               pSvcName, OSS_MAX_PATHSIZE, confFile ) ;
            utilCatPath( confFile, OSS_MAX_PATHSIZE, PMD_DFT_CONF ) ;

            if ( SDB_OK == nodeOptions.initFromFile( confFile, FALSE ) )
            {
               nodeOptions.rmCatAddrItem( eHost.valuestr(),
                                          nodeOptions.catService() ) ;
               nodeOptions.reflush2File() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         /// ignore error
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::getOptions( const CHAR *arg1,
                                      bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = _getSvcNameFromArg( arg1 ) ;
      CHAR  cfgPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR  cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      pmdOptionsCB nodeOptions ;
      BOOLEAN hasLock = FALSE ;

      rc = utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                              pSvcName, OSS_MAX_PATHSIZE, cfgPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config path for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( cfgPath, PMD_DFT_CONF, OSS_MAX_PATHSIZE,
                              cfgFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config file for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      lockBucket( pSvcName ) ;
      hasLock = TRUE ;

      rc = nodeOptions.initFromFile( cfgFile, FALSE ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = SDBCM_NODE_NOTEXISTED ;
         }
         else
         {
            PD_LOG( PDERROR, "Extract node[%s] config failed, rc: %d",
                    pSvcName, rc ) ;
         }
         goto error ;
      }

      nodeOptions.toBSON( options ) ;
   done:
      if ( hasLock )
      {
         releaseBucket( pSvcName ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omAgentNodeMgr::clearData( const CHAR *arg1 )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSvcName = _getSvcNameFromArg( arg1 ) ;
      pmdOptionsCB nodeOptions ;
      BOOLEAN hasLock = FALSE ;
      CHAR  cfgPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR  cfgFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( !pSvcName )
      {
         PD_LOG( PDERROR, "Failed to get svc name from arg" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = utilBuildFullPath( sdbGetOMAgentOptions()->getLocalCfgPath(),
                              pSvcName, OSS_MAX_PATHSIZE, cfgPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config path for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( cfgPath, PMD_DFT_CONF, OSS_MAX_PATHSIZE,
                              cfgFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build config file for service[%s] failed, rc: %d",
                 pSvcName, rc ) ;
         goto error ;
      }

      lockBucket( pSvcName ) ;
      hasLock = TRUE ;

      rc = nodeOptions.initFromFile( cfgFile, FALSE ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = SDBCM_NODE_NOTEXISTED ;
         }
         else
         {
            PD_LOG( PDERROR, "Extract node[%s] config failed, rc: %d",
                    pSvcName, rc ) ;
         }
         goto error ;
      }

      try
      {
         BSONObj configObj( arg1 ) ;
         BSONObjIterator itr( configObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), PMD_OPTION_CLUSTER_NAME ) &&
                 0 != ossStrcmp( e.fieldName(), PMD_OPTION_BUSINESS_NAME ) &&
                 0 != ossStrcmp( e.fieldName(), PMD_OPTION_USERTAG ) )
            {
               continue ;
            }
            string name ;
            nodeOptions.getFieldStr( e.fieldName(), name, "" ) ;
            if ( 0 != ossStrcmp( e.valuestrsafe(), name.c_str() ) )
            {
               rc = SDBCM_NODE_NOTEXISTED ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace config obj occur exception: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = stopANode( pSvcName, NODE_START_CLIENT, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to stop node[%s]:%d", pSvcName, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "begin to delete data of node[%s]", pSvcName ) ;

      rc = nodeOptions.removeAllDir() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove dir of node[%s]:%d",
                 pSvcName, rc ) ;
         goto error ;
      }

      rc = nodeOptions.makeAllDir() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to make all dir of node[%s]:%d",
                 pSvcName, rc ) ;
         goto error ;
      }
   done:
      if ( hasLock )
      {
         releaseBucket( pSvcName ) ;
      }
      return rc ;
   error:
      goto done ;
   }
}


