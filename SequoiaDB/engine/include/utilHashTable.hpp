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

   Source File Name = utilHashTable.hpp

   Descriptive Name = utility of hash table header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for hash table

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2017  HGM Initial draft

   Last Changed =

*******************************************************************************/

#ifndef UTILHASHTABLE_HPP__
#define UTILHASHTABLE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilPooledObject.hpp"
#include "ossRWMutex.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define UTIL_HASH_TABLE_DFT_BUCKET_NUM  ( 128 )
   #define UTIL_HASH_TABLE_MAX_BUCKET_NUM  ( 4096 )
   #define UTIL_HASH_TABLE_DFT_LOCK_NUM    ( 64 )

   class _utilHashTableListItem ;
   class _utilHashTableList ;

   /*
      _utilHashTableListItem define
    */
   class _utilHashTableListItem : public _utilPooledObject
   {
      public :
         _utilHashTableListItem ()
         : _pList( NULL ),
           _pPrev( NULL ),
           _pNext( NULL )
         {
         }

         virtual ~_utilHashTableListItem () {}

         OSS_INLINE void setItem ( _utilHashTableList *pList,
                                   _utilHashTableListItem *pPrev,
                                   _utilHashTableListItem *pNext )
         {
            _pList = pList ;
            _pPrev = pPrev ;
            _pNext = pNext ;
         }

         OSS_INLINE _utilHashTableList *getList ()
         {
            return _pList ;
         }

         OSS_INLINE void setList ( _utilHashTableList *pList )
         {
            _pList = pList ;
         }

         OSS_INLINE _utilHashTableListItem *getPrev ()
         {
            return _pPrev ;
         }

         OSS_INLINE void setPrev ( _utilHashTableListItem *pPrev )
         {
            _pPrev = pPrev ;
         }

         OSS_INLINE _utilHashTableListItem *getNext ()
         {
            return _pNext ;
         }

         OSS_INLINE void setNext ( _utilHashTableListItem *pNext )
         {
            _pNext = pNext ;
         }

      protected :
         _utilHashTableList *       _pList ;
         _utilHashTableListItem *   _pPrev ;
         _utilHashTableListItem *   _pNext ;
   } ;

   /*
      _utilHashTableList define
    */
   class _utilHashTableList : public SDBObject
   {
      public :
         _utilHashTableList ()
         : _pHead( NULL ),
           _pTail( NULL )
         {
         }

         virtual ~_utilHashTableList () {}

         OSS_INLINE _utilHashTableListItem *getHead ()
         {
            return _pHead ;
         }

         OSS_INLINE _utilHashTableListItem *getTail ()
         {
            return _pTail ;
         }

         OSS_INLINE void addItem ( _utilHashTableListItem *pItem )
         {
            if ( NULL != pItem->getList() )
            {
               return ;
            }

            if ( NULL != _pTail )
            {
               _pTail->setNext( pItem ) ;
               pItem->setItem( this, _pTail, NULL ) ;
               _pTail = pItem ;
            }
            else
            {
               pItem->setItem( this, NULL, NULL ) ;
               _pHead = pItem ;
               _pTail = pItem ;
            }
         }

         OSS_INLINE BOOLEAN removeItem ( _utilHashTableListItem *pItem )
         {
            if ( pItem->getList() != this )
            {
               return FALSE ;
            }

            _utilHashTableListItem *pPrev = pItem->getPrev() ;
            _utilHashTableListItem *pNext = pItem->getNext() ;

            if ( _pHead == pItem )
            {
               _pHead = pNext ;
            }
            if ( _pTail == pItem )
            {
               _pTail = pPrev ;
            }
            if ( pPrev )
            {
               pPrev->setNext( pNext ) ;
            }
            if ( pNext )
            {
               pNext->setPrev( pPrev ) ;
            }

            pItem->setItem( NULL, NULL, NULL ) ;

            return TRUE ;
         }

         OSS_INLINE void clearList ( BOOLEAN deleteItem )
         {
            while ( NULL != _pHead )
            {
               _utilHashTableListItem *pCurItem = _pHead ;
               removeItem( pCurItem ) ;
               if ( deleteItem )
               {
                  SDB_OSS_DEL pCurItem ;
               }
            }
         }

      protected :
         _utilHashTableListItem * _pHead ;
         _utilHashTableListItem * _pTail ;
   } ;

   /*
      _utilHashTableKey define
    */
   class _utilHashTableKey
   {
      public :
         _utilHashTableKey ()
         : _keyCode( 0 )
         {
         }

         _utilHashTableKey ( const _utilHashTableKey & key )
         : _keyCode( key._keyCode )
         {
         }

         virtual ~_utilHashTableKey () {}

         OSS_INLINE virtual UINT32 getKeyCode () const
         {
            return _keyCode ;
         }

         OSS_INLINE virtual void setKeyCode ( UINT32 keyCode )
         {
            _keyCode = _getHashCode( keyCode ) ;
         }

      protected :
         OSS_INLINE static UINT32 _getHashCode ( UINT32 keyCode )
         {
            return keyCode ;
         }

         OSS_INLINE static UINT32 _getMurmur3HashCode ( UINT32 keyCode )
         {
            // Murmur3 hash code
            keyCode ^= keyCode >> 16 ;
            keyCode *= 0x85ebca6b ;
            keyCode ^= keyCode >> 13 ;
            keyCode *= 0xc2b2ae35 ;
            keyCode ^= keyCode >> 16 ;
            return keyCode ;
         }

      protected :
         UINT32 _keyCode ;
   } ;

   /*
      _utilHashTableBucket define
   */
   class _utilHashTableItem : public _utilHashTableListItem
   {
      public :
         _utilHashTableItem () : _utilHashTableListItem() {}

         virtual ~_utilHashTableItem () {}

         OSS_INLINE virtual UINT32 getKeyCode () const = 0 ;
   } ;

   /*
      _utilHashTableBucket define
    */
   template < class utilHashTableKey, class utilHashTableItem >
   class _utilHashTableBucket : public SDBObject
   {
      public :
         _utilHashTableBucket ()
         {
         }

         virtual ~_utilHashTableBucket ()
         {
            clearBucket() ;
         }

         void clearBucket ()
         {
            _list.clearList ( TRUE ) ;
         }

         // Get item matched with given key from bucket
         // NOTE: should be called with shared lock
         utilHashTableItem *getItem ( const utilHashTableKey &key )
         {
            utilHashTableItem *pCurItem = (utilHashTableItem *)_list.getHead() ;
            while ( NULL != pCurItem )
            {
               if ( pCurItem->isEqual( key ) )
               {
                  break ;
               }
               pCurItem = (utilHashTableItem *)pCurItem->getNext() ;
            }
            return pCurItem ;
         }

         // Add item into bucket
         // NOTE: should be called with exclusively lock
         BOOLEAN addItem ( utilHashTableItem *pItem )
         {
            SDB_ASSERT( pItem != NULL, "pItem is invalid" ) ;

            utilHashTableItem *pCurItem = getItem( pItem->getKey() ) ;
            if ( NULL != pCurItem )
            {
               return FALSE ;
            }

            _list.addItem( pItem ) ;

            return TRUE ;
         }

         // Remove item from bucket
         // NOTE: should be called with exclusive lock
         BOOLEAN removeItem ( utilHashTableItem *pItem )
         {
            SDB_ASSERT( pItem != NULL, "pItem is invalid" ) ;
            return _list.removeItem( pItem ) ;
         }

         // Get head of bucket
         // NOTE: should be called with lock
         OSS_INLINE utilHashTableItem *getHead ()
         {
            return (utilHashTableItem *)_list.getHead() ;
         }

      protected :
         _utilHashTableList   _list ;
   } ;

   /*
      _utilHashTable define
    */
   template < class utilHashTableKey,
              class utilHashTableItem,
              UINT32 lockNum = UTIL_HASH_TABLE_DFT_LOCK_NUM >
   class _utilHashTable
   {
      protected :
         typedef _utilHashTableBucket< utilHashTableKey, utilHashTableItem >
                 utilHashTableBucket ;

         typedef _utilHashTable< utilHashTableKey, utilHashTableItem, lockNum >
                 utilHashTable ;

      public :
         _utilHashTable ()
         : _bucketNum( 0 ),
           _bucketModulo( 0 ),
           _lockModulo( lockNum - 1 ),
           _buckets( NULL ),
           _enableAddItem( FALSE )
         {
            SDB_ASSERT( 16 == lockNum || 32 == lockNum || 64 == lockNum ||
                        128 == lockNum || 256 == lockNum,
                        "Invalid number of locks" ) ;
         }

         virtual ~_utilHashTable ()
         {
            deinitialize() ;
         }

         BOOLEAN initialize ( UINT32 bucketNum = UTIL_HASH_TABLE_DFT_BUCKET_NUM )
         {
            ossScopedRWLock scopedLock( &_bucketNumLock, EXCLUSIVE ) ;

            if ( isInitialized() )
            {
               return TRUE ;
            }

            SDB_ASSERT( getRoundedUpBucketNum( bucketNum ) == bucketNum,
                        "bucket number is invalid" ) ;

            if ( 0 == bucketNum )
            {
               return FALSE ;
            }

            _lockBuckets( EXCLUSIVE ) ;

            _buckets = new(std::nothrow) utilHashTableBucket[ bucketNum ] ;

            if ( _buckets != NULL )
            {
               _bucketNum = bucketNum ;
               _bucketModulo = bucketNum - 1 ;
            }

            _unlockBuckets( EXCLUSIVE ) ;

            return ( _bucketNum > 0 ? TRUE : FALSE ) ;
         }

         void deinitialize ()
         {
            ossScopedRWLock scopedLock( &_bucketNumLock, EXCLUSIVE ) ;

            _lockBuckets( EXCLUSIVE ) ;

            if ( _buckets != NULL )
            {
               delete [] _buckets ;
               _buckets = NULL ;
            }

            _bucketNum = 0 ;
            _bucketModulo = 0 ;

            _unlockBuckets( EXCLUSIVE ) ;
         }

         OSS_INLINE UINT32 getBucketNum () const
         {
            return _bucketNum ;
         }

         OSS_INLINE UINT32 getRoundedUpBucketNum ( UINT32 bucketNum )
         {
            UINT32 roundedBucketNum = 0 ;
            if ( bucketNum > 0 )
            {
               // Bucket number should be power of 2, between 128 and 4096
               for ( roundedBucketNum = UTIL_HASH_TABLE_DFT_BUCKET_NUM ;
                     roundedBucketNum < UTIL_HASH_TABLE_MAX_BUCKET_NUM ;
                     roundedBucketNum <<= 1 )
               {
                  if ( bucketNum <= roundedBucketNum )
                  {
                     break ;
                  }
               }
            }
            return roundedBucketNum ;
         }

         OSS_INLINE UINT32 getLockNum () const
         {
            return lockNum ;
         }

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return ( NULL != _buckets ) ;
         }

         utilHashTableItem *getItem ( const utilHashTableKey &key )
         {
            utilHashTableItem *pItem = NULL ;
            UINT32 bucketID = 0 ;
            utilHashTableBucket *pBucket =
                  getBucketByKey( key.getKeyCode(), SHARED, bucketID ) ;

            if ( NULL == pBucket )
            {
               return NULL ;
            }

            pItem = pBucket->getItem( key ) ;

            if ( NULL != pItem )
            {
               _afterGetItem( bucketID, pItem ) ;
            }

            releaseBucket( bucketID, SHARED ) ;

            return pItem ;
         }

         BOOLEAN addItem ( utilHashTableItem *pItem )
         {
            SDB_ASSERT( pItem != NULL, "pItem is invalid" ) ;

            BOOLEAN added = FALSE ;
            UINT32 bucketID = 0 ;
            utilHashTableBucket *pBucket =
                  getBucketByKey( pItem->getKeyCode(), EXCLUSIVE, bucketID ) ;

            if ( NULL == pBucket )
            {
               return FALSE ;
            }

            if ( _enableAddItem )
            {
               added = pBucket->addItem( pItem ) ;
               if ( added )
               {
                  _afterAddItem( bucketID, pItem ) ;
               }
            }

            releaseBucket( bucketID, EXCLUSIVE ) ;

            return added ;
         }

         BOOLEAN removeItem ( utilHashTableItem *pItem )
         {
            SDB_ASSERT( pItem != NULL, "pItem is invalid" ) ;

            BOOLEAN removed = FALSE ;
            UINT32 bucketID = 0 ;
            utilHashTableBucket *pBucket =
                  getBucketByKey( pItem->getKeyCode(), EXCLUSIVE, bucketID ) ;

            if ( NULL == pBucket )
            {
               return FALSE ;
            }

            removed = pBucket->removeItem( pItem ) ;

            if ( removed )
            {
               _afterRemoveItem( bucketID, pItem ) ;
            }

            releaseBucket( bucketID, EXCLUSIVE ) ;

            return removed ;
         }

         utilHashTableBucket *getBucketByKey ( UINT32 keyCode,
                                               OSS_LATCH_MODE mode,
                                               UINT32 &bucketID )
         {
            UINT32 lockID = keyCode & _lockModulo ;
            _lock( lockID, mode ) ;

            bucketID = keyCode & _bucketModulo ;
            if ( bucketID < _bucketNum )
            {
               return &( _buckets[ bucketID ] ) ;
            }

            _unlock( lockID, mode ) ;
            return NULL ;
         }

         utilHashTableBucket *getBucket ( UINT32 bucketID,
                                          OSS_LATCH_MODE mode )
         {
            UINT32 lockID = bucketID & _lockModulo ;

            _lock( lockID, mode ) ;
            if ( bucketID < _bucketNum )
            {
               return &( _buckets[ bucketID ] ) ;
            }

            _unlock( lockID, mode ) ;
            return NULL ;
         }

         void releaseBucket ( UINT32 bucketID, OSS_LATCH_MODE mode )
         {
            UINT32 lockID = bucketID & _lockModulo ;
            _unlock( lockID, mode ) ;
         }

      protected :
         // Hooks
         virtual void _afterAddItem ( UINT32 bucketID,
                                      utilHashTableItem *pItem )
         {
         }

         virtual void _afterGetItem ( UINT32 bucketID,
                                      utilHashTableItem *pItem )
         {
         }

         virtual void _afterRemoveItem ( UINT32 bucketID,
                                         utilHashTableItem *pItem )
         {
         }

         virtual void _setEnableAddItem ( BOOLEAN enableAddItem )
         {
            _lockBuckets( EXCLUSIVE ) ;
            _enableAddItem = enableAddItem ;
            _unlockBuckets( EXCLUSIVE ) ;
         }

         virtual void _lockBuckets ( OSS_LATCH_MODE mode )
         {
            for ( UINT32 lockID = 0 ; lockID < lockNum ; lockID ++ )
            {
               _lock( lockID, mode ) ;
            }
         }

         virtual void _unlockBuckets ( OSS_LATCH_MODE mode )
         {
            for ( UINT32 lockID = 0 ; lockID < lockNum ; lockID ++ )
            {
               _unlock( lockID, mode ) ;
            }
         }

         virtual void _lock ( UINT32 lockID, OSS_LATCH_MODE mode )
         {
            SDB_ASSERT( lockID < lockNum, "lockID is invalid" ) ;

            if ( EXCLUSIVE == mode )
            {
               _locks[ lockID ].lock_w() ;
            }
            else
            {
               _locks[ lockID ].lock_r() ;
            }
         }

         virtual void _unlock ( UINT32 lockID, OSS_LATCH_MODE mode )
         {
            SDB_ASSERT( lockID < lockNum, "lockID is invalid" ) ;

            if ( EXCLUSIVE == mode )
            {
               _locks[ lockID ].release_w() ;
            }
            else
            {
               _locks[ lockID ].release_r() ;
            }
         }

      protected :
         UINT32 _bucketNum ;
         UINT32 _bucketModulo ;
         UINT32 _lockModulo ;

         utilHashTableBucket *_buckets ;
         ossRWMutex _locks[ lockNum ] ;

         ossRWMutex _bucketNumLock ;
         BOOLEAN    _enableAddItem ;
   } ;

}

#endif //UTILHASHTABLE_HPP__
