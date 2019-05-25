/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilZlibStream.hpp

   Descriptive Name = Zlib compression stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/5/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_CONCURRENT_MAP_
#define UTIL_CONCURRENT_MAP_

#include "ossLatch.hpp"
#include <boost/functional/hash.hpp>
#include <map>

namespace engine
{
   #define UTIL_CONCURRENT_MAP_DEFAULT_BUCKET_NUM  32
   #define UTIL_CONCURRENT_MAP_MAX_BUCKET_NUM      128

   template < class Key,
              class T,
              INT32 BUCKET_NUM = UTIL_CONCURRENT_MAP_DEFAULT_BUCKET_NUM,
              class Hash = boost::hash<Key> >
   class utilConcurrentMap: public SDBObject
   {
   public:
      typedef std::pair< const Key, T > value_type ;
      typedef typename std::map< Key, T > map_type ;
      typedef typename std::map< Key, T >::iterator map_iterator ;
      typedef typename std::map< Key, T >::const_iterator map_const_iterator ;
   private:
      typedef utilConcurrentMap< Key, T, BUCKET_NUM, Hash > cmap_type ;

   private:
      utilConcurrentMap( const utilConcurrentMap& ) ;
      void operator=( const utilConcurrentMap& ) ;

   public:
      utilConcurrentMap()
      {
         SDB_ASSERT( BUCKET_NUM <= UTIL_CONCURRENT_MAP_MAX_BUCKET_NUM &&
                     BUCKET_NUM > 1,
            "bucket num must > 1 and <= UTIL_CONCURRENT_MAP_MAX_BUCKET_NUM" ) ;
         SDB_ASSERT( BUCKET_NUM % 2 == 0, "bucket num must be a power of 2" ) ;
      }

      ~utilConcurrentMap()
      {
      }

   public:
      class Bucket: public ossSpinSLatch
      {
         friend class utilConcurrentMap< Key, T, BUCKET_NUM, Hash > ;
      private:
         Bucket( const Bucket& ) ;
         void operator=( const Bucket& ) ;

      public:
         Bucket() {}
         ~Bucket() {}
         
         OSS_INLINE map_iterator begin()
         {
            return _map.begin() ;
         }

         OSS_INLINE map_const_iterator begin() const
         {
            return _map.begin() ;
         }

         OSS_INLINE map_iterator end()
         {
            return _map.end() ;
         }

         OSS_INLINE map_const_iterator end() const
         {
            return _map.end() ;
         }

         OSS_INLINE BOOLEAN empty() const
         {
            return _map.empty() ;
         }

         OSS_INLINE UINT32 size() const
         {
            return _map.size() ;
         }

         OSS_INLINE std::pair<map_iterator,bool> insert( const value_type& val )
         {
            return _map.insert( val ) ;
         }

         OSS_INLINE UINT32 erase( const Key& key )
         {
            return _map.erase( key ) ;
         }

         OSS_INLINE map_const_iterator find( const Key& key ) const
         {
            return _map.find( key ) ;
         }

         OSS_INLINE void clear()
         {
            _map.clear() ;
         }

      private:
         map_type _map ;
      } ;

      #define BUCKET_XLOCK( _bucket ) ossScopedLock __lock( &(_bucket), EXCLUSIVE )
      #define BUCKET_SLOCK( _bucket ) ossScopedLock __lock( &(_bucket), SHARED )

   private:
      OSS_INLINE INT32 _getBucketIndex( const Key& key ) const
      {
         UINT32 keyHash = _hasher( key ) ;
         return (INT32)( keyHash & ( BUCKET_NUM - 1 ) ) ;
      }

      OSS_INLINE Bucket& _bucketAt( INT32 index )
      {
         SDB_ASSERT( index >= 0 && index < BUCKET_NUM,
                     "bucket index out of range" ) ;

         return _buckets[ index ] ;
      }

   public:
      OSS_INLINE Bucket& getBucket( const Key& key )
      {
         INT32 index = _getBucketIndex( key ) ;
         return _bucketAt( index ) ;
      }

      OSS_INLINE UINT32 size( BOOLEAN lock = TRUE )
      {
         UINT32 count = 0 ;

         for ( INT32 i = 0 ; i < BUCKET_NUM ; i++ )
         {
            Bucket& bucket = _bucketAt( i ) ;
            if ( lock )
            {
               BUCKET_SLOCK( bucket ) ;
               count += bucket.size() ;
            }
            else
            {
               count += bucket.size() ;
            }
         }

         return count ;
      }

      OSS_INLINE bool empty()
      {
         return size() == 0 ;
      }

      OSS_INLINE std::pair<map_iterator,bool> insert( const value_type& val )
      {
         Bucket& bucket = getBucket( val.first ) ;
         BUCKET_XLOCK( bucket ) ;
         return bucket.insert( val ) ;
      }

      OSS_INLINE std::pair<map_iterator,bool> insert( Key& key, T& value )
      {
         return insert( value_type( key, value ) ) ;
      }

      OSS_INLINE void erase( const Key& key )
      {
         Bucket& bucket = getBucket( key ) ;
         BUCKET_XLOCK( bucket ) ;
         bucket.erase( key ) ;
      }

