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

   Source File Name = ixmUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains common function of ixm
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/09/20  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMUTIL_HPP_
#define IXMUTIL_HPP_

#include "ossTypes.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   BOOLEAN        ixmIsTextIndex( const BSONObj& indexDef ) ;

   INT32          ixmGetIndexType( const BSONObj& indexDef,
                                   UINT16 &type ) ;

   ossPoolString  ixmGetIndexTypeDesp( UINT16 type ) ;

   INT32          ixmBuildExtDataName( UINT64 clUniqID,
                                       const CHAR *idxName,
                                       CHAR *extName, UINT32 buffSize ) ;

   BOOLEAN        ixmIsSameDef( const BSONObj &defObj1,
                                const BSONObj &defObj2,
                                BOOLEAN strict = FALSE ) ;

}

#endif

