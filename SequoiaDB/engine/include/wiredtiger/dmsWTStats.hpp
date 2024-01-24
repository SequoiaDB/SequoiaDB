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

   Source File Name = dmsWTStats.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STATS_HPP_
#define DMS_WT_STATS_HPP_

#include "dmsDef.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   enum class dmsWTStatsCatalog
   {
      STATS_ALL,
      STATS_SIZE,
      STATS_FAST
   } ;

   OSS_INLINE const CHAR *dmsWTGetStatsConfig( dmsWTStatsCatalog catalog )
   {
      switch ( catalog )
      {
         case dmsWTStatsCatalog::STATS_ALL:
            return "statistics=(all)" ;
         case dmsWTStatsCatalog::STATS_SIZE:
            return "statistics=(size)" ;
         case dmsWTStatsCatalog::STATS_FAST:
            return "statistics=(fast)" ;
         default:
            return "statistics=(all)" ;
      }
   }

}
}

#endif // DMS_WT_ITEM_HPP_
