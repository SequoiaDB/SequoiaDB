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

   Source File Name = sptDBRegex.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBRegex.hpp"
using namespace bson ;
namespace engine
{
   #define SPT_REGEX_NAME  "Regex"
   #define SPT_REGEX_REGEX_FIELD  "_regex"
   #define SPT_REGEX_OPTION_FIELD "_option"
   #define SPT_REGEX_SPECIALOBJ_REGEX_FIELD  "$regex"
   #define SPT_REGEX_SPECIALOBJ_OPTION_FIELD "$options"
   #define SPT_REGEX_FIELD_NUM   2
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBRegex, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBRegex, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBRegex, help )

   JS_BEGIN_MAPPING( _sptDBRegex, SPT_REGEX_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_REGEX_SPECIALOBJ_REGEX_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBRegex::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBRegex::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBRegex::bsonToJSObj )
   JS_MAPPING_END()
   _sptDBRegex::_sptDBRegex()
   {
   }

   _sptDBRegex::~_sptDBRegex()
   {
   }

   INT32 _sptDBRegex::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string pattern ;
      string options ;
      rc = arg.getString( 0, pattern ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Regex pattern argument must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Regex Pattern argument must be String" ) ;
         goto error ;
      }
      rc = arg.getString( 1, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Regex options argument must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Regex options argument must be String" ) ;
         goto error ;
      }
      rval.addSelfProperty( SPT_REGEX_REGEX_FIELD )->setValue( pattern ) ;
      rval.addSelfProperty( SPT_REGEX_OPTION_FIELD )->setValue( options ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRegex::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBRegex::cvtToBSON( const CHAR* key, const sptObject &value,
                                 BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                 string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string regexStr ;
      string option ;
      if( isSpecialObj )
      {
         UINT32 fieldNumber = 0 ;
         rc = value.getFieldNumber( fieldNumber ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get field number" ;
            goto error ;
         }
         if( SPT_REGEX_FIELD_NUM != fieldNumber )
         {
            BSONObj tmpBSON ;
            rc = value.toBSON( tmpBSON ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Invalid Object" ;
               goto error ;
            }
            builder.append( key, tmpBSON ) ;
            goto done ;
         }
         // Regex obj
         rc = value.getStringField( SPT_REGEX_SPECIALOBJ_REGEX_FIELD,
                                    regexStr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Regex regex value must be String" ;
            goto error ;
         }
         rc = value.getStringField( SPT_REGEX_SPECIALOBJ_OPTION_FIELD,
                                    option ) ;
         if( SDB_OK != rc )
         {
            rc = SDB_SPT_NOT_SPECIAL_JSON ;
            errMsg = "Regex option value must be String" ;
            goto error ;
         }
      }
      else
      {
         rc = value.getStringField( SPT_REGEX_REGEX_FIELD, regexStr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Regex regex value must be String" ;
            goto error ;
         }
         rc = value.getStringField( SPT_REGEX_OPTION_FIELD, option ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Regex option value must be String" ;
            goto error ;
         }
      }
      builder.appendRegex( key, regexStr, option ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRegex::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                 string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string regexStr ;
      string option ;
      rc = value.getStringField( SPT_REGEX_REGEX_FIELD, regexStr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get regex field" ;
         goto error ;
      }
      rc = value.getStringField( SPT_REGEX_OPTION_FIELD, option ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get option field" ;
         goto error ;
      }
      retObj = BSON( SPT_REGEX_REGEX_FIELD << regexStr <<
                     SPT_REGEX_OPTION_FIELD << option ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRegex::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                   _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string regexStr ;
      string option ;
      BSONElement regexEle ;
      BSONElement optionEle ;
      sptDBRegex *pRegex = NULL ;

      regexEle = data.getField( SPT_REGEX_REGEX_FIELD ) ;
      optionEle = data.getField( SPT_REGEX_OPTION_FIELD ) ;
      if( String != regexEle.type() )
      {
         detail = BSON( SPT_ERR << "Regex must be string" ) ;
         goto error ;
      }
      if( String != optionEle.type() )
      {
         detail = BSON( SPT_ERR << "Option must be string" ) ;
         goto error ;
      }
      regexStr = regexEle.String() ;
      option = optionEle.String() ;

      pRegex = SDB_OSS_NEW sptDBRegex() ;
      if( NULL == pRegex )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBRegex obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBRegex >( pRegex ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      rval.addReturnValProperty( SPT_REGEX_REGEX_FIELD )->setValue( regexStr ) ;
      rval.addReturnValProperty( SPT_REGEX_OPTION_FIELD )->setValue( option ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pRegex ) ;
      goto done ;
   }

   INT32 _sptDBRegex::help( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"Regex\" : " << endl ;
      ss << "   { \"$regex\": <pattern>, \"$options\": <options> }   " << endl ;
      ss << "   Regex( <pattern>, <options> ) " << endl ;
      ss << "                              "
         << "- Regular expression" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"Regex\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"Regex\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}
