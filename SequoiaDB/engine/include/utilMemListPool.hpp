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

   Source File Name = utilMemListPool.hpp

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

#ifndef UTIL_MEM_LIST_POOL_HPP__
#define UTIL_MEM_LIST_POOL_HPP__

#include "utilMemBlockPool.hpp"

namespace engine
{

   /*
      Config
   */
   void     utilSetMaxTCSize( UINT32 maxCacheSize ) ;       /// bytes
   UINT32   utilGetMaxTCSize() ;

   #define UTIL_MEM_THREAD_NAME_LEN          ( 64 )

   /*
      _utilMemListNode define
   */
   struct _utilMemListNode
   {
      _utilMemListNode  *_next ;

      _utilMemListNode()
      {
         reset() ;
      }
      void reset()
      {
         _next = NULL ;
      }
   } ;
   typedef _utilMemListNode utilMemListNode ;

   /*
      _utilMemListEvent define
   */
   class _utilMemListEvent
   {
      public:
         _utilMemListEvent() {}
         virtual ~_utilMemListEvent() {}

      public:
         virtual void      onReleaseCache( UINT64 size ) = 0 ;
         virtual void      onAllocCache( UINT32 blockSize ) = 0 ;
         virtual void      onPushedCache( UINT32 blockSize ) = 0 ;
         virtual void      onOutOfCache( UINT32 blockSize,
                                         UINT64 selfCacheSize ) = 0 ;
         virtual BOOLEAN   canAllocBlockSize( UINT32 size, UINT32 blockSize ) = 0 ;
         virtual BOOLEAN   canCacheBlock( UINT32 blockSize ) = 0 ;
   } ;
   typedef _utilMemListEvent utilMemListEvent ;

   /*
      _utilMemListItem define
   */
   class _utilMemListItem
   {
      public:
         _utilMemListItem( UINT32 blockSize,
                           utilMemListEvent *pEvent = NULL ) ;

         ~_utilMemListItem() ;

         void*    alloc( UINT32 size, const CHAR *pFile, UINT32 line,
                         UINT32 *pRealSize = NULL ) ;
         void     dealloc( void *p ) ;

         void     clear() ;
         UINT64   shrink( UINT64 expectSize ) ;

         UINT64   getCacheSize() const { return _cachedSize ; }
         UINT32   getBlockSize() const { return _blockSize ; }

         UINT32   dump( CHAR *pBuff, UINT32 buffSize ) ;

      protected:
         BOOLEAN _canAllocBlockSize( UINT32 size )
         {
            if ( !_pEvent || _pEvent->canAllocBlockSize( size, _blockSize ) )
            {
               return TRUE ;
            }
            return FALSE ;
         }
         BOOLEAN _canCacheBlock() ;

      private:
         const UINT32            _blockSize ;

         utilMemListNode         *_header ;
         utilMemListEvent        *_pEvent ;
         UINT64                  _cachedSize ;

         /// stat info
         UINT64                  _allocCount ;
         UINT64                  _deallocCount ;
         UINT64                  _hitCount ;
         UINT64                  _pushCount ;
   } ;
   typedef _utilMemListItem utilMemListItem ;

   #define UTIL_MEM_POOL_MAXCACHE_SIZE_DFT         ( 2097152 )    /// 2MB
   #define UTIL_MEM_POOL_LIST_NUM                  ( 8 )
   #define UTIL_MEM_LIST_BASE_EXPONENT             ( 5 )          /// 32

   /*
      _utilMemListPool define
   */
   class _utilMemListPool : public _utilMemListEvent
   {
      public:
         _utilMemListPool() ;
         ~_utilMemListPool() ;

         INT32    init() ;
         void     fini() ;

         void     setName( const CHAR *pName ) ;
         BOOLEAN  isInit() const { return _hasInit ; }

         const CHAR* getName() const { return _name ; }

