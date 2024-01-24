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

   Source File Name = utilMath.hpp

   Descriptive Name = math utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  HAS Initial Draft

   Last Changed =

******************************************************************************/


#ifndef UTILMATH_HPP__
#define UTILMATH_HPP__



#include "ossTypes.h"


namespace engine
{

   OSS_INLINE INT64 utilAbs( INT64 value )
   {
      return value >= 0 ? value : -value ;
   }

   OSS_INLINE INT32 utilAbs( INT32 value )
   {
      return value >= 0 ? value : -value ;
   }

   OSS_INLINE BOOLEAN utilCanConvertToINT32( INT64 value )
   {
      return value <= OSS_SINT32_MAX_LL && value >= OSS_SINT32_MIN_LL ;
   }

   //
   // overflow check in basic operation
   //
   BOOLEAN utilAddIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilSubIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilMulIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilDivIsOverflow( INT64 l, INT64 r ) ;

   FLOAT64 utilPercentage( INT64 x, INT64 y ) ;
}


#endif // UTILMATH_HPP__
