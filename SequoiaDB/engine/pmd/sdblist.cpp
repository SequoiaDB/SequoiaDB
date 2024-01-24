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

   Source File Name = sdblist.cpp

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
#include "ossPath.hpp"
#include "msgDef.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdDef.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"
#include "utilNodeOpr.hpp"
#include "utilCommon.hpp"
#include "ossVer.h"
#include "utilParam.hpp"
#include "utilStr.hpp"
#include "pmdDaemon.hpp"
#include "../bson/bson.h"
#include <string>
#include <iostream>
#include <vector>

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      OPTION DEFINE
   */
   #define SDB_CONF_FILE_PATH_FORMAT   SDBCM_LOCAL_PATH OSS_FILE_SEP "%s" OSS_FILE_SEP PMD_DFT_CONF

   /*
      COMMANDS OPTION DEFINE
   */
   #define PMD_OPTION_LONG_LOCATION    "long-location"

   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_TYPE, ",t" ), po::value<string>(), "node type: db/om/cm/all, default: db" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_SVCNAME, ",p" ), po::value<string>(), "service name, use ',' as seperator" )  \
       ( PMD_COMMANDS_STRING( PMD_OPTION_MODE, ",m" ), po::value<string>(),"mode type: run/local, default: run" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_ROLE, ",r" ), po::value<string>(), "role type: coord/data/catalog/om/cm" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_LONG, ",l" ), "show long style" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_DETAIL, "show details" ) \
       ( PMD_OPTION_EXPAND, "show expanded details" ) \
       ( PMD_OPTION_LONG_LOCATION, "show long style with location" )


   /*
      Long format define
   */
   #define PMD_LIST_LONG_FORMAT  "%-10.9s %-13.12s %-11.10s %-9.8s %-6.5s %-6.5s %-4.3s %-20.19s %-20.19s %s"
   #define PMD_LIST_TITLE        "Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath"

   /*
      Long location format define
   */
   #define PMD_LIST_LONG_LOCATION  "%-10.9s %-13.12s %-11.10s %-9.8s %-6.5s %-6.5s %-4.3s %-20.19s %-20.19s %-7.6s %-20.19s %s"
   #define PMD_LIST_LOCATION_TITLE "Name       SvcName       Role        PID       GID    NID    PRY  GroupName            Location             LocPRY  StartTime            DBPath"

   //print node's detail configuration by sdb conf file and svcname
   void _printfDetail( const CHAR *rootPath, const CHAR *svcname, INT32 type )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;
      desc.add_options()
         ( "*", po::value<string>(), "" ) ;
      CHAR sdbConfPath[OSS_MAX_PATHSIZE + 1] = { 0 } ;

      if( (SDB_TYPE)type != SDB_TYPE_OMA )
      {
         CHAR sdbConfTemp[OSS_MAX_PATHSIZE + 1] = { 0 } ;
         ossSnprintf( sdbConfTemp, OSS_MAX_PATHSIZE,
                      SDB_CONF_FILE_PATH_FORMAT, svcname ) ;
         utilBuildFullPath( rootPath, sdbConfTemp,
                            OSS_MAX_PATHSIZE, sdbConfPath ) ;
      }
      else
      {
         utilBuildFullPath( rootPath, SDBCM_CONF_PATH_FILE,
                            OSS_MAX_PATHSIZE, sdbConfPath ) ;
      }
      rc = ossAccess( sdbConfPath, 0 ) ;
      if ( rc )
      {
         return ;
      }
      utilReadConfigureFile( sdbConfPath, desc, vm ) ;

      po::variables_map::iterator it = vm.begin() ;
      while( it != vm.end() )
      {
         ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, it->first.data(),
                    it->second.as<string>().c_str() ) ;
         ++it ;
      }
   }

   //print node's expand configuration by sdb conf file and svcname
   void _printfExpand( const CHAR *rooPath, const CHAR *svcname, INT32 type )
   {
      INT32 rc = SDB_OK ;
      BSONObj objData ;
      CHAR sdbConfPath[OSS_MAX_PATHSIZE + 1] = { 0 } ;

      if( (SDB_TYPE)type != SDB_TYPE_OMA )
      {
         CHAR sdbConfTemp[OSS_MAX_PATHSIZE + 1] = { 0 } ;
         ossSnprintf( sdbConfTemp, OSS_MAX_PATHSIZE,
                      SDB_CONF_FILE_PATH_FORMAT, svcname ) ;
         utilBuildFullPath( rooPath, sdbConfTemp,
                            OSS_MAX_PATHSIZE, sdbConfPath ) ;

         engine::pmdOptionsCB option ;
         rc = ossAccess( sdbConfPath, 0 ) ;
         if ( rc )
         {
            return  ;
         }
         option.initFromFile( sdbConfPath, FALSE ) ;
         option.toBSON( objData ) ;
         BSONObjIterator it = objData.begin() ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            if( e.type() == String )
            {
               ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(),
                          e.valuestr() ) ;
            }
            else if( e.type() == NumberInt )
            {
               ossPrintf( "   %-18.18s: %d" OSS_NEWLINE, e.fieldName(),
                          e.numberInt() ) ;
            }
            else if( e.type() == NumberLong )
            {
               ossPrintf( "   %-18.18s: %lld" OSS_NEWLINE, e.fieldName(),
                          e.numberLong() ) ;
            }
            else if( e.type() == NumberDouble )
            {
               ossPrintf( "   %-18.18s: %f" OSS_NEWLINE, e.fieldName(),
                          e.numberDouble() ) ;
            }
            else if( e.type() == Bool )
            {
               ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(),
                          (e.boolean() ? "TRUE" : "FALSE" ) ) ;
            }
            else
            {
               ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(), "-" ) ;
            }
         }
      }
      else
      {
         _printfDetail( rooPath, NULL, type ) ;
      }
   }

   //printf detail or expand
   void _printfAll( const CHAR *rooPath, utilNodeInfo &node,
                    BOOLEAN detail, BOOLEAN expand,
                    BOOLEAN showLong, BOOLEAN showLocation )
   {
      CHAR tmpPID[ 11 ] = { '-', 0 } ;

      if ( node._pid != OSS_INVALID_PID )
      {
         ossSnprintf( tmpPID, sizeof( tmpPID ) - 1, "%d", node._pid ) ;
      }

      if ( showLong || showLocation )
      {
         struct tm otm ;
         time_t tt = node._startTime ;

         CHAR tmpGID[ 11 ] = { '-', 0 } ;
         CHAR tmpNID[ 11 ] = { '-', 0 } ;
         CHAR tmpPRY[ 11 ] = { '-', 0 } ;
         CHAR tmpTime[ 21 ] = { 0 } ;
         string roleStr = utilDBRoleStr( (SDB_ROLE)node._role ) ;
         // Long style
         // name       svcname       role        pid    gid    nid    PRY    GroupName           StartTime            dbpath
         // sequoaidb  11810         standalone  15896  1001   1001   Y      db1                 2014-02-02-11:01:01  /opt/sequoiadb/database/coord/11810
         // sdbcm      11790         -           10076  -      -             -                   2014-02-02-11:01:01  -

         // Long location style
         // name       svcname       role        pid    gid    nid    PRY    GroupName           Location           LocPRY    StartTime            dbpath
         // sequoaidb  11810         standalone  15896  1001   1001   Y      db1                 A                  Y         2014-02-02-11:01:01  /opt/sequoiadb/database/coord/11810
         // sdbcm      11790         -           10076  -      -      -      -                   -                  -         2014-02-02-11:01:01  -

#if defined (_WINDOWS)
         localtime_s( &otm, &tt ) ;
#else
         localtime_r( &tt, &otm ) ;
#endif
         ossSnprintf( tmpTime, sizeof( tmpTime ) - 1,
                      "%04d-%02d-%02d-%02d.%02d.%02d",
                      otm.tm_year+1900,
                      otm.tm_mon+1,
                      otm.tm_mday,
                      otm.tm_hour,
                      otm.tm_min,
                      otm.tm_sec ) ;

         if ( 0 != node._groupID )
         {
            ossSnprintf( tmpGID, sizeof( tmpGID ) - 1, "%d", node._groupID ) ;
         }
         if ( 0 != node._nodeID )
         {
            ossSnprintf( tmpNID, sizeof( tmpNID ) - 1, "%d", node._nodeID ) ;
         }

         if ( -1 != node._primary )
         {
            ossStrcpy( tmpPRY, ( 1 == node._primary ) ? "Y" : "N" ) ;
         }

         if ( showLocation )
         {
            CHAR tmpLocPRY[ 11 ] = { '-', 0 } ;
            if ( -1 != node._locPrimary && ! node._location.empty() )
            {
               ossStrcpy( tmpLocPRY, ( 1 == node._locPrimary ) ? "Y" : "N" ) ;
            }
            ossPrintf( PMD_LIST_LONG_LOCATION OSS_NEWLINE,
                       utilDBTypeStr( (SDB_TYPE)node._type ),
                       node._svcname.c_str(),
                       roleStr.empty() ? "-" : roleStr.c_str(),
                       tmpPID,
                       tmpGID,
                       tmpNID,
                       tmpPRY,
                       node._groupName.empty() ? "-" : node._groupName.c_str(),
                       node._location.empty() ?  "-" : node._location.c_str(),
                       tmpLocPRY,
                       tmpTime,
                       node._dbPath.empty() ? "-" : node._dbPath.c_str() ) ;
         }
         else
         {
            ossPrintf( PMD_LIST_LONG_FORMAT OSS_NEWLINE,
                       utilDBTypeStr( (SDB_TYPE)node._type ),
                       node._svcname.c_str(),
                       roleStr.empty() ? "-" : roleStr.c_str(),
                       tmpPID,
                       tmpGID,
                       tmpNID,
                       tmpPRY,
                       node._groupName.empty() ? "-" : node._groupName.c_str(),
                       tmpTime,
                       node._dbPath.empty() ? "-" : node._dbPath.c_str() ) ;
         }
      }
      else
      {
         ossPrintf( "%s(%s) (%s) %s" OSS_NEWLINE,
                    utilDBTypeStr( (SDB_TYPE)node._type ),
                    node._svcname.c_str(), tmpPID,
                    utilDBRoleShortStr( (SDB_ROLE)node._role ) ) ;
      }

      if( detail )
      {
         _printfDetail( rooPath, node._svcname.c_str(), node._type ) ;
      }
      else if( expand )
      {
         _printfExpand( rooPath, node._svcname.c_str(), node._type ) ;
      }
   }

   // initialize options
   void init ( po::options_description &desc )
   {
      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END
   }

   void displayArg ( po::options_description &desc )
   {
      std::cout << desc << std::endl ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLIST_RESVARG, "resolveArgument" )
   INT32 resolveArgument ( po::options_description &desc,
                           INT32 argc, CHAR **argv,
                           vector<string> &listServices,
                           INT32 &typeFilter, INT32 &modeFilter,
                           INT32 &roleFilter, BOOLEAN &detail,
                           BOOLEAN &expand, BOOLEAN &showLong,
                           BOOLEAN &showLocation )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBLIST_RESVARG ) ;
      po::variables_map vm ;

      rc = utilReadCommandLine( argc, argv,  desc, vm, FALSE ) ;
      if ( rc )
      {
         std::cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_HELP ) )
      {
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto error ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "Sdb List Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_SVCNAME ) )
      {
         string svcname = vm[PMD_OPTION_SVCNAME].as<string>() ;
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
         if ( 0 == ossStrcasecmp( listType.c_str(), SDBLIST_TYPE_DB_STR ) )
         {
            typeFilter = SDB_TYPE_DB ;
         }
         else if ( 0 == ossStrcasecmp( listType.c_str(),
                   SDBLIST_TYPE_OM_STR ) )
         {
            typeFilter = SDB_TYPE_OM ;
         }
         else if ( 0 == ossStrcasecmp( listType.c_str(),
                   SDBLIST_TYPE_OMA_STR ) )
         {
            typeFilter = SDB_TYPE_OMA ;
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
      if ( vm.count( PMD_OPTION_MODE ))
      {
         string modeTemp = vm[PMD_OPTION_MODE].as<string>() ;
         if( 0 == ossStrcasecmp( modeTemp.c_str(),
                                 SDB_RUN_MODE_TYPE_LOCAL_STR ) )
         {
            modeFilter = RUN_MODE_LOCAL ;
         }
         else if( 0 == ossStrcasecmp( modeTemp.c_str(),
                                      SDB_RUN_MODE_TYPE_RUN_STR ) )
         {
            modeFilter = RUN_MODE_RUN ;
         }
         else
         {
            std::cout << "mode invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( vm.count( PMD_OPTION_ROLE ))
      {
         string roleTemp = vm[PMD_OPTION_ROLE].as<string>() ;
         roleFilter = utilGetRoleEnum( roleTemp.c_str() ) ;
         if ( SDB_ROLE_MAX == roleFilter )
         {
            std::cout << "role invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         typeFilter = -1 ;
      }

      if( vm.count(PMD_OPTION_DETAIL ) )
      {
         detail = TRUE ;
      }

      if ( vm.count(PMD_OPTION_EXPAND ) )
      {
         expand = TRUE ;
         detail = FALSE ;
      }

      if ( vm.count( PMD_OPTION_LONG ) )
      {
         showLong = TRUE ;
      }

      if ( vm.count( PMD_OPTION_LONG_LOCATION ) )
      {
         showLocation = TRUE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_SDBLIST_RESVARG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLIST_MAIN, "mainEntry" )
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBLIST_MAIN );
      INT32 total = 0 ;
      vector<string> listServices ;
      UTIL_VEC_NODES listNodes ;
      BOOLEAN bFind        = TRUE ;
      INT32 typeFilter     = SDB_TYPE_DB ;
      BOOLEAN detail       = FALSE ;
      BOOLEAN expand       = FALSE ;
      BOOLEAN showLong     = FALSE ;
      BOOLEAN showLocation = FALSE ;
      INT32 roleFilter     =  -1 ;
      INT32 modeFilter     = RUN_MODE_RUN ;
      CHAR rootPath[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      CHAR localPath[OSS_MAX_PATHSIZE + 1] = { 0 } ;

      po::options_description desc ( "Command options" ) ;
      init ( desc ) ;

      // validate arguments
      rc = resolveArgument ( desc, argc, argv, listServices, typeFilter,
                             modeFilter, roleFilter, detail, expand,
                             showLong, showLocation ) ;
      if( rc )
      {
         if( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            std::cout << "Invalid argument" << endl ;
            displayArg ( desc ) ;
         }
         goto done ;
      }

      // get program's running  path
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if( rc )
      {
        ossPrintf( "Error:Get module self path failed: %d" OSS_NEWLINE, rc ) ;
        goto error ;
      }
      rc = utilBuildFullPath( rootPath, SDBCM_LOCAL_PATH, OSS_MAX_PATHSIZE,
                              localPath ) ;
      if ( rc )
      {
         ossPrintf( "Error: Build local config path failed: %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      if ( listServices.size() > 0 )
      {
         // if used -p, so list all nodes
         typeFilter = -1 ;
         roleFilter = -1 ;
      }

      utilListNodes( listNodes, typeFilter, NULL, OSS_INVALID_PID,
                     roleFilter, FALSE, showLocation ) ;

      if ( RUN_MODE_RUN == modeFilter )
      {
         UTIL_VEC_NODES::iterator it = listNodes.begin() ;
         while( it != listNodes.end() && listServices.size() > 0 )
         {
            bFind = FALSE ;
            utilNodeInfo &info = *it ;
            for ( UINT32 j = 0 ; j < listServices.size() ; ++j )
            {
               if ( info._svcname == listServices[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = listNodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }
      }
      else
      {
         UTIL_VEC_NODES tmpNodes = listNodes ;
         listNodes.clear() ;
         utilEnumNodes( localPath, listNodes, typeFilter, NULL,
                        roleFilter ) ;
         if ( ( -1 == typeFilter || SDB_TYPE_OMA == typeFilter ) &&
              ( -1 == roleFilter || SDB_ROLE_OMA == roleFilter ) )
         {
            CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
            utilNodeInfo node ;
            node._orgname = "" ;
            node._pid = OSS_INVALID_PID ;
            node._role = SDB_ROLE_OMA ;
            node._type = SDB_TYPE_OMA ;
            ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;
            utilGetCMService( rootPath, hostName, node._svcname, TRUE ) ;
            listNodes.push_back( node ) ;
         }

         UTIL_VEC_NODES::iterator it = listNodes.begin() ;
         while ( it != listNodes.end() && listServices.size() > 0 )
         {
            utilNodeInfo &info = *it ;
            bFind = FALSE ;
            for ( UINT32 j = 0 ; j < listServices.size() ; ++j )
            {
               if ( info._svcname == listServices[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = listNodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }

         for ( UINT32 i = 0 ; i < listNodes.size() ; ++i )
         {
            for ( UINT32 k = 0 ; k < tmpNodes.size() ; ++k )
            {
               if ( listNodes[ i ]._svcname == tmpNodes[ k ]._svcname )
               {
                  listNodes[ i ] = tmpNodes[ k ] ;
                  break ;
               }
            }
         }
      }

      if ( showLocation )
      {
         // print Location title
         ossPrintf( "%s" OSS_NEWLINE, PMD_LIST_LOCATION_TITLE ) ;
      }
      else if ( showLong )
      {
         // print title
         ossPrintf( "%s" OSS_NEWLINE, PMD_LIST_TITLE ) ;
      }
      // print
      for ( UINT32 i = 0 ; i < listNodes.size() ; ++i )
      {
         ++total ;
         _printfAll( rootPath, listNodes[ i ], detail, expand, showLong, showLocation ) ;
      }

      // if no -p, and list all/list cm, need to show sdbcmd
      if ( listServices.size() == 0 &&
           ( SDB_TYPE_OMA == typeFilter || -1 == typeFilter ) &&
           ( roleFilter == -1 || SDB_ROLE_OMA == roleFilter ) )
      {
         vector < ossProcInfo > procs ;
         ossEnumProcesses( procs, PMDDMN_EXE_NAME, TRUE, FALSE ) ;

         for ( UINT32 i = 0 ; i < procs.size() ; ++i )
         {
            ++total ;
            if ( showLocation )
            {
               CHAR tmpPID[ 11 ] = { 0 } ;
               ossSnprintf( tmpPID, sizeof( tmpPID ) - 1, "%d",
                            procs[ i ]._pid ) ;
               ossPrintf( PMD_LIST_LONG_LOCATION OSS_NEWLINE,
                          PMDDMN_SVCNAME_DEFAULT,
                          "-", "-", tmpPID, "-", "-", "-", "-", "-", "-", "-", "-" ) ;
            }
            else if ( showLong )
            {
               CHAR tmpPID[ 11 ] = { 0 } ;
               ossSnprintf( tmpPID, sizeof( tmpPID ) - 1, "%d",
                            procs[ i ]._pid ) ;
               ossPrintf( PMD_LIST_LONG_FORMAT OSS_NEWLINE,
                          PMDDMN_SVCNAME_DEFAULT,
                          "-", "-", tmpPID, "-", "-", "-", "-", "-", "-" ) ;
            }
            else
            {
               ossPrintf( "%s (%d)" OSS_NEWLINE, PMDDMN_SVCNAME_DEFAULT,
                          procs[ i ]._pid ) ;
            }
         }
      }
      ossPrintf ( "Total: %d" OSS_NEWLINE, total ) ;

   done :
      PD_TRACE_EXITRC ( SDB_SDBLIST_MAIN, rc );
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         return 0 ;
      }
      return total > 0 ? 0 : ( rc ? SDB_SRC_INVALIDARG : 1 ) ;
   error :
      goto done ;
   }
}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}


