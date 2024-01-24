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

   Source File Name = utilMemListPool.cpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/05/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/


#include "utilMemListPool.hpp"
#include "ossUtil.hpp"

#ifdef SDB_ENGINE
   #include "pmdEnv.hpp"
#endif // SDB_ENGINE

#if UTIL_MEM_LIST_BASE_EXPONENT < 5
   #error "Invalid UTIL_MEM_LIST_BASE_EXPONENT define"
#endif

#define UTIL_THREAD_MEM_STAT_FILE            ".memtcstat"
#define UTIL_MEM_TRACEDUMP_TM_BUF            64

extern BOOLEAN ossMemDebugEnabled ;

namespace engine
{

   /*
      Configs
   */
   static UINT32 g_maxTCCacheSize = UTIL_MEM_POOL_MAXCACHE_SIZE_DFT ;

   #define UTIL_DUMP_BUFFSIZE             ( 2800 )
   #define UTIL_MIN_EBB_SIZE              ( 2048 )    /// 2K
   #define UTIL_MAX_EBB_SIZE              ( 131072 )  /// 128K

   #define UTIL_MEMLIST_FREE_TIMEOUT      ( 300 * OSS_ONE_SEC )
   #define UTIL_TC_SHRINK_INTERVAL        ( 60 * OSS_ONE_SEC )

   void utilSetMaxTCSize( UINT32 maxCacheSize )
   {
      g_maxTCCacheSize = maxCacheSize ;
   }

   UINT32 utilGetMaxTCSize()
   {
      return g_maxTCCacheSize ;
   }

   /*
      _utilMemListItem implement
   */
   _utilMemListItem::_utilMemListItem( UINT32 blockSize,
                                       utilMemListEvent *pEvent )
   : _blockSize( blockSize )
   {
#ifdef _DEBUG
      SDB_ASSERT( blockSize >= sizeof( utilMemListNode ),
                  "Invalid blockSize" ) ;
#endif //_DEBUG
      _header  = NULL ;
      _pEvent  = pEvent ;
      _cachedSize = 0 ;

      _allocCount = 0 ;
      _deallocCount = 0 ;
      _hitCount = 0 ;
      _pushCount = 0 ;
      _shrinkSize = 0 ;

      _lastAllocTick = 0 ;
   }

   _utilMemListItem::~_utilMemListItem()
   {
      clear() ;
   }

