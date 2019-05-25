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

   Source File Name = utilCache.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilCache.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

namespace engine
{

   #define UTIL_MAX_EXCEED_SLOT_SIZE            ( 3 )
   #define UTIL_MIN_EXCEED_SLOT_SIZE            ( 5 )

   #define UTIL_MAX_BLK_RECYCLE_SIZE            ( 40 * 1048576 )     /// MB
   #define UTIL_DFT_BLK_RECYCLE_SIZE            ( 4  * 1048576 )     /// MB

   #define UTIL_PAGE_ALLOC_WAIT_TIMEPIECE       ( 30 )               /// ms
   #define UTIL_PAGE_ALLOC_FIX_TIMEPIECE        ( 3 )                /// ms

   /*
      _utilCachePage implement
   */
   _utilCachePage::_utilCachePage()
   {
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;
      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;
      _status = 0 ;
      _pinCnt = 0 ;
      _lockCnt = 0 ;
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   _utilCachePage::_utilCachePage( const _utilCachePage& right )
   {
      _start = right._start ;
      _length = right._length ;
      _dirtyStart = right._dirtyStart ;
      _dirtyLength = right._dirtyLength ;
      _lastTime = right._lastTime ;
      _lastWriteTime = right._lastWriteTime ;
      _readTimes = right._readTimes ;
      _writeTimes = right._writeTimes ;
      _status = right._status ;
      _pinCnt = right._pinCnt ;
      _lockCnt = right._lockCnt ;
      _beginLSN = right._beginLSN ;
      _endLSN = right._endLSN ;
      _lsnNum = right._lsnNum ;

      _first._size = 0 ;
      _first._pBuff = NULL ;

      UINT32 pos = right.beginBlock() ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      while( NULL != ( pPage = right.nextBlock( pageSize, pos ) ) )
      {
         addPage( pPage, pageSize ) ;
      }
   }

   _utilCachePage::~_utilCachePage()
   {
      clear() ;
   }

   void _utilCachePage::clearDataInfo()
   {
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;

      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;

      clearLSNInfo() ;
   }

   void _utilCachePage::clearLSNInfo()
   {
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   BOOLEAN _utilCachePage::isDataEmpty() const
   {
      return 0 == _length ? TRUE : FALSE ;
   }

   void _utilCachePage::clear()
   {
      _next.clear() ;
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;
      _first._size = 0 ;
      _first._pBuff = NULL ;
      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;
      _status = 0 ;
      _pinCnt = 0 ;
      _lockCnt = 0 ;
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   UINT32 _utilCachePage::size() const
   {
      UINT32 totalSize = 0 ;
      UINT32 blockSize = 0 ;
      UINT32 pos = beginBlock() ;
      while( NULL != nextBlock( blockSize, pos ) )
      {
         totalSize += blockSize ;
      }
      return totalSize ;
   }

   _utilCachePage& _utilCachePage::operator= ( const _utilCachePage& rhs )
   {
      clear() ;

      _start = rhs._start ;
      _length = rhs._length ;
      _dirtyStart = rhs._dirtyStart ;
      _dirtyLength = rhs._dirtyLength ;
      _lastTime = rhs._lastTime ;
      _lastWriteTime = rhs._lastWriteTime ;
      _readTimes = rhs._readTimes ;
      _writeTimes = rhs._writeTimes ;
      _status = rhs._status ;
      _pinCnt = rhs._pinCnt ;
      _lockCnt = rhs._lockCnt ;
      _beginLSN = rhs._beginLSN ;
      _endLSN = rhs._endLSN ;
      _lsnNum = rhs._lsnNum ;

      UINT32 pos = rhs.beginBlock() ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      while( NULL != ( pPage = rhs.nextBlock( pageSize, pos ) ) )
      {
         addPage( pPage, pageSize ) ;
      }
      return *this ;
   }

   UINT32 _utilCachePage::blockNum() const
   {
      if ( _first.empty() )
      {
         return 0 ;
      }
      return 1 + _next.size() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEPAGE__WRITE, "_utilCachePage::_write" )
   INT32 _utilCachePage::_write( const CHAR *pBuf, UINT32 offset,
                                 UINT32 len, BOOLEAN dirty )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEPAGE__WRITE ) ;

      ossTimestamp t ;
      UINT32 pos = 0 ;
      CHAR *pPage = NULL ;
      UINT32 blockSize = 0 ;
      UINT32 lastOffset = offset ;
      UINT32 lastLen = len ;
      UINT32 onceWrite = 0 ;

      if ( size() < offset + len )
      {
         rc = SDB_NOSPC ;
         goto error ;
      }

      pos = beginBlock() ;
      while ( lastLen > 0 && NULL != ( pPage = nextBlock( blockSize, pos ) ) )
      {
         if ( blockSize <= lastOffset )
         {
            lastOffset -= blockSize ;
            continue ;
         }
         else if ( lastOffset > 0 )
         {
            pPage += lastOffset ;
            blockSize -= lastOffset ;
            lastOffset = 0 ;
         }
         onceWrite = lastLen < blockSize ? lastLen : blockSize ;
         ossMemcpy( pPage, pBuf, onceWrite ) ;
         lastLen -= onceWrite ;
         pBuf += onceWrite ;
      }

      SDB_ASSERT( lastLen == 0, "Last len must be 0" ) ;

      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;
      if ( dirty )
      {
         ++_writeTimes ;
         _lastWriteTime = _lastTime ;

         if ( 0 == _dirtyLength || offset < _dirtyStart )
         {
            _dirtyStart = offset ;
         }
         if ( offset + len > _dirtyLength )
         {
            _dirtyLength = offset + len ;
         }
      }

      if ( 0 == _length || offset < _start )
      {
         _start = offset ;
      }
      if ( offset + len > _length )
      {
         _length = offset + len ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEPAGE__WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCachePage::write( const CHAR *pBuf, UINT32 offset,
                                UINT32 len, BOOLEAN &setDirty )
   {
      setDirty = FALSE ;
      INT32 rc = _write( pBuf, offset, len, TRUE ) ;
      if ( SDB_OK == rc && !isDirty() )
      {
         makeDirty() ;
         setDirty = TRUE ;
      }
      return rc ;
   }

   INT32 _utilCachePage::load( const CHAR *pBuf, UINT32 offset, UINT32 len )
   {
      return _write( pBuf, offset, len, FALSE ) ;
   }

   INT32 _utilCachePage::loadWithoutData( UINT32 offset, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      ossTimestamp t ;

      if ( size() < offset + len )
      {
         rc = SDB_NOSPC ;
         goto error ;
      }

      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;

      if ( (UINT32)~0 == _length || offset < _start )
      {
         _start = offset ;
      }
      if ( offset + len > _length )
      {
         _length = offset + len ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEPAGE_READ, "_utilCachePage::read" )
   UINT32 _utilCachePage::read( CHAR *pBuf, UINT32 offset, UINT32 len )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEPAGE_READ ) ;
      ossTimestamp t ;
      UINT32 hasRead = 0 ;
      CHAR *pPage = NULL ;
      UINT32 blockSize = 0 ;
      UINT32 onceRead = 0 ;
      UINT32 lastOffset = offset ;
      UINT32 lastRead = len ;
      UINT32 pos = 0 ;

      pos = beginBlock() ;
      while ( lastRead > 0 && NULL != ( pPage = nextBlock( blockSize, pos ) ) )
      {
         if ( blockSize <= lastOffset )
         {
            lastOffset -= blockSize ;
            continue ;
         }
         else if ( lastOffset > 0 )
         {
            pPage += lastOffset ;
            blockSize -= lastOffset ;
            lastOffset = 0 ;
         }
         onceRead = lastRead < blockSize ? lastRead : blockSize ;
         ossMemcpy( pBuf + hasRead, pPage, onceRead ) ;
         lastRead -= onceRead ;
         hasRead += onceRead ;
      }

      ++_readTimes ;
      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;

      PD_TRACE1( SDB__UTILCACHEPAGE_READ, PD_PACK_UINT( hasRead ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEPAGE_READ ) ;
      return hasRead ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEPAGE_COPY, "_utilCachePage::copy" )
   INT32 _utilCachePage::copy( const _utilCachePage &right )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEPAGE_COPY ) ;
      INT32 rc = SDB_OK ;
      CHAR *pData = NULL ;
      UINT32 blockSz = 0 ;
      UINT32 offset = 0 ;

      UINT32 rightPos = right.beginBlock() ;
      while( NULL != ( pData = right.nextBlock( blockSz, rightPos ) ) )
      {
         rc = load( pData, offset, blockSz ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Copy data failed, rc: %d", rc ) ;
            goto error ;
         }
         offset += blockSz ;
      }

      _start = right._start ;
      _length = right._length ;
      _dirtyStart = right._dirtyStart ;
      _dirtyLength = right._dirtyLength ;
      _lastTime = right._lastTime ;
      _lastWriteTime = right._lastWriteTime ;
      _readTimes = right._readTimes ;
      _writeTimes = right._writeTimes ;
      _status = right._status ;
      _pinCnt = right._pinCnt ;
      _lockCnt = right._lockCnt ;
      _beginLSN = right._beginLSN ;
      _endLSN = right._endLSN ;
      _lsnNum = right._lsnNum ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEPAGE_COPY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCachePage::addPage( CHAR *pPage, UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( !pPage || 0 == size )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _first.empty() )
      {
         _first._pBuff = pPage ;
         _first._size = size ;
      }
      else
      {
         _next.push_back( utilCacheBlock( pPage, size ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _utilCachePage::str() const
   {
      SDB_ASSERT( 1 == blockNum(), "Block number must be 1" ) ;
      return _first._pBuff ;
   }

   CHAR* _utilCachePage::str()
   {
      SDB_ASSERT( 1 == blockNum(), "Block number must be 1" ) ;
      return _first._pBuff ;
   }

   UINT32 _utilCachePage::beginBlock() const
   {
      return 0 ;
   }

   CHAR* _utilCachePage::nextBlock( UINT32 &size, UINT32 &pos ) const
   {
      UINT32 tmpPos = pos ;
      ++pos ;

      if ( 0 == tmpPos )
      {
         if ( _first.empty() )
         {
            return NULL ;
         }
         size = _first._size ;
         return _first._pBuff ;
      }
      else if ( _next.size() >= tmpPos )
      {
         const utilCacheBlock& item = _next[ tmpPos - 1 ] ;
         size = item._size ;
         return item._pBuff ;
      }

      return NULL ;
   }

   void _utilCachePage::addLSN( UINT64 lsn )
   {
      if ( (UINT64)~0 == _beginLSN )
      {
         _beginLSN = lsn ;
         _endLSN = lsn ;
      }
      else
      {
         _endLSN = lsn ;
      }
      ++_lsnNum ;
   }

   #define UTIL_STAT_RATIO_INTERVAL                ( 1000 )       // ms

   /*
      _utilCacheMgr implement
   */
   _utilCacheMgr::_utilCacheMgr()
   :_freeSize( 0 ), _totalSize( 0 ), _totalUseTimes( 0 ), _nonEmptySlotNum( 0 ),
    _allocSize( 0 ), _releaseSize( 0 ), _nullTimes( 0 )
   {
      _beginPageSizeSqrt = 0 ;
      _maxCacheSize = 0 ;
      _pStat = NULL ;
      _lastRecycleTime = 0 ;

      for ( UINT32 i = 0 ; i < UTIL_STAT_WINDOW_SIZE ; ++i )
      {
         _statFreeRatio[ i ] = (UINT32)~0 ;
      }
      _statIndex = 0 ;
      _lastStatTime = 0 ;
   }

   _utilCacheMgr::~_utilCacheMgr()
   {
   }

   INT32 _utilCacheMgr::init( UINT64 cacheSize )
   {
      INT32 rc = SDB_OK ;
      blkLatch* pLatch = NULL ;
      _maxCacheSize = cacheSize * 1024 * 1024 ;

      if ( !ossIsPowerOf2( UTIL_PAGE_SLOT_BEGIN_SIZE, &_beginPageSizeSqrt ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         pLatch = SDB_OSS_NEW blkLatch() ;
         if ( pLatch )
         {
            _latch.push_back( pLatch ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alloc bucket latch" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      _pStat = new (std::nothrow) vector< utilCacheStat >( UTIL_PAGE_SLOT_SIZE ) ;
      if ( !_pStat )
      {
         PD_LOG( PDERROR, "Failed to alloc stat vector" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         _getBucketCache( i )._pageSize = _slot2PageSize( i ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheMgr::fini()
   {
      SDB_ASSERT( _totalSize.peek() == _freeSize.peek(),
                  "Total size must be equal with the free size" ) ;

      UINT32 size = 0 ;
      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         size = _slot[ i ].size() ;
         for ( UINT32 j = 0 ; j < size ; ++j )
         {
            SDB_OSS_FREE( _slot[ i ][ j ] ) ;
         }
         _slot[ i ].clear() ;
      }

      for ( UINT32 i = 0 ; i < _latch.size() ; ++i )
      {
         SDB_OSS_DEL _latch[ i ] ;
      }

      if ( _pStat )
      {
         delete _pStat ;
         _pStat = NULL ;
      }
      _latch.clear() ;
   }

   UINT32 _utilCacheMgr::avgNullTimes()
   {
      UINT32 nonEmptyBucket = _nonEmptySlotNum.fetch() ;
      if ( nonEmptyBucket > 1 )
      {
         return _nullTimes.fetch() / nonEmptyBucket ;
      }
      return _nullTimes.fetch() ;
   }

   void _utilCacheMgr::resetEvent()
   {
      _releaseEvent.reset() ;
   }

   INT32 _utilCacheMgr::waitEvent( INT64 millisec )
   {
      return _releaseEvent.wait( millisec ) ;
   }

   void _utilCacheMgr::getCacheStat( UINT32 bucketID,
                                     utilCacheStat &stat ) const
   {
      if ( _pStat && bucketID < UTIL_PAGE_SLOT_SIZE )
      {
         _latch[ bucketID ]->get() ;
         stat = (*_pStat)[ bucketID ] ;
         _latch[ bucketID ]->release() ;
      }
      else
      {
         stat.reset() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_ALLOC, "_utilCacheMgr::alloc" )
   INT32 _utilCacheMgr::alloc( UINT32 size, utilCachePage &item )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_ALLOC ) ;
      PD_TRACE1( SDB__UTILCACHEMGR_ALLOC, PD_PACK_UINT( size ) ) ;

      UINT32 beginSlot = 0 ;
      UINT32 pageSize = 0 ;
      UINT32 extendNum = 0 ;
      UINT32 lastSize = 0 ;
      UINT32 exceedSlot = 0 ;
      blkLatch *pLatch = NULL ;

      if ( item.size() >= size )
      {
         return rc ;
      }
      lastSize = size - item.size() ;

   retry:
      beginSlot = _sizeUp2Slot( lastSize ) ;
      exceedSlot = 0 ;

      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= lastSize )
      {
         pLatch = _latch[ beginSlot ] ;
         pLatch->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            item.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            _allocSize.add( pageSize ) ;
            utilCacheStat &stat = _getBucketCache( beginSlot ) ;
            stat._freeSize -= pageSize ;
            stat._allocSize += pageSize ;
            stat._useTimes += 1 ;
            pLatch->release() ;
            _totalUseTimes.inc() ;
            goto done ;
         }
         pLatch->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      beginSlot = _sizeUp2Slot( lastSize ) ;
      exceedSlot = 0 ;
      if ( beginSlot >= UTIL_PAGE_SLOT_SIZE )
      {
         beginSlot = UTIL_PAGE_SLOT_SIZE - 1 ;
      }
      while ( beginSlot > 0 &&
              exceedSlot < UTIL_MIN_EXCEED_SLOT_SIZE &&
              _freeSize.peek() >= lastSize )
      {
         --beginSlot ;
         ++exceedSlot ;
         pageSize = _slot2PageSize( beginSlot ) ;
         pLatch = _latch[ beginSlot ] ;
         pLatch->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         while( !slotItem.empty() )
         {
            item.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            _allocSize.add( pageSize ) ;

            utilCacheStat &stat = _getBucketCache( beginSlot ) ;
            stat._freeSize -= pageSize ;
            stat._allocSize += pageSize ;
            stat._useTimes += 1 ;
            _totalUseTimes.inc() ;

            if ( lastSize > pageSize )
            {
               lastSize -= pageSize ;
            }
            else
            {
               pLatch->release() ;
               goto done ;
            }
         }
         pLatch->release() ;
      }

      if ( _sizeUp2Slot( lastSize ) >= UTIL_PAGE_SLOT_SIZE )
      {
         pageSize = _slot2PageSize( UTIL_PAGE_SLOT_SIZE - 1 ) ;
         extendNum = ( lastSize + pageSize - 1 ) / pageSize ;
      }
      else
      {
         pageSize = _sizeUp2PageSize( lastSize ) ;
         extendNum = 1 ;
      }
      rc = _allocMem( pageSize, extendNum ) ;
      if ( SDB_OK == rc )
      {
         goto retry ;
      }
      else
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEMGR_ALLOC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_ALLOCWHOLE, "_utilCacheMgr::allocWholePage" )
   INT32 _utilCacheMgr::allocWholePage( UINT32 size, utilCachePage &item,
                                        BOOLEAN keepData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_ALLOCWHOLE ) ;
      PD_TRACE2( SDB__UTILCACHEMGR_ALLOCWHOLE, PD_PACK_UINT( size ),
                 PD_PACK_INT( keepData ) ) ;

      UINT32 beginSlot = 0 ;
      UINT32 pageSize = 0 ;
      UINT32 exceedSlot = 0 ;
      utilCachePage tmpPage ;

      if ( item.size() >= size )
      {
         return rc ;
      }

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            tmpPage.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            _allocSize.add( pageSize ) ;
            utilCacheStat &stat = _getBucketCache( beginSlot ) ;
            stat._freeSize -= pageSize ;
            stat._allocSize += pageSize ;
            stat._useTimes += 1 ;
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done_assign ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      rc = _allocMem( size ) ;
      if ( SDB_OK == rc )
      {
         goto retry ;
      }
      else
      {
         goto error ;
      }

   done_assign:
      if ( item.isPinned() || item.isLocked() ||
           item.isDirty() ||
           ( keepData && !item.isDataEmpty() ) )
      {
         rc = tmpPage.copy( item ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Copy data from source page failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         item.resetLock() ;
         item.resetPin() ;
         item.clearDirty() ;
         item.clearDataInfo() ;
      }
      if ( !item.isEmpty() )
      {
         release( item ) ;
      }
      item = tmpPage ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEMGR_ALLOCWHOLE, rc ) ;
      return rc ;
   error:
      release( tmpPage ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_RELEASE, "_utilCacheMgr::release" )
   void _utilCacheMgr::release( utilCachePage &item )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_RELEASE ) ;
      UINT32 pos = 0 ;
      UINT32 slot = 0 ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      SDB_ASSERT( !item.isDirty(),  "Page can't be dirty" ) ;
      SDB_ASSERT( !item.isLocked(), "Page can't be locked" ) ;
      SDB_ASSERT( !item.isPinned(), "Page can't be pined" ) ;

      pos = item.beginBlock() ;

      while( NULL != ( pPage = item.nextBlock( pageSize, pos ) ) )
      {
         slot = _sizeDown2Slot( pageSize ) ;
         SDB_ASSERT( pageSize == _slot2PageSize( slot ),
                     "PageSize is not invalid" ) ;
         _latch[ slot ]->get() ;
         _slot[ slot ].push_back( pPage ) ;
         utilCacheStat &stat = _getBucketCache( slot ) ;
         UINT32 slotSize = _slot2PageSize( slot ) ;
         stat._freeSize += slotSize ;
         stat._releaseSize += slotSize ;
         _latch[ slot ]->release() ;
         _freeSize.add( slotSize ) ;
         _releaseSize.add( slotSize ) ;
      }
      _releaseEvent.signalAll() ;

      item.clear() ;

      PD_TRACE_EXIT( SDB__UTILCACHEMGR_RELEASE ) ;
   }

   INT32 _utilCacheMgr::alloc( UINT32 size, utilCachePage &item,
                               BOOLEAN wholePage, BOOLEAN keepData )
   {
      if ( !wholePage )
      {
         return alloc( size, item ) ;
      }
      return allocWholePage( size, item, keepData ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_ALLOCBLOCK, "_utilCacheMgr::allocBlock" )
   CHAR* _utilCacheMgr::allocBlock( UINT32 size, UINT32 &blockSize )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_ALLOCBLOCK ) ;
      CHAR *pBlock      = NULL ;
      UINT32 beginSlot  = 0 ;
      UINT32 pageSize   = 0 ;
      UINT32 exceedSlot = 0 ;

      blockSize = 0 ;

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            pBlock = slotItem.back() ;
            blockSize = pageSize ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            _allocSize.add( pageSize ) ;
            utilCacheStat &stat = _getBucketCache( beginSlot ) ;
            stat._freeSize -= pageSize ;
            stat._allocSize += pageSize ;
            stat._useTimes += 1 ;
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      if ( SDB_OK == _allocMem( size ) )
      {
         goto retry ;
      }
      else
      {
         goto done ;
      }

   done:
      PD_TRACE_EXIT( SDB__UTILCACHEMGR_ALLOCBLOCK ) ;
      return pBlock ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_REALLOCBLOCK, "_utilCacheMgr::reallocBlock" )
   CHAR* _utilCacheMgr::reallocBlock( UINT32 size, CHAR *pBlock,
                                      UINT32 &blockSize )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_REALLOCBLOCK ) ;
      CHAR *pTmpBlock   = NULL ;
      UINT32 tmpBlockSize = 0 ;
      UINT32 beginSlot  = 0 ;
      UINT32 pageSize   = 0 ;
      UINT32 exceedSlot = 0 ;

      if ( blockSize >= size )
      {
         goto done ;
      }

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            pTmpBlock = pBlock ;
            tmpBlockSize = blockSize ;
            pBlock = slotItem.back() ;
            blockSize = pageSize ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            _allocSize.add( pageSize ) ;
            utilCacheStat &stat = _getBucketCache( beginSlot ) ;
            stat._freeSize -= pageSize ;
            stat._allocSize += pageSize ;
            stat._useTimes += 1 ;
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done_assign ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      if ( SDB_OK == _allocMem( size ) )
      {
         goto retry ;
      }
      else
      {
         releaseBlock( pBlock, blockSize ) ;
         goto done ;
      }

   done_assign:
      if ( pTmpBlock )
      {
         ossMemcpy( pBlock, pTmpBlock, tmpBlockSize ) ;
         releaseBlock( pTmpBlock, tmpBlockSize ) ;
      }

   done:
      PD_TRACE_EXIT( SDB__UTILCACHEMGR_REALLOCBLOCK ) ;
      return pBlock ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_RELEASEBLOCK, "_utilCacheMgr::releaseBlock" )
   void _utilCacheMgr::releaseBlock( CHAR *&pBlock, UINT32 &blockSize )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_RELEASEBLOCK ) ;
      UINT32 slot = 0 ;

      if ( pBlock && blockSize > 0 )
      {
         slot = _sizeDown2Slot( blockSize ) ;
         SDB_ASSERT( blockSize == _slot2PageSize( slot ),
                     "BlockSize is not invalid" ) ;

         _latch[ slot ]->get() ;
         _slot[ slot ].push_back( pBlock ) ;
         utilCacheStat &stat = _getBucketCache( slot ) ;
         UINT32 slotSize = _slot2PageSize( slot ) ;
         stat._freeSize += slotSize ;
         stat._releaseSize += slotSize ;
         _latch[ slot ]->release() ;
         _freeSize.add( slotSize ) ;
         _releaseSize.add( slotSize ) ;

         _releaseEvent.signalAll() ;
      }

      pBlock = NULL ;
      blockSize = 0 ;

      PD_TRACE_EXIT( SDB__UTILCACHEMGR_RELEASEBLOCK ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR__ALLOCMEM, "_utilCacheMgr::_allocMem" )
   INT32 _utilCacheMgr::_allocMem( UINT32 size, UINT32 pageNum )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR__ALLOCMEM ) ;

      UINT32 count = 0 ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;
      UINT32 slot = 0 ;
      BOOLEAN addNonEmpty = FALSE ;

      if ( 0 == size )
      {
         goto done ;
      }

      pageSize = _sizeUp2PageSize( size ) ;
      slot = _sizeUp2Slot( size ) ;

      if ( slot >= UTIL_PAGE_SLOT_SIZE )
      {
         PD_LOG( PDERROR, "Size[%u] is more than the max page size[2^%u]",
                 size, _beginPageSizeSqrt + UTIL_PAGE_SLOT_SIZE - 1 ) ;
         SDB_ASSERT( slot < UTIL_PAGE_SLOT_SIZE, "Size is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      while( count < pageNum )
      {
         if ( _totalSize.peek() + pageSize > _maxCacheSize )
         {
            UINT64 lastSize = pageSize * ( pageNum - count ) ;
            utilCacheStat &stat = _getBucketCache( slot ) ;

            _latch[ slot ]->get() ;
            stat._nullTimes += 1 ;
            if ( stat._freeSize < lastSize )
            {
               _releaseEvent.reset() ;
            }
            _latch[ slot ]->release() ;
            _nullTimes.inc() ;

            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }

         pPage = (CHAR*)SDB_OSS_MALLOC( pageSize ) ;
         if ( !pPage )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _latch[ slot ]->get() ;
         if ( !addNonEmpty && 0 == _getBucketCache( slot )._totalSize &&
              pageSize > 0 )
         {
            addNonEmpty = TRUE ;
         }
         _slot[ slot ].push_back( pPage ) ;
         _getBucketCache( slot )._totalSize += pageSize ;
         _getBucketCache( slot )._freeSize += pageSize ;
         _latch[ slot ]->release() ;
         _totalSize.add( pageSize ) ;
         _freeSize.add( pageSize ) ;
         ++count ;
      }

      if ( addNonEmpty )
      {
         _nonEmptySlotNum.inc() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEMGR__ALLOCMEM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheMgr::registerUnit( _utilCacheUnit *pUnit )
   {
   }

   void _utilCacheMgr::unregUnit( _utilCacheUnit *pUnit )
   {
   }

   UINT32 _utilCacheMgr::_getFreeRatio( UINT64 currentTime )
   {
      UINT32 freeRatio = 0 ;
      UINT64 diffTime = 0 ;

      if ( currentTime > _lastStatTime )
      {
         diffTime = currentTime - _lastStatTime ;
      }
      else
      {
         diffTime = _lastStatTime - currentTime ;
      }

      if ( diffTime > UTIL_STAT_RATIO_INTERVAL )
      {
         _lastStatTime = currentTime ;

         UINT64 totalSz = _totalSize.fetch() ;
         UINT64 freeSz = _freeSize.fetch() ;

         UINT64 allocSize = _allocSize.swap( 0 ) ;
         UINT64 releaseSize = _releaseSize.swap( 0 ) ;

         if ( allocSize > releaseSize )
         {
            UINT64 diff = allocSize - releaseSize ;
            if ( freeSz > diff )
            {
               freeSz -= diff ;
            }
            else
            {
               freeSz = 0 ;
            }
         }

         if ( freeSz == totalSz )
         {
            _statFreeRatio[ _statIndex ] = UTIL_BLOCK_RECYCLE_FREE_RATIO - 1 ;
         }
         else if ( totalSz > 0 )
         {
            _statFreeRatio[ _statIndex ] = ( freeSz * 100 ) / totalSz ;
         }
         else
         {
            _statFreeRatio[ _statIndex ] = 0 ;
         }

         if ( ++_statIndex >= UTIL_STAT_WINDOW_SIZE )
         {
            _statIndex = 0 ;
         }

         UINT32 totalRatio = 0 ;
         for ( UINT32 i = 0 ; i < UTIL_STAT_WINDOW_SIZE ; ++i )
         {
            if ( _statFreeRatio[ i ] > 100 )
            {
               totalRatio = 0 ;
               break ;
            }
            else
            {
               totalRatio += _statFreeRatio[ i ] ;
            }
         }

         freeRatio = totalRatio / UTIL_STAT_WINDOW_SIZE ;
      }

      return freeRatio ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_CANRECYCLE, "_utilCacheMgr::canRecycle" )
   BOOLEAN _utilCacheMgr::canRecycle()
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_CANRECYCLE ) ;
      BOOLEAN needRecycle = FALSE ;
      ossTimestamp t ;
      UINT64 curTime = 0 ;
      UINT32 freeRatio = 0 ;

      if ( totalSize() <= 0 )
      {
         goto done ;
      }
      else if ( 0 == maxCacheSize() )
      {
         needRecycle = TRUE ;
         goto done ;
      }

      ossGetCurrentTime( t ) ;
      curTime = t.time * 1000 + t.microtm / 1000 ;
      freeRatio = _getFreeRatio( curTime ) ;

      if ( totalSize() * 100 / maxCacheSize() >= UTIL_CACHE_RATIO &&
           freeRatio >= UTIL_BLOCK_RECYCLE_FREE_RATIO )
      {
         needRecycle = TRUE ;
      }
      else
      {
         UINT64 timeDiff = 0 ;
         if ( curTime >= _lastRecycleTime )
         {
            timeDiff = curTime - _lastRecycleTime ;
         }
         else
         {
            timeDiff = _lastRecycleTime - curTime ;
         }

         if ( ( _nonEmptySlotNum.fetch() >= 2 &&
                _nullTimes.fetch() > 0 &&
                timeDiff >= UTIL_BLOCK_RECYCLE_MIN_TIMEOUT ) ||
              timeDiff > UTIL_BLOCK_RECYCLE_MAX_TIMEOUT )
         {
            needRecycle = TRUE ;
         }
      }

   done:
      PD_TRACE1( SDB__UTILCACHEMGR_CANRECYCLE, PD_PACK_INT( needRecycle ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEMGR_CANRECYCLE ) ;
      return needRecycle ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_RECYCLEBLOCKS, "_utilCacheMgr::recycleBlocks" )
   UINT64 _utilCacheMgr::recycleBlocks()
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_RECYCLEBLOCKS ) ;

      UINT64 recycleSize = 0 ;
      UINT64 tmpTimes = 0 ;
      UINT64 tmpNullTimes = 0 ;
      blkLatch *pLatch = NULL ;
      vector< CHAR* > freeItem ;
      vector< CHAR* >::iterator it ;

      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      _lastRecycleTime = t.time * 1000 + t.microtm / 1000 ;

      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         vector< CHAR* > &slotItem = _slot[ i ] ;
         pLatch = _latch[ i ] ;
         utilCacheStat &statItem = (*_pStat)[ i ] ;

         if ( 0 == statItem._totalSize || 0 == statItem._freeSize )
         {
            continue ;
         }

         pLatch->get() ;

         if (  statItem.getFreeRatio() >= UTIL_BLOCK_RECYCLE_FREE_RATIO ||
              ( statItem._totalSize > 0 && totalNullTimes() > 0 &&
                statItem._nullTimes * _nonEmptySlotNum.fetch() <
                totalNullTimes() )
             )
         {
            recycleSize += _recycleBucket( slotItem, &statItem, freeItem ) ;
         }

         tmpTimes += statItem._useTimes ;
         tmpNullTimes += statItem._nullTimes ;
         statItem._useTimes = 0 ;
         statItem._nullTimes = 0 ;

         pLatch->release() ;

         it = freeItem.begin() ;
         while( it != freeItem.end() )
         {
            SDB_OSS_FREE( *it ) ;
            ++it ;
         }
         freeItem.clear() ;
      }
      _totalUseTimes.sub( tmpTimes ) ;
      _nullTimes.sub( tmpNullTimes ) ;

      PD_LOG( PDDEBUG, "Recycle %lld blocks", recycleSize ) ;

      PD_TRACE1( SDB__UTILCACHEMGR_RECYCLEBLOCKS, PD_PACK_ULONG( recycleSize ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEMGR_RECYCLEBLOCKS ) ;
      return recycleSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR__RECYCLEBLK, "_utilCacheMgr::_recycleBucket" )
   UINT64 _utilCacheMgr::_recycleBucket( vector<CHAR *> &slotItem,
                                         utilCacheStat *pStat,
                                         vector< CHAR* > &freeItem )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR__RECYCLEBLK ) ;

      UINT64 recycleSize = 0 ;
      UINT32 size = ( slotItem.size() + 3 ) / 4 ;
      UINT32 standSize = UTIL_MAX_BLK_RECYCLE_SIZE ;

      if ( ( pStat->_nullTimes > 0 &&
             pStat->_nullTimes * _nonEmptySlotNum.fetch() >=
             _nullTimes.fetch() * 8 / 10 ) ||
           ( pStat->_useTimes > 0 && _nullTimes.compare( 0 ) ) )
      {
         standSize = UTIL_DFT_BLK_RECYCLE_SIZE ;
      }
      standSize = ( standSize + pStat->_pageSize - 1 ) / pStat->_pageSize ;

      if ( size > standSize )
      {
         size = standSize ;
      }

      for ( UINT32 i = 0 ; i < size ; ++i )
      {
         freeItem.push_back( slotItem.back() ) ;
         slotItem.pop_back() ;

         recycleSize += pStat->_pageSize ;

         if ( slotItem.empty() )
         {
            break ;
         }
      }
      pStat->_freeSize -= recycleSize ;
      pStat->_totalSize -= recycleSize ;
      _freeSize.sub( recycleSize ) ;
      _totalSize.sub( recycleSize ) ;

      if ( 0 == pStat->_totalSize && recycleSize > 0 )
      {
         _nonEmptySlotNum.dec() ;
      }

      PD_TRACE1( SDB__UTILCACHEMGR__RECYCLEBLK, PD_PACK_ULONG( recycleSize ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEMGR__RECYCLEBLK ) ;
      return recycleSize ;
   }

   /*
      _utilCacheBucket implement
   */
   _utilCacheBucket::_utilCacheBucket( UINT32 blkID )
   {
      _blkID = blkID ;
      _dirtyPages = 0 ;
   }

   _utilCacheBucket::~_utilCacheBucket()
   {
      SDB_ASSERT( _pages.size() == 0, "Pages must be released" ) ;
   }

   utilCachePage* _utilCacheBucket::getPage( INT32 pageID )
   {
      MAP_BLK_PAGE::iterator it = _pages.find( pageID ) ;
      if ( it != _pages.end() )
      {
         return &(it->second) ;
      }
      return NULL ;
   }

   utilCachePage* _utilCacheBucket::addPage( INT32 pageID,
                                             const utilCachePage &page )
   {
      SDB_ASSERT( !page.isInvalid(), "Page cant' be invalid" ) ;
      utilCachePage& tmpPage = _pages[ pageID ] ;
      tmpPage = page ;
      return &tmpPage ;
   }

   void _utilCacheBucket::delPage( INT32 pageID )
   {
      MAP_BLK_PAGE::iterator it = _pages.find( pageID ) ;
      if ( it != _pages.end() )
      {
         utilCachePage* pPage = &(it->second) ;
         SDB_ASSERT( !pPage->isDirty() &&
                     !pPage->isPinned() &&
                     !pPage->isLocked(),
                     "Page can't be dirty, pinned and locked" ) ;
         _pages.erase( it ) ;
      }
   }

   utilCachePage* _utilCacheBucket::allocPage( INT32 pageID, UINT32 size )
   {
      utilCachePage* pPage = NULL ;

      if ( _dirtyPages >= _pages.size() )
      {
         goto done ;
      }
      else
      {
         MAP_BLK_PAGE::iterator it = _pages.begin() ;
         while ( it != _pages.end() )
         {
            pPage = &(it->second) ;
            if ( !pPage->isDirty() && !pPage->isLocked() &&
                 !pPage->isPinned() && pPage->size() >= size )
            {
               pPage->clearDataInfo() ;
               pPage->clearNewest() ;
               utilCachePage tmpPage( *pPage ) ;

               _pages.erase( it ) ;
               utilCachePage &newPage = _pages[ pageID ] ;
               newPage = tmpPage ;
               pPage = &newPage ;
               goto done ;
            }
            ++it ;
         }
         pPage = NULL ;
      }

   done:
      return pPage ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEBLK_LOCK, "_utilCacheBucket::lock" )
   INT32 _utilCacheBucket::lock( OSS_LATCH_MODE mode, INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEBLK_LOCK ) ;

      if ( EXCLUSIVE == mode )
      {
         rc = _rwMutex.lock_w( millisec ) ;
      }
      else if ( SHARED == mode )
      {
         rc = _rwMutex.lock_r( millisec ) ;
      }
      else
      {
         goto done ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEBLK_LOCK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEBLK_UNLOCK, "_utilCacheBucket::unlock" )
   void _utilCacheBucket::unlock( OSS_LATCH_MODE mode )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEBLK_UNLOCK ) ;

      if ( SHARED == mode )
      {
         _rwMutex.release_r() ;
      }
      else if ( EXCLUSIVE == mode )
      {
         _rwMutex.release_w() ;
      }

      PD_TRACE_EXIT( SDB__UTILCACHEBLK_UNLOCK ) ;
   }

