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

   Source File Name = sptDBNumberLong.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_NUMBERLONG_HPP
#define SPT_DB_NUMBERLONG_HPP
#include "sptApi.hpp"

#define SPT_NUMBERLONG_NAME             "NumberLong"
#define SPT_NUMBERLONG_VALUE_FIELD      "_v"
#define SPT_NUMBERLONG_SPECIALOBJ_FIELD "$numberLong"

namespace engine
{
   class _sptDBNumberLong : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBNumberLong )
   public:
      _sptDBNumberLong() ;
      virtual ~_sptDBNumberLong() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;
      INT64 getValue() { return _value ; }
      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;

      static INT32 cvtToString( const sptObject &obj, BOOLEAN isSpecialObj,
                                string &retVal ) ;

      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   private:
      static BOOLEAN _isValidNumberLong( const CHAR *value ) ;
      INT64 _value ;
   } ;
   typedef _sptDBNumberLong sptDBNumberLong ;
}
#endif