   BOOLEAN _utilMemListItem::_canCacheBlock()
   {
      UINT32 divide = UTIL_MEM_POOL_LIST_NUM ;

      if ( _blockSize <= UTIL_MEM_ELEMENT_256 )
      {
         divide = UTIL_MEM_POOL_LIST_NUM >> 1 ;
      }

      if ( 0 == g_maxTCCacheSize ||
           ( _allocCount << 2 ) < _deallocCount ||
           _cachedSize + _blockSize > g_maxTCCacheSize / divide )
      {
         return FALSE ;
      }
      else if ( _pEvent && !_pEvent->canCacheBlock( _blockSize ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void* _utilMemListItem::alloc( UINT32 size, const CHAR *pFile, UINT32 line,
                                  UINT32 *pRealSize, const CHAR *pInfo )
   {
      if ( 0 == size )
      {
         if ( pRealSize )
         {
            *pRealSize = 0 ;
         }
         return NULL ;
      }

#ifdef _DEBUG
      SDB_ASSERT( size + UTIL_MEM_TOTAL_FILL_LEN <= _blockSize, "Size should <= blockSize" ) ;
#endif //_DEBUG

      void *ptr = NULL ;

      ++_allocCount ;

#ifdef SDB_ENGINE
      _lastAllocTick = pmdGetDBTick() ;
#endif // SDB_ENGINE

#ifdef _DEBUG
         SDB_ASSERT( NULL == _header || _cachedSize >= _blockSize,
                     "Invalid cachedSize" ) ;
#endif //_DEBUG

      if ( _header && _cachedSize >= _blockSize )
      {
         ptr = (void*)_header ;
         _header = _header->_next ;

         ++_hitCount ;
         _cachedSize -= _blockSize ;
         if ( _pEvent )
         {
            _pEvent->onAllocCache( _blockSize ) ;
         }

         if ( pRealSize )
         {
            *pRealSize = _blockSize - UTIL_MEM_TOTAL_FILL_LEN ;
         }

         /// set tc inuse flag
         *UTIL_MEM_PTR_FLAG_PTR( UTIL_MEM_USERPTR_2_PTR(ptr) ) = UTIL_MEM_FLAG_TC_INUSE ;

         if ( ossMemDebugEnabled )
         {
            ossThreadMemTrack( ptr, size, ossHashFileName( pFile ), line, pInfo ) ;
         }
      }
      else
      {
         if ( NULL != _header )
         {
            // in this else branch, the header should not be valid
            // if header is valid, means the cached size is wrong
#ifdef _DEBUG
            SDB_ASSERT( FALSE, "header should not be valid" ) ;
#endif
            // cut the link
            _header = NULL ;
         }

         if ( _pEvent )
         {
            _pEvent->onOutOfCache( _blockSize, _cachedSize ) ;
         }
         BOOLEAN canAllocBlockSize = _canAllocBlockSize( size ) ;

         ptr = utilPoolAlloc( canAllocBlockSize ?
                              ( _blockSize - UTIL_MEM_TOTAL_FILL_LEN ) :
                              size,
                              pFile, line,
                              pRealSize,
                              pInfo ) ;
      }

      return ptr ;
   }

   void _utilMemListItem::dealloc( void *p )
   {
      if ( !p ) return ;

#ifdef _DEBUG
      SDB_ASSERT( utilPoolGetPtrSize( p ) + UTIL_MEM_TOTAL_FILL_LEN ==
                  _blockSize, "Invalid size" ) ;
#endif //_DEBUG

      ++_deallocCount ;

      if ( ossMemDebugEnabled )
      {
         ossThreadMemUnTrack( p ) ;
      }

      if ( _canCacheBlock() )
      {
         /// set tc nouse flag
         *UTIL_MEM_PTR_FLAG_PTR( UTIL_MEM_USERPTR_2_PTR(p) ) = UTIL_MEM_FLAG_TC_NOUSE ;

         utilMemListNode *pNode = (utilMemListNode*)p ;
         pNode->_next = _header ;
         _header = pNode ;

         _cachedSize += _blockSize ;
         ++_pushCount ;

         if ( _pEvent )
         {
            _pEvent->onPushedCache( _blockSize ) ;
         }
      }
      else
      {
         utilPoolRelease( p ) ;
      }
   }

   void _utilMemListItem::clear()
   {
      utilMemListNode *pNode = NULL ;
      while ( _header )
      {
         pNode = _header ;
         _header = pNode->_next ;
         utilPoolRelease( (void*&)pNode ) ;
      }
      if ( _pEvent && _cachedSize > 0 )
      {
         _pEvent->onReleaseCache( _cachedSize ) ;
      }
      _cachedSize = 0 ;

      _allocCount = 0 ;
      _deallocCount = 0 ;
      _hitCount = 0 ;
      _pushCount = 0 ;
      _shrinkSize = 0 ;

      _lastAllocTick = 0 ;
   }

   UINT64 _utilMemListItem::shrinkAll()
   {
      UINT64 freedSize = 0 ;
      utilMemListNode *pNode = NULL ;

      while ( _header )
      {
         pNode = _header ;
         _header = pNode->_next ;
         utilPoolRelease( (void*&)pNode ) ;

         freedSize += _blockSize ;
#ifdef _DEBUG
         SDB_ASSERT( _cachedSize >= _blockSize, "Invalid cachedSize" ) ;
#endif //_DEBUG
         _cachedSize -= _blockSize ;
      }

      if ( freedSize > 0 && _pEvent )
      {
         _pEvent->onReleaseCache( freedSize ) ;
      }
      _shrinkSize += freedSize ;

      return freedSize ;
   }

   UINT32 _utilMemListItem::dump( CHAR * pBuff, UINT32 buffSize )
   {
      UINT32 len = 0 ;

      if ( _cachedSize != 0 || _allocCount != 0 || _deallocCount != 0 ||
           _hitCount != 0 || _pushCount != 0 )
      {
         FLOAT64 hitRatio = 0.0 ;
         FLOAT64 pushRatio = 0.0 ;

         if ( _allocCount > 0 )
         {
            hitRatio = ( FLOAT64 )_hitCount / _allocCount * 100 ;
         }
         if ( _deallocCount > 0 )
         {
            pushRatio = ( FLOAT64 )_pushCount / _deallocCount * 100 ;
         }

         len = ossSnprintf( pBuff, buffSize,
                            OSS_NEWLINE
                            "   BlockSize : %u" OSS_NEWLINE
                            "   CacheSize : %llu" OSS_NEWLINE
                            "  AllocCount : %llu" OSS_NEWLINE
                            "DeallocCount : %llu" OSS_NEWLINE
                            "  ShrinkSize : %llu" OSS_NEWLINE
                            "    HitCount : %llu (%.2f%%)" OSS_NEWLINE
                            "   PushCount : %llu (%.2f%%)" OSS_NEWLINE,
                            _blockSize, _cachedSize,
                            _allocCount, _deallocCount, _shrinkSize,
                            _hitCount, hitRatio,
                            _pushCount, pushRatio ) ;
      }

      return len ;
   }

   UINT64 _utilMemListItem::shrink( UINT64 expectSize )
   {
      UINT64 freedSize = 0 ;
      utilMemListNode *pNode = NULL ;

      while ( _header && freedSize < expectSize )
      {
         pNode = _header ;
         _header = pNode->_next ;
         utilPoolRelease( (void*&)pNode ) ;

         freedSize += _blockSize ;
#ifdef _DEBUG
         SDB_ASSERT( _cachedSize >= _blockSize, "Invalid cachedSize" ) ;
#endif //_DEBUG
         _cachedSize -= _blockSize ;
      }

      if ( freedSize > 0 && _pEvent )
      {
         _pEvent->onReleaseCache( freedSize ) ;
      }
      _shrinkSize += freedSize ;

      return freedSize ;
   }

   BOOLEAN _utilMemListItem::canShrink( UINT64 *pExpectSize ) const
   {
      if ( 0 == _cachedSize )
      {
         return FALSE ;
      }
      else if ( 0 == g_maxTCCacheSize )
      {
         if ( pExpectSize )
         {
            *pExpectSize = _cachedSize ;
         }
         return TRUE ;
      }
#ifdef SDB_ENGINE
      else if ( pmdGetTickSpanTime( _lastAllocTick ) >= UTIL_MEMLIST_FREE_TIMEOUT )
      {
         if ( pExpectSize )
         {
            *pExpectSize = _cachedSize >> 1 ;
         }
         return TRUE ;
      }
#endif // SDB_ENGINE

      return FALSE ;
   }

   /*
      _utilMemListPool implement
   */
   _utilMemListPool::_utilMemListPool()
   :_cachedSize( 0 )
   {
      _hasInit = FALSE ;
      _minType = _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;
      _maxType = _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;

      ossMemset( _name, 0, sizeof( _name ) ) ;
      SDB_ASSERT( sizeof( _arrayList ) / sizeof( utilMemListItem* ) ==
                  UTIL_MEM_POOL_LIST_NUM + 1,
                  "Invalid arrayList size" ) ;
      ossMemset( _arrayList, 0, sizeof( _arrayList ) ) ;

      _pEBB = NULL ;
      _EBBSize = 0 ;
      _allocEBBCount = 0 ;

#ifdef SDB_ENGINE
      _lastEBBTick = 0 ;
      _lastShrinkTick = pmdGetDBTick() ;
#endif // SDB_ENGINE

      _allocCount = 0 ;
      _reallocCount = 0 ;
      _deallocCount = 0 ;
      _hitCount = 0 ;
      _pushCount = 0 ;
      _copyCount = 0 ;
      _shrinkSize = 0 ;

      _outrangeAlloc = 0 ;
      _outrangeDealloc = 0 ;
      _outpoolDealloc = 0 ;
   }

   _utilMemListPool::~_utilMemListPool()
   {
      clear() ;
      fini() ;
   }

   void _utilMemListPool::fini()
   {
      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         if ( _arrayList[ i ] )
         {
            delete _arrayList[ i ] ;
            _arrayList[ i ] = NULL ;
         }
      }
      _hasInit = FALSE ;
      _minType = _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;
      _maxType = _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;
      _cachedSize = 0 ;
   }

   INT32 _utilMemListPool::init()
   {
      INT32 rc = SDB_OK ;
      UINT32 lastBlockSize = 0 ;
      UINT32 blockSize = 0 ;

      _utilMemBlockPool::MEMBLOCKPOOL_TYPE type =
         _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;

      if ( _hasInit )
      {
         goto done ;
      }

      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         blockSize = 1 << ( UTIL_MEM_LIST_BASE_EXPONENT + i ) ;
         type = _utilMemBlockPool::size2MemType( blockSize ) ;
         blockSize = _utilMemBlockPool::type2Size( type ) ;

         SDB_ASSERT( _utilMemBlockPool::MEMBLOCKPOOL_TYPE_DYN != type &&
                     _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX != type,
                     "Type Invalid" ) ;

         if ( _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX == _minType )
         {
            _minType = type ;
         }
         _maxType = type ;

         SDB_ASSERT( (INT32)type - _minType == (INT32)i,
                     "Type must increase by 1 step" ) ;
         SDB_ASSERT( lastBlockSize != blockSize,
                     "Last block size should not the same with current" ) ;

         lastBlockSize = blockSize ;

         _arrayList[ i ] = new ( std::nothrow ) utilMemListItem( blockSize,
                                                                 this ) ;
         if ( !_arrayList[ i ] )
         {
            PD_LOG( PDERROR, "Allocate utilMemListItem(BlockSize:%u) failed",
                    blockSize ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      _hasInit = TRUE ;

   done:
      return rc ;
   error:
      fini() ;
      goto done ;
   }

   void _utilMemListPool::setName( const CHAR *pName )
   {
      ossStrncpy( _name, pName, UTIL_MEM_THREAD_NAME_LEN ) ;
   }

   void _utilMemListPool::clearEBB()
   {
      if ( _pEBB )
      {
         onReleaseCache( _EBBSize ) ;

         SDB_POOL_FREE( _pEBB ) ;
         _pEBB = NULL ;
         _EBBSize = 0 ;
      }
   }

   void _utilMemListPool::clear()
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         if ( _arrayList[ i ] )
         {
            _arrayList[ i ]->clear() ;
         }
      }

      clearEBB() ;
      _allocEBBCount = 0 ;

#ifdef SDB_ENGINE
      _lastEBBTick = 0 ;
      _lastShrinkTick = pmdGetDBTick() ;
#endif // SDB_ENGINE

      _allocCount = 0 ;
      _reallocCount = 0 ;
      _deallocCount = 0 ;
      _hitCount = 0 ;
      _pushCount = 0 ;
      _copyCount = 0 ;
      _shrinkSize = 0 ;

      _outrangeAlloc = 0 ;
      _outrangeDealloc = 0 ;
      _outpoolDealloc = 0 ;
   }

   UINT64 _utilMemListPool::shrinkAll()
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      if ( 0 == _cachedSize )
      {
         return 0 ;
      }

#ifdef SDB_ENGINE
      _lastShrinkTick = pmdGetDBTick() ;
#endif // SDB_ENGINE

      utilMemListItem *pMemList = NULL ;
      UINT64 freeSize = 0 ;

      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         pMemList = _arrayList[ i ] ;
         freeSize += pMemList->shrinkAll() ;
      }

      /// EBB
      if ( _pEBB )
      {
         freeSize += _EBBSize ;
         clearEBB() ;
      }

      if ( freeSize > 0 )
      {
         PD_LOG( PDDEBUG, "MemList freed %llu space", freeSize ) ;
         _shrinkSize += freeSize ;
      }

      return freeSize ;
   }

