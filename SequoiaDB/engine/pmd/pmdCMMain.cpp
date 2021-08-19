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

   Source File Name = pmdCMMain.cpp

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/17/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "utilStr.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "omagentMgr.hpp"
#include "pd.hpp"
#include "ossVer.h"
#include "pmd.hpp"
#include "pmdProc.hpp"
#include "utilPidFile.hpp"
#include "pmdPipeManager.hpp"

namespace engine
{

   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" ) \
      ( PMD_OPTION_STANDALONE, "use standalone mode to start" ) \
      ( PMD_OPTION_ALIVE_TIME, po::value<int>(), "alive time out" ) \
      ( PMD_OPTION_PORT, boost::program_options::value<string>(), "agent port" ) \

   void displayArg ( po::options_description &desc )
   {
      std::cout << "Usage:  sdbcm [OPTION]" <<std::endl;
      std::cout << desc << std::endl ;
   }

   // initialize options
   INT32 initArgs ( INT32 argc, CHAR **argv, po::variables_map &vm )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ( "Command options" ) ;
      po::options_description all ( "Command options" ) ;

      PMD_ADD_PARAM_OPTIONS_BEGIN ( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // validate arguments
      rc = utilReadCommandLine( argc, argv, all, vm ) ;
      if ( rc )
      {
         std::cout << "Invalid arguments: " << rc << std::endl ;
         displayArg ( desc ) ;
         goto done ;
      }

      /// read cmd first
      if ( vm.count( PMD_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "Sdb CM version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

   done:
      return rc ;
   }

   void pmdOnQuit()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
      iPmdProc::stop( 0 ) ;
   }

   INT32 pmdThreadMainEntry( INT32 argc, CHAR** argv )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR pidFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      INT32 delSig[] = { 17, 0 } ; // del SIGCHLD
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      po::variables_map vm ;

      rc = initArgs( argc, argv, vm ) ;
      if ( rc )
      {
         if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
         {
            rc = SDB_OK ;
         }
         goto done ;
      }

      // 1. get root path
      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         std::cout << "Get current path failed: " << rc << std::endl ;
         goto error ;
      }
      /// set current path
      ossChDir( currentPath ) ;

      // 2. enable dialog
      rc = utilBuildFullPath( currentPath, SDBCM_LOG_PATH,
                              OSS_MAX_PATHSIZE, dialogPath ) ;
      if ( rc )
      {
         std::cout << "Build dialog path failed: " << rc << std::endl ;
         goto error ;
      }
      // make sure the dir exist
      rc = ossMkdir( dialogPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cout << "Create dialog dir: " << dialogPath << " failed: "
                   << rc << std::endl ;
         goto error ;
      }
      rc = utilBuildFullPath( dialogPath, SDBCM_DIALOG_FILE_NAME,
                              OSS_MAX_PATHSIZE, dialogFile ) ;
      if ( rc )
      {
         std::cout << "Build dialog path failed: " << rc << std::endl ;
         goto error ;
      }
      sdbEnablePD( dialogFile ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start cm[%s]...", verText) ;

      // 3. create pid file
      rc = utilBuildFullPath( dialogPath, SDBCM_PID_FILE_NAME,
                              OSS_MAX_PATHSIZE, pidFile ) ;
      if ( rc )
      {
         std::cout << "Build dialog path failed: " << rc << std::endl ;
         goto error ;
      }
      rc = createPIDFile( pidFile ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to create pid file, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      // 4. init param
      rc = sdbGetOMAgentOptions()->init( currentPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init config, rc: %d", rc ) ;
         goto error ;
      }
      if ( vm.count( PMD_OPTION_CURUSER ) )
      {
         sdbGetOMAgentOptions()->setCurUser() ;
      }
      if ( vm.count( PMD_OPTION_STANDALONE ) )
      {
         sdbGetOMAgentOptions()->setStandAlone() ;
         if ( vm.count( PMD_OPTION_ALIVE_TIME ) )
         {
            UINT32 timeout = vm[ PMD_OPTION_ALIVE_TIME ].as<INT32>() ;
            sdbGetOMAgentOptions()->setAliveTimeout( timeout ) ;
         }
      }
      if ( vm.count( PMD_OPTION_PORT ) )
      {
         string svcname = vm[ PMD_OPTION_PORT ].as<string>() ;
         sdbGetOMAgentOptions()->setCMServiceName( svcname.c_str() ) ;
         pmdSetLocalPort( (UINT16)ossAtoi( svcname.c_str() ) ) ;
      }
      setPDLevel( sdbGetOMAgentOptions()->getDiagLevel() ) ;

      // 5. print all config
      {
         string configs ;
         sdbGetOMAgentOptions()->toString( configs ) ;
         PD_LOG( PDEVENT, "All configs:\n%s\nLimit info:\n%s",
                 configs.c_str(),
                 pmdGetLimit()->str().c_str() ) ;
      }

      pmdSetDBRole( SDB_ROLE_OMA ) ;

      // 6. handlers and init global mem
      rc = pmdEnableSignalEvent( dialogPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 delSig ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to enable trap, rc: %d", rc ) ;

#if defined( _LINUX )
      signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

      // 7. register agent cb
      PMD_REGISTER_CB( sdbGetOMAgentMgr() ) ;

      // 8. initialize pipe manager
      rc = sdbGetSystemPipeManager()->init(
                           sdbGetOMAgentOptions()->getCMServiceName(), TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, "
                   "rc: %d", rc ) ;

      // 9. init krcb
      rc = krcb->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init krcb, rc: %d", rc ) ;

      // 10. change process name
#if defined (_LINUX)
      {
         CHAR pmdProcessName [ OSS_RENAME_PROCESS_BUFFER_LEN + 1 ] = {0} ;
         ossSnprintf ( pmdProcessName, OSS_RENAME_PROCESS_BUFFER_LEN,
                       "%s(%s)", utilDBTypeStr( pmdGetDBType() ),
                       sdbGetOMAgentOptions()->getCMServiceName() ) ;
         ossEnableNameChanges ( argc, argv ) ;
         ossRenameProcess ( pmdProcessName ) ;
      }
#endif // _LINUX

      // Now master thread get into big loop and check shutdown flag
      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs ( 1 ) ;
         krcb->onTimer( OSS_ONE_SEC ) ;
      }
      rc = krcb->getShutdownCode() ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
      removePIDFile( pidFile ) ;
      krcb->destroy () ;
      PD_LOG ( PDEVENT, "Stop programme, exit code: %d",
               krcb->getShutdownCode() ) ;
      return rc == SDB_OK ? 0 : 1 ;
   error:
      goto done ;
   }

}

INT32 main( INT32 argc, CHAR** argv )
{
   return engine::pmdThreadMainEntry( argc, argv ) ;
}

