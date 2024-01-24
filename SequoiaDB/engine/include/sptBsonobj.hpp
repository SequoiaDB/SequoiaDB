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

   Source File Name = sptBsonobj.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_BSONOBJ_HPP_
#define SPT_BSONOBJ_HPP_

#include "sptApi.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   /*
      _sptBsonobj define
   */
   class _sptBsonobj : public SDBObject
   {
   JS_DECLARE_CLASS( _sptBsonobj )

   public:
      _sptBsonobj() ;
      _sptBsonobj( const bson::BSONObj &obj ) ;
      virtual ~_sptBsonobj() ;

      bson::BSONObj getBson() { return _obj ; }

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 toJson( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 destruct() ;

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;

      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;

      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
   private:
      bson::BSONObj _obj ;
   } ;
   typedef _sptBsonobj sptBsonobj ;

}

#endif // SPT_BSONOBJ_HPP_

