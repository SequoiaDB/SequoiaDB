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

   Source File Name = sptDBRecycleBin.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_DB_RECYCLEBIN_HPP
#define SPT_DB_RECYCLEBIN_HPP

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::sdbRecycleBin ;
using sdbclient::_sdbRecycleBin ;

namespace engine
{

   class _sptDBRecycleBin : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBRecycleBin )

   public:
      _sptDBRecycleBin( _sdbRecycleBin *pRecycleBin = NULL ) ;
      ~_sptDBRecycleBin() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 getDetail( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 enable( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 disable( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 setAttributes( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 alter( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 list( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 snapshot( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 count( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 dropItem( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;
      INT32 dropAll( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;
      INT32 returnItem( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;
      INT32 returnItemToName( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;
      static INT32 cvtToBSON( const CHAR *key,
                              const sptObject &value,
                              BOOLEAN isSpecialObj,
                              BSONObjBuilder &builder,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;
   protected:
      sdbRecycleBin _recycleBin ;
   } ;

   typedef class _sptDBRecycleBin sptDBRecycleBin ;
}

#endif // SPT_DB_RECYCLEBIN_HPP
