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

   Source File Name = sdbcmart.cpp

   Descriptive Name = sdbcmart Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcmStart,
   which is used to start SequoiaDB Cluster Manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossProc.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "ossProc.hpp"
#include "pmdDaemon.hpp"
#include "pmdDef.hpp"
#include "utilParam.hpp"
#include "utilNodeOpr.hpp"
#include "pmdOptions.h"
#include "utilStr.hpp"
#include "ossVer.h"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "ossIO.hpp"
#include <string>
#include <iostream>

using namespace std ;

namespace engine
{

   #define SDBCMART_LOG_FILE_NAME      "sdbcmart.log"
   #define PMD_SDBCM_WAIT_TIMEOUT      ( 30 )   /// seconds

#if defined( _WINDOWS )
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_AS_PROC, "as process, not service" ) \

#else
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_IGNOREULIMIT, ",i"), "skip checking ulimit" )\

#endif // _WINDOWS

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" ) \
      ( PMD_OPTION_STANDALONE, "use standalone mode to start" ) \
      ( PMD_OPTION_ALIVE_TIME, po::value<int>(), "alive time out" ) \
      ( PMD_OPTION_PORT, po::value<string>(), "agent port" ) \

   void displayArg ( po::options_description &desc )
   {
      std::cout << "Usage:  sdbcmart [OPTION]" <<std::endl;
      std::cout << desc << std::endl ;
   }

#if defined (_WINDOWS)

      INT32 startSdbcm ( list<const CHAR*> &argv, OSSPID &pid, BOOLEAN asProc )
      {
         if ( asProc )
         {
            return ossStartProcess( argv, pid ) ;
         }
         else
         {
            return ossStartService( PMDDMN_SVCNAME_DEFAULT ) ;
         }
      }

#elif defined (_LINUX)

      INT32 startSdbcm ( list<const CHAR*> &argv, OSSPID &pid, BOOLEAN asProc )
      {
         return ossStartProcess( argv, pid ) ;
      }
