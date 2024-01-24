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

   Source File Name = utilParam.cpp

   Descriptive Name =

   When/how to use: str util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/04/2014  XJH Initial Draft

   Last Changed =

******************************************************************************/


#include "utilParam.hpp"
#include "ossIO.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "pmdDef.hpp"
#include "oss.h"
#include "pmdOptions.hpp"
#include "msgDef.h"
#include "omagentDef.hpp"
#include "utilCommon.hpp"
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace engine
{

   /*
      limits.conf
   */
   #define UTIL_OPTION_LIMIT_CORE       "core_file_size"
   #define UTIL_OPTION_LIMIT_DATA       "data_seg_size"
   #define UTIL_OPTION_LIMIT_FILESIZE   "file_size"
   #define UTIL_OPTION_LIMIT_VM         "virtual_memory"
   #define UTIL_OPTION_LIMIT_FD         "open_files"
   #define UTIL_OPTION_LIMIT_STACKSIZE  "stack_size"

   #define UTIL_OPTION_LIMIT_BOUNDARY_VAL       -1

   /*
      The default value of limits.conf
   */
   #define UTIL_OPTION_LIMIT_CORE_DEFAULT       0
   #define UTIL_OPTION_LIMIT_DATA_DEFAULT       -1
   #define UTIL_OPTION_LIMIT_FILESIZE_DEFAULT   -1
   #define UTIL_OPTION_LIMIT_VM_DEFAULT         -1
   #define UTIL_OPTION_LIMIT_FD_DEFAULT         60000
   #define UTIL_OPTION_LIMIT_STACKSIZE_DEFAULT  524288

   INT32 utilReadConfigureFile( const CHAR *file,
                                po::options_description &desc,
                                po::variables_map &vm )
   {
      INT32 rc = SDB_OK;

      try
      {
         po::store( po::parse_config_file<char> ( file, desc, TRUE ), vm ) ;
         po::notify ( vm ) ;
      }
      catch( po::reading_file )
      {
         std::cerr << "Failed to open config file: "
                   <<( std::string ) file << std::endl ;
         rc = ossAccess( file, OSS_MODE_READ ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_TOO_MANY_OPEN_FD ;
         }
         goto error ;
      }
      catch ( po::unknown_option &e )
      {
         std::cerr << "Unknown config element: "
                   << e.get_option_name () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      catch ( po::invalid_option_value &e )
      {
         std::cerr << ( std::string ) "Invalid config element: "
                   << e.get_option_name () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      catch( po::error &e )
      {
         std::cerr << e.what () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 utilReadCommandLine3( INT32 argc, CHAR **argv,
                               po::options_description &desc,
                               po::positional_options_description &posDesc,
                               po::variables_map &vm,
                               BOOLEAN allowUnreg )
   {
      INT32 rc = SDB_OK;

      try
      {
         po::parsed_options parsed = po::command_line_parser(
               argc, argv).options( desc ).positional(posDesc).allow_unregistered().run() ;
         po::store ( parsed, vm ) ;
         if ( !allowUnreg )
         {
            vector<string> to_pass_further = po::collect_unrecognized(
               parsed.options, po::include_positional ) ;
            if ( to_pass_further.size() )
            {
               std::cerr << "Unrecongnized options: ";
               for ( vector<string>::iterator i = to_pass_further.begin() ;
                     i != to_pass_further.end() ; ++i )
               {
                  std::cerr << *i << ' ' ;
               }
               std::cerr << std::endl ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         po::notify ( vm ) ;
      }
      catch( po::error &e )
      {
         std::cerr << e.what () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilReadCommandLine2( INT32 argc, CHAR **argv,
                               po::options_description &desc,
                               po::variables_map &vm,
                               BOOLEAN allowUnreg )
   {
      INT32 rc = SDB_OK;

      try
      {
         po::parsed_options parsed = po::command_line_parser(
               argc, argv).options( desc ).allow_unregistered().run() ;
         po::store ( parsed, vm ) ;
         if ( !allowUnreg )
         {
            vector<string> to_pass_further = po::collect_unrecognized(
               parsed.options, po::include_positional ) ;
            if ( to_pass_further.size() )
            {
               std::cerr << "Unrecongnized options: ";
               for ( vector<string>::iterator i = to_pass_further.begin() ;
                     i != to_pass_further.end() ; ++i )
               {
                  std::cerr << *i << ' ' ;
               }
               std::cerr << std::endl ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         po::notify ( vm ) ;
      }
      catch( po::error &e )
      {
         std::cerr << e.what () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilReadCommandLine( INT32 argc, CHAR **argv,
                              po::options_description &desc,
                              po::variables_map &vm,
                              BOOLEAN allowUnreg )
   {
      INT32 rc = SDB_OK;

      try
      {
         if ( allowUnreg )
         {
            po::store ( po::command_line_parser( argc, argv).options(
                        desc ).allow_unregistered().run(), vm ) ;
         }
         else
         {
            po::store( po::parse_command_line( argc, argv, desc ), vm ) ;
         }
         po::notify ( vm ) ;
      }
      catch ( po::unknown_option &e )
      {
         std::cerr <<  "Unknown argument: "
                   << e.get_option_name () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      catch ( po::invalid_option_value &e )
      {
         std::cerr << "Invalid argument: "
                   << e.get_option_name () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      catch( po::error &e )
      {
         std::cerr << e.what () << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilWriteConfigFile( const CHAR * pFile, const CHAR * pData,
                              BOOLEAN createOnly )
   {
      INT32 rc = SDB_OK ;
      std::string tmpFile = pFile ;
      tmpFile += ".tmp" ;
      OSSFILE file ;
      BOOLEAN isOpen = FALSE ;
      BOOLEAN isBak = FALSE ;

      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

      // 1. first back up the file
      if ( SDB_OK == ossAccess( pFile ) )
      {
         if ( createOnly )
         {
            rc = SDB_FE ;
            goto error ;
         }
         if ( SDB_OK == ossRenamePath( pFile, tmpFile.c_str() ) )
         {
            isBak = TRUE ;
         }
      }

      // 2. Create the file
      rc = ossOpen ( pFile, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_DEFAULTFILE | OSS_RO , file ) ;
      if ( rc )
      {
         goto error ;
      }
      isOpen = TRUE ;

      // 3. write data
      {
         SINT64 written = 0 ;
         SINT64 len = ossStrlen( pData ) ;
         while ( 0 < len )
         {
            SINT64 tmpWritten = 0 ;
            rc = ossWrite( &file, pData + written , len, &tmpWritten ) ;
            if ( rc && SDB_INTERRUPT != rc )
            {
               PD_LOG( PDERROR, "Failed to write file[%s]:%d", pFile, rc ) ;
               goto error ;
            }
            written += tmpWritten ;
            len -= tmpWritten ;
            rc = SDB_OK ;
         }
      }

      // 4. remove tmp
      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      if ( isBak )
      {
         if ( isOpen )
         {
            ossClose( file ) ;
            isOpen = FALSE ;
            ossDelete( pFile ) ;
         }
         ossRenamePath( tmpFile.c_str(), pFile ) ;
      }
      goto done ;
   }

   INT32 utilGetServiceByConfigPath( const string &confPath,
                                     const string &defaultName,
                                     string &svcname,
                                     BOOLEAN allowFileNotExist,
                                     BOOLEAN *isConfFileValid,
                                     ossPoolString *errMsg )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;
      desc.add_options()
         ( PMD_OPTION_SVCNAME, po::value<string>(), "" ) ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;

      if ( NULL != isConfFileValid )
      {
         *isConfFileValid = TRUE ;
      }

      if ( defaultName.empty() )
      {
         svcname = boost::lexical_cast<string>(OSS_DFT_SVCPORT) ;
      }
      else
      {
         svcname = defaultName ;
      }

      rc = utilBuildFullPath ( confPath.c_str(), PMD_DFT_CONF,
                               OSS_MAX_PATHSIZE, conf ) ;
      if ( rc )
      {
         if ( NULL != errMsg )
         {
            *errMsg = "Failed to build full path" ;
         }
         goto error ;
      }

      if ( SDB_OK != ossAccess( conf, OSS_MODE_ACCESS | OSS_MODE_READ ) )
      {
         if ( NULL != isConfFileValid )
         {
            *isConfFileValid = FALSE ;
         }

         if ( allowFileNotExist )
         {
            goto done ;
         }
      }

      rc = utilReadConfigureFile( conf, desc, vm ) ;
      if ( allowFileNotExist && SDB_IO == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_SVCNAME ) )
      {
         svcname = vm [ PMD_OPTION_SVCNAME ].as<string>() ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 utilGetRoleByConfigPath( const string &confPath, INT32 &role,
                                  BOOLEAN allowFileNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;
      desc.add_options()
         ( PMD_OPTION_ROLE, po::value<string>(), "" ) ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      role = SDB_ROLE_STANDALONE ;

      rc = utilBuildFullPath ( confPath.c_str(), PMD_DFT_CONF,
                               OSS_MAX_PATHSIZE, conf ) ;
      if ( rc )
      {
         std::cerr << "Failed to build full path, rc: " << rc << std::endl ;
         goto error ;
      }

      if ( allowFileNotExist && SDB_OK != ossAccess( conf ) )
      {
         goto done ;
      }

      rc = utilReadConfigureFile( conf, desc, vm ) ;
      if ( allowFileNotExist && SDB_IO == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_ROLE ) )
      {
         string roleStr = vm [ PMD_OPTION_ROLE ].as<string>() ;
         role = utilGetRoleEnum( roleStr.c_str() ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 utilGetDBPathByConfigPath( const string & confPath,
                                    string &dbPath,
                                    BOOLEAN allowFileNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;
      desc.add_options()
         ( PMD_OPTION_DBPATH, po::value<string>(), "" ) ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      dbPath = PMD_CURRENT_PATH ;

      rc = utilBuildFullPath ( confPath.c_str(), PMD_DFT_CONF,
                               OSS_MAX_PATHSIZE, conf ) ;
      if ( rc )
      {
         std::cerr << "Failed to build full path, rc: " << rc << std::endl ;
         goto error ;
      }

      if ( allowFileNotExist && SDB_OK != ossAccess( conf ) )
      {
         goto done ;
      }

      rc = utilReadConfigureFile( conf, desc, vm ) ;
      if ( allowFileNotExist && SDB_IO == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_DBPATH ) )
      {
         dbPath = vm [ PMD_OPTION_DBPATH ].as<string>() ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 utilGetCMService( const string & rootPath,
                           const string & hostname,
                           string & svcname,
                           BOOLEAN allowFileNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po ::variables_map vm ;
      CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      desc.add_options()
         ( "*", po::value<string>(), "" )
         ;
      string hostnameKey = hostname + string( SDBCM_CONF_PORT ) ;
      svcname = boost::lexical_cast<string>( SDBCM_DFT_PORT ) ;

      rc = utilBuildFullPath( rootPath.c_str(), SDBCM_CONF_PATH_FILE,
                              OSS_MAX_PATHSIZE, confFile ) ;
      if ( rc )
      {
         std::cerr << "Failed to build full path, rc: " << rc << std::endl ;
         goto error ;
      }

      if ( allowFileNotExist && SDB_OK != ossAccess( confFile ) )
      {
         goto done ;
      }

      rc = utilReadConfigureFile( confFile, desc, vm ) ;
      if ( allowFileNotExist && SDB_IO == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count ( hostnameKey ) )
      {
         svcname = vm [ hostnameKey ].as<string>() ;
      }
      else if ( vm.count( SDBCM_CONF_DFTPORT ) )
      {
         svcname = vm [ SDBCM_CONF_DFTPORT ].as<string>() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilGetInstallInfo( utilInstallInfo & info )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         ( SDB_INSTALL_RUN_FILED, po::value<string>(), "after to run cmd" ) \
         ( SDB_INSTALL_USER_FIELD, po::value<string>(), "user" ) \
         ( SDB_INSTALL_PATH_FIELD, po::value<string>(), "install path" ) \
         ( SDB_INSTALL_MD5_FIELD, po::value<string>(), "md5" )
      PMD_ADD_PARAM_OPTIONS_END

      rc = ossAccess( SDB_INSTALL_FILE_NAME ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Access file[%s] failed, rc: %d",
                 SDB_INSTALL_FILE_NAME, rc ) ;
         goto error ;
      }

      rc = utilReadConfigureFile( SDB_INSTALL_FILE_NAME, desc, vm ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read install info from file, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( vm.count( SDB_INSTALL_RUN_FILED ) )
      {
         info._run = vm[ SDB_INSTALL_RUN_FILED ].as<string>() ;
      }
      if ( vm.count( SDB_INSTALL_USER_FIELD ) )
      {
         info._user = vm[ SDB_INSTALL_USER_FIELD ].as<string>() ;
      }
      if ( vm.count( SDB_INSTALL_PATH_FIELD ) )
      {
         info._path = vm[ SDB_INSTALL_PATH_FIELD ].as<string>() ;
      }
      if ( vm.count( SDB_INSTALL_MD5_FIELD ) )
      {
         info._md5 = vm[ SDB_INSTALL_MD5_FIELD ].as<string>() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _compareAndSetUlimit( const CHAR *limitStr, INT64 expVal,
                               ossProcLimits &pProcLim )
   {
      INT32 rc       = SDB_OK ;
      INT64 curSoft  = 0 ;
      INT64 curHard  = 0 ;
      BOOLEAN hasGot = FALSE ;

      hasGot = pProcLim.getLimit( limitStr, curSoft, curHard ) ;
      if ( !hasGot || curSoft != expVal )
      {
         if ( -1 == expVal || ( -1 != curHard && curHard < expVal ) )
         {
            curHard = expVal ;
         }
         rc = pProcLim.setLimit( limitStr, expVal, curHard ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to set ulimit[%s] to [%lld]",
                    limitStr, expVal ) ;
         }
      }
      return rc ;
   }


   INT32 utilSetAndCheckUlimit()
   {
      INT32 rc = SDB_OK ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR confFileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR *relativePath = NULL ;
      po::options_description limitDesc ;
      po::variables_map limitVarmap ;
      ossProcLimits procLim ;
      vector< pair< pair<string, INT64>, string > > vec ;
      vector< pair< pair<string, INT64>, string > >::iterator it ;

      /// get full path of limits.conf
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Error: Failed to get module self path: %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }
      relativePath = ".." OSS_FILE_SEP "conf" OSS_FILE_SEP "limits.conf" ;
      rc = utilBuildFullPath( rootPath, relativePath, OSS_MAX_PATHSIZE,
                              confFileName ) ;
      if ( rc )
      {
         ossPrintf( "Error: Failed to build limits.conf path name: %d"
                    OSS_NEWLINE, rc ) ;
         goto error ;
      }

      /// load limits.conf
      limitDesc.add_options()
      ( UTIL_OPTION_LIMIT_CORE,      po::value<INT64>(), "" )
      ( UTIL_OPTION_LIMIT_DATA,      po::value<INT64>(), "" )
      ( UTIL_OPTION_LIMIT_FILESIZE,  po::value<INT64>(), "" )
      ( UTIL_OPTION_LIMIT_VM,        po::value<INT64>(), "" )
      ( UTIL_OPTION_LIMIT_FD,        po::value<INT64>(), "" )
      ( UTIL_OPTION_LIMIT_STACKSIZE, po::value<INT64>(), "" ) ;
      rc = utilReadConfigureFile( confFileName, limitDesc, limitVarmap ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            PD_LOG( PDWARNING, "Config[%s] not exist, use default config",
                    confFileName ) ;
            rc = SDB_OK ;
            goto done ;
         }
         ossPrintf( "Error: Failed to read config from file[%s]: %d" OSS_NEWLINE,
                    confFileName, rc ) ;
         goto error ;
      }

      /// set ulimit and check
      // e.g < < "open_files", 60000 >, "open files" >
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_CORE,
                                               UTIL_OPTION_LIMIT_CORE_DEFAULT ),
                     OSS_LIMIT_CORE_SZ ) ) ;
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_DATA,
                                               UTIL_OPTION_LIMIT_DATA_DEFAULT ),
                     OSS_LIMIT_DATA_SEG_SZ ) ) ;
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_FILESIZE,
                                               UTIL_OPTION_LIMIT_FILESIZE_DEFAULT ),
                     OSS_LIMIT_FILE_SZ ) ) ;
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_VM,
                                               UTIL_OPTION_LIMIT_VM_DEFAULT ),
                     OSS_LIMIT_VIRTUAL_MEM ) ) ;
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_FD,
                                               UTIL_OPTION_LIMIT_FD_DEFAULT ),
                     OSS_LIMIT_OPEN_FILE ) ) ;
      vec.push_back( make_pair<pair<string, INT64>, string>(
                     make_pair<string, INT64>( UTIL_OPTION_LIMIT_STACKSIZE,
                                              UTIL_OPTION_LIMIT_STACKSIZE_DEFAULT ),
                     OSS_LIMIT_STACK_SIZE ) ) ;
      for( it = vec.begin() ; it != vec.end() ; it++ )
      {
         string option   = it->first.first ;
         INT64 defVal    = it->first.second ;
         string limitStr = it->second ;
         INT64 expVal    = 0 ;
         INT64 curSoft   = 0 ;
         INT64 curHard   = 0 ;
         BOOLEAN hasGot  = FALSE ;
         INT32 tmpRC     = 0 ;


         // set ulimit
         po::variables_map::const_iterator iter = limitVarmap.find( option ) ;
         if ( iter == limitVarmap.end() ||
              UTIL_OPTION_LIMIT_BOUNDARY_VAL > iter->second.as<INT64>() )
         {
            expVal = defVal ;
         }
         else
         {
            expVal = iter->second.as<INT64>() ;
         }
         tmpRC = _compareAndSetUlimit( limitStr.c_str(), expVal, procLim ) ;
         if ( tmpRC && expVal != defVal )
         {
            PD_LOG( PDINFO, "Intend to reset ulimit[%s] by default value [%lld]",
                    limitStr.c_str(), defVal ) ;
            expVal = defVal ;
            _compareAndSetUlimit( limitStr.c_str(), expVal, procLim ) ;
         }

         // check ulimit
         hasGot = procLim.getLimit( limitStr.c_str(), curSoft, curHard ) ;
         if ( !hasGot )
         {
            rc = SDB_SYS ;
            ossPrintf( "Error: Failed to get ulimit[%s]" OSS_NEWLINE,
                       limitStr.c_str() ) ;
            goto error ;
         }
         if ( curSoft != -1 && ( curSoft < expVal || expVal == -1 ) )
         {
            rc = SDB_SYS ;
            ossPrintf( "Error: Failed to set ulimit[%s] to [%lld]"
                       OSS_NEWLINE, limitStr.c_str(), expVal ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 utilCheckAndChangeUserInfo( const CHAR * curFileName )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;
      OSSUID fileUID = OSS_INVALID_UID ;
      OSSGID fileGID = OSS_INVALID_GID ;
      OSSUID curUID  = OSS_INVALID_UID ;
      OSSGID curGID  = OSS_INVALID_GID ;

      // first compare file:cur uid/gid
      ossGetFileUserInfo( curFileName, fileUID, fileGID ) ;
      curUID = ossGetCurrentProcessUID() ;
      curGID = ossGetCurrentProcessGID() ;

      if ( OSS_INVALID_UID == fileUID || 0 == fileUID ||
           OSS_INVALID_GID == fileGID || 0 == fileGID )
      {
         // get install user info
         rc = utilGetInstallInfo( info ) ;
         if ( rc )
         {
            // no install info, not change
            rc = SDB_OK ;
            goto done ;
         }
         // get install user uid and gid
         rc = ossGetUserInfo( info._user.c_str(), fileUID, fileGID ) ;
         if ( rc )
         {
            // no install user, not change
            rc = SDB_OK ;
            goto done ;
         }
      }
      else
      {
         CHAR usrName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossGetUserInfo( fileUID, usrName, OSS_MAX_PATHSIZE ) ;
         info._user = usrName ;
      }

      if ( curGID != fileGID )
      {
         rc = ossSetCurrentProcessGID( fileGID ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      if ( curUID != fileUID )
      {
         rc = ossSetCurrentProcessUID( fileUID ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      std::cout << "Please run it by user: " << info._user << std::endl ;
      goto done ;
   }

}


