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

   Source File Name = rtnSortDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSORTDEF_HPP_
#define RTNSORTDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
   enum RTN_SORT_STEP
   {
      RTN_SORT_STEP_BEGIN = 0,
      RTN_SORT_STEP_FETCH_FROM_INTER,
      RTN_SORT_STEP_FETCH_FROM_MERGE,
   } ;

  const UINT32 RTN_SORT_MIN_BUFSIZE = 128 ;
  const UINT32 RTN_SORT_MIN_MERGESIZE = 3 ;
  const UINT32 RTN_SORT_MAX_MERGESIZE = 10 ;

  // Tuple directory(store tuple pointers) and tuple space are used by internal
  // sorting. Memory usages by them may increase if necessary.
  const UINT32 RTN_SORT_TUPLE_DIR_MIN_SZ = 65536 ;
  const UINT32 RTN_SORT_TUPLE_MIN_SZ = 524288 ;
}

#endif

