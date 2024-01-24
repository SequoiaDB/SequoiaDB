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

   Source File Name = sptDBCount.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBCount.hpp"
#include "sptDBCL.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_COUNT_NAME              "CLCount"
   #define SPT_COUNT_CONDITION_FIELD   "_condition"
   #define SPT_COUNT_HINT_FIELD        "_hint"
   #define SPT_COUNT_COLLECTION_FIELD  "_collection"
   #define SPT_COUNT_COUNT_FIELD       "count"

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBCount, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBCount, destruct )

   JS_BEGIN_MAPPING( _sptDBCount, "CLCount" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBCount::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBCount::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBCount::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBCount::_sptDBCount()
   {
   }

   _sptDBCount::~_sptDBCount()
   {
   }

   INT32 _sptDBCount::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      return SDB_OK ;
   }

   INT32 _sptDBCount::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBCount::cvtToBSON( const CHAR* key, const sptObject &value,
                                 BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                 string &errMsg )
   {
      errMsg = "SdbCount can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBCount::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                 string &errMsg )
   {
      INT32 rc = SDB_OK ;
      sptObjectPtr clPtr ;
      sptObjectPtr objPtr ;
      BSONObj cond ;
      BSONObj hint ;
      _sptDBCL *pCL = NULL ;
      SINT64 count = 0 ;

      rc = value.getObjectField( SPT_COUNT_COLLECTION_FIELD, clPtr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get collection js obj" ;
         goto error ;
      }
      rc = clPtr->getUserObj( _sptDBCL::__desc, (const void**)&pCL ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get SdbCL obj" ;
         goto error ;
      }
      // Get _query field
      if( value.isFieldExist( SPT_COUNT_CONDITION_FIELD ) )
      {
         rc = value.getObjectField( SPT_COUNT_CONDITION_FIELD, objPtr ) ;
         if( SDB_OK == rc )
         {
            rc = objPtr->toBSON( cond ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Field _query must be obj" ;
               goto error ;
            }
         }
      }
      // Get _hint field
      if( value.isFieldExist( SPT_COUNT_HINT_FIELD ) )
      {
         rc = value.getObjectField( SPT_COUNT_HINT_FIELD, objPtr ) ;
         if( SDB_OK == rc )
         {
            rc = objPtr->toBSON( hint ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Field _hint must be obj" ;
               goto error ;
            }
         }
      }
      rc = pCL->getCount( count, cond, hint ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get count" ;
         goto error ;
      }
      retObj = BSON( SPT_COUNT_COUNT_FIELD << count ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCount::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                   _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 count ;
      BSONElement ele ;
      ele = data.getField( SPT_COUNT_COUNT_FIELD ) ;
      if( NumberLong != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Invalid count type" ) ;
         goto error ;
      }
      count = ele.Long() ;
      rval.getReturnVal().setValue( count ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
}
