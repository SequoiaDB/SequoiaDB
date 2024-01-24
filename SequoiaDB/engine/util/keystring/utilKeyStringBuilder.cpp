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

   Source File Name = utilKeyStringBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/11/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "keystring/utilKeyStringBuilder.hpp"
#include "pd.hpp"
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>

using namespace std ;
using namespace bson ;

namespace engine
{
namespace keystring
{

   constexpr FLOAT64 _pow256[] = {
       1.0,                                            // 2**0
       1.0 * 256,                                      // 2**8
       1.0 * 256 * 256,                                // 2**16
       1.0 * 256 * 256 * 256,                          // 2**24
       1.0 * 256 * 256 * 256 * 256,                    // 2**32
       1.0 * 256 * 256 * 256 * 256 * 256,              // 2**40
       1.0 * 256 * 256 * 256 * 256 * 256 * 256,        // 2**48
       1.0 * 256 * 256 * 256 * 256 * 256 * 256 * 256   // 2**56
   } ;

   // First FLOAT64 that is not an int64
   constexpr FLOAT64 _minLargeFloat64 = 1ULL << 63 ;

   // Integers larger than this may not be representable as float64.
   constexpr FLOAT64 _maxIntegerForDouble = 1ULL << 53 ;

   INT32 _countLeadingZeros64( UINT64 num )
   {
      int highbit = 0 ;
      for ( UINT32 i = 1 ; i <= 63 ; ++ i )
      {
         if ( num >= ( 1ULL << i ) )
         {
            highbit = i ;
         }
         else
         {
            break ;
         }
      }
      return 64 - highbit - 1 ;
   }

   keyStringEncodedType _bsonTypeToSupertype( BSONType type )
   {
      switch ( type )
      {
      case MinKey :
         return keyStringEncodedType::minKey ;

      case EOO :
      case jstNULL :
         return keyStringEncodedType::nullish ;

      case Undefined :
         return keyStringEncodedType::undefined ;

      case NumberDecimal :
      case NumberDouble :
      case NumberInt :
      case NumberLong :
         return keyStringEncodedType::numeric ;

      case String :
      case Symbol :
         return keyStringEncodedType::stringLike ;

      case Object :
         return keyStringEncodedType::object ;
      case Array :
         return keyStringEncodedType::array ;
      case BinData :
         return keyStringEncodedType::binData ;
      case jstOID :
         return keyStringEncodedType::oid ;
      case Bool :
         return keyStringEncodedType::boolean ;
      case Date :
      case Timestamp :
         return keyStringEncodedType::time ;
      case RegEx :
         return keyStringEncodedType::regEx ;
      case DBRef :
         return keyStringEncodedType::dbRef ;

      case Code :
         return keyStringEncodedType::code ;
      case CodeWScope :
         return keyStringEncodedType::codeWithScope ;

      case MaxKey :
         return keyStringEncodedType::maxKey ;
      default :
         SDB_ASSERT( FALSE, "Unexpected bson type" ) ;
         return keyStringEncodedType::numeric ;
      }
   }

   /*
      _typeBitsBuilder implement
    */
   _typeBitsBuilder::~_typeBitsBuilder()
   {
      reset() ;
   }

   void _typeBitsBuilder::reset()
   {
      _curBit = 0 ;
      if ( nullptr != _buf )
      {
         SDB_THREAD_FREE( _buf ) ;
         _buf = nullptr ;
      }
      _bufSize = 0 ;
      _capacity = 0 ;
   }

