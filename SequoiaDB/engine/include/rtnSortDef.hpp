/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

}

#endif

