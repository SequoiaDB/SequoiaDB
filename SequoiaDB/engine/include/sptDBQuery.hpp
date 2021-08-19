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

   Source File Name = sptDBQuery.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_QUERY_HPP
#define SPT_DB_QUERY_HPP
#include "client.hpp"
#include "sptApi.hpp"
namespace engine
{
   class _sptDBQuery: public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBQuery )
   public:
      _sptDBQuery() ;
      virtual ~_sptDBQuery() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      INT32 resolve( const _sptArguments &arg, UINT32 opcode, BOOLEAN &processed,
                     string &callFunc, BOOLEAN &setIDProp, _sptReturnVal &rval,
                     BSONObj &detail ) ;

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToCursor( const sptObject &value,
                                sdbclient::_sdbCursor** pCursor,
                                string &errMsg ) ;
   private:
      sdbclient::_sdbCollection *_cl ;
   } ;
   typedef _sptDBQuery sptDBQuery ;
}
#endif
