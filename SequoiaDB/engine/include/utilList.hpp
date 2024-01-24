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
#include "ossMemPool.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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

            const_iterator ( const typename ossPoolList<T>::iterator &it )
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
            const T *                              _pData ;
            const T *                              _pSrc ;
            const UINT32 *                         _pEleSize ;
            typename ossPoolList<T>::iterator      _it ;
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

            iterator ( const typename ossPoolList<T>::iterator &it )
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
            UINT32 pos = position._pData - _staticBuf ;
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
            UINT32 b = first._pData - _staticBuf ;
            UINT32 e = last._pData - _staticBuf ;

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
            UINT32 pos = position._pData - _staticBuf ;
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
               _pList = new (std::nothrow) ossPoolList<T>( newSize ) ;
               if ( !_pList )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               /// copy stack data to deque
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
            /// copy back
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
            /// copy back
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
            /// copy back
            for ( i = 0 ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = tmp[ i ] ;
            }
         }
      }

      OSS_INLINE _utilList<T>& operator= ( const _utilList<T> &rhs )
      {
         UINT32 rSize = rhs.size() ;

         /// clear self
         clear( TRUE ) ;
         /// alloc space
         _ensureSpace( rSize ) ;
         /// copy all elements
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
               /// copy data to stack
               _eleSize = 0 ;
               typename ossPoolList<T>::iterator it = _pList->begin() ;
               while( it != _pList->end() )
               {
                  _staticBuf[ _eleSize++ ] = *it ;
                  ++it ;
               }
               /// release the deque
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
            _pList = new (std::nothrow) ossPoolList<T>() ;
            if ( !_pList )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            try
            {
               /// copy stack data to deque
               for ( UINT32 i = 0 ; i < _eleSize ; ++i )
               {
                  _pList->push_back( _staticBuf[ i ] ) ;
               }
               _eleSize = 0 ;
            }
            catch( std::exception & )
            {
               delete _pList ;
               _pList = NULL ;
               rc = SDB_OOM ;
               goto error ;
            }
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
                  /// find the position
                  pBuff[ pos ] = val ;
                  ++size ;
                  return ;
               }
            }
            /// insert to the begin
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
                  /// find the position
                  pBuff[ pos ] = val ;
                  ++size ;
                  return ;
               }
            }
            /// insert to the begin
            pBuff[ 0 ] = val ;
            ++size ;
         }
      }

   private:
      T                 _staticBuf[ stackSize ] ;
      ossPoolList<T>*   _pList ;
      UINT32            _eleSize ;
   } ;

