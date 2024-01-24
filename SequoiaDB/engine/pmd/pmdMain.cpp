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

   Source File Name = pmdMain.cpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "pmd.hpp"
#include "rtn.hpp"
#include "ossProc.hpp"
#include "ossTimeZone.hpp"
#include "utilCommon.hpp"
#include "pmdStartup.hpp"
#include "optQgmStrategy.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdController.hpp"
#include "rtnBackgroundJob.hpp"
#include "pmdEnv.hpp"
#include "pmdStartupHistoryLogger.hpp"
#include "pmdPipeManager.hpp"

using namespace std;
using namespace bson;

namespace engine
{
   /*
    * This function resolve all input arguments from command line
    * It first construct options_description to register all
    * possible arguments we may have
    * And then it will to load from config file
    * Then it will parse command line input again to override config file
    * Basically we want to make sure all parameters that
    * specified in config file
    * can be simply overrided from commandline
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDRESVARGS, "pmdResolveArguments" )
   INT32 pmdResolveArguments( INT32 argc, CHAR** argv )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDRESVARGS ) ;
      CHAR exePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      rc = ossGetEWD( exePath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         std::cerr << "Get module path failed: " << rc << std::endl ;
         goto error ;
      }

      rc = pmdGetOptionCB()->init( argc, argv, exePath ) ;
      // if user only ask for help information, we simply return
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         PMD_SHUTDOWN_DB( SDB_OK ) ;
         rc = SDB_OK;
         goto done;
      }
      else if ( rc )
      {
         goto error;
      }

   done :
      PD_TRACE_EXITRC ( SDB_PMDRESVARGS, rc );
      return rc ;
   error :
      goto done ;
   }

   void pmdOnQuit()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
   }

   static INT32 _pmdSystemInit()
   {
      INT32 rc = SDB_OK ;
      SDB_START_TYPE startType = SDB_START_NORMAL ;
      BOOLEAN bOk = TRUE ;
      pmdStartupHistoryLogger *logger = pmdGetStartupHstLogger() ;

      // analysis the start type
      rc = pmdGetStartup().init( pmdGetOptionCB()->getDbPath() ) ;
      PD_RC_CHECK( rc, PDERROR, "Start up check failed[rc:%d]", rc ) ;

      startType = pmdGetStartup().getStartType() ;
      bOk = pmdGetStartup().isOK() ;
      PD_LOG( PDEVENT, "Start up from %s, data is %s",
              pmdGetStartTypeStr( startType ),
              bOk ? "normal" : "abnormal" ) ;

      // Init qgm strategy table
      rc = getQgmStrategyTable()->init() ;
      PD_RC_CHECK( rc, PDERROR, "Init qgm strategy table failed, rc: %d",
                   rc ) ;

      // write start-up info, ignore error
      rc = logger->init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to init start-up logger, rc: %d", rc );
         rc = SDB_OK ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static void  _pmdOnRebuildEnd( INT32 rc )
   {
      if ( SDB_OK == rc )
      {
         pmdGetKRCB()->callPrimaryChangeHandler( TRUE,
                                                 SDB_EVT_OCCUR_BEFORE ) ;
         pmdSetPrimary( TRUE ) ;
         pmdGetKRCB()->callPrimaryChangeHandler( TRUE,
                                                 SDB_EVT_OCCUR_AFTER ) ;
      }
      else
      {
         /// stop db
         PMD_SHUTDOWN_DB( rc ) ;
      }
   }

   static INT32 _pmdPostInit()
   {
      if ( SDB_ROLE_STANDALONE == pmdGetDBRole() ||
           SDB_ROLE_OM == pmdGetDBRole() )
      {
         if ( !pmdGetStartup().isOK() )
         {
            rtnStartRebuildJob( (RTN_ON_REBUILD_DONE_FUNC)_pmdOnRebuildEnd ) ;
         }
         else
         {
            pmdGetKRCB()->callPrimaryChangeHandler( TRUE,
                                                    SDB_EVT_OCCUR_BEFORE ) ;
            pmdSetPrimary( TRUE ) ;
            pmdGetKRCB()->callPrimaryChangeHandler( TRUE,
                                                    SDB_EVT_OCCUR_AFTER ) ;
         }

         if ( SDB_ROLE_STANDALONE == pmdGetDBRole() )
         {
            MsgRouteID standAloneID ;
            standAloneID.value             = MSG_INVALID_ROUTEID ;
            standAloneID.columns.groupID   = INVALID_GROUPID ;
            standAloneID.columns.nodeID    = DATA_NODE_ID_BEGIN ;
            pmdSetNodeID( standAloneID ) ;
         }
      }

      return SDB_OK ;
   }

   // based on millisecond
   #define PMD_START_WAIT_TIME         ( 60000 )

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDMSTTHRDMAIN, "pmdMasterThreadMain" )
   INT32 pmdMasterThreadMain ( INT32 argc, CHAR** argv )
   {
      INT32 rc                             = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDMSTTHRDMAIN );
      pmdKRCB *krcb                        = pmdGetKRCB () ;
      UINT32 startTimerCount               = 0 ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      // 1. read command line first
      rc = pmdResolveArguments ( argc, argv ) ;
      if ( rc )
      {
         ossPrintf( "Failed resolving arguments(error=%d), exit" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }
      if ( PMD_IS_DB_DOWN() )
      {
         return rc ;
      }

      // 2. enalble pd log
      sdbEnablePD( pmdGetOptionCB()->getDiagLogPath(),
                   pmdGetOptionCB()->diagFileNum() ) ;
      setPDLevel( (PDLEVEL)( pmdGetOptionCB()->getDiagLevel() ) ) ;
      if ( pmdGetOptionCB()->diagSecureOn() )
      {
         pdEnableDiaglogSecure() ;
      }
      else
      {
         pdDisableDiaglogSecure() ;
      }
      // enalble pd audit
      sdbEnableAudit( pmdGetOptionCB()->getAuditLogPath(),
                      pmdGetOptionCB()->auditFileNum() ) ;
      pdSetAuditMask( pmdGetOptionCB()->auditMask() ) ;
      pdInitCurAuditMask( pdGetAuditMask() ) ;
      pmdSetLocalPort( pmdGetOptionCB()->getServicePort() ) ;

      krcb->getMonMgr()->setMonitorStatus( pmdGetOptionCB()->monGroupMask() ) ;
      krcb->getMonMgr()->setHistEventSize( pmdGetOptionCB()->monHistEvent() ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;

      PD_LOG ( ( getPDLevel() > PDEVENT ? PDEVENT : getPDLevel() ) ,
               "Start sequoiadb(%s) [%s]...",
               pmdGetOptionCB()->krcbRole(), verText ) ;

      // 3. printf all configs
      {
         BSONObj confObj ;
         krcb->getOptionCB()->toBSON( confObj ) ;
         PD_LOG( PDEVENT, "All configs: %s", confObj.toString().c_str() ) ;
      }

      ossSetSysMemInfo( pmdGetOptionCB()->getMemMXFast(),
                        pmdGetOptionCB()->getMemTrimThreshold(),
                        pmdGetOptionCB()->getMemMmapThreshold(),
                        pmdGetOptionCB()->getMemMmapMax(),
                        pmdGetOptionCB()->getMemTopPad() ) ;

      // 4. dump limit info
      {
         PD_LOG( PDEVENT, "dump limit info:\n%s",
                 pmdGetLimit()->str().c_str() ) ;
         INT64 sort = -1 ;
         INT64 hard = -1 ;
         if ( !pmdGetLimit()->getLimit( OSS_LIMIT_VIRTUAL_MEM, sort, hard ) )
         {
            PD_LOG( PDWARNING, "can not get limit of memory space!" ) ;
         }
         else if ( -1 != sort || -1 != hard )
         {
            PD_LOG( PDWARNING, "virtual memory is not unlimited!" ) ;
         }
      }

      // 5. Initialize TZ, ignore error
      rc = ossInitTZEnv() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to init the TZ environment variable, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      // 6. handlers and init global mem
      rc = pmdEnableSignalEvent( pmdGetOptionCB()->getDiagLogPath(),
                                 (PMD_ON_QUIT_FUNC)pmdOnQuit ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to enable trap, rc: %d", rc ) ;

      // 7. register cbs
      sdbGetPMDController()->registerCB( pmdGetDBRole() ) ;

      // 8. system init
      rc = _pmdSystemInit() ;
      if ( rc )
      {
         goto error ;
      }

      // 9. initialize pipe manager
      rc = sdbGetSystemPipeManager()->init( pmdGetOptionCB()->getServiceAddr(),
                                            TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, "
                   "rc: %d", rc ) ;

      // 10. init krcb
      rc = krcb->init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init krcb, rc: %d", rc ) ;
         goto error ;
      }

      // 11. post init
      rc = _pmdPostInit() ;
      if ( rc )
      {
         goto error ;
      }

      // wait until all daemon threads start
      while ( PMD_IS_DB_UP() && startTimerCount < PMD_START_WAIT_TIME &&
              !krcb->isBusinessOK() )
      {
         ossSleepmillis( 100 ) ;
         startTimerCount += 100 ;
      }

      if ( PMD_IS_DB_DOWN() )
      {
         rc = krcb->getShutdownCode() ;
         PD_LOG( PDERROR, "Start failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( startTimerCount >= PMD_START_WAIT_TIME )
      {
         PD_LOG( PDWARNING, "Start warning (timeout)" ) ;
      }

#if defined (_LINUX)
      {
         // once all threads starts ( especially we need to make sure the
         // TcpListener thread is successfully started ), we can rename the
         // process. Otherwise if TcpListener failed
         CHAR pmdProcessName [ OSS_RENAME_PROCESS_BUFFER_LEN + 1 ] = {0} ;
         ossSnprintf ( pmdProcessName, OSS_RENAME_PROCESS_BUFFER_LEN,
                       "%s(%s) %s", utilDBTypeStr( pmdGetDBType() ),
                       pmdGetOptionCB()->getServiceAddr(),
                       utilDBRoleShortStr( pmdGetDBRole() ) ) ;
         // rename the process to append port number and service type
         ossEnableNameChanges ( argc, argv ) ;
         ossRenameProcess ( pmdProcessName ) ;
      }
#endif // _LINUX

      // Now master thread get into big loop and check shutdown flag
      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs ( 1 ) ;
         sdbGetPMDController()->onTimer( OSS_ONE_SEC ) ;
         krcb->onTimer( OSS_ONE_SEC ) ;
      }
      rc = krcb->getShutdownCode() ;

   done :
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
      krcb->destroy () ;
      rc = krcb->getShutdownCode() ;
      if ( krcb->needRestart() )
      {
         pmdGetStartup().restart( TRUE, rc ) ;
      }
      pmdGetStartup().final() ;
      pmdDisableSignalEvent() ;
      PD_LOG ( PDEVENT, "Stop sequoiadb, exit code: %d",
               krcb->getShutdownCode() ) ;
      PD_TRACE_EXITRC ( SDB_PMDMSTTHRDMAIN, rc );
      return utilRC2ShellRC( rc ) ;
   error :
      goto done ;
   }

}

/**************************************/
/*   DATABASE MAIN FUNCTION           */
/**************************************/
//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDMAIN, "main" )
INT32 main ( INT32 argc, CHAR** argv )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_PMDMAIN );
   rc = engine::pmdMasterThreadMain ( argc, argv ) ;
   PD_TRACE_EXITRC ( SDB_PMDMAIN, rc );
   return rc ;
}

