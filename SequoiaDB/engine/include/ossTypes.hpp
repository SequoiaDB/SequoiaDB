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

   Source File Name = ossTypes.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSSTYPES_HPP_
#define OSSTYPES_HPP_

#include "ossTypes.h"

// Round a number up to the next multiple of X ( power of 2 ) bytes.
//
// UINT32_64 ( either 32bit or 64bit integer, which is large enough to contain
// a pointer ) is used for all alignment calculations.
//
// i [in]
//   The value to round up
// X [in]
//   The alignment amount. It MUST be a power of 2.
OSS_INLINE UINT32 ossAlignX( UINT32 i, UINT32_64 X  )
{
   return ( ( i + ( X - 1 ) ) & ( ~( ( UINT32 ) ( X - 1 ) ) ) ) ;
}


OSS_INLINE UINT64 ossAlignX( UINT64 i, UINT32_64   X )
{
   return ( ( i + ( X - 1 ) ) & ( ~( ( UINT64 ) ( X - 1 ) ) ) ) ;
}

// Round up a char * address number up to the next multiple of X ( power of 2 )
// pAddr [ in ]
//   The address number to round up
// X [in]
//   The alignment amount, MUST be a power of 2
OSS_INLINE char * ossAlignX( char * pAddr, UINT32_64 X )
{
   return ( ( char * ) ossAlignX( ( UINT32_64 ) pAddr, X ) ) ;
}


OSS_INLINE unsigned char * ossAlignX( unsigned char * pAddr, UINT32_64 X )
{
   return ( ( unsigned char * ) ossAlignX( ( UINT32_64 ) pAddr, X ) ) ;
}


OSS_INLINE void * ossAlignX( void * pAddr, UINT32_64 X )
{
   return ( ( void * ) ossAlignX( ( UINT32_64 ) pAddr, X ) ) ;
}

#define ossAlign2(x)  ossAlignX(x, 2)
#define ossAlign4(x)  ossAlignX(x, 4)
#define ossAlign8(x)  ossAlignX(x, 8)
#define ossAlign16(x) ossAlignX(x, 16)
#define ossAlign32(x) ossAlignX(x, 32)
#define ossAlign64(x) ossAlignX(x, 64)
#define ossAlign1K(x) ossAlignX(x, 1024)
#define ossAlign4K(x) ossAlignX(x, 4096)

#endif /* OSSTYPES_HPP_ */

