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

   Source File Name = utilPooledAutoPtr.cpp

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

#include "utilPooledAutoPtr.hpp"

namespace engine
{

   /*
      _utilPooledAutoPtr implement
   */
   _utilPooledAutoPtr::_utilPooledAutoPtr()
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = ALLOC_TC ;
   }

   _utilPooledAutoPtr::_utilPooledAutoPtr( CHAR *ptr, UTIL_ALLOC_TYPE type )
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = type ;
      if ( _pRef )
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

   _utilPooledAutoPtr::_utilPooledAutoPtr( const _utilPooledAutoPtr &rhs )
   {
      _ptr = rhs._ptr ;
      _pRef = rhs._pRef ;
      _allocType = rhs._allocType ;
      if ( _ptr )
      {
         INT64 orgRef = ossFetchAndIncrement64( _pRef ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
   }

   _utilPooledAutoPtr::~_utilPooledAutoPtr()
   {
      release() ;
   }

   _utilPooledAutoPtr& _utilPooledAutoPtr::operator= ( const _utilPooledAutoPtr &rhs )
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

   _utilPooledAutoPtr _utilPooledAutoPtr::alloc( UINT32 size,
                                                 const CHAR *pFile,
                                                 UINT32 line,
                                                 UTIL_ALLOC_TYPE type )
   {
      _utilPooledAutoPtr recordPtr ;
      if ( size > 0 )
      {
         UINT32 realSZ = size + sizeof( INT64 ) ;
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
            recordPtr._ptr = ptr + sizeof( INT64 ) ;
            recordPtr._allocType = type ;
         }
      }
      return recordPtr ;
   }

   _utilPooledAutoPtr _utilPooledAutoPtr::alloc( UINT32 size,
                                                 UTIL_ALLOC_TYPE type )
   {
      return alloc( size, __FILE__, __LINE__, type ) ;
   }

   _utilPooledAutoPtr _utilPooledAutoPtr::make( CHAR *ptr,
                                                UTIL_ALLOC_TYPE type )
   {
      _utilPooledAutoPtr recordPtr ;

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

   _utilPooledAutoPtr _utilPooledAutoPtr::makeRaw( CHAR *ptr,
                                                   UTIL_ALLOC_TYPE type )
   {
      _utilPooledAutoPtr recordPtr ;

      if ( ptr )
      {
         recordPtr._ptr = ptr ;
         recordPtr._pRef = (INT64 *)( ptr - sizeof( INT64 ) ) ;
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

   CHAR* _utilPooledAutoPtr::get() const
   {
      return _ptr ;
   }

   INT64 _utilPooledAutoPtr::refCount() const
   {
      return _pRef ? *_pRef : 0 ;
   }

   void _utilPooledAutoPtr::release()
   {
      if ( _pRef )
      {
         INT64 orgRef = ossFetchAndDecrement64( _pRef ) ;
         SDB_ASSERT( orgRef >= 1, "Ref is invlaid" ) ;
         if ( 1 == orgRef )
         {
            if ( _ptr - sizeof( INT64 ) == ( CHAR* )_pRef )
            {
               _ptr = NULL ;
            }

            if ( ALLOC_OSS == _allocType )
            {
               SDB_OSS_FREE( _pRef ) ;
               if ( _ptr )
               {
                  SDB_OSS_FREE( _ptr ) ;
               }
            }
            else if ( ALLOC_POOL == _allocType )
            {
               SDB_POOL_FREE( _pRef ) ;
               if ( _ptr )
               {
                  SDB_POOL_FREE( _ptr ) ;
               }
            }
            else
            {
               SDB_THREAD_FREE( _pRef ) ;
               if ( _ptr )
               {
                  SDB_THREAD_FREE( _ptr ) ;
               }
            }
         }

         _pRef = NULL ;
         _ptr = NULL ;
      }
   }

}