      OSS_INLINE std::pair<T, bool> find( const Key& key )
      {
         Bucket& bucket = getBucket( key ) ;
         BUCKET_SLOCK( bucket ) ;
         map_const_iterator it = bucket.find( key ) ;
         if ( it == bucket.end() )
         {
            return std::pair<T, bool>( T(), false ) ;
         }
         else
         {
            return std::pair<T, bool>( (*it).second, true ) ;
         }
      }

      OSS_INLINE void clear( BOOLEAN lock = TRUE )
      {
         for ( INT32 i = 0 ; i < BUCKET_NUM ; i++ )
         {
            Bucket& bucket = _bucketAt( i ) ;
            if ( lock )
            {
               BUCKET_XLOCK( bucket ) ;
               bucket.clear() ;
            }
            else
            {
               bucket.clear() ;
            }
         }
      }

   public:
      class bucket_iterator: public SDBObject
      {
         friend class utilConcurrentMap< Key, T, BUCKET_NUM, Hash > ;
      public:
         bucket_iterator()
         {
            _index = -1 ;
            _map = NULL ;
         }

         bucket_iterator( const bucket_iterator& rhs )
         {
            _index = rhs._index ;
            _map = rhs._map ;
         }

         OSS_INLINE INT32 index() const
         {
            return _index ;
         }

         bool operator== ( const bucket_iterator& rhs ) const
         {
            if ( _map != rhs._map )
            {
               return false ;
            }

            if ( _index == rhs._index )
            {
               return true ;
            }

            if ( ( _index < 0 || _index >= BUCKET_NUM ) &&
                 ( rhs._index < 0 || rhs._index >= BUCKET_NUM ) )
            {
               return true ;
            }

            return false ;
         }

         bool operator!= ( const bucket_iterator& rhs ) const
         {
            return !this->operator==( rhs ) ;
         }

         bucket_iterator& operator= ( const bucket_iterator& rhs )
         {
            _index = rhs._index ;
            _map = rhs._map ;
            return *this ;
         }

         bucket_iterator& operator++ ()
         {
            if ( _index < BUCKET_NUM )
            {
               _index++ ;
            }

            return *this ;
         }

         bucket_iterator operator++ ( int )
         {
            bucket_iterator tmp( *this ) ;
            ++(*this) ;
            return tmp ;
         }

         bucket_iterator& operator-- ()
         {
            if ( _index >= 0 )
            {
               _index-- ;
            }

            return *this ;
         }

         bucket_iterator operator-- ( int )
         {
            bucket_iterator tmp( *this ) ;
            --(*this) ;
            return tmp ;
         }

         Bucket& operator-> ()
         {
            SDB_ASSERT( _index >= 0 && _index < BUCKET_NUM,
                        "bucket index out of range" ) ;
            SDB_ASSERT( _map != NULL, "_map is null" ) ;

            return _map->_bucketAt( _index ) ;
         }

         Bucket& operator* ()
         {
            SDB_ASSERT( _index >= 0 && _index < BUCKET_NUM,
                        "bucket index out of range" ) ;
            SDB_ASSERT( _map != NULL, "_map is null" ) ;

            return _map->_bucketAt( _index ) ;
         }

      private:
         bucket_iterator( INT32 index, cmap_type* map )
         {
            SDB_ASSERT( index >= 0 && index <= BUCKET_NUM, 
                        "bucket index out of range" ) ;
            SDB_ASSERT( map != NULL, "_map is null" ) ;
            
            _index = index ;
            _map = map ;
         }

      private:
         INT32 _index ;
         cmap_type* _map ;
      } ;

   public:
      OSS_INLINE bucket_iterator begin() const
      {
         return bucket_iterator( 0, (cmap_type*)this ) ;
      }

      OSS_INLINE bucket_iterator end() const
      {
         return bucket_iterator( BUCKET_NUM, (cmap_type*)this ) ;
      }

   private:
      Bucket   _buckets[ BUCKET_NUM ] ;
      Hash     _hasher ;
   } ;

   #define FOR_EACH_CMAP_BUCKET_S( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ; \
         BUCKET_SLOCK( bucket ) ;

   #define FOR_EACH_CMAP_BUCKET_X( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ; \
         BUCKET_XLOCK( bucket ) ;

   #define FOR_EACH_CMAP_BUCKET( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ;

   #define FOR_EACH_CMAP_BUCKET_END }

   #define FOR_EACH_CMAP_ELEMENT_S( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ; \
         BUCKET_SLOCK( bucket ) ; \
      \
         for ( _MAP_TYPE::map_const_iterator it = bucket.begin() ; \
               it != bucket.end() ; \
               it++ ) \
         {

   #define FOR_EACH_CMAP_ELEMENT_X( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ; \
         BUCKET_XLOCK( bucket ) ; \
      \
         for ( _MAP_TYPE::map_const_iterator it = bucket.begin() ; \
               it != bucket.end() ; \
               it++ ) \
         {

   #define FOR_EACH_CMAP_ELEMENT( _MAP_TYPE, _map ) \
      for ( _MAP_TYPE::bucket_iterator bucketIt = (_map).begin() ; \
            bucketIt != (_map).end() ; \
            bucketIt++ ) \
      { \
         _MAP_TYPE::Bucket& bucket = *bucketIt ; \
      \
         for ( _MAP_TYPE::map_const_iterator it = bucket.begin() ; \
               it != bucket.end() ; \
               it++ ) \
         {

   #define FOR_EACH_CMAP_ELEMENT_END }}
}

#endif /* UTIL_CONCURRENT_MAP_ */

