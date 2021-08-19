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

   Source File Name = ossAllocator.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/09/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_ALLOCATOR_HPP__
#define OSS_ALLOCATOR_HPP__

#include "ossUtil.hpp"

#pragma warning( disable: 4200 )

namespace engine
{

   /*
      _utilPooledAllocator define
   */
   template < typename T >
   class _ossAllocator : public std::allocator<T>
   {
      public:
         typedef typename std::allocator<T>::size_type         size_type ;
         typedef typename std::allocator<T>::pointer           pointer ;
         typedef typename std::allocator<T>::value_type        value_type ;
         typedef typename std::allocator<T>::const_pointer     const_pointer ;
         typedef typename std::allocator<T>::reference         reference ;
         typedef typename std::allocator<T>::const_reference   const_reference ;

      public:
         _ossAllocator()
         {
         }

         _ossAllocator( const _ossAllocator &rhs )
         {
         }

         template < typename _T >
         _ossAllocator( const _ossAllocator<_T > &rhs )
         {
         }

         ~_ossAllocator()
         {
         }

         pointer allocate( size_type count, const void* pHint = NULL )
         {
            ossSignalShield shield ;
            shield.doNothing() ;
            return (pointer)malloc( count * sizeof( value_type ) ) ;
         }

         void deallocate( pointer ptr, size_type count )
         {
            ossSignalShield shield ;
            shield.doNothing() ;
            free( ptr ) ;
         }

         template < typename _Other >
         struct rebind
         {
            // convert this type to allocator<_Other>
            typedef _ossAllocator<_Other> other ;
         } ;

   } ;

}

#endif // OSS_ALLOCATOR_HPP__

