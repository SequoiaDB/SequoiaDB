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
   #define UTIL_MEM_A_SMALL_BLOCK_SIZE             ( 524288 )        ///512KB
   #define UTIL_MEM_A_MID_BLOCK_SIZE               ( 2097152 )       ///2MB
   #define UTIL_MEM_A_BIG_BLOCK_SIZE               ( 4194304 )       ///4MB
   #define UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM      ( 8 )
   #define UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM        ( 4 )
   #define UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM        ( 2 )

   /// Memory info:
   /// | B-Eye(2) | Size(4) | Type(2) | User Data | E-Eye(2) |

   #define UTIL_MEM_B_EYE_CHAR         0xBE38
   #define UTIL_MEM_E_EYE_CHAR         0xAC52

   #define UTIL_MEM_B_EYE_LEN          sizeof(UINT16)
   #define UTIL_MEM_SIZE_LEN           sizeof(UINT32)
   #define UTIL_MEM_TYPE_LEN           sizeof(UINT16)
   #define UTIL_MEM_E_EYE_LEN          sizeof(UINT16)

   #define UTIL_MEM_HEAD_FILL_LEN   \
      ( UTIL_MEM_B_EYE_LEN + UTIL_MEM_SIZE_LEN + UTIL_MEM_TYPE_LEN )

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
      ( (UINT16*)((CHAR*)UTIL_MEM_PTR_SIZE_PTR(ptr)+UTIL_MEM_SIZE_LEN) )

   #define UTIL_MEM_PTR_E_EYE_PTR(ptr, sz) \
      ( (UINT16*)( (CHAR*)(ptr) + (UINT32)(sz) - UTIL_MEM_E_EYE_LEN ) )

   /*
      Define element size
   */
   #define UTIL_MEM_ELEMENT_32                  ( 32 )
   #define UTIL_MEM_ELEMENT_64                  ( 64 )
   #define UTIL_MEM_ELEMENT_128                 ( 128 )
   #define UTIL_MEM_ELEMENT_256                 ( 256 )
   #define UTIL_MEM_ELEMENT_512                 ( 512 )
   #define UTIL_MEM_ELEMENT_1024                ( 1024 )
   #define UTIL_MEM_ELEMENT_2048                ( 2048 )
   #define UTIL_MEM_ELEMENT_4096                ( 4096 )
   #define UTIL_MEM_ELEMENT_8192                ( 8192 )

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
         MEMBLOCKPOOL_TYPE_MAX
      } ;

      static MEMBLOCKPOOL_TYPE      size2MemType( UINT32 size ) ;
      static UINT32                 type2Size( MEMBLOCKPOOL_TYPE type ) ;

   public: 
      _utilMemBlockPool( BOOLEAN isGlobal = FALSE ) ;
      ~_utilMemBlockPool() ;

      INT32       init( UINT64 maxSize = UTIL_MEM_BLOCK_POOL_DFT_MAX_SZ ) ;
      void        fini() ;
      void        shrink() ;

      void        setMaxSize( UINT64 maxSize ) ;

      UINT64      getTotalSize() ;

      void*       alloc( UINT32 size,
                         const CHAR *pFile,
                         UINT32 line,
                         UINT32 *pRealSize = NULL ) ;
      void*       realloc( void* ptr,
                           UINT32 size,
                           const CHAR *pFile,
                           UINT32 line,
                           UINT32 *pRealSize = NULL ) ;
      void        release( void*& ptr ) ;

      UINT32      dump( CHAR *pBuff, UINT32 buffLen ) ;

   public:
      virtual BOOLEAN   canAllocSegment( UINT64 size ) ;
      virtual void      onAllocSegment( UINT64 size ) ;
      virtual void      onReleaseSegment( UINT64 freedSize ) ;
      virtual BOOLEAN   canShrink( UINT32 blockSize,
                                   UINT64 totalSize,
                                   UINT64 usedSize ) ;

   protected:
      void                    _fillPtr( CHAR *ptr, UINT16 type, UINT32 size ) ;

      static BOOLEAN          _checkAndExtract( const CHAR *ptr,
                                                UINT32 *pUserSize,
                                                UINT16 *pType ) ;

      void                    _clearStat() ;

   // private attributes:
   private:
      BOOLEAN        _isGlobal ;

      _utilSegmentManager<element32B>  *_32BSeg; // mem segs with 32B  element
      _utilSegmentManager<element64B>  *_64BSeg; // mem segs with 64B  element
      _utilSegmentManager<element128B> *_128BSeg;// mem segs with 128B element
      _utilSegmentManager<element256B> *_256BSeg;// mem segs with 256B element
      _utilSegmentManager<element512B> *_512BSeg;// mem segs with 512B element
      _utilSegmentManager<element1K>   *_1KSeg;  // mem segs with 1 KB element
      _utilSegmentManager<element2K>   *_2KSeg;  // mem segs with 2 KB element
      _utilSegmentManager<element4K>   *_4KSeg;  // mem segs with 4 KB element
      _utilSegmentManager<element8K>   *_8KSeg;  // mem segs with 8 KB element

      UINT64         _maxSize ;
      ossAtomic64    _totalSize ;

      /// stat info
      UINT64         _acquireTimes ;
      UINT64         _releaseTimes ;
      UINT64         _oomTimes ;
      UINT64         _oolTimes ;
      UINT64         _shrinkSize ;

      ossAtomic64    _oorTimes ;
      UINT64         _lastOORTimes ;
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
                              UINT32 *pRealSize = NULL ) ;
   void*       utilPoolRealloc( void* ptr,
                                UINT32 size,
                                const CHAR *pFile,
                                UINT32 line,
                                UINT32 *pRealSize = NULL ) ;
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