   UINT64 _utilMemListPool::shrink()
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      if ( 0 == _cachedSize )
      {
         return 0 ;
      }
#ifdef SDB_ENGINE
      else if ( pmdGetTickSpanTime( _lastShrinkTick ) < UTIL_TC_SHRINK_INTERVAL )
      {
         return 0 ;
      }
      _lastShrinkTick = pmdGetDBTick() ;
#endif // SDB_ENGINE

      utilMemListItem *pMemList = NULL ;
      UINT64 expectSize = 0 ;
      UINT64 freeSize = 0 ;

      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         pMemList = _arrayList[ i ] ;

         if ( pMemList->canShrink( &expectSize ) )
         {
            freeSize += pMemList->shrink( expectSize ) ;
         }
      }

      /// EBB
      if ( _pEBB && ( 0 == g_maxTCCacheSize
#ifdef SDB_ENGINE
         || pmdGetTickSpanTime( _lastEBBTick ) >=
            UTIL_MEMLIST_FREE_TIMEOUT
#endif // SDB_ENGINE
          ) )
      {
         freeSize += _EBBSize ;
         clearEBB() ;
      }

      if ( freeSize > 0 )
      {
         PD_LOG( PDDEBUG, "MemList freed %llu space", freeSize ) ;
         _shrinkSize += freeSize ;
      }

      return freeSize ;
   }

   void _utilMemListPool::onAllocCache( UINT32 blockSize )
   {
      _cachedSize -= blockSize ;
      ++_hitCount ;
   }

   void _utilMemListPool::onPushedCache( UINT32 blockSize )
   {
      _cachedSize += blockSize ;
      ++_pushCount ;
   }

   void _utilMemListPool::onOutOfCache( UINT32 blockSize,
                                        UINT64 selfCacheSize )
   {
      /// do nothing
   }

   void _utilMemListPool::onReleaseCache( UINT64 size )
   {
      _cachedSize -= size ;
   }

   BOOLEAN _utilMemListPool::canAllocBlockSize( UINT32 size, UINT32 blockSize )
   {
      if ( blockSize <= UTIL_MEM_ELEMENT_256 )
      {
         return TRUE ;
      }
      else if ( _cachedSize + blockSize < g_maxTCCacheSize )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _utilMemListPool::canCacheBlock( UINT32 blockSize )
   {
      if ( _cachedSize + blockSize <= g_maxTCCacheSize )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   void* _utilMemListPool::allocFromEBB( UINT32 size,
                                         const CHAR *pFile,
                                         UINT32 line,
                                         UINT32 *pRealSize,
                                         const CHAR *pInfo )
   {
      void *p = NULL ;

      if ( _EBBSize >= size )
      {
         p = ( void* )_pEBB ;

         if ( pRealSize )
         {
            *pRealSize = _EBBSize ;
         }

         /// set tc inuse flag
         *UTIL_MEM_PTR_FLAG_PTR( UTIL_MEM_USERPTR_2_PTR(p) ) = UTIL_MEM_FLAG_TC_INUSE ;

         onAllocCache( _EBBSize ) ;

         if ( ossMemDebugEnabled )
         {
            ossThreadMemTrack( p, size, ossHashFileName( pFile ), line, pInfo ) ;
         }

         ++_allocEBBCount ;
#ifdef SDB_ENGINE
         _lastEBBTick = pmdGetDBTick() ;
#endif // SDB_ENGINE
         _EBBSize = 0 ;
         _pEBB = NULL ;
      }

      return p ;
   }

   void* _utilMemListPool::alloc( UINT32 size,
                                  const CHAR *pFile,
                                  UINT32 line,
                                  UINT32 *pRealSize,
                                  const CHAR *pInfo )
   {
      void *ptr = NULL ;
      INT32 index = 0 ;
      INT32 type = 0 ;

      if ( 0 == size )
      {
         if ( pRealSize )
         {
            *pRealSize = 0 ;
         }
         goto done ;
      }

      ++_allocCount ;

      /// calc type
      type = _utilMemBlockPool::size2MemType( size + UTIL_MEM_TOTAL_FILL_LEN ) ;
      if ( type < _minType || type > _maxType )
      {
         if ( size + UTIL_MEM_TOTAL_FILL_LEN <=
              _arrayList[0]->getBlockSize() )
         {
            index = 0 ;
         }
         else
         {
            index = UTIL_MEM_POOL_LIST_NUM + 1 ;
         }
      }
      else
      {
         index = type - _minType ;
      }

      if ( index >= 0 && index < UTIL_MEM_POOL_LIST_NUM )
      {
         if ( _arrayList[ index ]->getCacheSize() >= size )
         {
            ptr = _arrayList[ index ]->alloc( size, pFile, line, pRealSize, pInfo ) ;
         }
         else if ( _arrayList[ index + 1 ] &&
                   _arrayList[ index + 1 ]->getCacheSize() >= size )
         {
            ptr = _arrayList[ index + 1 ]->alloc( size, pFile, line, pRealSize, pInfo ) ;
         }
         else
         {
            ptr = _arrayList[ index ]->alloc( size, pFile, line, pRealSize, pInfo ) ;
         }
      }
      else if ( size >= UTIL_MIN_EBB_SIZE &&
                size <= UTIL_MAX_EBB_SIZE &&
                ( NULL != ( ptr = allocFromEBB( size, pFile,
                                                line, pRealSize, pInfo ) ) ) )
      {
         /// do nothing
      }
      else
      {
         ++_outrangeAlloc ;
         ptr = utilPoolAlloc( size, pFile, line, pRealSize, pInfo ) ;
      }

   done:
      return ptr ;
   }

   void* _utilMemListPool::realloc( void *ptr, UINT32 size,
                                    const CHAR *pFile,
                                    UINT32 line,
                                    UINT32 *pRealSize,
                                    const CHAR *pInfo )
   {
      void *pNewPtr = NULL ;
      UINT32 oldSize = 0 ;
      UINT16 type = _utilMemBlockPool::MEMBLOCKPOOL_TYPE_MAX ;

      if ( 0 == size )
      {
         pNewPtr = ptr ;
         goto done ;
      }
      else if ( !ptr )
      {
         pNewPtr = alloc( size, pFile, line, pRealSize, pInfo ) ;
         goto done ;
      }

      ++_reallocCount ;

      if ( utilPoolPtrCheck( ptr, &oldSize, &type ) )
      {
         if ( oldSize >= size )
         {
            pNewPtr = ptr ;
            if ( pRealSize )
            {
               *pRealSize = oldSize ;
            }
            goto done ;
         }

         if ( _utilMemBlockPool::MEMBLOCKPOOL_TYPE_DYN != type )
         {
            pNewPtr = alloc( size, pFile, line, pRealSize, pInfo ) ;
            if ( pNewPtr )
            {
               ++_copyCount ;
               /// copy
               ossMemcpy( pNewPtr, ptr, oldSize ) ;
               /// release old
               release( ptr ) ;
            }
            goto done ;
         }
      }

      if ( ossMemDebugEnabled )
      {
         ossThreadMemUnTrack( ptr ) ;
      }

      pNewPtr = utilPoolRealloc( ptr, size, pFile, line, pRealSize, pInfo ) ;

   done:
      return pNewPtr ;
   }

   void _utilMemListPool::release2EBB( void *&p, UINT32 size )
   {
      if ( ossMemDebugEnabled )
      {
         ossThreadMemUnTrack( p ) ;
      }

      if ( size >= UTIL_MIN_EBB_SIZE &&
           size <= UTIL_MAX_EBB_SIZE &&
           size > _EBBSize &&
           canCacheBlock( size - _EBBSize ) )
      {
         clearEBB() ;

         /// set tc nouse flag
         *UTIL_MEM_PTR_FLAG_PTR( UTIL_MEM_USERPTR_2_PTR(p) ) = UTIL_MEM_FLAG_TC_NOUSE ;

         _pEBB = (CHAR*)p ;
         _EBBSize = size ;

         onPushedCache( _EBBSize ) ;

         p = NULL ;
      }
      else
      {
         if ( size + UTIL_MEM_TOTAL_FILL_LEN <=
              _utilMemBlockPool::type2Size( (_utilMemBlockPool::MEMBLOCKPOOL_TYPE)_maxType ) )
         {
            ++_outpoolDealloc ;
         }
         else
         {
            ++_outrangeDealloc ;
         }
         SDB_POOL_FREE( p ) ;
      }
   }

   void _utilMemListPool::release( void *& ptr )
   {
      if ( !ptr ) return ;

      UINT32 oldSize = 0 ;
      UINT16 type = 0 ;

      ++_deallocCount ;

      if ( utilPoolPtrCheck( ptr, &oldSize, &type ) )
      {
         if ( type >= (UINT16)_minType &&
              type <= (UINT16)_maxType )
         {
            _arrayList[ type - (UINT16)_minType ]->dealloc( ptr ) ;
            ptr = NULL ;
         }
         else
         {
            release2EBB( ptr, oldSize ) ;
         }
      }
      else
      {
         ++_outrangeDealloc ;
         utilPoolRelease( ptr ) ;
      }
   }

   UINT32 _utilMemListPool::dump( CHAR *pBuff, UINT32 buffSize )
   {
      UINT32 len = 0 ;

      FLOAT64 hitRatio = 0.0 ;
      FLOAT64 pushRatio = 0.0 ;
      FLOAT64 copyRatio = 0.0 ;

      if ( _allocCount > 0 )
      {
         hitRatio = ( FLOAT64 )_hitCount / _allocCount * 100 ;
      }
      if ( _deallocCount > 0 )
      {
         pushRatio = ( FLOAT64 )_pushCount / _deallocCount * 100 ;
      }
      if ( _reallocCount > 0 )
      {
         copyRatio = ( FLOAT64 )_copyCount / _reallocCount * 100 ;
      }

      /// dump self
      len = ossSnprintf( pBuff, buffSize,
                         "   CacheSize : %llu" OSS_NEWLINE
                         "    EBB Size : %u" OSS_NEWLINE
                         "  AllocCount : %llu" OSS_NEWLINE
                         "   OOR Alloc : %llu" OSS_NEWLINE
                         "   EBB Alloc : %llu" OSS_NEWLINE
                         "ReallocCount : %llu" OSS_NEWLINE
                         "DeallocCount : %llu" OSS_NEWLINE
                         " OOR Dealloc : %llu" OSS_NEWLINE
                         " OOP Dealloc : %llu" OSS_NEWLINE
                         "  ShrinkSize : %llu" OSS_NEWLINE
                         "    HitCount : %llu (%.2f%%)" OSS_NEWLINE
                         "   PushCount : %llu (%.2f%%)" OSS_NEWLINE
                         "   CopyCount : %llu (%.2f%%)" OSS_NEWLINE,
                         _cachedSize,
                         _EBBSize,
                         _allocCount, _outrangeAlloc, _allocEBBCount,
                         _reallocCount, _deallocCount, _outrangeDealloc,
                         _outpoolDealloc, _shrinkSize,
                         _hitCount, hitRatio,
                         _pushCount, pushRatio,
                         _copyCount, copyRatio ) ;

      if ( len >= buffSize )
      {
         goto done ;
      }

      /// dump item
      for ( UINT32 i = 0 ; i < UTIL_MEM_POOL_LIST_NUM ; ++i )
      {
         if ( _arrayList[ i ] )
         {
            len += _arrayList[ i ]->dump( pBuff + len, buffSize - len ) ;
            if ( len >= buffSize )
            {
               break ;
            }
         }
      }

   done:
      return len ;
   }

   /*
      Callback Funcs
   */
   void* utilTCMemRealPtr( void *p )
   {
      return (void*)UTIL_MEM_USERPTR_2_PTR( p ) ;
   }

   class _utilTCCallbackAssit
   {
      public:
         _utilTCCallbackAssit()
         {
            ossSetTCMemRealPtrFunc( (OSS_TC_MEMREALPTR_FUNC)utilTCMemRealPtr ) ;
         }
   } ;
   _utilTCCallbackAssit s_assitTCCallbak ;

   /*
      Global implement
   */

   static OSS_THREAD_LOCAL utilMemListPool   *g_thdMemPool = NULL ;

   void utilSetThreadMemPool( utilMemListPool *pPool )
   {
      if ( NULL == g_thdMemPool )
      {
         g_thdMemPool = pPool ;
      }
      else if ( NULL == pPool )
      {
         SDB_ASSERT( g_thdMemPool->getCacheSize() == 0,
                     "Total cache size must be 0" ) ;
         g_thdMemPool = pPool ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Mempool is already valid" ) ;
      }
   }

   UINT64 utilShrinkThreadMemPool( BOOLEAN forced )
   {
      if ( g_thdMemPool )
      {
         if ( forced )
         {
            return g_thdMemPool->shrinkAll() ;
         }
         else
         {
            return g_thdMemPool->shrink() ;
         }
      }

      return 0 ;
   }

   void utilClearThreadMemPool()
   {
      if ( g_thdMemPool )
      {
         g_thdMemPool->clear() ;
      }
   }

   void utilDumpThreadMemPoolInfo( const CHAR *pType,
                                   const CHAR *pName )
   {
      if ( g_thdMemPool )
      {
         CHAR buff[ UTIL_DUMP_BUFFSIZE ] = { 0 } ;
         g_thdMemPool->dump( buff, UTIL_DUMP_BUFFSIZE - 1 ) ;

         PD_LOG( PDEVENT, "Dump thread memory info:" OSS_NEWLINE
                          " Thread Type : %s" OSS_NEWLINE
                          " Thread Name : %s" OSS_NEWLINE
                          "%s",
                 pType, pName, buff ) ;
      }
   }

   void utilDumpThreadMemBegin( const CHAR *pPath )
   {
      UINT32 len = 0 ;
      CHAR buff[ 256 ] = { 0 } ;
      CHAR timebuff[ UTIL_MEM_TRACEDUMP_TM_BUF ] = { 0 } ;
      ossTimestamp current ;

      ossGetCurrentTime( current ) ;
      ossTimestampToString( current, timebuff ) ;

      len = ossSnprintf( buff, sizeof( buff ),
                         OSS_NEWLINE OSS_NEWLINE
                         "====> Dump threads memory status( %s ) ====>"
                         OSS_NEWLINE,
                         timebuff ) ;

      utilDumpInfo2File( pPath, UTIL_THREAD_MEM_STAT_FILE, buff, len ) ;
   }

   void utilDumpThreadMemPoolInfo( const CHAR *pPath )
   {
      if ( g_thdMemPool )
      {
         CHAR buff[ UTIL_DUMP_BUFFSIZE ] = { 0 } ;
         UINT32 len = 0 ;

         g_thdMemPool->shrink() ;

         len = ossSnprintf( buff, UTIL_DUMP_BUFFSIZE,
                            OSS_NEWLINE
                            "---- Thread( ID: %u, Name: %s ) ----" OSS_NEWLINE,
                            ossGetCurrentThreadID(),
                            g_thdMemPool->getName() ) ;
         len += g_thdMemPool->dump( buff + len, UTIL_DUMP_BUFFSIZE - len ) ;

         utilDumpInfo2File( pPath, UTIL_THREAD_MEM_STAT_FILE, buff, len ) ;
      }
   }

   UINT32 utilThreadMemPoolSize()
   {
      if ( g_thdMemPool )
      {
         return g_thdMemPool->getCacheSize() ;
      }
      return 0 ;
   }

   void* utilThreadAlloc( UINT32 size,
                          const CHAR *pFile,
                          UINT32 line,
                          UINT32 *pRealSize,
                          const CHAR *pInfo )
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      if ( g_thdMemPool )
      {
         return g_thdMemPool->alloc( size, pFile, line, pRealSize, pInfo ) ;
      }
      return utilPoolAlloc( size, pFile, line, pRealSize, pInfo ) ;
   }

   void* utilThreadRealloc( void* ptr, UINT32 size,
                            const CHAR *pFile, UINT32 line,
                            UINT32 *pRealSize,
                            const CHAR *pInfo )
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      if ( g_thdMemPool )
      {
         return g_thdMemPool->realloc( ptr, size, pFile, line, pRealSize, pInfo ) ;
      }
      else
      {
         if ( ossMemDebugEnabled && ptr )
         {
            ossThreadMemUnTrack( ptr ) ;
         }
         return utilPoolRealloc( ptr, size, pFile, line, pRealSize, pInfo ) ;
      }
   }

   void utilThreadRelease( void*& ptr )
   {
      ossSignalShield shield ;
      shield.doNothing() ;

      if ( g_thdMemPool )
      {
         g_thdMemPool->release( ptr ) ;
      }
      else
      {
         if ( ossMemDebugEnabled && ptr )
         {
            ossThreadMemUnTrack( ptr ) ;
         }
         utilPoolRelease( ptr ) ;
      }
   }

   extern "C"
   {
      void* utilTCAlloc( UINT32 size )
      {
         return utilThreadAlloc( size, __FILE__, __LINE__ ) ;
      }

      void* utilTCRealloc( void * ptr, UINT32 size )
      {
         return utilThreadRealloc( ptr, size, __FILE__, __LINE__ ) ;
      }

      void utilTCRelease( void * ptr )
      {
         utilThreadRelease( (void*&)ptr ) ;
      }
   }

}

