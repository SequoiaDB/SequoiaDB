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

   Source File Name = utilKeyStringModifier.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_KEY_STRING_MODIFIER_HPP_
#define UTIL_KEY_STRING_MODIFIER_HPP_

#include "keystring/utilKeyString.hpp"
#include "utilStreamAllocator.hpp"

namespace engine
{
namespace keystring
{

   /*
      _keyStringModifier define
    */
   class _keyStringModifier : public SDBObject
   {
   public:
      _keyStringModifier() = default ;
      _keyStringModifier( const keyString &src ) ;
      ~_keyStringModifier() ;
      _keyStringModifier( const _keyStringModifier& ) = delete ;
      _keyStringModifier& operator=( const _keyStringModifier& ) = delete ;

   public:
      void init( const keyString &ks ) ;
      void reset() ;

      INT32 incRID( BOOLEAN force = FALSE ) ;
      INT32 decRID( BOOLEAN force = FALSE ) ;

      keyString getShallowKeyString() const ;

   private:
      INT32 _ensureBuffer( UINT32 size ) ;

   private:
      keyString _src ;
      CHAR *_buffer = nullptr ;
      UINT32 _bufferSize = 0 ;
      utilStreamStackAllocator _allocator ;
   } ;

}
}


#endif // UTIL_KEY_STRING_MODIFIER_HPP_
