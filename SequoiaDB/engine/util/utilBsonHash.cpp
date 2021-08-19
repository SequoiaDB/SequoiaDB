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

   Source File Name = utilBsonHash.cpp

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
#include "pdTrace.hpp"
#include "utilTrace.hpp"

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
   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBSONHASHER_HASHOBJ, "_utilBSONHasher::hashObj" )
   UINT32 _utilBSONHasher::hashObj( const bson::BSONObj &obj,
                                    UINT32 partitionBit )
   {
      PD_TRACE_ENTRY( SDB__UTILBSONHASHER_HASHOBJ ) ;
      UINT32 hashCode = 0 ;
      BSONObjIterator i( obj ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         HASH_COMBINE( hashCode, hashElement( e ) ) ;
      }

      if ( 0 < partitionBit && partitionBit < 32 )
      {
         hashCode >>= 32 - partitionBit ;
      }

      PD_PACK_UINT( hashCode ) ;
      PD_TRACE_EXIT( SDB__UTILBSONHASHER_HASHOBJ ) ;
      return hashCode ;      
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBSONHASHER_HASHELE, "_utilBSONHasher::hashElement" )
   UINT32 _utilBSONHasher::hashElement( const bson::BSONElement &e )
   {
      PD_TRACE_ENTRY( SDB__UTILBSONHASHER_HASHELE ) ;
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
      {
         hashCode = hashFLoat64( hashCode, e.Number() ) ;
         break ;
      }

      case NumberDecimal:
      {
         hashCode = hashDecimal( hashCode, e.numberDecimal() ) ;
         break ;
      }

      case jstOID:
         HASH_COMBINE( hashCode, hashOid( e.OID() ) ) ;
         break ;

      case Code:
      case Symbol:
      case String:
         HASH_COMBINE( hashCode, hash( e.valuestrsafe(),
                                       e.valuestrsize() - 1 ) ) ;
         break ;

      case Object:
      case Array:
         HASH_COMBINE( hashCode, hashObj( e.embeddedObject() ) ) ;
         break ;

      case DBRef:
      case BinData:
         HASH_COMBINE( hashCode, hash( e.value(),
                                       e.valuesize() ) ) ;
         break ;

      case RegEx:
         HASH_COMBINE( hashCode, hashStr( e.regex() ) ) ;
         HASH_COMBINE( hashCode, hashStr( e.regexFlags() ) ) ;
         break ;

      case CodeWScope:
         HASH_COMBINE( hashCode, hashStr( e.codeWScopeCode() ) ) ;
         HASH_COMBINE( hashCode, hashObj( e.codeWScopeObject() ) ) ;
         break ;
      }

      PD_PACK_UINT( hashCode ) ;
      PD_TRACE_EXIT( SDB__UTILBSONHASHER_HASHELE ) ;
      return hashCode ;
   }

   UINT32 _utilBSONHasher::hashOid( const bson::OID &oid )
   {
      return hash( ( const CHAR * )( oid.getData() ), sizeof( oid ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBSONHASHER_HASH, "_utilBSONHasher::hash" )
   UINT32 _utilBSONHasher::hash( const void *v, UINT32 size )
   {
      PD_TRACE_ENTRY( SDB__UTILBSONHASHER_HASH ) ;
      UINT32 hashCode = 0 ;
      md5::md5digest digest ;
      md5::md5( v, size, digest ) ;
      hashCode = digest[3] ;
      hashCode |= ((UINT32)digest[2]) << 8 ;
      hashCode |= ((UINT32)digest[1]) << 16 ;
      hashCode |= ((UINT32)digest[0]) << 24 ;
      PD_PACK_UINT( hashCode ) ;
      PD_TRACE_EXIT( SDB__UTILBSONHASHER_HASH ) ;
      return hashCode ;
   }

   UINT32 _utilBSONHasher::hashStr( const CHAR *str )
   {
      return hash( str, ossStrlen( str ) ) ;
   }

   UINT32 _utilBSONHasher::hashFLoat64( UINT32 hashCode, FLOAT64 dv )
   {
      UINT64 uv  = dv ;
      UINT32 afp = ( dv - uv ) * 1000000 ;
      UINT32 h1  = 0 ;
      UINT32 h2  = 0 ;

      h1 = hash( &uv, sizeof( uv ) ) ;
      h2 = hash( &afp, sizeof( afp ) ) ;

      HASH_COMBINE( hashCode, h1 ) ;
      HASH_COMBINE( hashCode, h2 ) ;

      return hashCode ;
   }

   UINT32 _utilBSONHasher::hashDecimal( UINT32 hashCode, 
                                        const bson::bsonDecimal &decimal )
   {
      bsonDecimal maxFloat ;
      bsonDecimal minFloat ;

      maxFloat.fromDouble( numeric_limits<double>::max() ) ;

      minFloat.fromDouble( -numeric_limits<double>::max() ) ;

      if ( decimal.compare( maxFloat ) > 0 || 
           decimal.compare( minFloat ) < 0 )
      {
         INT16 sign          = 0 ;
         INT16 weight        = 0 ;
         INT32 ndigits       = 0 ;
         INT32 i             = 0 ;
         const INT16 *digits = NULL ;

         sign    = decimal.getSign() ;
         weight  = decimal.getWeight() ;
         ndigits = decimal.getNdigit() ;
         digits  = decimal.getDigits() ;

         HASH_COMBINE( hashCode, hash( &sign, sizeof( sign ) ) ) ;
         HASH_COMBINE( hashCode, hash( &weight, sizeof( weight ) ) ) ;
         HASH_COMBINE( hashCode, hash( &ndigits, sizeof( ndigits ) ) ) ;
         for ( i = 0 ; i < ndigits ; i++ )
         {
            HASH_COMBINE( hashCode, hash( &digits[i], sizeof( digits[i] ) ) ) ;
         }
      }
      else
      {
         FLOAT64 dv = 0.0 ;
         decimal.toDouble( &dv ) ;
         hashCode = hashFLoat64( hashCode, dv ) ;
      }

      return hashCode ;
   }

   UINT32 _utilBSONHasher::hashCombine ( UINT32 x, UINT32 y )
   {
      HASH_COMBINE( x, y ) ;
      return x ;
   }
}

