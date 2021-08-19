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

   Source File Name = ossLatch.cpp

   Descriptive Name = Operating System Services Latch

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for latch operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossLatch.hpp"

void ossLatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      latch->get_shared () ;
   else
      latch->get () ;
}
void ossLatch ( ossXLatch *latch )
{
   latch->get () ;
}
void ossUnlatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      latch->release_shared () ;
   else
      latch->release();
}
void ossUnlatch ( ossXLatch *latch )
{
   latch->release () ;
}
BOOLEAN ossTestAndLatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      return latch->try_get_shared () ;
   else
      return latch->try_get();
}
BOOLEAN ossTestAndLatch ( ossXLatch *latch )
{
   return latch->try_get () ;
}

