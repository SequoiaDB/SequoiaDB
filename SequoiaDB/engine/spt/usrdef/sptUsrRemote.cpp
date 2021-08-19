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

   Source File Name = sptUsrRemote.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrRemote.hpp"
#include "sptUsrRemoteAssit.hpp"
#include "ossUtil.hpp"
#include "msgDef.h"
#include "omagentDef.hpp"
#include "utilParam.hpp"
#include "../bson/bsonobj.h"


using namespace bson ;

namespace engine
{
   /*
      Function Define
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrRemote, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrRemote, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrRemote, toString )
   JS_MEMBER_FUNC_DEFINE( _sptUsrRemote, runCommand )
   JS_MEMBER_FUNC_DEFINE( _sptUsrRemote, close )
   JS_MEMBER_FUNC_DEFINE( _sptUsrRemote, getInfo )

   /*
      Function Map
   */
   JS_BEGIN_MAPPING( _sptUsrRemote, "Remote" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "toString", toString )
      JS_ADD_MEMBER_FUNC_WITHATTR( "__runCommand", runCommand, 0 )
      JS_ADD_MEMBER_FUNC( "getInfo", getInfo )
      JS_ADD_MEMBER_FUNC( "close", close )
   JS_MAPPING_END()

   _sptUsrRemote::_sptUsrRemote()
   {
      CHAR tmpName[ 10 ] = { 0 } ;
      ossSnprintf( tmpName, sizeof( tmpName ) - 1, "%u", SDBCM_DFT_PORT ) ;
      _hostname = "localhost" ;
      _svcname = tmpName ;
   }

   _sptUsrRemote::~_sptUsrRemote()
   {
   }

   INT32 _sptUsrRemote::construct( const _sptArguments & arg,
                                   _sptReturnVal & rval,
                                   BSONObj & detail)
   {
      INT32 rc = SDB_OK ;
      INT32 retCode = SDB_OK ;
      CHAR* retBuf = NULL ;
      // get hostname
      if ( arg.argc() >= 1 )
      {
         rc = arg.getString( 0, _hostname ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "hostname must be string" ) ;
            PD_LOG( PDERROR, "Failed to get hostname, rc: %d", rc ) ;
            goto error ;
         }
      }

