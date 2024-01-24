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

   Source File Name = sdbcmtop.cpp

   Descriptive Name = sdbcmtop Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcmtop,
   which is used to stop SequoiaDB Cluster Manager.

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
#include "pmdDaemon.hpp"
#include "ossProc.hpp"
#include "utilParam.hpp"
#include "utilStr.hpp"
#include "ossVer.h"
#include "pmdDef.hpp"
#include "utilParam.hpp"
#include "utilNodeOpr.hpp"
#include "utilCommon.hpp"
#include "pmdOptions.h"
#include "ossIO.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include <string>
#include <iostream>
#include <vector>

using namespace std;

namespace engine
{

   /*
      Macro define
   */
   #define SDBCMTOP_LOG_FILE_NAME      "sdbcmtop.log"
   #define SDBCMTOP_TIMEOUT            ( 30000 )

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

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" ) \
      ( PMD_OPTION_PORT, po::value<string>(), "agent port" ) \

   /*
      Function implement
   */
   void init ( po::options_description &desc,
               po::options_description &all )
   {
      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN ( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END
   }

   void displayArg ( po::options_description &desc )
   {
      std::cout << "Usage:  sdbcmtop [OPTION]" <<std::endl;
      std::cout << desc << std::endl ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CMSTOP_TERMPROC, "_terminateWithTimeout" )
   static INT32 _terminateWithTimeout ( OSSPID &pid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CMSTOP_TERMPROC );
      PD_TRACE1 ( SDB_CMSTOP_TERMPROC, PD_PACK_INT(pid) ) ;

      UINT32 timeout = 0 ;
      rc = ossTerminateProcess( pid, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed terminate process %d, errno=%d",
                  pid, ossGetLastError() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // wait until process terminate
      while ( ossIsProcessRunning( pid ) )
      {
         ossSleep( OSS_ONE_SEC ) ;
         timeout += OSS_ONE_SEC ;
         if ( timeout > SDBCMTOP_TIMEOUT )
         {
            break ;
         }
      }

      if ( timeout > SDBCMTOP_TIMEOUT )
      {
         rc = SDB_TIMEOUT ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CMSTOP_TERMPROC, rc );
      return rc ;
   error :
      goto done ;
   }

   static INT32 _stopSdbcmd ()
   {
      INT32 rc = SDB_OK ;
      INT32 rctmp = SDB_OK ;
      vector < ossProcInfo > procs ;

      ossEnumProcesses( procs, PMDDMN_EXE_NAME, TRUE, FALSE ) ;

      for ( UINT32 i = 0 ; i < procs.size() ; ++i )
      {
         ossPrintf( "Terminating process %d: %s" OSS_NEWLINE,
                    procs[ i ]._pid, PMDDMN_SVCNAME_DEFAULT ) ;
         rctmp = _terminateWithTimeout( procs[ i ]._pid ) ;
         if ( rctmp )
         {
            ossPrintf ( "FAILED" OSS_NEWLINE ) ;
            PD_LOG ( PDERROR, "Failed to terminate process %d, rc = %d",
                     procs[ i ]._pid, rctmp ) ;
            rc = rctmp ;
         }
         else
         {
            ossPrintf ( "DONE" OSS_NEWLINE ) ;
         }
      }

      return rc ;
   }

   static INT32 _stopSdbcm ( const string &port )
   {
      INT32 rc = SDB_OK ;
      INT32 rctmp = SDB_OK ;
      UTIL_VEC_NODES nodes ;

      utilListNodes( nodes, SDB_TYPE_OMA, port.empty() ? NULL : port.c_str(),
                     OSS_INVALID_PID, -1, port.empty() ? FALSE : TRUE ) ;

      for ( UINT32 i = 0 ; i < nodes.size() ; ++i )
      {
         utilNodeInfo &info = nodes[ i ] ;
         ossPrintf ( "Terminating process %d: %s(%s)" OSS_NEWLINE,
                     info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                     info._svcname.c_str() ) ;
         rctmp = utilStopNode( info ) ;
         if ( SDB_OK == rctmp )
         {
            ossPrintf ( "DONE" OSS_NEWLINE ) ;
         }
         else
         {
            rc = rctmp ;
            ossPrintf ( "FAILED" OSS_NEWLINE ) ;
         }
      }

      return rc ;
   }

   static INT32 _stopSdbcmByProc( const string &port )
   {
      INT32 rc = SDB_OK ;
      vector < ossProcInfo > procs ;
      UINT32 timewait = 5 ;

      _stopSdbcm( port ) ;

      if ( !port.empty() )
      {
         goto done ;
      }

      // wait sdbcmd quit
      while ( timewait > 0 )
      {
         --timewait ;
         procs.clear() ;
         ossEnumProcesses( procs, PMDDMN_EXE_NAME, TRUE, TRUE ) ;
         if ( procs.size() == 0 )
         {
            goto done ;
         }
         ossSleep( OSS_ONE_SEC ) ;
      }

      rc = _stopSdbcmd() ;

   done:
      return rc ;
   }

#if defined (_LINUX)

   INT32 stopSdbcm ( BOOLEAN asProc, const string &port )
   {
      return _stopSdbcmByProc( port ) ;
   }

#elif defined (_WINDOWS)

   INT32 stopSdbcm ( BOOLEAN asProc, const string &port )
   {
      if ( asProc )
      {
         return _stopSdbcmByProc( port ) ;
      }
      else
      {
         return ossStopService( PMDDMN_SVCNAME_DEFAULT,
                                SDBCMTOP_TIMEOUT ) ;
      }
   }

#endif // _LINUX

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CMSTOP_MAIN, "mainEntry" )
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CMSTOP_MAIN ) ;
      po::options_description desc ( "Command options" ) ;
      po::options_description all ( "Command options" ) ;
      po::variables_map vm ;
      ossResultCode result ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      BOOLEAN asProc = FALSE ;
      string port = "" ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      init ( desc, all ) ;
      // validate arguments
      rc = utilReadCommandLine ( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid arguments, rc: %d", rc ) ;
         displayArg ( desc ) ;
         goto done ;
      }
      /// read cmd first
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
         ossPrintVersion( "Sdb CM Stop version" ) ;
         //rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }
