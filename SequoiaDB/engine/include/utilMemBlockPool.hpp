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

   Source File Name = utilMemBlockPool.hpp

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

#ifndef UTIL_MEM_BLOCK_POOL_HPP__
#define UTIL_MEM_BLOCK_POOL_HPP__

#include "utilSegment.hpp"

namespace engine
{

   #define UTIL_MEM_BLOCK_POOL_DFT_MAX_SZ          ( 8589934592LL )  ///8GB
   #define UTIL_MEM_SEG_BLOCK_SIZE                 ( 2105344 )       ///2MB+8KB
   #define UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM      ( 8 )
   #define UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM        ( 4 )
   #define UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM        ( 2 )

   #define UTIL_MEM_64K_BLOCK_SUBPOOL_NUM          ( 4 )

   /// Memory info:
   /// | B-Eye(2) | Size(4) | Type(1) | Flag(1) | User Data | E-Eye(2) |

   #define UTIL_MEM_B_EYE_CHAR         0xBE38
   #define UTIL_MEM_E_EYE_CHAR         0xAC52

   #define UTIL_MEM_FLAG_NORMAL        0x0F
   #define UTIL_MEM_FLAG_NOUSE         0xF0
   #define UTIL_MEM_FLAG_TC_INUSE      0x0A
   #define UTIL_MEM_FLAG_TC_NOUSE      0xA0

   #define UTIL_MEM_B_EYE_LEN          sizeof(UINT16)
   #define UTIL_MEM_SIZE_LEN           sizeof(UINT32)
   #define UTIL_MEM_TYPE_LEN           sizeof(UINT8)
   #define UTIL_MEM_FLAG_LEN           sizeof(UINT8)
   #define UTIL_MEM_E_EYE_LEN          sizeof(UINT16)

   #define UTIL_MEM_HEAD_FILL_LEN   \
      ( UTIL_MEM_B_EYE_LEN + UTIL_MEM_SIZE_LEN + UTIL_MEM_TYPE_LEN + UTIL_MEM_FLAG_LEN )

   #define UTIL_MEM_TAIL_FILL_LEN      ( UTIL_MEM_E_EYE_LEN )

   #define UTIL_MEM_TOTAL_FILL_LEN  \
      ( UTIL_MEM_HEAD_FILL_LEN + UTIL_MEM_TAIL_FILL_LEN )

   #define UTIL_MEM_SIZE_2_REALSIZE(sz) \
         ( (UINT32)sz + UTIL_MEM_TOTAL_FILL_LEN )

   #define UTIL_MEM_PTR_2_USERPTR(ptr) \
         ( (CHAR*)(ptr) + UTIL_MEM_HEAD_FILL_LEN )

   #define UTIL_MEM_USERPTR_2_PTR(userPtr) \
         ( (CHAR*)(userPtr) - UTIL_MEM_HEAD_FILL_LEN )

   #define UTIL_MEM_PTR_B_EYE_PTR(ptr) \
      ( (UINT16*)(CHAR*)(ptr) )

   #define UTIL_MEM_PTR_SIZE_PTR(ptr)  \
      ( (UINT32*)((CHAR*)UTIL_MEM_PTR_B_EYE_PTR(ptr)+UTIL_MEM_B_EYE_LEN) )

   #define UTIL_MEM_PTR_TYPE_PTR(ptr)  \
      ( (UINT8*)((CHAR*)UTIL_MEM_PTR_SIZE_PTR(ptr)+UTIL_MEM_SIZE_LEN) )

   #define UTIL_MEM_PTR_FLAG_PTR(ptr)  \
      ( (UINT8*)((CHAR*)UTIL_MEM_PTR_TYPE_PTR(ptr)+UTIL_MEM_TYPE_LEN) )

   #define UTIL_MEM_PTR_E_EYE_PTR(ptr, sz) \
      ( (UINT16*)( (CHAR*)(ptr) + (UINT32)(sz) - UTIL_MEM_E_EYE_LEN ) )

