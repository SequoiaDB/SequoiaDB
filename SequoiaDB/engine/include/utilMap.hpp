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

   Source File Name = utilMap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_MAP_HPP_
#define UTIL_MAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include <map>
#include <stdexcept>

using namespace std ;

#define UTIL_MAP_DEFAULT_STACK_SIZE          4
#define UTIL_MAP_MAX_STACK_SIZE              64

namespace engine
{
   template <typename Key, typename T, UINT32 stackSize = UTIL_MAP_DEFAULT_STACK_SIZE >
   class _utilMap : public SDBObject
   {
   public:
      typedef pair< const Key, T >           value_type ;
      typedef const pair< const Key, T >     const_value_type ;
      typedef pair< Key, T >                 i_value_type ;

      _utilMap()
      :_pMap( NULL ),
       _eleSize( 0 )
      {
         SDB_ASSERT( stackSize <= UTIL_MAP_MAX_STACK_SIZE,
                     "stackSize must <= UTIL_MAP_MAX_STACK_SIZE" ) ;
      }

      _utilMap( const _utilMap& rhs )
      {
         _pMap = NULL ;
         _eleSize = 0 ;

         UINT32 rSize = rhs.size() ;
         _ensureSpace( rSize ) ;
         const_iterator it = rhs.begin() ;
         while( it != rhs.end() )
         {
            insert( value_type( it->first, it->second ) ) ;
            ++it ;
         }
      }

      ~_utilMap()
      {
         clear( TRUE ) ;
      }

   public:
      class iterator
      {
         friend class _utilMap< Key, T, stackSize > ;
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
                  BOOLEAN leftEnd = _pData >= _pSrc + *_pEleSize ?
                                    TRUE : FALSE ;
                  BOOLEAN rightEnd = rhs._pData >= rhs._pSrc + *(rhs._pEleSize) ?
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
            value_type* operator-> ()
            {
               if ( _pData )
               {
                  return _pData ;
               }
               else
               {
                  return _it.operator->() ;
               }
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
            iterator( i_value_type* pData, const i_value_type *pSrc,
                      const UINT32 *pEleSize )
            {
               _pData         = reinterpret_cast< value_type* >( pData ) ;
               _pSrc          = reinterpret_cast< const value_type* >( pSrc ) ;
               _pEleSize      = pEleSize ;
            }
            iterator( typename map< Key, T >::iterator it )
            {
               _pData         = NULL ;
               _pSrc          = NULL ;
               _pEleSize      = NULL ;
               _it            = it ;
            }

         private:
            iterator operator++ ( int )
            {
               iterator tmp( *this ) ;
               ++(*this) ;
               return tmp ;
            }
            iterator operator-- ( int )
            {
               iterator tmp( *this ) ;
               --(*this) ;
               return tmp ;
            }

         private:
            value_type*                         _pData ;
            const value_type*                   _pSrc ;
            const UINT32*                       _pEleSize ;
            typename map< Key, T >::iterator    _it ;
      } ;

