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

   Source File Name = sptDBCS.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBCS.hpp"
#include "sptDBCL.hpp"
using sdbclient::_sdbCollectionSpace ;
using sdbclient::sdbCollectionSpace ;
using sdbclient::_sdbCollection ;
namespace engine
{
   #define SPT_CS_NAME  "SdbCS"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBCS, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBCS, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, createCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, dropCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, getCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, renameCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, alter )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, setDomain )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, removeDomain )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, enableCapped )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, disableCapped )
   JS_MEMBER_FUNC_DEFINE( _sptDBCS, setAttributes )
   JS_RESOLVE_FUNC_DEFINE( _sptDBCS, resolve )

   JS_BEGIN_MAPPING( _sptDBCS, SPT_CS_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "createCL", createCL)
      JS_ADD_MEMBER_FUNC( "dropCL", dropCL )
      JS_ADD_MEMBER_FUNC( "getCL", getCL )
      JS_ADD_MEMBER_FUNC( "renameCL", renameCL )
      JS_ADD_MEMBER_FUNC( "alter", alter )
      JS_ADD_MEMBER_FUNC( "setDomain", setDomain )
      JS_ADD_MEMBER_FUNC( "removeDomain", removeDomain )
      JS_ADD_MEMBER_FUNC( "enableCapped", enableCapped )
      JS_ADD_MEMBER_FUNC( "disableCapped", disableCapped )
      JS_ADD_MEMBER_FUNC( "setAttributes", setAttributes )
      JS_ADD_RESOLVE_FUNC( resolve )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBCS::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBCS::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBCS::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBCS::_sptDBCS( _sdbCollectionSpace *pCS )
   {
      _cs.pCollectionSpace = pCS ;
   }

   _sptDBCS::~_sptDBCS()
   {
   }

   INT32 _sptDBCS::construct( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR <<
                     "use of new SdbCS() is forbidden, you should use "
                     "other functions to produce a SdbCS object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBCS::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBCS::createCL( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string clName ;
      BSONObj options ;
      _sdbCollection *pCL = NULL ;
      sptDBCL *sptCL = NULL ;
      rc = arg.getString( 0, clName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Name must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _cs.createCollection( clName.c_str(), options, &pCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create cl" ) ;
         goto error ;
      }
      sptCL = SDB_OSS_NEW sptDBCL( pCL ) ;
      if( NULL == sptCL )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptCL obj" ) ;
         goto error ;
      }
      pCL = NULL ;

      rc = rval.setUsrObjectVal< sptDBCL >( sptCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      sptCL = NULL ;

      rval.getReturnVal().setName( clName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_CL_NAME_FIELD )->setValue( clName ) ;
      rval.addSelfToReturnValProperty( SPT_CL_CS_FIELD ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCL ) ;
      SAFE_OSS_DELETE( sptCL ) ;
      goto done ;
   }

   INT32 _sptDBCS::dropCL( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string clName ;

      rc = arg.getString( 0, clName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CLName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CLName must be string" ) ;
         goto error ;
      }
      rc = _cs.dropCollection( clName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop cl" ) ;
         goto error ;
      }
      rval.addSelfProperty( clName )->setDelete() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::getCL( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string clName ;

      rc = arg.getString( 0, clName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CLName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CLName must be string" ) ;
         goto error ;
      }

      rc = getCLAndSetProperty( clName, rval, detail ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::renameCL( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oldName ;
      string newName ;
      BSONObj options ;

      rc = arg.getString( 0, oldName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Old name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Old name must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, newName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "New name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "New name must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }

      rc = _cs.renameCollection( oldName.c_str(), newName.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to rename cl" ) ;
         goto error ;
      }
      rval.addSelfProperty( oldName )->setDelete() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::resolve( const _sptArguments &arg,
                            UINT32 opcode,
                            BOOLEAN &processed,
                            string &callFunc,
                            BOOLEAN &setIDProp,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      if( SPT_JSOP_GETPROP == opcode )
      {
         processed = TRUE ;
         callFunc = "_resolveCL" ;
      }
      return SDB_OK ;
   }

   INT32 _sptDBCS::getCLAndSetProperty( const string &clName,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCollection *pCL = NULL ;
      sptDBCL *sptCL = NULL ;

      rc = _cs.getCollection( clName.c_str(), &pCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get cl" ) ;
         goto error ;
      }

      sptCL = SDB_OSS_NEW sptDBCL( pCL ) ;
      if( NULL == sptCL )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBCL obj" ) ;
         goto error ;
      }
      pCL = NULL ;

      rc = rval.setUsrObjectVal< sptDBCL >( sptCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      sptCL = NULL ;

      rval.getReturnVal().setName( clName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_CL_NAME_FIELD )->setValue( clName ) ;
      rval.addSelfToReturnValProperty( SPT_CL_CS_FIELD ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCL ) ;
      SAFE_OSS_DELETE( sptCL ) ;
      goto done ;
   }

   INT32 _sptDBCS::alter( const _sptArguments &arg,
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
      rc = _cs.alterCollectionSpace( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter cs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::setDomain( const _sptArguments &arg,
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
      rc = _cs.setDomain( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set domain" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::removeDomain( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         goto error ;
      }
      rc = _cs.removeDomain() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove domain" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::enableCapped( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         goto error ;
      }
      rc = _cs.enableCapped() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to enable capped" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::disableCapped( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         goto error ;
      }
      rc = _cs.disableCapped() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to disable capped" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::setAttributes( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() == 0 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _cs.setAttributes( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set cs attributes" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg )
   {
      errMsg = "SdbCS can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBCS::fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string csName ;
      rc = value.getStringField( SPT_CS_NAME_FIELD, csName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cs _name field" ;
         goto error ;
      }
      retObj = BSON( SPT_CS_NAME_FIELD << csName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCS::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string csName ;
      _sdbCollectionSpace *pCS = NULL ;
      sptDBCS *pSptCS = NULL ;
      csName = data.getStringField( SPT_CS_NAME_FIELD ) ;
      rc = db.getCollectionSpace( csName.c_str(), &pCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get CollectionSpace" ) ;
         goto error ;
      }
      pSptCS = SDB_OSS_NEW sptDBCS( pCS ) ;
      if( NULL == pSptCS )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBCS obj" ) ;
         goto error ;
      }
      pCS = NULL ;

      rc = rval.setUsrObjectVal< sptDBCS >( pSptCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      pSptCS = NULL ;

      rval.getReturnVal().setName( csName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_CS_NAME_FIELD )->setValue( csName ) ;
      rval.addSelfToReturnValProperty( SPT_CS_CONN_FIELD ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCS ) ;
      SAFE_OSS_DELETE( pSptCS ) ;
      goto done ;
   }
}
