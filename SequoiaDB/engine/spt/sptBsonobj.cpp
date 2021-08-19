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

   Source File Name = sptBsonobj.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptBsonobj.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptBsonobj implement
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptBsonobj, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptBsonobj, destruct)
   JS_MEMBER_FUNC_DEFINE_NORESET( _sptBsonobj, toJson )

   JS_BEGIN_MAPPING( _sptBsonobj, "BSONObj" )
     JS_ADD_MEMBER_FUNC( "toJson", toJson )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
     JS_SET_CVT_TO_BSON_FUNC( _sptBsonobj::cvtToBSON )
     JS_SET_JSOBJ_TO_BSON_FUNC( _sptBsonobj::fmpToBSON )
     JS_SET_BSON_TO_JSOBJ_FUNC( _sptBsonobj::bsonToJSObj )
   JS_MAPPING_END()

   _sptBsonobj::_sptBsonobj()
   {

   }

   _sptBsonobj::_sptBsonobj( const bson::BSONObj &obj )
   {
      _obj = obj.copy() ;
   }

   _sptBsonobj::~_sptBsonobj()
   {

   }

   INT32 _sptBsonobj::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail)
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      string errMsgStr ;

      rc = arg.getBsonobj( 0, obj ) ;
      if ( rc )
      {
         errMsgStr = arg.getErrMsg() ;
         if ( errMsgStr.empty() )
         {
            detail = BSON( SPT_ERR <<  "The 1st param is invalid") ;
         }
         else
         {
            detail = BSON( SPT_ERR << errMsgStr ) ;
         }
         goto error ;
      }
      _obj = obj ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobj::toJson( const _sptArguments &arg,
                              _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _obj.toString( false, true ) ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobj::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptBsonobj::cvtToBSON( const CHAR* key, const sptObject &value,
                                 BOOLEAN isSpecialObj, BSONObjBuilder& builder,
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

   INT32 _sptBsonobj::fmpToBSON( const sptObject &value,
                                 BSONObj &retObj,
                                 string &errMsg )
   {
      INT32 rc = SDB_OK ;

      _sptBsonobj *pBsonObj = NULL ;
      rc = value.getUserObj( _sptBsonobj::__desc, (const void **)&pBsonObj ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get BSONObj field" ;
         goto error ;
      }

      retObj = pBsonObj->getBson() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobj::bsonToJSObj( sdbclient::sdb &db,
                                   const BSONObj &data,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sptBsonobj *pBsonObj = NULL ;

      pBsonObj = SDB_OSS_NEW _sptBsonobj( data ) ;
      if ( !pBsonObj )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new _sptBsonobj" ) ;
         goto error ;
      }

      rval.setUsrObjectVal<_sptBsonobj>( pBsonObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