#endif // _WINDOWS

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CMMINTHREADENTY, "mainThreadEntry" )
   INT32 mainThreadEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CMMINTHREADENTY );
      list<const CHAR*> argvs ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      CHAR progName[OSS_MAX_PATHSIZE+1] = {0};
      po::options_description desc( "Command options" ) ;
      po::options_description all( "Command options" ) ;
      po::variables_map vm ;
      OSSPID pid = OSS_INVALID_PID ;
      utilNodeInfo cmInfo ;
      vector < ossProcInfo > procs ;
      BOOLEAN asProc = FALSE ;
      BOOLEAN asStandalone = FALSE ;
      string  procShortName = PMDDMN_EXE_NAME ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN ( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      /// 1.validate arguments
      rc = utilReadCommandLine( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid arguments, rc: %d", rc ) ;
         displayArg ( desc ) ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         //rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "Sdb CM Start version" ) ;
         //rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }
#if defined( _WINDOWS )
      if ( vm.count( PMD_OPTION_AS_PROC ) )
      {
         asProc = TRUE ;
      }
#endif //_WINDOWS
      if ( vm.count( PMD_OPTION_STANDALONE ) )
      {
         asStandalone = TRUE ;
         asProc = TRUE ;
         procShortName = SDBSDBCMPROG ;
      }

      /// 2.check ulimit
#if defined( _LINUX )
      if ( !vm.count( PMD_OPTION_IGNOREULIMIT ) )
      {
         rc = utilSetAndCheckUlimit() ;
         if ( rc )
         {
            ossPrintf( "Error: Start sequoiadb will set ulimit by file"
                       "[conf/limits.conf], if you want to set ulimit by "
                       "current terminal, please use parameter '-i'."
                       OSS_NEWLINE  ) ;
            goto error ;
         }
      }
#endif

      /// 3.check user info before create dir or files
      if ( !vm.count( PMD_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }

      /// 4.build dialog info
      rc = ossGetEWD ( progName, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to get excutable file's working "
                     "directory" OSS_NEWLINE ) ;
         goto error ;
      }
      rc = utilBuildFullPath( progName, SDBCM_LOG_PATH,
                              OSS_MAX_PATHSIZE, dialogFile ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog path: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }
      // make sure the dir exist
      rc = ossMkdir( dialogFile ) ;
      if ( rc && SDB_FE != rc )
      {
         ossPrintf( "Create dialog dir[%s] failed, rc: %d" OSS_NEWLINE,
                    dialogFile, rc ) ;
         goto error ;
      }
      rc = utilCatPath( dialogFile, OSS_MAX_PATHSIZE,
                        SDBCMART_LOG_FILE_NAME ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog file: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }
      // enable pd log
      sdbEnablePD( dialogFile ) ;
      setPDLevel( PDINFO ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start programme[%s]...", verText ) ;

      utilCatPath( progName, OSS_MAX_PATHSIZE, procShortName.c_str() ) ;
      argvs.push_back( progName ) ;
      for ( INT32 i = 1; i < argc ; ++i )
      {
         argvs.push_back( argv[i] ) ;
      }
#if defined( _LINUX )
      if ( vm.count( PMD_OPTION_IGNOREULIMIT ) )
      {
         PD_LOG( PDWARNING, "Start programme with setting ulimit based on "
                 "current terminal" ) ;
      }
#endif
      if ( !asStandalone )
      {
         // first to check whether the process exist
         ossEnumProcesses( procs, procShortName.c_str(), TRUE, TRUE ) ;
         if ( procs.size() > 0 )
         {
            // find it
            ossPrintf( "Success: sdbcmd is already started (%d)" OSS_NEWLINE,
                       (*procs.begin())._pid ) ;
            goto done ;
         }
      }

      /// 5.start progress
      rc = startSdbcm ( argvs, pid, asProc ) ;
      if ( rc )
      {
         ossPrintf ( "Error: Failed to start sdbcm, rc: %d" OSS_NEWLINE,
                     rc ) ;
         goto error ;
      }

      if ( !asStandalone )
      {
         while ( ossIsProcessRunning( pid ) )
         {
            procs.clear() ;
            ossEnumProcesses( procs, procShortName.c_str(), TRUE, TRUE ) ;
            if ( procs.size() > 0 )
            {
               ossPrintf( "Success: sdbcmd is successfully started (%d)"
                          OSS_NEWLINE, (*procs.begin())._pid ) ;
               break ;
            }
            ossSleep( 200 ) ;
         }

         if ( procs.size() == 0 )
         {
            ossPrintf ( "Error: Failed to start sdbcm, rc: %d" OSS_NEWLINE,
                        rc ) ;
            goto error ;
         }
      }

      /// 6.wait bussiness ok
      rc = utilWaitNodeOK( cmInfo, NULL,
                           asStandalone ? pid : OSS_INVALID_PID,
                           SDB_TYPE_OMA, PMD_SDBCM_WAIT_TIMEOUT,
                           asStandalone ? TRUE : FALSE ) ;
      if ( SDB_OK == rc )
      {
         ossPrintf ( "Success: %s(%s) is successfully started (%d)"
                     OSS_NEWLINE, SDB_TYPE_OMA_STR, cmInfo._svcname.c_str(),
                     cmInfo._pid ) ;
      }
      else
      {
         ossPrintf ( "Error: Failed to wait sdbcm ok, rc: %d" OSS_NEWLINE,
                     rc ) ;
         goto error ;
      }

   done:
      PD_LOG( PDEVENT, "Stop programme." ) ;
      PD_TRACE_EXITRC ( SDB_CMMINTHREADENTY, rc );
      return SDB_OK == rc ? 0 : 1 ;
   error:
      goto done ;
   }

}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainThreadEntry( argc, argv ) ;
}

