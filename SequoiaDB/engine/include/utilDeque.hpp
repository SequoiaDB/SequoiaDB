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

   Source File Name = utilDeque.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_DEQUE_HPP_
#define UTIL_DEQUE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include <deque>

using namespace std ;

#define UTIL_DEQUE_DEFAULT_STACK_SIZE        4

namespace engine
{
   template <typename T, UINT32 stackSize = UTIL_DEQUE_DEFAULT_STACK_SIZE >
   class _utilDeque : public SDBObject
   {
   public:
      _utilDeque( UINT32 size = 0 )
      :_pDeque( NULL ),
       _eleSize( 0 )
      {
         resize( size ) ;
      }

      ~_utilDeque()
      {
         clear( TRUE ) ;
      }

   public:
      class iterator
      {
         friend class _utilDeque< T, stackSize > ;
         public:
            iterator()
            {
               _pData      = NULL ;
               _pSrc       = NULL ;
               _pEleSize   = NULL ;
            }
            iterator( const iterator &rhs )
            {
               _pData      = rhs._pData ;
               _pSrc       = rhs._pSrc ;
               _pEleSize   = rhs._pEleSize ;
               _it         = rhs._it ;
            }
            BOOLEAN operator== ( const iterator &rhs ) const
            {
               if ( _pData && rhs._pData )
               {
                  /// left, right is end
                  BOOLEAN leftEnd = _pData >= _pSrc + *_pEleSize ?
                                    TRUE : FALSE ;
                  BOOLEAN rightEnd = rhs._pData > rhs._pSrc + *(rhs._pEleSize) ?
                                     TRUE : FALSE ;
                  /// both end, equal
                  if ( leftEnd && rightEnd &&
                       _pSrc == rhs._pSrc &&
                       _pEleSize == rhs._pEleSize )
                  {
                     return TRUE ;
                  }
                  return _pData == rhs._pData ? TRUE : FALSE ;
               }
               else if ( !_pData && !rhs._pData )
               {
                  return _it == rhs._it ? TRUE : FALSE ;
               }
               return FALSE ;
            }
            BOOLEAN operator!= ( const iterator &rhs ) const
            {
               return this->operator==( rhs ) ? FALSE : TRUE ;
            }
            iterator& operator= ( const iterator &rhs )
            {
               _pData         = rhs._pData ;
               _pSrc          = rhs._pSrc ;
               _pEleSize      = rhs._pEleSize ;
               _it            = rhs._it ;
               return *this ;
            }
            const T& operator* () const
            {
               if ( _pData )
               {
                  return *_pData ;
               }
               return *_it ;
            }
            iterator& operator++ ()
            {
               if ( _pData )
               {
                  ++_pData ;
               }
               else
               {
                  ++_it ;
               }
               return *this ;
            }
            iterator& operator++ ( int )
            {
               if ( _pData )
               {
                  _pData++ ;
               }
               else
               {
                  _it++ ;
               }
               return *this ;
            }
            iterator& operator-- ()
            {
               if ( _pData )
               {
                  --_pData ;
               }
               else
               {
                  --_it ;
               }
               return *this ;
            }
            iterator& operator-- ( int )
            {
               if ( _pData )
               {
                  _pData-- ;
               }
               else
               {
                  _it-- ;
               }
               return *this ;
            }
            iterator& operator+ ( UINT32 step )
            {
               if ( _pData )
               {
                  _pData += step ;
               }
               else
               {
                  _it += step ;
               }
               return *this ;
            }
            iterator& operator- ( UINT32 step )
            {
               if ( _pData )
               {
                  _pData -= step ;
               }
               else
               {
                  _it -= step ;
               }
               return *this ;
            }

         protected:
            iterator( T* pData, T *pSrc, UINT32 *pEleSize )
            {
               _pData         = pData ;
               _pSrc          = pSrc ;
               _pEleSize      = pEleSize ;
            }
            iterator( typename const deque<T>::iterator &it )
            {
               _pData         = NULL ;
               _pSrc          = NULL ;
               _pEleSize      = NULL ;
               _it            = it ;
            }

         private:
            T*                            _pData ;
            T*                            _pSrc ;
            UINT32*                       _pEleSize ;
            typename deque<T>::iterator   _it ;
      } ;