#define UTIL_LIST_PARTITION_NUM 16

   // A partitioned list which increases concurrency for parallel writes.
   // Features:
   //   - Has 16 partitions/sublists (default)
   //   - Sychronization within each partition is self-contained.
   //   - Modifier functions on each sublist are X latch protected
   //   - Iterators are X latch protected by the iterator's current partition
   //   - Ideal for a high ratio of writes vs reads
   // Limitations:
   //   - Due to the nature of the partitioning, iteration on the list will
   //     be in random order (as opposed to insertion order)
   //   - Each partition can only allow a single thread access
   //   - External sychronization is needed for a snapshot view of the list.
   // Caution:
   //   - Sublist latches are obtained inside the iterator, so avoid having
   //     stale iterators in the code.
   //   - invalidate() can be called to release the sublist latch
   template< typename T, typename LIST_TYPE, UINT32 (*PARTITION_FUNC)(),
             UINT16 partitions = UTIL_LIST_PARTITION_NUM >
   class _utilPartitionList : public SDBObject
   {
      class latcher : public SDBObject
      {
      public :
         latcher( _utilPartitionList * list, UINT16 listID )
            : _list( list ), _listID( listID )
         {
            _list->_getLatch( _listID ) ;
         }
         ~latcher()
         {
            _list->_releaseLatch( _listID ) ;
         }

         _utilPartitionList *_list ;
         UINT16 _listID ;
      } ;
   public:

      class iterator
      {
      friend class _utilPartitionList ;
         _utilPartitionList* _partList ; // Pointer pointing to partition list
         UINT16 _curListID ;            // id of the sub list being traversed
         LIST_TYPE* _curList ;          // Pointer to the sub list being traversed

         boost::shared_ptr<latcher> _latchPtr ;
         typename LIST_TYPE::iterator _itr ; // iterator for the sub list

         // Construct an iterator from a sublist iterator.
         iterator( _utilPartitionList * list, UINT16 listID,
                   typename LIST_TYPE::iterator it )
            : _partList( list ), _curListID( listID ), _curList( NULL ),
              _itr( it )
         {
            if ( _curListID < partitions )
            {
               _curList = _partList->_getList( _curListID ) ;
            }
         }

         iterator( _utilPartitionList * list )
            : _partList( list ), _curListID( 0 ), _curList( NULL )
         {
            // To find the begin position of the iterator, we need to skip all
            // empty sub lists until we find the first non empty sub list or
            // hit the end of all sub lists.
            while ( _curListID < partitions )
            {
               _curList = _partList->_getList( _curListID ) ;
               _latchPtr.reset( SDB_OSS_NEW latcher( _partList, _curListID ) ) ;
               if ( _curList->size() > 0 )
               {
                  _itr = _curList->begin() ;
                  break ;
               }
               ++_curListID ;
            }

            // list is empty
            if ( _curListID == partitions )
            {
               _latchPtr.reset() ;
               _itr = _partList->_getList( _curListID )->end() ;
            }
         }

      public:
         iterator()
            : _partList( NULL ), _curListID( 0 ), _curList( NULL )
         {}
         ~iterator()
         {
            _latchPtr.reset() ;
         }

         // assignment operator.
         iterator& operator= ( const iterator& rhs )
         {
            // check self assignment
            if ( this != &rhs )
            {
               _curList = rhs._curList ;
               _curListID = rhs._curListID ;
               _partList = rhs._partList ;
               _itr = rhs._itr ;
               // This assignment will either increase use count of the
               // shared ptr or release shared ptr
               _latchPtr = rhs._latchPtr ;
            }
            return *this ;
         }

         // Pre-increment
         // Increment the itr of the current sub list, and move the itr to
         // next sublist once it reaches the end of the current list.
         // The latch of each sub list is obtained before get the first element
         // and is released after reach the end, the caller is not responsible
         // for obtaining any latch
         iterator& operator++ ()
         {
            if ( ++_itr == _curList->end() )
            {
               while ( ++_curListID < partitions )
               {
                  _curList = _partList->_getList( _curListID ) ;
                  _latchPtr.reset( SDB_OSS_NEW latcher( _partList, _curListID ) ) ;
                  if ( _curList->size() > 0 )
                  {
                     _itr = _curList->begin() ;
                     break ;
                  }
                  _itr = _curList->end() ;
               }

               // reach the end, set the iterator to the end
               if ( _curListID == partitions )
               {
                  ( *this ) = _partList->end() ;
               }
            }
            return *this;
         }

         iterator operator++ ( int ) // Post-increment
         {
            iterator tmp( *this ) ;
            ++( *this ) ;
            return tmp ;
         }

         BOOLEAN operator == ( const iterator& rhs ) const
         {
            return _itr == rhs._itr;
         }

         BOOLEAN operator != ( const iterator& rhs ) const
         {
            return _itr != rhs._itr ;
         }

         typename LIST_TYPE::reference operator* () const
         {
            return *_itr ;
         }

         typename LIST_TYPE::pointer operator-> () const
         {
            return &( *_itr ) ;
         }

         void invalidate()
         {
            _latchPtr.reset() ;
         }
      } ;

   public:

      _utilPartitionList()
        : _listLen ( 0 ),
          _endIt( this, partitions, _list[partitions].end() )
      {
      }

      ~_utilPartitionList() {}

      // return the begin iterator of this list
      iterator begin()
      {
         return iterator( this ) ;
      }
      // return the end iterator of this list
      iterator end()
      {
         return _endIt ;
      }

      // get size of the list
      UINT32 size()
      {
         return _listLen.fetch() ;
      }

      // add obj into the list. It first find which list is should be
      // put in.
      void add( T *obj )
      {
         UINT16 i = PARTITION_FUNC() % partitions ;
         _getLatch( i ) ;
         _list[i].push_front( *obj ) ;
         _releaseLatch( i ) ;
         _listLen.inc() ;
      }

      iterator erase( iterator it )
      {
         it._itr = it._curList->erase(it._itr) ;
         _listLen.dec() ;
         // As erase moves the itr to next, the next node might be end.
         // If it is the end of current sublist, then move to the begin
         // of next non-empty list
         if ( it._itr == it._curList->end() )
         {
            while ( ++it._curListID < partitions )
            {
               it._curList = _getList( it._curListID ) ;
               it._latchPtr.reset( new latcher(this, it._curListID ) ) ;
               if ( it._curList->size() > 0 )
               {
                  it._itr = it._curList->begin() ;
                  break ;
               }
               it._itr = it._curList->end() ;
            }

            // reach the end, set the iterator to the end
            if ( it._curListID == partitions )
            {
               return _endIt ;
            }
         }
         return it ;
      }

   protected:

      // get pointer of sublist based on the id
      LIST_TYPE* _getList( UINT16 id )
      {
         SDB_ASSERT( id < partitions + 1, "Invalid index" ) ;
         return &( _list[id] ) ;
      }

      // get latch of sub list based on the id
      void _getLatch ( UINT16 id )
      {
         SDB_ASSERT( id < partitions, "Invalid index" ) ;
         _listLatch[id].get() ;
      }

      // release latch of sub list based on the id
      void _releaseLatch( UINT16 id )
      {
         SDB_ASSERT( id < partitions, "Invalid index" ) ;
         _listLatch[id].release() ;
      }

      /* LATCHING PROTOCOL:
       * ADD: The list will obtain/release latch of the sub list,
       * which it will add to in x.
       *
       * SCAN: The list will obtain/release latch of the sub list,
       * which it will scan in x.
       *
       * DELETE: The list will obtain/release latch of the sub list in x.
       */
      ossAtomic32 _listLen ;     // total # of the elements in the list
      ossSpinXLatch _listLatch[partitions] ; // latch of each sub list

      // array of sub lists. There is an extra sublist to represent the
      // end iterator
      LIST_TYPE _list[partitions + 1] ;
      iterator _endIt ; // iterator representing the end
   } ;
}

#endif // UTIL_LIST_HPP_

