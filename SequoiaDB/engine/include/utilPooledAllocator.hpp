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

   Source File Name = utilPooledAllocator.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_POOLED_ALLOCATOR_HPP__
#define UTIL_POOLED_ALLOCATOR_HPP__

#include "utilMemListPool.hpp"
#include "pd.hpp"

#pragma warning( disable: 4200 )

namespace engine
{

   /*
      _utilPooledAllocator define
   */
   template < typename T >
   class _utilPooledAllocator : public std::allocator<T>
   {
      public:
         typedef typename std::allocator<T>::size_type         size_type ;
         typedef typename std::allocator<T>::pointer           pointer ;
         typedef typename std::allocator<T>::value_type        value_type ;
         typedef typename std::allocator<T>::const_pointer     const_pointer ;
         typedef typename std::allocator<T>::reference         reference ;
         typedef typename std::allocator<T>::const_reference   const_reference ;

      public:
         _utilPooledAllocator()
         {
         }

         _utilPooledAllocator( const _utilPooledAllocator &rhs )
         {
         }

         template < typename _T >
         _utilPooledAllocator( const _utilPooledAllocator<_T > &rhs )
         {
         }

         ~_utilPooledAllocator()
         {
         }

         pointer allocate( size_type count, const void* pHint = NULL )
         {
            pointer p = (pointer)SDB_THREAD_ALLOC( count *
                                                   sizeof( value_type ) ) ;
            if ( !p )
            {
               throw std::bad_alloc() ;
            }
            return p ;
         }

         void deallocate( pointer ptr, size_type count )
         {
            SDB_THREAD_FREE( ptr ) ;
         }

         template < typename _Other >
         struct rebind
         {
            // convert this type to allocator<_Other>
            typedef _utilPooledAllocator<_Other> other ;
         } ;

   } ;

}

#endif // UTIL_POOLED_ALLOCATOR_HPP__

