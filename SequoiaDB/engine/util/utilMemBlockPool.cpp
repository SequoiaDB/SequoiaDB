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

   Source File Name = utilMemBlockPool.cpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/04/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilMemBlockPool.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossUtil.hpp"

#include "pd.hpp"

#ifdef SDB_ENGINE
#include "pmdEnv.hpp"
#endif // SDB_ENGINE

extern BOOLEAN ossMemDebugEnabled ;

namespace engine
{

   #define UTIL_MEM_ALLOC_MAX_TRY_LEVEL         ( 2 )

   #define UTIL_POOL_MEM_STAT_FILE              ".mempoolstat"
   #define UTIL_MEMPOOL_TRACEDUMP_TM_BUF        64
   #define UTIL_MEMPOOL_DUMP_BUFFSIZE           ( 65536 )

   #define UTIL_MOST_BLOCK_MAXSZ(maxSize)       ( (maxSize) >> 2 )

   /*
      cache timeout(ms)
   */
   #define UTIL_POOL_CACHE_TIMEOUT              ( 300 * OSS_ONE_SEC )
   #define UTIL_POOL_CACHE_SHRINK_STEP_MIN      ( 64 )                  /// 128MB
   #define UTIL_POOL_CACHE_SHRINK_STEP_MAX      ( 1024 )                /// 2GB

   /*
      _utilMemBlockPool implement
   */
   _utilMemBlockPool::_utilMemBlockPool( BOOLEAN isGlobal )
   :_isGlobal( isGlobal ),
    _maxSize( 0 ),
    _maxCacheSize( 0 ),
    _allocThreshold( 0 ),
    _totalSize( 0 ),
    _oorTimes( 0 ),
    _curOOLSize( 0 ),
    _resetOOLTick( 0 ),
    _header( NULL ),
    _tailer( NULL ),
    _cacheNum( 0 ),
    _cacheSize( 0 )
   {
      for ( INT32 i = 0 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         _slots[ i ] = NULL ;
      }

      _clearStat() ;
   }

   _utilMemBlockPool::~_utilMemBlockPool()
   {
      fini() ;
   }

   void _utilMemBlockPool::setMaxSize( UINT64 maxSize )
   {
      _maxSize = maxSize ;
      _maxCacheSize = _maxSize ;
      UINT64 aBlockMaxSize = _maxSize >> 3 ;
      UINT64 aHugeBlockMaxSize = _maxSize >> 4 ;

      if ( _slots[ MEMBLOCKPOOL_TYPE_32 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_32 ]->setMaxSize( aBlockMaxSize ) ;
      }

      /*
         64B is most used. So, increase the block size
      */
      if ( _slots[ MEMBLOCKPOOL_TYPE_64 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_64 ]->setMaxSize( UTIL_MOST_BLOCK_MAXSZ(_maxSize) ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_128 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_128 ]->setMaxSize( UTIL_MOST_BLOCK_MAXSZ(_maxSize) ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_256 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_256 ]->setMaxSize( aBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_512 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_512 ]->setMaxSize( aBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_1024 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_1024 ]->setMaxSize( aBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_2048 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_2048 ]->setMaxSize( aBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_4096 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_4096 ]->setMaxSize( aBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_8192 ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_8192 ]->setMaxSize( aBlockMaxSize ) ;
      }

      /// Huge Block
      if ( _slots[ MEMBLOCKPOOL_TYPE_16K ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_16K ]->setMaxSize( aHugeBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_32K ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_32K ]->setMaxSize( aHugeBlockMaxSize ) ;
      }

      if ( _slots[ MEMBLOCKPOOL_TYPE_64K ] )
      {
         _slots[ MEMBLOCKPOOL_TYPE_64K ]->setMaxSize( aHugeBlockMaxSize ) ;
      }
   }

   void _utilMemBlockPool::setAllocThreshold( UINT32 allocThreshold )
   {
      _allocThreshold = allocThreshold ;
   }

   INT32 _utilMemBlockPool::init( UINT64 maxSize, UINT32 allocThreshold )
   {
      INT32 rc = SDB_OK ;

      UINT64 aBlockMaxSize = 0 ;
      UINT64 aHugeBlockMaxSize = 0 ;
      utilSegmentInterface *pSeg = NULL ;

      _maxSize = maxSize ;
      _maxCacheSize = _maxSize ;
      _allocThreshold = allocThreshold ;

      aBlockMaxSize = _maxSize >> 3 ;
      aHugeBlockMaxSize = _maxSize >> 4 ;

      /// when alloc or init failed, ignored
      pSeg = SDB_OSS_NEW _utilSegmentManager<element32B>( MEMBLOCKPOOL_TYPE_32, "32B" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_32 ] = pSeg ;
      }

