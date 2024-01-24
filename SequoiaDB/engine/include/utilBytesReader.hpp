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

   Source File Name = utilBytesReader.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_BYTES_READER_HPP_
#define UTIL_BYTES_READER_HPP_

#include "ossUtil.hpp"
#include "utilSlice.hpp"
#include "utilStrictBuffer.hpp"

namespace engine
{

   /*
      _utilBytesReader define
    */
   class _utilBytesReader
   {
   public:
      _utilBytesReader() = default ;
      ~_utilBytesReader() = default ;
      _utilBytesReader( const _utilBytesReader & ) = default ;
      _utilBytesReader &operator =( const _utilBytesReader & ) = default ;

      _utilBytesReader( const utilSlice &bytes, BOOLEAN reverse = FALSE )
      : _reverse( reverse ),
        _bytes( bytes ),
        _offset( ( reverse ) ? ( static_cast<INT64>( _bytes.getSize() ) - 1 ) : ( 0 ) )
      {
      }

      _utilBytesReader &operator ++()
      {
         slide(1) ;
         return *this ;
      }

   public:
      void init( const utilSlice &bytes, BOOLEAN reverse = FALSE )
      {
         _reverse = reverse ;
         _bytes = bytes ;
         _offset = ( reverse ) ? ( static_cast<INT64>( _bytes.getSize() ) - 1 ) : ( 0 ) ;
      }

      void reset()
      {
         _reverse = FALSE ;
         _bytes.reset() ;
         _offset = 0 ;
      }

      template<class T>
      BOOLEAN isReadale() const
      {
         return !_isOutOfBound( sizeof( T ) ) ;
      }

      template<class T>
      const T &get() const
      {
         const T *ptr = nullptr ;
         utilStrictBuffer buffer( _bytes.getSize(), _bytes.getData() ) ;
         if ( 0 <= _offset )
         {
            ptr = buffer.getReadableObjPtr<T>( static_cast<UINT32>( _offset ) ) ;
         }
         SDB_ASSERT( nullptr != ptr, "not readable" ) ;
         return *ptr ;
      }

      CHAR getChar() const
      {
         return get<CHAR>() ;
      }

      INT8 getINT8() const
      {
         return get<INT8>() ;
      }

      UINT8 getUINT8() const
      {
         return get<UINT8>() ;
      }

      BOOLEAN slide( UINT32 size = 1 )
      {
         if ( _reverse )
         {
            _offset -= size ;
         }
         else
         {
            _offset += size ;
         }

         return !_isOutOfBound( 1 ) ;
      }

      BOOLEAN isOutOfBound() const
      {
         return _isOutOfBound( 1 ) ;
      }

   private:
      OSS_INLINE BOOLEAN _isOutOfBound( UINT32 size ) const
      {
         return ( 0 > _offset ) ||
                ( ( _offset + size ) > static_cast<INT64>( _bytes.size() ) ) ;
      }

   private:
      BOOLEAN _reverse = FALSE ;
      utilSlice _bytes ;
      INT64 _offset = 0 ;
   } ;

   typedef class _utilBytesReader utilBytesReader ;

}


#endif // UTIL_BYTES_READER_HPP_