   /*
      _utilCacheContext implement
   */
   _utilCacheContext::_utilCacheContext()
   {
      _pData = NULL ;
      _offset = 0 ;
      _len = 0 ;
      _pageID = 0 ;
      _pPage = NULL ;
      _pBucket = NULL ;
      _pUnit = NULL ;
      _mode = -1 ;
      _isWrite = FALSE ;
      _newestMask = 0 ;
      _size = 0 ;
      _writeBack = FALSE ;
      _usePage = FALSE ;
      _hasDiscard = FALSE ;
   }

   _utilCacheContext::~_utilCacheContext()
   {
      release() ;
   }

   BOOLEAN _utilCacheContext::isValid() const
   {
      return ( _pBucket && _pUnit && _pageID >= 0 ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isPageValid() const
   {
      return ( _pPage && isValid() ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLocked() const
   {
      return ( EXCLUSIVE == _mode || SHARED == _mode ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLockRead() const
   {
      return SHARED == _mode ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLockWrite() const
   {
      return EXCLUSIVE == _mode ? TRUE : FALSE ;
   }

   void _utilCacheContext::unLock()
   {
      if ( _pBucket && isLocked() )
      {
         _pBucket->unlock( (OSS_LATCH_MODE)_mode ) ;
      }
      _mode = -1 ;
   }

   BOOLEAN _utilCacheContext::isDone() const
   {
      return _pData ? FALSE : TRUE ;
   }

   void _utilCacheContext::makeNewest( UINT32 newestMask )
   {
      if ( _pPage )
      {
         if ( OSS_BIT_TEST( newestMask, UTIL_WRITE_NEWEST_HEADER ) )
         {
            _pPage->makeNewestHead() ;
         }
         if ( OSS_BIT_TEST( newestMask, UTIL_WRITE_NEWEST_TAIL ) )
         {
            _pPage->makeNewestTail() ;
         }
      }
   }

   void _utilCacheContext::clearNewest()
   {
      if ( _pPage )
      {
         _pPage->clearNewest() ;
      }
   }

   void _utilCacheContext::discardPage( UINT32 &dirtyStart, UINT32 &dirtyLen,
                                        UINT64 &beginLSN, UINT64 &endLSN )
   {
      if ( isPageValid() && _pPage->isDirty() )
      {
         dirtyStart = _pPage->dirtyStart() ;
         dirtyLen = _pPage->dirtyLength() ;
         beginLSN = _pPage->beginLSN() ;
         endLSN = _pPage->endLSN() ;

         _pPage->clearDirty() ;
         _pUnit->decDirtyPages( _pBucket ) ;
         _pPage->invalidate() ;
         _hasDiscard = TRUE ;
      }
   }

   void _utilCacheContext::restorePage( UINT32 dirtyStart, UINT32 dirtyLen,
                                        UINT64 beginLSN, UINT64 endLSN )
   {
      if ( isPageValid() && _hasDiscard && !_pPage->isDirty() )
      {
         _pPage->validate() ;
         _pPage->restoryDirty( dirtyStart, dirtyLen, beginLSN, endLSN ) ;
         _pUnit->incDirtyPages( _pBucket ) ;
      }
      _hasDiscard = FALSE ;
   }

   BOOLEAN _utilCacheContext::isInCache( UINT32 offset, UINT32 len ) const
   {
      if ( _pPage && offset >= _pPage->start() &&
           offset + len <= _pPage->length() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHECTX_WRITE, "_utilCacheContext::write" )
   INT32 _utilCacheContext::write( const CHAR *pData,
                                   UINT32 offset,
                                   UINT32 len,
                                   IExecutor *cb,
                                   UINT32 newestMask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHECTX_WRITE ) ;

      if ( !isValid() )
      {
         SDB_ASSERT( FALSE, "Context is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( offset + len > _size )
      {
         SDB_ASSERT( FALSE, "offset + len must <= _size" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isDone() )
      {
         SDB_ASSERT( FALSE, "Must done before next write" ) ;
         submit( cb ) ;
      }

      if ( 0 == len )
      {
         goto done ;
      }

      if ( isPageValid() )
      {
         if ( !_pPage->isDataEmpty() &&
              ( offset + len < _pPage->start() ||
                _pPage->length() < offset ) )
         {
            if ( !_pPage->isDirty() )
            {
               _pPage->clearDataInfo() ;
            }
            else
            {
               UINT32 loadOffset = 0 ;
               UINT32 loadLen = 0 ;

               if ( offset + len < _pPage->start() )
               {
                  loadOffset = offset + len ;
                  loadLen = _pPage->start() - loadOffset ;
               }
               else
               {
                  loadOffset = _pPage->length() ;
                  loadLen = offset - loadOffset ;
               }

               rc = _loadPage( loadOffset, loadLen, cb ) ;
               if( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] failed, "
                          "rc: %d", _pageID, loadOffset, loadLen, rc ) ;
                  goto error ;
               }
            }
         }

         _pData = (CHAR*)pData ;
         _offset = offset ;
         _len = len ;
         _isWrite = TRUE ;
         _newestMask = newestMask ;
         _usePage = TRUE ;
         _writeBack = FALSE ;
      }
      else
      {
         utilCachFileBase* pFile = _pUnit->getCacheFile() ;
         rc = pFile->prepareWrite( _pageID, pData, len, offset, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Prepare write[PageID:%u, Len:%u, Offset:%u] "
                    "failed, rc: %d", _pageID, len, offset, rc ) ;
            goto error ;
         }
         else
         {
            _pData = (CHAR*)pData ;
            _offset = offset ;
            _len = len ;
            _isWrite = TRUE ;
            _newestMask = newestMask ;
            _usePage = FALSE ;
            _writeBack = FALSE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHECTX_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHECTX_READ, "_utilCacheContext::read" )
   INT32 _utilCacheContext::read( CHAR *pBuff,
                                  UINT32 offset,
                                  UINT32 len,
                                  IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHECTX_READ ) ;

      BOOLEAN readPage = FALSE ;

      if ( !isValid() )
      {
         SDB_ASSERT( FALSE, "Context is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( offset + len > _size )
      {
         SDB_ASSERT( FALSE, "offset + len must <= _size" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isDone() )
      {
         SDB_ASSERT( FALSE, "Must done before next read" ) ;
         submit( cb ) ;
      }

      if ( 0 == len )
      {
         goto done ;
      }

      if ( isPageValid() )
      {
         if ( offset >= _pPage->start() &&
              offset + len <= _pPage->length() )
         {
            readPage = TRUE ;
         }
         else if ( offset + len <= _pPage->start() ||
                   _pPage->length() <= offset )
         {
            readPage = FALSE ;
         }
         else if ( _pPage->isDirty() )
         {
            if ( offset < _pPage->start() )
            {
               rc = _loadPage( offset, _pPage->start() - offset, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] data "
                          "failed, rc: %d", _pageID, offset,
                          _pPage->start() - offset, rc ) ;
                  goto error ;
               }
            }
            if ( offset + len > _pPage->length() )
            {
               rc = _loadPage( _pPage->length(),
                               offset + len - _pPage->length(), cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] data "
                          "failed, rc: %d", _pageID, _pPage->length(),
                          offset + len - _pPage->length(), rc ) ;
                  goto error ;
               }
            }
            readPage = TRUE ;
         }
      }

      if ( readPage )
      {
         _pData = (CHAR*)pBuff ;
         _offset = offset ;
         _len = len ;
         _isWrite = FALSE ;
         _usePage = TRUE ;
         _writeBack = FALSE ;
      }
      else
      {
         utilCachFileBase* pFile = _pUnit->getCacheFile() ;
         rc = pFile->prepareRead( _pageID, pBuff, len, offset, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Prepare read[PageID:%u, Len:%u, Offset:%u] "
                    "failed, rc: %d", _pageID, len, offset, rc ) ;
            goto error ;
         }
         else
         {
            _pData = (CHAR*)pBuff ;
            _offset = offset ;
            _len = len ;
            _isWrite = FALSE ;
            _usePage = FALSE ;
            _writeBack = FALSE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHECTX_READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCacheContext::readAndCache( CHAR *pBuff,
                                          UINT32 offset,
                                          UINT32 len,
                                          IExecutor *cb )
   {
      INT32 rc = SDB_OK ;

      rc = read( pBuff, offset, len, cb ) ;
      if ( rc )
      {
         goto error ;
      }
      _writeBack = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHECTX_SUBMIT, "_utilCacheContext::submit" )
   UINT32 _utilCacheContext::submit( IExecutor *cb )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHECTX_SUBMIT ) ;
      UINT32 len = 0 ;

      if ( _pData )
      {
         INT32 rc = SDB_OK ;

         if ( _isWrite )
         {
            if ( _usePage )
            {
               BOOLEAN setDirty = FALSE ;
               rc = _pPage->write( _pData, _offset, _len, setDirty ) ;
               if ( setDirty )
               {
                  _pUnit->incDirtyPages( _pBucket ) ;
               }
               if( cb )
               {
                  _pPage->addLSN( cb->getEndLsn() ) ;
               }
            }
            else
            {
               utilCachFileBase* pFile = _pUnit->getCacheFile() ;
               rc = pFile->write( _pageID, _pData, _len, _offset,
                                  _newestMask, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Write page[ID:%d,Off:%u,Len:%u] to "
                          "file[%s] failed, rc: %d", _pageID, _offset, _len,
                          pFile->getFileName(), rc ) ;
               }
            }
         }
         else
         {
            if ( _usePage )
            {
               len = _pPage->read( _pData, _offset, _len ) ;
            }
            else
            {
               utilCachFileBase* pFile = _pUnit->getCacheFile() ;
               rc = pFile->read( _pageID, _pData, _len, _offset, len, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read page[ID:%d,Off:%u,Len:%u] from "
                          "file[%s] failed, rc: %d", _pageID, _offset,
                          _len, pFile->getFileName(), rc ) ;
               }
               else if ( _writeBack && _pPage )
               {
                  _pPage->load( _pData, _offset, len ) ;
               }
            }
         }

         if ( rc )
         {
            ossPanic() ;
         }

         _pData = NULL ;
         _len = 0 ;
         _offset = 0 ;
         _isWrite = FALSE ;
         _newestMask = 0 ;
         _writeBack = FALSE ;
      }
      _hasDiscard = FALSE ;

      PD_TRACE1( SDB__UTILCACHECTX_SUBMIT, PD_PACK_UINT( len ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHECTX_SUBMIT ) ;
      return len ;
   }

   void _utilCacheContext::rollback()
   {
      if ( _pData )
      {
         _pData = NULL ;
         _offset = 0 ;
         _len = 0 ;
         _isWrite = FALSE ;
         _newestMask = 0 ;
         _writeBack = FALSE ;
      }
   }

   void _utilCacheContext::release()
   {
      rollback() ;
      unLock() ;
      _pPage = NULL ;
      _pBucket = NULL ;
      _pUnit = NULL ;
      _hasDiscard = FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHECTX__LOADPAGE, "_utilCacheContext::_loadPage" )
   INT32 _utilCacheContext::_loadPage( UINT32 offset,
                                       UINT32 len,
                                       IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHECTX__LOADPAGE ) ;

      utilCachFileBase* pFile = NULL ;
      CHAR *pBuff = NULL ;
      UINT32 readLen = 0 ;

      SDB_ASSERT( _pPage && _pPage->size() >= offset + len,
                  "Page size must grater than offset + len" ) ;

      if ( _pPage->isNewest() )
      {
         rc = _pPage->loadWithoutData( offset, len ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Load without data in page failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else if ( 1 == _pPage->blockNum() )
      {
         CHAR *ptr = _pPage->str() ;
         pFile = _pUnit->getCacheFile() ;
         rc = pFile->read( _pageID, ptr + offset, len, offset, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Read from file[%s] failed, rc: %d",
                    pFile->getFileName(), rc ) ;
            goto error ;
         }
         rc = _pPage->loadWithoutData( offset, len ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Load without data in page failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = cb->allocBuff( len, &pBuff, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc buff from cb failed, rc: %d", rc ) ;
            goto error ;
         }

         pFile = _pUnit->getCacheFile() ;
         rc = pFile->read( _pageID, pBuff, len, offset, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Read from file[%s] failed, rc: %d",
                    pFile->getFileName(), rc ) ;
            goto error ;
         }
         rc = _pPage->load( pBuff, offset, readLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write data to page failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
         pBuff = NULL ;
      }
      PD_TRACE_EXITRC( SDB__UTILCACHECTX__LOADPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _utilCacheMerge implement
   */
   _utilCacheMerge::_utilCacheMerge()
   {
      _pCache = NULL ;
      _cacheSize = 0 ;
      _dataLength = 0 ;
      _firstPageID = -1 ;
      _lastPageID = -1 ;
      _pageNum = 0 ;

      _pageSize = 0 ;
      _pFile = NULL ;
      _isAlignment = FALSE ;
   }

   _utilCacheMerge::~_utilCacheMerge()
   {
      fini() ;
   }

   INT32 _utilCacheMerge::init( UINT32 pageSize,
                                utilCachFileBase *pFile,
                                BOOLEAN alignment,
                                UINT32 cacheSize )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( pageSize > 0 && 0 == ( pageSize % 4096 ),
                  "Page size is invalid" ) ;
      SDB_ASSERT( cacheSize > pageSize && 0 == ( cacheSize % pageSize ),
                  "Cache size must be multi of pageSize" ) ;

      if ( _pCache || !pFile ||
           0 == pageSize || 0 != ( pageSize % 4096 ) ||
           cacheSize <= pageSize )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      _pageSize = pageSize ;
      _pFile = pFile ;
      _isAlignment = alignment ;
      _cacheSize = ossAlignX( cacheSize, pageSize ) ;

      if ( alignment )
      {
         _pCache = ( CHAR* )ossAlignedAlloc( OSS_FILE_DIRECT_IO_ALIGNMENT,
                                             _cacheSize ) ;
      }
      else
      {
         _pCache = ( CHAR* )SDB_OSS_MALLOC( _cacheSize ) ;
      }
      if ( !_pCache )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory[Alignment: %s, Size: %d] failed",
                 alignment ? "true" : "false", _cacheSize ) ;
         _cacheSize = 0 ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheMerge::fini()
   {
      if ( _pCache )
      {
         if ( _isAlignment )
         {
            SDB_OSS_ORIGINAL_FREE( _pCache ) ;
         }
         else
         {
            SDB_OSS_FREE( _pCache ) ;
         }
         _pCache = NULL ;
         _cacheSize = 0 ;
      }

      _releasePages() ;
   }

   BOOLEAN _utilCacheMerge::isEmpty() const
   {
      return _dataLength == 0 ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheMerge::isFull() const
   {
      return _dataLength == _cacheSize ? TRUE : FALSE ;
   }

   UINT32 _utilCacheMerge::freeSize() const
   {
      return _cacheSize - _dataLength ;
   }

   UINT32 _utilCacheMerge::capacity() const
   {
      return _cacheSize ;
   }

   BOOLEAN _utilCacheMerge::isAlignment() const
   {
      return _isAlignment ;
   }

   UINT32 _utilCacheMerge::getLength() const
   {
      return _dataLength ;
   }

   const CHAR* _utilCacheMerge::getData() const
   {
      return _pCache ;
   }

   void _utilCacheMerge::_releasePages()
   {
      VEC_PAGE_PTR::iterator it = _vecPages.begin() ;
      while( it != _vecPages.end() )
      {
         (*it)->unpin() ;
         ++it ;
      }
      _vecPages.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_WRITE, "_utilCacheMerge::write" )
   INT32 _utilCacheMerge::write( INT32 pageID, utilCachePage *pPage,
                                 BOOLEAN hasPin )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_WRITE ) ;

      UINT32 pos = 0 ;
      UINT32 len = 0 ;
      CHAR *pBuff = NULL ;
      UINT32 offset = 0 ;
      UINT32 lastLen = pPage->dirtyLength() ;
      UINT32 alignLen = ossAlignX( lastLen, _pageSize ) ;

      if ( _pageNum > 0 && _lastPageID + 1 != pageID )
      {
         rc = SDB_IO ;
         goto error ;
      }
      else if ( !pPage->isNewest() )
      {
         rc = SDB_IO ;
         goto error ;
      }
      else if ( pPage->dirtyLength() < ( _pageSize >> 2 )  )
      {
         rc = SDB_IO ;
         goto error ;
      }
      else if ( _dataLength + alignLen > _cacheSize ||
                alignLen != _pageSize )
      {
         rc = SDB_NOSPC ;
         goto error ;
      }

      pos = pPage->beginBlock() ;
      while( NULL != ( pBuff = pPage->nextBlock( len, pos ) ) )
      {
         if ( pPage->dirtyStart() > offset + len )
         {
            offset += len ;
            lastLen -= len ;
            continue ;
         }
         else if ( pPage->dirtyStart() > offset )
         {
            UINT32 pageOffset = pPage->dirtyStart() - offset ;
            pBuff += pageOffset ;
            lastLen -= pageOffset ;
            offset += pageOffset ;
            len -= pageOffset ;
         }

         if ( lastLen < len )
         {
            len = lastLen ;
         }
         ossMemcpy( _pCache + _dataLength + offset,
                    pBuff,
                    len ) ;
         offset += len ;
         lastLen -= len ;
         if ( 0 == lastLen )
         {
            break ;
         }
      }

      _dataLength += alignLen ;
      if ( 0 == _pageNum )
      {
         _firstPageID = pageID ;
      }
      ++_pageNum ;
      _lastPageID = pageID ;

      if ( hasPin )
      {
         _vecPages.push_back( pPage ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEMGR_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEMGR_SYNC, "_utilCacheMerge::sync" )
   INT32 _utilCacheMerge::sync( IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEMGR_SYNC ) ;

      if ( _pageNum > 0 )
      {
         INT64 offset = _pFile->pageID2Offset( _firstPageID ) ;
         rc = _pFile->writeRaw( offset, _pCache, _dataLength,
                                cb, _isAlignment ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync dirty page[FirstID:%d, Offset: "
                    "%llu, Len: %d] to file[%s] failed, rc: %d",
                    _firstPageID, offset, _dataLength,
                    _pFile->getFileName(), rc ) ;
            goto error ;
         }

         _pageNum = 0 ;
         _firstPageID = -1 ;
         _lastPageID = -1 ;
         _dataLength = 0 ;
      }

      _releasePages() ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCACHEMGR_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _utilCacheUnit implement
   */

   /*
      Max sync pages number for once time
   */
   #define UTIL_CACHE_SYNC_ONCE_NUM          ( 2048 )
   #define UTIL_CACHE_SYNC_BLK_ONCE_NUM      ( 2 )
   #define UTIL_CACHE_RECYCLE_BLK_ONCE_NUM   ( UTIL_CACHE_SYNC_BLK_ONCE_NUM * 2 )
   #define UTIL_CACHE_SYNC_TOTAL_THRESHOLD   ( 100 )
   #define UTIL_CACHE_STAT_INTERVAL          ( 30000 )

   _utilCacheUnit::_utilCacheUnit( utilCacheMgr *pMgr )
   :_dirtySize( 0 ), _totalPage( 0 ),
    _statAllocNum( 0 ), _statAllocFromBlkNum( 0 ),
    _statAllocNullNum( 0 ), _statAllocWaitNum( 0 ), _statHitCacheNum( 0 ),
    _statMergeNum( 0 ), _statMergeSyncNum( 0 ), _statSyncNum( 0 ),
    _statRecycleNum( 0 ), _statTotalWaitTime( 0 ), _statMaxWaitTime( 0 ),
    _lastStatTime( 0 )
   {
      _pMgr = pMgr ;
      _pCacheFile = NULL ;
      _bucketSize = 0 ;
      _pageSize = 0 ;
      _allocTimeout = 0 ;
      _closed = TRUE ;
      _wholePage = FALSE ;
      _lastRecycleTime = 0 ;
      _lastSyncTime = 0 ;
      _useCache = TRUE ;
      _hasReg = FALSE ;

      _bgDirtyRatio = UTIL_CACHEUNIT_BG_DIRTY_RATIO ;
      _dirtyTimeout = UTIL_CACHEUNIT_DIRTY_TIMEOUT ;
      _bgFreeRatio = UTIL_CACHEUNIT_BG_FREE_RATIO ;
      _pageTimeout = UTIL_CACHEUNIT_PAGE_TIMEOUT ;

      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      _lastStatTime = t.time * 1000 + t.microtm / 1000 ;
   }

   _utilCacheUnit::~_utilCacheUnit()
   {
      SDB_ASSERT( 0 == _bucketSize, "Must call fini before this function" ) ;
   }

   INT32 _utilCacheUnit::init ( utilCachFileBase *pFile,
                                UINT32 pageSize,
                                UINT32 allocTimeout,
                                UINT32 bucketSize,
                                BOOLEAN useCache,
                                BOOLEAN wholePage )
   {
      INT32 rc = SDB_OK ;
      utilCacheBucket* pBucket = NULL ;

      if ( !_pMgr || !pFile || 0 == pageSize || 0 == bucketSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _pCacheFile = pFile ;
      _pageSize = pageSize ;
      _allocTimeout = allocTimeout ;
      _useCache = useCache ;
      _wholePage = wholePage ;

      for ( UINT32 i = 0 ; i < bucketSize ; ++i )
      {
         pBucket = SDB_OSS_NEW utilCacheBucket( i ) ;
         if ( !pBucket )
         {
            PD_LOG( PDERROR, "Alloc bucket[%u] failed", i ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _vecBucket.push_back( pBucket ) ;
         ++_bucketSize ;
      }

      _pMgr->registerUnit( this ) ;
      _hasReg = TRUE ;
      _closed = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheUnit::setUseCache( BOOLEAN useCache )
   {
      _useCache = useCache ;
   }

   void _utilCacheUnit::setUseWholePage( BOOLEAN wholePage )
   {
      _wholePage = wholePage ;
   }

   void _utilCacheUnit::setAllocTimeout( UINT32 allocTimeout )
   {
      _allocTimeout = allocTimeout ;
   }

   INT32 _utilCacheUnit::enableMerge( BOOLEAN alignment, UINT32 cacheSize )
   {
      SDB_ASSERT( _pCacheFile && _pageSize > 0, "Must init first" ) ;
      if ( _useCache && cacheSize > 0 &&
           _pMgr->maxCacheSize() > cacheSize )
      {
         return _cacheMerge.init( _pageSize, _pCacheFile,
                                  alignment, cacheSize ) ;
      }
      return SDB_OK ;
   }

   void _utilCacheUnit::updateMerge( BOOLEAN alignment, UINT32 cacheSize )
   {
      if ( _hasReg && _cacheMerge.isEmpty() )
      {
         UINT32 tmpCacheSize = ossAlignX( cacheSize, _pageSize ) ;

         if ( tmpCacheSize != _cacheMerge.capacity() ||
              alignment != _cacheMerge.isAlignment() )
         {
            lockPageCleaner() ;

            _cacheMerge.fini() ;

            if ( _useCache && cacheSize > 0 &&
                 _pMgr->maxCacheSize() > cacheSize )
            {
               INT32 rc = _cacheMerge.init( _pageSize, _pCacheFile,
                                            alignment, cacheSize ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Update merge info[aligment:%d, "
                          "cacheSize:%u] failed, rc: %d", alignment,
                          cacheSize ) ;
                  rc = SDB_OK ;
               }
            }

            unlockPageCleaner() ;
         }
      }
   }

   void _utilCacheUnit::setDirtyConfig( UINT32 bgDirtyRatio,
                                        UINT32 dirtyTimeout )
   {
      _bgDirtyRatio = bgDirtyRatio ;
      _dirtyTimeout = dirtyTimeout ;
   }

   void _utilCacheUnit::setRecycleConfig( UINT32 bgFreeRatio,
                                          UINT32 pageTimeout )
   {
      _bgFreeRatio = bgFreeRatio ;
      _pageTimeout = pageTimeout ;
   }

   void _utilCacheUnit::fini( IExecutor *cb )
   {
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;

      _closed = TRUE ;

      if ( _pMgr && _hasReg )
      {
         _pMgr->unregUnit( this ) ;
         _hasReg = FALSE ;
      }

      _pageCleaner.get() ;
      _pageCleaner.release() ;

      while( dirtyPages() > 0 )
      {
         if ( 0 == syncPages( cb, TRUE, TRUE ) )
         {
            SDB_ASSERT( dirtyPages() == 0, "Dirty Page must be 0" ) ;
            break ;
         }
      }

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         pBucket = _vecBucket[ i ] ;

         pPages = pBucket->getPages() ;

         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( it != pPages->end() )
         {
            _pMgr->release( it->second ) ;
            ++it ;
         }
         pPages->clear() ;

         SDB_OSS_DEL pBucket ;
      }
      _vecBucket.clear() ;
      _bucketSize = 0 ;

      _cacheMerge.fini() ;
   }

   UINT32 _utilCacheUnit::calcBucketID( INT32 pageID ) const
   {
      if ( 0 == _bucketSize )
      {
         return 0 ;
      }
      return (UINT32)pageID % _bucketSize ;
   }

   UINT64 _utilCacheUnit::totalPages()
   {
      return _totalPage.fetch() ;
   }

   UINT64 _utilCacheUnit::dirtyPages()
   {
      return _dirtySize.fetch() ;
   }

   UINT64 _utilCacheUnit::undirtyPages()
   {
      UINT64 totalPage = _totalPage.fetch() ;
      UINT64 dirtyPage = _dirtySize.fetch() ;

      if ( totalPage > dirtyPage )
      {
         return totalPage - dirtyPage ;
      }
      return 0 ;
   }

   void _utilCacheUnit::decDirtyPages( utilCacheBucket *pBucket )
   {
      _dirtySize.dec() ;
      pBucket->decDirty() ;
      SDB_ASSERT( (INT64)pBucket->dirtyPages() >= 0, "Dirty Size must >= 0" ) ;
   }

   void _utilCacheUnit::incDirtyPages( utilCacheBucket *pBucket )
   {
      _dirtySize.inc() ;
      pBucket->incDirty() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_GETANDLOCK, "_utilCacheUnit::getAndLock" )
   utilCachePage* _utilCacheUnit::getAndLock( INT32 pageID, UINT32 size,
                                              utilCacheBucket **ppBucket,
                                              OSS_LATCH_MODE mode,
                                              BOOLEAN alloc,
                                              IExecutor *cb )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_GETANDLOCK ) ;
      utilCachePage* pPage = NULL ;
      UINT32 timeout = 0 ;
      UINT32 bucketID = calcBucketID( pageID ) ;
      utilCacheBucket* pBucket = NULL ;
      OSS_LATCH_MODE tmpMode = mode ;
      BOOLEAN isRetry = FALSE ;

      pBucket = _vecBucket[ bucketID ] ;
      pBucket->lock( tmpMode ) ;
      *ppBucket = pBucket ;

      if ( pageID < 0 )
      {
         PD_LOG( PDERROR, "Page id[%d] is invalid", pageID ) ;
         SDB_ASSERT( pageID >= 0, "Page must >= 0" ) ;
         goto done ;
      }
      if ( !_useCache && _dirtySize.compare( 0 ) )
      {
         goto done ;
      }
      if ( size > 0 && size < _pageSize )
      {
         size = _pageSize ;
      }

   reget:
      pPage = pBucket->getPage( pageID ) ;
      if ( !pPage && alloc && size > 0 )
      {
         utilCachePage tmpPage ;
         INT32 rc = SDB_OK ;

         if ( _pMgr->maxCacheSize() < size || _closed )
         {
            goto done ;
         }
         if ( tmpMode != EXCLUSIVE )
         {
            pBucket->unlock( tmpMode ) ;
            tmpMode = EXCLUSIVE ;
            pBucket->lock( tmpMode ) ;
            goto reget ;
         }
         if ( !isRetry )
         {
            _incAllocNum( 1 ) ;
            isRetry = TRUE ;
         }
         rc = _pMgr->alloc( size, tmpPage, _wholePage ) ;
         if ( SDB_OK == rc )
         {
            pPage = pBucket->addPage( pageID, tmpPage ) ;
            _totalPage.inc() ;
         }
         else
         {
            if ( SDB_OSS_UP_TO_LIMIT != rc )
            {
               PD_LOG( PDERROR, "Alloc page[%u] failed, rc: %d",
                       size, rc ) ;
            }
            else
            {
               if ( NULL != ( pPage = pBucket->allocPage( pageID, size ) ) ||
                    NULL != ( pPage = _allocFromBucket( pBucket, size, pageID,
                                                        NULL, FALSE ) ) )
               {
                  _incAllocFromBlkNum( 1 ) ;
               }
               else if ( timeout < _allocTimeout )
               {
                  pBucket->unlock( tmpMode ) ;
                  if ( 0 == timeout )
                  {
                     _statAllocWaitNum.inc() ;
                  }
                  if ( _pMgr->waitEvent( UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ) )
                  {
                     timeout += UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ;
                     _statTotalWaitTime.add( UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ) ;
                  }
                  else
                  {
                     timeout += UTIL_PAGE_ALLOC_FIX_TIMEPIECE ;
                     _statTotalWaitTime.add( UTIL_PAGE_ALLOC_FIX_TIMEPIECE ) ;
                  }
                  _statMaxWaitTime.swapGreaterThan( timeout ) ;
                  pBucket->lock( tmpMode ) ;
                  goto reget ;
               }
               else
               {
                  _incAllocNullNum( 1 ) ;
               }
            }
            goto done ;
         }
      }
      else if ( pPage && pPage->size() < size )
      {
         if ( tmpMode != EXCLUSIVE )
         {
            pBucket->unlock( tmpMode ) ;
            tmpMode = EXCLUSIVE ;
            pBucket->lock( tmpMode ) ;
            goto reget ;
         }
         pPage->waitToUnlock() ;
         if ( !isRetry )
         {
            _incAllocNum( 1 ) ;
            isRetry = TRUE ;
         }
         INT32 rc = _pMgr->alloc( size, *pPage, _wholePage,
                                  pPage->isInvalid() ? FALSE : TRUE ) ;
         if ( rc )
         {
            /*pPage->pin() ;
            pBucket->unlock( tmpMode ) ;

            recyclePages( TRUE, size ) ;

            pBucket->lock( tmpMode ) ;
            pPage->unpin() ;

            pPage->waitToUnlock() ;
            rc = _pMgr->alloc( size, *pPage, _wholePage,
                               pPage->isInvalid() ? FALSE : TRUE ) ;
            if ( rc )*/
            if ( !_allocFromBucket( pBucket, size, pageID, pPage,
                                    pPage->isInvalid() ? FALSE : TRUE ) )
            {
               _incAllocNullNum( 1 ) ;
               if ( SDB_OSS_UP_TO_LIMIT != rc )
               {
                  PD_LOG( PDERROR, "Alloc page[%u] failed, rc: %d",
                          size, rc ) ;
               }

               if ( timeout < _allocTimeout )
               {
                  pBucket->unlock( tmpMode ) ;
                  if ( 0 == timeout )
                  {
                     _statAllocWaitNum.inc() ;
                  }
                  if ( _pMgr->waitEvent( UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ) )
                  {
                     timeout += UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ;
                     _statMaxWaitTime.add( UTIL_PAGE_ALLOC_WAIT_TIMEPIECE ) ;
                  }
                  else
                  {
                     timeout += UTIL_PAGE_ALLOC_FIX_TIMEPIECE ;
                     _statMaxWaitTime.add( UTIL_PAGE_ALLOC_FIX_TIMEPIECE ) ;
                  }
                  _statMaxWaitTime.swapGreaterThan( timeout ) ;
                  pBucket->lock( tmpMode ) ;
                  goto reget ;
               }
               else if ( pPage->isDirty() )
               {
                  rc = _syncPage( pBucket, TRUE, pPage, pageID, cb,
                                  FALSE, NULL, FALSE ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Sync page[%d] failed, rc: %d",
                             pageID, rc ) ;
                     ossPanic() ;
                  }
               }
               pPage->clearDataInfo() ;
               pPage = NULL ;
               goto done ;
            }
            else
            {
               _incAllocFromBlkNum( 1 ) ;
            }
         }
      }
      else if ( pPage )
      {
         _incHitCacheNum( 1 ) ;
      }

   done:
      if ( mode != tmpMode )
      {
         if ( pPage )
         {
            pPage->pin() ;
         }
         pBucket->unlock( tmpMode ) ;
         pBucket->lock( mode ) ;
         if ( pPage )
         {
            pPage->unpin() ;
         }
      }

      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_GETANDLOCK ) ;
      return pPage ;
   }

   utilCacheBucket* _utilCacheUnit::getBucket( UINT32 index )
   {
      if ( index >= _bucketSize )
      {
         return NULL ;
      }
      return _vecBucket[ index ] ;
   }

   void _utilCacheUnit::prepareWrite( INT32 pageID,
                                      UINT32 offset,
                                      UINT32 len,
                                      IExecutor *cb,
                                      utilCacheContext &context )
   {
      context.release() ;
      context._pageID = pageID ;
      context._pUnit = this ;
      context._mode = EXCLUSIVE ;
      context._size = offset + len ;
      context._pPage = getAndLock( pageID, offset + len,
                                   &context._pBucket,
                                   EXCLUSIVE, TRUE, cb ) ;
      if ( context._pPage && context._pPage->isInvalid() )
      {
         context._pPage->validate() ;
      }
   }

   void _utilCacheUnit::prepareRead( INT32 pageID,
                                     UINT32 offset,
                                     UINT32 len,
                                     IExecutor *cb,
                                     utilCacheContext &context )
   {
      context.release() ;
      context._pageID = pageID ;
      context._pUnit = this ;
      context._mode = SHARED ;
      context._size = offset + len ;
      context._pPage = getAndLock( pageID, offset + len,
                                   &context._pBucket,
                                   SHARED, FALSE, cb ) ;
      if ( context._pPage && context._pPage->isInvalid() )
      {
         SDB_ASSERT( FALSE, "Page must be valid" ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT__SYNCPAGE, "_utilCacheUnit::_syncPage" )
   INT32 _utilCacheUnit::_syncPage( utilCacheBucket *pBucket,
                                    BOOLEAN hasLock,
                                    utilCachePage *pPage,
                                    INT32 pageID,
                                    IExecutor *cb,
                                    BOOLEAN hasPin,
                                    BOOLEAN *pSync,
                                    BOOLEAN writeMerge )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT__SYNCPAGE ) ;

      UINT32 pos = 0 ;
      UINT32 len = 0 ;
      CHAR *pBuff = NULL ;
      UINT32 offset = 0 ;
      UINT32 lastLen = 0 ;
      BOOLEAN hasSync = FALSE ;
      BOOLEAN myBlkLock = FALSE ;
      BOOLEAN myPgLock = FALSE ;

      if ( !hasLock )
      {
         pBucket->lock( SHARED ) ;
         myBlkLock = TRUE ;
      }

      lastLen = pPage->dirtyLength() ;
      if ( !pPage->isDirty() )
      {
         goto done ;
      }
      else if ( pPage->isDirtyEmpty() )
      {
         hasSync = TRUE ;
         pPage->clearDirty() ;
         decDirtyPages( pBucket ) ;
         goto done ;
      }

      if ( writeMerge )
      {
         rc = _cacheMerge.write( pageID, pPage, hasPin ) ;
         if ( SDB_OK == rc )
         {
            _incMergeNum( 1 ) ;
            hasPin = FALSE ;
         }
         else
         {
            writeMerge = FALSE ;
         }
      }

      hasSync = TRUE ;
      pPage->clearDirty() ;
      decDirtyPages( pBucket ) ;

      if ( !writeMerge )
      {
         UINT32 newestMask = 0 ;
         if ( pPage->isNewestHead() )
         {
            newestMask |= UTIL_WRITE_NEWEST_HEADER ;
         }
         if ( pPage->isNewestTail() )
         {
            newestMask |= UTIL_WRITE_NEWEST_TAIL ;
         }

         if ( myBlkLock )
         {
            pPage->lock() ;
            myPgLock = TRUE ;
            pBucket->unlock( SHARED ) ;
            myBlkLock = FALSE ;
         }

         pos = pPage->beginBlock() ;
         while( NULL != ( pBuff = pPage->nextBlock( len, pos ) ) )
         {
            if ( pPage->dirtyStart() > offset + len )
            {
               offset += len ;
               lastLen -= len ;
               continue ;
            }
            else if ( pPage->dirtyStart() > offset )
            {
               UINT32 pageOffset = pPage->dirtyStart() - offset ;
               pBuff += pageOffset ;
               lastLen -= pageOffset ;
               offset += pageOffset ;
               len -= pageOffset ;
            }

            if ( lastLen < len )
            {
               len = lastLen ;
            }
            rc = _pCacheFile->write( pageID, pBuff, len, offset,
                                     newestMask, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Sync dirty page[ID:%d,Offset:%u,Len:%u] to "
                       "file[%s] failed, rc: %d", pageID, offset, len,
                       _pCacheFile->getFileName(), rc ) ;
               goto error ;
            }
            offset += len ;
            lastLen -= len ;
            if ( 0 == lastLen )
            {
               break ;
            }
         }
      }

   done:
      if ( myPgLock )
      {
         pPage->unlock() ;
      }
      if ( hasPin )
      {
         pPage->unpin() ;
      }
      if ( myBlkLock )
      {
         pBucket->unlock( SHARED ) ;
      }
      if ( pSync )
      {
         *pSync = hasSync ;
      }
      PD_TRACE_EXITRC( SDB__UTILCACHEUNIT__SYNCPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_LOCKCLEANER, "_utilCacheUnit::lockPageCleaner" )
   void _utilCacheUnit::lockPageCleaner()
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_LOCKCLEANER ) ;

      _pageCleaner.get() ;

      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_LOCKCLEANER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_UNLOCKCLEANER, "_utilCacheUnit::unlockPageCleaner" )
   void _utilCacheUnit::unlockPageCleaner()
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_UNLOCKCLEANER ) ;

      _pageCleaner.release() ;

      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_UNLOCKCLEANER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_CANSYNC, "_utilCacheUnit::canSync" )
   BOOLEAN _utilCacheUnit::canSync( BOOLEAN &force )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_CANSYNC ) ;
      BOOLEAN needSync = FALSE ;

      if ( dirtyPages() <= 0 )
      {
      }
      else if ( totalPages() > UTIL_CACHE_SYNC_TOTAL_THRESHOLD &&
                dirtyPages() * 100 / totalPages() >= _bgDirtyRatio )
      {
         PD_LOG( PDDEBUG, "Dirty pages: %u, Total pages: %u, Dirty ratio: %u",
                 dirtyPages(), totalPages(), _bgDirtyRatio ) ;
         force = TRUE ;
         needSync = TRUE ;
      }
      else
      {
         ossTimestamp t ;
         ossGetCurrentTime( t ) ;
         UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;

         force = FALSE ;

         if ( ( curTime >= _lastSyncTime &&
                curTime - _lastSyncTime > _dirtyTimeout ) ||
              ( curTime < _lastSyncTime &&
                _lastSyncTime - curTime > _dirtyTimeout ) )
         {
            PD_LOG( PDDEBUG, "Cur time: %llu, Last sync time: %llu, "
                    "Dirty timeout: %u", curTime, _lastSyncTime,
                    _dirtyTimeout ) ;
            needSync = TRUE ;
         }
      }

      PD_TRACE1( SDB__UTILCACHEUNIT_CANSYNC, PD_PACK_INT( needSync ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_CANSYNC ) ;
      return needSync ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_SYNCPAGES, "_utilCacheUnit::syncPages" )
   UINT32 _utilCacheUnit::syncPages( IExecutor *cb, BOOLEAN force,
                                     BOOLEAN ignoreClose )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_SYNCPAGES ) ;
      UINT32 totalPages = 0 ;
      utilCacheBucket* pBucket = NULL ;
      ossTimestamp t ;
      MAP_ID_2_PAGE_PRT tmpPages ;
      UINT32 blkSyncNum = 0 ;

      ossGetCurrentTime( t ) ;
      _lastSyncTime = t.time * 1000 + t.microtm / 1000 ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         if ( _closed && !ignoreClose )
         {
            break ;
         }

         pBucket = _vecBucket[ i ] ;
         pBucket->lock( SHARED ) ;

         utilCacheBucket::MAP_BLK_PAGE* pPages = pBucket->getPages() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         blkSyncNum = 0 ;
         while( it != pPages->end() )
         {
            utilCachePage &tmpPage = it->second ;

            if ( !tmpPage.isDirty() )
            {
               ++it ;
               continue ;
            }
            else if ( force || _lastSyncTime - tmpPage.lastWriteTime() >=
                      _dirtyTimeout )
            {
               tmpPage.pin() ;
               tmpPages[ it->first ] = &tmpPage ;
               ++blkSyncNum ;

               if ( force && blkSyncNum >= UTIL_CACHE_SYNC_BLK_ONCE_NUM )
               {
                  break ;
               }
            }
            ++it ;
         }

         pBucket->unlock( SHARED ) ;

         if ( tmpPages.size() >= UTIL_CACHE_SYNC_ONCE_NUM )
         {
            totalPages += _syncPages( tmpPages, cb, TRUE ) ;
            tmpPages.clear() ;
         }
      }

      totalPages += _syncPages( tmpPages, cb, TRUE ) ;
      PD_LOG( PDDEBUG, "Sync %d pages", totalPages ) ;

      _incSyncNum( totalPages ) ;

      PD_TRACE1( SDB__UTILCACHEUNIT_SYNCPAGES, PD_PACK_UINT( totalPages ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_SYNCPAGES ) ;
      return totalPages ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT__UNPINPAGES, "_utilCacheUnit::_unpinPages" )
   void _utilCacheUnit::_unpinPages( const _utilCacheUnit::MAP_ID_2_PAGE_PRT &pageMap )
   {
      utilCachePage *pPage = NULL ;
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT__UNPINPAGES ) ;

      MAP_ID_2_PAGE_PRT_CIT it = pageMap.begin() ;
      while( it != pageMap.end() )
      {
         pPage = ( utilCachePage* )( it->second ) ;
         pPage->unpin() ;
         ++it ;
      }

      PD_TRACE_EXIT( SDB__UTILCACHEUNIT__UNPINPAGES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_DROPDIRTY, "_utilCacheUnit::dropDirty" )
   UINT32 _utilCacheUnit::dropDirty()
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_DROPDIRTY ) ;
      UINT32 totalPages = 0 ;
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         pBucket = _vecBucket[ i ] ;
         pBucket->lock( EXCLUSIVE ) ;
         pPages = pBucket->getPages() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( it != pPages->end() )
         {
            utilCachePage& page = it->second ;

            if ( page.isDirty() )
            {
               page.clearDirty() ;
               ++totalPages ;
               decDirtyPages( pBucket ) ;
            }
            page.invalidate() ;
            ++it ;
         }
         pBucket->unlock( EXCLUSIVE ) ;
      }

      PD_LOG( PDDEBUG, "Dropped %lld pages", totalPages ) ;

      PD_TRACE1( SDB__UTILCACHEUNIT_DROPDIRTY, PD_PACK_UINT( totalPages ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_DROPDIRTY ) ;
      return totalPages ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT__SYNCPAGES, "_utilCacheUnit::_syncPages" )
   UINT32 _utilCacheUnit::_syncPages( const _utilCacheUnit::MAP_ID_2_PAGE_PRT &pageMap,
                                      IExecutor *cb,
                                      BOOLEAN hasPin )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT__SYNCPAGES ) ;

      UINT32 totalPages = 0 ;
      BOOLEAN hasSync = FALSE ;
      utilCacheBucket *pBucket = NULL ;
      MAP_ID_2_PAGE_PRT_CIT it = pageMap.begin() ;
      MAP_ID_2_PAGE_PRT_CIT itNext ;
      BOOLEAN write2Merge = FALSE ;

      UINT32 breakSize = UTIL_CACHE_SYNC_ONCE_NUM / 3 ;
      UINT32 lastBlkID = 0 ;
      UINT32 curBlkID = 0 ;

      while( it != pageMap.end() )
      {
         write2Merge = TRUE ;
         itNext =  it ;
         ++itNext ;

         curBlkID = calcBucketID( it->first ) ;
         if ( curBlkID < lastBlkID && totalPages >= breakSize )
         {
            break ;
         }
         else
         {
            lastBlkID = curBlkID ;
         }

         if ( !it->second->isNewest() )
         {
            write2Merge = FALSE ;
         }
         else if ( _cacheMerge.freeSize() < _pageSize )
         {
            write2Merge = FALSE ;
         }
         else if ( itNext != pageMap.end() &&
                   it->first + 1 != itNext->first )
         {
            write2Merge = FALSE ;
         }
         else if ( itNext == pageMap.end() &&
                   ( _cacheMerge.getPageNum() == 0 ||
                     _cacheMerge.getLastPageID() + 1 != it->first ) )
         {
            write2Merge = FALSE ;
         }

         if ( !write2Merge && _cacheMerge.getPageNum() > 0 )
         {
            rc = _cacheMerge.sync( cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Sync cache merge failed, rc: %d", rc ) ;
               ossPanic() ;
            }
            _incMergeSyncNum( 1 ) ;
         }

         pBucket = getBucket( calcBucketID( it->first ) ) ;
         rc = _syncPage( pBucket, FALSE, ( utilCachePage* )( it->second ),
                         it->first, cb, hasPin, &hasSync, write2Merge ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync page[%d] failed, rc: %d",
                    it->first, rc ) ;
            ossPanic() ;
         }
         if ( hasSync )
         {
            ++totalPages ;
         }
         ++it ;
      }

      if ( _cacheMerge.getPageNum() > 0 )
      {
         rc = _cacheMerge.sync( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync cache merge failed, rc: %d", rc ) ;
            ossPanic() ;
         }
         _incMergeSyncNum( 1 ) ;
      }

      utilCachePage *pTmpPage = NULL ;
      while ( hasPin && it != pageMap.end() )
      {
         pTmpPage = ( utilCachePage* )it->second ;
         pTmpPage->unpin() ;
         ++it ;
      }

      PD_TRACE1( SDB__UTILCACHEUNIT__SYNCPAGES, PD_PACK_UINT( totalPages ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT__SYNCPAGES ) ;
      return totalPages ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_CANRECYCLE, "_utilCacheUnit::canRecycle" )
   BOOLEAN _utilCacheUnit::canRecycle( BOOLEAN &force )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_CANRECYCLE ) ;
      BOOLEAN needRecycle = FALSE ;

      if ( totalPages() <= 0 )
      {
      }
      else if ( 0 == _pMgr->maxCacheSize() )
      {
         needRecycle = TRUE ;
      }
      else if ( _pMgr->totalSize() * 100 / _pMgr->maxCacheSize() >=
                UTIL_CACHE_RATIO &&
                _pMgr->freeSize() * 100 / _pMgr->totalSize() <=
                 _bgFreeRatio )
      {
         PD_LOG( PDDEBUG, "Total size: %u, Free size: %u, Free ratio: %u",
                 _pMgr->totalSize(), _pMgr->freeSize(), _bgFreeRatio ) ;
         force = TRUE ;
         needRecycle = TRUE ;
      }
      else
      {
         ossTimestamp t ;
         ossGetCurrentTime( t ) ;
         UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;

         force = FALSE ;

         if ( ( curTime >= _lastRecycleTime &&
                curTime - _lastRecycleTime > _pageTimeout ) ||
              ( curTime < _lastRecycleTime &&
                _lastRecycleTime - curTime > _pageTimeout ) )
         {
            PD_LOG( PDDEBUG, "Cur time: %llu, Last recycle time: %llu, "
                    "Page timeout: %u", curTime, _lastRecycleTime,
                    _pageTimeout ) ;
            needRecycle = TRUE ;
         }
      }

      PD_TRACE1( SDB__UTILCACHEUNIT_CANRECYCLE, PD_PACK_INT( needRecycle ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_CANRECYCLE ) ;
      return needRecycle ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT_RECYCLEPAGES, "_utilCacheUnit::recyclePages" )
   UINT64 _utilCacheUnit::recyclePages( BOOLEAN force, INT64 exceptSize )
   {
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT_RECYCLEPAGES ) ;
      UINT64 totalSize = 0 ;
      UINT32 totalPageNum = 0 ;
      ossTimestamp t ;
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;
      UINT32 blkRecycleNum = 0 ;
      UINT32 bucketPages = 0 ;

      ossGetCurrentTime( t ) ;
      _lastRecycleTime = t.time * 1000 + t.microtm / 1000 ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         if ( _closed )
         {
            break ;
         }
         blkRecycleNum = 0 ;
         pBucket = _vecBucket[ i ] ;
         pBucket->lock( EXCLUSIVE ) ;

         pPages = pBucket->getPages() ;
         bucketPages = pPages->size() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( bucketPages > pBucket->dirtyPages() &&
                 it != pPages->end() )
         {
            utilCachePage& page = it->second ;

            if ( page.isDirty() || page.isLocked() || page.isPinned() )
            {
               ++it ;
               continue ;
            }
            if ( !force && _lastRecycleTime - page.lastTime() < _pageTimeout )
            {
               ++it ;
               continue ;
            }
            totalSize += page.size() ;
            _pMgr->release( page ) ;
            pPages->erase( it++ ) ;
            --bucketPages ;
            _totalPage.dec() ;
            ++totalPageNum ;

            ++blkRecycleNum ;
            if ( force && blkRecycleNum >= UTIL_CACHE_RECYCLE_BLK_ONCE_NUM )
            {
               break ;
            }
         }
         pBucket->unlock( EXCLUSIVE ) ;

         if ( exceptSize > 0 && totalSize > ( ( UINT64 )exceptSize << 1 ) )
         {
            break ;
         }
      }

      PD_LOG( PDDEBUG, "Recycled %lld bytes, %d pages",
              totalSize, totalPageNum ) ;

      _incRecycleNum( totalPageNum ) ;

      PD_TRACE1( SDB__UTILCACHEUNIT_RECYCLEPAGES, PD_PACK_ULONG( totalSize ) ) ;
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT_RECYCLEPAGES ) ;
      return totalSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCACHEUNIT__ALLOCFROMBLK, "_utilCacheUnit::_allocFromBucket" )
   utilCachePage* _utilCacheUnit::_allocFromBucket( utilCacheBucket *pBucket,
                                                    UINT32 size,
                                                    INT32 pageID,
                                                    utilCachePage *pItem,
                                                    BOOLEAN keepData )
   {
      utilCachePage *pPage = NULL ;
      PD_TRACE_ENTRY( SDB__UTILCACHEUNIT__ALLOCFROMBLK ) ;

      utilCacheBucket* pTmpBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;
      UINT32 bucketPages = 0 ;
      utilCachePage tmpPage ;
      BOOLEAN bGet = FALSE ;

      if ( _dirtySize.fetch() >= _totalPage.fetch() )
      {
         goto done ;
      }

      if ( pItem )
      {
         pItem->pin() ;
      }

      pBucket->unlock( EXCLUSIVE ) ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         pTmpBucket = _vecBucket[ i ] ;
         pTmpBucket->lock( EXCLUSIVE ) ;

         pPages = pTmpBucket->getPages() ;
         bucketPages = pPages->size() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( bucketPages > pTmpBucket->dirtyPages() &&
                 it != pPages->end() )
         {
            utilCachePage& page = it->second ;

            if ( page.isDirty() || page.isLocked() || page.isPinned() )
            {
               ++it ;
               continue ;
            }
            else if ( page.size() < size )
            {
               ++it ;
               continue ;
            }

            tmpPage = page ;
            pPages->erase( it ) ;
            bGet = TRUE ;

            break ;
         }
         pTmpBucket->unlock( EXCLUSIVE ) ;

         if ( bGet )
         {
            break ;
         }
      }

      pBucket->lock( EXCLUSIVE ) ;

      if ( pItem )
      {
         pItem->unpin() ;
      }

      if ( !bGet )
      {
         goto done ;
      }

      tmpPage.validate() ;

      if ( pItem )
      {
         pItem->waitToUnlock() ;

         if ( pItem->isDirty() || pItem->isPinned() || pItem->isLocked() ||
              ( keepData && !pItem->isDataEmpty() ) )
         {
            tmpPage.copy( *pItem ) ;
            pItem->resetLock() ;
            pItem->resetPin() ;
            pItem->clearDirty() ;
            pItem->clearDataInfo() ;
         }

         _pMgr->release( *pItem ) ;
         _totalPage.dec() ;
         _incRecycleNum( 1 ) ;

         *pItem = tmpPage ;
         pPage = pItem ;
      }
      else
      {
         pPage = pBucket->getPage( pageID ) ;
         if ( pPage )
         {
            if ( pPage->size() >= size )
            {
               _pMgr->release( tmpPage ) ;
            }
            else
            {
               pPage->waitToUnlock() ;

               if ( pItem->isDirty() || pItem->isPinned() ||
                    pItem->isLocked() || !pPage->isDataEmpty() )
               {
                  tmpPage.copy( *pPage ) ;
                  pPage->resetLock() ;
                  pPage->resetPin() ;
                  pPage->clearDirty() ;
                  pPage->clearDataInfo() ;
               }

               _pMgr->release( *pPage ) ;
               *pPage = tmpPage ;
            }

            _totalPage.dec() ;
            _incRecycleNum( 1 ) ;
         }
         else
         {
            pPage = pBucket->addPage( pageID, tmpPage ) ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__UTILCACHEUNIT__ALLOCFROMBLK ) ;
      return pPage ;
   }

   void _utilCacheUnit::dumpStatInfo()
   {
      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;
      UINT32 diff = curTime - _lastStatTime ;

      if ( diff < UTIL_CACHE_STAT_INTERVAL )
      {
         return ;
      }

      UINT32 statTotalPage = totalPages() ;
      UINT32 statDirtyPage = dirtyPages() ;
      UINT32 statAlloc = _statAllocNum.swap( 0 ) ;
      UINT32 statAllocNull = _statAllocNullNum.swap( 0 ) ;
      UINT32 statAllocWait = _statAllocWaitNum.swap( 0 ) ;
      UINT32 statAllocBlk = _statAllocFromBlkNum.swap( 0 ) ;
      UINT32 statHitCache = _statHitCacheNum.swap( 0 ) ;
      UINT32 statMerge = _statMergeNum ;
      _statMergeNum = 0 ;
      UINT32 statMergeSync = _statMergeSyncNum ;
      _statMergeSyncNum = 0 ;
      UINT32 syncNum = _statSyncNum.swap( 0 ) ;
      UINT32 recyclNum = _statRecycleNum.swap( 0 ) ;
      UINT64 totalWaitTime = _statTotalWaitTime.swap( 0 ) ;
      UINT32 maxWaitTime = _statMaxWaitTime.swap( 0 ) ;

      _lastStatTime = curTime ;

      CHAR text[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      ossSnprintf( text, OSS_MAX_PATHSIZE,
                   OSS_NEWLINE
                   "Unit Name         : %s"OSS_NEWLINE
                   "Total Time(Sec)   : %u"OSS_NEWLINE
                   "Cache Total Sz(MB): %.2f"OSS_NEWLINE
                   "Cache Free Sz(MB) : %.2f"OSS_NEWLINE
                   "Total Page        : %u"OSS_NEWLINE
                   "Dirty Page        : %u"OSS_NEWLINE
                   "Alloc Num         : %u"OSS_NEWLINE
                   "Alloc Null Num    : %u"OSS_NEWLINE
                   "Alloc Wait Num    : %u"OSS_NEWLINE
                   "Alloc Blk Num     : %u"OSS_NEWLINE
                   "Hit Cache Num     : %u"OSS_NEWLINE
                   "Merge Num         : %u"OSS_NEWLINE
                   "Merge Sync        : %u"OSS_NEWLINE
                   "Sync Num          : %u"OSS_NEWLINE
                   "Recycle Num       : %u"OSS_NEWLINE
                   "Total Wait Tm(ms) : %llu"OSS_NEWLINE
                   "Max Wait Tm(ms)   : %u"OSS_NEWLINE
                   "Avg Wait Tm(ms)   : %.2f"OSS_NEWLINE
                   "Dirty ratio       : %.2f %%"OSS_NEWLINE
                   "Alloc Speed       : %.2f /s"OSS_NEWLINE
                   "Alloc Null Speed  : %.2f /s"OSS_NEWLINE
                   "Alloc Wait Speed  : %.2f /s"OSS_NEWLINE
                   "Alloc Blk Speed   : %.2f /s"OSS_NEWLINE
                   "Hit Cache Speed   : %.2f /s"OSS_NEWLINE
                   "Merge Speed       : %.2f /s"OSS_NEWLINE
                   "Merge Sync Avg Len: %.2f"OSS_NEWLINE
                   "Sync Speed        : %.2f /s"OSS_NEWLINE
                   "Recycle Speed     : %.2f /s"OSS_NEWLINE,
                   _pCacheFile->getFileName(),
                   diff / 1000,
                   (FLOAT64)_pMgr->totalSize() / 1048576,
                   (FLOAT64)_pMgr->freeSize() / 1048576,
                   statTotalPage,
                   statDirtyPage,
                   statAlloc,
                   statAllocNull,
                   statAllocWait,
                   statAllocBlk,
                   statHitCache,
                   statMerge,
                   statMergeSync,
                   syncNum,
                   recyclNum,
                   totalWaitTime,
                   maxWaitTime,
                   (FLOAT64)totalWaitTime / ( statAllocWait == 0 ? 1 : statAllocWait ),
                   (FLOAT64)statDirtyPage * 100 / ( statTotalPage == 0 ? 1 : statTotalPage ),
                   (FLOAT64)statAlloc / diff * 1000,
                   (FLOAT64)statAllocNull / diff * 1000,
                   (FLOAT64)statAllocWait / diff * 1000,
                   (FLOAT64)statAllocBlk / diff * 1000,
                   (FLOAT64)statHitCache / diff * 1000,
                   (FLOAT64)statMerge / diff * 1000,
                   (FLOAT64)statMerge / ( statMergeSync == 0 ? 1 : statMergeSync ) ,
                   (FLOAT64)syncNum / diff * 1000,
                   (FLOAT64)recyclNum / diff * 1000
                   ) ;

      PD_LOG( PDEVENT, text ) ;
   }

}