   #define UTIL_MEM_OVERFLOW_SZ                 ( UTIL_MEM_TOTAL_FILL_LEN + 22 )
   /*
      Define element size
   */
   #define UTIL_MEM_ELEMENT_32                  ( 32 )
   #define UTIL_MEM_ELEMENT_64                  ( 64 )
   #define UTIL_MEM_ELEMENT_128                 ( 128 )
   #define UTIL_MEM_ELEMENT_256                 ( 256 )
   #define UTIL_MEM_ELEMENT_512                 ( 512 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_1024                ( 1024 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_2048                ( 2048 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_4096                ( 4096 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_8192                ( 8192 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_16K                 ( 16384 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_32K                 ( 32768 + UTIL_MEM_OVERFLOW_SZ )
   #define UTIL_MEM_ELEMENT_64K                 ( 65536 + UTIL_MEM_OVERFLOW_SZ )

   #define UTIL_MEM_OOL_PRINT_INTERVAL          ( 600000 )  /// 10 mins
   #define UTIL_MEM_OOL_PRINT_STEP              ( 5 * UTIL_MEM_OOL_PRINT_INTERVAL )

   #define UTIL_MEM_OOL_RESET_INTERVAL          ( 86400000 * 4 )   /// 4 days

   /*
      _utilMemSlotAssitStat
   */
   struct _utilMemSlotAssitStat
   {
      volatile UINT64            _oolTimes ;
      volatile UINT32            _fileCount ;
      volatile const CHAR       *_pLastFile ;
      volatile const CHAR       *_pLastInfo ;
      volatile UINT32            _lastLine ;

      volatile UINT64            _lastPrintTick ;
      volatile UINT64            _lastPrintTimes ;

      _utilMemSlotAssitStat()
      {
         reset() ;
      }

      void reset( UINT64 dbTiks = 0 )
      {
         _oolTimes = 0 ;
         _fileCount = 0 ;
         _pLastFile = "" ;
         _pLastInfo = "" ;
         _lastLine = 0 ;

         _lastPrintTick = dbTiks ;
         _lastPrintTimes = 0 ;
      }

      BOOLEAN inc( const CHAR *pFile, UINT32 line, const CHAR *pInfo,
                   _utilSegmentHandler *pHandle, UINT64 &oolInc )
      {
         UINT64 times = 0 ;
         UINT64 lastPrintTimes = _lastPrintTimes ;

         ++_oolTimes ;
         times = _oolTimes ;

         if ( pFile == _pLastFile && line == _lastLine )
         {
            ++_fileCount ;
         }
         else if ( _fileCount > 0 )
         {
            --_fileCount ;
         }
         else
         {
            _pLastFile = pFile ? pFile : "" ;
            _pLastInfo = pInfo ? pInfo : "" ;
            _lastLine = line ;
            ++_fileCount ;
         }

         if ( times > lastPrintTimes )
         {
            oolInc = times - lastPrintTimes ;
         }
         else
         {
            oolInc = 0 ;
         }

         if ( oolInc >= UTIL_MEM_OOL_PRINT_STEP &&
              pHandle->getTickSpanTime( _lastPrintTick ) >= UTIL_MEM_OOL_PRINT_INTERVAL )
         {
            _lastPrintTimes = times ;
            _lastPrintTick = pHandle->getDBTick() ;
            return TRUE ;
         }
         return FALSE ;
      }
   } ;
   typedef _utilMemSlotAssitStat utilMemSlotAssitStat ;

   /** definition of _utilMemBlockPool
    *  _utilMemBlockPool is a place holder for a set of memory pools based on 
    *  the fixed size of element in the pool
    **/
   class _utilMemBlockPool : public SDBObject, public _utilSegmentHandler
   {
      friend BOOLEAN utilPoolPtrCheck( void *ptr, UINT32 *pUserSize, UINT16 *pType ) ;
      /*
         Small: 32,   64,   128
         Mid  : 256,  512,  1024
         Big  : 2048, 4096, 8192
         Huge : 16K,  32K,  64K
      */
      typedef  CHAR    element32B[UTIL_MEM_ELEMENT_32] ;
      typedef  CHAR    element64B[UTIL_MEM_ELEMENT_64] ;
      typedef  CHAR    element128B[UTIL_MEM_ELEMENT_128] ;
      typedef  CHAR    element256B[UTIL_MEM_ELEMENT_256] ;
      typedef  CHAR    element512B[UTIL_MEM_ELEMENT_512] ;
      typedef  CHAR    element1K[UTIL_MEM_ELEMENT_1024] ;
      typedef  CHAR    element2K[UTIL_MEM_ELEMENT_2048] ;
      typedef  CHAR    element4K[UTIL_MEM_ELEMENT_4096] ;
      typedef  CHAR    element8K[UTIL_MEM_ELEMENT_8192] ;
      typedef  CHAR    element16K[UTIL_MEM_ELEMENT_16K] ;
      typedef  CHAR    element32K[UTIL_MEM_ELEMENT_32K] ;
      typedef  CHAR    element64K[UTIL_MEM_ELEMENT_64K] ;

