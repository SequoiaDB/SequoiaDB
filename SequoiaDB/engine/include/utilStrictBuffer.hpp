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

   Source File Name = utilStrictBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_STRICT_BUFFER_HPP_
#define UTIL_STRICT_BUFFER_HPP_

#include "utilSlice.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _utilStrictBuffer define
    */
   /// WARNING: _utilStrictBuffer does not own the memory!
   class _utilStrictBuffer : public SDBObject
   {
   public:
      _utilStrictBuffer() = default ;
      ~_utilStrictBuffer() = default ;

      explicit _utilStrictBuffer( UINT32 size, const void *data)
      : _size(size),
        _rptr( (const CHAR *)data ),
        _wptr( nullptr )
      {
      }

      _utilStrictBuffer( const _utilStrictBuffer &o )
      : _size( o._size ),
        _rptr( o._rptr ),
        _wptr( o._wptr )
      {
      }

      _utilStrictBuffer &operator =( const _utilStrictBuffer &o )
      {
         if ( &o != this )
         {
            _size = o._size ;
            _rptr = o._rptr ;
            _wptr = o._wptr ;
         }
         return *this ;
      }

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return nullptr != _rptr ;
      }

      OSS_INLINE BOOLEAN isWritable() const
      {
         return nullptr != _wptr ;
      }

      OSS_INLINE UINT32 getSize() const
      {
         return _size ;
      }

      OSS_INLINE const CHAR *getRPtr() const
      {
         return _rptr ;
      }

      OSS_INLINE CHAR *getWPtr ()
      {
         return _wptr ;
      }

      OSS_INLINE void reset ()
      {
         _size = 0 ;
         _rptr = nullptr ;
         _wptr = nullptr ;
      }

      OSS_INLINE void reset( UINT32 size, const void *buf )
      {
         SDB_ASSERT( nullptr != buf && 0 < size, "can not be invalid" ) ;
         _size = size ;
         _rptr = (const CHAR *)buf ;
         _wptr = nullptr ;
      }

      OSS_INLINE void makeWritable( UINT32 size, void *buf )
      {
         SDB_ASSERT( nullptr != buf && 0 < size, "can not be invalid" ) ;
         _size = size ;
         _rptr = (const CHAR *)buf ;
         _wptr = (CHAR *)buf ;
      }

   public:
      OSS_INLINE const CHAR *getReadablePtr( UINT32 offset, UINT32 size ) const
      {
         SDB_ASSERT( isValid(), "can not be invalid" ) ;
         return _isValidAccessing( offset, size ) ? ( _rptr + offset ) : nullptr ;
      }

      OSS_INLINE const CHAR *getReadablePtrWithoutSize( UINT32 offset ) const
      {
         SDB_ASSERT( isValid(), "can not be invalid" ) ;
         return _isValidAccessing( offset, 1 ) ? ( _rptr + offset ) : nullptr ;
      }

      OSS_INLINE CHAR *getWritablePtr( UINT32 offset, UINT32 size )
      {
         SDB_ASSERT( isWritable(), "must be writable" ) ;
         return ( isWritable() && _isValidAccessing( offset, size ) ) ?
                     ( _wptr + offset ) : nullptr ;
      }
      OSS_INLINE CHAR *getWritablePtrWithoutSize( UINT32 offset )
      {
         SDB_ASSERT( isWritable(), "must be writable" ) ;
         return ( isWritable() && _isValidAccessing( offset, 1 ) ) ?
                     ( _wptr + offset ) : nullptr ;
      }

   public:
      template<class T>
      const T *getReadableObjPtr( UINT32 offset ) const
      {
         return (const T *)( getReadablePtr( offset, sizeof( T ) ) ) ;
      }

      template<class T>
      T *getWritableObjPtr( UINT32 offset )
      {
         return (T *)( getWritablePtr( offset, sizeof( T ) ) ) ;
      }

   public:
      void setBuffer( CHAR v = 0x00 ) ;
      void setBuffer( UINT32 offset, UINT32 size, CHAR v = 0x00 ) ;
      INT32 write( UINT32 offset, UINT32 size, const void *data ) ;
      INT32 read( UINT32 offset, UINT32 size, void *data ) const ;
      utilSlice getSlice( UINT32 offset, UINT32 size ) const ;
      utilSlice getSlice() const ;
      _utilStrictBuffer getReadableBuffer() const ;
      _utilStrictBuffer getReadableBuffer( UINT32 size, UINT32 offset ) const ;
      _utilStrictBuffer getWritableBuffer( UINT32 size, UINT32 offset ) ;

   private:
      OSS_INLINE BOOLEAN _isValidAccessing( UINT32 offset, UINT32 size ) const
      {
         return ( offset + size ) <= _size ;
      }

   private:
      UINT32 _size = 0 ;
      const CHAR *_rptr = nullptr ;
      CHAR *_wptr = nullptr ;
   } ;

   typedef class _utilStrictBuffer utilStrictBuffer ;

}



#endif // UTIL_STRICT_BUFFER_HPP_