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

extern BOOLEAN ossMemDebugEnabled ;

namespace engine
{

   #define UTIL_MEM_ALLOC_MAX_TRY_LEVEL         ( 2 )

   #define UTIL_POOL_MEM_STAT_FILE              ".mempoolstat"
   #define UTIL_MEMPOOL_TRACEDUMP_TM_BUF        64
   #define UTIL_MEMPOOL_DUMP_BUFFSIZE           ( 65536 )

   /*
      _utilMemBlockPool implement
   */
   _utilMemBlockPool::_utilMemBlockPool( BOOLEAN isGlobal )
   :_isGlobal( isGlobal ),
    _maxSize( 0 ),
    _totalSize( 0 ),
    _oorTimes( 0 )
   {
      _32BSeg = NULL ;
      _64BSeg = NULL ;
      _128BSeg = NULL ;
      _256BSeg = NULL ;
      _512BSeg = NULL ;
      _1KSeg = NULL ;
      _2KSeg = NULL ;
      _4KSeg = NULL ;
      _8KSeg = NULL ;

      _clearStat() ;
   }

   _utilMemBlockPool::~_utilMemBlockPool()
   {
      fini() ;
   }

   void _utilMemBlockPool::setMaxSize( UINT64 maxSize )
   {
      _maxSize = maxSize ;
      UINT64 aBlockMaxSize = _maxSize >> 3 ;

      if ( _32BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_32 ) ;
         _32BSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _64BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_64 ) ;
         _64BSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _128BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_128 ) ;
         _128BSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }
   
      if ( _256BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_256 ) ;
         _256BSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _512BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_512 ) ;
         _512BSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _1KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_1024 ) ;
         _1KSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _2KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_2048 ) ;
         _2KSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _4KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_4096 ) ;
         _4KSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }

      if ( _8KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_8192 ) ;
         _8KSeg->setMaxObjects( aBlockMaxSize / typeSize ) ;
      }
   }

   INT32 _utilMemBlockPool::init( UINT64 maxSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 smallBlockSize = UTIL_MEM_A_SMALL_BLOCK_SIZE *
                              UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM ;
      UINT32 midBlockSize = UTIL_MEM_A_MID_BLOCK_SIZE *
                            UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM ;
      UINT32 bigBlockSize = UTIL_MEM_A_BIG_BLOCK_SIZE *
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM ;
      UINT64 aBlockMaxSize = 0 ;

      _maxSize = maxSize ;

      aBlockMaxSize = _maxSize >> 3 ;

      /// when alloc or init failed, ignored
      _32BSeg = SDB_OSS_NEW _utilSegmentManager<element32B>( "32B" ) ;
      if ( _32BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_32 ) ;
         rc = _32BSeg->init( smallBlockSize / typeSize,
                             aBlockMaxSize / typeSize,
                             UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                             this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _32BSeg ;
            _32BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _64BSeg = SDB_OSS_NEW _utilSegmentManager<element64B>( "64B" ) ;
      if ( _64BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_64 ) ;
         rc = _64BSeg->init( smallBlockSize / typeSize,
                             aBlockMaxSize / typeSize,
                             UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                             this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _64BSeg ;
            _64BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _128BSeg = SDB_OSS_NEW _utilSegmentManager<element128B>( "128B" ) ;
      if ( _128BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_128 ) ;
         rc = _128BSeg->init( smallBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _128BSeg ;
            _128BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _256BSeg = SDB_OSS_NEW _utilSegmentManager<element256B>( "256B" ) ;
      if ( _256BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_256 ) ;
         rc = _256BSeg->init( midBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _256BSeg ;
            _256BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _512BSeg = SDB_OSS_NEW _utilSegmentManager<element512B>( "512B" ) ;
      if ( _512BSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_512 ) ;
         rc = _512BSeg->init( midBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _512BSeg ;
            _512BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _1KSeg = SDB_OSS_NEW _utilSegmentManager<element1K>( "1KB" ) ;
      if ( _1KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_1024 ) ;
         rc = _1KSeg->init( midBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _1KSeg ;
            _1KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _2KSeg = SDB_OSS_NEW _utilSegmentManager<element2K>( "2KB" ) ;
      if ( _2KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_2048 ) ;
         rc = _2KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _2KSeg ;
            _2KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _4KSeg = SDB_OSS_NEW _utilSegmentManager<element4K>( "4KB" ) ;
      if ( _4KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_4096 ) ;
         rc = _4KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _4KSeg ;
            _4KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _8KSeg = SDB_OSS_NEW _utilSegmentManager<element8K>( "8KB" ) ;
      if ( _8KSeg )
      {
         UINT32 typeSize = type2Size( MEMBLOCKPOOL_TYPE_8192 ) ;
         rc = _8KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _8KSeg ;
            _8KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      return rc ;
   }

   void _utilMemBlockPool::fini()
   {
      if ( _32BSeg )
      {
         SDB_OSS_DEL _32BSeg ;
         _32BSeg = NULL ;
      }
      if ( _64BSeg )
      {
         SDB_OSS_DEL _64BSeg ;
         _64BSeg = NULL ;
      }
      if ( _128BSeg )
      {
         SDB_OSS_DEL _128BSeg ;
         _128BSeg = NULL ;
      }
      if ( _256BSeg )
      {
         SDB_OSS_DEL _256BSeg ;
         _256BSeg = NULL ;
      }
      if ( _512BSeg )
      {
         SDB_OSS_DEL _512BSeg ;
         _512BSeg = NULL ;
      }
      if ( _1KSeg )
      {
         SDB_OSS_DEL _1KSeg ;
         _1KSeg = NULL ;
      }
      if ( _2KSeg )
      {
         SDB_OSS_DEL _2KSeg ;
         _2KSeg = NULL ;
      }
      if ( _4KSeg )
      {
         SDB_OSS_DEL _4KSeg ;
         _4KSeg = NULL ;
      }
      if ( _8KSeg )
      {
         SDB_OSS_DEL _8KSeg ;
         _8KSeg = NULL ;
      }
   }

   void _utilMemBlockPool::shrink()
   {
      UINT64 hasFreeSize = 0 ;
      UINT64 tmpFreedSize = 0 ;

      BOOLEAN isFull = FALSE ;
      if ( 0 == _maxSize || _totalSize.fetch() >= _maxSize )
      {
         isFull = TRUE ;
      }

      if ( _32BSeg )
      {
         _32BSeg->shrink( ( isFull ? 0 : 1 ), &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _64BSeg )
      {
         _64BSeg->shrink( ( isFull? 0 : 1 ), &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _128BSeg )
      {
         _128BSeg->shrink( ( isFull ? 0 : 1 ), &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _256BSeg )
      {
         _256BSeg->shrink( ( isFull ? 0 : 1 ), &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _512BSeg )
      {
         _512BSeg->shrink( ( isFull ? 0 : 1 ), &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _1KSeg )
      {
         _1KSeg->shrink( 0, &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _2KSeg )
      {
         _2KSeg->shrink( 0, &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _4KSeg )
      {
         _4KSeg->shrink( 0, &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }
      if ( _8KSeg )
      {
         _8KSeg->shrink( 0, &tmpFreedSize ) ;
         hasFreeSize += tmpFreedSize ;
      }

      if ( hasFreeSize > 0 )
      {
         PD_LOG( PDINFO, "Has freed %llu bytes pooled memory, Total:%llu",
                 hasFreeSize, getTotalSize() ) ;
      }
   }

   UINT64 _utilMemBlockPool::getTotalSize()
   {
      return _totalSize.fetch() ;
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
         default :
            break ;
      }
      return size ;
   }

   void* _utilMemBlockPool::alloc( UINT32 size,
                                   const CHAR *pFile,
                                   UINT32 line,
                                   UINT32 *pRealSize )
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

      if ( _maxSize > 0 )
      {
         switch ( type )
         {
            case MEMBLOCKPOOL_TYPE_32 :
               if ( _32BSeg && SDB_OK == _32BSeg->acquire( (element32B*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_32 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_64 :
               if ( _64BSeg && SDB_OK == _64BSeg->acquire( (element64B*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_64 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_128 :
               if ( _128BSeg && SDB_OK == _128BSeg->acquire( (element128B*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_128 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_256 :
               if ( _256BSeg && SDB_OK == _256BSeg->acquire( (element256B*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_256 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_512 :
               if ( _512BSeg && SDB_OK == _512BSeg->acquire( (element512B*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_512 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_1024 :
               if ( _1KSeg && SDB_OK == _1KSeg->acquire( (element1K*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_1024 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_2048 :
               if ( _2KSeg && SDB_OK == _2KSeg->acquire( (element2K*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_2048 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_4096 :
               if ( _4KSeg && SDB_OK == _4KSeg->acquire( (element4K*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_4096 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            case MEMBLOCKPOOL_TYPE_8192 :
               if ( _8KSeg && SDB_OK == _8KSeg->acquire( (element8K*&)ptr ) )
               {
                  realType = MEMBLOCKPOOL_TYPE_8192 ;
                  break ;
               }
               if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
               {
                  break ;
               }
               /// don't break
            default :
               if ( realSize > MEMBLOCKPOOL_TYPE_8192 )
               {
                  _oorTimes.inc() ;
               }
               break ;
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
         ossPoolMemTrack( (void*)ptr, size, ossHashFileName( pFile ), line ) ;
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
      *UTIL_MEM_PTR_TYPE_PTR( ptr ) = type ;
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
         UINT16 type = *UTIL_MEM_PTR_TYPE_PTR( ptr ) ;
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
                                     UINT32 *pRealSize )
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

      newUserPtr = alloc( size, pFile, line, pRealSize ) ;
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
      CHAR *realPtr  = NULL ;

      if ( NULL == ptr )
      {
         goto done ;
      }

      realPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
      if ( !_checkAndExtract( realPtr, NULL, &type ) )
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

      switch ( type ) 
      {
         case MEMBLOCKPOOL_TYPE_32 :
            rc = _32BSeg->release( (element32B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_64 :
            rc = _64BSeg->release( (element64B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_128:
            rc = _128BSeg->release( (element128B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_256:
            rc = _256BSeg->release( (element256B *)realPtr) ;
            break ;
         case MEMBLOCKPOOL_TYPE_512:
            rc = _512BSeg->release( (element512B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_1024:
            rc = _1KSeg->release( (element1K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_2048:
            rc = _2KSeg->release( (element2K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_4096:
            rc = _4KSeg->release( (element4K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_8192:
            rc = _8KSeg->release( (element8K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_DYN :
            SDB_OSS_FREE( realPtr ) ;
            break ;
         default:
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid type (%d)", type ) ;
            SDB_ASSERT( FALSE, "Invalid arguments" ) ;
            break ;
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

   void _utilMemBlockPool::onAllocSegment( UINT64 size )
   {
      _totalSize.add( size ) ;
   }

   void _utilMemBlockPool::onReleaseSegment( UINT64 size )
   {
      _totalSize.sub( size ) ;
   }

   BOOLEAN _utilMemBlockPool::canShrink( UINT32 blockSize,
                                         UINT64 totalSize,
                                         UINT64 usedSize )
   {
      if ( 0 == _maxSize || _totalSize.fetch() >= _maxSize )
      {
         return TRUE ;
      }
      else if ( totalSize > 0 )
      {
         FLOAT64 ratio = (FLOAT64)usedSize / totalSize ;
         if ( ratio < UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD )
         {
            return TRUE ;
         }
      }
      return FALSE ;
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

      len = ossSnprintf( pBuff, buffLen,
                         "      Max Size : %llu"OSS_NEWLINE
                         "    Total Size : %llu"OSS_NEWLINE,
                         _maxSize,
                         _totalSize.fetch() ) ;

      if ( _32BSeg )
      {
         len += _32BSeg->dump( pBuff + len, buffLen - len,
                               &acquireTimes,
                               &releaseTimes,
                               &oomTimes,
                               &oolTimes,
                               &shrinkSize ) ;
      }
      if ( _64BSeg )
      {
         len += _64BSeg->dump( pBuff + len, buffLen - len,
                               &acquireTimes,
                               &releaseTimes,
                               &oomTimes,
                               &oolTimes,
                               &shrinkSize ) ;
      }
      if ( _128BSeg )
      {
         len += _128BSeg->dump( pBuff + len, buffLen - len,
                                &acquireTimes,
                                &releaseTimes,
                                &oomTimes,
                                &oolTimes,
                                &shrinkSize ) ;
      }
      if ( _256BSeg )
      {
         len += _256BSeg->dump( pBuff + len, buffLen - len,
                                &acquireTimes,
                                &releaseTimes,
                                &oomTimes,
                                &oolTimes,
                                &shrinkSize ) ;
      }
      if ( _512BSeg )
      {
         len += _512BSeg->dump( pBuff + len, buffLen - len,
                                &acquireTimes,
                                &releaseTimes,
                                &oomTimes,
                                &oolTimes,
                                &shrinkSize ) ;
      }
      if ( _1KSeg )
      {
         len += _1KSeg->dump( pBuff + len, buffLen - len,
                              &acquireTimes,
                              &releaseTimes,
                              &oomTimes,
                              &oolTimes,
                              &shrinkSize ) ;
      }
      if ( _2KSeg )
      {
         len += _2KSeg->dump( pBuff + len, buffLen - len,
                              &acquireTimes,
                              &releaseTimes,
                              &oomTimes,
                              &oolTimes,
                              &shrinkSize ) ;
      }
      if ( _4KSeg )
      {
         len += _4KSeg->dump( pBuff + len, buffLen - len,
                              &acquireTimes,
                              &releaseTimes,
                              &oomTimes,
                              &oolTimes,
                              &shrinkSize ) ;
      }
      if ( _8KSeg )
      {
         len += _8KSeg->dump( pBuff + len, buffLen - len,
                              &acquireTimes,
                              &releaseTimes,
                              &oomTimes,
                              &oolTimes,
                              &shrinkSize ) ;
      }

      len += ossSnprintf( pBuff + len, buffLen - len,
                          OSS_NEWLINE
                          "Pool Memory Stat"OSS_NEWLINE
                          " Acquire Times : %llu (Inc: %lld )"OSS_NEWLINE
                          " Release Times : %llu (Inc: %lld )"OSS_NEWLINE
                          "     OOM Times : %llu (Inc: %lld )"OSS_NEWLINE
                          "     OOL Times : %llu (Inc: %lld )"OSS_NEWLINE
                          "     OOR Times : %llu (Inc: %lld )"OSS_NEWLINE
                          "   Shrink Size : %llu (Inc: %lld )"OSS_NEWLINE,
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

   void utilGetPoolMemInfo( void *p, UINT64 &size, INT32 &pool, INT32 &index )
   {
      size = 0 ;
      pool = -1 ;
      index = -1 ;

      if ( p )
      {
         size = (UINT64)*UTIL_MEM_PTR_SIZE_PTR( p ) ;

         UINT16 type = *UTIL_MEM_PTR_TYPE_PTR( p ) ;
         if ( _utilMemBlockPool::MEMBLOCKPOOL_TYPE_DYN != type )
         {
            _utilSegmentPool<UINT64>::_objX *pObjX =
               ( _utilSegmentPool<UINT64>::_objX* )_GET_OBJX_ADDRESS( p ) ;
            pool = _GET_UNPACKED_POOLID( pObjX->_index ) ;
            index = pObjX->_index & _SEGMENT_OBJ_INDEX_MASK ;
         }
      }
   }

   class _utilPoolCallbackAssit
   {
      public:
         _utilPoolCallbackAssit()
         {
            ossSetPoolMemcheckFunc( (OSS_POOL_MEMCHECK_FUNC)utilPoolMemCheck ) ;
            ossSetPoolMemInfoFunc( (OSS_POOL_MEMINFO_FUNC)utilGetPoolMemInfo ) ;
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
                        UINT32 *pRealSize )
   {
      return g_memPool.alloc( size, pFile, line, pRealSize ) ;
   }

   void* utilPoolRealloc( void* ptr,
                          UINT32 size,
                          const CHAR *pFile,
                          UINT32 line,
                          UINT32 *pRealSize )
   {
      return g_memPool.realloc( ptr, size, pFile, line, pRealSize ) ;
   }

   void utilPoolRelease( void*& ptr )
   {
      g_memPool.release( ptr ) ;
   }

}


