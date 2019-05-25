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

   Source File Name = utilList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_LIST_HPP_
#define UTIL_LIST_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include <list>

using namespace std ;

#define UTIL_LIST_DEFAULT_STACK_SIZE         4

namespace engine
{
   template <typename T, UINT32 stackSize = UTIL_LIST_DEFAULT_STACK_SIZE >
   class _utilList : public SDBObject
   {
   public:
      _utilList( UINT32 size = 0 )
      :_pList( NULL ),
       _eleSize( 0 )
      {
         resize( size ) ;
      }

      ~_utilList()
      {
         clear( TRUE ) ;
      }

   public:
      class const_iterator
      {
         friend class _utilList< T, stackSize > ;
         public:
            const_iterator ()
            {
               _pData      = NULL ;
               _pSrc       = NULL ;
               _pEleSize   = NULL ;
            }

            const_iterator ( const const_iterator &rhs )
            {
               _pData      = rhs._pData ;
               _pSrc       = rhs._pSrc ;
               _pEleSize   = rhs._pEleSize ;
               _it         = rhs._it ;
            }

            virtual ~const_iterator ()
            {
            }

            BOOLEAN operator == ( const const_iterator &rhs ) const
            {
               return this->_equals( rhs ) ;
            }

            BOOLEAN operator != ( const const_iterator &rhs ) const
            {
               return this->_equals( rhs ) ? FALSE : TRUE ;
            }

            const_iterator & operator = ( const const_iterator &rhs )
            {
               this->_assign( rhs ) ;
               return ( *this ) ;
            }

            const T & operator * () const
            {
               return this->_reference() ;
            }

            const T * operator -> () const
            {
               return this->_pointer() ;
            }

            const_iterator & operator ++ ()
            {
               this->_increase() ;
               return ( *this ) ;
            }

            const_iterator operator ++ ( int )
            {
               const_iterator temp( *this ) ;
               this->_increase() ;
               return temp ;
            }

            const_iterator & operator -- ()
            {
               this->_decrease() ;
               return *this ;
            }

            const_iterator operator -- ( int )
            {
               const_iterator temp( *this ) ;
               this->_decrease() ;
               return temp ;
            }

         protected:
            const_iterator ( const T * pData, const T * pSrc, const UINT32 * pEleSize )
            {
               _pData         = pData ;
               _pSrc          = pSrc ;
               _pEleSize      = pEleSize ;
            }

            const_iterator ( const typename list<T>::iterator &it )
            {
               _pData         = NULL ;
               _pSrc          = NULL ;
               _pEleSize      = NULL ;
               _it            = it ;
            }

            void _assign ( const const_iterator &rhs )
            {
               _pData         = rhs._pData ;
               _pSrc          = rhs._pSrc ;
               _pEleSize      = rhs._pEleSize ;
               _it            = rhs._it ;
            }

