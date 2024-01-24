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

   Source File Name = utilKeyStringBuilder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/11/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_KEY_STRING_BUILDER_H_
#define UTIL_KEY_STRING_BUILDER_H_

#include "keystring/utilKeyStringDef.hpp"
#include "keystring/utilKeyString.hpp"
#include "keystring/utilKeyStringCoder.hpp"
#include "keystring/utilKeyStringMetaByte.hpp"
#include "utilStreamAllocator.hpp"
#include "dmsMetadata.hpp"
#include "dpsDef.hpp"
#include "rtnPredicate.hpp"

#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>

namespace engine
{
namespace keystring
{

   /*
      _typeBitsBuilder define
    */
   class _typeBitsBuilder
   {
   public:
      _typeBitsBuilder() = default ;
      ~_typeBitsBuilder() ;
      _typeBitsBuilder operator=( const _typeBitsBuilder& ) = delete ;
      _typeBitsBuilder( const _typeBitsBuilder& ) = delete ;

   public:
      void reset() ;
      INT32 appendBit( UINT8 oneOrZero ) ;
      INT32 appendString() ;
      INT32 appendSymbol() ;
      INT32 appendDate() ;
      INT32 appendTimestamp() ;
      INT32 appendNumberDouble() ;
      INT32 appendNumberInt() ;
      INT32 appendNumberLong() ;
      INT32 appendNumberDecimal() ;
      INT32 appendPositiveZero() ;
      INT32 appendNegativeZero() ;
      INT32 appendBits( const UINT8 *bytes, const UINT32 bytesSize ) ;
      INT32 appendDecimalMeta( const bson::bsonDecimal &dec ) ;

      const CHAR *getBuf() const
      {
         return _buf ;
      }

      UINT32 getBufSize() const
      {
         return _bufSize ;
      }

      BOOLEAN isEmpty() const
      {
         return nullptr == _buf || 0 == _bufSize ;
      }

   private:
      INT32 _ensureBytes( UINT32 length ) ;

   private:
      UINT32 _curBit = 0 ;
      CHAR *_buf = nullptr ;
      UINT32 _bufSize = 0 ;
      UINT32 _capacity = 0 ;
   } ;

   typedef class _typeBitsBuilder typeBitsBuilder ;

   /*
      _keyStringBuilderImpl define
    */
   class _keyStringBuilderImpl : public SDBObject
   {
   public:
      _keyStringBuilderImpl( utilStreamAllocator &allocator ) ;
      ~_keyStringBuilderImpl() ;

      _keyStringBuilderImpl( const _keyStringBuilderImpl& ) = delete ;
      _keyStringBuilderImpl operator =( const _keyStringBuilderImpl& ) = delete ;

   private:
      enum class keyStringBuilderStatus : UINT16
      {
         EMPTY = 0,
         BEFORE_ELEMENTS = 1,
         APPENDING_ELEMENTS = 2,
         AFTER_ELEMENTS = 3,
         DONE = 4
      } ;

   public:
      using stringTransformFn =
                     std::function<std::string( const bson::StringData & )> ;
      void reset() ;
      void resetTypeBits( const _typeBitsBuilder &tb ) ;
      INT32 appendBSONElement( const bson::BSONElement &elem,
                               BOOLEAN isDescending = FALSE ) ;
      INT32 appendAllElements( const bson::BSONObj &obj,
                               const bson::Ordering &o,
                               keyStringDiscriminator d =
                                     keyStringDiscriminator::INCLUSIVE,
                               BOOLEAN *isAllUndefined = nullptr ) ;
      template<typename T,
               typename = typename std::enable_if<std::is_unsigned<T>::value>::type>
      INT32 appendUnsignedWithoutType( const T &val,
                                       BOOLEAN isDescending = FALSE ) ;

      template<typename T,
               typename = typename std::enable_if<std::is_signed<T>::value>::type>
      INT32 appendSignedWithoutType( const T &val,
                                     BOOLEAN isDescending = FALSE ) ;

      INT32 appendRID( const dmsRecordID &rid,
                       BOOLEAN force = FALSE ) ;
      INT32 appendIndexID( const dmsIdxMetadataKey &indexID,
                           BOOLEAN force = FALSE ) ;
      INT32 appendLSN( UINT64 lsn, BOOLEAN force = FALSE ) ;

      INT32 buildPredicate( const VEC_ELE_CMP &elements,
                            const bson::Ordering &o,
                            const VEC_BOOLEAN &im,
                            BOOLEAN forward,
                            utilSlice keyHeader = utilSlice() ) ;

      INT32 buildPredicate( const bson::BSONObj &key,
                            const bson::Ordering &o,
                            const dmsRecordID &recordID,
                            BOOLEAN forward,
                            keyStringDiscriminator d,
                            utilSlice keyHeader = utilSlice() ) ;

      INT32 buildPredicate( const bson::BSONObj &key,
                            UINT32 prefixNum,
                            const bson::Ordering &o,
                            BOOLEAN forward,
                            utilSlice keyHeader = utilSlice() ) ;

