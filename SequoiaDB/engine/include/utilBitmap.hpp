/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

   #define UTIL_BITMAP_UNIT_SIZE       ( 8 )

   #define UTIL_BITMAP_UNIT_MODULO     ( 7 )

   #define UTIL_BITMAP_UNIT_LOG2SIZE   ( 3 )

   static UINT8 _utilBitmapIndex[ UTIL_BITMAP_UNIT_SIZE ] =
   {
       0x01,
       0x02,
       0x04,
       0x08,
       0x10,
       0x20,
       0x40,
       0x80
   } ;

   /*
      _utilBitmapBase define and implement
    */
   class _utilBitmapBase : public SDBObject
   {
      protected :
         _utilBitmapBase ()
         : _size( 0 ),
           _bitmapSize( 0 ),
           _bitmap( NULL )
         {
         }

         virtual ~_utilBitmapBase ()
         {
            _size = 0 ;
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
               OSS_BIT_SET( _bitmap[ unitIndex ],
                            _utilBitmapIndex[ bitIndex ] ) ;
            }
         }

         OSS_INLINE void clearBit ( UINT32 index )
         {
            if ( index < _size )
            {
               UINT32 unitIndex = _calcUnitIndex( index ) ;
               UINT8 bitIndex = _calcBitIndex( index ) ;
               OSS_BIT_CLEAR( _bitmap[ unitIndex ],
                              _utilBitmapIndex[ bitIndex ] ) ;
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

      protected :
         OSS_INLINE UINT32 _calcUnitIndex ( UINT32 index ) const
         {
            return index >> UTIL_BITMAP_UNIT_LOG2SIZE ;
         }

         OSS_INLINE UINT8 _calcBitIndex ( UINT32 index ) const
         {
            return index & UTIL_BITMAP_UNIT_MODULO ;
         }

      protected :
         UINT32   _size ;
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
         OSS_INLINE virtual void _allocateBitmap ( UINT32 size )
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

         OSS_INLINE virtual void _freeBitmap ()
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
   template < UINT32 BITMAPSIZE >
   class _utilStackBitmap : public _utilBitmapBase
   {
      public :
         _utilStackBitmap ()
         : _utilBitmapBase()
         {
            _size = BITMAPSIZE * UTIL_BITMAP_UNIT_SIZE ;
            _bitmapSize = BITMAPSIZE ;
            if ( BITMAPSIZE > 0 )
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
         UINT8 _bitmapBuf[ BITMAPSIZE ] ;
   } ;

}

#endif //UTILBITMAP_HPP__

