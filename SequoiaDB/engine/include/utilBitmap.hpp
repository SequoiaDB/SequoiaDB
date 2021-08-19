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

   Source File Name = utilBitmap.hpp

   Descriptive Name = utility of bitmap header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for bitmap

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/07/2017  HGM Initial draft

   Last Changed =

*******************************************************************************/

#ifndef UTILBITMAP_HPP__
#define UTILBITMAP_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"

namespace engine
{

   // UINT8 as one unit in bitmap
   #define UTIL_BITMAP_UNIT_SIZE       ( 8 )

   // Modulo to index the unit in bitmap
   #define UTIL_BITMAP_UNIT_MODULO     ( ( UTIL_BITMAP_UNIT_SIZE ) - 1 )

   // log2(unit size) in a bitmap unit
   #define UTIL_BITMAP_UNIT_LOG2SIZE   ( 3 )

   // Search table for each bit in a bitmap unit
   static UINT8 _utilBitmapIndex[ UTIL_BITMAP_UNIT_SIZE ] =
   {
       0x80,   /// 10000000
       0x40,   /// 01000000
       0x20,   /// 00100000
       0x10,   /// 00010000
       0x08,   /// 00001000
       0x04,   /// 00000100
       0x02,   /// 00000010
       0x01    /// 00000001
   } ;

   /*
      _utilBitmapBase define and implement
    */
   class _utilBitmapBase : public SDBObject
   {
      protected :
         _utilBitmapBase ()
         : _size( 0 ),
           _freeSize( 0 ),
           _bitmapSize( 0 ),
           _bitmap( NULL )
         {
         }

         virtual ~_utilBitmapBase ()
         {
            _size = 0 ;
            _freeSize = 0 ;
            _bitmapSize = 0 ;
            _bitmap = NULL ;
         }

      public :
         OSS_INLINE void setBit ( UINT32 index )
         {
            if ( index < _size )
            {
               UINT32 unitIndex = _calcUnitIndex( index ) ;
               UINT8 bitIndex = _calcBitIndex( index ) ;

               if ( ! OSS_BIT_TEST( _bitmap[ unitIndex ],
                                    _utilBitmapIndex[ bitIndex ] ) )
               {
                  OSS_BIT_SET( _bitmap[ unitIndex ],
                               _utilBitmapIndex[ bitIndex ] ) ;
                  --_freeSize ;
               }
            }
         }

         OSS_INLINE void clearBit ( UINT32 index )
         {
            if ( index < _size )
            {
               UINT32 unitIndex = _calcUnitIndex( index ) ;
               UINT8 bitIndex = _calcBitIndex( index ) ;

               if ( OSS_BIT_TEST( _bitmap[ unitIndex ],
                                  _utilBitmapIndex[ bitIndex ] ) )
               {
                  OSS_BIT_CLEAR( _bitmap[ unitIndex ],
                                 _utilBitmapIndex[ bitIndex ] ) ;
                  ++_freeSize ;
               }
            }
         }

         OSS_INLINE BOOLEAN testBit ( UINT32 index ) const
         {
            if ( index < _size )
            {
               UINT32 unitIndex = _calcUnitIndex( index ) ;
               UINT8 bitIndex = _calcBitIndex( index ) ;
               return OSS_BIT_TEST( _bitmap[ unitIndex ],
                                    _utilBitmapIndex[ bitIndex ] ) ?
                      TRUE : FALSE ;
            }
            return FALSE ;
         }

         OSS_INLINE void resetBitmap ()
         {
            if ( NULL != _bitmap )
            {
               ossMemset( _bitmap, 0, _bitmapSize ) ;
               _freeSize = _size ;
            }
         }

         OSS_INLINE UINT32 getSize () const
         {
            return _size ;
         }

         OSS_INLINE void setBitmap ( const _utilBitmapBase & bitmap )
         {
            resetBitmap() ;

            UINT32 lhsIdx = 0, rhsIdx = 0 ;
            while ( lhsIdx < _bitmapSize &&
                    rhsIdx < bitmap._bitmapSize )
            {
               _bitmap[ lhsIdx ] = bitmap._bitmap[ rhsIdx ] ;
               lhsIdx ++ ;
               rhsIdx ++ ;
            }

            _calcFreeSize() ;
         }

         OSS_INLINE BOOLEAN hasIntersaction ( const _utilBitmapBase & bitmap ) const
         {
            UINT32 lhsIdx = 0, rhsIdx = 0 ;
            while ( lhsIdx < _bitmapSize &&
                    rhsIdx < bitmap._bitmapSize )
            {
               if ( OSS_BIT_TEST( _bitmap[ lhsIdx ],
                                  bitmap._bitmap[ rhsIdx ] ) )
               {
                  return TRUE ;
               }
               lhsIdx ++ ;
               rhsIdx ++ ;
            }
            return FALSE ;
         }

