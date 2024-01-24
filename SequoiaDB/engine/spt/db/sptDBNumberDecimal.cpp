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

   Source File Name = sptDBNumberDecimal.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBNumberDecimal.hpp"
using namespace bson ;

namespace engine
{
   #define SPT_NUMBERDECIMAL_NAME                       "NumberDecimal"
   #define SPT_NUMBERDECIMAL_DECIMAL_FIELD              "_decimal"
   #define SPT_NUMBERDECIMAL_PRECISION_FIELD            "_precision"
   #define SPT_NUMBERDECIMAL_SPECIALOBJ_DECIMAL_FIELD   "$decimal"
   #define SPT_NUMBERDECIMAL_SPECIALOBJ_PRECISION_FIELD "$precision"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBNumberDecimal, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBNumberDecimal, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBNumberDecimal, help )

   JS_BEGIN_MAPPING( _sptDBNumberDecimal, SPT_NUMBERDECIMAL_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_NUMBERDECIMAL_SPECIALOBJ_DECIMAL_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBNumberDecimal::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBNumberDecimal::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBNumberDecimal::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBNumberDecimal::_sptDBNumberDecimal()
   {
   }

   _sptDBNumberDecimal::~_sptDBNumberDecimal()
   {
   }

   INT32 _sptDBNumberDecimal::construct( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32  rc = SDB_OK ;
      string decimalStr ;
      string precisionStr ;
      INT32  precision[2] = { 0 } ;    // stored temp precision and temp scale

      if( arg.argc() < 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "NumberDecimal() need at least one argument" ) ;
         goto error ;
      }

      if( arg.argc() > 2 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "NumberDecimal() has too many arguments" ) ;
         goto error ;
      }