      /*
         _blockNode define
      */
      struct _blockNode
      {
         _blockNode        *_next ;
         _blockNode        *_prev ;
         UINT64            _dbTick ;
      } ;

   public:
      enum MEMBLOCKPOOL_TYPE
      {
         MEMBLOCKPOOL_TYPE_DYN = 1,  // dynamically allocate space
         MEMBLOCKPOOL_TYPE_32,
         MEMBLOCKPOOL_TYPE_64,       // allocate from 64B pool
         MEMBLOCKPOOL_TYPE_128,
         MEMBLOCKPOOL_TYPE_256,
         MEMBLOCKPOOL_TYPE_512,
         MEMBLOCKPOOL_TYPE_1024,
         MEMBLOCKPOOL_TYPE_2048,
         MEMBLOCKPOOL_TYPE_4096,
         MEMBLOCKPOOL_TYPE_8192,
         MEMBLOCKPOOL_TYPE_16K,
         MEMBLOCKPOOL_TYPE_32K,
         MEMBLOCKPOOL_TYPE_64K,
         MEMBLOCKPOOL_TYPE_MAX
      } ;

      static MEMBLOCKPOOL_TYPE      size2MemType( UINT32 size ) ;
      static UINT32                 type2Size( MEMBLOCKPOOL_TYPE type ) ;

   public: 
      _utilMemBlockPool( BOOLEAN isGlobal = FALSE ) ;
      ~_utilMemBlockPool() ;

      INT32       init( UINT64 maxSize = UTIL_MEM_BLOCK_POOL_DFT_MAX_SZ,
                        UINT32 allocThreshold = 0 ) ;
      void        fini() ;
      UINT64      shrink( BOOLEAN forced = FALSE ) ;

      void        setMaxSize( UINT64 maxSize ) ;
      void        setAllocThreshold( UINT32 allocThreshold ) ;

      UINT64      getTotalSize() ;
      UINT64      getUsedSize() ;
      UINT64      getMaxOOLSize() ;

      void*       alloc( UINT32 size,
                         const CHAR *pFile,
                         UINT32 line,
                         UINT32 *pRealSize = NULL,
                         const CHAR *pInfo = NULL ) ;
      void*       realloc( void* ptr,
                           UINT32 size,
                           const CHAR *pFile,
                           UINT32 line,
                           UINT32 *pRealSize = NULL,
                           const CHAR *pInfo = NULL ) ;
      void        release( void*& ptr ) ;

      UINT32      dump( CHAR *pBuff, UINT32 buffLen ) ;

   public:
      virtual BOOLEAN   canAllocSegment( UINT64 size ) ;
      virtual BOOLEAN   canBallonUp( INT32 id, UINT64 size ) ;
      virtual BOOLEAN   canShrink( UINT32 objectSize,
                                   UINT32 totalObjNum,
                                   UINT32 usedObjNum ) ;
      virtual UINT64    getDBTick() const ;
      virtual UINT64    getTickSpanTime( UINT64 lastTick ) const ;
      virtual BOOLEAN   canShrinkDiscrete() const ;

   private:
      /// only for interface call
      virtual void*     allocBlock( UINT64 size ) ;
      virtual void      releaseBlock( void *p, UINT64 size ) ;

      _blockNode*       _popBlock( UINT64 size )
      {
         _blockNode *pNode = NULL ;

         if ( _cacheNum.fetch() > 0 && _cacheSize >= size )
         {
            SDB_ASSERT( _header, "Header can't be NULL" ) ;

            if ( _header )
            {
               if ( _header == _tailer )
               {
                  pNode = _header ;
                  _header = NULL ;
                  _tailer = NULL ;
               }
               else
               {
                  pNode = _header ;
                  _header = _header->_next ;
                  _header->_prev = NULL ;
               }

               _cacheNum.dec() ;
               _cacheSize -= size ;
            }
         }

         return pNode ;
      }

