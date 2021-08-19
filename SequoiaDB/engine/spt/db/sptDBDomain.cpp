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

   Source File Name = sptDBDomain.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBDomain.hpp"
#include "sptDBCursor.hpp"
using namespace sdbclient ;
using namespace bson ;

namespace engine
{
   #define SPT_DOMAIN_NAME    "SdbDomain"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBDomain, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBDomain, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, alter )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, listCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, listCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, listGroup )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, addGroups )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, removeGroups )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, setGroups )
   JS_MEMBER_FUNC_DEFINE( _sptDBDomain, setAttributes )

   JS_BEGIN_MAPPING( _sptDBDomain, SPT_DOMAIN_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "alter", alter  )
      JS_ADD_MEMBER_FUNC( "listCollectionSpaces", listCS  )
      JS_ADD_MEMBER_FUNC( "listCollections", listCL  )
      JS_ADD_MEMBER_FUNC( "listGroups", listGroup  )
      JS_ADD_MEMBER_FUNC( "addGroups", addGroups  )
      JS_ADD_MEMBER_FUNC( "removeGroups", removeGroups  )
      JS_ADD_MEMBER_FUNC( "setGroups", setGroups  )
      JS_ADD_MEMBER_FUNC( "setAttributes", setAttributes  )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBDomain::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBDomain::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBDomain::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBDomain::_sptDBDomain( _sdbDomain *pDomain )
   {
      _domain.pDomain = pDomain ;
   }

   _sptDBDomain::~_sptDBDomain()
   {
   }

   INT32 _sptDBDomain::construct( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "use of new SdbDomain() is forbidden,"
                "you should use other functions to produce a SdbDomain object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBDomain::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBDomain::alter( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _domain.alterDomain( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter domain" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::listCL( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _domain.listCollectionsInDomain( &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list cl in domain" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBDomain::listCS( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _domain.listCollectionSpacesInDomain( &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list cs in domain" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBDomain::listGroup( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _domain.listReplicaGroupInDomain( &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list group in domain" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBDomain::addGroups( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _domain.addGroups( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to add groups" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::removeGroups( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _domain.removeGroups( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove groups" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::setGroups( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _domain.setGroups( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set groups" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::setAttributes( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _domain.setAttributes( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set attributes" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::cvtToBSON( const CHAR* key, const sptObject &value,
                                  BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                  string &errMsg )
   {
      errMsg = "SdbDomain can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBDomain::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                  string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string domainName ;
      rc = value.getStringField( SPT_DOMAIN_NAME_FIELD, domainName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get Domain name field" ;
         goto error ;
      }
      retObj = BSON( SPT_DOMAIN_NAME_FIELD << domainName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDomain::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                    _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string domainName ;
      _sdbDomain *pDomain = NULL ;
      sptDBDomain *pSptDomain = NULL ;
      domainName = data.getStringField( SPT_DOMAIN_NAME_FIELD ) ;
      rc = db.getDomain( domainName.c_str(), &pDomain ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get domain" ) ;
         goto error ;
      }
      pSptDomain = SDB_OSS_NEW sptDBDomain( pDomain ) ;
      if( NULL == pSptDomain )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBDomain obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBDomain >( pSptDomain ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      rval.addReturnValProperty( SPT_DOMAIN_NAME_FIELD )->setValue( domainName ) ;
   done:
      return rc ;
   error:
      if( NULL != pSptDomain )
      {
         SDB_OSS_DEL pSptDomain ;
         pSptDomain = NULL ;
         pDomain = NULL ;
      }
      SAFE_OSS_DELETE( pDomain ) ;
      goto done ;
   }
}
