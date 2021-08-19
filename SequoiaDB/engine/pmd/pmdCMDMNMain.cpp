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

   Source File Name = pmdDaemon.cpp

   Descriptive Name = pmdDaemon

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
#include "pmdDaemon.hpp"
#include "ossProc.hpp"
#include "utilStr.hpp"
#include "pmdDef.hpp"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "ossIO.hpp"
#include "pd.hpp"
#include "ossUtil.h"
#include "ossVer.h"
#include <iostream>

namespace engine
{

#if defined( _WINDOWS )
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_AS_PROC, "as process, not service" ) \

#else
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \

#endif // _WINDOWS

   void displayArg ( po::options_description &desc )
   {
      std::cout << "Usage:  sdbcmd [OPTION]" <<std::endl;
      std::cout << desc << std::endl ;
   }

   // initialize options
   INT32 initArgs ( INT32 argc, CHAR **argv, po::variables_map &vm,
                    BOOLEAN &asProc )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ( "Command options" ) ;

      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // validate arguments
      rc = utilReadCommandLine( argc, argv, desc, vm ) ;
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
      if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "Sdb CMD version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }
#if defined( _WINDOWS )
      if ( vm.count( PMD_OPTION_AS_PROC ) )
      {
         asProc = TRUE ;
      }
#endif //_WINDOWS

   done:
      return rc ;
   }

   INT32 mainEntry( INT32 argc, CHAR** argv )
   {
      INT32 rc = SDB_OK ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      cCMService svc;
      cPmdDaemon daemon( PMDDMN_SVCNAME_DEFAULT ) ;
      BOOLEAN asProc = FALSE ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      po::variables_map vm ;

      rc = ossGetEWD( dialogFile, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Failed to get working directory, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      rc = engine::utilCatPath( dialogFile, OSS_MAX_PATHSIZE, SDBCM_LOG_PATH ) ;
      if ( rc )
      {
         ossPrintf( "Failed to make dialog path, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      // make sure the dir exist
      rc = ossMkdir( dialogFile ) ;
      if ( rc && SDB_FE != rc )
      {
         ossPrintf( "Create dialog dir[%s] failed, rc: %d"OSS_NEWLINE,
                    dialogFile, rc ) ;
         goto error ;
      }
      rc = engine::utilCatPath( dialogFile, OSS_MAX_PATHSIZE,
                                PMDDMN_DIALOG_FILE_NAME ) ;
      if ( rc )
      {
         ossPrintf( "Failed to make dialog path, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

      rc = initArgs( argc, argv, vm, asProc ) ;
      if ( rc )
      {
         if ( SDB_PMD_VERSION_ONLY == rc || SDB_PMD_HELP_ONLY == rc )
         {
            rc = SDB_OK ;
         }
         goto error ;
      }

      // enable pd log
      sdbEnablePD( dialogFile ) ;
      setPDLevel( PDINFO ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start cmd[%s]...", verText ) ;

      svc.setArgInfo( argc, argv ) ;
      rc = svc.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cm(rc=%d)", rc ) ;
      rc = daemon.addChildrenProcess( &svc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add childrenProcess(rc=%d)", rc ) ;
      rc = daemon.init();
      if ( rc != SDB_OK )
      {
         ossPrintf( "Failed to init daemon process(rc=%d)", rc ) ;
         goto error;
      }

      rc = daemon.run( argc, argv, asProc );
      PD_RC_CHECK( rc, PDERROR, "Execute failed(rc=%d)", rc ) ;

   done:
      daemon.stop() ;
      PD_LOG( PDEVENT, "Stop programme." ) ;
      return SDB_OK == rc ? 0 : 1 ;
   error:
      goto done ;
   }

}

INT32 main( INT32 argc, CHAR** argv )
{
   return engine::mainEntry( argc, argv ) ;
}

