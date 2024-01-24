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

   Source File Name = utilLocation.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2022/12/20  HYQ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LOCATION_HPP_
#define UTIL_LOCATION_HPP_

#include "core.hpp"

namespace engine
{
   BOOLEAN utilCalAffinity( const CHAR *location1, const CHAR *location2 ) ;

   struct _utilLocationInfo
   {
      UINT8 primaryLocationNodes ;
      UINT8 locations ;
      UINT8 affinitiveLocations ;
      UINT8 affinitiveNodes ;

      _utilLocationInfo()
      {
         primaryLocationNodes = 0 ;
         locations = 0 ;
         affinitiveLocations = 0 ;
         affinitiveNodes = 0 ;
      }
   } ;

   typedef _utilLocationInfo utilLocationInfo ;
}

#endif // UTIL_LOCATION_HPP_