#if defined( _WINDOWS )
      if ( vm.count( PMD_OPTION_AS_PROC ) )
      {
         asProc = TRUE ;
      }
#endif // _WINDOWS

      // check user before create dir or files
      if ( !vm.count( PMD_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }
      if ( vm.count( PMD_OPTION_PORT ) )
      {
         port = vm[ PMD_OPTION_PORT ].as< string >() ;
         if ( !port.empty() )
         {
            asProc = TRUE ;
         }
      }

      rc = ossGetEWD ( dialogFile, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to get excutable file's working "
                     "directory" OSS_NEWLINE ) ;
         goto error ;
      }
      rc = engine::utilCatPath( dialogFile, OSS_MAX_PATHSIZE, SDBCM_LOG_PATH ) ;
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
      rc = engine::utilCatPath( dialogFile, OSS_MAX_PATHSIZE,
                                SDBCMTOP_LOG_FILE_NAME ) ;
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

      // stop cm
      rc = stopSdbcm ( asProc, port ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to stop sdbcm, rc: %d", rc ) ;
         ossPrintf ( "Failed to stop sdbcm, rc: %d" OSS_NEWLINE, rc ) ;
      }
      else
      {
         PD_LOG ( PDEVENT, "Successful to stop sdbcm" ) ;
         ossPrintf ( "Successful to stop sdbcm" OSS_NEWLINE ) ;
      }

   done:
      PD_LOG( PDEVENT, "Stop programme." ) ;
      PD_TRACE_EXITRC ( SDB_CMSTOP_MAIN, rc ) ;
      return SDB_OK == rc ? 0 : 1 ;
   error:
      goto done ;
   }

}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}

