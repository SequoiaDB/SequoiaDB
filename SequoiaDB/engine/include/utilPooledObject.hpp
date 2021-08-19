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

   Source File Name = utilPooledObject.hpp

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
#ifndef UTIL_POOLED_OBJECT_HPP__
#define UTIL_POOLED_OBJECT_HPP__

#pragma warning( disable: 4290 )
#include "utilMemListPool.hpp"
#include "ossMem.hpp"

namespace engine
{
   // we should NEVER AND EVER put any virtual function in this object
   // and we should never cast an object to _utilPooledObject and delete.
   class _utilPooledObject
   {
   protected :
      // never instantiate SDBObject
      _utilPooledObject () {}
   public :
   // do NOT put virtual destructor because it will change the inherited object
   // size

      // regular new
      void * operator new ( size_t size ) throw ( const char * )
      {
         void *p = SDB_THREAD_ALLOC( size ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void * operator new[] ( size_t size ) throw ( const char * )
      {
         void *p = SDB_THREAD_ALLOC( size ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      // placement new
      void * operator new ( size_t size, void* p ) throw ( const char * )
      {
         if ( !p ) throw "allocation failure" ;
         return p;
      }

      void * operator new[] ( size_t size, void* p ) throw ( const char * )
      {
         if ( !p ) throw "allocation failure" ;
         return p;
      }

      void operator delete ( void *p )
      {
         SDB_THREAD_FREE( p ) ;
      }

      void operator delete[] ( void *p )
      {
         SDB_THREAD_FREE( p ) ;
      }

      // placement delete (no-op)
      void operator delete ( void* p , void* p2) throw ()
      {
      }

      void operator delete[] ( void* p, void* p2 ) throw ()
      {
      }

      // new with file/line number
      void * operator new ( size_t size, const CHAR *pFile, UINT32 line )
            throw ( const char * )
      {
         void *p = utilThreadAlloc( size, pFile, line ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void * operator new[] ( size_t size, const CHAR *pFile, UINT32 line )
            throw ( const char * )
      {
         void *p = utilThreadAlloc( size, pFile, line ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void operator delete ( void *p, const CHAR *pFile, UINT32 line )
      {
         SDB_THREAD_FREE( p ) ;
      }

      void operator delete[] ( void *p, const CHAR *pFile, UINT32 line )
      {
         SDB_THREAD_FREE( p ) ;
      }

      // no throw
      void * operator new ( size_t size, const std::nothrow_t & )
      {
         return SDB_THREAD_ALLOC( size ) ;
      }

      void * operator new[] ( size_t size, const std::nothrow_t & )
      {
         return SDB_THREAD_ALLOC( size ) ;
      }

      void operator delete ( void *p, const std::nothrow_t & )
      {
         SDB_THREAD_FREE(p) ;
      }

      void operator delete[] ( void *p, const std::nothrow_t & )
      {
         SDB_THREAD_FREE(p) ;
      }

      // no throw with line number
      void * operator new ( size_t size, const CHAR *pFile,
                            UINT32 line, const std::nothrow_t & )
      {
         return utilThreadAlloc( size, pFile, line ) ;
      }

      void * operator new[] ( size_t size, const CHAR *pFile,
                              UINT32 line, const std::nothrow_t & )
      {
         return utilThreadAlloc( size, pFile, line ) ;
      }

      void operator delete ( void *p, const CHAR *pFile,
                             UINT32 line, const std::nothrow_t & )
      {
         SDB_THREAD_FREE(p) ;
      }

      void operator delete[] ( void *p, const CHAR *pFile,
                               UINT32 line, const std::nothrow_t & )
      {
         SDB_THREAD_FREE(p) ;
      }
   } ;
   typedef class _utilPooledObject utilPooledObject ;

}

#endif // UTIL_POOLED_OBJECT_HPP__

