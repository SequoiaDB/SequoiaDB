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

   Source File Name = utilLocation.cpp

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

#include "utilLocation.hpp"
#include "ossUtil.hpp"

namespace engine
{
   BOOLEAN utilCalAffinity( const CHAR *location1, const CHAR *location2 )
   {
      BOOLEAN ret = FALSE ;
      const CHAR *pos1 = NULL, *pos2 = NULL ;
      UINT32 prefixLen1 = 0, prefixLen2 = 0 ;
      UINT32 locationLen1 = 0, locationLen2 = 0 ;

      if ( NULL == location1 || 0 == location1[0] ||
           NULL == location2 || 0 == location2[0] )
      {
         goto done ;
      }

      locationLen1 = ossStrlen( location1 ) ;
      locationLen2 = ossStrlen( location2 ) ;
      pos1 = ossStrchr( location1, '.' ) ;
      pos2 = ossStrchr( location2, '.' ) ;
      prefixLen1 = NULL != pos1 ? pos1 - location1 : 0 ;
      prefixLen2 = NULL != pos2 ? pos2 - location2 : 0 ;

      // e.g. "A" and "A"
      if ( NULL == pos1 && NULL == pos2 &&
           locationLen1 == locationLen2 &&
           0 == ossStrncasecmp( location1, location2, locationLen1 ) )
      {
         ret = TRUE ;
      }
      // e.g. "A" and "A.a"
      else if ( ( NULL == pos1 && prefixLen2 == locationLen1 &&
                  0 == ossStrncasecmp( location1, location2, prefixLen2 ) ) ||
                ( NULL == pos2 && prefixLen1 == locationLen2 &&
                  0 == ossStrncasecmp( location2, location1, prefixLen1 ) ) )
      {
         ret = TRUE ;
      }
      // e.g. "A.a" and "A.b"
      else if ( NULL != pos1 && NULL != pos2 &&
                prefixLen1 == prefixLen2 &&
                0 != prefixLen1 &&
                0 == ossStrncasecmp( location1, location2, prefixLen1 ) )
      {
         ret = TRUE ;
      }

   done:
      return ret ;
   }
}
