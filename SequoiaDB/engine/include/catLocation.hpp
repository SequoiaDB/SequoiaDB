/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = catLocation.hpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/20/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_LOCATION_HPP__
#define CAT_LOCATION_HPP__

#include "msgCatalogDef.h"
#include "clsDef.hpp"

namespace engine
{

   INT32 catGetLocationInfo( const BSONObj &groupObj,
                             CLS_LOC_INFO_MAP *pLocationInfo ) ;

   INT32 catGetLocationID( const BSONObj &groupObj,
                           const CHAR *pLocation,
                           UINT32 &locationID ) ;

   INT32 catCheckAndGetActiveLocation( const BSONObj &groupObj,
                                       const UINT32 groupID,
                                       const ossPoolString &newActLoc,
                                       ossPoolString &oldActLoc ) ;
}

#endif // CAT_LOCATION_HPP__