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

   Source File Name = dmsCachedPlanUnit.cpp

   Descriptive Name = DMS Cached Access Plan Units

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   management of cached access plans of collections.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsCachedPlanUnit.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsCLCachedPlanUnit implement
    */
   _dmsCLCachedPlanUnit::_dmsCLCachedPlanUnit ()
   : _utilSUCacheUnit()
   {
      _paramInvalidCount = 0 ;
      _mainCLInvalidCount = 0 ;
   }

   _dmsCLCachedPlanUnit::_dmsCLCachedPlanUnit ( UINT16 mbID, UINT64 createTime )
   : _utilSUCacheUnit( mbID, createTime )
   {
      _paramInvalidCount = 0 ;
      _mainCLInvalidCount = 0 ;
   }

   _dmsCLCachedPlanUnit::~_dmsCLCachedPlanUnit ()
   {
   }

}