            BOOLEAN _equals ( const const_iterator &rhs ) const
            {
               if ( _pData && rhs._pData )
               {
                  BOOLEAN leftEnd = _pData >= _pSrc + *_pEleSize ?
                                    TRUE : FALSE ;
                  BOOLEAN rightEnd = rhs._pData > rhs._pSrc + *(rhs._pEleSize) ?
                                     TRUE : FALSE ;
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

            const T & _reference () const
            {
               if ( _pData )
               {
                  return ( *_pData ) ;
               }
               return ( *_it ) ;
            }

            const T * _pointer () const
            {
               if ( _pData )
               {
                  return _pData ;
               }
               return &( *_it ) ;
            }

            void _increase ()
            {
               if ( _pData )
               {
                  ++_pData ;
               }
               else
               {
                  ++_it ;
               }
            }

            void _decrease ()
            {
               if ( _pData )
               {
                  --_pData ;
               }
               else
               {
                  --_it ;
               }
            }

         protected :
            const T *                        _pData ;
            const T *                        _pSrc ;
            const UINT32 *                   _pEleSize ;
            typename list<T>::iterator       _it ;
      } ;

      class iterator : public const_iterator
      {
         friend class _utilList< T, stackSize > ;
         public:
            iterator ()
            : const_iterator()
            {
            }

            iterator ( const iterator &rhs )
            : const_iterator( rhs )
            {
            }

            BOOLEAN operator == ( const iterator &rhs ) const
            {
               return this->_equals( rhs ) ;
            }

            BOOLEAN operator != ( const iterator &rhs ) const
            {
               return this->_equals( rhs ) ? FALSE : TRUE ;
            }

            iterator & operator = ( const iterator &rhs )
            {
               this->_assign( rhs ) ;
               return ( *this ) ;
            }

            T & operator * () const
            {
               return (T &)( this->_reference() ) ;
            }

            T * operator -> () const
            {
               return (T *)( this->_pointer() ) ;
            }

            iterator & operator ++ ()
            {
               this->_increase() ;
               return ( *this ) ;
            }

            iterator operator ++ ( int )
            {
               iterator temp( *this ) ;
               this->_increase() ;
               return temp ;
            }

            iterator & operator -- ()
            {
               this->_decrease() ;
               return ( *this ) ;
            }

            iterator operator -- ( int )
            {
               iterator temp( *this ) ;
               this->_decrease() ;
               return temp ;
            }

         protected :
            iterator ( T * pData, T * pSrc, UINT32 * pEleSize )
            : const_iterator( pData, pSrc, pEleSize )
            {
            }

            iterator ( const typename list<T>::iterator &it )
            : const_iterator( it )
            {
            }
      } ;

   public:
      OSS_INLINE iterator begin()
      {
         if ( _pList )
         {
            return iterator( _pList->begin() ) ;
         }
         return iterator( _staticBuf, _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE iterator end()
      {
         if ( _pList )
         {
            return iterator( _pList->end() ) ;
         }
         return iterator( &_staticBuf[ stackSize ], _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE const_iterator begin() const
      {
         if ( _pList )
         {
            return const_iterator( _pList->begin() ) ;
         }
         return const_iterator( _staticBuf, _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE const_iterator end() const
      {
         if ( _pList )
         {
            return const_iterator( _pList->end() ) ;
         }
         return const_iterator( &_staticBuf[ stackSize ], _staticBuf, &_eleSize ) ;
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
            return iterator( _pList->erase( position._it ) ) ;
         }
      }

      OSS_INLINE iterator erase( iterator first, iterator last )
      {
         if ( _pList )
         {
            return iterator( _pList->erase( first._it, last._it ) ) ;
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

         if ( _pList )
         {
            it = iterator( _pList->insert( val ) ) ;
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
         if ( _pList )
         {
            return _pList->size() ;
         }
         return _eleSize ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         if ( _pList )
         {
            return _pList->empty() ? TRUE : FALSE ;
         }
         return 0 == _eleSize ? TRUE : FALSE ;
      }

      INT32 resize( UINT32 newSize )
      {
         INT32 rc = SDB_OK ;

         if ( !_pList )
         {
            if ( newSize > stackSize )
            {
               _pList = new (std::nothrow) list<T>( newSize ) ;
               if ( !_pList )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               for ( UINT32 i = 0 ; i < _eleSize ; ++i )
               {
                  _pList->push_back( _staticBuf[ i ] ) ;
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
               _pList->resize( newSize ) ;
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

      OSS_INLINE const T& front() const
      {
         if ( _pList )
         {
            return _pList->front() ;
         }
         return _staticBuf[ 0 ] ;
      }

      OSS_INLINE T& front()
      {
         if ( _pList )
         {
            return _pList->front() ;
         }
         return _staticBuf[ 0 ] ;
      }

      OSS_INLINE const T& back() const
      {
         if ( _pList )
         {
            return _pList->back() ;
         }
         return _staticBuf[ _eleSize - 1 ] ;
      }

      OSS_INLINE T& back()
      {
         if ( _pList )
         {
            return _pList->back() ;
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

         if ( !_pList )
         {
            _staticBuf[ _eleSize++ ] = value ;
         }
         else
         {
            try
            {
               _pList->push_back( value ) ;
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

         if ( !_pList )
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
               _pList->push_front( value ) ;
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
         if ( _pList )
         {
            _pList->pop_back() ;
            shift2Stack() ;
         }
         else if ( _eleSize > 0 )
         {
            --_eleSize ;
         }
      }

      OSS_INLINE void pop_front()
      {
         if ( _pList )
         {
            _pList->pop_front() ;
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
         if ( _pList )
         {
            if ( resetMem )
            {
               delete _pList ;
               _pList = NULL ;
            }
            else
            {
               _pList->clear() ;
            }
         }
         _eleSize = 0 ;
      }

      OSS_INLINE void remove( const T& val )
      {
         if ( _pList )
         {
            _pList->remove( val ) ;
            shift2Stack() ;
         }
         else
         {
            UINT32 i = 0 ;
            UINT32 j = 0 ;
            for ( ; i < _eleSize ; ++i )
            {
               if ( _staticBuf[ i ] == val )
               {
                  continue ; /// removed
               }
               if ( j != i )
               {
                  _staticBuf[ j ] = _staticBuf[ i ] ;
               }
               ++j ;
            }
            _eleSize = j ;
         }
      }

      template< class Predicate >
      OSS_INLINE void remove_if( Predicate pred )
      {
         if ( _pList )
         {
            _pList->remove_if ( pred ) ;
            shift2Stack() ;
         }
         else
         {
            UINT32 i = 0 ;
            UINT32 j = 0 ;
            for ( ; i < _eleSize ; ++i )
            {
               if ( pred( _staticBuf[ i ] ) )
               {
                  continue ; /// removed
               }
               if ( j != i )
               {
                  _staticBuf[ j ] = _staticBuf[ i ] ;
               }
               ++j ;
            }
            _eleSize = j ;
         }
      }

      OSS_INLINE void sort()
      {
         if ( _pList )
         {
            _pList->sort() ;
         }
         else
         {
            T tmp[ stackSize ] ;
            UINT32 tmpSize = 0 ;
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _insertBySort( tmp, tmpSize, _staticBuf[ i ] ) ;
            }
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = tmp[ i ] ;
            }
         }
      }

      template < class Compare >
      OSS_INLINE void sort( Compare comp )
      {
         if ( _pList )
         {
            _pList->sort( comp ) ;
         }
         else
         {
            T tmp[ stackSize ] ;
            UINT32 tmpSize = 0 ;
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _insertBySort( tmp, tmpSize, comp, _staticBuf[ i ] ) ;
            }
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = tmp[ i ] ;
            }
         }
      }

      OSS_INLINE void reverse()
      {
         if ( _pList )
         {
            _pList->reverse() ;
         }
         else if ( _eleSize > 1 )
         {
            T tmp[ stackSize ] ;
            UINT32 i = 0 ;
            UINT32 j = _eleSize ;
            for ( ; i < _eleSize ; ++i, --j )
            {
               tmp[ i ] = _staticBuf[ j - 1 ] ;
            }
            for ( i = 0 ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = tmp[ i ] ;
            }
         }
      }

      OSS_INLINE _utilList<T>& operator= ( const _utilList<T> &rhs )
      {
         UINT32 rSize = rhs.size() ;

         clear( TRUE ) ;
         _ensureSpace( rSize ) ;
         const_iterator it = rhs.begin() ;
         while ( it != rhs.end() )
         {
            push_back( *it ) ;
            ++it ;
         }
         return *this ;
      }

      OSS_INLINE void shift2Stack( UINT32 divisor = 8 )
      {
         if ( _pList )
         {
            UINT32 threshold = 0 ;
            if ( divisor > 0 )
            {
               threshold = stackSize / divisor ;
            }
            if ( _pList->size() <= threshold )
            {
               _eleSize = 0 ;
               typename list<T>::iterator it = _pList->begin() ;
               while( it != _pList->end() )
               {
                  _staticBuf[ _eleSize++ ] = *it ;
                  ++it ;
               }
               delete _pList ;
               _pList = NULL ;
            }
         }
      }

   protected:
      OSS_INLINE UINT32 _capacity() const
      {
         if ( _pList )
         {
            return _pList->size() ;
         }
         return stackSize ;
      }

      OSS_INLINE INT32 _ensureSpace( UINT32 size )
      {
         INT32 rc = SDB_OK ;

         if ( !_pList && size > stackSize )
         {
            _pList = new (std::nothrow) list<T>() ;
            if ( !_pList )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               _pList->push_back( _staticBuf[ i ] ) ;
            }
            _eleSize = 0 ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE void _insertBySort( T* pBuff, UINT32 &size, const T& val )
      {
         if ( 0 == size )
         {
            pBuff[ 0 ] = val ;
            ++size ;
         }
         else
         {
            UINT32 pos = size ;
            while( pos > 0 )
            {
               if ( pBuff[ pos - 1 ] > val )
               {
                  pBuff[ pos ] = pBuff[ pos - 1 ] ;
                  --pos ;
               }
               else
               {
                  pBuff[ pos ] = val ;
                  ++size ;
                  return ;
               }
            }
            pBuff[ 0 ] = val ;
            ++size ;
         }
      }

      template < class Compare >
      OSS_INLINE void _insertBySort( T* pBuff, UINT32 &size,
                                     Compare comp, const T& val )
      {
         if ( 0 == size )
         {
            pBuff[ 0 ] = val ;
            ++size ;
         }
         else
         {
            UINT32 pos = size ;
            while( pos > 0 )
            {
               if ( comp( val, pBuff[ pos - 1 ] ) )
               {
                  pBuff[ pos ] = pBuff[ pos - 1 ] ;
                  --pos ;
               }
               else
               {
                  pBuff[ pos ] = val ;
                  ++size ;
                  return ;
               }
            }
            pBuff[ 0 ] = val ;
            ++size ;
         }
      }

   private:
      T              _staticBuf[ stackSize ] ;
      list<T>*       _pList ;
      UINT32         _eleSize ;
   } ;
}

#endif // UTIL_LIST_HPP_

