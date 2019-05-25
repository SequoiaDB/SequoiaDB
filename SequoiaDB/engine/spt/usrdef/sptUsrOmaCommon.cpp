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

   Source File Name = sptUsrOMACommon.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/18/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrOmaCommon.hpp"
#include "utilParam.hpp"
#include "omagentDef.hpp"
#include "ossUtil.h"
#include "cmdUsrOmaUtil.hpp"
#include "ossIO.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "utilStr.hpp"

using namespace bson ;
namespace engine
{
   INT32 _sptUsrOmaCommon::getOmaInstallInfo( BSONObj& retObj, string &err )
   {
      utilInstallInfo info ;
      INT32 rc = utilGetInstallInfo( info ) ;
      if ( SDB_OK != rc )
      {
         err = "Install file is not exist" ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         builder.append( SDB_INSTALL_RUN_FILED, info._run ) ;
         builder.append( SDB_INSTALL_USER_FIELD, info._user ) ;
         builder.append( SDB_INSTALL_PATH_FIELD, info._path ) ;
         builder.append( SDB_INSTALL_MD5_FIELD, info._md5 ) ;
         retObj = builder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::getOmaInstallFile( string &retStr, string &err )
   {
      retStr = SDB_INSTALL_FILE_NAME ;
      return SDB_OK ;
   }

   INT32 _sptUsrOmaCommon::getOmaConfigFile( string &retStr, string &err )
   {
      INT32 rc = SDB_OK ;

      rc = _getConfFile( retStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to get config file" ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::getOmaConfigs( const bson::BSONObj &arg,
                                         bson::BSONObj &retObj,
                                         string &err )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;

      if ( arg.hasField( "confFile" ) )
      {
         if( String != arg.getField( "confFile" ).type() )
         {
            err = "confFile must be string" ;
            goto error ;
         }
         confFile = arg.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to get config file" ;
            goto error ;
         }
      }

      rc = _getConfInfo( confFile, conf, err ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      retObj = conf ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::setOmaConfigs( const BSONObj &arg,
                                         const BSONObj &confObj,
                                         string &err )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      string str ;

      if ( arg.hasField( "confFile" ) )
      {
         if( String != arg.getField( "confFile" ).type() )
         {
            err = "confFile must be string" ;
            goto error ;
         }
         confFile = arg.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to get config file" ;
            goto error ;
         }
      }

      rc = _confObj2Str( confObj, str, err ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         err = ss.str() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::getAOmaSvcName( const bson::BSONObj &arg,
                                          string &retStr,
                                          string &err )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObj confObj ;

      if( !arg.hasField( "hostname" ) )
      {
         err = "hostname must be config" ;
         goto error ;
      }
      else if( String != arg.getField( "hostname" ).type() )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      hostname = arg.getStringField( "hostname" ) ;

      if( arg.hasField( "confFile" ) )
      {
         if( String != arg.getField( "confFile" ).type() )
         {
            err = "confFile must be string" ;
            goto error ;
         }
         confFile = arg.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to get config file" ;
            goto error ;
         }
      }

      rc = _getConfInfo( confFile, confObj, err ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         if ( e.eoo() )
         {
            e = confObj.getField( SDBCM_CONF_DFTPORT ) ;
         }

         if ( e.type() == String )
         {
            retStr = e.valuestr() ;
         }
         else
         {
            stringstream ss ;
            ss << e.toString() << " is invalid" ;
            err = ss.str() ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::addAOmaSvcName( const BSONObj &valueObj,
                                          const BSONObj &optionObj,
                                          const BSONObj &matchObj,
                                          string &err )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      INT32 isReplace = TRUE ;
      string confFile ;
      BSONObj confObj ;
      string str ;

      if ( FALSE == valueObj.hasField( "hostname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         err = "hostname must be config" ;
         goto error ;
      }
      else if ( String != valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname must be string" ;
         goto error ;
      }
      hostname = valueObj.getStringField( "hostname" ) ;
      if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname can't be empty" ;
         goto error ;
      }

      if ( FALSE == valueObj.hasField( "svcname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         err = "svcname must be config" ;
         goto error ;
      }
      else if ( String != valueObj.getField( "svcname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "svcname must be string" ;
         goto error ;
      }
      svcname = valueObj.getStringField( "svcname" ) ;
      if ( svcname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "svcname can't be empty" ;
         goto error ;
      }

      if ( optionObj.hasField( "isReplace" ) )
      {
         if ( Bool != optionObj.getField( "isReplace" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "isReplace must be BOOLEAN" ;
            goto error ;
         }
         isReplace = optionObj.getBoolField( "isReplace" ) ;
      }

      if ( matchObj.hasField( "confFile" ) )
      {
         if ( String != matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "confFile must be string" ;
            goto error ;
         }
         confFile = matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to get config file" ;
            goto error ;
         }
      }

      rc = _getConfInfo( confFile, confObj, err, TRUE ) ;
      if ( rc )
      {
         err = "Failed to get conf info" ;
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         BSONElement e1 = confObj.getField( SDBCM_CONF_DFTPORT ) ;

         if ( e.type() == String )
         {
            if ( 0 == ossStrcmp( e.valuestr(), svcname.c_str() ) )
            {
               goto done ;
            }
            else if ( !isReplace )
            {
               stringstream ss ;
               ss << hostname << " already exist" ;
               err = ss.str() ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( e1.type() == String &&
                   0 == ossStrcmp( e1.valuestr(), svcname.c_str() ) )
         {
            goto done ;
         }
      }

      rc = _confObj2Str( confObj, str, err, hostname.c_str() ) ;
      if ( rc )
      {
         goto error ;
      }
      str += hostname ;
      str += "=" ;
      str += svcname ;
      str += OSS_NEWLINE ;

      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         err = ss.str() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::delAOmaSvcName( const bson::BSONObj &arg,
                                          string &err )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObj confObj ;
      string str ;

      if( !arg.hasField( "hostname" ) )
      {
         err = "hostname must be config" ;
         goto error ;
      }
      else if( String != arg.getField( "hostname" ).type() )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      hostname = arg.getStringField( "hostname" ) ;

      if( arg.hasField( "confFile" ) )
      {
         if( String != arg.getField( "confFile" ).type() )
         {
            err = "confFile must be string" ;
            goto error ;
         }
         confFile = arg.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to get config file" ;
            goto error ;
         }
      }

      rc = _getConfInfo( confFile, confObj, err, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         if ( e.eoo() )
         {
            goto done ;
         }
      }

      rc = _confObj2Str( confObj, str, err, hostname.c_str() ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         err = ss.str() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::_getConfFile( string &confFile )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;
      CHAR confPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( SDB_OK == utilGetInstallInfo( info ) )
      {
         if ( SDB_OK == utilBuildFullPath( info._path.c_str(),
                                           SPT_OMA_REL_PATH_FILE,
                                           OSS_MAX_PATHSIZE,
                                           confPath ) &&
              SDB_OK == ossAccess( confPath ) )
         {
            goto done ;
         }
      }

      rc = ossGetEWD( confPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get EWD, rc:%d", rc ) ;
         goto error ;
      }
      rc = utilCatPath( confPath, OSS_MAX_PATHSIZE, SDBCM_CONF_PATH_FILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build full path, rc:%d", rc ) ;
         goto error ;
      }
      rc = ossAccess( confPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access config file: %s, rc:%d",
                 confPath, rc ) ;
         goto error ;
      }
   done:
      if ( SDB_OK == rc )
      {
         confFile = confPath ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::_getConfInfo( const string & confFile, BSONObj &conf,
                                         string &err, BOOLEAN allowNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po ::variables_map vm ;

      MAP_CONFIG_DESC( desc ) ;

      rc = ossAccess( confFile.c_str() ) ;
      if ( rc )
      {
         if ( allowNotExist )
         {
            rc = SDB_OK ;
            goto done ;
         }
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         err =  ss.str() ;
         goto error ;
      }

      rc = utilReadConfigureFile( confFile.c_str(), desc, vm ) ;
      if ( SDB_FNE == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         err = ss.str() ;
         goto error ;
      }
      else if ( SDB_PERM == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] permission error" ;
         err = ss.str() ;
         goto error ;
      }
      else if ( rc )
      {
         stringstream ss ;
         ss << "read conf file[" << confFile << "] error" ;
         err = ss.str() ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         po ::variables_map::iterator it = vm.begin() ;
         while ( it != vm.end() )
         {
            if ( SDBCM_RESTART_COUNT == it->first ||
                 SDBCM_RESTART_INTERVAL == it->first ||
                 SDBCM_DIALOG_LEVEL == it->first )
            {
               builder.append( it->first, it->second.as<INT32>() ) ;
            }
            else if ( SDBCM_AUTO_START == it->first )
            {
               BOOLEAN autoStart = TRUE ;
               ossStrToBoolean( it->second.as<string>().c_str(), &autoStart ) ;
               builder.appendBool( it->first, autoStart ) ;
            }
            else
            {
               builder.append( it->first, it->second.as<string>() ) ;
            }
            ++it ;
         }
         conf = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::_confObj2Str( const BSONObj &conf, string &str,
                                         string &err,
                                         const CHAR* pExcept )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObjIterator it ( conf ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;

         if ( pExcept && 0 != pExcept[0] &&
              0 == ossStrcmp( pExcept, e.fieldName() ) )
         {
            continue ;
         }

         ss << e.fieldName() << "=" ;
         if ( e.type() == String )
         {
            ss << e.valuestr() ;
         }
         else if ( e.type() == NumberInt )
         {
            ss << e.numberInt() ;
         }
         else if ( e.type() == NumberLong )
         {
            ss << e.numberLong() ;
         }
         else if ( e.type() == NumberDouble )
         {
            ss << e.numberDouble() ;
         }
         else if ( e.type() == Bool )
         {
            ss << ( e.boolean() ? "TRUE" : "FALSE" ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            stringstream errss ;
            errss << e.toString() << " is invalid config" ;
            err = errss.str() ;
            goto error ;
         }
         ss << endl ;
      }
      str = ss.str() ;

   done:
      return rc ;
   error:
      goto done ;
   }
}
