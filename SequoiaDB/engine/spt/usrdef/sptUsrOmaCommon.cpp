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
   #define CHAR_DOUBLE_QUOTE  '"'
   #define CHAR_SINGLE_QUOTE  '\''
   #define SPT_DOUBLE_TEMP_SIZE  512

   enum itemType
   {
      ITEM_TYPE_STRING = 0,
      ITEM_TYPE_INT,
      ITEM_TYPE_LONG,
      ITEM_TYPE_DOUBLE,
      ITEM_TYPE_BOOLEAN
   } ;

   typedef struct _itemInfo  : public SDBObject
   {
      itemType    type ;
      INT32       stringLength ;
      INT32       varInt ;
      BOOLEAN     varBool ;
      INT64       varLong ;
      FLOAT64     varDouble ;
      const CHAR *pVarString ;
   } itemInfo ;

   static INT32 _parseNumber( const CHAR *pBuffer, INT32 size,
                              itemType &csvType,
                              INT32 *pVarInt,
                              INT64 *pVarLong,
                              FLOAT64 *pVarDouble )
   {
      INT32 rc = SDB_OK ;
      itemType type = ITEM_TYPE_INT ;
      FLOAT64 n = 0 ;
      FLOAT64 sign = 1 ;
      FLOAT64 scale = 0 ;
      FLOAT64 subscale = 0 ;
      FLOAT64 signsubscale = 1 ;
      INT32 n1 = 0 ;
      INT64 n2 = 0 ;

      if ( 0 == size )
      {
         type = ITEM_TYPE_STRING ;
         goto done ;
      }

      if ( *pBuffer != '+' && *pBuffer != '-' &&
           ( *pBuffer < '0' || *pBuffer >'9' ) )
      {
         type = ITEM_TYPE_STRING ;
         goto done ;
      }

      /* Could use sscanf for this? */
      /* Has sign? */
      if ( '-' == *pBuffer )
      {
         sign = -1 ;
         --size ;
         ++pBuffer ;
      }
      else if ( '+' == *pBuffer )
      {
         sign = 1 ;
         --size ;
         ++pBuffer ;
      }

      while ( size > 0 && '0' == *pBuffer )
      {
         /* is zero */
         ++pBuffer ;
         --size ;
      }

      if ( size > 0 && *pBuffer >= '1' && *pBuffer <= '9' )
      {
         do
         {
            n  = ( n  * 10.0 ) + ( *pBuffer - '0' ) ;   
            n1 = ( n1 * 10 )   + ( *pBuffer - '0' ) ;
            n2 = ( n2 * 10 )   + ( *pBuffer - '0' ) ;
            --size ;
            ++pBuffer ;
            if ( (INT64)n1 != n2 )
            {
               type = ITEM_TYPE_LONG ;
            }
         }
         while ( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' ) ;
      }

      if ( size > 0 && *pBuffer == '.' &&
           pBuffer[1] >= '0' && pBuffer[1] <= '9' )
      {
         type = ITEM_TYPE_DOUBLE ;
         --size ;
         ++pBuffer ;
         while ( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' )
         {
            n = ( n ) + ( *pBuffer - '0' ) / pow( 10.0, ++scale ) ;
            --size ;
            ++pBuffer ;
         }
      }
      else if( size == 1 && *pBuffer == '.' )
      {
         ++pBuffer ;
         --size ;
      }

      if ( size > 0 && ( *pBuffer == 'e' || *pBuffer == 'E' ) )
      {
         --size ;
         ++pBuffer ;
         if ( size > 0 && '+' == *pBuffer )
         {
            --size ;
            ++pBuffer ;
            signsubscale = 1 ;
         }
         else if ( size > 0 && '-' == *pBuffer )
         {
            type = ITEM_TYPE_DOUBLE ;
            --size ;
            ++pBuffer ;
            signsubscale = -1 ;
         }
         while ( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' )
         {
            subscale = ( subscale * 10 ) + ( *pBuffer - '0' ) ;
            --size ;
            ++pBuffer ;
         }
      }

      if ( size == 0 )
      {
         if ( ITEM_TYPE_DOUBLE == type )
         {
            n = sign * n * pow ( 10.0, ( subscale * signsubscale * 1.0 ) ) ;
         }
         else if ( ITEM_TYPE_LONG == type )
         {
            if ( 0 != subscale )
            {
               n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
            }
            else
            {
               n2 = ( ( (INT64) sign ) * n2 ) ;
            }
         }
         else if ( ITEM_TYPE_INT == type )
         {
             n1 = (INT32)( sign * n1 * pow( 10.0, subscale * 1.00 ) ) ;
             n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
             if ( (INT64)n1 != n2 )
             {
                type = ITEM_TYPE_LONG ;
             }
         }
      }
      else
      {
         type = ITEM_TYPE_STRING ;
      }

   done:
      csvType = type ;
      if ( pVarInt )
      {
         (*pVarInt) = n1 ;
      }
      if ( pVarLong )
      {
         (*pVarLong) = n2 ;
      }
      if( pVarDouble )
      {
         (*pVarDouble) = n ;
      }
      return rc ;
   }

   static INT32 _parseValue( const CHAR *pStr, INT32 length, itemInfo &value,
                             BOOLEAN enableType, BOOLEAN strDelimiter )
   {
      INT32 rc = SDB_OK ;

      //is string "xxxx"
      if ( CHAR_DOUBLE_QUOTE == *pStr &&
           CHAR_DOUBLE_QUOTE == *(pStr + length - 1) )
      {
         value.type = ITEM_TYPE_STRING ;
         value.pVarString = pStr + 1 ;
         value.stringLength = length - 2 ;
         goto done ;
      }
      //is string 'xxxx'
      else if ( FALSE == strDelimiter &&
                CHAR_SINGLE_QUOTE == *pStr &&
                CHAR_SINGLE_QUOTE == *(pStr + length - 1) )
      {
         value.type = ITEM_TYPE_STRING ;
         value.pVarString = pStr + 1 ;
         value.stringLength = length - 2 ;
         goto done ;
      }
      //not string  xxxxx
      else
      {
         //is number
         if ( TRUE == enableType )
         {
            rc =  _parseNumber ( pStr, length,
                                 value.type,
                                 &value.varInt,
                                 &value.varLong,
                                 &value.varDouble ) ;
            if( rc )
            {
               goto error ;
            }
         }
         else
         {
            value.type = ITEM_TYPE_STRING ;
         }

         if ( ITEM_TYPE_STRING == value.type )
         {
            //is bool
            if ( TRUE == enableType &&
                 SDB_OK == ossStrToBoolean( pStr, &value.varBool ) )
            {
               value.type = ITEM_TYPE_BOOLEAN ;
            }
            else
            {
               value.pVarString = pStr ;
               value.stringLength = length ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

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

   INT32 _sptUsrOmaCommon::getIniConfigs( const bson::BSONObj &arg,
                                          bson::BSONObj &retObj,
                                          string &err )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN enableType = FALSE ;
      BOOLEAN strDelimiter = TRUE ;
      string confFile ;
      BSONObj conf ;

      if( !arg.hasField( "confFile" ) )
      {
         rc = SDB_INVALIDARG ;
         err = "confFile must be config" ;
         goto error ;
      }

      if( String != arg.getField( "confFile" ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "confFile must be string" ;
         goto error ;
      }

      confFile = arg.getStringField( "confFile" ) ;

      if( arg.hasField( "EnableType" ) )
      {
         if( Bool != arg.getField( "EnableType" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "EnableType must be BOOLEAN" ;
            goto error ;
         }

         enableType = arg.getBoolField( "EnableType" ) ;
      }

      if( arg.hasField( "StrDelimiter" ) )
      {
         if( Bool != arg.getField( "StrDelimiter" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "StrDelimiter must be BOOLEAN" ;
            goto error ;
         }

         strDelimiter = arg.getBoolField( "StrDelimiter" ) ;
      }

      rc = _getConfInfo( confFile, conf, err, FALSE, FALSE,
                         enableType, strDelimiter ) ;
      if ( rc )
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

   INT32 _sptUsrOmaCommon::setIniConfigs( const bson::BSONObj &arg,
                                          const bson::BSONObj &confObj,
                                          string &err )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN noDelimiter = FALSE ;
      BOOLEAN enableType = FALSE ;
      BOOLEAN strDelimiter = TRUE ;
      string confFile ;
      string str ;

      if( !arg.hasField( "confFile" ) )
      {
         rc = SDB_INVALIDARG ;
         err = "confFile must be config" ;
         goto error ;
      }

      if( String != arg.getField( "confFile" ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "confFile must be string" ;
         goto error ;
      }

      confFile = arg.getStringField( "confFile" ) ;

      if( arg.hasField( "EnableType" ) )
      {
         if( Bool != arg.getField( "EnableType" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "EnableType must be BOOLEAN" ;
            goto error ;
         }

         enableType = arg.getBoolField( "EnableType" ) ;
      }

      if( arg.hasField( "StrDelimiter" ) )
      {
         if( jstNULL == arg.getField( "StrDelimiter" ).type() )
         {
            noDelimiter = TRUE ;
         }
         else if( Bool == arg.getField( "StrDelimiter" ).type() )
         {
            strDelimiter = arg.getBoolField( "StrDelimiter" ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            err = "StrDelimiter must be BOOLEAN" ;
            goto error ;
         }
      }

      rc = _config2Ini( confObj, str, err,
                        noDelimiter, enableType, strDelimiter ) ;
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

      // get hostname
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

      // get svcname
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

      // get isReplace
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

      // get confFile
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
               // same with hostname, not change
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
            // same with default, not change
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
            // not exist, don't delete
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

      // exePath + ../conf/sdbcm.conf
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

   INT32 _sptUsrOmaCommon::_getConfInfo( const string &confFile, BSONObj &conf,
                                         string &err,
                                         BOOLEAN allowNotExist,
                                         BOOLEAN isSdbConfig,
                                         BOOLEAN enableType,
                                         BOOLEAN strDelimiter )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po ::variables_map vm ;

      if ( isSdbConfig )
      {
         MAP_CONFIG_DESC( desc ) ;
      }
      else
      {
         MAP_NORMAL_CONFIG_DESC( desc ) ;
      }

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
            if ( isSdbConfig &&( SDBCM_RESTART_COUNT == it->first ||
                                 SDBCM_RESTART_INTERVAL == it->first ||
                                 SDBCM_DIALOG_LEVEL == it->first ) )
            {
               builder.append( it->first, it->second.as<INT32>() ) ;
            }
            else if ( isSdbConfig && SDBCM_AUTO_START == it->first )
            {
               BOOLEAN autoStart = TRUE ;
               ossStrToBoolean( it->second.as<string>().c_str(), &autoStart ) ;
               builder.appendBool( it->first, autoStart ) ;
            }
            else
            {
               string value = it->second.as<string>() ;
               itemInfo valueData ;

               _parseValue( value.c_str(), value.length(), valueData,
                            enableType, strDelimiter ) ;

               if ( ITEM_TYPE_INT == valueData.type )
               {
                  builder.append( it->first, valueData.varInt ) ;
               }
               else if ( ITEM_TYPE_LONG == valueData.type )
               {
                  builder.append( it->first, valueData.varLong ) ;
               }
               else if ( ITEM_TYPE_DOUBLE == valueData.type )
               {
                  builder.append( it->first, valueData.varDouble ) ;
               }
               else if ( ITEM_TYPE_BOOLEAN == valueData.type )
               {
                  builder.appendBool( it->first, valueData.varBool ) ;
               }
               else
               {
                  builder.appendStrWithNoTerminating( it->first,
                                                      valueData.pVarString,
                                                      valueData.stringLength ) ;
               }
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

   INT32 _sptUsrOmaCommon::_config2Ini( const BSONObj &config,
                                        string &out,
                                        string &err,
                                        BOOLEAN noDelimiter,
                                        BOOLEAN enableType,
                                        BOOLEAN strDelimiter )
   {
      INT32 rc = SDB_OK ;
      CHAR delimiterChar ;
      map<string, string> sectionList ;
      BSONObjIterator it ( config ) ;

      if ( strDelimiter )
      {
         delimiterChar = CHAR_DOUBLE_QUOTE ;
      }
      else
      {
         delimiterChar = CHAR_SINGLE_QUOTE ;
      }

      while ( it.more() )
      {
         BSONElement e = it.next() ;
         BSONType type = e.type() ;
         const CHAR *pKey = e.fieldName() ;
         const CHAR *pPoint = ossStrchr( pKey, '.' ) ;
         string section ;
         string key ;
         stringstream ss ;

         if ( pPoint )
         {
            INT32 sectionLength = pPoint - pKey ;
            INT32 length = ossStrlen( pKey ) ;

            section = string( pKey, 0, sectionLength ) ;
            key = string( pKey, sectionLength + 1, length - sectionLength - 1 );
         }
         else
         {
            section = "" ;
            key = pKey ;
         }

         ss << key << "=" ;

         if ( FALSE == noDelimiter &&
              ( FALSE == enableType || e.type() == String ) )
         {
            ss << delimiterChar ;
         }

         if ( String == type )
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
            FLOAT64 val = e.numberDouble() ;
            CHAR tmp[SPT_DOUBLE_TEMP_SIZE] = { 0 } ;

            ossSnprintf( tmp, SPT_DOUBLE_TEMP_SIZE, "%.16g", val ) ;
            ss << tmp ;
         }
         else if ( e.type() == Bool )
         {
            ss << ( e.boolean() ? "true" : "false" ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            err = e.toString() + " is invalid config" ;
            goto error ;
         }

         if ( FALSE == noDelimiter &&
              ( FALSE == enableType || e.type() == String ) )
         {
            ss << delimiterChar ;
         }

         ss << endl ;

         if ( sectionList.find( section ) == sectionList.end() )
         {
            sectionList[section] = ss.str() ;
         }
         else
         {
            sectionList[section] += ss.str() ;
         }
      }

      {
         stringstream ss ;
         map<string, string>::iterator iter ;

         for ( iter = sectionList.begin(); iter != sectionList.end(); ++iter )
         {
            string section = iter->first ;

            if ( iter != sectionList.begin() )
            {
               ss << endl ;
            }

            if ( section.length() > 0 )
            {
               ss << "[" << section << "]" << endl ;
            }

            ss << iter->second ;
         }

         out = ss.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaCommon::_confObj2Str( const BSONObj &conf,
                                         string &str,
                                         string &err,
                                         const CHAR* pExcept,
                                         BOOLEAN isSdbConfig,
                                         BOOLEAN enableType,
                                         BOOLEAN strDelimiter )
   {
      INT32 rc = SDB_OK ;
      CHAR delimiterChar ;
      stringstream ss ;
      BSONObjIterator it ( conf ) ;

      if ( strDelimiter )
      {
         delimiterChar = CHAR_DOUBLE_QUOTE ;
      }
      else
      {
         delimiterChar = CHAR_SINGLE_QUOTE ;
      }

      while ( it.more() )
      {
         BSONElement e = it.next() ;

         if ( pExcept && 0 != pExcept[0] &&
              0 == ossStrcmp( pExcept, e.fieldName() ) )
         {
            continue ;
         }

         ss << e.fieldName() << "=" ;

         if ( ( FALSE == enableType || e.type() == String ) &&
              FALSE == isSdbConfig )
         {
            ss << delimiterChar ;
         }

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
            FLOAT64 val = e.numberDouble() ;
            CHAR tmp[SPT_DOUBLE_TEMP_SIZE] = { 0 } ;

            ossSnprintf( tmp, SPT_DOUBLE_TEMP_SIZE, "%.16g", val ) ;
            ss << tmp ;
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

         if ( ( FALSE == enableType || e.type() == String ) &&
              FALSE == isSdbConfig )
         {
            ss << delimiterChar ;
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
