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

   Source File Name = ossEndian.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSS_ENDIAN_HPP_
#define OSS_ENDIAN_HPP_

#include "ossUtil.hpp"
#include <type_traits>

namespace engine
{

   template <
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   T ossNativeToBigEndian( T in )
   {
   #ifdef SDB_BIG_ENDIAN
      return in ;
   #else
      T out ;
      ossEndianConvertIf( in, out, TRUE ) ;
      return out ;
   #endif
   }

   template <
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   T ossBigEndianToNative( T in )
   {
   #ifdef SDB_BIG_ENDIAN
      return in ;
   #else
      T out ;
      ossEndianConvertIf( in, out, TRUE ) ;
      return out ;
   #endif
   }

   void ossMemcpyFlipBits( void *dst, const void *src, size_t len ) ;

}

#endif // OSS_ENDIAN_HPP_

