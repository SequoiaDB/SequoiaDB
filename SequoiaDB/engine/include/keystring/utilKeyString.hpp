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

   Source File Name = utilKeyString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft
          07/20/2022  ZHY  Implement
   Last Changed =

*******************************************************************************/

#ifndef UTIL_KEY_STRING_HPP_
#define UTIL_KEY_STRING_HPP_

#include "ossUtil.hpp"
#include "dms.hpp"
#include "dmsMetadata.hpp"
#include "utilBytesReader.hpp"
#include "utilSlice.hpp"
#include "ossMemPool.hpp"
#include "keystring/utilKeyStringDef.hpp"
#include "../bson/bson.hpp"
#include "../bson/ordering.h"


namespace engine
{
namespace keystring
{

   /*
      _keyString define
    */
   class _keyString
   {
      friend class _keyStringBuilderImpl ;

   public:
      _keyString() = default ;
      ~_keyString() ;
      explicit _keyString( const utilSlice &data,
                           const utilSlice &typeBits = utilSlice() ) ;
      explicit _keyString( UINT32 size, const CHAR *data ) ;

      /// WARNING: shallow copy!
      _keyString( const _keyString &o ) ;
      _keyString &operator =( const _keyString &o ) ;

      _keyString( _keyString && o ) noexcept ;
      _keyString &operator =( _keyString &&o ) noexcept ;

   public:
      void reset() ;
      INT32 init( const utilSlice &data,
                  const utilSlice &tail = utilSlice() ) ;
      INT32 getOwned() ;

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return _ref.isValid() ;
      }

      OSS_INLINE BOOLEAN isOwned() const
      {
         return nullptr != _bufferOwned ;
      }

      OSS_INLINE const utilSlice &getRawData() const
      {
         return _ref ;
      }

      OSS_INLINE const CHAR *getRawDataPtr() const
      {
         return _ref.data() ;
      }

      OSS_INLINE UINT32 getRawDataSize() const
      {
         return _ref.getSize() ;
      }

      OSS_INLINE const utilSlice &getTailData() const
      {
         return _tailRef ;
      }

      OSS_INLINE const CHAR *getTailDataPtr() const
      {
         return _tailRef.data() ;
      }

      OSS_INLINE UINT32 getTailDataSize() const
      {
         return _tailRef.getSize() ;
      }

      OSS_INLINE UINT32 getComparableSize() const
      {
         return _desc.keySize ;
      }

      OSS_INLINE UINT32 getKeySize() const
      {
         return _desc.keySize ;
      }

      OSS_INLINE UINT32 getKeyHeadSize() const
      {
         return _desc.keyHeadSize ;
      }

      OSS_INLINE UINT32 getKeyTailSize() const
      {
         return _desc.keyTailSize ;
      }

      OSS_INLINE UINT32 getKeyElementsSize() const
      {
         return _desc.getKeyElementsSize() ;
      }

      OSS_INLINE UINT32 getKeySizeAfterHeader() const
      {
         return getKeySize() - getKeyHeadSize() ;
      }

      OSS_INLINE UINT32 getTypeBitsSize() const
      {
         return _desc.typeBitsSize ;
      }

      OSS_INLINE UINT32 getKeySizeExceptTail() const
      {
         return _desc.keySize - _desc.keyTailSize ;
      }

      OSS_INLINE BOOLEAN hasKeyHead() const
      {
         return 0 < getKeyHeadSize() ;
      }

      OSS_INLINE BOOLEAN hasKeyElements() const
      {
         return 0 < _desc.getKeyElementsSize() ;
      }

      OSS_INLINE BOOLEAN hasKeyTail() const
      {
         return 0 < getKeyTailSize() ;
      }

      OSS_INLINE BOOLEAN hasTypeBits() const
      {
         return 0 < getTypeBitsSize() ;
      }

      utilSlice getKeySlice() const ;
      utilSlice getKeyHeadSlice() const ;
      utilSlice getKeyElementsSlice() const ;
      utilSlice getKeyTailSlice() const ;
      utilSlice getKeySliceExceptTail() const ;
      utilSlice getKeySliceAfterHeader() const ;
      utilSlice getSliceAfterKeyElements() const ;
      utilSlice getSliceAfterKey() const ;
      utilSlice getTypeBits() const ;
      bson::BSONObj toBSON( const bson::BSONObj &pattern,
                            BOOLEAN withFieldName = FALSE ) const ;
      bson::BSONObj toBSON( const bson::BSONObj &pattern,
                            bson::BufBuilder &builder,
                            BOOLEAN withFieldName = FALSE ) const ;

   public:
      dmsRecordID getRID() const ;

