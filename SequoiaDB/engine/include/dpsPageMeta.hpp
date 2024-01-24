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

   Source File Name = dpsPageMeta.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSPAGEMETA_HPP_
#define DPSPAGEMETA_HPP_

#include "core.hpp"

namespace engine
{
   struct _dpsPageMeta
   {
      INT32 offset ;
      INT32 beginSub ;
      UINT32 pageNum ;
      UINT32 totalLen ;

      _dpsPageMeta()
      :offset(-1),
       beginSub(-1),
       pageNum(0),
       totalLen(0)
      {

      }

      BOOLEAN valid()const
      {
         return -1 != offset &&
                -1 != beginSub &&
                0 != pageNum &&
                0 != totalLen ;
      }

      void clear()
      {
         if ( -1 != offset )
         {
            offset = -1 ;
            beginSub = -1 ;
            pageNum = 0 ;
            totalLen = 0 ;
         }
         SDB_ASSERT( -1 == beginSub &&
                     0 == pageNum &&
                     0 == totalLen, "impossible" ) ;
      }
   } ;
   typedef struct _dpsPageMeta dpsPageMeta ;
}

#endif