   INT32 _typeBitsBuilder::appendBit( UINT8 oneOrZero )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( oneOrZero == 0 || oneOrZero == 1,
                  "Bit to append must be 1 or 0" ) ;
      const UINT32 byte = _curBit / 8 ;
      const UINT8 offsetInByte = _curBit % 8 ;
      if ( offsetInByte == 0 )
      {
         rc = _ensureBytes( 1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to skip byte, rc: %d", rc ) ;
            goto error ;
         }
         _buf[ byte ] = oneOrZero << 7 ; // 0b10000000 or 0b00000000
         _bufSize += 1 ;
      }
      else
      {
         _buf[ byte ] |= ( oneOrZero << ( 7 - offsetInByte ) ) ;
      }
      _curBit ++ ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _typeBitsBuilder::appendString()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::STRING ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendSymbol()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::SYMBOL ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendDate()
   {
      INT32 rc = SDB_OK ;
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DATE ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DATE ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", NumberInt, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendTimestamp()
   {
      INT32 rc = SDB_OK ;
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::TIMESTAMP ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::TIMESTAMP ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", NumberInt, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendNumberDouble()
   {
      INT32 rc = SDB_OK ;
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DOUBLE ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DOUBLE ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", NumberInt, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendNumberInt()
   {
      INT32 rc = SDB_OK ;
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::INT ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::INT ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendNumberLong()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::LONG ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::LONG ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendNumberDecimal()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DECIMAL ) >> 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::DECIMAL ) & 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendPositiveZero()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::POSITIVE_ZERO ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendNegativeZero()
   {
      INT32 rc = SDB_OK ;

      rc = appendBit( static_cast<UINT8>( keyStringTypeBitsType::NEGATIVE_ZERO ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::_ensureBytes( UINT32 length )
   {
      INT32 rc = SDB_OK ;
      UINT32 needSize = _capacity + length ;

      if ( nullptr == _buf )
      {
         _buf = static_cast<CHAR *>( SDB_THREAD_ALLOC( needSize ) ) ;
         if ( nullptr == _buf )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "out of memory" ) ;
            goto error ;
         }
         _capacity = needSize ;
      }
      else if ( ( _capacity - _bufSize ) < length )
      {
         CHAR *ptr =
               static_cast<CHAR *>( SDB_THREAD_REALLOC( _buf, needSize ) ) ;
         if ( nullptr == ptr )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "out of memory" ) ;
            goto error ;
         }

         _buf = ptr ;
         _capacity = needSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _typeBitsBuilder::appendBits( const UINT8 *bytes,
                                       const UINT32 bytesSize )
   {
      INT32 rc = SDB_OK ;
      for ( UINT32 i = 0 ; i < bytesSize * 8 ; i ++ )
      {
         UINT8 byte = static_cast<UINT8>( bytes[ i / 8 ] ) ;
         UINT8 oneOrZero = ( byte >> ( 7 - ( i % 8 ) ) ) & 0b00000001 ;
         rc = appendBit( oneOrZero ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bit to type bits, rc: %d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _typeBitsBuilder::appendDecimalMeta( const bsonDecimal &dec )
   {
      INT32 rc = SDB_OK ;

      INT32 typemod = dec.getTypemod() ;
      INT16 ndigit = dec.getNdigit() ;
      INT16 dscale = dec.getDScale() ;

      rc = appendBits( reinterpret_cast<const UINT8 *>( &typemod ), sizeof( typemod ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append decimal typemod to type bits, rc: %d", rc ) ;

      rc = appendBits( reinterpret_cast<const UINT8 *>( &ndigit ), sizeof( ndigit ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append decimal ndigit to type bits, rc: %d", rc ) ;

      rc = appendBits( reinterpret_cast<const UINT8 *>( &dscale ), sizeof( dscale ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append decimal dscale to type bits, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   /*
      _keyStringBuilderImpl implement
    */
   _keyStringBuilderImpl::_keyStringBuilderImpl( utilStreamAllocator &allocator )
   : _allocator( allocator )
   {
   }

   _keyStringBuilderImpl::~_keyStringBuilderImpl()
   {
      reset() ;
   }

   void _keyStringBuilderImpl::reset()
   {
      _status = keyStringBuilderStatus::EMPTY ;
      _typeBits.reset() ;
      if ( nullptr != _buf )
      {
         _allocator.free( _buf ) ;
         _buf = nullptr ;
      }
      _bufSize = 0 ;
      _capacity = 0 ;
      _sizeAheadElements = 0 ;
      _sizeOfElements = 0 ;
      _sizeAfterElements = 0 ;
      _typeBitsSize = 0 ;
   }

   INT32 _keyStringBuilderImpl::_appendBytes( const void *source,
                                              UINT32 len,
                                              BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;

      if ( nullptr == source )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _ensureBytes( len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to ensure bytes, rc: %d", rc ) ;
         goto error ;
      }

      if ( invert )
      {
         ossMemcpyFlipBits( _buf + _bufSize, source, len ) ;
      }
      else
      {
         ossMemcpy( _buf + _bufSize, source, len ) ;
      }
      _bufSize += len ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_ensureBytes( UINT32 length )
   {
      INT32 rc = SDB_OK ;
      UINT32 needSize = _bufSize + length ;

      if ( nullptr == _buf )
      {
         UINT32 fastAllocSize = _allocator.getFastAllocSize() ;
         if ( 0 == fastAllocSize || fastAllocSize < needSize )
         {
            UINT32 alignSize = ossAlign4( needSize ) ;
            _buf = static_cast<CHAR*>( _allocator.malloc( alignSize ) ) ;
            if ( nullptr == _buf )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "out of memory" ) ;
               goto error ;
            }
            _capacity = alignSize ;
         }
         else
         {
            _buf = static_cast<CHAR*>( _allocator.malloc( fastAllocSize ) ) ;
            if ( nullptr == _buf )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "out of memory" ) ;
               goto error ;
            }
            _capacity = fastAllocSize ;
         }
      }
      else if ( _capacity < needSize )
      {
         UINT32 newCapacity = 2 * _capacity ;
         while ( newCapacity < needSize )
         {
            newCapacity *= 2 ;
         }

         CHAR *ptr = static_cast<CHAR*>( _allocator.realloc( _buf,
                                                             newCapacity ) ) ;
         if ( nullptr == ptr )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "out of memory" ) ;
            goto error ;
         }

         _buf = ptr ;
         _capacity = newCapacity ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   void _keyStringBuilderImpl::_verifyStatus()
   {
      SDB_ASSERT(
            _status == keyStringBuilderStatus::EMPTY ||
            _status == keyStringBuilderStatus::BEFORE_ELEMENTS ||
            _status == keyStringBuilderStatus::APPENDING_ELEMENTS,
            "Unexpected appending state" ) ;
      if ( _status == keyStringBuilderStatus::EMPTY )
      {
         _sizeAheadElements = 0 ;
         _transition( keyStringBuilderStatus::APPENDING_ELEMENTS ) ;
      }
      else if ( _status == keyStringBuilderStatus::BEFORE_ELEMENTS )
      {
         _sizeAheadElements = _bufSize ;
         _transition( keyStringBuilderStatus::APPENDING_ELEMENTS ) ;
      }
   }

   void _keyStringBuilderImpl::_transition( keyStringBuilderStatus to )
   {
      {
         if ( to == keyStringBuilderStatus::EMPTY )
         {
            _status = to ;
            return ;
         }
         if ( _status == to )
         {
            return ;
         }

         switch ( _status )
         {
         case keyStringBuilderStatus::EMPTY :
            SDB_ASSERT(
                  to == keyStringBuilderStatus::BEFORE_ELEMENTS ||
                  to == keyStringBuilderStatus::APPENDING_ELEMENTS ||
                  to == keyStringBuilderStatus::DONE,
                  "Invalid builder status" ) ;
            break ;
         case keyStringBuilderStatus::BEFORE_ELEMENTS :
            SDB_ASSERT(
                  to == keyStringBuilderStatus::APPENDING_ELEMENTS ||
                  to == keyStringBuilderStatus::DONE,
                  "Invalid builder status" ) ;
            break ;
         case keyStringBuilderStatus::APPENDING_ELEMENTS :
            SDB_ASSERT(
                  to == keyStringBuilderStatus::AFTER_ELEMENTS ||
                  to == keyStringBuilderStatus::DONE,
                  "Invalid builder status" ) ;
            break ;
         case keyStringBuilderStatus::AFTER_ELEMENTS :
            SDB_ASSERT( to == keyStringBuilderStatus::DONE,
                        "Invalid builder status" ) ;
            break ;
         default :
            SDB_ASSERT( FALSE, "Invalid builder status" ) ;
         } // switch (_status)
         _status = to ;
      }
   }

   INT32 _keyStringBuilderImpl::appendAllElements( const BSONObj &obj,
                                                   const Ordering &o,
                                                   keyStringDiscriminator d,
                                                   BOOLEAN *isAllUndefined )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator it( obj ) ;
      UINT32 elemCount = 0, undefinedCount = 0 ;
      if ( obj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;

      while ( it.more() )
      {
         auto elem = it.next() ;
         BOOLEAN invert = o.get( elemCount ) == -1 ;
         rc = appendBSONElement( elem, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson elements, rc: %d", rc ) ;
            goto error ;
         }
         if ( elem.type() == Undefined )
         {
            undefinedCount ++ ;
         }
         elemCount += 1 ;
      }
      if ( elemCount > o.getNKeys() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed append bson elements, rc: %d", rc ) ;
         goto error ;
      }
      if ( nullptr != isAllUndefined )
      {
         *isAllUndefined = ( elemCount == undefinedCount ) ;
      }
      rc = _appendDiscriminator( d ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
         goto error ;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements ;


   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::appendBSONElement( const BSONElement &elem,
                                                   BOOLEAN isDescending )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == elem.size() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _appendBsonValue( elem, nullptr, isDescending ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bson element, rc: %d", rc ) ;
         goto error ;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements ;

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendBsonValue( const BSONElement &elem,
                                                  const StringData *name,
                                                  BOOLEAN invert,
                                                  const stringTransformFn &f )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 0 != elem.size(), "can not be zero" ) ;

      if ( name )
      {
         rc = _appendBytes( name->data(), name->size() + 1, invert ) ; // + 1 for NUL
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson name, rc: %d", rc ) ;
            goto error ;
         }
      }

      switch ( elem.type() )
      {
      case MinKey :
      case MaxKey :
      case EOO :
      case Undefined :
      case jstNULL :
      {
         rc = _append( _bsonTypeToSupertype( elem.type() ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
         }
         break ;
      }

      case NumberDouble :
      {
         rc = _appendNumberDouble( elem._numberDouble(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case String :
      {
         rc = _appendString( elem.String(), invert, f ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Object :
      {
         rc = _appendObject( elem.Obj(), invert, f ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Array :
      {
         rc = _appendArray( BSONArray( elem.Obj() ), invert, f ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case BinData :
      {
         INT32 len ;
         const CHAR *data = elem.binData( len ) ;
         rc = _appendBinData( data, len, elem.binDataType(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case jstOID :
      {
         rc = _appendOID( elem.__oid(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Bool :
      {
         rc = _appendBool( elem.boolean(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Date :
      {
         rc = _appendDate( elem.date(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case RegEx :
      {
         rc = _appendRegex( elem.regex(), elem.regexFlags(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case DBRef :
      {
         rc = _appendDBRef(
               { elem.dbrefNS(), static_cast<UINT32>( elem.valuestrsize()
                     - 1 ) },
               elem.dbrefOID(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Symbol :
      {
         rc = _appendSymbol(
               { elem.valuestr(), static_cast<UINT32>( elem.valuestrsize()
                     - 1 ) },
               invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Code :
      {
         rc = _appendCode( elem.code(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case CodeWScope :
      {
         rc = _appendCodeWScope( elem.codeWScopeCode(),
                                 elem.codeWScopeObject(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case NumberInt :
      {
         rc = _appendNumberInt( elem._numberInt(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case Timestamp :
      {
         rc = _appendTimestamp( elem, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case NumberLong :
      {
         rc = _appendNumberLong( elem._numberLong(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      case NumberDecimal :
      {
         rc = _appendNumberDecimal( elem.Decimal(), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element[%d], rc: %d",
                    elem.type(), rc ) ;
            goto error ;
         }
         break ;
      }

      default :
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendNumberInt( const INT32 num,
                                                  BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      rc = _typeBits.appendNumberInt() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append type bits, rc: %d", rc ) ;
         goto error ;
      }
      rc = _appendInteger( num, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append number int, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendNumberLong( const INT64 num,
                                                   BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      rc = _typeBits.appendNumberLong() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append type bits, rc: %d", rc ) ;
         goto error ;
      }
      rc = _appendInteger( num, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append number long, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendInteger( const INT64 num,
                                                BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      const BOOLEAN isNegative = num < 0 ;
      const UINT64 magnitude = isNegative ? -num : num ;
      if ( num == numeric_limits<INT64>::min() )
      {
         FLOAT64 doubleVal = static_cast<FLOAT64>( num ) ;
         SDB_ASSERT( -doubleVal == _minLargeFloat64, "must be equal" ) ;
         rc = _appendLargeDouble( doubleVal,
                                  keyStringContinuousMarker::hasNoContinuation,
                                  invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append large double, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
      else if ( num == 0 )
      {
         rc = _append( keyStringEncodedType::numericZero, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
      rc = _appendPreshiftedInteger( magnitude << 1, isNegative, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append integer, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendPreshiftedInteger( UINT64 value,
                                                          BOOLEAN isNegative,
                                                          BOOLEAN invert )
   {
      SDB_ASSERT( value != 0ULL, "Unexpected value" ) ;
      SDB_ASSERT( value != 1ULL, "Unexpected value" ) ;

      INT32 rc = SDB_OK ;
      const UINT32 bytesNeeded = ( 64 - _countLeadingZeros64( value ) + 7 ) / 8 ;

      // Append the low bytes of value in big endian order.
      value = ossNativeToBigEndian( value ) ;
      const void *firstUsedByte =
                  reinterpret_cast<const char*>( ( &value ) + 1 ) - bytesNeeded ;

      if ( isNegative )
      {
         rc = _append( static_cast<UINT8>(
                  static_cast<UINT8>( keyStringEncodedType::numericNegative1ByteInt ) -
                  ( bytesNeeded - 1 ) ),
               invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
            goto error ;
         }
         rc = _appendBytes( firstUsedByte, bytesNeeded, !invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bytes, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _append( static_cast<UINT8>(
                  static_cast<UINT8>( keyStringEncodedType::numericPositive1ByteInt ) +
                  ( bytesNeeded - 1 ) ),
               invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
            goto error ;
         }
         rc = _appendBytes( firstUsedByte, bytesNeeded, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bytes, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendNumberDouble( const FLOAT64 num,
                                                     BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      rc = _typeBits.appendNumberDouble() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to append number FLOAT64 to type bits, rc: %d", rc ) ;
         goto error ;
      }
      if ( 0.0 == num )
      {
         signbit( num ) ?
               _typeBits.appendNegativeZero() :
               _typeBits.appendPositiveZero() ;
      }
      rc = _appendDoubleWithoutTypeBits(
            num, keyStringContinuousMarker::hasNoContinuation, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append double, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendDoubleWithoutTypeBits( FLOAT64 num,
                                                              keyStringContinuousMarker dcm,
                                                              BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      const BOOLEAN isNegative = num < 0.0 ;
      const FLOAT64 magnitude = isNegative ? -num : num ;

      if ( !( magnitude >= 1.0 ) )
      {
         if ( magnitude > 0.0 )
         {
            rc = _appendSmallDouble( num, dcm, invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append small double, rc: %d", rc ) ;
               goto error ;
            }
         }
         else if ( num == 0.0 )
         {
            rc = _append( keyStringEncodedType::numericZero, invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( std::isnan( num ), "value must be NaN" ) ;
            rc = _append( keyStringEncodedType::numericNaN, invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
               goto error ;
            }
         }
         goto done ;
      }
      if ( magnitude >= _minLargeFloat64 )
      {
         _appendLargeDouble( num, dcm, invert ) ;
         goto done ;
      }
      else
      {
         UINT64 integerPart = static_cast<UINT64>( magnitude ) ;
         if ( static_cast<FLOAT64>( integerPart ) == magnitude && dcm
               == keyStringContinuousMarker::hasNoContinuation )
         {
            // No fractional part
            rc = _appendPreshiftedInteger( integerPart << 1, isNegative,
                                           invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append preshifted integer, rc: %d", rc ) ;
               goto error ;
            }
            goto done ;
         }

         const UINT32 fractionalBytes = _countLeadingZeros64( integerPart << 1 ) / 8 ;
         if ( magnitude >= _maxIntegerForDouble && fractionalBytes == 0 )
         {
            _appendPreshiftedInteger(
                  ( integerPart << 1 ) | static_cast<UINT8>( dcm ),
                  isNegative, invert ) ;
         }
         else
         {
            const UINT8 type =
                  isNegative ?
                        static_cast<UINT8>( keyStringEncodedType::numericNegative8ByteInt ) + fractionalBytes :
                        static_cast<UINT8>( keyStringEncodedType::numericPositive8ByteInt ) - fractionalBytes ;
            rc = _append( type, invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
               goto error ;
            }
            UINT64 encoding = static_cast<UINT64>( magnitude
                  * _pow256[ fractionalBytes ] ) ;
            SDB_ASSERT( encoding == magnitude * _pow256[ fractionalBytes ],
                        "must be equal" ) ;
            encoding += ( integerPart + 1 ) << ( fractionalBytes * 8 ) ;
            SDB_ASSERT( ( encoding & 0x1ULL ) == 0, "must be zero" ) ;
            encoding |= static_cast<UINT8>( dcm ) ;
            encoding = ossNativeToBigEndian( encoding ) ;
            rc = _append( encoding, isNegative ? !invert : invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append FLOAT64 encoding, rc: %d", rc ) ;
               goto error ;
            }
         }
      }


   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendSmallDouble( FLOAT64 value,
                                                    keyStringContinuousMarker dcm,
                                                    BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isNegative = value < 0 ;
      UINT64 encoding = 0 ;
      FLOAT64 magnitude = isNegative ? -value : value ;
      SDB_ASSERT( !std::isnan( value ) && value != 0 && magnitude < 1,
                  "Unexpected value" ) ;

      rc = _append(
            isNegative ?
                  keyStringEncodedType::numericNegativeSmallMagnitude :
                  keyStringEncodedType::numericPositiveSmallMagnitude,
            invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      ossMemcpy( &encoding, &magnitude, sizeof( encoding ) ) ;
      encoding <<= 1 ;
      encoding |= static_cast<UINT8>( dcm ) ;
      rc = _append( ossNativeToBigEndian( encoding ),
                    isNegative ? !invert : invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded bytes, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendLargeDouble( FLOAT64 value,
                                                    keyStringContinuousMarker dcm,
                                                    BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isNegative = value < 0 ;
      SDB_ASSERT( !std::isnan( value ), "Can not be NaN" ) ;
      SDB_ASSERT( value != 0.0, "Can not be zero" ) ;
      rc = _append(
            isNegative ?
                  keyStringEncodedType::numericNegativeLargeMagnitude :
                  keyStringEncodedType::numericPositiveLargeMagnitude,
            invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }
      UINT64 encoding ;
      ossMemcpy( &encoding, &value, sizeof( encoding ) ) ;
      if ( isfinite( value ) )
      {
         encoding <<= 1 ;
         encoding |= static_cast<UINT8>( dcm ) ;
      }
      else
      {
         encoding = ~0ULL ; // infinity
      }
      encoding = ossNativeToBigEndian( encoding ) ;
      rc = _append( encoding, isNegative ? !invert : invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded bytes, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendNumberDecimal( const bsonDecimal &dec,
                                                      BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      INT64 decLong = 0 ;
      bsonDecimal roundDec ;
      bsonDecimal decFromDouble ;
      BOOLEAN isNegative = dec.getSign() == SDB_DECIMAL_NEG ;
      rc = _typeBits.appendNumberDecimal() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to append number decimal to type bits, rc: %d", rc ) ;
         goto error ;
      }
      if ( dec.isZero() )
      {
         rc = _append( keyStringEncodedType::numericZero, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append zero, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
      else if ( dec.isNan() )
      {
         rc = _append( keyStringEncodedType::numericNaN, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append NaN, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
      else if ( dec.isMax() )
      {
         rc = _appendLargeDouble(
               numeric_limits<FLOAT64>::infinity(),
               keyStringContinuousMarker::hasNoContinuation, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append double, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
      else if ( dec.isMin() )
      {
         rc = _appendLargeDouble(
               -numeric_limits<FLOAT64>::infinity(),
               keyStringContinuousMarker::hasNoContinuation, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append double, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      if ( isNegative )
      {
         rc = dec.ceil( roundDec ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get ceil of decimal, rc: %d", rc ) ;
      }
      else
      {
         rc = dec.floor( roundDec ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get floor of decimal, rc: %d", rc ) ;
      }
      rc = roundDec.toLong( &decLong ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get long value of decimal, rc: %d", rc ) ;

      rc = _typeBits.appendDecimalMeta( dec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to append number decimal meta to type bits, rc: %d",
                 rc ) ;
         goto error ;
      }
      if ( decLong != 0 && dec.compareLong( decLong ) == 0 )
      {
         if ( decLong == numeric_limits<INT64>::min() )
         {
            FLOAT64 doubleVal = static_cast<FLOAT64>( decLong ) ;
            SDB_ASSERT( -doubleVal == _minLargeFloat64, "must be equal" ) ;
            rc = _appendLargeDouble( doubleVal,
                                     keyStringContinuousMarker::hasNoContinuation,
                                     invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append large double, rc: %d", rc ) ;
               goto error ;
            }
         }
         else
         {
            UINT64 matitude = isNegative ? -decLong : decLong ;
            rc = _appendPreshiftedInteger( matitude << 1, isNegative, invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append preshift integer, rc: %d", rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         FLOAT64 doubleTowardZero ;
         ossPoolStringStream ss ;
         ss << std::fixed ;
         dec.toDouble( &doubleTowardZero ) ;
         if ( dec.getDScale() >= 0 )
         {
            ss << setprecision( dec.getDScale() ) << doubleTowardZero ;
         }
         else
         {
            ss << setprecision( _doublePrecision10 ) << doubleTowardZero ;
         }
         decFromDouble.fromString( ss.str().c_str() ) ;
         if ( decFromDouble.compare( dec ) == 0 )
         {
            rc = _appendDoubleWithoutTypeBits( doubleTowardZero,
                                               keyStringContinuousMarker::hasNoContinuation,
                                               invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append double, rc: %d", rc ) ;
               goto error ;
            }
         }
         else
         {
            static bsonDecimal _minLargeFloat64AsDecimal( "9223372036854775808" ) ;
            static bsonDecimal _maxIntegerForDoubleAsDecimal( "9007199254740992" ) ;
            bsonDecimal absDec( dec ) ;
            rc = absDec.abs() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get absolute value, rc: %d", rc ) ;

            if ( absDec.compare( _maxIntegerForDoubleAsDecimal ) < 0 ||
                 absDec.compare( _minLargeFloat64AsDecimal ) >= 0 )
            {
               if ( ( decFromDouble.compare( dec ) > 0 && !isNegative ) ||
                    ( decFromDouble.compare( dec ) < 0 && isNegative ) )
               {
                  *(UINT64*)( &doubleTowardZero ) -= 1 ;
               }
               rc = _appendDoubleWithoutTypeBits( doubleTowardZero,
                                                  keyStringContinuousMarker::hasContinuation,
                                                  invert ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append double, rc: %d", rc ) ;
               rc = _appendDecimalEncoding( dec, invert ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append decimal encoding, rc: %d", rc ) ;
            }
            else
            {
               // too large to convert to double
               if ( decLong == numeric_limits<INT64>::min() )
               {
                  FLOAT64 doubleVal = static_cast<FLOAT64>( decLong ) ;
                  SDB_ASSERT( -doubleVal == _minLargeFloat64, "must be equal" ) ;
                  rc = _appendLargeDouble( doubleVal,
                                           keyStringContinuousMarker::hasNoContinuation,
                                           invert ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to append large double, rc: %d", rc ) ;
               }
               else
               {
                  UINT64 matitude = isNegative ? -decLong : decLong ;
                  rc = _appendPreshiftedInteger(
                           ( matitude << 1 |
                             static_cast<UINT8>( keyStringContinuousMarker::hasContinuation ) ),
                           isNegative, invert ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to append preshift integer, rc: %d", rc ) ;
                     goto error ;
                  }
               }
               rc = _appendDecimalEncoding( dec, invert ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append decimal encoding, rc: %d", rc ) ;
            }
         }
      }
   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendDecimalEncoding( const bsonDecimal &dec,
                                                        BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      INT32 weight = dec.getWeight() ;
      UINT32 ndigit = static_cast<UINT32>( dec.getNdigit() ) ;
      const INT16 *digits = dec.getDigits() ;
      UINT32 integerPartNDigit = 0 ;
      UINT32 actualIntegerPartNDigit = 0 ;
      invert = dec.getSign() == SDB_DECIMAL_NEG ? !invert : invert ;
      if ( weight >= 0 )
      {
         integerPartNDigit = weight + 1 ;
         actualIntegerPartNDigit = min(
               static_cast<UINT32>( weight + 1 ), ndigit ) ;
      }
      else
      {
         integerPartNDigit = 0 ;
         actualIntegerPartNDigit = 0 ;
      }
      rc = _append( ossNativeToBigEndian( integerPartNDigit ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append integer part ndigit, rc: %d",
                 rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < actualIntegerPartNDigit ; ++ i )
      {
         rc = _append( ossNativeToBigEndian( digits[ i ] ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append integer part digits, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

      if ( weight < 0 )
      {
         UINT16 fractionPartNdigit = static_cast<UINT16>( -weight ) ;
         rc = _append( ossNativeToBigEndian( fractionPartNdigit ),
                       !invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append fraction part ndigit, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

      for ( UINT32 i = actualIntegerPartNDigit ; i < ndigit ; ++ i )
      {
         UINT16 absDigit = digits[ i ] << 1 ;
         if ( i != ndigit - 1 )
         {
            absDigit |= 0b1 ;
         }
         rc = _append(
               ossNativeToBigEndian( static_cast<UINT16>( absDigit ) ),
               invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append fraction part digits, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendBool( BOOLEAN val,
                                             BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;

      _verifyStatus() ;
      rc = _append(
            val ? keyStringEncodedType::booleanTrue : keyStringEncodedType::booleanFalse,
            invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bool value, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendDate( const Date_t &val,
                                             BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      _verifyStatus() ;
      INT64 seconds = val / 1000 ;
      UINT32 microSeconds = ( val % 1000 ) * 1000 ;
      rc = _typeBits.appendDate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append type bits, rc: %d", rc ) ;
         goto error ;
      }
      rc = _appendTime( seconds, microSeconds, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append time, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendTimestamp( const BSONElement &elem,
                                                  BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      _verifyStatus() ;
      INT64 seconds = (long long)elem.timestampTime() / 1000 ;
      UINT32 microSeconds = elem.timestampInc() ;
      seconds += microSeconds / 1000000 ;
      microSeconds %= 1000000 ;
      rc = _typeBits.appendTimestamp() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append type bits, rc: %d", rc ) ;
         goto error ;
      }
      rc = _appendTime( seconds, microSeconds, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append time, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendTime( INT64 seconds,
                                             UINT32 microseconds,
                                             BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      INT64 encoded = seconds ^ numeric_limits<INT64>::min() ;
      rc = _append( keyStringEncodedType::time, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( ossNativeToBigEndian( encoded ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append timestamp value, rc: %d", rc ) ;
         goto error ;
      }
      rc = _append( ossNativeToBigEndian( microseconds ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append timestamp value, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendOID( const OID &val,
                                            BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::oid, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( val, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append OID value, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendSymbol( const StringData &val,
                                               BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == val.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _typeBits.appendSymbol() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append type bits, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( keyStringEncodedType::stringLike, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _appendStringLike( val, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append symbol data ,rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendCode( const StringData &val,
                                             BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == val.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::code, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( keyStringEncodedType::stringLike, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _appendStringLike( val, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append code data, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendCodeWScope( const StringData &code,
                                                   const BSONObj &scope,
                                                   BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == code.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::codeWithScope, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( keyStringEncodedType::stringLike, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      rc = _appendStringLike( code, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append code value, rc: %d" ) ;
         goto error ;
      }

      rc = _appendBson( scope, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append code scope, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendBinData( const CHAR *data,
                                                UINT32 dataSize,
                                                BinDataType type,
                                                BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == data )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::binData, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      if ( 0xff > dataSize )
      {
         rc = _append( static_cast<UINT8>( dataSize ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append data size, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _append( static_cast<INT8>( 0xff ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bit, rc: %d", rc ) ;
            goto error ;
         }

         rc = _append(
               ossNativeToBigEndian( static_cast<UINT32>( dataSize ) ),
               invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append data size, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = _append( static_cast<UINT8>( type ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bin data type, rc: %d" ) ;
         goto error ;
      }

      if ( dataSize > 0 )
      {
         rc = _appendBytes( data, dataSize, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bin data value, rc: %d" ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendRegex(
         const StringData &regex, const StringData &flags,
         BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == regex.data() || nullptr == flags.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::regEx, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d" ) ;
         goto error ;
      }

      rc = _appendBytes( regex.data(), regex.size(), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append regex value, rc: %d" ) ;
         goto error ;
      }

      rc = _append( static_cast<INT8>( 0 ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d" ) ;
         goto error ;
      }

      rc = _appendBytes( flags.data(), flags.size(), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append flags value, rc: %d" ) ;
         goto error ;
      }

      rc = _append( static_cast<INT8>( 0 ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bit, rc: %d" ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendDBRef( const StringData &dbrefNS,
                                              const OID &dbrefOID,
                                              BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == dbrefNS.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::dbRef, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d" ) ;
         goto error ;
      }

      rc = _append(
            ossNativeToBigEndian( static_cast<UINT32>( dbrefNS.size() ) ),
            invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append dbref namespace size, rc: %d" ) ;
         goto error ;
      }

      rc = _appendBytes( dbrefNS.data(), dbrefNS.size(), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append dbref namespace value, rc: %d" ) ;
         goto error ;
      }

      rc = _append( dbrefOID, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed append dbref oid value, rc: %d" ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendArray( const BSONArray &val,
                                              BOOLEAN invert,
                                              const stringTransformFn &f )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator it ;

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::array, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d" ) ;
         goto error ;
      }

      it = BSONObjIterator( val ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;
         rc = _appendBsonValue( e, nullptr, invert, f ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed append bson element, rc: %d", rc ) ;
            goto error ;
         }
      }
      rc = _append( static_cast<UINT8>( 0 ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append Null, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendString( const StringData &val,
                                               BOOLEAN invert,
                                               const stringTransformFn &f )
   {
      INT32 rc = SDB_OK ;
      if ( nullptr == val.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _verifyStatus() ;
      rc = _typeBits.appendString() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed append type bits, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( keyStringEncodedType::stringLike, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
         goto error ;
      }

      if ( f )
      {
         rc = _appendStringLike( f( val ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append string data, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _appendStringLike( val, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append string data, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendObject( const BSONObj &val,
                                               BOOLEAN invert,
                                               const stringTransformFn &f )
   {
      INT32 rc = SDB_OK ;

      _verifyStatus() ;
      rc = _append( keyStringEncodedType::object, invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append encoded type object, rc: %d",
                 rc ) ;
         goto error ;
      }

      rc = _appendBson( val, invert, f ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append object data, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendStringLike( const StringData &str,
                                                   BOOLEAN invert )
   {
      INT32 rc = SDB_OK ;
      _verifyStatus() ;
      const CHAR *data = str.data() ;
      UINT32 size = str.size() ;

      if ( nullptr == str.data() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      while ( TRUE )
      {
         INT32 firstNul = ::strnlen( data, size ) ;
         if ( -1 == firstNul )
         {
            firstNul = size ;
         }

         rc = _appendBytes( data, static_cast<UINT32>( firstNul ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bytes, rc: %d", rc ) ;
            goto error ;
         }

         if ( firstNul == static_cast<INT32>( size ) || firstNul
               == static_cast<INT32>( string::npos ) )
         {
            rc = _append( static_cast<INT8>( 0 ), invert ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to append bytes, rc: %d", rc ) ;
               goto error ;
            }

            break ;
         }

         rc = _appendBytes( "\x00\xff", 2, invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bytes, rc: %d", rc ) ;
            goto error ;
         }

         data = data + firstNul + 1 ;
         size = size - firstNul - 1 ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendBson( const BSONObj &obj,
                                             BOOLEAN invert,
                                             const stringTransformFn &f )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator it ;
      _verifyStatus() ;

      it = BSONObjIterator( obj ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;
         rc = _append( _bsonTypeToSupertype( e.type() ), invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append encoded type, rc: %d", rc ) ;
            goto error ;
         }

         StringData name( e.fieldName() ) ;
         rc = _appendBsonValue( e, &name, invert, f ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append element value, rc: %d", rc ) ;
            goto error ;
         }
      }
      rc = _append( static_cast<UINT8>( 0 ), invert ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append Null, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendDiscriminator( keyStringDiscriminator d )
   {
      INT32 rc = SDB_OK ;
      if ( keyStringDiscriminator::EXCLUSIVE_BEFORE == d )
      {
         rc = _append( keyStringDiscriminatorValue::LESS, FALSE ) ;
      }
      else if ( keyStringDiscriminator::EXCLUSIVE_AFTER == d )
      {
         rc = _append( keyStringDiscriminatorValue::GREATER, FALSE ) ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append Discriminator, rc: %d", rc ) ;
         goto error ;
      }
      // else Discriminator::Inclusive, No discriminator byte

      // append the end byte in all cases
      rc = _append( keyStringDiscriminatorValue::END, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append End, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // TypeBits size: 0, 1 or 5 bytes
   INT32 _keyStringBuilderImpl::_appendTypeBits()
   {
      INT32 rc = SDB_OK ;
      const UINT32 tbBufSize = _typeBits.getBufSize() ;
      if ( tbBufSize > 0 )
      {
         rc = _appendBytes( _typeBits.getBuf(), tbBufSize, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append typebits buffer, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // MetaBlock
   // key string size: 1 or 5 bytes
   // Size ahead key string: 0, 1 or 5byte
   // metaByte: 1 byte whose 1 bit to indicate existence ahead key string, 1 bit
   // to indicate existence of key string, 1 bit to indicate existence of
   // typebits size.
   // version: 1 byte.
   INT32 _keyStringBuilderImpl::_appendMetaBlock( UINT32 keyHeaderSize,
                                                  UINT32 keyElementsSize,
                                                  UINT32 keyTailSize,
                                                  UINT32 typeBitsSize )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( keyStringBuilderStatus::DONE != _status, "can not be invalid" ) ;
      UINT32 comparableSize = keyHeaderSize + keyElementsSize + keyTailSize ;
      keyStringMetaByte mbyte( keyHeaderSize, keyTailSize, typeBitsSize ) ;

      if ( mbyte.hasTypeBits() )
      {
         rc = _appendMetaBlockSizeWord( typeBitsSize, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append type bits size:%d", rc ) ;
            goto error ;
         }
      }

      if ( mbyte.hasKeyTail() )
      {
         rc = _appendMetaBlockSizeWord( keyTailSize, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append key tail size:%d", rc ) ;
            goto error ;
         }
      }

      if ( mbyte.hasKeyHead() )
      {
         rc = _appendMetaBlockSizeWord( keyHeaderSize, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append key head size:%d", rc ) ;
            goto error ;
         }
      }

      rc = _appendMetaBlockSizeWord( comparableSize, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append full key size:%d", rc ) ;
         goto error ;
      }

      rc = _append( mbyte.getValue(), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append metaByte, rc: %d", rc ) ;
         goto error ;
      }

      rc = _append( getVersion(), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append version, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendMetaBlock()
   {
      INT32 rc = SDB_OK ;
      rc = _appendMetaBlock( _sizeAheadElements, _sizeOfElements,
                             _sizeAfterElements, _typeBitsSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build meta block:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   keyString _keyStringBuilderImpl::getShallowKeyString() const
   {
      SDB_ASSERT( keyStringBuilderStatus::DONE == _status, "can not be invalid" ) ;
      keyString ks ;
      ks._ref.reset( _bufSize, _buf ) ;
      ks._desc.keySize = _sizeAheadElements + _sizeOfElements + _sizeAfterElements ;
      ks._desc.keyHeadSize = _sizeAheadElements ;
      ks._desc.keyTailSize = _sizeAfterElements ;
      ks._desc.typeBitsSize = _typeBitsSize ;
      return ks ;
   }

   keyString _keyStringBuilderImpl::reap()
   {
      SDB_ASSERT( keyStringBuilderStatus::DONE == _status, "can not be invalid" ) ;
      SDB_ASSERT( _allocator.isMovable(), "must be movable" ) ;
      keyString ks ;
      ks._ref.reset( _bufSize, _buf ) ;
      ks._desc.keySize = _sizeAheadElements + _sizeOfElements + _sizeAfterElements ;
      ks._desc.keyHeadSize = _sizeAheadElements ;
      ks._desc.keyTailSize = _sizeAfterElements ;
      ks._desc.typeBitsSize = _typeBitsSize ;
      ks._bufferOwned = _buf ;
      ks._bufferSize = _capacity ;
      _buf = nullptr ;
      reset() ;
      return ks ;
   }

   INT32 _keyStringBuilderImpl::done()
   {
      INT32 rc = SDB_OK ;
      if ( keyStringBuilderStatus::DONE == _status )
      {
         goto done ;
      }
      rc = _appendTypeBits() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append typebits, rc: %d", rc ) ;
         goto error ;
      }

      _typeBitsSize = _typeBits.getBufSize() ;

      rc = _appendMetaBlock() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append meta block, rc: %d", rc ) ;
         goto error ;
      }
      _transition( keyStringBuilderStatus::DONE ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::appendRID( const dmsRecordID &rid, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      if ( !force && !rid.isValid() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( (UINT32)( rid._extent ), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append extent, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( (UINT32)( rid._offset ), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append offset, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::appendIndexID( const dmsIdxMetadataKey &indexID,
                                               BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      if ( !force && !indexID.isValid() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( (UINT64)( indexID.getCLOrigUID() ), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append collection unique ID, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( (UINT32)( indexID.getCLOrigLID() ), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append collection logical ID, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( (UINT32)( indexID.getIdxInnerID() ), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append index inner unique ID, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::appendLSN( UINT64 lsn, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      if ( !force && DPS_INVALID_LSN_OFFSET == lsn )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = appendUnsignedWithoutType( lsn, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append lsn, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildPredicate( const VEC_ELE_CMP &elements,
                                                const Ordering &o,
                                                const VEC_BOOLEAN &im,
                                                BOOLEAN forward,
                                                utilSlice keyHeader )
   {
      INT32 rc = SDB_OK ;

      INT32 mask = 1 ;
      BOOLEAN hasDiscriminator = FALSE ;

      reset() ;

      if ( elements.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( o.getNKeys() < elements.size() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( keyHeader.isValid() )
      {
         rc = _ensureBytes( keyHeader.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
            goto error ;
         }
         ossMemcpy( _buf + _bufSize, keyHeader.data(), keyHeader.size() ) ;
         _bufSize += keyHeader.size() ;
         _transition( keyStringBuilderStatus::BEFORE_ELEMENTS ) ;
      }

      _verifyStatus() ;
      for ( UINT32 i = 0 ; i < elements.size() ; i ++, mask <<= 1 )
      {
         BOOLEAN invert = o.descending( mask ) ;
         rc = appendBSONElement( *elements[ i ], invert ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append bson elements, rc: %d", rc ) ;
            goto error ;
         }
         if ( forward )
         {
            if ( im[ i ] )
            {
               continue ;
            }
            else
            {
               rc = _append( keyStringDiscriminatorValue::GREATER, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
               hasDiscriminator = TRUE ;
               break ;
            }
         }
         else
         {
            if ( im[ i ] )
            {
               continue ;
            }
            else
            {
               rc = _append( keyStringDiscriminatorValue::LESS, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
               hasDiscriminator = TRUE ;
               break ;
            }
         }
      }
      if ( !hasDiscriminator )
      {
         if ( forward )
         {
            rc = _append( keyStringDiscriminatorValue::LESS, FALSE ) ;
         }
         else
         {
            rc = _append( keyStringDiscriminatorValue::GREATER, FALSE ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
         hasDiscriminator = TRUE ;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements ;
      rc = done() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to be done , rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildPredicate( const BSONObj &key,
                                                const Ordering &o,
                                                const dmsRecordID &recordID,
                                                BOOLEAN forward,
                                                keyStringDiscriminator d,
                                                utilSlice keyHeader )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator it( key ) ;
      INT32 mask = 1 ;

      reset() ;

      if ( key.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( keyHeader.isValid() )
      {
         rc = _ensureBytes( keyHeader.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
            goto error ;
         }
         ossMemcpy( _buf + _bufSize, keyHeader.data(), keyHeader.size() ) ;
         _bufSize += keyHeader.size() ;
         _transition( keyStringBuilderStatus::BEFORE_ELEMENTS ) ;
      }

      _verifyStatus() ;
      while ( it.more() )
      {
         auto elem = it.next() ;
         BOOLEAN invert = o.descending( mask ) ;
         rc = appendBSONElement( elem, invert ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append bson elements, rc: %d", rc ) ;

         mask <<= 1 ;
      }
      if ( recordID.isValid() )
      {
         keyStringDiscriminatorValue dv = keyStringDiscriminatorValue::END ;
         dmsRecordID tmpRID ;

         if ( keyStringDiscriminator::INCLUSIVE == d )
         {
            tmpRID = recordID ;
         }
         else
         {
            if ( ( forward != FALSE ) ==
                 ( keyStringDiscriminator::EXCLUSIVE_AFTER == d ) )
            {
               if ( recordID.isMax() )
               {
                  dv = keyStringDiscriminatorValue::GREATER ;
                  tmpRID.reset() ;
               }
               else
               {
                  tmpRID.fromUINT64( recordID.toUINT64() + 1 ) ;
               }
            }
            else
            {
               if ( recordID.isMin() )
               {
                  dv = keyStringDiscriminatorValue::LESS ;
                  tmpRID.reset() ;
               }
               else
               {
                  tmpRID.fromUINT64( recordID.toUINT64() - 1 ) ;
               }
            }
         }

         rc = _append( dv, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;

         _sizeOfElements = _bufSize - _sizeAheadElements ;

         if ( tmpRID.isValid() )
         {
            rc = appendRID( tmpRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append record ID, rc: %d", rc ) ;
         }
      }
      else
      {
         if ( keyStringDiscriminator::INCLUSIVE == d )
         {
            rc = _append( keyStringDiscriminatorValue::END, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
         }
         else
         {
            if ( ( forward != FALSE ) ==
                 ( keyStringDiscriminator::EXCLUSIVE_AFTER == d ) )
            {
               rc = _append( keyStringDiscriminatorValue::GREATER, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
            }
            else
            {
               rc = _append( keyStringDiscriminatorValue::LESS, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
            }
         }
         _sizeOfElements = _bufSize - _sizeAheadElements ;
      }
      rc = done() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to be done , rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildPredicate( const bson::BSONObj &prefixKey,
                                                UINT32 prefixNum,
                                                const bson::Ordering &o,
                                                BOOLEAN forward,
                                                utilSlice keyHeader )
   {
      INT32 rc = SDB_OK ;

      INT32 mask = 1 ;
      UINT32 i = 0 ;
      BSONObjIterator iter( prefixKey ) ;

      reset() ;

      if ( keyHeader.isValid() )
      {
         rc = _ensureBytes( keyHeader.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
            goto error ;
         }
         ossMemcpy( _buf + _bufSize, keyHeader.data(), keyHeader.size() ) ;
         _bufSize += keyHeader.size() ;
         _transition( keyStringBuilderStatus::BEFORE_ELEMENTS ) ;
      }

      _verifyStatus() ;

      while ( i < prefixNum && iter.more() )
      {
         auto elem = iter.next() ;
         BOOLEAN invert = o.descending( mask ) ;
         rc = appendBSONElement( elem, invert ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append bson elements, rc: %d", rc ) ;

         i ++ ;
         mask <<= 1 ;
      }

      if ( forward )
      {
         rc = _append( keyStringDiscriminatorValue::GREATER, FALSE ) ;
      }
      else
      {
         rc = _append( keyStringDiscriminatorValue::LESS, FALSE ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;
      _sizeOfElements = _bufSize - _sizeAheadElements ;
      rc = done() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to be done , rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildIndexEntryKey( const BSONObj &key,
                                                    const Ordering &o,
                                                    const dmsRecordID &rid,
                                                    const dmsIdxMetadataKey *indexid,
                                                    const UINT64 *lsn,
                                                    BOOLEAN *isAllUndefined )
   {
      INT32 rc = SDB_OK ;
      reset() ;
      if ( indexid )
      {
         rc = appendIndexID( *indexid ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append index id, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = appendAllElements( key, o, keyStringDiscriminator::INCLUSIVE, isAllUndefined ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append bson elements, rc: %d", rc ) ;
         goto error ;
      }

      rc = appendRID( rid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append record id, rc: %d", rc ) ;
         goto error ;
      }

      if ( lsn )
      {
         rc = appendLSN( *lsn ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append lsn, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = done() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to be done, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildForKeyStringFunc( const BSONElement &ele,
                                                       INT32 direction )
   {
      INT32 rc = SDB_OK ;

      rc = appendBSONElement( ele, direction > 0 ? FALSE : TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append bson elements, rc: %d", rc ) ;

      rc = _appendDiscriminator( keyStringDiscriminator::INCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append discriminator, rc: %d", rc ) ;

      rc = done() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append done, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::buildBoundaryKey( const dmsIdxMetadataKey &indexID,
                                                  BOOLEAN asUpBound,
                                                  UINT32 bufSize,
                                                  CHAR *buf,
                                                  UINT32 &size )
   {
      INT32 rc = SDB_OK ;

      constexpr UINT32 _MIN_BUF_SIZE =
            _keyStringCoder::INDEX_ID_ENCODEING_SIZE + KEY_STRING_MB_HEADER_SIZE
            + KEY_STRING_TYNI_SWRORD_SIZE + /// key size word
            KEY_STRING_TYNI_SWRORD_SIZE ;  /// key head size word;
      _keyStringCoder coder ;
      keyStringMetaByte b ;
      keyStringMetaBlockHeader *header = nullptr ;

      if ( !indexID.isValid() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( nullptr == buf || bufSize < _MIN_BUF_SIZE )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      coder.encodeIndexID( indexID, asUpBound, buf ) ;
      *( buf + _keyStringCoder::INDEX_ID_ENCODEING_SIZE ) =
            _keyStringCoder::INDEX_ID_ENCODEING_SIZE ;
      *( buf + _keyStringCoder::INDEX_ID_ENCODEING_SIZE + 1 ) =
            _keyStringCoder::INDEX_ID_ENCODEING_SIZE ;
      header = reinterpret_cast<keyStringMetaBlockHeader*>( buf
            + _keyStringCoder::INDEX_ID_ENCODEING_SIZE + 2 ) ;

      b.setHasKeyHead() ;
      header->version = (UINT8)( keyStringVersion::version_1 ) ;
      header->metaByte = b.getValue() ;
      size = _MIN_BUF_SIZE ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::_appendMetaBlockSizeWord( UINT32 size,
                                                          BOOLEAN nonzero )
   {
      INT32 rc = SDB_OK ;
      if ( nonzero && 0 == size )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( size < KEY_STRING_TYNI_SIZE_BOUND )
      {
         rc = _append( static_cast<UINT8>( size ), FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append tiny size word, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _append( static_cast<UINT32>( size ), FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append size word, rc: %d", rc ) ;
            goto error ;
         }
         rc = _append( static_cast<UINT8>( KEY_STRING_TYNI_SIZE_BOUND ),
                       FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to append size word bound, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringBuilderImpl::rebuildEntryKey( const keyString &ks,
                                                 const dmsRecordID &rid,
                                                 utilSlice keyHeader )
   {
      INT32 rc = SDB_OK ;
      reset() ;
      if ( ( !ks.isValid() || !rid.isValid() ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         utilSlice elements = ks.getKeyElementsSlice() ;
         utilSlice bits = ks.getTypeBits() ;

         if ( keyHeader.isValid() )
         {
            rc = _ensureBytes( keyHeader.size() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to extend buffer, rc: %d", rc ) ;
               goto error ;
            }
            ossMemcpy( _buf + _bufSize, keyHeader.data(),
                       keyHeader.size() ) ;
            _bufSize += keyHeader.size() ;
         }

         rc = _ensureBytes( elements.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buffer, rc: %d", rc ) ;
            goto error ;
         }
         ossMemcpy( _buf + _bufSize, elements.data(), elements.size() ) ;
         _bufSize += elements.size() ;

         rc = _ensureBytes( _keyStringCoder::RID_ENCODING_SIZE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buffer, rc: %d", rc ) ;
            goto error ;
         }
         _keyStringCoder().encodeRID( rid, _buf + _bufSize ) ;
         _bufSize += _keyStringCoder::RID_ENCODING_SIZE ;

         rc = _ensureBytes( bits.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buffer, rc: %d", rc ) ;
            goto error ;
         }
         ossMemcpy( _buf + _bufSize, bits.data(), bits.size() ) ;
         _bufSize += bits.size() ;

         rc = _appendMetaBlock( keyHeader.size(), elements.size(),
                                _keyStringCoder::RID_ENCODING_SIZE,
                                bits.size() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build meta block, rc: %d", rc ) ;
            goto error ;
         }

         /// reset members to construct keystring desc
         _sizeAheadElements = keyHeader.size() ;
         _sizeOfElements = elements.size() ;
         _sizeAfterElements = _keyStringCoder::RID_ENCODING_SIZE ;
         _typeBitsSize = bits.size() ;

         _transition( keyStringBuilderStatus::DONE ) ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

}
}
