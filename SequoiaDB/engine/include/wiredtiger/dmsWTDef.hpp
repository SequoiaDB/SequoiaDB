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

   Source File Name = dmsWTDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_DEF_HPP_
#define DMS_WT_DEF_HPP_

#include "ossUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   #define DMS_DFT_WT_CACHE_SIZE       ( 2048 )
   #define DMS_MAX_WT_CACHE_SIZE       ( 10 * 1024 * 1024 )
   #define DMS_MIN_WT_CACHE_SIZE       ( 256 )

   #define DMS_DFT_WT_EVICT_TARGET     ( 80 )
   #define DMS_MIN_WT_EVICT_TARGET     ( 10 )
   #define DMS_MAX_WT_EVICT_TARGET     ( 100 )

   #define DMS_DFT_WT_EVICT_TRIGGER    ( 95 )
   #define DMS_MIN_WT_EVICT_TRIGGER    ( 10 )
   #define DMS_MAX_WT_EVICT_TRIGGER    ( 100 )

   #define DMS_DFT_WT_EVICT_DIRTY_TARGET ( 5 )
   #define DMS_MIN_WT_EVICT_DIRTY_TARGET ( 1 )
   #define DMS_MAX_WT_EVICT_DIRTY_TARGET ( 100 )

   #define DMS_DFT_WT_EVICT_DIRTY_TRIGGER ( 20 )
   #define DMS_MIN_WT_EVICT_DIRTY_TRIGGER ( 1 )
   #define DMS_MAX_WT_EVICT_DIRTY_TRIGGER ( 100 )

   #define DMS_DFT_WT_EVICT_UPDATES_TARGET ( 2 )
   #define DMS_MIN_WT_EVICT_UPDATES_TARGET ( 0 )
   #define DMS_MAX_WT_EVICT_UPDATES_TARGET ( 100 )

   #define DMS_DFT_WT_EVICT_UPDATES_TRIGGER ( 10 )
   #define DMS_MIN_WT_EVICT_UPDATES_TRIGGER ( 0 )
   #define DMS_MAX_WT_EVICT_UPDATES_TRIGGER ( 100 )

   #define DMS_DFT_WT_EVICT_THREADS_MIN ( 4 )
   #define DMS_MIN_WT_EVICT_THREADS_MIN ( 1 )
   #define DMS_MAX_WT_EVICT_THREADS_MIN ( 20 )

   #define DMS_DFT_WT_EVICT_THREADS_MAX ( 4 )
   #define DMS_MIN_WT_EVICT_THREADS_MAX ( 1 )
   #define DMS_MAX_WT_EVICT_THREADS_MAX ( 20 )

   #define DMS_DFT_WT_CHECK_POINT_INTERVAL ( 60 )
   #define DMS_MIN_WT_CHECK_POINT_INTERVAL ( 1 )
   #define DMS_MAX_WT_CHECK_POINT_INTERVAL ( OSS_UINT32_MAX )

   #define DMS_WT_FORMART_V1 ( 1 )
   #define DMS_WT_FORMART_VER_CUR ( DMS_WT_FORMART_V1 )

}
}

#endif // DMS_WT_DEF_HPP_