   public:
      ///WARNING: same code format only!
      INT32 compare( const _keyString &ks ) const ;

      INT32 compareElements( const _keyString &ks ) const ;

      ///WARNING: same code format only!
      ///WARNING: will not do any validation inside!
      static INT32 compareCoding( UINT32 sizea, const CHAR *bufa,
                                  UINT32 sizeb, const CHAR *bufb ) ;

      static INT32 parseMetaFromSlice( const utilSlice &metadataSlice,
                                       keyStringDescriptor &desc ) ;

      /*
         _typeBitsReader define
       */
      class _typeBitsReader : public SDBObject
      {
      public:
         _typeBitsReader() = default ;
         _typeBitsReader( const CHAR *buf, UINT32 bufSize ) ;
         ~_typeBitsReader() = default ;
         _typeBitsReader operator=( const _typeBitsReader& ) = delete ;
         _typeBitsReader( const _typeBitsReader& ) = delete ;

      public:
         keyStringTypeBitsType readNumeric() ;
         keyStringTypeBitsType readZero() ;
         keyStringTypeBitsType readStringLike() ;
         keyStringTypeBitsType readTimestampOrDate() ;
         void readBitsAndAssign( CHAR *dst, UINT32 bytesSize ) ;
         UINT8 readByte() ;

         template<typename T>
         T read()
         {
            T t ;
            UINT8 *ptr = reinterpret_cast<UINT8 *>( &t ) ;
            for ( UINT32 i = 0 ; i < sizeof( t ) ; ++ i )
            {
               UINT8 byte = readByte() ;
               ossMemcpy( ptr + i, &byte, 1 ) ;
            }
            return t ;
         }

         template<typename T>
         void skip()
         {
            for ( UINT32 i = 0 ; i < sizeof( T ) ; ++ i )
            {
               readByte() ;
            }
         }

      private:
         UINT8 _readBit() ;

      private:
         const CHAR *_buf = nullptr ;
         UINT32 _bufSize = 0 ;
         UINT32 _curBit = 0 ;
      } ;

      /*
         _bodyReader define
       */
      class _bodyReader : public SDBObject
      {
      public:
         _bodyReader( const CHAR *bodyBuf,
                      UINT32 bufSize,
                      const CHAR *typeBitsBuf,
                      UINT32 typeBitsBufSize ) ;

      public:
         bson::BSONObj toBSON( const bson::BSONObj &pattern,
                               BSONObjBuilder &bufBuilder,
                               BOOLEAN withFieldName = FALSE ) ;
         void toBSONValue( keyStringEncodedType type,
                           BOOLEAN inverted,
                           bson::BSONObjBuilder &builder,
                           const CHAR *fieldName = nullptr ) ;

      private:
         template<typename T>
         T _read( BOOLEAN inverted ) ;

         template<typename T>
         T _peek( BOOLEAN inverted ) const ;

         void _readBytes( BOOLEAN inverted, CHAR *bytes, UINT32 len ) ;
         void _readCString( BOOLEAN inverted, ossPoolString &s ) ;
         void _toBSON( BOOLEAN inverted,
                       bson::BSONObjBuilder &builder,
                       const CHAR *fieldName = nullptr ) ;
         bson::BSONObj _toBSON( BOOLEAN inverted, BOOLEAN withFieldName ) ;

         void _toNumeric( keyStringEncodedType type,
                          BOOLEAN inverted,
                          bson::BSONObjBuilder &builder,
                          const CHAR *fieldName = nullptr ) ;
         void _decodeStringLike( BOOLEAN inverted, ossPoolString &s ) ;
         bson::bsonDecimal _decodeDecimal( BOOLEAN inverted,
                                           BOOLEAN isNegative ) ;

      private:
         const CHAR *_buf = nullptr ;
         UINT32 _bufSize = 0 ;
         UINT32 _offset = 0 ;
         _typeBitsReader _typeReader ;
      } ;

   private:
         INT32 _parse( const utilSlice &data,
                       const utilSlice &typeBits,
                       keyStringDescriptor &desc ) const ;
         INT32 _getOwnedWithException() const ;
   private:
         static BOOLEAN _loadSizeData( utilBytesReader &reader,
                                       BOOLEAN nonzero,
                                       UINT32 &size ) ;

   protected:
      utilSlice _ref ;
      utilSlice _tailRef ;
      keyStringDescriptor _desc ;

   private:
      CHAR *_bufferOwned = nullptr ;
      UINT32 _bufferSize = 0 ;
   } ;

   typedef class _keyString keyString ;

}
}

#endif // UTIL_KEY_STRING_HPP_