      // get svcname
      if ( arg.argc() >= 2 )
      {
         rc = arg.getString( 1, _svcname ) ;
         if ( rc )
         {
            UINT16 port = 0 ;
            rc = arg.getNative( 1, (void*)&port, SPT_NATIVE_INT16 ) ;
            if ( rc )
            {
               detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
            }
            else if ( port <= 0 || port >= 65535 )
            {
               detail = BSON( SPT_ERR << "svcname must in range "
                              "( 0, 65535 )" ) ;
               rc = SDB_INVALIDARG ;
            }
            else
            {
               _svcname = boost::lexical_cast< string >( port ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;
      }

      rc = _assit.connect( _hostname.c_str(), _svcname.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect %s:%s, rc: %d",
                   _hostname.c_str(), _svcname.c_str(), rc ) ;

      rval.addSelfProperty( "_host" )->setValue( _hostname ) ;
      rval.addSelfProperty( "_svcname" )->setValue( _svcname ) ;

      rc = _assit.runCommand( "oma test", NULL, &retBuf, retCode ) ;
      sdbClearErrorInfo() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to check target server info" ) ;
         goto error ;
      }
      else if( SDB_OK != retCode )
      {
         rc = retCode ;
         detail = BSON( SPT_ERR << "Target server must be sdbcm" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      _assit.disconnect() ;
      goto done ;
   }

   INT32 _sptUsrRemote::destruct()
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrRemote::toString( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   BSONObj &detail )
   {
      string name = _hostname ;

      name += ":" ;
      name += _svcname ;
      rval.getReturnVal().setValue( name ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrRemote::close( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrRemote::runCommand( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 needRecv = TRUE ;
      BSONObj optionObj ;
      BSONObj matchObj ;
      BSONObj valueObj ;
      BSONObjBuilder builder ;
      BSONObj recvObj ;
      string command ;

      // get command
      rc = arg.getString( 0, command ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
         PD_LOG( PDERROR, "Command must be config, rc: %d", rc ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
         PD_LOG( PDERROR, "Command must be string, rc: %d", rc ) ;
         goto error ;
      }
      else if ( command.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "command can't be empty" ) ;
         PD_LOG( PDERROR, "Command can't be empty, rc: %d", rc ) ;
         goto error ;
      }

      // get optionObj
      if ( arg.argc() >= 2 )
      {
         rc = arg.getBsonobj( 1, optionObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get BSONObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get matchObj
      if ( arg.argc() >= 3 )
      {
         rc = arg.getBsonobj( 2, matchObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "matchObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get mathcObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get valueObj
      if ( arg.argc() >= 4 )
      {
         rc = arg.getBsonobj( 3, valueObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "valueObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get valueObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get argument needRecv
      if ( arg.argc() >= 5 )
      {
         rc = arg.getNative( 4, &needRecv, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "needRecv must be bool" ) ;
            goto error ;
         }
      }

      rc = runCommand( command, optionObj, matchObj, valueObj, detail,
                       recvObj, needRecv ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      rval.getReturnVal().setValue( recvObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrRemote::runCommand( const string &command,
                                    const BSONObj &optionObj,
                                    const BSONObj &matchObj,
                                    const BSONObj &valueObj,
                                    BSONObj &errDetail, BSONObj &retObj,
                                    BOOLEAN needRecv )
   {
      INT32 rc = SDB_OK ;
      INT32 retCode = SDB_OK ;
      CHAR *retBuffer = NULL ;
      BSONObjBuilder builder ;

      // merge arg
      rc = _mergeArg( optionObj, matchObj, valueObj, builder ) ;
      if ( SDB_OK != rc )
      {
         errDetail = BSON( SPT_ERR << "Failed to mergeArg" ) ;
         PD_LOG( PDERROR, "Failed to merge arg, rc: %d", rc ) ;
         goto error ;
      }

      // run command and get retrun BSONObj
      rc = _assit.runCommand( command, builder.obj().objdata(),
                              &retBuffer, retCode, needRecv ) ;
      if ( SDB_OK != rc )
      {
         errDetail = BSON( SPT_ERR << getErrDesp( rc ) ) ;
         PD_LOG( PDERROR, "Failed to run command in client, rc: %d", rc ) ;
         goto error ;
      }

      // if need recv, need to build recvObj ;
      if ( needRecv )
      {
         // build recvObj
         SDB_ASSERT( retBuffer, "retBuffer can't be null" ) ;
         rc = _initBSONObj( retObj, retBuffer ) ;
         if( SDB_OK != rc )
         {
            errDetail= BSON( SPT_ERR << "Failed to build recvObj" ) ;
            goto error ;
         }

         // check remote result
         if ( SDB_OK != retCode )
         {
            rc = retCode ;
            errDetail = BSON( SPT_ERR << retObj.getStringField( OP_ERR_DETAIL ) ) ;
            PD_LOG( PDERROR, "Failed to run command in remote sdbcm, "
                             "rc: %d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrRemote::getInfo( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      builder.append( "hostname", _hostname ) ;
      builder.append( "svcname", _svcname ) ;

      rval.getReturnVal().setValue( builder.obj() ) ;

      return rc ;
   }

   INT32 _sptUsrRemote::_mergeArg( const bson::BSONObj& optionObj,
                                   const bson::BSONObj& matchObj,
                                   const bson::BSONObj& valueObj,
                                   bson::BSONObjBuilder& builder )
   {
      builder.append( "$optionObj", optionObj ) ;
      builder.append( "$matchObj", matchObj ) ;
      builder.append( "$valueObj", valueObj ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrRemote::_initBSONObj( BSONObj &recvObj, const CHAR* buf )
   {
      INT32 rc = SDB_OK ;
      try
      {
         recvObj.init( buf ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build recvObj, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}
