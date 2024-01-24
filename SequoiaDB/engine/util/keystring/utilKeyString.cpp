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

   Source File Name = utilKeyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/16/2022  LYC  Initial Draft
          07/20/2022  ZHY  Implement
   Last Changed =

******************************************************************************/

#include "keystring/utilKeyString.hpp"
#include "keystring/utilKeyStringMetaByte.hpp"
#include "keystring/utilKeyStringCoder.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilAllocator.hpp"
#include "ossLikely.hpp"
#include "ossEndian.hpp"

#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

using namespace std ;
using namespace bson ;

namespace engine
{
namespace keystring
{

   constexpr FLOAT64 invPow256[] =
   { 1.0,                                            // 2**0
     1.0 / 256,                                      // 2**(-8)
     1.0 / 256 / 256,                                // 2**(-16)
     1.0 / 256 / 256 / 256,                          // 2**(-24)
     1.0 / 256 / 256 / 256 / 256,                    // 2**(-32)
     1.0 / 256 / 256 / 256 / 256 / 256,              // 2**(-40)
     1.0 / 256 / 256 / 256 / 256 / 256 / 256,        // 2**(-48)
     1.0 / 256 / 256 / 256 / 256 / 256 / 256 / 256   // 2**(-56)
   } ;

   static UINT32 _neededBytesNumForInteger( keyStringEncodedType type )
   {
      if ( type <= keyStringEncodedType::numericNegative1ByteInt )
      {
         SDB_ASSERT( type >= keyStringEncodedType::numericNegative8ByteInt,
                     "Unexpected encoded type" ) ;
         return
               static_cast<UINT32>(
                     static_cast<UINT8>( keyStringEncodedType::numericNegative1ByteInt ) -
                     static_cast<UINT8>( type ) +
                     1 ) ;
      }
      SDB_ASSERT( type >= keyStringEncodedType::numericPositive1ByteInt,
                  "Unexpected encoded type" ) ;
      SDB_ASSERT( type <= keyStringEncodedType::numericPositive8ByteInt,
                  "Unexpected encoded type" ) ;
      return static_cast< UINT32 >(
                   static_cast< UINT8 >( type ) -
                   static_cast< UINT8 >( keyStringEncodedType::numericPositive1ByteInt ) +
                   1 ) ;
   }

   /*
      _keyString implement
    */
   _keyString::~_keyString()
   {
      reset() ;
   }

   _keyString::_keyString( const utilSlice &data,
                           const utilSlice &tail )
   : _ref( data ),
     _tailRef( tail )
   {
      if ( SDB_OK != _parse( _ref, _tailRef, _desc ) )
      {
         reset() ;
      }
   }

   _keyString::_keyString( UINT32 size, const CHAR *data )
   : _ref( size, data )
   {
      if ( SDB_OK != _parse( _ref, _tailRef, _desc ) )
      {
         reset() ;
      }
   }

   _keyString::_keyString( const _keyString &o )
   : _ref( o._ref ),
     _tailRef( o._tailRef ),
     _desc( o._desc )
   {
   }

   _keyString &_keyString::operator =( const _keyString &o )
   {
      if ( &o != this )
      {
         reset() ;
         _ref = o._ref ;
         _tailRef = o._tailRef ;
         _desc = o._desc ;
      }
      return *this ;
   }

   _keyString::_keyString( _keyString &&o ) noexcept
   {
      if ( o.isValid() )
      {
         _bufferOwned = o._bufferOwned ;
         _bufferSize = o._bufferSize ;
         _ref = o._ref ;
         _tailRef = o._tailRef ;
         _desc = o._desc ;
         o._bufferOwned = nullptr ;
         o.reset() ;
      }
   }

   _keyString &_keyString::operator =( _keyString && o ) noexcept
   {
      if ( &o != this )
      {
         reset() ;
         if ( o.isValid() )
         {
            _bufferOwned = o._bufferOwned ;
            _bufferSize = o._bufferSize ;
            _ref = o._ref ;
            _tailRef = o._tailRef ;
            _desc = o._desc ;
            o._bufferOwned = nullptr ;
            o.reset() ;
         }
      }
      return *this ;
   }

   void _keyString::reset()
   {
      _ref.reset() ;
      _tailRef.reset() ;
      _desc.reset() ;
      if ( nullptr != _bufferOwned )
      {
         SDB_THREAD_FREE( _bufferOwned ) ;
         _bufferOwned = nullptr ;
      }
      _bufferSize = 0 ;
   }

