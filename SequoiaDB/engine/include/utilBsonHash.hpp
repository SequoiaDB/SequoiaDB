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

   Source File Name = utilBsonHash.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/3/2015   YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_BSONHASH_HPP_
#define UTIL_BSONHASH_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _utilBSONHasherObsolete : public SDBObject
   {
   private:
      _utilBSONHasherObsolete() {}
      ~_utilBSONHasherObsolete() {}

   public:
      static UINT32 hash( const bson::BSONObj &obj,
                          UINT32 partitionBit = 0 ) ;

      static UINT32 hash( const bson::BSONElement &e ) ;

      static UINT32 hash( const bson::OID &oid ) ;

      static UINT32 hash( const void *v, UINT32 size ) ;

      static UINT32 hash( const CHAR *str ) ;
   } ;

   typedef class _utilBSONHasherObsolete BSON_HASHER_OBSOLETE ;

   class _utilBSONHasher : public SDBObject
   {
   private:
      _utilBSONHasher() {}
      ~_utilBSONHasher() {}

   public:
      static UINT32 hashObj( const bson::BSONObj &obj,
                             UINT32 partitionBit = 0 ) ;

      static UINT32 hashElement( const bson::BSONElement &e ) ;

      static UINT32 hashOid( const bson::OID &oid ) ;

      static UINT32 hash( const void *v, UINT32 size ) ;

      static UINT32 hashStr( const CHAR *str ) ;

      static UINT32 hashFLoat64( UINT32 hashCode, FLOAT64 value ) ;

      static UINT32 hashDecimal( UINT32 hashCode, 
                                 const bson::bsonDecimal &decimal ) ;

      static UINT32 hashCombine( UINT32 x, UINT32 y ) ;
   } ;

   typedef class _utilBSONHasher BSON_HASHER ;
}

#endif

