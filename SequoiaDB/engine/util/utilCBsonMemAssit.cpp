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

   Source File Name = utilCBsonMemAssit.cpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/05/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilMemListPool.hpp"
#include "../client/bson/bson.h"

namespace engine
{

   class _utilCBsonMemAssist
   {
      public:
         _utilCBsonMemAssist()
         {
            bson_set_malloc_func( (bson_malloc_func_p)utilTCAlloc ) ;
            bson_set_realloc_func( (bson_realloc_func_p)utilTCRealloc ) ;
            bson_set_free_func( (bson_free_func_p)utilTCRelease ) ;
         }
   } ;

   _utilCBsonMemAssist s_tmpAssist ;

}