   INT32 _keyString::init( const utilSlice &data,
                           const utilSlice &tail )
   {
      INT32 rc = SDB_OK ;

      reset() ;

      if ( OSS_UNLIKELY( !data.isValid() ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _ref = data ;
      _tailRef = tail ;
      rc = _parse( _ref, _tailRef, _desc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to parese key stirng data, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   INT32 _keyString::getOwned()
   {
      INT32 rc = SDB_OK;

      if ( !isValid() )
      {
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( !isOwned() )
      {
         _bufferOwned = (CHAR *)(
            SDB_THREAD_ALLOC( _ref.getSize() + _tailRef.getSize() ) ) ;
         if ( OSS_UNLIKELY( nullptr == _bufferOwned ) )
         {
            PD_LOG( PDERROR, "Failed to allocate memory" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         _bufferSize = _ref.getSize() + _tailRef.getSize() ;
         ossMemcpy( _bufferOwned, _ref.data(), _ref.getSize() ) ;
         if ( _tailRef.isValid() )
         {
            ossMemcpy( _bufferOwned + _ref.getSize(),
                       _tailRef.data(),
                       _tailRef.getSize() ) ;
         }
         _ref.reset( _ref.getSize(), _bufferOwned ) ;
         _tailRef.reset() ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyString::_parse( const utilSlice &data,
                             const utilSlice &tail,
                             keyStringDescriptor &desc ) const
   {
      INT32 rc = SDB_OK ;

      utilBytesReader reader ;
      desc.reset() ;

      rc = parseMetaFromSlice( tail.isValid() ? tail : data, desc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to parse descriptor from slice, %d", rc ) ;
         goto error ;
      }

      if ( data.getSize() + tail.getSize() != desc.getStringSizeExpected() )
      {
         PD_LOG( PDERROR, "unexpected total string size[%d, %d, %d]",
                 data.getSize(), tail.getSize(), desc.getStringSizeExpected() ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

   done:
      return rc ;
   error:
      desc.reset() ;
      goto done ;
   }

   INT32 _keyString::_getOwnedWithException() const
   {
      INT32 rc = SDB_OK ;

      keyString *ref = const_cast<keyString *>( this ) ;
      rc = ref->getOwned() ;
      if ( SDB_OK != rc )
      {
         throw pdGeneralException( rc, "Failed to get key string owned" ) ;
      }
      return rc ;
   }

   BOOLEAN _keyString::_loadSizeData( utilBytesReader &reader,
                                      BOOLEAN nonzero,
                                      UINT32 &size )
   {
      SDB_ASSERT( !reader.isOutOfBound(), "can not be invalid" ) ;
      size = 0 ;
      if ( reader.getUINT8() < KEY_STRING_TYNI_SIZE_BOUND )
      {
         size = reader.getUINT8() ;
      }
      else if ( !reader.slide( KEY_STRING_SWORD_SIZE ) )
      {
         return FALSE ;
      }
      else
      {
         size = reader.get<UINT32>() ;
      }

      return !nonzero || 0 != size ;
   }

   utilSlice _keyString::getKeySlice() const
   {
      if ( OSS_UNLIKELY( !isValid() ) )
      {
         return utilSlice() ;
      }
      else
      {
         if ( OSS_UNLIKELY( _ref.getSize() < _desc.keySize && _tailRef.isValid() ) )
         {
            _getOwnedWithException() ;
         }
         return _ref.getSlice( 0, _desc.keySize ) ;
      }
   }

   utilSlice _keyString::getKeyHeadSlice() const
   {
      return hasKeyHead() ?
                  _ref.getSlice( 0, _desc.keyHeadSize ) :
                  utilSlice() ;
   }

   utilSlice _keyString::getKeySliceAfterHeader() const
   {
      return _desc.keyHeadSize < _desc.keySize ?
                  _ref.getSlice( _desc.keyHeadSize,
                                 _desc.keySize - _desc.keyHeadSize ) :
                  utilSlice() ;
   }

   utilSlice _keyString::getKeyElementsSlice() const
   {
      UINT32 size = getKeyElementsSize() ;
      return 0 < size ?
                  _ref.getSlice( _desc.keyHeadSize, size ) :
                  utilSlice() ;
   }

   utilSlice _keyString::getKeyTailSlice() const
   {
      return hasKeyTail() ?
                  ( ( ( _ref.getSize() <= _desc.keySize - _desc.keyTailSize ) &&
                      ( _tailRef.isValid() ) ) ?
                    ( _tailRef.getSlice( 0, _desc.keyTailSize ) ) :
                    ( _ref.getSlice( _desc.keySize - _desc.keyTailSize,
                                     _desc.keyTailSize ) ) ) :
                  ( utilSlice() ) ;
   }

   utilSlice _keyString::getKeySliceExceptTail() const
   {
      return _desc.keyTailSize < _desc.keySize ?
                   _ref.getSlice( 0, _desc.keySize - _desc.keyTailSize ) :
                   utilSlice() ;
   }

   utilSlice _keyString::getSliceAfterKeyElements() const
   {
      if ( _desc.keyTailSize < _desc.keySize )
      {
         if ( !_tailRef.isValid() )
         {
            return _ref.getSliceFromOffsetToEnd( _desc.keySize - _desc.keyTailSize ) ;
         }
         else
         {
            _getOwnedWithException() ;
            return _ref.getSliceFromOffsetToEnd( _desc.keySize - _desc.keyTailSize ) ;
         }
      }
      return utilSlice() ;
   }

   utilSlice _keyString::getSliceAfterKey() const
   {
      return isValid() ?
                  ( _tailRef.isValid() ?
                    ( _desc.keySize >= _ref.getSize() ?
                      _tailRef.getSliceFromOffsetToEnd( _desc.keySize - _ref.getSize() ) :
                      utilSlice() ) :
                    ( _ref.getSliceFromOffsetToEnd( _desc.keySize ) ) ) :
                  ( utilSlice() ) ;
   }

   utilSlice _keyString::getTypeBits() const
   {
      return isValid() && hasTypeBits() ?
                  ( _tailRef.isValid() ?
                    ( _desc.keySize >= _ref.getSize() ?
                      _tailRef.getSlice( _desc.keySize - _ref.getSize(),
                                           _desc.typeBitsSize ) :
                      utilSlice() ) :
                    ( _ref.getSlice( _desc.keySize, _desc.typeBitsSize ) ) ) :
                  ( utilSlice() ) ;
   }

   INT32 _keyString::compare( const _keyString &s ) const
   {
      SDB_ASSERT( isValid() && s.isValid(), "can not be invalid" ) ;
      if ( !_tailRef.isValid() && s._tailRef.isValid() )
      {
         return getKeySlice().compare( s.getKeySlice() ) ;
      }
      INT32 res = compareElements( s ) ;
      if ( 0 == res )
      {
         return getRID().compare( s.getRID() ) ;
      }
      return res ;
   }

   INT32 _keyString::compareElements( const _keyString &ks ) const
   {
      SDB_ASSERT( isValid() && ks.isValid(), "can not be invalid" ) ;
      return getKeyElementsSlice().compare( ks.getKeyElementsSlice() ) ;
   }

   dmsRecordID _keyString::getRID() const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      dmsRecordID rid ;
      utilSlice data = getKeyTailSlice() ;
      if ( data.getSize() <= keyStringCoder::RID_ENCODING_SIZE )
      {
         rid = keyStringCoder().decodeToRID( data.getData() ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid data size after key" ) ;
      }
      return rid ;
   }

   INT32 _keyString::compareCoding( UINT32 sizea, const CHAR *bufa,
                                    UINT32 sizeb, const CHAR *bufb )
   {
      UINT32 keySize0 =
            (UINT8)( bufa[ sizea - KEY_STRING_MB_HEADER_SIZE - 1 ] ) ;
      UINT32 keySize1 =
            (UINT8)( bufb[ sizeb - KEY_STRING_MB_HEADER_SIZE - 1 ] ) ;
      if ( keySize0 == KEY_STRING_TYNI_SIZE_BOUND )
      {
         keySize0 = *( (const UINT32*)( bufa + sizea
               - KEY_STRING_MB_HEADER_SIZE - KEY_STRING_SWORD_SIZE ) ) ;
      }
      if ( keySize1 == KEY_STRING_TYNI_SIZE_BOUND )
      {
         keySize1 = *( (const UINT32*)( bufb + sizeb
               - KEY_STRING_MB_HEADER_SIZE - KEY_STRING_SWORD_SIZE ) ) ;
      }

      INT32 res = ossMemcmp( bufa, bufb, OSS_MIN(keySize0, keySize1) ) ;
      if ( 0 == res )
      {
         if ( keySize0 < keySize1 )
         {
            res = -1 ;
         }
         else if ( keySize0 > keySize1 )
         {
            res = 1 ;
         }
      }

      return res ;
   }

   INT32 _keyString::parseMetaFromSlice( const utilSlice &metadataSlice,
                                         keyStringDescriptor &desc )
   {
      INT32 rc = SDB_OK ;
      const keyStringMetaBlockHeader *header = nullptr ;
      keyStringMetaByte mbyte ;
      utilBytesReader reader ;
      desc.reset() ;

      if ( OSS_UNLIKELY( !metadataSlice.isValid() ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( metadataSlice.getSize() <= KEY_STRING_MB_HEADER_SIZE )
      {
         PD_LOG( PDERROR, "Failed to parse metadata, invalid slice size: %d",
                 metadataSlice.getSize() ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }
      header = reinterpret_cast<const keyStringMetaBlockHeader *>(
            metadataSlice.getData() + metadataSlice.getSize() - KEY_STRING_MB_HEADER_SIZE ) ;

      if ( !header->isValid() )
      {
         PD_LOG( PDERROR, "Failed to parse metadata, invalid key string meta block header" ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      mbyte.init( header->metaByte ) ;
      reader.init( metadataSlice.getSlice(
         0, metadataSlice.getSize() - KEY_STRING_MB_HEADER_SIZE ), TRUE ) ;

      if ( !_loadSizeData( reader, FALSE, desc.keySize ) )
      {
         PD_LOG( PDERROR, "Failed to load key size" ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      if ( mbyte.hasKeyHead() )
      {
         if ( !reader.slide( 1 ) )
         {
            PD_LOG( PDERROR, "Failed to move to key head size begin pos" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         if ( !_loadSizeData( reader, TRUE, desc.keyHeadSize ) )
         {
            PD_LOG( PDERROR, "Failed to load key head size" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
      }

      if ( mbyte.hasKeyTail() )
      {
         if ( !reader.slide( 1 ) )
         {
            PD_LOG( PDERROR, "Failed to move to key tail size begin pos" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         if ( !_loadSizeData( reader, TRUE, desc.keyTailSize ) )
         {
            PD_LOG( PDERROR, "Failed to load key tail size" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
      }

      if ( mbyte.hasTypeBits() )
      {
         if ( !reader.slide( 1 ) )
         {
            PD_LOG( PDERROR, "Failed to move to type bits size begin pos" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         if ( !_loadSizeData( reader, TRUE, desc.typeBitsSize ) )
         {
            PD_LOG( PDERROR, "Failed to load type bits size" ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
      }

      if ( !desc.isValid() )
      {
         PD_LOG( PDERROR, "invalid string descriptor parsed" ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

   done:
      return rc ;

   error:
      desc.reset() ;
      goto done ;
   }


   BSONObj _keyString::toBSON( const BSONObj &pattern,
                               BOOLEAN withFieldName ) const
   {
      BSONObjBuilder builder ;
      SDB_ASSERT( isValid(), "must be valid" ) ;
      utilSlice s = getKeyElementsSlice() ;
      _bodyReader br( s.data(), s.size(), getTypeBits().data(),
                      getTypeBitsSize() ) ;
      br.toBSON( pattern, builder, withFieldName ) ;
      return builder.obj() ;
   }

   BSONObj _keyString::toBSON( const BSONObj &pattern,
                               BufBuilder &bufBuilder,
                               BOOLEAN withFieldName ) const
   {
      BSONObjBuilder builder( bufBuilder ) ;
      SDB_ASSERT( isValid(), "must be valid" ) ;
      utilSlice s = getKeyElementsSlice() ;
      _bodyReader br( s.data(), s.size(), getTypeBits().data(),
                      getTypeBitsSize() ) ;
      return br.toBSON( pattern, builder, withFieldName ) ;
   }

   /*
      _typeBitsReader implement
    */
   _keyString::_typeBitsReader::_typeBitsReader( const CHAR *buf,
                                                 UINT32 bufSize )
   : _buf( buf ), _bufSize( bufSize )
   {
   }

   UINT8 _keyString::_typeBitsReader::_readBit()
   {
      const UINT32 byte = _curBit / 8 ;
      SDB_ASSERT( byte < _bufSize, "must be less than buffer size" ) ;
      const UINT32 offsetInByte = _curBit % 8 ;
      UINT8 oneOrZero = ( _buf[ byte ] >> ( 7 - offsetInByte ) )
            & 0b00000001 ;
      _curBit ++ ;
      return oneOrZero ;
   }

   keyStringTypeBitsType _keyString::_typeBitsReader::readNumeric()
   {
      UINT8 output = 0 ;
      output += ( ( _readBit() << 1 ) + _readBit() ) ;
      return static_cast<keyStringTypeBitsType>( output ) ;
   }

   UINT8 _keyString::_typeBitsReader::readByte()
   {
      UINT8 output = 0 ;
      for ( UINT32 i = 0 ; i < 8 ; i ++ )
      {
         output = ( output << 1 ) + _readBit() ;
      }
      return output ;
   }

   keyStringTypeBitsType _keyString::_typeBitsReader::readZero()
   {
      return static_cast<keyStringTypeBitsType>( _readBit() ) ;
   }

   keyStringTypeBitsType _keyString::_typeBitsReader::readStringLike()
   {
      return static_cast<keyStringTypeBitsType>( _readBit() ) ;
   }

   keyStringTypeBitsType _keyString::_typeBitsReader::readTimestampOrDate()
   {
      UINT8 output = 0 ;
      output += ( ( _readBit() << 1 ) + _readBit() ) ;
      return static_cast<keyStringTypeBitsType>( output ) ;
   }

   /*
      _bodyReader implement
    */
   _keyString::_bodyReader::_bodyReader( const CHAR *bodyBuf,
                                         UINT32 bufSize,
                                         const CHAR *typeBitsBuf,
                                         UINT32 typeBitsBufSize )
   : _buf( bodyBuf ),
     _bufSize( bufSize ),
     _typeReader( typeBitsBuf, typeBitsBufSize )
   {
   }

   template<typename T>
   T _keyString::_bodyReader::_read( BOOLEAN inverted )
   {
      UINT32 size = sizeof(T) ;
      T t ;
      if ( inverted )
      {
         ossMemcpyFlipBits( &t, _buf + _offset, size ) ;
      }
      else
      {
         ossMemcpy( &t, _buf + _offset, size ) ;
      }
      SDB_ASSERT( _offset + size <= _bufSize, "out of buffer size" ) ;
      _offset += size ;
      return t ;
   }

   template<typename T>
   T _keyString::_bodyReader::_peek( BOOLEAN inverted ) const
   {
      UINT32 size = sizeof(T) ;
      T t ;
      if ( inverted )
      {
         ossMemcpyFlipBits( &t, _buf + _offset, size ) ;
      }
      else
      {
         ossMemcpy( &t, _buf + _offset, size ) ;
      }
      return t ;
   }

   BSONObj _keyString::_bodyReader::toBSON( const BSONObj &pattern,
                                            BSONObjBuilder &builder,
                                            BOOLEAN withFieldName )
   {
      _offset = 0 ;
      BSONObjIterator it( pattern ) ;
      while ( _offset < _bufSize && it.more() )
      {
         BSONElement ele = it.next() ;
         BOOLEAN inverted = ele.numberInt() == -1 ? TRUE : FALSE ;
         keyStringEncodedType type = static_cast<keyStringEncodedType>( _read<UINT8>(
               inverted ) ) ;
         keyStringDiscriminatorValue dv = static_cast<keyStringDiscriminatorValue>( type ) ;
         if ( keyStringDiscriminatorValue::END == dv ||
              keyStringDiscriminatorValue::LESS == dv ||
              keyStringDiscriminatorValue::GREATER == dv )
         {
            break ;
         }
         toBSONValue( type, inverted, builder,
                      withFieldName ? ele.fieldName() : nullptr ) ;
      }
      return builder.done() ;
   }

   void _keyString::_bodyReader::_toBSON( BOOLEAN inverted,
                                          BSONObjBuilder &builder,
                                          const CHAR *fieldName )
   {
      BSONObjBuilder newObjBuilder ;
      while ( 0 != static_cast<UINT8>( _read<keyStringEncodedType>( inverted ) ) )
      {
         ossPoolString name ;
         _readCString( inverted, name ) ;
         toBSONValue( _read<keyStringEncodedType>( inverted ), inverted,
                      newObjBuilder, name.data() ) ;
      }
      fieldName ?
            builder.appendObject( fieldName,
                                  newObjBuilder.done().objdata() ) :
            builder.appendObject( "", newObjBuilder.done().objdata() ) ;
   }

   BSONObj _keyString::_bodyReader::_toBSON( BOOLEAN inverted,
                                             BOOLEAN withFieldName )
   {
      BSONObjBuilder newObjBuilder ;
      while ( 0 != static_cast<UINT8>( _read<keyStringEncodedType>( inverted ) ) )
      {
         ossPoolString name ;
         _readCString( inverted, name ) ;
         toBSONValue( _read<keyStringEncodedType>( inverted ), inverted,
                      newObjBuilder, withFieldName ? name.data() : "" ) ;
      }
      return newObjBuilder.obj() ;
   }

   void _keyString::_bodyReader::toBSONValue( keyStringEncodedType type,
                                              BOOLEAN inverted,
                                              BSONObjBuilder &builder,
                                              const CHAR *fieldName )
   {
      switch ( type )
      {
      case keyStringEncodedType::minKey :
         fieldName ?
               builder.appendMinKey( fieldName ) :
               builder.appendMinKey( "" ) ;
         break ;
      case keyStringEncodedType::undefined :
         fieldName ?
               builder.appendUndefined( fieldName ) :
               builder.appendUndefined( "" ) ;
         break ;
      case keyStringEncodedType::nullish :
         fieldName ?
               builder.appendNull( fieldName ) :
               builder.appendNull( "" ) ;
         break ;
      case keyStringEncodedType::numericNaN :
      case keyStringEncodedType::numericNegativeLargeMagnitude :
      case keyStringEncodedType::numericNegative8ByteInt :
      case keyStringEncodedType::numericNegative7ByteInt :
      case keyStringEncodedType::numericNegative6ByteInt :
      case keyStringEncodedType::numericNegative5ByteInt :
      case keyStringEncodedType::numericNegative4ByteInt :
      case keyStringEncodedType::numericNegative3ByteInt :
      case keyStringEncodedType::numericNegative2ByteInt :
      case keyStringEncodedType::numericNegative1ByteInt :
      case keyStringEncodedType::numericNegativeSmallMagnitude :
      case keyStringEncodedType::numericZero :
      case keyStringEncodedType::numericPositiveSmallMagnitude :
      case keyStringEncodedType::numericPositive1ByteInt :
      case keyStringEncodedType::numericPositive2ByteInt :
      case keyStringEncodedType::numericPositive3ByteInt :
      case keyStringEncodedType::numericPositive4ByteInt :
      case keyStringEncodedType::numericPositive5ByteInt :
      case keyStringEncodedType::numericPositive6ByteInt :
      case keyStringEncodedType::numericPositive7ByteInt :
      case keyStringEncodedType::numericPositive8ByteInt :
      case keyStringEncodedType::numericPositiveLargeMagnitude :
         _toNumeric( type, inverted, builder, fieldName ) ;
         break ;
      case keyStringEncodedType::stringLike :
      {
         ossPoolString s ;
         _decodeStringLike( inverted, s ) ;
         keyStringTypeBitsType originalType = _typeReader.readStringLike() ;
         if ( keyStringTypeBitsType::STRING == originalType )
         {
            fieldName ?
                  builder.append( fieldName, s ) :
                  builder.append( "", s ) ;
         }
         else
         {
            fieldName ?
                  builder.appendSymbol( fieldName, s ) :
                  builder.appendSymbol( "", s ) ;
         }
         break ;
      }
      case keyStringEncodedType::object :
      {
         _toBSON( inverted, builder, fieldName ) ;
         break ;
      }
      case keyStringEncodedType::array :
      {
         BSONObjBuilder arrayBuilder ;
         keyStringEncodedType type = _read<keyStringEncodedType>( inverted ) ;
         UINT32 elemCount = 0 ;
         while ( 0 != static_cast<UINT8>( type ) )
         {
            toBSONValue( type, inverted, arrayBuilder,
                         std::to_string( elemCount ).c_str() ) ;
            elemCount ++ ;
            type = _read<keyStringEncodedType>( inverted ) ;
         }
         BSONArray arr( arrayBuilder.obj() ) ;
         fieldName ?
               builder.appendArray( fieldName, arr ) :
               builder.appendArray( "", arr ) ;
         break ;
      }

      case keyStringEncodedType::binData :
      {
         UINT8 firstByteLen = _read<UINT8>( inverted ) ;
         UINT32 binDataLen = 0 ;
         if ( 0xFF == firstByteLen )
         {
            binDataLen = ossBigEndianToNative(
                  _read<UINT32>( inverted ) ) ;
         }
         else
         {
            binDataLen = firstByteLen ;
         }
         BinDataType binDataType =
               static_cast<BinDataType>( _read<UINT8>( inverted ) ) ;
         CHAR data[ binDataLen ] ;
         _readBytes( inverted, data, binDataLen ) ;
         if ( binDataType == BinDataType::ByteArrayDeprecated )
         {
            fieldName ?
                  builder.appendBinDataArrayDeprecated( fieldName,
                                                        data + 4,
                                                        binDataLen - 4 ) :
                  builder.appendBinDataArrayDeprecated( "", data + 4,
                                                        binDataLen - 4 ) ;
         }
         else
         {
            fieldName ?
                  builder.appendBinData( fieldName, binDataLen,
                                         binDataType, data ) :
                  builder.appendBinData( "", binDataLen, binDataType,
                                         data ) ;
         }
         break ;
      }
      case keyStringEncodedType::oid :
      {
         OID oid = _read<OID>( inverted ) ;
         fieldName ?
               builder.appendOID( fieldName, &oid ) :
               builder.appendOID( "", &oid ) ;
         break ;
      }
      case keyStringEncodedType::booleanFalse :
      {
         fieldName ?
               builder.appendBool( fieldName, FALSE ) :
               builder.appendBool( "", FALSE ) ;
         break ;
      }
      case keyStringEncodedType::booleanTrue :
      {
         fieldName ?
               builder.appendBool( fieldName, TRUE ) :
               builder.appendBool( "", TRUE ) ;
         break ;
      }
      case keyStringEncodedType::time :
      {
         keyStringTypeBitsType originalType = _typeReader.readTimestampOrDate() ;
         INT64 encoded = ossBigEndianToNative(
               _read<INT64>( inverted ) ) ;
         INT64 seconds = encoded ^ std::numeric_limits < INT64 > ::min() ;
         UINT32 microseconds = ossBigEndianToNative(
               _read<UINT32>( inverted ) ) ;

         if ( originalType == keyStringTypeBitsType::DATE )
         {
            INT64 val = seconds * 1000 + microseconds / 1000 ;
            Date_t dt( val ) ;
            fieldName ?
                  builder.appendDate( fieldName, dt ) :
                  builder.appendDate( "", dt ) ;
         }
         else if ( originalType == keyStringTypeBitsType::TIMESTAMP )
         {
            fieldName ?
                  builder.appendTimestamp( fieldName, seconds * 1000,
                                           microseconds ) :
                  builder.appendTimestamp( "", seconds * 1000,
                                           microseconds ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "Reserved" ) ;
         }
         break ;
      }
      case keyStringEncodedType::regEx :
      {
         ossPoolString regex ;
         _readCString( inverted, regex ) ;
         ossPoolString flags ;
         _readCString( inverted, flags ) ;
         fieldName ?
               builder.appendRegex( fieldName, regex, flags ) :
               builder.appendRegex( "", regex, flags ) ;
         break ;
      }
      case keyStringEncodedType::dbRef :
      {
         UINT32 nsLen = ossBigEndianToNative(
               _read<UINT32>( inverted ) ) ;
         CHAR nsData[ nsLen + 1 ] ;
         _readBytes( inverted, nsData, nsLen ) ;
         nsData[ nsLen ] = '\0' ;
         StringData s( nsData, nsLen ) ;
         OID oid = _read<OID>( inverted ) ;
         fieldName ?
               builder.appendDBRef( fieldName, s, oid ) :
               builder.appendDBRef( "", s, oid ) ;
         break ;
      }

      case keyStringEncodedType::code :
      {
         keyStringEncodedType type = _read<keyStringEncodedType>( inverted ) ;
         SDB_ASSERT( keyStringEncodedType::stringLike == type,
                     "Unexpected encoded type" ) ;
         ossPoolString code ;
         _decodeStringLike( inverted, code ) ;
         fieldName ?
               builder.appendCode( fieldName, code ) :
               builder.appendCode( "", code ) ;
         break ;
      }
      case keyStringEncodedType::codeWithScope :
      {
         keyStringEncodedType type = _read<keyStringEncodedType>( inverted ) ;
         SDB_ASSERT( keyStringEncodedType::stringLike == type,
                     "Unexpected encoded type" ) ;
         ossPoolString code ;
         _decodeStringLike( inverted, code ) ;
         BSONObj scope = _toBSON( inverted, TRUE ) ;
         fieldName ?
               builder.appendCodeWScope( fieldName, code, scope ) :
               builder.appendCodeWScope( "", code, scope ) ;
         break ;
      }
      case keyStringEncodedType::maxKey :
         fieldName ?
               builder.appendMaxKey( fieldName ) :
               builder.appendMaxKey( "" ) ;
         break ;
      default :
         SDB_ASSERT( FALSE, "Unexpected encoded type" ) ;
         break ;
      }
   }

   void _keyString::_bodyReader::_toNumeric( keyStringEncodedType type,
                                             BOOLEAN inverted,
                                             BSONObjBuilder &builder,
                                             const CHAR *fieldName )
   {

      keyStringTypeBitsType originalType = _typeReader.readNumeric() ;
      BOOLEAN isNegative = FALSE ;
      switch ( type )
      {
      case keyStringEncodedType::numericNaN :
         if ( originalType == keyStringTypeBitsType::DOUBLE )
         {
            fieldName ?
                  builder.appendNumber(
                        fieldName,
                        std::numeric_limits<FLOAT64>::quiet_NaN() ) :
                  builder.appendNumber(
                        "",
                        std::numeric_limits<FLOAT64>::quiet_NaN() ) ;
         }
         else if ( originalType == keyStringTypeBitsType::DECIMAL )
         {
            bsonDecimal dec ;
            dec.setNan() ;
            fieldName ?
                  builder.append( fieldName, dec ) :
                  builder.append( "", dec ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "Unexpected original type" ) ;
         }
         break ;

      case keyStringEncodedType::numericZero :
         if ( originalType == keyStringTypeBitsType::INT )
         {
            fieldName ?
                  builder.appendNumber( fieldName,
                                        static_cast<INT32>( 0 ) ) :
                  builder.appendNumber( "", static_cast<INT32>( 0 ) ) ;
         }
         else if ( originalType == keyStringTypeBitsType::LONG )
         {
            fieldName ?
                  builder.appendNumber( fieldName,
                                        static_cast<INT64>( 0 ) ) :
                  builder.appendNumber( "", static_cast<INT64>( 0 ) ) ;
         }
         else if ( originalType == keyStringTypeBitsType::DOUBLE )
         {
            keyStringTypeBitsType zeroType = _typeReader.readZero() ;
            SDB_ASSERT(
                  zeroType == keyStringTypeBitsType::NEGATIVE_ZERO || zeroType
                        == keyStringTypeBitsType::POSITIVE_ZERO,
                  "Unexpected zero type" ) ;
            FLOAT64 zero =
                  zeroType == keyStringTypeBitsType::NEGATIVE_ZERO ? -0.0 : 0.0 ;
            fieldName ?
                  builder.appendNumber( fieldName, zero ) :
                  builder.appendNumber( "", zero ) ;
         }
         else if ( originalType == keyStringTypeBitsType::DECIMAL )
         {
            keyStringTypeBitsType zeroType = _typeReader.readZero() ;
            SDB_ASSERT(
                  zeroType == keyStringTypeBitsType::NEGATIVE_ZERO || zeroType
                        == keyStringTypeBitsType::POSITIVE_ZERO,
                  "Unexpected zero type" ) ;
            const CHAR *zero =
                  zeroType == keyStringTypeBitsType::NEGATIVE_ZERO ?
                        "-0.0" : "0.0" ;
            fieldName ?
                  builder.appendDecimal( fieldName, zero ) :
                  builder.appendDecimal( "", zero ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "Unexpected original type" ) ;
         }
         break ;
      case keyStringEncodedType::numericNegativeSmallMagnitude :
      case keyStringEncodedType::numericNegativeLargeMagnitude :
         isNegative = TRUE ;
         inverted = !inverted ;
      case keyStringEncodedType::numericPositiveSmallMagnitude :
      case keyStringEncodedType::numericPositiveLargeMagnitude :
      {
         UINT64 encoded = _read<UINT64>( inverted ) ;
         encoded = ossBigEndianToNative( encoded ) ;
         if ( encoded == ~0ULL )
         {
            if ( originalType == keyStringTypeBitsType::DOUBLE )
            {
               FLOAT64 num = std::numeric_limits<FLOAT64>::infinity() ;
               fieldName ?
                     builder.appendNumber( fieldName,
                                           isNegative ? -num : num ) :
                     builder.appendNumber( "", isNegative ? -num : num ) ;
            }
            else if ( originalType == keyStringTypeBitsType::DECIMAL )
            {
               bsonDecimal dec ;
               isNegative ? dec.setMin() : dec.setMax() ;
               fieldName ?
                     builder.append( fieldName, dec ) :
                     builder.append( "", dec ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "Unexpected original type" ) ;
            }
            break ;
         }
         keyStringContinuousMarker dcm = static_cast<keyStringContinuousMarker>( encoded & 1ULL ) ;
         encoded >>= 1 ;
         FLOAT64 abs ;
         ossMemcpy( &abs, &encoded, sizeof( abs ) ) ;
         if ( dcm == keyStringContinuousMarker::hasNoContinuation )
         {
            if ( originalType == keyStringTypeBitsType::DOUBLE )
            {
               fieldName ?
                     builder.appendNumber( fieldName,
                                           isNegative ? -abs : abs ) :
                     builder.appendNumber( "", isNegative ? -abs : abs ) ;
            }
            else if ( originalType == keyStringTypeBitsType::DECIMAL )
            {
               INT32 typemod = _typeReader.read<INT32>() ;
               INT16 ndigit = _typeReader.read<INT16>() ;
               INT16 dscale = _typeReader.read<INT16>() ;
               ossPoolStringStream ss ;
               ss << std::fixed ;
               if ( dscale >= 0 )
               {
                  ss << std::fixed << std::setprecision( dscale ) << ( isNegative ? -abs : abs ) ;
               }
               else
               {
                  ss << std::fixed << std::setprecision( _doublePrecision10 ) << ( isNegative ? -abs : abs ) ;
               }
               bsonDecimal dec ;
               dec.fromString( ss.str().c_str() ) ;
               dec.updateTypemod( typemod ) ;
               SDB_ASSERT( dec.getNdigit() == ndigit,
                           "Expected to be equal" ) ;
               fieldName ?
                     builder.append( fieldName, dec ) :
                     builder.append( "", dec ) ;
            }
            else if ( originalType == keyStringTypeBitsType::LONG )
            {
               INT64 val = static_cast<INT64>( isNegative ? -abs : abs ) ;
               fieldName ?
                     builder.appendNumber( fieldName, val ) :
                     builder.appendNumber( "", val ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "Unexpected originalType" ) ;
            }
         }
         else
         {
            bsonDecimal dec = _decodeDecimal( inverted,
                                                    isNegative ) ;
            fieldName ?
                  builder.append( fieldName, dec ) :
                  builder.append( "", dec ) ;
         }
         break ;
      }
      case keyStringEncodedType::numericNegative8ByteInt :
      case keyStringEncodedType::numericNegative7ByteInt :
      case keyStringEncodedType::numericNegative6ByteInt :
      case keyStringEncodedType::numericNegative5ByteInt :
      case keyStringEncodedType::numericNegative4ByteInt :
      case keyStringEncodedType::numericNegative3ByteInt :
      case keyStringEncodedType::numericNegative2ByteInt :
      case keyStringEncodedType::numericNegative1ByteInt :
         isNegative = TRUE ;
         inverted = !inverted ;
      case keyStringEncodedType::numericPositive1ByteInt :
      case keyStringEncodedType::numericPositive2ByteInt :
      case keyStringEncodedType::numericPositive3ByteInt :
      case keyStringEncodedType::numericPositive4ByteInt :
      case keyStringEncodedType::numericPositive5ByteInt :
      case keyStringEncodedType::numericPositive6ByteInt :
      case keyStringEncodedType::numericPositive7ByteInt :
      case keyStringEncodedType::numericPositive8ByteInt :
      {
         UINT64 encoded = 0 ;
         UINT32 integralNeededBytes = _neededBytesNumForInteger( type ) ;
         for ( UINT32 i = integralNeededBytes ; i ; i -- )
         {
            encoded = ( encoded << 8 ) | _read<UINT8>( inverted ) ;
         }

         BOOLEAN hasFractionPart = ( encoded & 1ULL ) ;
         INT64 integerValue = encoded >> 1 ;
         if ( !hasFractionPart )
         {
            if ( isNegative )
            {
               integerValue = -integerValue ;
            }
            switch ( originalType )
            {
               case keyStringTypeBitsType::INT :
                  fieldName ?
                        builder.appendNumber(
                              fieldName,
                              static_cast<INT32>( integerValue ) ) :
                        builder.appendNumber(
                              "", static_cast<INT32>( integerValue ) ) ;
                  break ;
               case keyStringTypeBitsType::LONG :
                  fieldName ?
                        builder.appendNumber(
                              fieldName,
                              static_cast<INT64>( integerValue ) ) :
                        builder.appendNumber(
                              "", static_cast<INT64>( integerValue ) ) ;
                  break ;
               case keyStringTypeBitsType::DOUBLE :
                  fieldName ?
                        builder.appendNumber(
                              fieldName,
                              static_cast<FLOAT64>( integerValue ) ) :
                        builder.appendNumber(
                              "", static_cast<FLOAT64>( integerValue ) ) ;
                  break ;
               case keyStringTypeBitsType::DECIMAL :
               {
                  bsonDecimal dec ;
                  dec.fromLong( integerValue ) ;
                  INT32 typemod = _typeReader.read<INT32>() ;
                  _typeReader.skip<INT16>() ;
                  _typeReader.skip<INT16>() ;
                  dec.updateTypemod( typemod ) ;
                  fieldName ?
                        builder.append( fieldName, dec ) :
                        builder.append( "", dec ) ;
                  break ;
               }
            }
            break ;
         }
         // Has fractional part
         UINT32 frcationalBytes = 8 - integralNeededBytes ;
         UINT64 fractionalEncoded = integerValue ;
         for ( int i = frcationalBytes ; i ; i -- )
         {
            fractionalEncoded = ( fractionalEncoded << 8 )
                  | _read<UINT8>( inverted ) ;
         }
         keyStringContinuousMarker dcm =
               static_cast<keyStringContinuousMarker>( fractionalEncoded & 1ULL ) ;
         if ( 0 == frcationalBytes )
         {
            dcm = static_cast<keyStringContinuousMarker>( encoded & 1ULL ) ;
         }
         if ( dcm == keyStringContinuousMarker::hasNoContinuation )
         {
            FLOAT64 abs =
                  static_cast<FLOAT64>( ( fractionalEncoded &= ( ~1ULL ) )
                        * invPow256[ frcationalBytes ] ) ;
            if ( originalType == keyStringTypeBitsType::DOUBLE )
            {
               fieldName ?
                     builder.appendNumber( fieldName,
                                           isNegative ? -abs : abs ) :
                     builder.appendNumber( "", isNegative ? -abs : abs ) ;
            }
            else if ( originalType == keyStringTypeBitsType::DECIMAL )
            {
               INT32 typemod = _typeReader.read<INT32>() ;
               INT16 ndigit = _typeReader.read<INT16>() ;
               INT64 dscale = _typeReader.read<INT16>() ;
               ossPoolStringStream ss ;
               ss << std::fixed ;
               if ( dscale >= 0 )
               {
                  ss << std::setprecision( dscale ) << ( isNegative ? -abs : abs ) ;
               }
               else
               {
                  ss << std::setprecision( _doublePrecision10 ) << ( isNegative ? -abs : abs ) ;
               }
               bsonDecimal decFromDouble ;
               decFromDouble.fromString( ss.str().c_str() ) ;
               decFromDouble.updateTypemod( typemod ) ;
               SDB_ASSERT( decFromDouble.getNdigit() == ndigit, "Expected to be equal" ) ;
               SDB_ASSERT( decFromDouble.getDScale() == dscale, "Expected to be equal" ) ;
               fieldName ?
                     builder.append( fieldName, decFromDouble ) :
                     builder.append( "", decFromDouble ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "Unexpected original type" ) ;
            }
         }
         else
         {
            bsonDecimal dec = _decodeDecimal( inverted, isNegative ) ;
            fieldName ?
                  builder.append( fieldName, dec ) :
                  builder.append( "", dec ) ;
         }
         break ;
      }
      default :
         SDB_ASSERT( FALSE, "Unexpected encoded type" ) ;
         break ;
      }
   }

   void _keyString::_bodyReader::_decodeStringLike( BOOLEAN inverted,
                                                    ossPoolString &s )
   {
      _readCString( inverted, s ) ;
      while ( _offset != _bufSize && 0xFF == _peek<UINT8>( inverted ) )
      {
         SDB_ASSERT( _offset + 1 <= _bufSize, "out of buffer size" ) ;
         _offset += 1 ;
         s.append( "\x00", 1 ) ;
         _readCString( inverted, s ) ;
      }
   }

   bsonDecimal _keyString::_bodyReader::_decodeDecimal( BOOLEAN inverted,
                                                        BOOLEAN isNegative )
   {
      bsonDecimal dec ;
      const UINT32 integerPartNdigit = ossBigEndianToNative(
            _read<UINT32>( inverted ) ) ;
      INT16 weight = 0 ;
      INT32 typemod = _typeReader.read<INT32>() ;
      INT16 ndigit = _typeReader.read<INT16>() ;
      INT16 dscale = _typeReader.read<INT16>() ;
      ossPoolStringStream decSS ;
      ossPoolStringStream digitSS ;
      if ( isNegative )
      {
         decSS << "-" ;
      }
      if ( 0 == integerPartNdigit )
      {
         decSS << "0" ;
         weight = -ossBigEndianToNative( _read<INT16>( !inverted ) ) ;
         SDB_ASSERT( weight < 0, "Unexpected weight" ) ;
         while ( weight ++ < -1 )
         {
            digitSS << "0000" ;
         }
         const UINT16 fractionPartNdigit = ndigit ;
         UINT16 digit = 0 ;
         for ( UINT16 i = fractionPartNdigit ; i ; i -- )
         {
            digit = static_cast<UINT16>(
                  ossBigEndianToNative( _read<UINT16>( inverted ) ) >> 1 ) ;
            SDB_ASSERT( digit >= 0 && digit < SDB_DECIMAL_NBASE, "Unexpected digit value" ) ;
            CHAR pBuffers[ SDB_DECIMAL_DEC_DIGITS + 1 ] = { } ;
            ossSnprintf( pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit ) ;
            digitSS << pBuffers ;
         }
      }
      else
      {
         UINT16 digit = 0 ;
         if ( static_cast<UINT16>( ndigit ) < integerPartNdigit )
         {
            UINT16 i = 0 ;
            for ( ; i < ndigit ; i ++ )
            {
               digit = ossBigEndianToNative( _read<UINT16>( inverted ) ) ;
               SDB_ASSERT( digit >= 0 && digit < SDB_DECIMAL_NBASE, "Unexpected digit value" ) ;
               CHAR pBuffers[ SDB_DECIMAL_DEC_DIGITS + 1 ] = { } ;
               ossSnprintf( pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d",
                            digit ) ;
               decSS << pBuffers ;
            }
            for ( ; i < integerPartNdigit ; i ++ )
            {
               decSS << "0000" ;
            }
         }
         else
         {
            for ( UINT16 i = integerPartNdigit ; i ; i -- )
            {
               digit = ossBigEndianToNative( _read<UINT16>( inverted ) ) ;
               SDB_ASSERT( digit >= 0 && digit < SDB_DECIMAL_NBASE, "Unexpected digit value" ) ;
               CHAR pBuffers[ SDB_DECIMAL_DEC_DIGITS + 1 ] = { } ;
               ossSnprintf( pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit ) ;
               decSS << pBuffers ;
            }
            for ( UINT16 i = ndigit - integerPartNdigit ; i ; i -- )
            {
               digit = static_cast<UINT16>( ossBigEndianToNative(
                     _read<UINT16>( inverted ) ) >> 1 ) ;
               SDB_ASSERT( digit >= 0 && digit < SDB_DECIMAL_NBASE, "Unexpected digit value" ) ;
               CHAR pBuffers[ SDB_DECIMAL_DEC_DIGITS + 1 ] = { } ;
               ossSnprintf( pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit ) ;
               digitSS << pBuffers ;
            }
         }
      }
      // fix dscale
      UINT32 digitLength = digitSS.str().size() ;
      if ( dscale > 0 && (UINT32)dscale != digitLength )
      {
         if ( digitLength > (UINT32)dscale )
         {
            decSS << "." << digitSS.str().substr( 0, dscale ) ;
         }
         else if ( (UINT32)dscale > digitLength )
         {
            decSS << "." ;
            if ( digitLength > 0 )
            {
               decSS << digitSS.str() ;
            }
            for ( UINT32 i = 0 ; i < (UINT32)dscale - digitLength ; ++ i )
            {
               decSS << "0" ;
            }
         }
      }
      else if ( digitLength > 0 )
      {
         decSS << "." << digitSS.str() ;
      }
      dec.fromString( decSS.str().c_str() ) ;
      dec.updateTypemod( typemod ) ;
      return dec ;
   }

   void _keyString::_bodyReader::_readBytes( BOOLEAN inverted, CHAR *bytes,
                                             UINT32 len )
   {
      if ( inverted )
      {
         ossMemcpyFlipBits( bytes, _buf + _offset, len ) ;
      }
      else
      {
         ossMemcpy( bytes, _buf + _offset, len ) ;
      }
      SDB_ASSERT( _offset + len <= _bufSize, "out of buffer size" ) ;
      _offset += len ;
   }

   void _keyString::_bodyReader::_readCString( BOOLEAN inverted,
                                               ossPoolString &s )
   {
      UINT32 strOldSize = s.size() ;
      const UINT8 endChar = inverted ? 0xFF : 0 ;
      const CHAR *start = static_cast<const CHAR*>( _buf + _offset ) ;
      const CHAR *end = static_cast<const CHAR*>( memchr(
            start, endChar, _bufSize - _offset ) ) ;
      UINT32 bytesNum = end - start ;
      SDB_ASSERT( _offset + bytesNum + 1 <= _bufSize,
                  "out of buffer size" ) ;
      _offset += ( bytesNum + 1 ) ;
      s.append( start, bytesNum ) ;
      if ( inverted )
      {
         for ( UINT32 i = strOldSize ; i < s.size() ; i ++ )
         {
            s[ i ] = ~s[ i ] ;
         }
      }
   }

}
}
