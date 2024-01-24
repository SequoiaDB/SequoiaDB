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

   Source File Name = utilBsonHasherObsolete.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/3/2015   YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilBsonHash.hpp"
#include "ossUtil.hpp"
#include "../bson/lib/md5.hpp"
#include "../bson/lib/md5.h"
#include "pd.hpp"
#include <boost/functional/hash.hpp>

using namespace bson ;

#define HASH_COMBINE( hash, v )\
        do\
        {\
           size_t __h = ( hash ) ;\
           boost::hash_combine( __h, (v) ) ;\
           (hash) = __h ;\
        } while ( FALSE )

namespace engine
{
   UINT32 _utilBSONHasherObsolete::hash( const bson::BSONObj &obj,
                                 UINT32 partitionBit )
   {
      UINT32 hashCode = 0 ;
      BSONObjIterator i( obj ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         HASH_COMBINE( hashCode, hash( e ) ) ;
      }

      if ( 0 < partitionBit )
      {
         UINT32 tmpValue = 1 << partitionBit ;
         tmpValue -= 1 ;
         hashCode &= tmpValue ;
      }
      return hashCode ;
   }

   UINT32 _utilBSONHasherObsolete::hash( const bson::BSONElement &e )
   {
      UINT32 hashCode = 0 ;
      HASH_COMBINE( hashCode, e.canonicalType() ) ;

      switch( e.type() )
      {
      case EOO:
      case Undefined:
      case jstNULL:
      case MaxKey:
      case MinKey:
         break ;

      case Bool:
         HASH_COMBINE( hashCode, e.boolean() ) ;
         break ;

      case Timestamp:
         {
         UINT64 v = e._opTime().asDate() ;
         UINT32 h = hash( &v, sizeof( v ) ) ;
         HASH_COMBINE( hashCode, h ) ;
         }
         break ;

      case Date:
         {
         UINT64 v = e.date().millis ;
         UINT32 h = hash( &v, sizeof( v ) ) ;
         HASH_COMBINE( hashCode, h ) ;
         }
         break ;

      case NumberDouble:
      case NumberLong:
      case NumberInt:
      case NumberDecimal:
      {
         FLOAT64 dv = e.Number() ;
         UINT64 uv = dv ;
         UINT32 afp = ( dv - uv ) * 1000000 ;
         UINT32 h1 = 0 ;
         UINT32 h2 = 0 ;

         h1 = hash( &uv, sizeof( uv ) ) ;
         h2 = hash( &afp, sizeof( afp ) ) ;

         HASH_COMBINE( hashCode, h1 ) ;
         HASH_COMBINE( hashCode, h2 ) ;
         break ;
      }

      case jstOID:
         HASH_COMBINE( hashCode, hash( e.OID() ) ) ;
         break ;

      case Code:
      case Symbol:
      case String:
         HASH_COMBINE( hashCode, hash( e.valuestrsafe(),
                                       e.valuestrsize() - 1 ) ) ;
         break ;

      case Object:
      case Array:
         HASH_COMBINE( hashCode, hash( e.embeddedObject() ) ) ;
         break ;

      case DBRef:
      case BinData:
         HASH_COMBINE( hashCode, hash( e.value(),
                                       e.valuesize() ) ) ;
         break ;

      case RegEx:
         HASH_COMBINE( hashCode, hash( e.regex() ) ) ;
         HASH_COMBINE( hashCode, hash( e.regexFlags() ) ) ;
         break ;

      case CodeWScope:
         HASH_COMBINE( hashCode, hash( e.codeWScopeCode() ) ) ;
         HASH_COMBINE( hashCode, hash( e.codeWScopeObject() ) ) ;
         break ;
      }
      return hashCode ;
   }

   UINT32 _utilBSONHasherObsolete::hash( const bson::OID &oid )
   {
      return hash( ( const CHAR * )( oid.getData() ), sizeof( oid ) ) ;
   }

   UINT32 _utilBSONHasherObsolete::hash( const void *v, UINT32 size )
   {
      UINT32 hashCode = 0 ;
      UINT32 i = 0 ;
      md5::md5digest digest ;
      md5::md5( v, size, digest ) ;
      while ( i < 4 )
      {
         hashCode |= ( (UINT32)digest[i] << ( 32 - 8 * i ) ) ;
         ++i ;
      }
      return hashCode ;
   }

   UINT32 _utilBSONHasherObsolete::hash( const CHAR *str )
   {
      return hash( str, ossStrlen( str ) ) ;
   }
}