      public:
         virtual void      onReleaseCache( UINT64 size ) ;
         virtual void      onAllocCache( UINT32 blockSize ) ;
         virtual void      onPushedCache( UINT32 blockSize ) ;
         virtual void      onOutOfCache( UINT32 blockSize,
                                         UINT64 selfCacheSize ) ;
         virtual BOOLEAN   canAllocBlockSize( UINT32 size, UINT32 blockSize ) ;
         virtual BOOLEAN   canCacheBlock( UINT32 blockSize ) ;

      public:
         void*       alloc( UINT32 size,
                            const CHAR *pFile,
                            UINT32 line,
                            UINT32 *pRealSize = NULL ) ;
         void*       realloc( void* ptr, UINT32 size,
                              const CHAR *pFile,
                              UINT32 line,
                              UINT32 *pRealSize = NULL ) ;
         void        release( void*& ptr ) ;

         void        clear() ;
         void        shrink() ;
         UINT64      getCacheSize() const { return _cachedSize ; }

         UINT32      dump( CHAR *pBuff, UINT32 buffSize ) ;

      protected:
         void        clearEBB() ;
         void*       allocFromEBB( UINT32 size,
                                   const CHAR *pFile,
                                   UINT32 line,
                                   UINT32 *pRealSize = NULL ) ;
         void        release2EBB( void *&p, UINT32 size ) ;

      private:
         UINT64            _cachedSize ;
         BOOLEAN           _hasInit ;
         CHAR              _name[ UTIL_MEM_THREAD_NAME_LEN + 1 ] ;
         utilMemListItem*  _arrayList[ UTIL_MEM_POOL_LIST_NUM + 1 ] ;

         CHAR              *_pEBB ;
         UINT32            _EBBSize ;
         UINT64            _allocEBBCount ;

         /// stat info
         UINT64            _allocCount ;
         UINT64            _reallocCount ;
         UINT64            _deallocCount ;
         UINT64            _hitCount ;
         UINT64            _pushCount ;
         UINT64            _copyCount ;

         UINT64            _outrangeAlloc ;
         UINT64            _outrangeDealloc ;
   } ;
   typedef _utilMemListPool utilMemListPool ;

   /*
      Global function
   */
   void        utilSetThreadMemPool( utilMemListPool *pPool ) ;
   void        utilDumpThreadMemPoolInfo( const CHAR *pType,
                                          const CHAR *pName ) ;

   void        utilDumpThreadMemBegin( const CHAR *pPath ) ;
   void        utilDumpThreadMemPoolInfo( const CHAR *pPath ) ;

   void        utilClearThreadMemPool() ;
   UINT32      utilThreadMemPoolSize() ;

   void*       utilThreadAlloc( UINT32 size,
                                const CHAR *pFile,
                                UINT32 line,
                                UINT32 *pRealSize = NULL ) ;
   void*       utilThreadRealloc( void* ptr, UINT32 size,
                                  const CHAR *pFile, UINT32 line,
                                  UINT32 *pRealSize = NULL ) ;
   void        utilThreadRelease( void*& ptr ) ;

   extern "C"
   {
      void*    utilTCAlloc( UINT32 size ) ;
      void*    utilTCRealloc( void* ptr, UINT32 size ) ;
      void     utilTCRelease( void* ptr ) ;
   }

}

#define SDB_THREAD_ALLOC(size)      engine::utilThreadAlloc(size,__FILE__,__LINE__,NULL)
#define SDB_THREAD_REALLOC(p,size)  engine::utilThreadRealloc(p,size,__FILE__,__LINE__,NULL)
#define SDB_THREAD_FREE(p)          engine::utilThreadRelease((void*&)p)

#define SDB_THREAD_ALLOC2(size,pRealSize) \
   engine::utilThreadAlloc(size,__FILE__,__LINE__,pRealSize)

#define SDB_THREAD_REALLOC2(p,size,pRealSize) \
   engine::utilThreadRealloc(p,size,__FILE__,__LINE__,pRealSize)


#endif //UTIL_MEM_LIST_POOL_HPP__

