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

   Source File Name = sptDBCursor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBCursor.hpp"
using namespace sdbclient ;
namespace engine
{
   #define SPT_CURSOR_NAME    "SdbCursor"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBCursor, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBCursor, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBCursor, close )
   JS_MEMBER_FUNC_DEFINE( _sptDBCursor, next )
   JS_MEMBER_FUNC_DEFINE( _sptDBCursor, current )
   JS_RESOLVE_FUNC_DEFINE( _sptDBCursor, resolve )

   JS_BEGIN_MAPPING( _sptDBCursor, SPT_CURSOR_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "close", close )
      JS_ADD_MEMBER_FUNC( "next", next )
      JS_ADD_MEMBER_FUNC( "current", current )
      JS_ADD_RESOLVE_FUNC( resolve )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBCursor::cvtToBSON )
      JS_SET_JSOBJ_TO_CURSOR_FUNC( _sptDBCursor::fmpToCursor )
   JS_MAPPING_END()

   _sptDBCursor::_sptDBCursor( _sdbCursor *pCursor ):
      _hasRead( FALSE ), _finishRead( FALSE )
   {
      this->_cursor.pCursor = pCursor ;
   }

   _sptDBCursor::~_sptDBCursor()
   {
   }

   INT32 _sptDBCursor::construct( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR <<
                     "use of new SdbCursor() is forbidden, you should use "
                     "other functions to produce a SdbCursor object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBCursor::destruct()
   {
      return _cursor.close() ;
   }

   INT32 _sptDBCursor::close( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _cursor.close() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to close cursor" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCursor::next( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj record ;
      if( _finishRead )
      {
         goto done ;
      }
      rc = _cursor.next( record ) ;
      if( SDB_OK == rc )
      {
         _hasRead = TRUE ;
         rval.getReturnVal().setValue( record ) ;
      }
      else if( SDB_DMS_EOC == rc )
      {
         engine::sdbSetReadData( _hasRead ) ;
         _finishRead = TRUE ;
         rc = SDB_OK ;
      }
      else
      {
         detail = BSON( SPT_ERR << "Failed to get next" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCursor::current( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj record ;
      if( _finishRead )
      {
         goto done ;
      }
      rc = _cursor.current( record ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get current" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( record ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCursor::resolve( const _sptArguments &arg,
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

   INT32 _sptDBCursor::cvtToBSON( const CHAR* key, const sptObject &value,
                                  BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                  string &errMsg )
   {
      errMsg = "SdbCursor can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBCursor::fmpToCursor( const sptObject &value, _sdbCursor** pCursor,
                                    string &errMsg )
   {
      INT32 rc = SDB_OK ;
      _sptDBCursor *pSptCursor = NULL ;
      rc = value.getUserObj( _sptDBCursor::__desc, (const void**)&pSptCursor ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cursor obj" ;
         goto error ;
      }
      *pCursor = pSptCursor->getCursor() ;
   done:
      return rc ;
   error:
      goto done ;
   }
}