         /*
            -1 : for no found the position
         */
         OSS_INLINE INT32 nextFreeBitPos( UINT32 fromPos = 0 )
         {
            INT32 pos = -1 ;

            if ( _freeSize > 0 )
            {
               while ( fromPos < _size )
               {
                  if ( 0 == ( fromPos & UTIL_BITMAP_UNIT_MODULO ) &&
                       0xFF == _bitmap[ fromPos >> UTIL_BITMAP_UNIT_LOG2SIZE ] )
                  {
                     fromPos += UTIL_BITMAP_UNIT_SIZE ;
                  }
                  else if ( testBit( fromPos ) )
                  {
                     ++fromPos ;
                  }
                  else
                  {
                     pos = fromPos ;
                     break ;
                  }
               }
            }
            return pos ;
         }

         /*
            -1 : for no found the position
         */
         OSS_INLINE INT32 nextSetBitPos( UINT32 fromPos = 0 )
         {
            INT32 pos = -1 ;

            if ( _freeSize < _size )
            {
               while ( fromPos < _size )
               {
                  if ( 0 == ( fromPos & UTIL_BITMAP_UNIT_MODULO ) &&
                       0 == _bitmap[ fromPos >> UTIL_BITMAP_UNIT_LOG2SIZE ] )
                  {
                     fromPos += UTIL_BITMAP_UNIT_SIZE ;
                  }
                  else if ( !testBit( fromPos ) )
                  {
                     ++fromPos ;
                  }
                  else
                  {
                     pos = fromPos ;
                     break ;
                  }
               }
            }
            return pos ;
         }

         OSS_INLINE UINT32 freeSize() const
         {
            return _freeSize ;
         }

         OSS_INLINE BOOLEAN isEmpty() const
         {
            return _freeSize >= _size ? TRUE : FALSE ;
         }

         OSS_INLINE BOOLEAN isFull() const
         {
            return ( 0 == _freeSize && _size > 0 ) ? TRUE : FALSE ;
         }

      protected :
         OSS_INLINE UINT32 _calcUnitIndex ( UINT32 index ) const
         {
            // Find the index to the unit in bitmap
            return index >> UTIL_BITMAP_UNIT_LOG2SIZE ;
         }

         OSS_INLINE UINT8 _calcBitIndex ( UINT32 index ) const
         {
            // Find the index to the bit in one unit
            return index & UTIL_BITMAP_UNIT_MODULO ;
         }

         OSS_INLINE void  _calcFreeSize()
         {
            _freeSize = 0 ;
            UINT32 beginPos = 0 ;
            while ( beginPos < _size )
            {
               if ( 0 == ( beginPos & UTIL_BITMAP_UNIT_MODULO ) &&
                    0 == _bitmap[ beginPos >> UTIL_BITMAP_UNIT_LOG2SIZE ] )
               {
                  beginPos += UTIL_BITMAP_UNIT_SIZE ;
                  _freeSize += UTIL_BITMAP_UNIT_SIZE ;
               }
               else
               {
                  if ( !testBit( beginPos ) )
                  {
                     ++_freeSize ;
                  }
                  ++beginPos ;
               }
            }
         }

      protected :
         UINT32   _size ;
         UINT32   _freeSize ;
         UINT32   _bitmapSize ;
         UINT8 *  _bitmap ;
   } ;

   /*
      _utilBitmap define and implement
    */
   class _utilBitmap : public _utilBitmapBase
   {
      public :
         _utilBitmap ( UINT32 size )
         : _utilBitmapBase()
         {
            _allocateBitmap( size ) ;
         }

         virtual ~_utilBitmap ()
         {
            _freeBitmap() ;
         }

         virtual void resize ( UINT32 size )
         {
            if ( size != _size )
            {
               _freeBitmap() ;
               _allocateBitmap( size ) ;
            }
         }

      protected :
         OSS_INLINE void _allocateBitmap ( UINT32 size )
         {
            if ( size > 0 )
            {
               UINT32 bitmapSize = ( size + UTIL_BITMAP_UNIT_MODULO ) /
                                   UTIL_BITMAP_UNIT_SIZE ;
               _bitmap = (UINT8 *)SDB_OSS_MALLOC( bitmapSize ) ;
               if ( NULL != _bitmap )
               {
                  _size = size ;
                  _bitmapSize = bitmapSize ;
               }
            }

            resetBitmap() ;
         }

         OSS_INLINE void _freeBitmap ()
         {
            SAFE_OSS_FREE( _bitmap ) ;
            _size = 0 ;
            _bitmapSize = 0 ;
         }
   } ;

   typedef class _utilBitmap utilBitmap ;

   /*
      _utilStackBitmap define and implement
    */
   template < UINT32 SIZE >
   class _utilStackBitmap : public _utilBitmapBase
   {
      public :
         _utilStackBitmap ()
         : _utilBitmapBase()
         {
            _size = SIZE ;
            _bitmapSize = ( SIZE + UTIL_BITMAP_UNIT_MODULO ) >>
                          UTIL_BITMAP_UNIT_LOG2SIZE ;
            if ( _bitmapSize > 0 )
            {
               _bitmap = &( _bitmapBuf[0] ) ;
            }

            resetBitmap() ;
         }

         virtual ~_utilStackBitmap ()
         {
            _bitmap = NULL ;
         }

      protected :
         UINT8 _bitmapBuf[ ( SIZE + UTIL_BITMAP_UNIT_MODULO ) >>
                           UTIL_BITMAP_UNIT_LOG2SIZE ] ;
   } ;

}

#endif //UTILBITMAP_HPP__