   public:
      OSS_INLINE iterator begin()
      {
         if ( _pDeque )
         {
            return iterator( _pDeque->begin() ) ;
         }
         return iterator( _staticBuf, _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE iterator end()
      {
         if ( _pDeque )
         {
            return iterator( _pDeque->end() ) ;
         }
         return iterator( &_staticBuf[ stackSize ], _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE iterator erase( iterator position )
      {
         if ( position._pData )
         {
            --_eleSize ;
            UINT32 pos = ( position._pData - _staticBuf ) / sizeof( T ) ;
            for ( UINT32 i = pos ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = _staticBuf[ i + 1 ] ;
            }
            return iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
         }
         else
         {
            return iterator( _pDeque->erase( position._it ) ) ;
         }
      }

      OSS_INLINE iterator erase( iterator first, iterator last )
      {
         if ( _pDeque )
         {
            return iterator( _pDeque->erase( first._it, last._it ) ) ;
         }
         else if ( _eleSize > 0 )
         {
            UINT32 b = ( first._pData - _staticBuf ) / sizeof( T ) ;
            UINT32 e = ( last._pData - _staticBuf ) / sizeof( T ) ;

            UINT32 i = b ;
            UINT32 j = b ;
            UINT32 tmpSize = _eleSize ;
            for ( ; i < tmpSize ; ++i )
            {
               if ( i < e )
               {
                  --_eleSize ;
                  continue ;
               }
               _staticBuf[ j++ ] = _staticBuf[ i ] ;
            }
            return iterator( &_staticBuf[ b ], _staticBuf, &_eleSize ) ;
         }
         return end() ;
      }

      OSS_INLINE iterator insert( iterator position, const T& val )
      {
         INT32 rc = SDB_OK ;
         iterator it ;

         rc = _ensureSpace( _eleSize + 1 ) ;
         if ( rc )
         {
            it = end() ;
            goto error ;
         }

         if ( _pDeque )
         {
            it = iterator( _pDeque->insert( val ) ) ;
         }
         else
         {
            UINT32 pos = ( position._pData - _staticBuf ) / sizeof( T ) ;
            if ( pos > _eleSize )
            {
               pos = _eleSize ;
            }
            for ( UINT32 i = _eleSize ; i > pos ; --i )
            {
               _staticBuf[ i ] = _staticBuf[ i - 1 ] ;
            }
            _staticBuf[ pos ] = val ;
            ++_eleSize ;
            it = iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
         }

      done:
         return it ;
      error:
         goto done ;
      }

      OSS_INLINE UINT32 size() const
      {
         if ( _pDeque )
         {
            return _pDeque->size() ;
         }
         return _eleSize ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         if ( _pDeque )
         {
            return _pDeque->empty() ? TRUE : FALSE ;
         }
         return 0 == _eleSize ? TRUE : FALSE ;
      }

      INT32 resize( UINT32 newSize )
      {
         INT32 rc = SDB_OK ;

         if ( !_pDeque )
         {
            if ( newSize > stackSize )
            {
               _pDeque = new (std::nothrow) deque<T>( newSize ) ;
               if ( !_pDeque )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               /// copy stack data to deque
               for ( UINT32 i = 0 ; i < _eleSize ; ++i )
               {
                  (*_pDeque)[i] = _staticBuf[ i ] ;
               }
               _eleSize = 0 ;
            }
            else if ( _eleSize >= newSize )
            {
               _eleSize = newSize ;
            }
            else
            {
               while( _eleSize < newSize )
               {
                  _staticBuf[ _eleSize++ ] = T() ;
               }
            }
         }
         else
         {
            try
            {
               _pDeque->resize( newSize ) ;
            }
            catch( std::exception & )
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE const T& at( UINT32 i ) const
      {
         if ( _pDeque )
         {
            return (*_pDeque)[ i ] ;
         }
         return _staticBuf[ i ] ;
      }

      OSS_INLINE T& at( UINT32 i )
      {
         if ( _pDeque )
         {
            return (*_pDeque)[ i ] ;
         }
         return _staticBuf[ i ] ;
      }

      OSS_INLINE const T& operator[]( UINT32 i ) const
      {
         return at( i ) ;
      }

      OSS_INLINE T& operator[]( UINT32 i )
      {
         return at( i ) ;
      }

      OSS_INLINE const T& front() const
      {
         if ( _pDeque )
         {
            return _pDeque->front() ;
         }
         return _staticBuf[ 0 ] ;
      }

      OSS_INLINE T& front()
      {
         if ( _pDeque )
         {
            return _pDeque->front() ;
         }
         return _staticBuf[ 0 ] ;
      }

      OSS_INLINE const T& back() const
      {
         if ( _pDeque )
         {
            return _pDeque->back() ;
         }
         return _staticBuf[ _eleSize - 1 ] ;
      }

      OSS_INLINE T& back()
      {
         if ( _pDeque )
         {
            return _pDeque->back() ;
         }
         return _staticBuf[ _eleSize - 1 ] ;
      }

      OSS_INLINE INT32 push_back( const T& value )
      {
         INT32 rc = SDB_OK ;

         rc = _ensureSpace( _eleSize + 1 ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( !_pDeque )
         {
            _staticBuf[ _eleSize++ ] = value ;
         }
         else
         {
            try
            {
               _pDeque->push_back( value ) ;
            }
            catch( std::exception & )
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE INT32 push_front( const T& value )
      {
         INT32 rc = SDB_OK ;

         rc = _ensureSpace( _eleSize + 1 ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( !_pDeque )
         {
            for ( UINT32 i = _eleSize ; i > 0 ; --i )
            {
               _staticBuf[ i ] = _staticBuf[ i - 1 ] ;
            }
            _staticBuf[ 0 ] = value ;
            ++_eleSize ;
         }
         else
         {
            try
            {
               _pDeque->push_front( value ) ;
            }
            catch( std::exception & )
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE void pop_back()
      {
         if ( _pDeque )
         {
            _pDeque->pop_back() ;
            shift2Stack() ;
         }
         else if ( _eleSize > 0 )
         {
            --_eleSize ;
         }
      }

      OSS_INLINE void pop_front()
      {
         if ( _pDeque )
         {
            _pDeque->pop_front() ;
            shift2Stack() ;
         }
         else if ( 1 == _eleSize )
         {
            _eleSize = 0 ;
         }
         else if ( _eleSize > 1 )
         {
            --_eleSize ;
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = _staticBuf[ i + 1 ] ;
            }
         }
      }

      OSS_INLINE void clear( BOOLEAN resetMem = TRUE )
      {
         if ( _pDeque )
         {
            if ( resetMem )
            {
               delete _pDeque ;
               _pDeque = NULL ;
            }
            else
            {
               _pDeque->clear() ;
            }
         }
         _eleSize = 0 ;
      }

      OSS_INLINE _utilDeque<T>& operator= ( const _utilDeque<T> &rhs )
      {
         UINT32 rSize = rhs.size() ;

         /// clear self
         clear( TRUE ) ;
         /// alloc space
         _ensureSpace( rSize ) ;
         /// copy all elements
         for ( UINT32 i = 0 ; i < rSize ; ++i )
         {
            push_back( rhs[ i ] ) ;
         }
         return *this ;
      }

      OSS_INLINE void shift2Stack( UINT32 divisor = 8 )
      {
         if ( _pDeque )
         {
            UINT32 threshold = 0 ;
            if ( divisor > 0 )
            {
               threshold = stackSize / divisor ;
            }
            if ( _pDeque->size() <= threshold )
            {
               /// copy data to stack
               _eleSize = _pDeque->size() ;
               for ( UINT32 i = 0 ; i < _eleSize ; ++i )
               {
                  _staticBuf[ i ] = (*_pDeque)[ i ] ;
               }
               /// release the deque
               delete _pDeque ;
               _pDeque = NULL ;
            }
         }
      }

   protected:
      OSS_INLINE UINT32 _capacity() const
      {
         if ( _pDeque )
         {
            return _pDeque->size() ;
         }
         return stackSize ;
      }

      OSS_INLINE INT32 _ensureSpace( UINT32 size )
      {
         INT32 rc = SDB_OK ;

         if ( !_pDeque && size > stackSize )
         {
            _pDeque = new (std::nothrow) deque<T>( _eleSize ) ;
            if ( !_pDeque )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            /// copy stack data to deque
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               (*_pDeque)[i] = _staticBuf[i] ;
            }
            _eleSize = 0 ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      T              _staticBuf[ stackSize ] ;
      deque<T>*      _pDeque ;
      UINT32         _eleSize ;
   } ;
}

#endif // UTIL_DEQUE_HPP_

