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

   Source File Name = sptDBCipherUser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          26/05/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptDBCipherUser.hpp"
#include "utilCipherFile.hpp"
#include "msgDef.h"

using namespace bson ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBCipherUser, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBCipherUser, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBCipherUser, help )
   JS_MEMBER_FUNC_DEFINE( _sptDBCipherUser, setToken )

   JS_BEGIN_MAPPING( _sptDBCipherUser, SPT_CIPHERUSER_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "_setToken", setToken )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBCipherUser::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBCipherUser::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBCipherUser::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBCipherUser::_sptDBCipherUser()
   {
   }

   _sptDBCipherUser::~_sptDBCipherUser()
   {
   }

   INT32 _sptDBCipherUser::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32  rc = SDB_OK ;
      string username ;
      string cipherFile ;
      string clusterName ;

      if( 0 == arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "You should input username" ) ;
         goto error ;
      }

      if( arg.isString( 0 ) )
      {
         rc = arg.getString( 0, username ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get username" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Username must be string" ) ;
         goto error ;
      }

      rc = passwd::utilBuildDefaultCipherFilePath( cipherFile ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to build default cipher file path" ) ;
         goto error ;
      }

      rval.addSelfProperty( SPT_CIPHERUSER_CIPHER_FILE_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( cipherFile ) ;
      rval.addSelfProperty( SPT_CIPHERUSER_CLUSTER_NAME_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( clusterName ) ;
      rval.addSelfProperty( SPT_CIPHERUSER_USER_FIELD )->setValue( username ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCipherUser::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBCipherUser::setToken( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32  rc = SDB_OK ;

      if ( arg.argc() > 0 )
      {
         // if token has been input, we don't save command to history file.
         sdbSetIsNeedSaveHistory( FALSE ) ;
         if( arg.isString( 0 ) )
         {
            rc = arg.getString( 0, _token ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to get token" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Token must be string" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _sptDBCipherUser::getToken()
   {
      return _token.c_str() ;
   }

   INT32 _sptDBCipherUser::cvtToBSON( const CHAR* key,
                                      const sptObject &value,
                                      BOOLEAN isSpecialObj,
                                      BSONObjBuilder& builder,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = fmpToBSON( value, obj, errMsg ) ;
      if ( rc )
      {
         goto error ;
      }

      builder.append( key, obj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCipherUser::fmpToBSON( const sptObject &value,
                                      BSONObj &retObj,
                                      string &errMsg )
   {
      INT32  rc = SDB_OK ;
      string username ;
      string clusterName ;
      string cipherFile ;

      rc = value.getStringField( SPT_CIPHERUSER_USER_FIELD, username ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get username" ;
         goto error ;
      }

      rc = value.getStringField( SPT_CIPHERUSER_CLUSTER_NAME_FIELD,
                                 clusterName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cluster name" ;
         goto error ;
      }

      rc = value.getStringField( SPT_CIPHERUSER_CIPHER_FILE_FIELD,
                                 cipherFile ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cipher file" ;
         goto error ;
      }

      retObj = BSON( SDB_AUTH_USER << username.c_str() <<
                     SPT_CIPHERUSER_FIELD_NAME_CLUSTER_NAME <<
                     clusterName.c_str() <<
                     SPT_CIPHERUSER_FIELD_NAME_CIPHER_FILE <<
                     cipherFile.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCipherUser::bsonToJSObj( sdbclient::sdb &db,
                                        const BSONObj &data,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBCipherUser *pCipherUser = NULL ;

      pCipherUser = SDB_OSS_NEW sptDBCipherUser() ;
      if ( NULL == pCipherUser )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new CipherUser obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptDBCipherUser >( pCipherUser ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

      rval.addReturnValProperty( SPT_CIPHERUSER_USER_FIELD )
      ->setValue( data.getStringField( SDB_AUTH_USER ) ) ;

      rval.addReturnValProperty( SPT_CIPHERUSER_CLUSTER_NAME_FIELD )
      ->setValue( data.getStringField( SPT_CIPHERUSER_FIELD_NAME_CLUSTER_NAME ) ) ;

      rval.addReturnValProperty( SPT_CIPHERUSER_CIPHER_FILE_FIELD )
      ->setValue( data.getStringField( SPT_CIPHERUSER_FIELD_NAME_CIPHER_FILE ) ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCipherUser ) ;
      goto done ;
   }

   INT32 _sptDBCipherUser::help( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"CipherUser\": " << endl ;
      ss << "   CipherUser( <username> )[.token( <token> )]" << endl ;
      ss << "                           [.clusterName( <clusterName> )]" << endl ;
      ss << "                           [.cipherFile( <cipherFile> )]"
         << "   -- Create a CipherUser object" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"CipherUser\": " << endl ;
      ss << "   toString()                     "
         << "-- Convert CipherUser to string format" << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"CipherUser\": " << endl ;
      ss << "   getUsername()                  "
         << "-- Get username" << endl ;
      ss << "   getClusterName()               "
         << "-- Get cluster name" << endl ;
      ss << "   getCipherFile()                "
         << "-- Get cipher file path" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