      INT32 buildIndexEntryKey( const bson::BSONObj &key,
                                const bson::Ordering &o,
                                const dmsRecordID &rid,
                                const dmsIdxMetadataKey *indexid = nullptr,
                                const UINT64 *lsn = nullptr,
                                BOOLEAN *isAllUndefined = nullptr ) ;

      INT32 buildForKeyStringFunc( const bson::BSONElement &ele,
                                   INT32 direction ) ;

      INT32 rebuildEntryKey( const keyString &ks,
                             const dmsRecordID &rid,
                             utilSlice keyHeader = utilSlice() ) ;

      static INT32 buildBoundaryKey( const dmsIdxMetadataKey &indexID,
                                     BOOLEAN asUpBound,
                                     UINT32 bufSize,
                                     CHAR *buf,
                                     UINT32 &size ) ;

   public:
      INT32 done() ;
      UINT8 getVersion() const
      {
         return (UINT8)( keyStringVersion::version_1 ) ;
      }
      keyString getShallowKeyString() const ;
      keyString reap() ;

   protected:
      INT32 _appendBool( BOOLEAN val, BOOLEAN invert ) ;
      INT32 _appendDate( const bson::Date_t &val, BOOLEAN invert ) ;
      INT32 _appendTimestamp( const bson::BSONElement &elem,
                              BOOLEAN invert ) ;
      INT32 _appendTime( INT64 seconds, UINT32 microseconds,
                         BOOLEAN invert ) ;
      INT32 _appendOID( const bson::OID &val, BOOLEAN invert ) ;
      INT32 _appendString( const bson::StringData &val, BOOLEAN invert,
                           const stringTransformFn &f = nullptr ) ;
      INT32 _appendSymbol( const bson::StringData &val, BOOLEAN invert ) ;
      INT32 _appendCode( const bson::StringData &val, BOOLEAN invert ) ;
      INT32 _appendCodeWScope( const bson::StringData &code,
                               const bson::BSONObj &scope, BOOLEAN invert ) ;
      INT32 _appendBinData( const CHAR *data, UINT32 dataSize,
                            bson::BinDataType type, BOOLEAN invert ) ;
      INT32 _appendRegex( const bson::StringData &regex,
                          const bson::StringData &flags, BOOLEAN invert ) ;
      INT32 _appendDBRef( const bson::StringData &dbrefNS,
                          const bson::OID &dbrefOID, BOOLEAN invert ) ;
      INT32 _appendArray( const bson::BSONArray &val, BOOLEAN invert,
                          const stringTransformFn &f = nullptr ) ;
      INT32 _appendObject( const bson::BSONObj &val, BOOLEAN invert,
                           const stringTransformFn &f = nullptr ) ;
      INT32 _appendNumberDouble( const FLOAT64 num, BOOLEAN invert ) ;
      INT32 _appendNumberInt( const INT32 num, BOOLEAN invert ) ;
      INT32 _appendNumberLong( const INT64 num, BOOLEAN invert ) ;
      INT32 _appendNumberDecimal( const bson::bsonDecimal &dec,
                                  BOOLEAN invert ) ;
      INT32 _appendDecimalEncoding( const bson::bsonDecimal &dec,
                                    BOOLEAN invert ) ;
      INT32 _appendBsonValue( const bson::BSONElement &elem,
                              const bson::StringData *name, BOOLEAN invert,
                              const stringTransformFn &f = nullptr ) ;

      INT32 _appendStringLike( const bson::StringData &str,
                               BOOLEAN invert ) ;
      INT32 _appendBson( const bson::BSONObj &obj, BOOLEAN invert,
                         const stringTransformFn &f = nullptr ) ;
      INT32 _appendSmallDouble( FLOAT64 value, keyStringContinuousMarker dcm,
                                BOOLEAN invert ) ;
      INT32 _appendLargeDouble( FLOAT64 value, keyStringContinuousMarker dcm,
                                BOOLEAN invert ) ;
      INT32 _appendInteger( const INT64 num, BOOLEAN invert ) ;
      INT32 _appendPreshiftedInteger( UINT64 value, BOOLEAN isNegative,
                                      BOOLEAN invert ) ;

      INT32 _appendDoubleWithoutTypeBits( FLOAT64 num,
                                          keyStringContinuousMarker dcm,
                                          BOOLEAN invert ) ;
      INT32 _appendHugeDecimalWithoutTypeBits( const bson::bsonDecimal &dec,
                                               BOOLEAN invert ) ;
      INT32 _appendTinyDecimalWithoutTypeBits( const bson::bsonDecimal &dec,
                                               FLOAT64 bin,
                                               BOOLEAN invert ) ;
      INT32 _appendBytes( const void *source, UINT32 len, BOOLEAN invert ) ;

      template<typename T>
      INT32 _append( const T &t, BOOLEAN invert )
      {
         return _appendBytes( &t, sizeof( t ), invert ) ;
      }

