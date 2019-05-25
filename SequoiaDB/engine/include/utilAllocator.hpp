/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilAllocator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/04/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_ALLOCATOR_HPP_
#define UTIL_ALLOCATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"

using namespace std ;

#define UTIL_ALLOCATOR_SIZE   265

namespace engine
{
   template < UINT32 stackSize = UTIL_ALLOCATOR_SIZE >
   class _utilAllocator
   {
      public :
         _utilAllocator()
         {
            _offset = 0 ;
         }

         virtual ~_utilAllocator()
         {
            _offset = 0 ;
         }

         void* allocate ( size_t size )
         {
            void *p = NULL ;
            if ( _offset + size <= stackSize )
            {
               p = _mem + _offset ;
               _offset += size ;
            }

            return p ;
         }

         BOOLEAN isAllocatedByme ( void *p )
         {
            if ( p >= _mem && p < _mem + stackSize )
            {
               return TRUE ;
            }

            return FALSE ;
         }

      protected :
         char _mem[ stackSize ] ;
         INT32 _offset ;
   } ;
}

#endif // UTIL_ALLOCATOR_HPP_

