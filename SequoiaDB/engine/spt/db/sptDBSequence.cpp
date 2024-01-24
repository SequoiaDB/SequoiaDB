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

   Source File Name = sptDBSequence.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/09/2020  LSQ Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBSequence.hpp"
using namespace std ;
using namespace bson ;
using sdbclient::_sdbSequence ;
using sdbclient::sdbSequence ;
namespace engine
{
   #define SPT_SEQ_NAME  "SdbSequence"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBSequence, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBSequence, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, setAttributes )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, getNextValue )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, getCurrentValue )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, setCurrentValue )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, fetch )
   JS_MEMBER_FUNC_DEFINE( _sptDBSequence, restart )

   JS_BEGIN_MAPPING( _sptDBSequence, SPT_SEQ_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "setAttributes", setAttributes )
      JS_ADD_MEMBER_FUNC( "getNextValue", getNextValue )
      JS_ADD_MEMBER_FUNC( "getCurrentValue", getCurrentValue )
      JS_ADD_MEMBER_FUNC( "setCurrentValue", setCurrentValue )
      JS_ADD_MEMBER_FUNC( "fetch", fetch )
      JS_ADD_MEMBER_FUNC( "restart", restart )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBSequence::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBSequence::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBSequence::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBSequence::_sptDBSequence( _sdbSequence *pSequence )
   {
      _sequence.pSequence = pSequence ;
   }

   _sptDBSequence::~_sptDBSequence()
   {
   }

   INT32 _sptDBSequence::construct( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      detail = BSON( SPT_ERR <<
                     "use of new SdbSequence() is forbidden, you should use "
                     "other functions to produce a SdbSequence object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBSequence::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBSequence::setAttributes( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() == 0 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "Options must be configured" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sequence.setAttributes( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set sequence attributes" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::getNextValue( const _sptArguments &arg,
                                       _sptReturnVal &rval,
                                       BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 value = 0 ;

      if( arg.argc() > 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many arguments" ) ;
         goto error ;
      }
      rc = _sequence.getNextValue( value ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get next value" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( value ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::getCurrentValue( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 value = 0 ;

      if( arg.argc() > 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many arguments" ) ;
         goto error ;
      }
      rc = _sequence.getCurrentValue( value ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get current value" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( value ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::setCurrentValue( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 value = 0 ;

      if( arg.argc() < 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "CurrentValue must be configured" ) ;
         goto error ;
      }
      if( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many arguments" ) ;
         goto error ;
      }
      if( !arg.isInt( 0 ) && !arg.isLong( 0 ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "CurrentValue should be number" ) ;
         goto error ;
      }
      rc = arg.getNative( 0, &value, SPT_NATIVE_INT64 ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to parse current value" ) ;
         goto error ;
      }
      rc = _sequence.setCurrentValue( value ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set current value" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::fetch( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 fetchNum = 0 ;
      INT64 nextValue = 0 ;
      INT32 returnNum = 0 ;
      INT32 increment = 0 ;
      bson::BSONObjBuilder builder( 64 ) ;
      bson::BSONObj result ;

      if( arg.argc() < 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Fetch number must be configured" ) ;
         goto error ;
      }
      if( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many arguments" ) ;
         goto error ;
      }
      if( !arg.isInt( 0 ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Argument should be number" ) ;
         goto error ;
      }
      rc = arg.getNative( 0, &fetchNum, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to parse fetch number" ) ;
         goto error ;
      }

      rc = _sequence.fetch( fetchNum, nextValue, returnNum, increment ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to fetch values" ) ;
         goto error ;
      }

      builder.append( SPT_SEQ_NEXT_VALUE_FIELD, nextValue ) ;
      builder.append( SPT_SEQ_RETURN_NUM_FIELD, returnNum ) ;
      builder.append( SPT_SEQ_INCREMENT_FIELD, increment ) ;
      result = builder.obj() ;

      rval.getReturnVal().setValue( result ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::restart( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT64 startValue = 0 ;

      if( arg.argc() < 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Start value must be configured" ) ;
         goto error ;
      }
      if( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many arguments" ) ;
         goto error ;
      }
      if( !arg.isInt( 0 ) && !arg.isLong( 0 ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Argument should be number" ) ;
         goto error ;
      }
      rc = arg.getNative( 0, &startValue, SPT_NATIVE_INT64 ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to parse start value" ) ;
         goto error ;
      }

      rc = _sequence.restart( startValue ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to restart sequence" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::cvtToBSON( const CHAR* key, const sptObject &value,
                                    BOOLEAN isSpecialObj,
                                    BSONObjBuilder& builder, string &errMsg )
   {
      errMsg = "SdbSequence can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBSequence::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                    string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string seqName ;
      rc = value.getStringField( SPT_SEQ_NAME_FIELD, seqName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get sequence _name field" ;
         goto error ;
      }
      retObj = BSON( SPT_SEQ_NAME_FIELD << seqName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSequence::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string seqName ;
      _sdbSequence *pSequence = NULL ;
      sptDBSequence *pSptSequence = NULL ;
      seqName = data.getStringField( SPT_SEQ_NAME_FIELD ) ;
      rc = db.getSequence( seqName.c_str(), &pSequence ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get sequence" ) ;
         goto error ;
      }
      pSptSequence = SDB_OSS_NEW sptDBSequence( pSequence ) ;
      if( NULL == pSptSequence )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBSequence obj" ) ;
         goto error ;
      }
      pSequence = NULL ;

      rc = rval.setUsrObjectVal< sptDBSequence >( pSptSequence ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      pSptSequence = NULL ;

      rval.getReturnVal().setName( seqName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_SEQ_NAME_FIELD )->setValue( seqName ) ;
      rval.addSelfToReturnValProperty( SPT_SEQ_CONN_FIELD ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSequence ) ;
      SAFE_OSS_DELETE( pSptSequence ) ;
      goto done ;
   }
}