      INT32 _appendDiscriminator( keyStringDiscriminator d ) ;
      INT32 _appendTypeBits() ;
      INT32 _appendMetaBlock() ;
      INT32 _appendMetaBlock( UINT32 keyHeaderSize, UINT32 keyElementsSize,
                              UINT32 keyTailSize, UINT32 typeBitsSize ) ;

      void _verifyStatus() ;
      void _transition( keyStringBuilderStatus to ) ;
      INT32 _ensureBytes( UINT32 length ) ;

      INT32 _appendMetaBlockSizeWord( UINT32 size, BOOLEAN nonzero ) ;

   private:
      keyStringBuilderStatus _status = keyStringBuilderStatus::EMPTY ;
      _typeBitsBuilder _typeBits ;
      CHAR *_buf = nullptr ;
      UINT32 _bufSize = 0 ;
      UINT32 _sizeAheadElements = 0 ;
      UINT32 _sizeOfElements = 0 ;
      UINT32 _sizeAfterElements = 0 ;
      UINT32 _typeBitsSize = 0 ;
      UINT32 _capacity = 0 ;
      utilStreamAllocator &_allocator ;
   } ;

   typedef class _keyStringBuilderImpl keyStringBuilderImpl ;

   template<typename T, typename >
   INT32 _keyStringBuilderImpl::appendUnsignedWithoutType( const T &val,
                                                           BOOLEAN isDescending )
   {
      INT32 rc = SDB_OK ;
      if ( keyStringBuilderStatus::EMPTY == _status ||
           keyStringBuilderStatus::BEFORE_ELEMENTS == _status )
      {
         _transition( keyStringBuilderStatus::BEFORE_ELEMENTS ) ;
      }
      else if ( keyStringBuilderStatus::APPENDING_ELEMENTS == _status )
      {
         _transition( keyStringBuilderStatus::AFTER_ELEMENTS ) ;
      }

      rc = _append( ossNativeToBigEndian( val ), isDescending ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append unsigned fixed field, rc: %d", rc ) ;
         goto error ;
      }
      if ( keyStringBuilderStatus::BEFORE_ELEMENTS == _status )
      {
         SDB_ASSERT(
               _sizeAheadElements + sizeof( val ) <= 0xff,
               "size ahead key string must be equal or less than 255" ) ;
         _sizeAheadElements += sizeof( val ) ;
      }
      else if ( keyStringBuilderStatus::AFTER_ELEMENTS == _status )
      {
         SDB_ASSERT(
               _sizeAfterElements + sizeof( val ) <= 0xff,
               "size ahead key string must be equal or less than 255" ) ;
         _sizeAfterElements += sizeof( val ) ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   template<typename T, typename >
   INT32 _keyStringBuilderImpl::appendSignedWithoutType( const T &val,
                                                         BOOLEAN isDescending )
   {
      INT32 rc = SDB_OK ;
      T mask = std::numeric_limits < T > ::min() ;
      T tmp = val ;
      if ( keyStringBuilderStatus::DONE == _status )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG( PDERROR, "building process has already done" ) ;
         goto error ;
      }
      else if ( keyStringBuilderStatus::EMPTY == _status )
      {
         _transition( keyStringBuilderStatus::BEFORE_ELEMENTS ) ;
      }
      else if ( keyStringBuilderStatus::APPENDING_ELEMENTS == _status )
      {
         _transition( keyStringBuilderStatus::AFTER_ELEMENTS ) ;
      }

      tmp ^= mask ;
      rc = _append( ossNativeToBigEndian( tmp ), isDescending ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to append signed fixed field, rc: %d", rc ) ;
         goto error ;
      }
      if ( keyStringBuilderStatus::BEFORE_ELEMENTS == _status )
      {
         SDB_ASSERT(
               _sizeAheadElements + sizeof( val ) <= 0xff,
               "size ahead key string must be equal or less than 255" ) ;
         _sizeAheadElements += sizeof( val ) ;
      }
      else if ( keyStringBuilderStatus::AFTER_ELEMENTS == _status )
      {
         SDB_ASSERT(
               _sizeAfterElements + sizeof( val ) <= 0xff,
               "size ahead key string must be equal or less than 255" ) ;
         _sizeAfterElements += sizeof( val ) ;
      }

   done:
      return rc ;

   error:
      reset() ;
      goto done ;
   }

   /*
      _keyStringBuilder define
    */
   template<typename Allocator = utilStreamStackAllocator>
   class _keyStringBuilder : public _keyStringBuilderImpl
   {
   public:
      _keyStringBuilder()
      : _keyStringBuilderImpl( _allocator )
      {
      }

      ~_keyStringBuilder()
      {
         reset() ;
      }

      _keyStringBuilder( const _keyStringBuilder& ) = delete ;
      _keyStringBuilder operator=( const _keyStringBuilder& ) = delete ;

   protected:
      Allocator _allocator ;
   } ;

   using keyStringStackBuilder = _keyStringBuilder<utilStreamStackAllocator> ;
   using keyStringBuilder = _keyStringBuilder<utilStreamPoolAllocator> ;

}
}

#endif // UTIL_KEY_STRING_BUILDER_H_
