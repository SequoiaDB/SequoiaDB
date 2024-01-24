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

   Source File Name = utilKeyStringMetaByte.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_KEY_STRING_META_BYTE_HPP_
#define UTIL_KEY_STRING_META_BYTE_HPP_

#include "ossUtil.hpp"
#include <limits>

namespace engine
{

namespace keystring
{
   /*
      _keyStringMetaByte define
    */

#pragma pack(1)

   class _keyStringMetaByte
   {
      public:
      _keyStringMetaByte() = default ;

      explicit _keyStringMetaByte( UINT32 keyHeadSize,
                                   UINT32 keyTailSize,
                                   UINT32 typeBitsSize )
      : _b( 0 )
      {
         if ( 0 < keyHeadSize )
         {
            setHasKeyHead() ;
         }
         if ( 0 < keyTailSize )
         {
            setHasKeyTail() ;
         }
         if ( 0 < typeBitsSize )
         {
            setHasTypeBits() ;
         }
      }

      explicit _keyStringMetaByte( UINT8 b )
      : _b(b)
      {
      }

   public:
      enum FLAG : UINT8
      {
         HAS_KEY_HEAD = 0x01,
         HAS_KEY_TAIL = 0x02,
         HAS_TYPE_BITS = 0x04,
      } ;

   public:
      OSS_INLINE void init( UINT8 b )
      {
         _b = b ;
      }

      OSS_INLINE UINT8 getValue() const
      {
         return _b ;
      }

      OSS_INLINE void setHasKeyHead()
      {
         OSS_BIT_SET( _b, HAS_KEY_HEAD ) ;
      }

      OSS_INLINE BOOLEAN hasKeyHead() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_KEY_HEAD ) ;
      }

      OSS_INLINE void setHasKeyTail()
      {
         OSS_BIT_SET( _b, HAS_KEY_TAIL ) ;
      }

      OSS_INLINE BOOLEAN hasKeyTail() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_KEY_TAIL ) ;
      }

      OSS_INLINE void setHasTypeBits()
      {
         OSS_BIT_SET( _b, HAS_TYPE_BITS ) ;
      }

      OSS_INLINE BOOLEAN hasTypeBits() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_TYPE_BITS ) ;
      }

   private:
      UINT8 _b = 0 ;
   } ;

   static_assert( 1 == sizeof( _keyStringMetaByte ), "invalid size" ) ;
   typedef class _keyStringMetaByte keyStringMetaByte ;

#pragma pack()

}
}

#endif // UTIL_KEY_STRING_META_BYTE_HPP_