      // get decimal data
      if( arg.isString( 0 ) )
      {
         rc = arg.getString( 0, decimalStr ) ;
         if( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Failed to get data" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "NumberDecimal decimal argument must be String" ) ;
         goto error ;
      }

      // get precision
      if( arg.argc() == 2 )
      {
         if( arg.isObject( 1 ) )
         {
            BSONObj precisionObj ;
            stringstream ss ;

            rc = arg.getBsonobj( 1, precisionObj ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to get precision" ) ;
               goto error ;
            }

            try
            {
               BSONObj::iterator itr( precisionObj ) ;
               UINT8 i = 0 ;
               while( itr.more() )
               {
                  if( 2 == i )
                  {
                     rc = SDB_INVALIDARG ;
                     detail = BSON( SPT_ERR <<
                                    "NumberDecimal precision has too many arguments" ) ;
                     goto error ;
                  }
                  itr.next().Val( precision[i] ) ;
                  ++i;
               }
               if( 1 == i )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR <<
                                 "NumberDecimal precision has too few arguments" ) ;
                  goto error ;
               }
               else
               {
                  ss << precision[0] << "," << precision[1] ;
               }
               precisionStr = ss.str() ;
            }
            catch( std::exception e )
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR << "Failed to get precision element" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "NumberDecimal precision argument must be int array" ) ;
            goto error ;
         }

         rc = _checkParameters( decimalStr, precision[0], precision[1], FALSE,
                                detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         rval.addSelfProperty( SPT_NUMBERDECIMAL_DECIMAL_FIELD )
                               ->setValue( decimalStr ) ;
         rval.addSelfProperty( SPT_NUMBERDECIMAL_PRECISION_FIELD )
                               ->setValue( precisionStr ) ;
      }
      else
      {
         rc = _checkParameters( decimalStr, precision[0], precision[1], TRUE,
                                detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         rval.addSelfProperty( SPT_NUMBERDECIMAL_DECIMAL_FIELD )
                               ->setValue( decimalStr ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberDecimal::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBNumberDecimal::cvtToBSON( const CHAR* key, const sptObject &value,
                                         BOOLEAN isSpecialObj,
                                         BSONObjBuilder& builder,
                                         string &errMsg )
   {
      INT32  rc = SDB_OK ;
      string decimalStr ;
      string precisionStr ;
      INT32  totalPrecision = 0 ;
      INT32  scale = 0 ;
      BOOLEAN appendSuccess = FALSE ;

      if( isSpecialObj )
      {
         rc = value.getStringField( SPT_NUMBERDECIMAL_SPECIALOBJ_DECIMAL_FIELD,
                                    decimalStr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "NumberDecimal decimal value must be String" ;
            goto error ;
         }

         if( value.isFieldExist( SPT_NUMBERDECIMAL_SPECIALOBJ_PRECISION_FIELD ) )
         {
            rc = value.getStringField( SPT_NUMBERDECIMAL_SPECIALOBJ_PRECISION_FIELD,
                                       precisionStr,
                                       SPT_CVT_FLAGS_FROM_OBJECT ) ;
            if( SDB_OK != rc )
            {
               errMsg = "NumberDecimal precision value must be int array" ;
               goto error ;
            }

            rc = _getDecimalPrecision( precisionStr.c_str(), &totalPrecision,
                                       &scale ) ;
            if ( SDB_OK != rc )
            {
               errMsg = "Invalid precision value: " + precisionStr ;
               goto error ;
            }

            appendSuccess = builder.appendDecimal( key, decimalStr,
                                                   totalPrecision, scale ) ;
         }
         else
         {
            appendSuccess = builder.appendDecimal( key, decimalStr ) ;
         }
      }
      else
      {
         rc = value.getStringField( SPT_NUMBERDECIMAL_DECIMAL_FIELD,
                                    decimalStr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "NumberDecimal decimal value must be String" ;
            goto error ;
         }

         if( value.isFieldExist( SPT_NUMBERDECIMAL_PRECISION_FIELD ) )
         {
            rc = value.getStringField( SPT_NUMBERDECIMAL_PRECISION_FIELD,
                                       precisionStr ) ;
            if( SDB_OK == rc )
            {
               rc = _getDecimalPrecision( precisionStr.c_str(), &totalPrecision,
                                          &scale ) ;

               if ( SDB_OK != rc )
               {
                  errMsg = "Invalid precision value: " + precisionStr ;
                  goto error ;
               }
               appendSuccess = builder.appendDecimal( key, decimalStr,
                                                      totalPrecision, scale ) ;
            }
            else
            {
               errMsg = "NumberDecimal precision value must be String" ;
               goto error ;
            }
         }
         else
         {
            appendSuccess = builder.appendDecimal( key, decimalStr ) ;
         }
      }

      if( FALSE == appendSuccess )
      {
         rc = SDB_INVALIDARG ;
         errMsg = "Invalid decimal value: " + decimalStr ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberDecimal::fmpToBSON( const sptObject &value,
                                         BSONObj &retObj, string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string decimalStr ;
      string precisionStr ;

      rc = value.getStringField( SPT_NUMBERDECIMAL_DECIMAL_FIELD, decimalStr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get decimal field" ;
         goto error ;
      }
      rc = value.getStringField( SPT_NUMBERDECIMAL_PRECISION_FIELD,
                                 precisionStr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get precision field" ;
         goto error ;
      }
      retObj = BSON( SPT_NUMBERDECIMAL_DECIMAL_FIELD << decimalStr <<
                     SPT_NUMBERDECIMAL_PRECISION_FIELD << precisionStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberDecimal::bsonToJSObj( sdbclient::sdb &db,
                                           const BSONObj &data,
                                           _sptReturnVal &rval,
                                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBNumberDecimal *pDecimal = NULL ;
      BSONElement decimalEle = data.getField( SPT_NUMBERDECIMAL_DECIMAL_FIELD ) ;
      BSONElement precisionEle = data.getField( SPT_NUMBERDECIMAL_PRECISION_FIELD );

      if( String != decimalEle.type() )
      {
         detail = BSON( SPT_ERR << "Decimal must be string" ) ;
         goto error ;
      }

      if( String != precisionEle.type() )
      {
         detail = BSON( SPT_ERR << "Precision must be string" ) ;
         goto error ;
      }

      pDecimal = SDB_OSS_NEW sptDBNumberDecimal() ;
      if( NULL == pDecimal )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBNumberDecimal obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptDBNumberDecimal >( pDecimal ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }

      rval.addReturnValProperty( SPT_NUMBERDECIMAL_DECIMAL_FIELD )
                               ->setValue( decimalEle.String() ) ;
      rval.addReturnValProperty( SPT_NUMBERDECIMAL_PRECISION_FIELD )
                               ->setValue( precisionEle.String() ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pDecimal ) ;
      goto done ;
   }

   INT32 _sptDBNumberDecimal::_getDecimalPrecision( const CHAR *precisionStr,
                                                    INT32 *totalPrecision,
                                                    INT32 *scale )
   {
      //precisionStr:10,6
      BOOLEAN isFirst = TRUE ;
      INT32 rc        = SDB_OK ;
      const CHAR *p   = precisionStr ;

      while ( NULL != p && '\0' != *p )
      {
         if ( ' ' == *p )
         {
            p++ ;
            continue ;
         }

         if ( ',' == *p )
         {
            if ( !isFirst )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            isFirst = FALSE ;
            p++ ;
            continue ;
         }

         if ( *p < '0' || *p > '9' )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         p++ ;
      }

      rc = sscanf ( precisionStr, "%d,%d", totalPrecision, scale ) ;
      if ( 2 != rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = SDB_OK ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberDecimal::_checkParameters( const bson::StringData& strDecimal,
                                                int precision,
                                                int scale,
                                                BOOLEAN isPrecisionNull,
                                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      bson::bsonDecimal decimal ;

      if( !isPrecisionNull )
      {
         rc = decimal.init( precision, scale ) ;
         if ( 0 != rc )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Invalid precision value" ) ;
            goto error ;
         }
      }

      rc = decimal.fromString( strDecimal.data() ) ;
      if ( 0 != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Invalid decimal value" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNumberDecimal::help( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"NumberDecimal\": " << endl ;
      ss << "   { \"$decimal\": <data> }   " << endl ;
      ss << "   { \"$decimal\": <data>, " << endl ;
      ss << "     \"$precision\": [ <precision>, <scale> ] }   " << endl ;
      ss << "   NumberDecimal( <data>[, [ <precision>, <scale> ] ] )" << endl ;
      ss << "                              "
         << "- Data type: high-precision number" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"NumberDecimal\": " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"NumberDecimal\": " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}