      class const_iterator
      {
         friend class _utilMap< Key, T, stackSize > ;
         public:
            const_iterator()
            {
               _pData      = NULL ;
               _pSrc       = NULL ;
               _pEleSize   = NULL ;
            }
            const_iterator( const const_iterator &rhs )
            {
               _pData      = rhs._pData ;
               _pSrc       = rhs._pSrc ;
               _pEleSize   = rhs._pEleSize ;
               _it         = rhs._it ;
            }
            const_iterator( const iterator &rhs )
            {
               _pData      = rhs._pData ;
               _pSrc       = rhs._pSrc ;
               _pEleSize   = rhs._pEleSize ;
               _it         = rhs._it ;
            }
            BOOLEAN operator== ( const const_iterator &rhs ) const
            {
               if ( _pData && rhs._pData )
               {
                  BOOLEAN leftEnd = _pData >= _pSrc + *_pEleSize ?
                                    TRUE : FALSE ;
                  BOOLEAN rightEnd = rhs._pData >= rhs._pSrc + *(rhs._pEleSize) ?
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
            BOOLEAN operator!= ( const const_iterator &rhs ) const
            {
               return this->operator==( rhs ) ? FALSE : TRUE ;
            }
            const_iterator& operator= ( const const_iterator &rhs )
            {
               _pData         = rhs._pData ;
               _pSrc          = rhs._pSrc ;
               _pEleSize      = rhs._pEleSize ;
               _it            = rhs._it ;
               return *this ;
            }
            const_value_type* operator-> ()
            {
               if ( _pData )
               {
                  return _pData ;
               }
               else
               {
                  return _it.operator->() ;
               }
            }
            const_iterator& operator++ ()
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
            const_iterator& operator-- ()
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
            const_iterator& operator+ ( UINT32 step )
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
            const_iterator& operator- ( UINT32 step )
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
            const_iterator( const i_value_type* pData, const i_value_type *pSrc,
                            const UINT32 *pEleSize )
            {
               _pData         = reinterpret_cast< const_value_type*>( pData ) ;
               _pSrc          = reinterpret_cast< const_value_type*>( pSrc ) ;
               _pEleSize      = pEleSize ;
            }
            const_iterator( typename map< Key, T >::const_iterator it )
            {
               _pData         = NULL ;
               _pSrc          = NULL ;
               _pEleSize      = NULL ;
               _it            = it ;
            }
         private:
            const_iterator operator++ ( int )
            {
               const_iterator tmp( *this ) ;
               ++(*this) ;
               return tmp ;
            }
            const_iterator operator-- ( int )
            {
               const_iterator tmp( *this ) ;
               ++(*this) ;
               return tmp ;
            }

         private:
            const_value_type*                      _pData ;
            const_value_type*                      _pSrc ;
            const UINT32*                          _pEleSize ;
            typename map< Key, T >::const_iterator _it ;
      } ;

   public:
      OSS_INLINE UINT32 size() const
      {
         if ( _pMap )
         {
            return _pMap->size() ;
         }
         return _eleSize ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         if ( _pMap )
         {
            return _pMap->empty() ? TRUE : FALSE ;
         }
         return 0 == _eleSize ? TRUE : FALSE ;
      }

      OSS_INLINE iterator begin()
      {
         if ( _pMap )
         {
            return iterator( _pMap->begin() ) ;
         }
         return iterator( _staticBuf, _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE const_iterator begin() const
      {
         if ( _pMap )
         {
            return const_iterator( iterator( _pMap->begin() ) ) ;
         }
         return const_iterator( _staticBuf, _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE iterator end()
      {
         if ( _pMap )
         {
            return iterator( _pMap->end() ) ;
         }
         return iterator( &_staticBuf[ stackSize ], _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE const_iterator end() const
      {
         if ( _pMap )
         {
            return const_iterator( iterator( _pMap->end() ) ) ;
         }
         return const_iterator( &_staticBuf[ stackSize ], _staticBuf,
                                &_eleSize ) ;
      }

      OSS_INLINE iterator erase( iterator position )
      {
         if ( _pMap )
         {
            _pMap->erase( position._it++ ) ;
         }
         else if ( (UINT32)(position._pData - position._pSrc) < _eleSize )
         {
            --_eleSize ;
            UINT32 pos = ( position._pData - position._pSrc ) ;
            for ( UINT32 i = pos ; i < _eleSize ; ++i )
            {
               _staticBuf[ i ] = _staticBuf[ i + 1 ] ;
            }
         }
         return position ;
      }

      OSS_INLINE UINT32 erase( const Key& key )
      {
         if ( _pMap )
         {
            return (UINT32)_pMap->erase( key ) ;
         }
         else if ( _eleSize > 0 )
         {
            INT32 pos = _findInStackBuf( key ) ;
            if ( pos != -1 )
            {
               --_eleSize ;
               for ( UINT32 i = pos ; i < _eleSize ; ++i )
               {
                  _staticBuf[ i ] = _staticBuf[ i + 1 ] ;
               }
               return 1 ;
            }
         }
         return 0 ;
      }

      OSS_INLINE void erase( iterator first, iterator last )
      {
         if ( _pMap )
         {
            _pMap->erase( first._it, last._it ) ;
         }
         else if ( _eleSize > 0 )
         {
            UINT32 b = ( first._pData - first._pSrc ) ;
            UINT32 e = ( last._pData - last._pSrc ) ;

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
         }
      }

      OSS_INLINE void clear( BOOLEAN resetMem = TRUE )
      {
         if ( _pMap )
         {
            if ( resetMem )
            {
               delete _pMap ;
               _pMap = NULL ;
            }
            else
            {
               _pMap->clear() ;
            }
         }
         _eleSize = 0 ;
      }

      OSS_INLINE pair<iterator,BOOLEAN> insert( const value_type& val )
      {
         if ( SDB_OK != _ensureSpace( _eleSize + 1 ) )
         {
            throw std::out_of_range( "out-of-memory" ) ;
         }

         if ( _pMap )
         {
            pair< typename map< Key, T >::iterator, bool > tmp = _pMap->insert( val ) ;
            return pair<iterator, BOOLEAN>( iterator( tmp.first ),
                                            tmp.second ? TRUE : FALSE ) ;
         }
         else if ( 0 == _eleSize )
         {
            _staticBuf[ 0 ].first = val.first ;
            _staticBuf[ 0 ].second = val.second ;
            ++_eleSize ;
            return pair<iterator, BOOLEAN>( begin(), TRUE ) ;
         }
         else
         {
            BOOLEAN exist = FALSE ;
            INT32 pos = _findBoundInStackBuf( val.first, FALSE, &exist ) ;
            if ( exist )
            {
               return pair<iterator, BOOLEAN>( end(), FALSE ) ;
            }
            else if ( -1 == pos )
            {
               pos = _eleSize ;
            }
            else
            {
               for ( INT32 i = _eleSize ; i > pos ; --i )
               {
                  _staticBuf[ i ] = _staticBuf[ i - 1 ] ;
               }
            }
            ++_eleSize ;
            _staticBuf[ pos ].first = val.first ;
            _staticBuf[ pos ].second = val.second ;

            return pair<iterator, BOOLEAN>( iterator( &_staticBuf[ pos ],
                                                      _staticBuf,
                                                      &_eleSize ),
                                            TRUE ) ;
         }
      }

      OSS_INLINE _utilMap<Key,T,stackSize>& operator= ( const _utilMap<Key,T,stackSize> &rhs )
      {
         UINT32 rSize = rhs.size() ;

         clear( TRUE ) ;
         _ensureSpace( rSize ) ;
         const_iterator it = rhs.begin() ;
         while( it != rhs.end() )
         {
            insert( value_type( it->first, it->second ) ) ;
            ++it ;
         }
         return *this ;
      }

      OSS_INLINE T& at( const Key& key )
      {
         if ( _pMap )
         {
            return _pMap->at( key ) ;
         }
         else
         {
            INT32 pos = _findInStackBuf( key ) ;
            if ( pos != -1 )
            {
               return _staticBuf[ pos ].second ;
            }
            throw std::out_of_range( "out-of-range" ) ;
         }
      }

      OSS_INLINE const T& at( const Key& key ) const
      {
         if ( _pMap )
         {
            return _pMap->at( key ) ;
         }
         else
         {
            INT32 pos = _findInStackBuf( key ) ;
            if ( pos != -1 )
            {
               return _staticBuf[ pos ].second ;
            }
            throw std::out_of_range( "out-of-range" ) ;
         }
      }

      OSS_INLINE T& operator[] ( const Key& key )
      {
         if ( _pMap )
         {
            return _pMap->operator[](key) ;
         }
         else
         {
            INT32 pos = _findInStackBuf( key ) ;
            if ( pos != -1 )
            {
               return _staticBuf[ pos ].second ;
            }
            pair< iterator, BOOLEAN > ret = insert( value_type( key, T() ) ) ;
            return (ret.first)->second ;
         }
      }

      OSS_INLINE iterator find( const Key& key )
      {
         if ( _pMap )
         {
            return iterator( _pMap->find( key ) ) ;
         }
         INT32 pos = _findInStackBuf( key ) ;
         if ( -1 == pos )
         {
            return end() ;
         }
         return iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE const_iterator find( const Key& key ) const
      {
         if ( _pMap )
         {
            return const_iterator( iterator( _pMap->find( key ) ) ) ;
         }
         INT32 pos = _findInStackBuf( key ) ;
         if ( -1 == pos )
         {
            return end() ;
         }
         return const_iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
      }

      OSS_INLINE UINT32 count( const Key& key ) const
      {
         if ( _pMap )
         {
            return (UINT32)_pMap->count( key ) ;
         }
         return _findInStackBuf( key ) == -1 ? 0 : 1 ;
      }

      OSS_INLINE iterator lower_bound( const Key& key )
      {
         if ( _pMap )
         {
            return iterator( _pMap->lower_bound( key ) ) ;
         }
         else
         {
            INT32 pos = _findBoundInStackBuf( key, TRUE ) ;
            if ( -1 != pos )
            {
               return iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
            }
            return end() ;
         }
      }

      OSS_INLINE const_iterator lower_bound( const Key& key ) const
      {
         if ( _pMap )
         {
            return const_iterator( iterator( _pMap->lower_bound( key ) ) ) ;
         }
         else
         {
            INT32 pos = _findBoundInStackBuf( key, TRUE ) ;
            if ( -1 != pos )
            {
               return const_iterator( &_staticBuf[ pos ], _staticBuf,
                                      &_eleSize ) ;
            }
            return end() ;
         }
      }

      OSS_INLINE iterator upper_bound( const Key& key )
      {
         if ( _pMap )
         {
            return iterator( _pMap->upper_bound( key ) ) ;
         }
         else
         {
            INT32 pos = _findBoundInStackBuf( key, FALSE ) ;
            if ( -1 != pos )
            {
               return iterator( &_staticBuf[ pos ], _staticBuf, &_eleSize ) ;
            }
            return end() ;
         }
      }

      OSS_INLINE const_iterator upper_bound( const Key& key ) const
      {
         if ( _pMap )
         {
            return const_iterator( iterator( _pMap->upper_bound( key ) ) ) ;
         }
         else
         {
            INT32 pos = _findBoundInStackBuf( key, FALSE ) ;
            if ( -1 != pos )
            {
               return const_iterator( &_staticBuf[ pos ], _staticBuf,
                                      &_eleSize ) ;
            }
            return end() ;
         }
      }

      OSS_INLINE pair<iterator, iterator> equal_range( const Key& key )
      {
         if ( _pMap )
         {
            pair< typename map< Key, T >::iterator, typename map< Key, T >::iterator > tmp =
               _pMap->equal_range( key ) ;
            return pair< iterator, iterator >( iterator( tmp.first ),
                                               iterator( tmp.second ) ) ;
         }
         else
         {
            iterator itBegin = end() ;
            iterator itEnd = end() ;

            BOOLEAN equal = FALSE ;
            INT32 pos = _findBoundInStackBuf( key, TRUE, &equal ) ;
            if ( -1 != pos && equal )
            {
               itBegin = iterator( &_staticBuf[ pos ], _staticBuf,
                                   &_eleSize ) ;
               for ( UINT32 i = pos + 1 ; i < _eleSize ; ++i )
               {
                  if ( _staticBuf[ i ].first != key )
                  {
                     itEnd = iterator( &_staticBuf[ i ], _staticBuf,
                                       &_eleSize ) ;
                     break ;
                  }
               }
            }
            return pair< iterator, iterator >( itBegin, itEnd ) ;
         }
      }

      OSS_INLINE pair<const_iterator, const_iterator> equal_range( const Key& key ) const
      {
         if ( _pMap )
         {
            pair< typename map< Key, T >::const_iterator, typename map< Key, T >::const_iterator > tmp =
               _pMap->equal_range( key ) ;
            return pair< const_iterator, const_iterator >( const_iterator( tmp.first ),
                                                           const_iterator( tmp.second ) ) ;
         }
         else
         {
            const_iterator itBegin = end() ;
            const_iterator itEnd = end() ;

            BOOLEAN equal = FALSE ;
            INT32 pos = _findBoundInStackBuf( key, TRUE, &equal ) ;
            if ( -1 != pos && equal )
            {
               itBegin = const_iterator( &_staticBuf[ pos ], _staticBuf,
                                         &_eleSize ) ;
               for ( UINT32 i = pos + 1 ; i < _eleSize ; ++i )
               {
                  if ( _staticBuf[ i ].first != key )
                  {
                     itEnd = const_iterator( &_staticBuf[ i ], _staticBuf,
                                             &_eleSize ) ;
                     break ;
                  }
               }
            }
            return pair< const_iterator, const_iterator >( itBegin, itEnd ) ;
         }
      }

      OSS_INLINE void shift2Stack( UINT32 divisor = 8 )
      {
         if ( _pMap )
         {
            UINT32 threshold = 0 ;
            if ( divisor > 0 )
            {
               threshold = stackSize / divisor ;
            }
            if ( _pMap->size() <= threshold )
            {
               _eleSize = 0 ;
               typename map< Key, T >::iterator it = _pMap->begin() ;
               for ( ; it != _pMap->end() ; ++it )
               {
                  _staticBuf[ _eleSize ].first = it->first ;
                  _staticBuf[ _eleSize ].second = it->second ;
                  ++_eleSize ;
               }
               delete _pMap ;
               _pMap = NULL ;
            }
         }
      }

   protected:
      OSS_INLINE INT32 _findInStackBuf( const Key& key ) const
      {
         if ( 0 == _eleSize )
         {
            return -1 ;
         }

         INT32 l = 0 ;
         INT32 h = _eleSize - 1 ;
         INT32 m = 0 ;

         while( l <= h )
         {
            m = ( l + h ) / 2 ;
            if ( _staticBuf[ m ].first == key ) /// find
            {
               return m ;
            }
            else if ( _staticBuf[ m ].first < key )
            {
               l = m + 1 ;
            }
            else
            {
               h = m - 1 ;
            }
         }
         return -1 ;
      }

      OSS_INLINE INT32 _findBoundInStackBuf( const Key& key,
                                             BOOLEAN includeEqual,
                                             BOOLEAN *pEqual = NULL ) const
      {
         if ( 0 == _eleSize )
         {
            return -1 ;
         }

         if ( pEqual )
         {
            *pEqual = FALSE ;
         }

         INT32 l = 0 ;
         INT32 h = _eleSize - 1 ;
         INT32 m = 0 ;

         while( l <= h )
         {
            m = ( l + h ) / 2 ;
            if ( _staticBuf[ m ].first == key ) /// find
            {
               if ( pEqual )
               {
                  *pEqual = TRUE ;
               }
               if ( includeEqual )
               {
                  h = m - 1 ;
               }
               else
               {
                  l = m + 1 ;
               }
            }
            else if ( _staticBuf[ m ].first < key )
            {
               l = m + 1 ;
            }
            else
            {
               h = m - 1 ;
            }
         }
         return (UINT32)l >= _eleSize ? -1 : l ;
      }

      OSS_INLINE UINT32 _capacity() const
      {
         if ( _pMap )
         {
            return _pMap->size() ;
         }
         return stackSize ;
      }

      OSS_INLINE INT32 _ensureSpace( UINT32 size )
      {
         INT32 rc = SDB_OK ;

         if ( !_pMap && size > stackSize )
         {
            _pMap = new (std::nothrow) map< Key, T >() ;
            if ( !_pMap )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            for ( UINT32 i = 0 ; i < _eleSize ; ++i )
            {
               (*_pMap)[ _staticBuf[ i ].first ] = _staticBuf[ i ].second ;
               _staticBuf[ i ].second = T() ;
            }
            _eleSize = 0 ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      i_value_type            _staticBuf[ stackSize ] ;
      map< Key, T >*          _pMap ;
      UINT32                  _eleSize ;

   } ;

   /*
      _utilMapStringKey define
    */
   struct _utilMapStringKey
   {
      OSS_INLINE _utilMapStringKey ( const CHAR * pString = NULL )
      {
         _pString = pString ;
      }

      OSS_INLINE _utilMapStringKey( const _utilMapStringKey &key )
      {
         _pString = key._pString ;
      }

      OSS_INLINE bool operator== ( const _utilMapStringKey &key ) const
      {
         return ossStrcmp( _pString, key._pString ) == 0 ;
      }

      OSS_INLINE bool operator< ( const _utilMapStringKey &key ) const
      {
         return ossStrcmp( _pString, key._pString ) < 0 ;
      }

      OSS_INLINE _utilMapStringKey &operator= ( const _utilMapStringKey &key )
      {
         _pString = key._pString ;
         return (*this) ;
      }

      const CHAR * _pString ;
   } ;

   template < typename T, UINT32 stackSize = UTIL_MAP_DEFAULT_STACK_SIZE >
   class _utilStringMap : public _utilMap< _utilMapStringKey, T, stackSize >
   {
      private :
         OSS_INLINE T& operator[] ( const _utilMapStringKey& key )
         {
            return _utilMap< _utilMapStringKey, T, stackSize >::operator[]( key ) ;
         }
   } ;

}

#endif // UTIL_MAP_HPP_

