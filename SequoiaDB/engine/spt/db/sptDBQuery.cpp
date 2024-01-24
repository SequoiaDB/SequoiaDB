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

   Source File Name = sptDBQuery.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBQuery.hpp"
#include "msgDef.hpp"
#include "sptDBCL.hpp"
using sdbclient::_sdbCursor ;
namespace engine
{
   #define SPT_QUERY_NAME           "SdbQuery"
   #define SPT_QUERY_QUERY_FIELD    "_query"
   #define SPT_QUERY_SELECT_FIELD   "_select"
   #define SPT_QUERY_SORT_FIELD     "_sort"
   #define SPT_QUERY_HINT_FIELD     "_hint"
   #define SPT_QUERY_SKIP_FIELD     "_skip"
   #define SPT_QUERY_OPTIONS_FIELD  "_options"
   #define SPT_QUERY_LIMIT_FIELD    "_limit"
   #define SPT_QUERY_FLAGS_FIELD    "_flags"
   #define SPT_QUERY_COLLECTION_FIELD  "_collection"

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBQuery, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBQuery, destruct )
   JS_RESOLVE_FUNC_DEFINE( _sptDBQuery, resolve )
   JS_BEGIN_MAPPING( _sptDBQuery, SPT_QUERY_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_RESOLVE_FUNC( resolve )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBQuery::cvtToBSON )
      JS_SET_JSOBJ_TO_CURSOR_FUNC( _sptDBQuery::fmpToCursor )
   JS_MAPPING_END()
   _sptDBQuery::_sptDBQuery(): _cl( NULL )
   {
   }

   _sptDBQuery::~_sptDBQuery()
   {
      _cl = NULL ;
   }

   INT32 _sptDBQuery::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 skip = 0 ;
      INT32 limit = -1 ;
      INT32 flags = 0 ;
      rval.addSelfProperty( SPT_QUERY_SKIP_FIELD, SPT_PROP_ENUMERATE )->setValue( skip ) ;
      rval.addSelfProperty( SPT_QUERY_LIMIT_FIELD, SPT_PROP_ENUMERATE )->setValue( limit ) ;
      rval.addSelfProperty( SPT_QUERY_FLAGS_FIELD, SPT_PROP_ENUMERATE )->setValue( flags ) ;
      return SDB_OK ;
   }

   INT32 _sptDBQuery::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBQuery::resolve( const _sptArguments &arg,
                               UINT32 opcode,
                               BOOLEAN &processed,
                               string &callFunc,
                               BOOLEAN &setIDProp,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      if( SPT_JSOP_GETELEMENT == opcode )
      {
         processed = TRUE ;
         setIDProp = TRUE ;
         callFunc = "arrayAccess" ;
      }
      return SDB_OK ;
   }

   INT32 _sptDBQuery::cvtToBSON( const CHAR* key, const sptObject &value,
                                 BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                 string &errMsg )
   {
      errMsg = "SdbQuery can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBQuery::fmpToCursor( const sptObject &value, _sdbCursor** pCursor,
                                   string &errMsg )
   {
      INT32 rc = SDB_OK ;
      sptObjectPtr clPtr ;
      sptObjectPtr objPtr ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj sort ;
      BSONObj hint ;
      BSONObj options ;
      INT32 numToSkip = 0 ;
      INT32 numToRet = -1 ;
      INT32 flags = 0 ;
      _sptDBCL *pCL = NULL ;

      rc = value.getObjectField( SPT_QUERY_COLLECTION_FIELD, clPtr ) ;
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
      if( value.isFieldExist( SPT_QUERY_QUERY_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERY_QUERY_FIELD, objPtr ) ;
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
      // Get _select field
      if( value.isFieldExist( SPT_QUERY_SELECT_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERY_SELECT_FIELD, objPtr ) ;
         if( SDB_OK == rc )
         {
            rc = objPtr->toBSON( sel ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Field _select must be obj" ;
               goto error ;
            }
         }
      }
      // Get _sort field
      if( value.isFieldExist( SPT_QUERY_SORT_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERY_SORT_FIELD, objPtr ) ;
         if( SDB_OK == rc )
         {
            rc = objPtr->toBSON( sort ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Field _sort must be obj" ;
               goto error ;
            }
         }
      }
      // Get _hint field
      if( value.isFieldExist( SPT_QUERY_HINT_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERY_HINT_FIELD, objPtr ) ;
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
      // Get _options field
      if( value.isFieldExist( SPT_QUERY_OPTIONS_FIELD ) )
      {
         rc = value.getObjectField( SPT_QUERY_OPTIONS_FIELD, objPtr ) ;
         if( SDB_OK == rc )
         {
            rc = objPtr->toBSON( options ) ;
            if( SDB_OK != rc )
            {
               errMsg = "Field _options must be obj" ;
               goto error ;
            }
         }
      }
      // Get _skip field
      if( value.isFieldExist( SPT_QUERY_SKIP_FIELD ) )
      {
         rc = value.getIntField( SPT_QUERY_SKIP_FIELD, numToSkip,
                                 SPT_CVT_FLAGS_FROM_INT |
                                 SPT_CVT_FLAGS_FROM_DOUBLE ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Field _skip must be number" ;
         }
      }
      // Get _limit field
      if( value.isFieldExist( SPT_QUERY_LIMIT_FIELD ) )
      {
         rc = value.getIntField( SPT_QUERY_LIMIT_FIELD, numToRet,
                                 SPT_CVT_FLAGS_FROM_INT |
                                 SPT_CVT_FLAGS_FROM_DOUBLE ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Field _limit must be number" ;
         }
      }
      // Get _flags field
      if( value.isFieldExist( SPT_QUERY_FLAGS_FIELD ) )
      {
         rc = value.getIntField( SPT_QUERY_FLAGS_FIELD, flags,
                                 SPT_CVT_FLAGS_FROM_INT ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Field _flags must be number" ;
         }
      }
      rc = pCL->query( cond, sel, sort, hint, options, numToSkip, numToRet,
                       flags, pCursor ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to convert query to cursor obj" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
