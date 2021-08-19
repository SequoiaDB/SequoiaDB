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

   Source File Name = sptDBCursor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_CURSOR_HPP
#define SPT_DB_CURSOR_HPP
#include "client.hpp"
#include "sptApi.hpp"

namespace engine
{
   class _sptDBCursor : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBCursor )
   public:
      _sptDBCursor( sdbclient::_sdbCursor *pCursor = NULL ) ;
      virtual ~_sptDBCursor() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 next( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 current( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 resolve( const _sptArguments &arg, UINT32 opcode,
                     BOOLEAN &processed, string &callFunc, BOOLEAN &setIDProp,
                     _sptReturnVal &rval, BSONObj &detail ) ;

      sdbclient::_sdbCursor *getCursor()
      {
         return _cursor.pCursor ;
      }

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToCursor( const sptObject &value,
                                sdbclient::_sdbCursor** pCursor,
                                string &errMsg ) ;
   private:
      sdbclient::sdbCursor _cursor ;
      BOOLEAN _hasRead ;
      BOOLEAN _finishRead ;
   };
   typedef _sptDBCursor sptDBCursor ;

   #define SPT_SET_CURSOR_TO_RETURNVAL( pCursor )\
      do\
      {\
         sptDBCursor *__sptCursor__ = SDB_OSS_NEW sptDBCursor( pCursor ) ;\
         if( NULL == __sptCursor__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBCursor" ) ;\
            goto error ;\
         }\
         rc = rval.setUsrObjectVal< sptDBCursor >( __sptCursor__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptCursor__ ) ;\
            pCursor = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
      }while(0)

}
#endif