      BOOLEAN           _pushBlock( _blockNode *pNode, UINT64 size )
      {
         if ( _cacheSize + size < _maxCacheSize )
         {
            pNode->_next = NULL ;
            pNode->_prev = NULL ;
            pNode->_dbTick = getDBTick() ;

            if ( !_header )
            {
               SDB_ASSERT( !_tailer && 0 == _cacheNum.fetch() && 0 == _cacheSize,
                           "Tailer must be NULL and num must be 0" ) ;
               _header = pNode ;
               _tailer = pNode ;
            }
            /// insert to tailer
            else
            {
               _tailer->_next = pNode ;
               pNode->_prev = _tailer ;
               pNode->_next = NULL ;
               _tailer = pNode ;
            }

            _cacheNum.inc() ;
            _cacheSize += size ;
            return TRUE ;
         }

         return FALSE ;
      }

      void              _clearBlock() ;

      UINT64            _shrinkCache( BOOLEAN forced ) ;

   protected:
      void                    _fillPtr( CHAR *ptr, UINT16 type, UINT32 size ) ;

      static BOOLEAN          _checkAndExtract( const CHAR *ptr,
                                                UINT32 *pUserSize,
                                                UINT16 *pType ) ;

      void                    _clearStat() ;

      UINT64                  _shrink( UINT64 expectSize,
                                       UINT32 beginSlot,
                                       INT32 skipSlot ) ;

   // private attributes:
   private:
      BOOLEAN        _isGlobal ;
      utilSegmentInterface             *_slots[ MEMBLOCKPOOL_TYPE_MAX ] ;
      utilMemSlotAssitStat              _stats[ MEMBLOCKPOOL_TYPE_MAX ] ;

      UINT64         _maxSize ;
      UINT64         _maxCacheSize ;
      UINT32         _allocThreshold ;
      ossAtomic64    _totalSize ;

      /// stat info
      UINT64         _acquireTimes ;
      UINT64         _releaseTimes ;
      UINT64         _oomTimes ;
      UINT64         _oolTimes ;
      UINT64         _shrinkSize ;

      ossAtomic64    _oorTimes ;
      UINT64         _lastOORTimes ;

      ossAtomicSigned64 _curOOLSize ;
      volatile INT64 _maxOOLSize ;

      UINT64         _resetOOLTick ;

      /// memory cache info
      _blockNode    *_header ;
      _blockNode    *_tailer ;
      ossAtomic32    _cacheNum ;
      UINT64         _cacheSize ;
      ossSpinXLatch  _cacheLatch ;

   } ;
   typedef _utilMemBlockPool utilMemBlockPool ;

   /*
      Tool function
   */
   INT32 utilDumpInfo2File( const CHAR *pPath,
                            const CHAR *pFilePostfix,
                            const CHAR *pBuff,
                            UINT32 buffLen ) ;

   void  utilDumpPoolMemInfo( const CHAR *pPath ) ;

   /*
      Global function
   */
   utilMemBlockPool* utilGetGlobalMemPool() ;

   void*       utilPoolAlloc( UINT32 size,
                              const CHAR *pFile,
                              UINT32 line,
                              UINT32 *pRealSize = NULL,
                              const CHAR *pInfo = NULL ) ;
   void*       utilPoolRealloc( void* ptr,
                                UINT32 size,
                                const CHAR *pFile,
                                UINT32 line,
                                UINT32 *pRealSize = NULL,
                                const CHAR *pInfo = NULL ) ;
   void        utilPoolRelease( void*& ptr ) ;
   BOOLEAN     utilPoolPtrCheck( void *ptr, UINT32 *pUserSize = NULL,
                                 UINT16 *pType = NULL ) ;
   UINT32      utilPoolGetPtrSize( void *ptr ) ;

}

#define SDB_POOL_ALLOC(size)     engine::utilPoolAlloc(size,__FILE__,__LINE__,NULL)
#define SDB_POOL_REALLOC(p,size) engine::utilPoolRealloc(p,size,__FILE__,__LINE__,NULL)
#define SDB_POOL_FREE(p)         engine::utilPoolRelease((void*&)p)

#define SDB_POOL_ALLOC2(size,pRealSize) \
   engine::utilPoolAlloc(size,__FILE__,__LINE__,pRealSize)

#define SDB_POOL_REALLOC2(p,size,pRealSize) \
   engine::utilPoolRealloc(p,size,__FILE__,__LINE__,pRealSize)


#endif //UTIL_MEM_BLOCK_POOL_HPP__

