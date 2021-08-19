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

   Source File Name = sptDBNumberLong.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBNumberLong.hpp"
#include <boost/lexical_cast.hpp>
using namespace bson ;
namespace engine
{
   #define SPT_NUMBERLONG_NAME             "NumberLong"
   #define SPT_NUMBERLONG_VALUE_FIELD      "_v"
   #define SPT_NUMBERLONG_SPECIALOBJ_FIELD "$numberLong"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBNumberLong, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBNumberLong, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBNumberLong, help )

   JS_BEGIN_MAPPING( _sptDBNumberLong, SPT_NUMBERLONG_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_NUMBERLONG_SPECIALOBJ_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBNumberLong::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBNumberLong::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBNumberLong::bsonToJSObj )
      JS_SET_CVT_TO_STRING_FUNC( _sptDBNumberLong::cvtToString )
   JS_MAPPING_END()

   _sptDBNumberLong::_sptDBNumberLong()
   {
   }

   _sptDBNumberLong::~_sptDBNumberLong()
   {
   }

   INT32 _sptDBNumberLong::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "NumberLong() only need one argument" ) ;
         goto error ;
      }

      if( arg.isString( 0 ) )
      {
         string dataStr ;
         rc = arg.getString( 0, dataStr ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         rval.addSelfProperty( SPT_NUMBERLONG_VALUE_FIELD )
            ->setValue( dataStr ) ;
      }
      else if( arg.isNumber( 0 ) )
      {
         FLOAT64 dataNumber ;
         rc = arg.getNative( 0, &dataNumber, SPT_NATIVE_FLOAT64 ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         rval.addSelfProperty( SPT_NUMBERLONG_VALUE_FIELD )
                              ->setValue( dataNumber ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "NumberLong argument must be String or Number" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberLong::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBNumberLong::cvtToBSON( const CHAR* key, const sptObject &value,
                                      BOOLEAN isSpecialObj,
                                      BSONObjBuilder& builder,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string valStr ;
      INT64 val ;
      if( isSpecialObj )
      {
         rc = value.getStringField( SPT_NUMBERLONG_SPECIALOBJ_FIELD, valStr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Numberlong $numberlong value must be String" ;
            goto error ;
         }
         if ( !_isValidNumberLong( valStr.c_str() ) )
         {
            errMsg = "Invalid Numberlong value: " + valStr ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         val  = ossAtoll( valStr.c_str() ) ;
      }
      else
      {
         SPT_JS_TYPE fieldType ;
         rc = value.getFieldType( SPT_NUMBERLONG_VALUE_FIELD, fieldType ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get _v field type" ;
            goto error ;
         }
         if( SPT_JS_TYPE_STRING == fieldType )
         {
            rc = value.getStringField( SPT_NUMBERLONG_VALUE_FIELD, valStr,
                                       SPT_CVT_FLAGS_FROM_STRING ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Numberlong data value must be String or Number" ;
               goto error ;
            }
            try
            {
               val = boost::lexical_cast<INT64>( valStr ) ;
            }
            catch ( std::bad_cast &e )
            {
               errMsg =  "Invalid NumberLong value: " + valStr + ", " ;
               errMsg += e.what() ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if( SPT_JS_TYPE_INT == fieldType ||
                  SPT_JS_TYPE_DOUBLE == fieldType )
         {
            FLOAT64 tmpNumber = 0 ;
            rc = value.getDoubleField( SPT_NUMBERLONG_VALUE_FIELD, tmpNumber,
                                       SPT_CVT_FLAGS_FROM_INT |
                                       SPT_CVT_FLAGS_FROM_DOUBLE ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Numberlong data value must be String or Number" ;
               goto error ;
            }
            val = static_cast< INT64 >( tmpNumber ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            errMsg = "Numberlong data value must be String or Number" ;
            goto error ;
         }
      }
      builder.append( key, val ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberLong::cvtToString( const sptObject &obj,
                                        BOOLEAN isSpecialObj,
                                        string &retVal )
   {
      INT32 rc = SDB_OK ;
      string valStr ;

      if( isSpecialObj )
      {
         SPT_JS_TYPE fieldType ;
         rc = obj.getFieldType( SPT_NUMBERLONG_SPECIALOBJ_FIELD, fieldType ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         if( SPT_JS_TYPE_STRING == fieldType )
         {
            rc = obj.getStringField( SPT_NUMBERLONG_SPECIALOBJ_FIELD, valStr ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            if ( !_isValidNumberLong( valStr.c_str() ) )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            retVal  = valStr ;
         }
         else if( SPT_JS_TYPE_INT == fieldType ||
                  SPT_JS_TYPE_DOUBLE == fieldType )
         {
            FLOAT64 doubleNumber ;
            stringstream ss ;
            rc = obj.getDoubleField( SPT_NUMBERLONG_VALUE_FIELD, doubleNumber,
                                     SPT_CVT_FLAGS_FROM_INT |
                                     SPT_CVT_FLAGS_FROM_DOUBLE ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            ss << doubleNumber ;
            retVal = ss.str() ;
         }
      }
      else
      {
         SPT_JS_TYPE fieldType ;
         rc = obj.getFieldType( SPT_NUMBERLONG_VALUE_FIELD, fieldType ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         if( SPT_JS_TYPE_STRING == fieldType )
         {
            rc = obj.getStringField( SPT_NUMBERLONG_VALUE_FIELD, valStr,
                                     SPT_CVT_FLAGS_FROM_STRING ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            retVal = valStr ;
         }
         else if( SPT_JS_TYPE_INT == fieldType ||
                  SPT_JS_TYPE_DOUBLE == fieldType )
         {
            FLOAT64 tmpNumber ;
            stringstream ss ;
            rc = obj.getDoubleField( SPT_NUMBERLONG_VALUE_FIELD, tmpNumber,
                                     SPT_CVT_FLAGS_FROM_INT |
                                     SPT_CVT_FLAGS_FROM_DOUBLE ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            ss << tmpNumber ;
            retVal = ss.str() ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberLong::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      SPT_JS_TYPE type ;
      rc = value.getFieldType( SPT_NUMBERLONG_VALUE_FIELD, type ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get numberLong field type" ;
         goto error ;
      }
      if( SPT_JS_TYPE_INT == type || SPT_JS_TYPE_DOUBLE == type )
      {
         FLOAT64 val = 0 ;
         rc = value.getDoubleField( SPT_NUMBERLONG_VALUE_FIELD, val,
                                    SPT_CVT_FLAGS_FROM_INT |
                                    SPT_CVT_FLAGS_FROM_DOUBLE ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get numberLong field" ;
            goto error ;
         }
         retObj = BSON( SPT_NUMBERLONG_VALUE_FIELD << val ) ;
      }
      else if( SPT_JS_TYPE_STRING == type )
      {
         string val ;
         rc = value.getStringField( SPT_NUMBERLONG_VALUE_FIELD, val ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get numberLong field" ;
            goto error ;
         }
         retObj = BSON( SPT_NUMBERLONG_VALUE_FIELD << val ) ;
      }
      else
      {
         errMsg = "NumberLong field type must be number or string" ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberLong::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONType type ;
      BSONElement ele ;
      string valStr ;
      FLOAT64 valDouble = 0.0 ;
      sptDBNumberLong *pNumberLong = NULL ;
      ele = data.getField( SPT_NUMBERLONG_VALUE_FIELD ) ;
      type = ele.type() ;
      if( String == type )
      {
         valStr = ele.String() ;
      }
      else if( NumberDouble == type )
      {
         valDouble = ele.Double() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Field must be string or double" ) ;
         goto error ;
      }
      pNumberLong = SDB_OSS_NEW sptDBNumberLong() ;
      if( NULL == pNumberLong )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBNumberLong obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBNumberLong >( pNumberLong ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      if( String == type )
      {
         rval.addReturnValProperty( SPT_NUMBERLONG_VALUE_FIELD )
            ->setValue( valStr ) ;
      }
      else if( NumberDouble == type )
      {
         rval.addReturnValProperty( SPT_NUMBERLONG_VALUE_FIELD )
            ->setValue( valDouble ) ;
      }
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNumberLong ) ;
      goto done ;
   }

   BOOLEAN _sptDBNumberLong::_isValidNumberLong( const CHAR *value )
   {
      UINT32 len = 0 ;
      UINT32 i = 0;
      if ( NULL == value  )
         return FALSE ;
      len = ossStrlen( value ) ;
      if ( len > 0 )
      {
         if ( value[0] == '-' )
         {
            ++i ;
         }
      }
      for ( ; i < len; ++i )
      {
         if ( ! ( value[i] >= '0' && value[i] <= '9' ) )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   INT32 _sptDBNumberLong::help( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"NumberLong\": " << endl ;
      ss << "   { \"$numberLong\": <data> }   " << endl ;
      ss << "   NumberLong( <data> )             "
         << "-- Data type: long integer" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"NumberLong\": " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"NumberLong\": " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

