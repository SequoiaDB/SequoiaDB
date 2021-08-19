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

   Source File Name = utilPooledAutoPtr.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_POOLED_AUTO_PTR_HPP__
#define UTIL_POOLED_AUTO_PTR_HPP__

#include "ossTypes.hpp"
#include "ossAtomicBase.hpp"
#include "utilMemListPool.hpp"

namespace engine
{

   /*
      UTIL_ALLOC_TYPE define
   */
   enum UTIL_ALLOC_TYPE
   {
      ALLOC_OSS = 0,
      ALLOC_POOL,
      ALLOC_TC
   } ;

   /*
      _utilPooledAutoPtr define
   */
   class _utilPooledAutoPtr
   {
      public:
         _utilPooledAutoPtr() ;
         _utilPooledAutoPtr( const _utilPooledAutoPtr &rhs ) ;
         ~_utilPooledAutoPtr() ;

         _utilPooledAutoPtr& operator= ( const _utilPooledAutoPtr &rhs ) ;
         bool operator! () const { return get() ? false : true ; }

         operator bool () { return get() ? true : false ; }
         operator CHAR* () { return get () ; }
         operator BOOLEAN () { return get() ? TRUE : FALSE ; }
         operator const CHAR* () { return get() ; }

      public:
         static _utilPooledAutoPtr alloc( UINT32 size,
                                          const CHAR *pFile,
                                          UINT32 line,
                                          UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static _utilPooledAutoPtr alloc( UINT32 size,
                                          UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static _utilPooledAutoPtr make( CHAR *ptr,
                                         UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         // NOTICE: ptr must be created by utilPooledAutoPtr
         static _utilPooledAutoPtr makeRaw( CHAR *ptr,
                                            UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      private:
         _utilPooledAutoPtr( CHAR *ptr, UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      public:
         CHAR*       get() const ;
         INT64       refCount() const ;
         void        release() ;

      private:
         CHAR                 *_ptr ;
         INT64                *_pRef ;
         UTIL_ALLOC_TYPE      _allocType ;
   } ;
   typedef _utilPooledAutoPtr utilPooledAutoPtr ;

   /*
      utilSharePtr define
   */
   template < typename T >
   class utilSharePtr
   {
      public:
         utilSharePtr() ;
         utilSharePtr( const utilSharePtr &rhs ) ;
         ~utilSharePtr() ;

         utilSharePtr& operator= ( const utilSharePtr &rhs ) ;
         bool operator! () const { return get() ? false : true ; }
         T* operator->() { return get() ; }
         const T* operator->() const { return get() ; }

         operator bool () { return get() ? true : false ; }
         operator T* () { return get () ; }
         operator BOOLEAN () { return get() ? TRUE : FALSE ; }
         operator const T* () { return get() ; }

      public:
         static utilSharePtr alloc( const CHAR *pFile,
                                    UINT32 line,
                                    UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static utilSharePtr alloc( UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         // NOTICE: ptr must call construct after allocRaw
         static utilSharePtr allocRaw( const CHAR *pFile,
                                       UINT32 line,
                                       UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         // NOTICE: ptr must call construct after allocRaw
         static utilSharePtr allocRaw( UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static utilSharePtr make( T *ptr,
                                   UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         // NOTICE: ptr must be created by utilPooledAutoPtr
         static utilSharePtr makeRaw( T *ptr,
                                      UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      public:
         T*          get() const { return _ptr ; }
         INT64       refCount() const { return _pRef ? *_pRef : 0 ; }
         void        release() ;

      private:
         utilSharePtr( T *ptr, UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      private:
         T                    *_ptr ;
         INT64                *_pRef ;
         UTIL_ALLOC_TYPE      _allocType ;

   } ;

   template< typename T >
   utilSharePtr<T>::utilSharePtr()
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = ALLOC_TC ;
   }

   template< typename T >
   utilSharePtr<T>::utilSharePtr( const utilSharePtr &rhs )
   {
      _ptr = rhs._ptr ;
      _pRef = rhs._pRef ;
      _allocType = rhs._allocType ;
      if ( _pRef )
      {
         INT64 orgRef = ossFetchAndIncrement64( _pRef ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
   }

   template< typename T >
   utilSharePtr<T>::utilSharePtr( T *ptr, UTIL_ALLOC_TYPE type )
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = type ;
      if ( ptr )
      {
         /// create ref
         if ( ALLOC_OSS == _allocType )
         {
            _pRef = ( INT64* )SDB_OSS_MALLOC( sizeof( INT64 ) ) ;
         }
         else if ( ALLOC_POOL == _allocType )
         {
            _pRef = ( INT64* )SDB_POOL_ALLOC( sizeof( INT64 ) ) ;
         }
         else
         {
            _pRef = ( INT64* )SDB_THREAD_ALLOC( sizeof( INT64 ) ) ;
         }

         if ( !_pRef )
         {
            throw std::bad_alloc() ;
         }

         *_pRef = 1 ;
         _ptr = ptr ;
      }
   }

   template< typename T >
   utilSharePtr<T>::~utilSharePtr()
   {
      release() ;
   }

   template< typename T >
   utilSharePtr<T>& utilSharePtr<T>::operator= ( const utilSharePtr &rhs )
   {
      // avoid assigning by self
      if ( _ptr != rhs._ptr )
      {
         release() ;
         _ptr = rhs._ptr ;
         _pRef = rhs._pRef ;
         _allocType = rhs._allocType ;
         if ( _pRef )
         {
            INT64 orgRef = ossFetchAndIncrement64( _pRef ) ;
            SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
            SDB_UNUSED( orgRef ) ;
         }
      }
      return *this ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::alloc( const CHAR *pFile,
                                           UINT32 line,
                                           UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;
      UINT32 realSZ = sizeof( T ) + sizeof( INT64 ) ;
      CHAR *ptr = NULL ;

      if ( ALLOC_OSS == type )
      {
         ptr = ( CHAR* )ossMemAlloc( realSZ, pFile, line ) ;
      }
      else if ( ALLOC_POOL == type )
      {
         ptr = ( CHAR* )utilPoolAlloc( realSZ, pFile, line ) ;
      }
      else
      {
         ptr = ( CHAR* )utilThreadAlloc( realSZ, pFile, line ) ;
      }

      if ( ptr )
      {
         *(INT64*)ptr = 1 ;
         recordPtr._pRef = (INT64*)ptr ;
         recordPtr._ptr = (T*)( ptr + sizeof( INT64 ) ) ;
         recordPtr._allocType = type ;
         new ( recordPtr._ptr ) T () ;
      }
      return recordPtr ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::alloc( UTIL_ALLOC_TYPE type )
   {
      return alloc( __FILE__, __LINE__, type ) ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::allocRaw( const CHAR *pFile,
                                              UINT32 line,
                                              UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;
      UINT32 realSZ = sizeof( T ) + sizeof( INT64 ) ;
      CHAR *ptr = NULL ;

      if ( ALLOC_OSS == type )
      {
         ptr = ( CHAR* )ossMemAlloc( realSZ, pFile, line ) ;
      }
      else if ( ALLOC_POOL == type )
      {
         ptr = ( CHAR* )utilPoolAlloc( realSZ, pFile, line ) ;
      }
      else
      {
         ptr = ( CHAR* )utilThreadAlloc( realSZ, pFile, line ) ;
      }

      if ( ptr )
      {
         *(INT64*)ptr = 1 ;
         recordPtr._pRef = (INT64*)ptr ;
         recordPtr._ptr = (T*)( ptr + sizeof( INT64 ) ) ;
         recordPtr._allocType = type ;
      }
      return recordPtr ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::allocRaw( UTIL_ALLOC_TYPE type )
   {
      return allocRaw( __FILE__, __LINE__, type ) ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::make( T *ptr,
                                          UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;

      if ( ptr )
      {
         /// create ref
         if ( ALLOC_OSS == type )
         {
            recordPtr._pRef = ( INT64* )SDB_OSS_MALLOC( sizeof( INT64 ) ) ;
         }
         else if ( ALLOC_POOL == type )
         {
            recordPtr._pRef = ( INT64* )SDB_POOL_ALLOC( sizeof( INT64 ) ) ;
         }
         else
         {
            recordPtr._pRef = ( INT64* )SDB_THREAD_ALLOC( sizeof( INT64 ) ) ;
         }

         if ( recordPtr._pRef )
         {
            *(recordPtr._pRef) = 1 ;
            recordPtr._ptr = ptr ;
            recordPtr._allocType = type ;
         }
      }
      return recordPtr ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::makeRaw( T *ptr,
                                             UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;

      if ( ptr )
      {
         recordPtr._ptr = ptr ;
         recordPtr._pRef = (INT64 *)( (CHAR *)ptr - sizeof( INT64 ) ) ;
         recordPtr._allocType = type ;
         if ( recordPtr._pRef )
         {
            INT64 orgRef = ossFetchAndIncrement64( recordPtr._pRef ) ;
            SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
            SDB_UNUSED( orgRef ) ;
         }
      }

      return recordPtr ;
   }

   template< typename T >
   void utilSharePtr<T>::release()
   {
      if ( _pRef )
      {
         INT64 orgRef = ossFetchAndDecrement64( _pRef ) ;
         SDB_ASSERT( orgRef >= 1, "Ref is invlaid" ) ;
         if ( 1 == orgRef )
         {
            if ( (CHAR*)_ptr - sizeof( INT64 ) == ( CHAR* )_pRef )
            {
               _ptr->~T() ;
            }
            else
            {
               SDB_OSS_DEL( _ptr ) ;
            }

            if ( ALLOC_OSS == _allocType )
            {
               SDB_OSS_FREE( _pRef ) ;
            }
            else if ( ALLOC_POOL == _allocType )
            {
               SDB_POOL_FREE( _pRef ) ;
            }
            else
            {
               SDB_THREAD_FREE( _pRef ) ;
            }
         }

         _pRef = NULL ;
         _ptr = NULL ;
      }
   }

   /*
      get_pointer: used for boost bind
    */
   OSS_INLINE CHAR *get_pointer( const utilPooledAutoPtr & p )
   {
      return p.get() ;
   }

   template<class T>
   OSS_INLINE T *get_pointer( const utilSharePtr<T> &p)
   {
       return p.get();
   }

}

#endif // UTIL_POOLED_AUTO_PTR_HPP__

