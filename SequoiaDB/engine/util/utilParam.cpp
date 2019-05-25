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

      rc = ossOpen ( pFile, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RWXU, file ) ;
      if ( rc )
      {
         goto error ;
      }
      isOpen = TRUE ;

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

   INT32 utilGetServiceByConfigPath( const string & confPath,
                                     string & svcname,
                                     const string &defaultName,
                                     BOOLEAN allowFileNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;
      desc.add_options()
         ( PMD_OPTION_SVCNAME, po::value<string>(), "" ) ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
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

   INT32 utilSetAndCheckUlimit()
   {
      INT32 rc = SDB_OK ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR confFileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR *relativePath = NULL ;
      po::options_description limitDesc ;
      po::variables_map limitVarmap ;
      ossProcLimits procLim ;
      vector<pair<string, string> > vec ;
      vector<pair<string, string> >::iterator it ;

      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Error: Failed to get module self path: %d"OSS_NEWLINE,
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

      limitDesc.add_options()
      ( PMD_OPTION_LIMIT_CORE,      po::value<INT64>(), "" )
      ( PMD_OPTION_LIMIT_DATA,      po::value<INT64>(), "" )
      ( PMD_OPTION_LIMIT_FILESIZE,  po::value<INT64>(), "" )
      ( PMD_OPTION_LIMIT_VM,        po::value<INT64>(), "" )
      ( PMD_OPTION_LIMIT_FD,        po::value<INT64>(), "" ) ;
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
         ossPrintf( "Error: Failed to read config from file[%s]: %d"OSS_NEWLINE,
                    confFileName, rc ) ;
         goto error ;
      }

      vec.push_back( make_pair<string,string>( PMD_OPTION_LIMIT_CORE,
                                               OSS_LIMIT_CORE_SZ ) ) ;
      vec.push_back( make_pair<string,string>( PMD_OPTION_LIMIT_DATA,
                                               OSS_LIMIT_DATA_SEG_SZ ) ) ;
      vec.push_back( make_pair<string,string>( PMD_OPTION_LIMIT_FILESIZE,
                                               OSS_LIMIT_FILE_SZ ) ) ;
      vec.push_back( make_pair<string,string>( PMD_OPTION_LIMIT_VM,
                                               OSS_LIMIT_VIRTUAL_MEM ) ) ;
      vec.push_back( make_pair<string,string>( PMD_OPTION_LIMIT_FD,
                                               OSS_LIMIT_OPEN_FILE ) ) ;
      for( it = vec.begin() ; it != vec.end() ; it++ )
      {
         string option = it->first ;
         string limStr = it->second ;

         if ( !limitVarmap.count( option ) )
         {
            continue ;
         }
         INT64 limVal = limitVarmap[ option ].as<INT64>() ;

         INT64 curSoft = 0, curHard = 0 ;
         BOOLEAN hasGot = FALSE ;

         hasGot = procLim.getLimit( limStr.c_str(), curSoft, curHard ) ;
         if ( !hasGot ||  curSoft != limVal )
         {
            procLim.setLimit( limStr.c_str(), limVal, limVal ) ;
         }

         hasGot = procLim.getLimit( limStr.c_str(), curSoft, curHard ) ;
         if ( !hasGot )
         {
            rc = SDB_SYS ;
            ossPrintf( "Error: Failed to get ulimit[%s]"OSS_NEWLINE,
                       limStr.c_str() ) ;
            goto error ;
         }
         if ( curSoft < limVal && curSoft != -1 )
         {
            rc = SDB_SYS ;
            ossPrintf( "Error: Failed to set ulimit[%s] to [%lld]"
                       OSS_NEWLINE, limStr.c_str(), limVal ) ;
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

      ossGetFileUserInfo( curFileName, fileUID, fileGID ) ;
      curUID = ossGetCurrentProcessUID() ;
      curGID = ossGetCurrentProcessGID() ;

      if ( OSS_INVALID_UID == fileUID || 0 == fileUID ||
           OSS_INVALID_GID == fileGID || 0 == fileGID )
      {
         rc = utilGetInstallInfo( info ) ;
         if ( rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         rc = ossGetUserInfo( info._user.c_str(), fileUID, fileGID ) ;
         if ( rc )
         {
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


