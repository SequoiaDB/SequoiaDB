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

   Source File Name = utilKeyStringDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_KEY_STRING_DEF_HPP_
#define UTIL_KEY_STRING_DEF_HPP_

#include "ossUtil.hpp"

namespace engine
{
namespace keystring
{
   /*
      keyStringVersion define
    */
   enum class keyStringVersion : UINT8
   {
      invalid = 0x0,
      version_1 = 0x01,
   } ;

   /*
      _keyStringMetaBlockHeader define
    */

#pragma pack(2)

   class _keyStringMetaBlockHeader
   {
   public:
      _keyStringMetaBlockHeader() = default ;
      ~_keyStringMetaBlockHeader() = default ;

      OSS_INLINE BOOLEAN isValid() const
      {
         return (UINT8)( keyStringVersion::version_1 ) == version ;
      }

      UINT8 metaByte = 0 ;
      UINT8 version = (UINT8)( keyStringVersion::invalid ) ;
   } ;

   constexpr UINT32 KEY_STRING_MB_HEADER_SIZE = sizeof( _keyStringMetaBlockHeader ) ;
   static_assert( 2 == KEY_STRING_MB_HEADER_SIZE, "invalid size" ) ;
   typedef class _keyStringMetaBlockHeader keyStringMetaBlockHeader ;

#pragma pack()

   constexpr UINT32 KEY_STRING_TYNI_SIZE_BOUND = 0xFF ;
   constexpr UINT32 KEY_STRING_TYNI_SWRORD_SIZE = sizeof(UINT8) ;
   constexpr UINT32 KEY_STRING_SWORD_SIZE = sizeof(UINT32) ;
   constexpr UINT32 KEY_STRING_EXT_SWORD_SIZE = KEY_STRING_TYNI_SWRORD_SIZE + KEY_STRING_SWORD_SIZE ;
   constexpr UINT32 KEY_STRING_ONLY_END_SIZE = 1 ;
   constexpr UINT32 KEY_STRING_DISCRIMINATOR_AND_END_SIZE = 2 ;

   class _keyStringDescriptor
   {
   public:
      OSS_INLINE void reset()
      {
         keySize = 0;
         keyHeadSize = 0;
         keyTailSize = 0;
         typeBitsSize = 0;
      }

      OSS_INLINE BOOLEAN isValid() const
      {
         return ( (UINT64)keyHeadSize + keyTailSize ) <= (UINT64)keySize ;
      }

      OSS_INLINE UINT32 getKeyElementsSize() const
      {
         return keySize - ( keyHeadSize + keyTailSize ) ;
      }

      OSS_INLINE UINT32 getMetaBlockSize() const
      {
         UINT32 size = KEY_STRING_MB_HEADER_SIZE ;

         size += keySize < KEY_STRING_TYNI_SIZE_BOUND ?
                 KEY_STRING_TYNI_SWRORD_SIZE :
                 KEY_STRING_EXT_SWORD_SIZE ;
         if ( 0 < keyHeadSize )
         {
            size += keyHeadSize < KEY_STRING_TYNI_SIZE_BOUND ?
                    KEY_STRING_TYNI_SWRORD_SIZE :
                    KEY_STRING_EXT_SWORD_SIZE ;
         }
         if ( 0 < keyTailSize )
         {
            size += keyTailSize < KEY_STRING_TYNI_SIZE_BOUND ?
                    KEY_STRING_TYNI_SWRORD_SIZE :
                    KEY_STRING_EXT_SWORD_SIZE ;
         }
         if ( 0 < typeBitsSize )
         {
            size += typeBitsSize < KEY_STRING_TYNI_SIZE_BOUND ?
                    KEY_STRING_TYNI_SWRORD_SIZE :
                    KEY_STRING_EXT_SWORD_SIZE ;
         }

         return size ;
      }

      OSS_INLINE UINT32 getStringSizeExpected() const
      {
         return keySize + typeBitsSize + getMetaBlockSize() ;
      }

      UINT32 keySize = 0 ;
      UINT32 keyHeadSize = 0 ;
      UINT32 keyTailSize = 0 ;
      UINT32 typeBitsSize = 0 ;
   } ;

   typedef class _keyStringDescriptor keyStringDescriptor ;

   enum class keyStringEncodedType : UINT8
   {
      minKey = 10,
      undefined = 15,
      nullish = 20,
      numeric = 30,
      numericNaN = numeric + 0,
      numericNegativeLargeMagnitude = numeric + 1,
      numericNegative8ByteInt = numeric + 2,
      numericNegative7ByteInt = numeric + 3,
      numericNegative6ByteInt = numeric + 4,
      numericNegative5ByteInt = numeric + 5,
      numericNegative4ByteInt = numeric + 6,
      numericNegative3ByteInt = numeric + 7,
      numericNegative2ByteInt = numeric + 8,
      numericNegative1ByteInt = numeric + 9,
      numericNegativeSmallMagnitude = numeric + 10,
      numericZero = numeric + 11,
      numericPositiveSmallMagnitude = numeric + 12,
      numericPositive1ByteInt = numeric + 13,
      numericPositive2ByteInt = numeric + 14,
      numericPositive3ByteInt = numeric + 15,
      numericPositive4ByteInt = numeric + 16,
      numericPositive5ByteInt = numeric + 17,
      numericPositive6ByteInt = numeric + 18,
      numericPositive7ByteInt = numeric + 19,
      numericPositive8ByteInt = numeric + 20,
      numericPositiveLargeMagnitude = numeric + 21,
      stringLike = 60,
      object = 70,
      array = 80,
      binData = 90,
      oid = 100,
      boolean = 110,
      booleanFalse = boolean + 0,
      booleanTrue = boolean + 1,
      time = 130,
      regEx = 140,
      dbRef = 150,
      code = 160,
      codeWithScope = 170,
      maxKey = 240
   } ;

   static_assert( keyStringEncodedType::numericPositiveLargeMagnitude <
                     keyStringEncodedType::stringLike,
                  "NumericPositiveLargeMagnitude must be less than StringLike" ) ;

   enum class keyStringContinuousMarker : UINT8
   {
      hasNoContinuation = 0b0,
      hasContinuation = 0b1,
   } ;

   enum class keyStringTypeBitsType : UINT8
   {
      STRING = 0b0,
      SYMBOL = 0b1,

      INT = 0b00,
      LONG = 0b01,
      DOUBLE = 0b10,
      DECIMAL = 0b11,
      POSITIVE_ZERO = 0b0,
      NEGATIVE_ZERO = 0b1,

      DATE = 0b00,
      TIMESTAMP = 0b01,
      TIMESTAMP2 = 0b10,
      RESERVED = 0b11
   } ;

   enum class keyStringDiscriminator : UINT8
   {
      INCLUSIVE,
      EXCLUSIVE_BEFORE,
      EXCLUSIVE_AFTER
   } ;

   enum class keyStringDiscriminatorValue : UINT8
   {
      LESS = 1,
      GREATER = 254,
      END = 4
   } ;

   constexpr INT32 _doublePrecision10 = std::numeric_limits<FLOAT64>::max_digits10 ;

}
}

#endif // UTIL_KEY_STRING_DEF_HPP_