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

   Source File Name = utilCache.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_CACHE_HPP_
#define UTIL_CACHE_HPP_

#include "oss.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "ossEvent.hpp"
#include "ossUtil.hpp"
#include "sdbInterface.hpp"
#include <vector>
#include "ossMemPool.hpp"

using namespace std ;


namespace engine
{

   #define UTIL_PAGE_SLOT_BEGIN_SIZE            ( 256 )  /// BYTE
   #define UTIL_PAGE_SLOT_SIZE                  ( 22 )   /// 512M

   /// totalSize / maxCacheSize
   #define UTIL_CACHE_RATIO                     ( 20 )   /// >=20%

   /*
      Write newest mask
   */
   #define UTIL_WRITE_NEWEST_HEADER             0x00000001
   #define UTIL_WRITE_NEWEST_TAIL               0x00000002
   #define UTIL_WRITE_NEWEST_BOTH               ( UTIL_WRITE_NEWEST_HEADER |\
                                                  UTIL_WRITE_NEWEST_TAIL )

   class _utilCacheMgr ;
   class _utilCacheUnit ;

   /*
      _utilCacheBlock define
   */
   struct _utilCacheBlock
   {
      CHAR*       _pBuff ;
      UINT32      _size ;

      _utilCacheBlock()
      {
         _pBuff      = NULL ;
         _size       = 0 ;
      }
      _utilCacheBlock( CHAR *pBuff, UINT32 size )
      {
         _pBuff      = pBuff ;
         _size       = size ;
      }
      BOOLEAN empty() const
      {
         return ( NULL == _pBuff || 0 == _size ) ? TRUE : FALSE ;
      }
   } ;
   typedef _utilCacheBlock                   utilCacheBlock ;
   typedef vector< utilCacheBlock >          utilCacheBlockSuit ;

   #define UTIL_CACHE_PAGE_DIRTY_FLAG        0x01
   #define UTIL_CACHE_PAGE_INVALID_FLAG      0x02
   #define UTIL_CACHE_PAGE_NEWEST_HEAD_FLAG  0x10
   #define UTIL_CACHE_PAGE_NEWEST_TAIL_FLAG  0x20

   /*
      _utilCachePage define
   */
   class _utilCachePage : public SDBObject
   {
      friend class _utilCacheMgr ;
      public:
         _utilCachePage() ;
         ~_utilCachePage() ;
         _utilCachePage( const _utilCachePage& right ) ;

         _utilCachePage& operator= ( const _utilCachePage& rhs ) ;

         void        clearDataInfo() ;
         void        clearLSNInfo() ;

         BOOLEAN     isDataEmpty() const ;

         UINT32      size() const ;
         UINT32      start() const { return _start ; }
         UINT32      length() const { return _length ; }
         UINT32      dirtyStart() const { return _dirtyStart ; }
         UINT32      dirtyLength() const { return _dirtyLength ; }
         UINT64      lastTime() const { return _lastTime ; }
         UINT64      lastWriteTime() const { return _lastWriteTime ; }
         UINT32      readTimes() const { return _readTimes ; }
         UINT32      writeTimes() const { return _writeTimes ; }

         void        makeDirty()
         {
            OSS_BIT_SET( _status, UTIL_CACHE_PAGE_DIRTY_FLAG ) ;
         }
         void        clearDirty()
         {
            OSS_BIT_CLEAR( _status, UTIL_CACHE_PAGE_DIRTY_FLAG ) ;
            _dirtyStart = 0 ;
            _dirtyLength = 0 ;
            clearLSNInfo() ;
         }
         void        restoryDirty( UINT32 dirtyStart, UINT32 dirtyLength,
                                   UINT64 beginLSN, UINT64 endLSN )
         {
            OSS_BIT_SET( _status, UTIL_CACHE_PAGE_DIRTY_FLAG ) ;

            if ( 0 == _dirtyLength || dirtyStart < _dirtyStart )
            {
               _dirtyStart = dirtyStart ;
            }
            if ( dirtyLength > _dirtyLength )
            {
               _dirtyLength = dirtyLength ;
            }

            if ( 0 == _lsnNum && (UINT64)~0 != beginLSN )
            {
               addLSN( beginLSN ) ;
               if ( endLSN != beginLSN )
               {
                  addLSN( endLSN ) ;
               }
            }
         }
         BOOLEAN     isDirty() const
         {
            return OSS_BIT_TEST( _status, UTIL_CACHE_PAGE_DIRTY_FLAG ) ?
                   TRUE : FALSE ;
         }
         void        invalidate()
         {
            SDB_ASSERT( !isDirty(), "Can't be dirty" ) ;
            OSS_BIT_SET( _status, UTIL_CACHE_PAGE_INVALID_FLAG ) ;
         }
         void        validate()
         {
            OSS_BIT_CLEAR( _status, UTIL_CACHE_PAGE_INVALID_FLAG ) ;
         }
         BOOLEAN     isInvalid() const
         {
            return OSS_BIT_TEST( _status, UTIL_CACHE_PAGE_INVALID_FLAG ) ?
                   TRUE : FALSE ;
         }
         void        pin()
         {
            ossFetchAndIncrement32( &_pinCnt ) ;
         }
         void        unpin()
         {
            ossFetchAndDecrement32( &_pinCnt ) ;
         }
         BOOLEAN     isPinned() const
         {
            return !ossCompareAndSwap32( (INT32*)&_pinCnt, 0, 0 ) ;
         }
         INT32       getPinkCnt() const
         {
            return ossFetchAndAdd32( (INT32*)&_pinCnt, 0 ) ;
         }
         void        resetPin()
         {
            ossAtomicExchange32( (INT32*)&_pinCnt, 0 ) ;
         }
         void        lock()
         {
            ossFetchAndIncrement32( &_lockCnt ) ;
         }
         void        unlock()
         {
            ossFetchAndDecrement32( &_lockCnt ) ;
         }
         BOOLEAN     isLocked() const
         {
            return !ossCompareAndSwap32( (INT32*)&_lockCnt, 0, 0 ) ;
         }
         INT32       getLockCnt() const
         {
            return ossFetchAndAdd32( (INT32*)&_lockCnt, 0 ) ;
         }
         void        resetLock()
         {
            ossAtomicExchange32( (INT32*)&_lockCnt, 0 ) ;
         }
         INT32       waitToUnlock( INT64 timeout = -1 ) const
         {
            INT64 tmpTimeout = timeout >= 0 ? timeout : OSS_SINT64_MAX ;
            while( isLocked() )
            {
               if ( 0 == tmpTimeout )
               {
                  return SDB_TIMEOUT ;
               }
               ossSleep( 10 ) ;
               tmpTimeout -= 1 ;
            }
            return SDB_OK ;
         }
         void        makeNewestHead()
         {
            OSS_BIT_SET( _status, UTIL_CACHE_PAGE_NEWEST_HEAD_FLAG ) ;
         }
         void        makeNewestTail()
         {
            OSS_BIT_SET( _status, UTIL_CACHE_PAGE_NEWEST_TAIL_FLAG ) ;
         }
         void        makeNewest()
         {
            makeNewestHead() ;
            makeNewestTail() ;
         }
         void        clearNewestHead()
         {
            OSS_BIT_CLEAR( _status, UTIL_CACHE_PAGE_NEWEST_HEAD_FLAG ) ;
         }
         void        clearNewestTail()
         {
            OSS_BIT_CLEAR( _status, UTIL_CACHE_PAGE_NEWEST_TAIL_FLAG ) ;
         }
         void        clearNewest()
         {
            clearNewestHead() ;
            clearNewestTail() ;
         }
         BOOLEAN     isNewestHead() const
         {
            return OSS_BIT_TEST( _status, UTIL_CACHE_PAGE_NEWEST_HEAD_FLAG ) ?
                   TRUE : FALSE ;
         }
         BOOLEAN     isNewestTail() const
         {
            return OSS_BIT_TEST( _status, UTIL_CACHE_PAGE_NEWEST_TAIL_FLAG ) ?
                   TRUE : FALSE ;
         }
         BOOLEAN     isNewest() const
         {
            return isNewestHead() && isNewestTail() ;
         }
         void        addLSN( UINT64 lsn ) ;

         UINT64      beginLSN() const { return _beginLSN ; }
         UINT64      endLSN() const { return _endLSN ; }
         UINT32      lsnNum() const { return _lsnNum ; }

         UINT32      blockNum() const ;
         BOOLEAN     isEmpty() const
         {
            return 0 == blockNum() ? TRUE : FALSE ;
         }
         BOOLEAN     isDirtyEmpty() const
         {
            return 0 == _dirtyLength ? TRUE : FALSE ;
         }

         INT32       write( const CHAR* pBuf, UINT32 offset,
                            UINT32 len, BOOLEAN &setDirty ) ;
         INT32       load( const CHAR* pBuf, UINT32 offset, UINT32 len ) ;
         INT32       loadWithoutData( UINT32 offset, UINT32 len ) ;
         UINT32      read( CHAR* pBuf, UINT32 offset, UINT32 len ) ;
         INT32       copy( const _utilCachePage &right ) ;

         UINT32      beginBlock() const ;
         CHAR*       nextBlock( UINT32 &size, UINT32 &pos ) const ;

         /*
            Used only when the 1 == blockNum()
         */
         const CHAR* str() const ;
         CHAR*       str() ;

      protected:
         INT32       addPage( CHAR* pPage, UINT32 size ) ;
         void        clear() ;

      private:
         INT32       _write( const CHAR* pBuf, UINT32 offset,
                             UINT32 len, BOOLEAN dirty ) ;

      private:
         utilCacheBlock             _first ;
         utilCacheBlockSuit         _next ;
         UINT32                     _start ;
         UINT32                     _length ;
         UINT32                     _dirtyStart ;
         UINT32                     _dirtyLength ;
         UINT64                     _lastTime ;
         UINT64                     _lastWriteTime ;
         UINT32                     _readTimes ;
         UINT32                     _writeTimes ;
         UINT16                     _status ;
         INT32                      _pinCnt ;
         INT32                      _lockCnt ;
         UINT64                     _beginLSN ;
         UINT64                     _endLSN ;
         UINT32                     _lsnNum ;

   } ;
   typedef _utilCachePage utilCachePage ;

   /*
      _utilCacheStat define
   */
   struct _utilCacheStat
   {
      UINT32      _pageSize ;
      UINT64      _totalSize ;
      UINT64      _freeSize ;
      UINT64      _useTimes ;
      UINT64      _allocSize ;
      UINT64      _releaseSize ;
      UINT32      _nullTimes ;

      _utilCacheStat()
      {
         _pageSize = 0 ;
         _totalSize = 0 ;
         _freeSize = 0 ;
         _useTimes = 0 ;
         _allocSize = 0 ;
         _releaseSize = 0 ;
         _nullTimes = 0 ;
      }

      void reset()
      {
         _pageSize = 0 ;
         _totalSize = 0 ;
         _freeSize = 0 ;
         _useTimes = 0 ;
         _allocSize = 0 ;
         _releaseSize = 0 ;
         _nullTimes = 0 ;
      }

      UINT32 getFreeRatio()
      {
         UINT32 ratio = 0 ;

         if ( _totalSize > 0 )
         {
            UINT64 incSize = _allocSize + _nullTimes * _pageSize ;
            if ( incSize > _releaseSize )
            {
               UINT64 diff = incSize - _releaseSize ;
               if ( _freeSize > diff )
               {
                  ratio = ( _freeSize - diff ) * 100 / _totalSize ;
               }
            }
            else
            {
               ratio = _freeSize * 100 / _totalSize ;
            }
         }

         _allocSize = 0 ;
         _releaseSize = 0 ;

         return ratio ;
      }
   } ;
   typedef _utilCacheStat utilCacheStat ;

   #define UTIL_BLOCK_RECYCLE_FREE_RATIO              ( 60 )   /// >=60%
   #define UTIL_BLOCK_RECYCLE_MIN_TIMEOUT             ( 3000 )
   #define UTIL_BLOCK_RECYCLE_MAX_TIMEOUT             ( 15000 )

   #define UTIL_STAT_WINDOW_SIZE                      ( 10 )

   /*
      _utilCacheMgr define
   */
   class _utilCacheMgr : public SDBObject
   {
      typedef ossSpinXLatch                     blkLatch ;

      public:
         _utilCacheMgr() ;
         virtual ~_utilCacheMgr() ;

         /*
            cacheSize unit MB
         */
         virtual INT32     init( UINT64 cacheSize = 0 ) ;
         virtual void      fini() ;

         UINT64   maxCacheSize() const { return _maxCacheSize ; }
         UINT64   totalSize() { return _totalSize.fetch() ; }
         UINT64   freeSize() { return _freeSize.fetch() ; }
         UINT64   totalUseTimes() { return _totalUseTimes.fetch() ; }
         UINT32   totalNullTimes() { return _nullTimes.fetch() ; }
         UINT32   nonEmptyBucketNum() { return _nonEmptySlotNum.fetch() ; }

         UINT32   avgNullTimes() ;

         /// Get the detail info of the specified bucket
         void     getCacheStat( UINT32 bucketID,
                                utilCacheStat &stat ) const ;

         /*
            CachePage alloc/release
         */
         INT32    alloc( UINT32 size, utilCachePage &item ) ;
         INT32    allocWholePage( UINT32 size, utilCachePage &item,
                                  BOOLEAN keepData = FALSE ) ;

         INT32    alloc( UINT32 size, utilCachePage &item,
                         BOOLEAN wholePage,
                         BOOLEAN keepData = FALSE ) ;
         void     release( utilCachePage &item ) ;

         /*
            Block alloc/release
         */
         CHAR*    allocBlock( UINT32 size, UINT32 &blockSize ) ;
         /*
            When return NULL, the orignal pBlock will be released
         */
         CHAR*    reallocBlock( UINT32 size, CHAR *pBlock, UINT32 &blockSize ) ;
         void     releaseBlock( CHAR *&pBlock, UINT32 &blockSize ) ;

         INT32    waitEvent( INT64 millisec = OSS_ONE_SEC ) ;
         void     resetEvent() ;

         UINT64   recycleBlocks() ;
         BOOLEAN  canRecycle() ;

         /*
            CacheUnit Management Functions
         */
         virtual void   registerUnit( _utilCacheUnit *pUnit ) ;
         virtual void   unregUnit( _utilCacheUnit *pUnit ) ;

      protected:
         UINT32   _roundUp2PageSizeSqrt( UINT32 size ) const
         {
            if ( size <= UTIL_PAGE_SLOT_BEGIN_SIZE )
            {
               return _beginPageSizeSqrt ;
            }
            UINT32 tmpSize = ( size - 1 ) >> _beginPageSizeSqrt ;
            UINT32 tmpSqrt = 0 ;
            while( tmpSize != 0 )
            {
               tmpSize = tmpSize >> 1 ;
               ++tmpSqrt ;
            }
            return tmpSqrt + _beginPageSizeSqrt ;
         }

         INT32    _roundDown2PageSizeSqrt( UINT32 size ) const
         {
            if ( size < UTIL_PAGE_SLOT_BEGIN_SIZE )
            {
               return _beginPageSizeSqrt - 1 ;
            }
            UINT32 tmpSize = size >> _beginPageSizeSqrt ;
            UINT32 tmpSqrt = 0 ;
            while( tmpSize != 0 )
            {
               tmpSize = tmpSize >> 1 ;
               ++tmpSqrt ;
            }
            return tmpSqrt - 1 + _beginPageSizeSqrt ;
         }

         UINT32   _sizeUp2Slot( UINT32 size ) const
         {
            return _roundUp2PageSizeSqrt( size ) - _beginPageSizeSqrt ;
         }

         INT32    _sizeDown2Slot( UINT32 size ) const
         {
            return _roundDown2PageSizeSqrt( size ) - _beginPageSizeSqrt ;
         }

         UINT32   _sizeUp2PageSize( UINT32 size ) const
         {
            return (UINT32)1 << _roundUp2PageSizeSqrt( size ) ;
         }

         UINT32   _slot2PageSize( UINT32 slot ) const
         {
            return (UINT32)1 << ( slot + _beginPageSizeSqrt ) ;
         }

         INT32    _allocMem( UINT32 size, UINT32 pageNum = 1 ) ;

      private:
         utilCacheStat&       _getBucketCache( UINT32 id )
         {
            SDB_ASSERT( id < UTIL_PAGE_SLOT_SIZE, "Invalid id" ) ;
            return (*_pStat)[ id ] ;
         }
         UINT64               _recycleBucket( vector< CHAR* > &slotItem,
                                              utilCacheStat *pStat,
                                              vector< CHAR* > &freeItem ) ;

         UINT32               _getFreeRatio( UINT64 currentTime ) ;

      protected:
         UINT32               _beginPageSizeSqrt ;
         UINT64               _maxCacheSize ;
         ossAtomic64          _freeSize ;
         ossAtomic64          _totalSize ;
         ossAtomic64          _totalUseTimes ;
         ossAtomic32          _nonEmptySlotNum ;

         vector< CHAR* >      _slot[ UTIL_PAGE_SLOT_SIZE ] ;
         vector< blkLatch* >  _latch ;
         vector< utilCacheStat >* _pStat ;
         ossEvent             _releaseEvent ;

         UINT64               _lastRecycleTime ;

      private:
         UINT32               _statFreeRatio[ UTIL_STAT_WINDOW_SIZE ] ;
         UINT32               _statIndex ;
         UINT64               _lastStatTime ;

         ossAtomic64          _allocSize ;
         ossAtomic64          _releaseSize ;
         ossAtomic32          _nullTimes ;

   } ;
   typedef _utilCacheMgr utilCacheMgr ;

   /*
      _utilCacheBucket
   */
   class _utilCacheBucket : public SDBObject
   {
      public:
         typedef ossPoolMap< INT32, utilCachePage >      MAP_BLK_PAGE ;

         _utilCacheBucket( UINT32 blkID ) ;
         ~_utilCacheBucket() ;

         UINT32            getID() const { return _blkID ; }

         /*
            Caller must hold the lock
         */
         utilCachePage*    getPage( INT32 pageID ) ;
         /*
            Caller must hold the lock
         */
         utilCachePage*    addPage( INT32 pageID,
                                    const utilCachePage& page ) ;
         /*
            Caller must hold the lock
         */
         void              delPage( INT32 pageID ) ;

         /*
            Caller must hold the write lock
            1. find a un-dirty and un-lock page
            2. assign to the new pageID
            3. make sure the page size >= size
            4. if not found, return NULL
         */
         utilCachePage*    allocPage( INT32 pageID, UINT32 size ) ;

         void              incDirty() { ++_dirtyPages ; }
         void              decDirty() { --_dirtyPages ; }
         UINT64            dirtyPages() const { return _dirtyPages ; }
         UINT64            totalPages() const { return _pages.size() ; }

         MAP_BLK_PAGE*     getPages() { return &_pages ; }

         INT32             lock( OSS_LATCH_MODE mode,
                                 INT32 millisec = -1 ) ;
         void              unlock( OSS_LATCH_MODE mode ) ;

      private:
         MAP_BLK_PAGE               _pages ;
         UINT32                     _blkID ;
         ossRWMutex                 _rwMutex ;
         UINT64                     _dirtyPages ;

   } ;
   typedef _utilCacheBucket utilCacheBucket ;

   /*
      _utilCacheContext define
   */
   class _utilCacheContext : public SDBObject
   {
      friend class _utilCacheUnit ;

      public:
         _utilCacheContext() ;
         ~_utilCacheContext() ;

         BOOLEAN isValid() const ;
         BOOLEAN isPageValid() const ;
         BOOLEAN isLocked() const ;
         BOOLEAN isLockRead() const ;
         BOOLEAN isLockWrite() const ;
         void    unLock() ;
         BOOLEAN isDone() const ;
         BOOLEAN isUsePage() const { return _usePage ; }

         BOOLEAN  isInCache( UINT32 offset, UINT32 len ) const ;
         void     discardPage( UINT32 &dirtyStart, UINT32 &dirtyLen,
                               UINT64 &beginLSN, UINT64 &endLSN ) ;
         void     restorePage( UINT32 dirtyStart, UINT32 dirtyLen,
                               UINT64 beginLSN, UINT64 endLSN ) ;

         void     makeNewest( UINT32 newestMask = UTIL_WRITE_NEWEST_BOTH ) ;
         void     clearNewest() ;

         /*
            Need call submit or rollback to done
         */
         INT32    write( const CHAR *pData,
                         UINT32 offset,
                         UINT32 len,
                         IExecutor *cb,
                         UINT32 newestMask ) ;

         /*
            Need call submit or rollback to done
         */
         INT32    read( CHAR *pBuff,
                        UINT32 offset,
                        UINT32 len,
                        IExecutor *cb ) ;

         /*
            Need call submit or rollback to done
         */
         INT32    readAndCache( CHAR *pBuff,
                                UINT32 offset,
                                UINT32 len,
                                IExecutor *cb ) ;

         UINT32   submit( IExecutor *cb ) ;

         void     rollback() ;

         void     release() ;

      protected:
         INT32    _loadPage( UINT32 offset,
                             UINT32 len,
                             IExecutor *cb ) ;

      private:
         CHAR*             _pData ;
         UINT32            _offset ;
         UINT32            _len ;
         BOOLEAN           _isWrite ;
         UINT32            _newestMask ;
         BOOLEAN           _writeBack ;
         BOOLEAN           _usePage ;
         BOOLEAN           _hasDiscard ;

         /// should value in cache unit
         INT32             _pageID ;
         utilCachePage*    _pPage ;
         utilCacheBucket*  _pBucket ;
         _utilCacheUnit*   _pUnit ;
         INT32             _mode ;
         UINT32            _size ;

   } ;
   typedef _utilCacheContext utilCacheContext ;

   /*
      _utilCachFileBase define
   */
   class _utilCachFileBase : public SDBObject
   {
      public:
         _utilCachFileBase() {}
         virtual ~_utilCachFileBase() {}

         virtual const CHAR*     getFileName() const = 0 ;

      public:
         virtual INT32  prepareWrite( INT32 pageID,
                                      const CHAR *pData,
                                      UINT32 len,
                                      UINT32 offset,
                                      IExecutor *cb ) = 0 ;

         virtual INT32  write( INT32 pageID, const CHAR *pData,
                               UINT32 len, UINT32 offset,
                               UINT32 newestMask,
                               IExecutor *cb ) = 0 ;

         virtual INT32  prepareRead( INT32 pageID,
                                     CHAR *pData,
                                     UINT32 len,
                                     UINT32 offset,
                                     IExecutor *cb ) = 0 ;

         virtual INT32  read( INT32 pageID, CHAR *pData,
                              UINT32 len, UINT32 offset,
                              UINT32 &readLen,
                              IExecutor *cb ) = 0 ;

         virtual INT64  pageID2Offset( INT32 pageID,
                                       UINT32 pageOffset = 0 ) const = 0 ;

         virtual INT32  offset2PageID( INT64 offset,
                                       UINT32 *pageOffset = NULL ) const = 0 ;

         virtual INT32  writeRaw( INT64 offset, const CHAR *pData,
                                  UINT32 len, IExecutor *cb,
                                  BOOLEAN isAligned,
                                  UINT32 newestMask =
                                  UTIL_WRITE_NEWEST_BOTH ) = 0 ;

         virtual INT32  readRaw( INT64 offset, UINT32 len,
                                 CHAR *buf, UINT32 &readLen,
                                 IExecutor *cb,
                                 BOOLEAN isAligned ) = 0 ;

   } ;
   typedef _utilCachFileBase utilCachFileBase ;

   /*
      _utilCacheMerge define
   */
   class _utilCacheMerge : public SDBObject
   {
      typedef vector< utilCachePage* >    VEC_PAGE_PTR ;

      public:
         _utilCacheMerge() ;
         ~_utilCacheMerge() ;

         INT32       init( UINT32 pageSize,
                           utilCachFileBase *pFile,
                           BOOLEAN alignment,
                           UINT32 cacheSize ) ;
         void        fini() ;

         BOOLEAN     isEmpty() const ;
         BOOLEAN     isFull() const ;
         UINT32      freeSize() const ;
         UINT32      capacity() const ;
         BOOLEAN     isAlignment() const ;

         UINT32      getLength() const ;
         const CHAR* getData() const ;

         INT32       write( INT32 pageID, utilCachePage *pPage,
                            BOOLEAN hasPin ) ;
         INT32       sync( IExecutor *cb ) ;

         UINT32      getPageNum() const { return _pageNum ; }
         INT32       getFirstPageID() const { return _firstPageID ; }
         INT32       getLastPageID() const { return _lastPageID ; }

      protected:
         void        _releasePages() ;

      private:
         CHAR                    *_pCache ;
         UINT32                  _cacheSize ;
         UINT32                  _dataLength ;
         INT32                   _firstPageID ;
         INT32                   _lastPageID ;
         UINT32                  _pageNum ;

         UINT32                  _pageSize ;
         utilCachFileBase        *_pFile ;
         BOOLEAN                 _isAlignment ;

         VEC_PAGE_PTR            _vecPages ;
   } ;
   typedef _utilCacheMerge utilCacheMerge ;

   #define UTIL_CACHEUNIT_BUCKET_SZ                ( 2048 )
   #define UTIL_CACHEUNIT_PAGE_TIMEOUT             ( 2000 ) /// ms
   #define UTIL_CACHEUNIT_DIRTY_TIMEOUT            ( 3000 ) /// ms
   #define UTIL_CACHEUNIT_BG_DIRTY_RATIO           ( 40 )   /// >=40%
   #define UTIL_CACHEUNIT_BG_FREE_RATIO            ( 30 )   /// <=30%

   /*
      _utilCacheUnit define
   */
   class _utilCacheUnit : public SDBObject
   {
      typedef ossPoolMap< INT32, utilCachePage* >     MAP_ID_2_PAGE_PRT ;
      typedef MAP_ID_2_PAGE_PRT::iterator             MAP_ID_2_PAGE_PRT_IT ;
      typedef MAP_ID_2_PAGE_PRT::const_iterator       MAP_ID_2_PAGE_PRT_CIT ;

      public:
         _utilCacheUnit( utilCacheMgr *pMgr ) ;
         ~_utilCacheUnit() ;

         /*
            Stat dump
         */
         void           dumpStatInfo() ;

         /*
            Return the recycle total size
         */
         UINT64         recyclePages( BOOLEAN force, INT64 exceptSize = -1 ) ;
         BOOLEAN        canRecycle( BOOLEAN &force ) ;
         /*
            Return the sync pages
         */
         UINT32         syncPages( IExecutor *cb,
                                   BOOLEAN force = TRUE,
                                   BOOLEAN ignoreClose = FALSE ) ;
         BOOLEAN        canSync( BOOLEAN &force ) ;

         UINT32         dropDirty() ;

         void           lockPageCleaner() ;
         void           unlockPageCleaner() ;

         INT32          init( utilCachFileBase *pFile,
                              UINT32 pageSize,
                              UINT32 allocTimeout,
                              UINT32 bucketSize = UTIL_CACHEUNIT_BUCKET_SZ,
                              BOOLEAN useCache = TRUE,
                              BOOLEAN wholePage = FALSE ) ;
         void           fini( IExecutor *cb ) ;

         INT32          enableMerge( BOOLEAN alignment,
                                     UINT32 cacheSize ) ;

         void           updateMerge( BOOLEAN alignment,
                                     UINT32 cacheSize ) ;

         BOOLEAN        isClosed() const { return _closed ; }

         /*
            Set and Get dirty/recycle configs
         */
         void           setDirtyConfig( UINT32 bgDirtyRatio =
                                        UTIL_CACHEUNIT_BG_DIRTY_RATIO,
                                        UINT32 dirtyTimeout =
                                        UTIL_CACHEUNIT_DIRTY_TIMEOUT ) ;
         void           setRecycleConfig( UINT32 bgFreeRatio =
                                          UTIL_CACHEUNIT_BG_FREE_RATIO,
                                          UINT32 pageTimeout =
                                          UTIL_CACHEUNIT_PAGE_TIMEOUT ) ;
         void           setUseCache( BOOLEAN useCache ) ;
         void           setUseWholePage( BOOLEAN wholePage ) ;
         void           setAllocTimeout( UINT32 allocTimeout ) ;

         UINT32         getBGDirtyRatio() const { return _bgDirtyRatio ; }
         UINT32         getDirtyTimeout() const { return _dirtyTimeout ; }
         UINT32         getBGFreeRatio() const { return _bgFreeRatio ; }
         UINT32         getPageTimeout() const { return _pageTimeout ; }

         UINT32         calcBucketID( INT32 pageID ) const ;
         UINT32         bucketSize() const { return _bucketSize ; }

         utilCacheBucket*     getBucket( UINT32 index ) ;
         utilCachFileBase*    getCacheFile() { return _pCacheFile ; }
         utilCacheMgr*        getCacheMgr() { return _pMgr ; }

         UINT64         totalPages() ;
         UINT64         dirtyPages() ;
         UINT64         undirtyPages() ;
         BOOLEAN        isUseCache() const { return _useCache ; }

         void           decDirtyPages( utilCacheBucket *pBucket ) ;
         void           incDirtyPages( utilCacheBucket *pBucket ) ;

         void           prepareWrite( INT32 pageID,
                                      UINT32 offset,
                                      UINT32 len,
                                      IExecutor *cb,
                                      utilCacheContext &context ) ;

         void           prepareRead( INT32 pageID,
                                     UINT32 offset,
                                     UINT32 len,
                                     IExecutor *cb,
                                     utilCacheContext &context ) ;

      protected:
         utilCachePage* getAndLock( INT32 pageID, UINT32 size,
                                    utilCacheBucket **ppBucket,
                                    OSS_LATCH_MODE mode,
                                    BOOLEAN alloc,
                                    IExecutor *cb ) ;

         /*
            Caller must hold the bucket lock
         */
         INT32          _syncPage( utilCacheBucket *pBucket,
                                   BOOLEAN hasLock,
                                   utilCachePage *pPage,
                                   INT32 pageID,
                                   IExecutor *cb,
                                   BOOLEAN hasPin,
                                   BOOLEAN *pSync = NULL,
                                   BOOLEAN writeMerge = FALSE ) ;

         UINT32         _syncPages( const MAP_ID_2_PAGE_PRT &pageMap,
                                    IExecutor *cb,
                                    BOOLEAN hasPin ) ;

         void           _unpinPages( const MAP_ID_2_PAGE_PRT &pageMap ) ;

         /*
            Caller must hold the bucket EXCLUSIVE lock
         */
         utilCachePage* _allocFromBucket( utilCacheBucket *pBucket,
                                          UINT32 size,
                                          INT32 pageID,
                                          utilCachePage *pItem,
                                          BOOLEAN keepData ) ;

         /*
            For stat
         */
         OSS_INLINE void _incAllocNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incAllocNullNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incAllocFromBlkNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incHitCacheNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incMergeNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incMergeSyncNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incSyncNum( UINT32 num = 1 ) ;
         OSS_INLINE void _incRecycleNum( UINT32 num = 1 ) ;

      private:
         utilCacheMgr*              _pMgr ;
         utilCachFileBase*          _pCacheFile ;
         UINT32                     _bucketSize ;
         UINT32                     _pageSize ;
         UINT32                     _allocTimeout ;
         BOOLEAN                    _wholePage ;
         vector< utilCacheBucket* > _vecBucket ;
         BOOLEAN                    _closed ;
         BOOLEAN                    _useCache ;
         BOOLEAN                    _hasReg ;

         /*
            Dirty page configs
         */
         UINT32                     _bgDirtyRatio ;   /// background dirty ratio
         UINT32                     _dirtyTimeout ;   /// ms

         /*
            Page recycle config
         */
         UINT32                     _bgFreeRatio ;    /// background free ratio
         UINT32                     _pageTimeout ;    /// ms

         ossAtomic64                _dirtySize ;
         ossAtomic64                _totalPage ;

         UINT64                     _lastRecycleTime ;
         UINT64                     _lastSyncTime ;
         ossSpinXLatch              _pageCleaner ;

         utilCacheMerge             _cacheMerge ;

         ossAtomic32                _statAllocNum ;
         ossAtomic32                _statAllocFromBlkNum ;
         ossAtomic32                _statAllocNullNum ;
         ossAtomic32                _statAllocWaitNum ;
         ossAtomic32                _statHitCacheNum ;
         volatile UINT32            _statMergeNum ;
         volatile UINT32            _statMergeSyncNum ;
         ossAtomic32                _statSyncNum ;
         ossAtomic32                _statRecycleNum ;
         ossAtomic64                _statTotalWaitTime ;
         ossAtomic32                _statMaxWaitTime ;

         volatile UINT64            _lastStatTime ;

   } ;
   typedef _utilCacheUnit utilCacheUnit ;

   OSS_INLINE void _utilCacheUnit::_incAllocNum( UINT32 num )
   {
      _statAllocNum.add( num ) ;
   }
   OSS_INLINE void _utilCacheUnit::_incAllocNullNum( UINT32 num )
   {
      _statAllocNullNum.add( num ) ;
   }
   OSS_INLINE void _utilCacheUnit::_incHitCacheNum( UINT32 num )
   {
      _statHitCacheNum.add( num ) ;
   }
   OSS_INLINE void _utilCacheUnit::_incMergeNum( UINT32 num )
   {
      _statMergeNum += num ;
   }
   OSS_INLINE void _utilCacheUnit::_incMergeSyncNum( UINT32 num )
   {
      _statMergeSyncNum += num ;
   }
   OSS_INLINE void _utilCacheUnit::_incAllocFromBlkNum( UINT32 num )
   {
      _statAllocFromBlkNum.add( num ) ;
   }
   OSS_INLINE void _utilCacheUnit::_incSyncNum( UINT32 num )
   {
      _statSyncNum.add( num ) ;
   }
   OSS_INLINE void _utilCacheUnit::_incRecycleNum( UINT32 num )
   {
      _statRecycleNum.add( num ) ;
   }

}

#endif // UTIL_CACHE_HPP_

