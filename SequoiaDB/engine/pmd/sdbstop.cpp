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

   Source File Name = sdbstop.cpp

   Descriptive Name = sdbstop Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbstop,
   which is used to stop SequoiaDB engine.

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
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdDef.hpp"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "utilCommon.hpp"
#include "utilNodeOpr.hpp"
#include "omagentDef.hpp"
#include "utilStr.hpp"
#include "ossVer.h"
#include "ossIO.hpp"

namespace engine
{
   #define SDBSTOP_LOG_FILE_NAME    "sdbstop.log"
   #define SDBSTOP_OPTION_ALL       "all"

   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" )\
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_COMMANDS_STRING( SDBSTOP_OPTION_ALL, ",a" ), "stop all nodes include db and om" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_TYPE, ",t" ), po::value<string>(), "node type: db/om/all" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_ROLE, ",r" ), po::value<string>(), "role type: coord/data/catalog/om" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_SVCNAME, ",p" ), po::value<string>(), "service name, separated by comma (',')" ) \
       ( PMD_OPTION_FORCE, "force stop when the node can't stop normally" )

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" ) \

   // initialize options
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

   void displayUsage()
   {
      std::cout << "Usage:" << endl ;
      std::cout << "  sdbstop -a            # stop all nodes include db and om"
                << endl;
      std::cout << "  sdbstop -t db         # stop db nodes" << endl ;
      std::cout << "  sdbstop -r data       # stop data nodes" << endl ;
      std::cout << "  sdbstop -p <svcname>  # stop the node with the specified service name"
                << endl ;
   }

