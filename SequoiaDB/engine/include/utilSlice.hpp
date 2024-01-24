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

   Source File Name = utilSlice.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_SLICE_HPP_
#define UTIL_SLICE_HPP_

#include "ossUtil.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _utilSlice define
    */
   class _utilSlice : public SDBObject
   {
   public:
      _utilSlice() = default ;
      ~_utilSlice() = default ;
      _utilSlice( const _utilSlice &r ) = default ;
      _utilSlice& operator=( const _utilSlice &r ) = default ;

      explicit _utilSlice( UINT32 size, const void *data )
      : _size( size ),
        _data( (const CHAR *)data )
      {
      }

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return nullptr != _data && 0 < _size ;
      }

      OSS_INLINE UINT32 getSize() const
      {
         return _size ;
      }

      OSS_INLINE UINT32 size() const
      {
         return _size ;
      }

      OSS_INLINE const CHAR *getData() const
      {
         return _data ;
      }

      OSS_INLINE const CHAR *data() const
      {
         return _data ;
      }

      OSS_INLINE void reset()
      {
         _size = 0 ;
         _data = nullptr ;
         return ;
      }

      OSS_INLINE void reset( UINT32 size, const void *data )
      {
         _size = size ;
         _data = (const CHAR *)data ;
         return ;
      }

      OSS_INLINE _utilSlice getSlice( UINT32 offset, UINT32 size ) const
      {
         _utilSlice s ;
         if ( ( offset + size ) <= _size )
         {
            s.reset( size, _data + offset ) ;
         }
         return s ;
      }

      OSS_INLINE _utilSlice getSliceFromOffsetToEnd( UINT32 offset ) const
      {
         _utilSlice s ;
         if ( offset < _size )
         {
            s.reset( _size - offset, _data + offset ) ;
         }
         return s ;
      }

      OSS_INLINE INT32 compare( const _utilSlice &o ) const
      {
         UINT32 n = OSS_MIN( _size, o._size ) ;
         INT32 res = ossMemcmp( _data, o._data, n ) ;
         if ( 0 == res )
         {
            if ( _size < o._size )
            {
               res = -1 ;
            }
            else if ( _size > o._size )
            {
               res = 1 ;
            }
         }

         return res ;
      }

      OSS_INLINE INT32 compare( const _utilSlice &prefix,
                                const _utilSlice &suffix ) const
      {
         UINT32 n = OSS_MIN( _size, prefix._size ) ;
         INT32 res = ossMemcmp( _data, prefix._data, n ) ;
         if ( 0 == res )
         {
            if ( n < prefix.size() )
            {
               return -1 ;
            }
            else
            {
               UINT32 remain = _size - n ;
               UINT32 m = OSS_MIN( remain, suffix._size ) ;
               res = ossMemcmp( _data + n, suffix._data, m ) ;
               if ( 0 == res )
               {
                  if ( remain < suffix._size )
                  {
                     return -1 ;
                  }
                  else if ( remain == suffix._size )
                  {
                     return 0 ;
                  }
                  else
                  {
                     return 1 ;
                  }
               }
               else
               {
                  return res ;
               }
            }
         }
         else
         {
            return res ;
         }
      }

      OSS_INLINE BOOLEAN equal( const _utilSlice &o ) const
      {
         return _size == o._size && 0 == ossMemcmp( _data, o._data, _size ) ;
      }

      OSS_INLINE _utilSlice commonPrefix( const _utilSlice &r ) const
      {
         UINT32 len = 0 ;
         UINT32 minLen = _size < r._size ? _size : r._size ;
         while ( len < minLen && _data[ len ] == r._data[ len ] )
         {
            len ++ ;
         }
         return _utilSlice( len, _data ) ;
      }

      template<class T>
      const T* castTo( UINT32 offset = 0 ) const
      {
         return
               ( sizeof(T) + offset ) <= _size ?
                     reinterpret_cast<const T*>( _data ) : nullptr ;
      }

      ossPoolString toPoolString() const
      {
         ossPoolStringStream ss ;
         ss << std::hex ;
         for ( UINT32 i = 0 ; i < _size ; ++ i )
         {
            if ( i > 0 )
            {
               ss << " " ;
            }
            ss << (UINT32)( (UINT8)( _data[ i ] ) ) ;
         }
         return ss.str() ;
      }

   private:
      UINT32 _size = 0 ;
      const CHAR *_data = nullptr ;
   } ;

   typedef class _utilSlice utilSlice ;

}

#endif // UTIL_SLICE_HPP_
