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

   Source File Name = sptDBBinData.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBBinData.hpp"
#include <boost/lexical_cast.hpp>
using namespace bson ;
namespace engine
{
   #define SPT_BINDATA_NAME                     "BinData"
   #define SPT_BINDATA_DATA_FIELD               "_data"
   #define SPT_BINDATA_TYPE_FIELD               "_type"
   #define SPT_BINDATA_SPECIALOBJ_BINARY_FIELD  "$binary"
   #define SPT_BINDATA_SPECIALOBJ_TYPE_FIELD    "$type"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBBinData, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBBinData, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBBinData, help )

   JS_BEGIN_MAPPING( _sptDBBinData, SPT_BINDATA_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_BINDATA_SPECIALOBJ_BINARY_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBBinData::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBBinData::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBBinData::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBBinData::_sptDBBinData()
   {
   }

   _sptDBBinData::~_sptDBBinData()
   {
   }

   INT32 _sptDBBinData::construct( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string binData ;
      string type ;
      if( arg.argc() != 2 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "BinData() need two arguments" ) ;
         goto error ;
      }
      rc = arg.getString( 0, binData ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "BinData binary argument must be String" ) ;
         goto error ;
      }
      if( arg.isString( 1 ) )
      {
         rc = arg.getString( 1, type ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         // Check whether input str is a valid number
         try
         {
            INT32 typeNumber = boost::lexical_cast<UINT32>( type ) ;
            if ( typeNumber < 0 || 255 < typeNumber )
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR << "BinData type argument must to be between 0 and 255" ) ;
               goto error ;
            }
         }
         catch ( std::exception & )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Invalid BinData type value: " + type ) ;
            goto error ;
         }
      }
      else if( arg.isInt( 1 ))
      {
         INT32 typeNumber = -1 ;
         rc = arg.getNative( 1, &typeNumber, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         if ( typeNumber < 0 || 255 < typeNumber )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "BinData type argument must to be between 0 and 255" ) ;
            goto error ;
         }
         try
         {
            type = boost::lexical_cast<string>( typeNumber ) ;
         }
         catch ( std::exception & )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Invalid BinData type value: " + typeNumber ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "BinData type argument must be String or int" ) ;
         goto error ;
      }
      rval.addSelfProperty( SPT_BINDATA_DATA_FIELD )->setValue( binData ) ;
      rval.addSelfProperty( SPT_BINDATA_TYPE_FIELD )->setValue( type ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBBinData::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBBinData::cvtToBSON( const CHAR* key, const sptObject &value,
                                   BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                   string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string data ;
      string type ;
      INT32 binType = 0 ;
      INT32 decodeSize = 0 ;
      CHAR* decode = NULL ;

      if( isSpecialObj )
      {
         UINT32 fieldNumber = 0 ;
         rc = value.getFieldNumber( fieldNumber ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get filed number" ;
            goto error ;
         }
         if( fieldNumber != 2 )
         {
            BSONObj tmpObj ;
            rc = value.toBSON( tmpObj ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Failed to convert js obj to bson" ;
               goto error ;
            }
            builder.append( key, tmpObj ) ;
            goto done ;
         }
         rc = value.getStringField( SPT_BINDATA_SPECIALOBJ_BINARY_FIELD, data ) ;
         if( SDB_OK != rc )
         {
            errMsg = "BinData binary value must be String" ;
            goto error ;
         }
         rc = value.getStringField( SPT_BINDATA_SPECIALOBJ_TYPE_FIELD, type ) ;
         if( SDB_OK != rc )
         {
            errMsg = "BinData type value must be String" ;
            goto error ;
         }
      }
      else
      {
         rc = value.getStringField( SPT_BINDATA_DATA_FIELD, data ) ;
         if( SDB_OK != rc )
         {
            errMsg = "BinData binary value must be String" ;
            goto error ;
         }
         rc = value.getStringField( SPT_BINDATA_TYPE_FIELD, type ) ;
         if( SDB_OK != rc )
         {
            errMsg = "BinData type value must be String" ;
            goto error ;
         }
      }
      try
      {
         binType = boost::lexical_cast<INT32>( type.c_str() ) ;
         if ( binType > 255 )
         {
            errMsg = "Invalid BinData type value: " + type + ", type value must to be between 0 and 255";
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::bad_cast & )
      {
         errMsg = "Invalid BinData type value: " + type ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      decodeSize = getDeBase64Size( data.c_str() ) ;
      if ( decodeSize < 0 )
      {
         errMsg = "Invalid BinData binary value: " + data ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( decodeSize > 0 )
      {
         decode = ( CHAR * )SDB_OSS_MALLOC( decodeSize ) ;
         if ( NULL == decode )
         {
            errMsg = "Failed to allocate memory" ;
            rc = SDB_OOM ;
            goto error ;
         }
         memset ( decode, 0, decodeSize ) ;
         if ( base64Decode( data.c_str(), decode, decodeSize ) < 0 )
         {
            errMsg = "Failed to decode base64 code" ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         /// we can not push '\0' to bson
         builder.appendBinData( key, decodeSize - 1,
                                static_cast<BinDataType>( binType ), decode ) ;
      }
      else
      {
         builder.appendBinData( key, 0, static_cast<BinDataType>( binType ),
                                "" ) ;
      }
   done:
      if( NULL != decode )
      {
         SDB_OSS_FREE( decode ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBBinData::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                   string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string data ;
      string type ;
      rc = value.getStringField( SPT_BINDATA_DATA_FIELD, data ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get BinData data field" ;
         goto error ;
      }
      rc = value.getStringField( SPT_BINDATA_TYPE_FIELD, type ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get BinData type field" ;
         goto error ;
      }
      retObj = BSON( SPT_BINDATA_DATA_FIELD << data <<
                     SPT_BINDATA_TYPE_FIELD << type ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBBinData::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string binData ;
      string type ;
      BSONElement binDataEle ;
      BSONElement typeEle ;
      sptDBBinData *pBinData = NULL ;

      binDataEle = data.getField( SPT_BINDATA_DATA_FIELD ) ;
      typeEle = data.getField( SPT_BINDATA_TYPE_FIELD ) ;
      if( String != binDataEle.type() )
      {
         detail = BSON( SPT_ERR << "BinData data field must be string" ) ;
         goto error ;
      }
      if( String != typeEle.type() )
      {
         detail = BSON( SPT_ERR << "BinData type field must be string" ) ;
         goto error ;
      }
      binData = binDataEle.String() ;
      type = typeEle.String() ;
      pBinData = SDB_OSS_NEW sptDBBinData() ;
      if( NULL == pBinData )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBBinData obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBBinData >( pBinData ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      rval.addReturnValProperty( SPT_BINDATA_DATA_FIELD )->setValue( binData ) ;
      rval.addReturnValProperty( SPT_BINDATA_TYPE_FIELD )->setValue( type ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pBinData ) ;
      goto done ;
   }

   INT32 _sptDBBinData::help( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"BinData\" : " << endl ;
      ss << "   { \"$binary\": <data>, \"$type\": <type> }   " << endl ;
      ss << "   BinData( <data>, <type> )  "
         << "- Data type: binary data in base64 form" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"BinData\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"BinData\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}
