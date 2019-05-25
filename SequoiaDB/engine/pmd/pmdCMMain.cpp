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

      rc = utilReadCommandLine( argc, argv, all, vm ) ;
      if ( rc )
      {
         std::cout << "Invalid arguments: " << rc << std::endl ;
         displayArg ( desc ) ;
         goto done ;
      }

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

      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         std::cout << "Get current path failed: " << rc << std::endl ;
         goto error ;
      }
      ossChDir( currentPath ) ;

      rc = utilBuildFullPath( currentPath, SDBCM_LOG_PATH,
                              OSS_MAX_PATHSIZE, dialogPath ) ;
      if ( rc )
      {
         std::cout << "Build dialog path failed: " << rc << std::endl ;
         goto error ;
      }
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

      {
         string configs ;
         sdbGetOMAgentOptions()->toString( configs ) ;
         PD_LOG( PDEVENT, "All configs:\n%s", configs.c_str() ) ;
      }

      pmdSetDBRole( SDB_ROLE_OMA ) ;

      rc = pmdEnableSignalEvent( dialogPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 delSig ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to enable trap, rc: %d", rc ) ;

#if defined( _LINUX )
      signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

      PMD_REGISTER_CB( sdbGetOMAgentMgr() ) ;

      rc = krcb->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init krcb, rc: %d", rc ) ;

      {
         EDUID agentEDU = PMD_INVALID_EDUID ;
         pmdEDUMgr *eduMgr = krcb->getEDUMgr() ;
         rc = eduMgr->startEDU ( EDU_TYPE_PIPESLISTENER,
                                 (void*)sdbGetOMAgentOptions()->getCMServiceName(),
                                 &agentEDU ) ;
         PD_RC_CHECK( rc, PDERROR, "Start PIPELISTENER failed, rc: %d",
                      rc ) ;

         rc = eduMgr->waitUntil( agentEDU, PMD_EDU_RUNNING ) ;
         PD_RC_CHECK( rc, PDERROR, "Wait pipe listener to running "
                      "failed, rc: %d", rc ) ;
      }

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

      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs ( 1 ) ;
      }
      rc = krcb->getShutdownCode() ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
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