   void displayArg ( po::options_description &desc )
   {
      displayUsage() ;
      std::cout << desc << std::endl ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSTOP_RESVARG, "resolveArgument" )
   INT32 resolveArgument ( po::options_description &desc,
                           po::options_description &all,
                           po::variables_map &vm,
                           INT32 argc, CHAR **argv,
                           vector<string> &listServices,
                           INT32 &typeFilter, INT32 &roleFilter,
                           BOOLEAN &bForce )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSTOP_RESVARG );

      rc = utilReadCommandLine2( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         std::cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if( 0 == vm.size() && 1 < argc )
      {
         std::cout << "Unrecongnized options: " << argv[1] <<endl ;
         rc = SDB_INVALIDARG ;
         std::cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if( 0 == vm.size() )
      {
         rc = SDB_INVALIDARG ;
         std::cout << "Sdbstop requires at least one parameter. You maybe want"
                   << " to stop all nodes.\nFor detail, please refer to "
                   << "\"sdbstop --all\""
                   << std::endl ;
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_HELP ) )
      {
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto error ;
      }

      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "Sdb Stop Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_SVCNAME ) )
      {
         string svcname = vm[PMD_OPTION_SVCNAME].as<string>() ;
         if( svcname.empty() )
         {
            std::cout << "Service name can't be empty" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // break service names using ';'
         rc = utilSplitStr( svcname, listServices, ", \t" ) ;
         if ( rc )
         {
            std::cout << "Parse svcname failed: " << rc << endl ;
            goto error ;
         }
      }

      if ( vm.count( PMD_OPTION_TYPE ) )
      {
         string listType = vm[ PMD_OPTION_TYPE ].as<string>() ;
         if ( 0 == ossStrcasecmp( listType.c_str(),
                                  SDBLIST_TYPE_DB_STR ) )
         {
            typeFilter = SDB_TYPE_DB ;
         }
         else if ( 0 == ossStrcasecmp( listType.c_str(),
                                       SDBLIST_TYPE_OM_STR ) )
         {
            typeFilter = SDB_TYPE_OM ;
         }
         else if ( 0 == ossStrcasecmp( listType.c_str(),
                                       SDBLIST_TYPE_ALL_STR ) )
         {
            typeFilter = -1 ;
         }
         else
         {
            std::cout << "type invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( vm.count( PMD_OPTION_ROLE ))
      {
         string roleTemp = vm[PMD_OPTION_ROLE].as<string>() ;
         roleFilter = utilGetRoleEnum( roleTemp.c_str() ) ;
         if ( SDB_ROLE_MAX == roleFilter ||
              SDB_ROLE_OMA == roleFilter )
         {
            std::cout << "role invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         typeFilter = -1 ;
      }

      if ( vm.count( PMD_OPTION_FORCE ) )
      {
         bForce = TRUE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_SDBSTOP_RESVARG, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSTOP_MAIN, "mainEntry" )
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSTOP_MAIN ) ;
      INT32 success = 0 ;
      INT32 total = 0 ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      vector<string> listServices ;
      UTIL_VEC_NODES listNodes ;
      UTIL_VEC_NODES::iterator itrNode ;
      BOOLEAN bFind = TRUE ;
      INT32 typeFilter = -1 ;
      INT32 roleFilter =  -1 ;
      BOOLEAN bForce = FALSE ;
      po::options_description desc ( "Command options" ) ;
      po::options_description all ( "Command options" ) ;
      po::variables_map vm ;

      init ( desc, all ) ;

      // validate arguments
      rc = resolveArgument ( desc, all, vm, argc, argv, listServices, typeFilter,
                             roleFilter, bForce ) ;
      if ( rc )
      {
         if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            std::cout << "Invalid argument" << endl ;
            displayArg ( desc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
         goto done ;
      }

      if ( !vm.count( PMD_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }

      // make path
      rc = ossGetEWD( dialogFile, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Error: Get module self path failed:  %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }
      // dialog path and file
      rc = utilCatPath( dialogFile, OSS_MAX_PATHSIZE, SDBCM_LOG_PATH ) ;
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
         // not go to error, continue
         rc = SDB_OK ;
      }
      rc = engine::utilCatPath( dialogFile, OSS_MAX_PATHSIZE,
                                SDBSTOP_LOG_FILE_NAME ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog file: %d" OSS_NEWLINE, rc ) ;
         // not go to error, continue
         rc = SDB_OK ;
      }
      // enable pd log
      sdbEnablePD( dialogFile ) ;
      setPDLevel( PDINFO ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start programme[%s]...", verText ) ;

      if ( listServices.size() > 0 )
      {
         // if used -p, list all nodes
         typeFilter = -1 ;
         roleFilter = -1 ;
      }

      // list all nodes
      utilListNodes( listNodes, typeFilter, NULL, OSS_INVALID_PID,
                     roleFilter ) ;

      itrNode = listNodes.begin() ;
      while( itrNode != listNodes.end() )
      {
         utilNodeInfo &info = *itrNode ;

         // can't stop oma
         if ( SDB_TYPE_OMA == info._type )
         {
            itrNode = listNodes.erase( itrNode ) ;
            continue ;
         }

         if ( listServices.size() > 0 )
         {
            bFind = FALSE ;
            for ( UINT32 j = 0 ; j < listServices.size() ; ++j )
            {
               if ( 0 == ossStrcmp( info._svcname.c_str(),
                                    listServices[ j ].c_str() ) )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
         }
         else
         {
            bFind = TRUE ;
         }

         if ( bFind )
         {
            ++total ;
            rc = utilAsyncStopNode( info ) ;
            if ( rc )
            {
               ossPrintf ( "Terminating process %d: %s(%s)" OSS_NEWLINE,
                           info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                           info._svcname.c_str() ) ;
               if ( SDB_CLS_NODE_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
                  ++success ;
                  ossPrintf ( "DONE" OSS_NEWLINE ) ;
               }
               else
               {
                  ossPrintf ( "FAILED" OSS_NEWLINE ) ;
               }

               itrNode = listNodes.erase( itrNode ) ;
               continue ;
            }
         }
         else
         {
            itrNode = listNodes.erase( itrNode ) ;
            continue ;
         }
         ++itrNode ;
      }

      /// The second time for wait
      itrNode = listNodes.begin() ;
      while( itrNode != listNodes.end() )
      {
         utilNodeInfo &info = *itrNode ;

         ossPrintf ( "Terminating process %d: %s(%s)" OSS_NEWLINE,
                     info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                     info._svcname.c_str() ) ;

         rc = utilStopNode( info, UTIL_STOP_NODE_TIMEOUT, bForce,
                            TRUE, TRUE ) ;
         if ( SDB_OK == rc )
         {
            ++success ;
            ossPrintf ( "DONE" OSS_NEWLINE ) ;
         }
         else
         {
            ossPrintf ( "FAILED" OSS_NEWLINE ) ;
         }
         ++itrNode ;
      }

      ossPrintf ( "Total: %d; Success: %d; Failed: %d" OSS_NEWLINE,
                  total, success, total - success ) ;

      if ( total == success )
      {
         rc = SDB_OK ;
      }
      else if ( success == 0 )
      {
         rc = STOPFAIL ;
      }
      else
      {
         rc = STOPPART ;
      }

   done :
      PD_LOG( PDEVENT, "Stop programme(%d).", rc ) ;
      PD_TRACE_EXITRC( SDB_SDBSTOP_MAIN, rc ) ;
      return ( rc >= 0 ) ? rc : utilRC2ShellRC( rc ) ;
   error:
      goto done ;
   }

}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}


