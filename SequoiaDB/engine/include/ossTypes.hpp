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

OSS_INLINE UINT32 ossAlignX( UINT32 i, UINT32_64 X  )
{
   return ( ( i + ( X - 1 ) ) & ( ~( ( UINT32 ) ( X - 1 ) ) ) ) ;
}


OSS_INLINE UINT64 ossAlignX( UINT64 i, UINT32_64   X )
{
   return ( ( i + ( X - 1 ) ) & ( ~( ( UINT64 ) ( X - 1 ) ) ) ) ;
}

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