      /*
         64B is most used. So, increase the block size
      */
      pSeg = SDB_OSS_NEW _utilSegmentManager<element64B>( MEMBLOCKPOOL_TYPE_64, "64B" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          UTIL_MOST_BLOCK_MAXSZ(_maxSize),
                          UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_64 ] = pSeg ;
      }

      /*
         dpsTransLRB(64) and dpsTransLRBHeader(114), so increase the block size
      */
      pSeg = SDB_OSS_NEW _utilSegmentManager<element128B>( MEMBLOCKPOOL_TYPE_128, "128B" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          UTIL_MOST_BLOCK_MAXSZ(_maxSize),
                          UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_128 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element256B>( MEMBLOCKPOOL_TYPE_256, "256B" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_256 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element512B>( MEMBLOCKPOOL_TYPE_512, "512B" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_512 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element1K>( MEMBLOCKPOOL_TYPE_1024, "1KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_1024 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element2K>( MEMBLOCKPOOL_TYPE_2048, "2KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_2048 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element4K>( MEMBLOCKPOOL_TYPE_4096, "4KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_4096 ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element8K>( MEMBLOCKPOOL_TYPE_8192, "8KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aBlockMaxSize,
                          UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_8192 ] = pSeg ;
      }

      /// Huge block
      pSeg = SDB_OSS_NEW _utilSegmentManager<element16K>( MEMBLOCKPOOL_TYPE_16K, "16KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aHugeBlockMaxSize,
                          UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_16K ] = pSeg ;
      }

      pSeg = SDB_OSS_NEW _utilSegmentManager<element32K>( MEMBLOCKPOOL_TYPE_32K, "32KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aHugeBlockMaxSize,
                          UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_32K ] = pSeg ;
      }

      /*
         64K is the query default size, so increase the pool
      */
      pSeg = SDB_OSS_NEW _utilSegmentManager<element64K>( MEMBLOCKPOOL_TYPE_64K, "64KB" ) ;
      if ( pSeg )
      {
         rc = pSeg->init( UTIL_MEM_SEG_BLOCK_SIZE,
                          aHugeBlockMaxSize,
                          UTIL_MEM_64K_BLOCK_SUBPOOL_NUM,
                          this ) ;
         if ( rc )
         {
            SDB_OSS_DEL pSeg ;
            pSeg = NULL ;
            rc = SDB_OK ;
         }
         _slots[ MEMBLOCKPOOL_TYPE_64K ] = pSeg ;
      }

      UINT64 curDBTick = getDBTick() ;
      /// reset stats
      for ( INT32 i = 0 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         _stats[ i ].reset( curDBTick ) ;
      }
      _resetOOLTick = curDBTick ;

      return rc ;
   }

   void _utilMemBlockPool::fini()
   {
      /// disable cache
      _maxCacheSize = 0 ;

      for ( INT32 i = 0 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         if ( _slots[ i ] )
         {
            SDB_OSS_DEL _slots[ i ] ;
            _slots[ i ] = NULL ;
         }
      }

      /// release cache
      _clearBlock() ;
   }

   void _utilMemBlockPool::_clearBlock()
   {
      _blockNode *p = NULL ;
      while( _header )
      {
         p = _header ;
         _header = _header->_next ;
         SDB_OSS_FREE( (void*)p ) ;
      }

      _totalSize.sub( _cacheSize ) ;
      _tailer = NULL ;
      _cacheNum.swap( 0 ) ;
      _cacheSize = 0 ;
   }

   UINT64 _utilMemBlockPool::_shrinkCache( BOOLEAN forced )
   {
      _blockNode *p = NULL ;
      UINT64 shrinkSize = 0 ;
      UINT32 shrinkNum = 0 ;
      UINT32 shrinkNumThreshold = _cacheNum.fetch() >> 1 ;

      if ( shrinkNumThreshold < UTIL_POOL_CACHE_SHRINK_STEP_MIN )
      {
         shrinkNumThreshold = UTIL_POOL_CACHE_SHRINK_STEP_MIN ;
      }
      else if ( shrinkNumThreshold > UTIL_POOL_CACHE_SHRINK_STEP_MAX )
      {
         shrinkNumThreshold = UTIL_POOL_CACHE_SHRINK_STEP_MAX ;
      }

      {
         ossScopedLock __lock( &_cacheLatch ) ;

         while( _header )
         {
            if ( forced || _cacheSize > _maxCacheSize ||
                 ( getTickSpanTime( _header->_dbTick ) > UTIL_POOL_CACHE_TIMEOUT &&
                   shrinkNum < shrinkNumThreshold ) )
            {
               if ( !p )
               {
                  p = _header ;
               }
               _header = _header->_next ;

               _cacheNum.dec() ;
               _cacheSize -= UTIL_MEM_SEG_BLOCK_SIZE ;

               shrinkSize += UTIL_MEM_SEG_BLOCK_SIZE ;
               ++shrinkNum ;
            }
            else
            {
               break ;
            }
         }

         if ( !_header )
         {
            _tailer = NULL ;
         }
         else if ( p && _header->_prev )
         {
            _header->_prev->_next = NULL ;
            _header->_prev = NULL ;
         }
      }

      _totalSize.sub( shrinkSize ) ;

      /// free memory out-of lock
      _blockNode *tmp = NULL ;
      while ( p )
      {
         tmp = p ;
         p = p->_next ;
         SDB_OSS_FREE( (void*)tmp ) ;
      }

      return shrinkSize ;
   }

   UINT64 _utilMemBlockPool::shrink( BOOLEAN forced )
   {
      UINT64 shrinkSize = 0 ;

      /// reset stat
      if ( getTickSpanTime( _resetOOLTick ) > UTIL_MEM_OOL_RESET_INTERVAL )
      {
         for ( INT32 i = 0 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
         {
            _stats[ i ].reset( getDBTick() ) ;
         }
         /// not reset _curOOLSize
         _maxOOLSize = 0 ;

         _resetOOLTick = getDBTick() ;
      }

      shrinkSize += _shrink( ( forced ? ~0 : 0 ), 0, -1 ) ;
      shrinkSize += _shrinkCache( forced ) ;

      return shrinkSize ;
   }

   UINT64 _utilMemBlockPool::_shrink( UINT64 expectSize, UINT32 beginSlot, INT32 skipSlot )
   {
      UINT32 freeSegToKeep = UTIL_SEGMENT_SEGKEEP_AUTO ;
      UINT64 hasFreeSize = 0 ;
      UINT64 slotExpectSize = expectSize ;
      UINT32 count = 0 ;

      if ( 0 == _maxSize || expectSize > 0 || _totalSize.fetch() >= _maxSize )
      {
         freeSegToKeep = 0 ;
      }

      if ( beginSlot >= MEMBLOCKPOOL_TYPE_MAX )
      {
         beginSlot = 0 ;
      }

      while ( count++ < MEMBLOCKPOOL_TYPE_MAX )
      {
         if ( skipSlot != (INT32)beginSlot && _slots[ beginSlot ] )
         {
            _slots[ beginSlot ]->shrink( freeSegToKeep, &hasFreeSize, slotExpectSize ) ;
            if ( expectSize > 0 )
            {
               if ( hasFreeSize >= expectSize )
               {
                  break ;
               }
               slotExpectSize =  expectSize - hasFreeSize ;
            }
         }

         ++beginSlot ;
         if ( beginSlot >= MEMBLOCKPOOL_TYPE_MAX )
         {
            beginSlot = 0 ;
         }
      }

      if ( hasFreeSize > 0 )
      {
         PD_LOG( PDINFO, "Has freed %llu bytes pooled memory, Total:%llu",
                 hasFreeSize, getTotalSize() ) ;
      }

      return hasFreeSize ;
   }

   UINT64 _utilMemBlockPool::getTotalSize()
   {
      return _totalSize.fetch() ;
   }

   UINT64 _utilMemBlockPool::getUsedSize()
   {
      UINT64 usedSize = 0 ;
      for ( INT32 i = MEMBLOCKPOOL_TYPE_32 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         if ( _slots[ i ] )
         {
            usedSize += _slots[ i ]->getUsedSize() ;
         }
      }
      return usedSize ;
   }

   UINT64 _utilMemBlockPool::getMaxOOLSize()
   {
      INT64 maxOOLSize = _maxOOLSize ;
      if ( maxOOLSize < 0 )
      {
         maxOOLSize = 0 ;
      }
      return (UINT64)maxOOLSize ;
   }

   _utilMemBlockPool::MEMBLOCKPOOL_TYPE
      _utilMemBlockPool::size2MemType( UINT32 size )
   {
      if ( size <= UTIL_MEM_ELEMENT_32 )
      {
         return MEMBLOCKPOOL_TYPE_32 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_64 )
      {
         return MEMBLOCKPOOL_TYPE_64 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_128 )
      {
         return MEMBLOCKPOOL_TYPE_128 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_256 )
      {
         return MEMBLOCKPOOL_TYPE_256 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_512 )
      {
         return MEMBLOCKPOOL_TYPE_512 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_1024 )
      {
         return MEMBLOCKPOOL_TYPE_1024 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_2048 )
      {
         return MEMBLOCKPOOL_TYPE_2048 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_4096 )
      {
         return MEMBLOCKPOOL_TYPE_4096 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_8192 )
      {
         return MEMBLOCKPOOL_TYPE_8192 ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_16K )
      {
         return MEMBLOCKPOOL_TYPE_16K ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_32K )
      {
         return MEMBLOCKPOOL_TYPE_32K ;
      }
      else if ( size <= UTIL_MEM_ELEMENT_64K )
      {
         return MEMBLOCKPOOL_TYPE_64K ;
      }

      return MEMBLOCKPOOL_TYPE_DYN ;
   }

   UINT32 _utilMemBlockPool::type2Size( MEMBLOCKPOOL_TYPE type )
   {
      UINT32 size = 0 ;

      switch ( type )
      {
         case MEMBLOCKPOOL_TYPE_32 :
            size = UTIL_MEM_ELEMENT_32 ;
            break ;
         case MEMBLOCKPOOL_TYPE_64 :
            size = UTIL_MEM_ELEMENT_64 ;
            break ;
         case MEMBLOCKPOOL_TYPE_128 :
            size = UTIL_MEM_ELEMENT_128 ;
            break ;
         case MEMBLOCKPOOL_TYPE_256 :
            size = UTIL_MEM_ELEMENT_256 ;
            break ;
         case MEMBLOCKPOOL_TYPE_512 :
            size = UTIL_MEM_ELEMENT_512 ;
            break ;
         case MEMBLOCKPOOL_TYPE_1024 :
            size = UTIL_MEM_ELEMENT_1024 ;
            break ;
         case MEMBLOCKPOOL_TYPE_2048 :
            size = UTIL_MEM_ELEMENT_2048 ;
            break ;
         case MEMBLOCKPOOL_TYPE_4096 :
            size = UTIL_MEM_ELEMENT_4096 ;
            break ;
         case MEMBLOCKPOOL_TYPE_8192 :
            size = UTIL_MEM_ELEMENT_8192 ;
            break ;
         case MEMBLOCKPOOL_TYPE_16K :
            size = UTIL_MEM_ELEMENT_16K ;
            break ;
         case MEMBLOCKPOOL_TYPE_32K :
            size = UTIL_MEM_ELEMENT_32K ;
            break ;
         case MEMBLOCKPOOL_TYPE_64K :
            size = UTIL_MEM_ELEMENT_64K ;
            break ;
         default :
            break ;
      }
      return size ;
   }

   void* _utilMemBlockPool::alloc( UINT32 size,
                                   const CHAR *pFile,
                                   UINT32 line,
                                   UINT32 *pRealSize,
                                   const CHAR *pInfo )
   {
      MEMBLOCKPOOL_TYPE type  = MEMBLOCKPOOL_TYPE_MAX ;
      UINT32 realSize   = 0 ;
      MEMBLOCKPOOL_TYPE realType = MEMBLOCKPOOL_TYPE_MAX ;
      CHAR * ptr        = NULL ;
      void * userPtr    = NULL ;
      UINT32 tryLevel   = 0 ;

      if ( 0 == size )
      {
         if ( pRealSize )
         {
            *pRealSize = 0 ;
         }
         goto done ;
      }

      realSize = UTIL_MEM_SIZE_2_REALSIZE( size ) ;
      type = size2MemType( realSize ) ;

      if ( _maxSize > 0 && ( 0 == _allocThreshold || size <= _allocThreshold ) )
      {
         while ( type >= MEMBLOCKPOOL_TYPE_32 &&
                 type < MEMBLOCKPOOL_TYPE_MAX &&
                 ++tryLevel <= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
         {
            if ( _slots[ type ] &&
                 SDB_OK == _slots[ type ]->acquire( (void*&)ptr ) )
            {
               realType = type ;
               break ;
            }

            /// print info
            UINT64 oolInc = 0 ;
            if ( 1 == tryLevel && _stats[ type ].inc( pFile, line, pInfo, this, oolInc ) )
            {
               INT64 maxOOLSize = _maxOOLSize ;
               if ( maxOOLSize < 0 )
               {
                  maxOOLSize = 0 ;
               }

               PD_LOG( PDWARNING, "The mempool(%s) OOL times upto %llu, reference(%s:%u:%s), "
                       "should increase the mempool maxsize(%u MB) to (> %u MB). "
                       "Use 'kill -39 <pid>' for detail information in diaglog "
                       "path's <pid>.mempoolstat file",
                       _slots[ type ]->getName(), oolInc,
                       _stats[ type ]._pLastFile, _stats[ type ]._lastLine,
                       _stats[ type ]._pLastInfo, (UINT32)( _maxSize >> 20 ),
                       (UINT32)( ( _maxSize + maxOOLSize ) >> 20 ) ) ;
            }

            type = (MEMBLOCKPOOL_TYPE)( (INT32)type + 1 ) ;
         }

         if ( !ptr )
         {
            if ( realSize > UTIL_MEM_ELEMENT_64K )
            {
               _oorTimes.inc() ;
            }

            /// set curOOL and maxOOL
            INT64 tmpCurOOLSize = _curOOLSize.add( realSize ) ;
            if ( tmpCurOOLSize + realSize > _maxOOLSize )
            {
               _maxOOLSize = tmpCurOOLSize + realSize ;
            }
         }
      }

      if ( !ptr )
      {
         ptr = (CHAR*)ossMemAlloc( realSize, pFile, line ) ;
         if ( !ptr )
         {
            goto done ;
         }
         realType = MEMBLOCKPOOL_TYPE_DYN ;
      }
      else
      {
         realSize = type2Size( realType ) ;
         SDB_ASSERT( realSize > 0, "Real size is invalid" ) ;
      }

      _fillPtr( ptr, (UINT16)realType, realSize ) ;
      /// set user ptr
      userPtr = (void*)UTIL_MEM_PTR_2_USERPTR( ptr ) ;

      if ( pRealSize )
      {
         *pRealSize = realSize - UTIL_MEM_TOTAL_FILL_LEN ;
      }

      if ( MEMBLOCKPOOL_TYPE_DYN != realType &&
           _isGlobal &&
           ossMemDebugEnabled )
      {
         ossPoolMemTrack( (void*)ptr, size, ossHashFileName( pFile ), line, pInfo ) ;
      }

   done :
      return userPtr ;
   }

   void _utilMemBlockPool::_fillPtr( CHAR * ptr, UINT16 type, UINT32 size )
   {
      /// set b-eye
      *UTIL_MEM_PTR_B_EYE_PTR( ptr ) = UTIL_MEM_B_EYE_CHAR ;
      /// set e-eye
      *UTIL_MEM_PTR_E_EYE_PTR( ptr, size ) = UTIL_MEM_E_EYE_CHAR ;
      /// set size
      *UTIL_MEM_PTR_SIZE_PTR( ptr ) = size ;
      /// set type
      *UTIL_MEM_PTR_TYPE_PTR( ptr ) = (UINT8)type ;
      /// set flag
      *UTIL_MEM_PTR_FLAG_PTR( ptr ) = UTIL_MEM_FLAG_NORMAL ;
   }

   BOOLEAN _utilMemBlockPool::_checkAndExtract( const CHAR *ptr,
                                                UINT32 *pUserSize,
                                                UINT16 *pType )
   {
      BOOLEAN valid = TRUE ;

      if ( UTIL_MEM_B_EYE_CHAR != *UTIL_MEM_PTR_B_EYE_PTR( ptr ) )
      {
         SDB_ASSERT( FALSE, "Invalid b-eye" ) ;
         valid = FALSE ;
      }
      else
      {
         UINT16 type = (UINT16)(*UTIL_MEM_PTR_TYPE_PTR( ptr )) ;
         UINT32 size = *UTIL_MEM_PTR_SIZE_PTR( ptr ) ;

         if ( type < MEMBLOCKPOOL_TYPE_DYN || type >= MEMBLOCKPOOL_TYPE_MAX )
         {
            SDB_ASSERT( FALSE, "Invalid type" ) ;
            valid = FALSE ;
         }
         else if ( size < UTIL_MEM_TOTAL_FILL_LEN )
         {
            SDB_ASSERT( FALSE, "Invalid size" ) ;
            valid = FALSE ;
         }
         else if ( MEMBLOCKPOOL_TYPE_DYN != type &&
                   size != type2Size( (MEMBLOCKPOOL_TYPE)type ) )
         {
            SDB_ASSERT( FALSE, "Invalid size" ) ;
            valid = FALSE ;
         }
         else if ( UTIL_MEM_E_EYE_CHAR != *UTIL_MEM_PTR_E_EYE_PTR( ptr, size ) )
         {
            SDB_ASSERT( FALSE, "Invalid e-eye" ) ;
            valid = FALSE ;
         }
         else
         {
            if ( pUserSize )
            {
               *pUserSize = size - UTIL_MEM_TOTAL_FILL_LEN ;
            }
            if ( pType )
            {
               *pType = type ;
            }
         }
      }

      return valid ;
   }

   void* _utilMemBlockPool::realloc( void *ptr,
                                     UINT32 size,
                                     const CHAR *pFile,
                                     UINT32 line,
                                     UINT32 *pRealSize,
                                     const CHAR *pInfo )
   {
      void  *newUserPtr = NULL ;
      UINT32 oldUserSize = 0 ;

      if ( 0 == size )
      {
         newUserPtr = ptr ;
         goto done ;
      }

      if ( ptr )
      {
         UINT16 oldType = MEMBLOCKPOOL_TYPE_MAX ;
         CHAR *oldRealPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
         if ( !_checkAndExtract( oldRealPtr, &oldUserSize, &oldType ) )
         {
            newUserPtr = SDB_OSS_REALLOC( ptr, size ) ;
            if ( pRealSize )
            {
               *pRealSize = size ;
            }
            goto done ;
         }
         if ( oldUserSize >= size )
         {
            newUserPtr = ptr ;
            if ( pRealSize )
            {
               *pRealSize = oldUserSize ;
            }
            goto done ;
         }
         else if ( MEMBLOCKPOOL_TYPE_DYN == oldType )
         {
            CHAR *newPtr = NULL ;
            UINT32 newRealSize = UTIL_MEM_SIZE_2_REALSIZE( size ) ;

            newPtr = (CHAR*)SDB_OSS_REALLOC( oldRealPtr, newRealSize ) ;
            if ( newPtr )
            {
               _fillPtr( newPtr, MEMBLOCKPOOL_TYPE_DYN, newRealSize ) ;
               newUserPtr = (void*)UTIL_MEM_PTR_2_USERPTR( newPtr ) ;

               if ( pRealSize )
               {
                  *pRealSize = newRealSize - UTIL_MEM_TOTAL_FILL_LEN ;
               }
            }
            goto done ;
         }
      }

      newUserPtr = alloc( size, pFile, line, pRealSize, pInfo ) ;
      if ( !newUserPtr )
      {
         goto done ;
      }
      if ( ptr )
      {
         ossMemcpy( newUserPtr, ptr, oldUserSize ) ;
         release( ptr ) ;
      }

   done:
      return newUserPtr ;
   }

   void _utilMemBlockPool::release( void *&ptr )
   {
      INT32 rc       = SDB_OK ;
      UINT16 type    = MEMBLOCKPOOL_TYPE_MAX ;
      UINT32 userSize = 0 ;
      CHAR *realPtr  = NULL ;

      if ( NULL == ptr )
      {
         goto done ;
      }

      realPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
      if ( !_checkAndExtract( realPtr, &userSize, &type ) )
      {
         SDB_OSS_FREE( ptr ) ;
         ptr = NULL ;
         goto done ;
      }

      if ( MEMBLOCKPOOL_TYPE_DYN != type &&
           _isGlobal &&
           ossMemDebugEnabled )
      {
         ossPoolMemUnTrack( (void*)realPtr ) ;
      }

      if ( MEMBLOCKPOOL_TYPE_DYN == type )
      {
         SDB_OSS_FREE( realPtr ) ;

         /// dec curOOL
         INT64 before = _curOOLSize.sub( userSize + UTIL_MEM_TOTAL_FILL_LEN ) ;
         if ( before < (INT64)(userSize + UTIL_MEM_TOTAL_FILL_LEN) )
         {
            _curOOLSize.swapGreaterThan( 0 ) ;
         }
      }
      else if ( type >= MEMBLOCKPOOL_TYPE_32 && type < MEMBLOCKPOOL_TYPE_MAX )
      {
         *UTIL_MEM_PTR_FLAG_PTR(realPtr) = UTIL_MEM_FLAG_NOUSE ;
         if ( _slots[ type ] )
         {
            rc = _slots[ type ]->release( (void *)realPtr ) ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid type (%d)", type ) ;
         SDB_ASSERT( FALSE, "Invalid arguments" ) ;
      }

      SDB_ASSERT( ( SDB_OK == rc ), "Sever error during release" ) ;

      if ( SDB_OK == rc )
      {
         ptr = NULL ;
      }

   done:
      return ;
   }

   BOOLEAN _utilMemBlockPool::canAllocSegment( UINT64 size )
   {
      if ( _totalSize.fetch() + size >= _maxSize )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _utilMemBlockPool::canBallonUp( INT32 id, UINT64 size )
   {
      UINT64 canShrinkSize = 0 ;

      SDB_ASSERT( size == UTIL_MEM_SEG_BLOCK_SIZE, "Invalid size" ) ;

      /// first check _totalSize
      if ( _totalSize.fetch() + size < _maxSize )
      {
         return TRUE ;
      }

      if ( size == UTIL_MEM_SEG_BLOCK_SIZE && !_cacheNum.compare( 0 ) )
      {
         return TRUE ;
      }

      /// then check slot's pending or empty(shrink)
      for ( INT32 i = MEMBLOCKPOOL_TYPE_32 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         /// should skip self
         if ( i != id && _slots[ i ] )
         {
            canShrinkSize += ( (UINT64)( _slots[ i ]->getPendingAndEmptySegNum() ) *
                               _slots[ i ]->getSegBlockSize() ) ;
            if ( canShrinkSize >= size )
            {
               break ;
            }
         }
      }

      /// shrink
      if ( canShrinkSize >= size )
      {
         /// should skip self
         if ( _shrink( ( size << 1 ), ossRand() % MEMBLOCKPOOL_TYPE_MAX, id ) > size )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   void* _utilMemBlockPool::allocBlock( UINT64 size )
   {
      void *p = NULL ;

      /// only cache the size UTIL_MEM_SEG_BLOCK_SIZE
      SDB_ASSERT( size == UTIL_MEM_SEG_BLOCK_SIZE, "Invalid size" ) ;

      if ( size == UTIL_MEM_SEG_BLOCK_SIZE && !_cacheNum.compare( 0 ) )
      {
         ossScopedLock __lock( &_cacheLatch ) ;
         p = (void*)_popBlock( size ) ;
      }

      if ( !p )
      {
         /// directly allocate
         p = (void*)SDB_OSS_MALLOC( size ) ;

         if ( p )
         {
            _totalSize.add( size ) ;
         }
      }

      return p ;
   }

   void _utilMemBlockPool::releaseBlock( void *p, UINT64 size )
   {
      BOOLEAN hasPushed = FALSE ;

      if ( !p )
      {
         return ;
      }

      /// only cache the size UTIL_MEM_SEG_BLOCK_SIZE
      SDB_ASSERT( size == UTIL_MEM_SEG_BLOCK_SIZE, "Invalid size" ) ;

      if ( size == UTIL_MEM_SEG_BLOCK_SIZE &&
           _maxCacheSize > 0 &&
           _cacheSize + size < _maxCacheSize )
      {
         ossScopedLock __lock( &_cacheLatch ) ;
         hasPushed = _pushBlock( (_blockNode*)p, size ) ;
      }

      if ( !hasPushed )
      {
         /// directly free
         SDB_OSS_FREE( p ) ;

         _totalSize.sub( size ) ;
      }
   }

   BOOLEAN _utilMemBlockPool::canShrink( UINT32 objectSize,
                                         UINT32 totalObjNum,
                                         UINT32 usedObjNum )
   {
      if ( 0 == _maxSize || _totalSize.fetch() >= _maxSize )
      {
         return TRUE ;
      }
      else if ( totalObjNum > 0 )
      {
         FLOAT64 ratio = (FLOAT64)usedObjNum / totalObjNum ;
         if ( ratio < UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   UINT64 _utilMemBlockPool::getDBTick() const
   {
#ifdef SDB_ENGINE
      return pmdGetDBTick() ;
#else
      return ossGetCurrentMilliseconds() ;
#endif // SDB_ENGINE
   }

   UINT64 _utilMemBlockPool::getTickSpanTime( UINT64 lastTick ) const
   {
#ifdef SDB_ENGINE
      return pmdGetTickSpanTime( lastTick ) ;
#else
      UINT64 curTime = ossGetCurrentMilliseconds() ;
      if ( curTime > lastTick )
      {
         return curTime - lastTick ;
      }
      return 0 ;
#endif // SDB_ENGINE
   }

   BOOLEAN _utilMemBlockPool::canShrinkDiscrete() const
   {
      return TRUE ;
   }

   void _utilMemBlockPool::_clearStat()
   {
      _acquireTimes = 0 ;
      _releaseTimes = 0 ;
      _oomTimes = 0 ;
      _oolTimes = 0 ;
      _shrinkSize = 0 ;
      _oorTimes.swap( 0 ) ;
      _lastOORTimes = 0 ;

      _curOOLSize.swap( 0 ) ;
      _maxOOLSize = 0 ;
   }

   UINT32 _utilMemBlockPool::dump( CHAR *pBuff, UINT32 buffLen )
   {
      UINT32 len = 0 ;

      UINT64 acquireTimes = 0 ;
      UINT64 releaseTimes = 0 ;
      UINT64 oomTimes = 0 ;
      UINT64 oolTimes = 0 ;
      UINT64 shrinkSize = 0 ;
      UINT64 oorTimes = _oorTimes.fetch() ;
      UINT64 totalSize = _totalSize.fetch() ;
      UINT64 usedSize = 0 ;
      INT64  curOOLSize = _curOOLSize.fetch() ;
      INT64  maxOOLSize = _maxOOLSize ;

      usedSize = getUsedSize() ;

      if ( curOOLSize < 0 )
      {
         curOOLSize = 0 ;
      }
      if ( maxOOLSize < 0 )
      {
         maxOOLSize = 0 ;
      }

      len = ossSnprintf( pBuff, buffLen,
                         "         Max Size : %llu" OSS_NEWLINE
                         "      Cur OOLSize : %llu" OSS_NEWLINE
                         "      Max OOLSize : %llu" OSS_NEWLINE
                         "       Total Size : %llu" OSS_NEWLINE
                         "       Cache Size : %llu" OSS_NEWLINE
                         "        Cache Num : %u" OSS_NEWLINE
                         "        Used Size : %llu" OSS_NEWLINE
                         "        Free Size : %llu" OSS_NEWLINE
                         "   Alloc Threshold: %u" OSS_NEWLINE
                         "      Shrink Type : %s" OSS_NEWLINE,
                         _maxSize,
                         curOOLSize,
                         maxOOLSize,
                         totalSize,
                         _cacheSize,
                         _cacheNum.fetch(),
                         usedSize,
                         totalSize - usedSize,
                         _allocThreshold,
                         canShrinkDiscrete() ? "discrete" : "tail" ) ;

      for ( INT32 i = MEMBLOCKPOOL_TYPE_32 ; i < MEMBLOCKPOOL_TYPE_MAX ; ++i )
      {
         if ( _slots[ i ] )
         {
            len += _slots[ i ]->dump( pBuff + len, buffLen - len,
                                      &acquireTimes,
                                      &releaseTimes,
                                      &oomTimes,
                                      &oolTimes,
                                      &shrinkSize ) ;
         }
      }

      len += ossSnprintf( pBuff + len, buffLen - len,
                          OSS_NEWLINE
                          "Pool Memory Stat" OSS_NEWLINE
                          "    Acquire Times : %llu (Inc: %lld )" OSS_NEWLINE
                          "    Release Times : %llu (Inc: %lld )" OSS_NEWLINE
                          "        OOM Times : %llu (Inc: %lld )" OSS_NEWLINE
                          "        OOL Times : %llu (Inc: %lld )" OSS_NEWLINE
                          "        OOR Times : %llu (Inc: %lld )" OSS_NEWLINE
                          "      Shrink Size : %llu (Inc: %lld )" OSS_NEWLINE,
                          acquireTimes,
                          acquireTimes - _acquireTimes,
                          releaseTimes,
                          releaseTimes - _releaseTimes,
                          oomTimes,
                          oomTimes - _oomTimes,
                          oolTimes,
                          oolTimes - _oolTimes,
                          oorTimes,
                          oorTimes - _lastOORTimes,
                          shrinkSize,
                          shrinkSize - _shrinkSize ) ;

      _acquireTimes = acquireTimes ;
      _releaseTimes = releaseTimes ;
      _oomTimes = oomTimes ;
      _oolTimes = oolTimes ;
      _lastOORTimes = oorTimes ;
      _shrinkSize = shrinkSize ;

      return len ;
   }

   /*
      Callback Funcs
   */
   BOOLEAN utilPoolMemCheck( void *p )
   {
      if ( p )
      {
         return utilPoolPtrCheck( UTIL_MEM_PTR_2_USERPTR( p ), NULL ) ;
      }
      return TRUE ;
   }

   void utilGetPoolMemInfo( void *p, UINT64 &size, INT32 &pool,
                            INT32 &index, BOOLEAN *pFreeInTc )
   {
      size = 0 ;
      pool = -1 ;
      index = -1 ;

      if ( p )
      {
         size = (UINT64)*UTIL_MEM_PTR_SIZE_PTR( p ) ;

         UINT16 type = (UINT16)(*UTIL_MEM_PTR_TYPE_PTR( p )) ;
         if ( _utilMemBlockPool::MEMBLOCKPOOL_TYPE_DYN != type )
         {
            _utilSegmentPool<UINT64>::_objX *pObjX =
               ( _utilSegmentPool<UINT64>::_objX* )_GET_OBJX_ADDRESS( p ) ;
            pool = _GET_UNPACKED_POOLID( pObjX->_index ) ;
            index = pObjX->_index & _SEGMENT_OBJ_INDEX_MASK ;
         }

         if ( pFreeInTc &&
              UTIL_MEM_FLAG_TC_NOUSE == *UTIL_MEM_PTR_FLAG_PTR( p ) )
         {
            *pFreeInTc = TRUE ;
         }
      }
   }

   void* utilGetPoolMemUserPtr( void *p )
   {
      return (void*)UTIL_MEM_PTR_2_USERPTR( p ) ;
   }

   class _utilPoolCallbackAssit
   {
      public:
         _utilPoolCallbackAssit()
         {
            ossSetPoolMemcheckFunc( (OSS_POOL_MEMCHECK_FUNC)utilPoolMemCheck ) ;
            ossSetPoolMemInfoFunc( (OSS_POOL_MEMINFO_FUNC)utilGetPoolMemInfo ) ;
            ossSetPoolMemUserPtrFunc( (OSS_POOL_MEMUSERPTR_FUNC)utilGetPoolMemUserPtr ) ;
         }
   } ;
   _utilPoolCallbackAssit s_assitPoolCallbak ;

   /*
      Tool function
   */
   INT32 utilDumpInfo2File( const CHAR *pPath,
                            const CHAR *pFilePostfix,
                            const CHAR *pBuff,
                            UINT32 buffLen )
   {
      ossPrimitiveFileOp trapFile ;
      CHAR fileName [ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      UINT32 len = 0 ;
      INT32 rc = SDB_OK ;

      // build trace file name
      len = ossSnprintf ( fileName, sizeof(fileName), "%s%s%u%s",
                          pPath, OSS_FILE_SEP,
                          ossGetCurrentProcessID(),
                          pFilePostfix ) ;
      if ( len >= sizeof( fileName ) )
      {
         rc = SDB_INVALIDPATH ;
         // file path invalid
         goto error ;
      }
      else
      {
         static ossSpinXLatch s_dumpLatch ;

         ossSignalShield shield ;
         shield.doNothing() ;

         ossScopedLock lock( &s_dumpLatch ) ;
   
         // open file
         trapFile.Open ( fileName ) ;
         if ( !trapFile.isValid() )
         {
            rc = SDB_IO ;
            goto error ;
         }

         trapFile.seekToEnd () ;
         trapFile.Write( pBuff, (INT64)buffLen ) ;
      }

   done :
      trapFile.Close() ;
      return rc ;
   error:
      goto done ;
   }

   void utilDumpPoolMemInfo( const CHAR *pPath )
   {
      if ( utilGetGlobalMemPool() )
      {
         UINT32 len = 0 ;
         CHAR *pBuff = ( CHAR* )SDB_OSS_MALLOC( UTIL_MEMPOOL_DUMP_BUFFSIZE )  ;
         if ( !pBuff )
         {
            return ;
         }
         ossMemset( pBuff, 0, UTIL_MEMPOOL_DUMP_BUFFSIZE ) ;

         CHAR timebuff[ UTIL_MEMPOOL_TRACEDUMP_TM_BUF ] = { 0 } ;
         ossTimestamp current ;

         ossGetCurrentTime( current ) ;
         ossTimestampToString( current, timebuff ) ;

         /// dump header
         len = ossSnprintf( pBuff, UTIL_MEMPOOL_DUMP_BUFFSIZE,
                            OSS_NEWLINE OSS_NEWLINE
                            "====> Dump pool memory status( %s ) ====>"
                            OSS_NEWLINE,
                            timebuff ) ;

         utilDumpInfo2File( pPath, UTIL_POOL_MEM_STAT_FILE, pBuff, len ) ;

         /// dump context
         len = utilGetGlobalMemPool()->dump( pBuff,
                                             UTIL_MEMPOOL_DUMP_BUFFSIZE ) ;
         utilDumpInfo2File( pPath, UTIL_POOL_MEM_STAT_FILE, pBuff, len ) ;

         SDB_OSS_FREE( pBuff ) ;
      }
   }

   /*
      Global var
   */
   static _utilMemBlockPool  g_memPool( TRUE ) ;

   utilMemBlockPool* utilGetGlobalMemPool()
   {
      return &g_memPool ;
   }

   BOOLEAN utilPoolPtrCheck( void *ptr, UINT32 *pUserSize, UINT16 *pType )
   {
      if ( ptr )
      {
         void *pRealPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
         return _utilMemBlockPool::_checkAndExtract( ( const CHAR* )pRealPtr,
                                                     pUserSize,
                                                     pType ) ;
      }
      return FALSE ;
   }

   UINT32 utilPoolGetPtrSize( void *ptr )
   {
      UINT32 size = 0 ;
      utilPoolPtrCheck( ptr, &size ) ;
      return size ;
   }

   void* utilPoolAlloc( UINT32 size,
                        const CHAR *pFile,
                        UINT32 line,
                        UINT32 *pRealSize,
                        const CHAR *pInfo )
   {
      return g_memPool.alloc( size, pFile, line, pRealSize, pInfo ) ;
   }

   void* utilPoolRealloc( void* ptr,
                          UINT32 size,
                          const CHAR *pFile,
                          UINT32 line,
                          UINT32 *pRealSize,
                          const CHAR *pInfo )
   {
      return g_memPool.realloc( ptr, size, pFile, line, pRealSize, pInfo ) ;
   }

   void utilPoolRelease( void*& ptr )
   {
      g_memPool.release( ptr ) ;
   }

}


