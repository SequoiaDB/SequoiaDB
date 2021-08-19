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

   Source File Name = utilArray.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_ARRAY_HPP_
#define UTIL_ARRAY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"

#define UTIL_ARRAY_DEFAULT_SIZE        4 

namespace engine
{
   template <typename T, UINT32 stackSize = UTIL_ARRAY_DEFAULT_SIZE >
   class _utilArray : public SDBObject
   {
   public:
      _utilArray( UINT32 size = 0 )
      :_dynamicBuf( _staticBuf ),
       _bufSize( stackSize ),
       _eleSize( 0 )
      {
         resize( size ) ;
      }

      ~_utilArray()
      {
         clear( TRUE ) ;
      }

   public:
      class iterator
      {
      public:
         iterator( _utilArray<T,stackSize> &t )
         :_t( &t ),
          _now( 0 )
         {
         }

         ~iterator(){}

      public:
         BOOLEAN more() const
         {
            return _now < _t->_eleSize ;
         }

         BOOLEAN next( T &t )
         {
            if ( more() )
            {
               t = _t->_dynamicBuf[_now++] ;
               return TRUE ;
            }
            return FALSE ;
         }

      private:
         _utilArray<T,stackSize> *_t ;
         UINT32 _now ;
      } ;

   public:
      OSS_INLINE const T &operator[]( UINT32 i ) const
      {
         return _dynamicBuf[ i ] ;
      }

      OSS_INLINE T &operator[]( UINT32 i )
      {
         return _dynamicBuf[ i ] ;
      }

      OSS_INLINE UINT32 size() const
      {
         return _eleSize ;
      }

      OSS_INLINE UINT32 capacity() const
      {
         return _bufSize ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return 0 == _eleSize ;
      }

      OSS_INLINE void clear( BOOLEAN resetMem = TRUE )
      {
         if ( resetMem && _dynamicBuf != _staticBuf )
         {
            SDB_OSS_FREE( _dynamicBuf ) ;
            _dynamicBuf = _staticBuf ;
            _bufSize = stackSize ;
         }
         _eleSize = 0 ;
      }

      OSS_INLINE INT32 append( const T &t )
      {
         INT32 rc = SDB_OK ;

         if ( _eleSize >= _bufSize )
         {
            /// resize by 2 mutiple
            rc = resize( _bufSize << 1 ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         _dynamicBuf[ _eleSize++ ] = t ;

      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 resize( UINT32 size )
      {
         INT32 rc = SDB_OK ;
         if ( size <= _bufSize )
         {
            goto done ;
         }
         else if ( _dynamicBuf == _staticBuf )
         {
            T* pTmp = (T*)SDB_OSS_MALLOC( sizeof( T ) * size ) ;
            if ( !pTmp )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            /// copy data
            ossMemcpy( pTmp, _dynamicBuf, sizeof( T ) * _eleSize ) ;
            /// set pointer
            _dynamicBuf = pTmp ;
            _bufSize = size ;
         }
         else
         {
            T *tmp = _dynamicBuf ;
            _dynamicBuf = (T*)SDB_OSS_REALLOC( _dynamicBuf,
                                               sizeof( T ) * size ) ;
            if ( NULL == _dynamicBuf )
            {
               _dynamicBuf = tmp ;
               rc = SDB_OOM ;
               goto error ;
            }
            _bufSize = size ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE _utilArray<T>& operator= ( const _utilArray<T> &rhs )
      {
         if ( SDB_OK == resize( rhs.size() ) )
         {
            ossMemcpy( _dynamicBuf, rhs._dynamicBuf, rhs.size() * sizeof(T) ) ;
            _eleSize = rhs.size() ;
         }
         return *this ;
      }

      OSS_INLINE INT32 copyTo( _utilArray<T> &rhs )
      {
         INT32 rc = SDB_OK ;

         rc = rhs.resize( size() ) ;
         if ( rc )
         {
            goto error ;
         }
         ossMemcpy( rhs._dynamicBuf, _dynamicBuf, size() * sizeof(T) ) ;
         rhs._eleSize = size() ;

      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      T _staticBuf[ stackSize ] ;
      T *_dynamicBuf ;
      UINT32 _bufSize ;
      UINT32 _eleSize ;
   } ;
}

#endif // UTIL_ARRAY_HPP_

