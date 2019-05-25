/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rplUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RPL_UTIL_HPP_
#define RPL_UTIL_HPP_

#include "ossTypes.hpp"

namespace replay
{
   #define RPL_LOG_OP_INSERT           "insert"
   #define RPL_LOG_OP_UPDATE           "update"
   #define RPL_LOG_OP_DELETE           "delete"
   #define RPL_LOG_OP_CREATE_CS        "createcs"
   #define RPL_LOG_OP_DELETE_CS        "deletecs"
   #define RPL_LOG_OP_CREATE_CL        "createcl"
   #define RPL_LOG_OP_DELETE_CL        "deletecl"
   #define RPL_LOG_OP_TRUNCATE_CL      "truncatecl"
   #define RPL_LOG_OP_CREATE_IX        "createix"
   #define RPL_LOG_OP_DELETE_IX        "deleteix"
   #define RPL_LOG_OP_LOB_WRITE        "lobwrite"
   #define RPL_LOG_OP_LOB_REMOVE       "lobremove"
   #define RPL_LOG_OP_LOB_UPDATE       "lobupdate"
   #define RPL_LOG_OP_LOB_TRUNCATE     "lobtruncate"
   #define RPL_LOG_OP_DUMMY            "dummy"
   #define RPL_LOG_OP_CL_RENAME        "renamecl"
   #define RPL_LOG_OP_TS_COMMIT        "commit"
   #define RPL_LOG_OP_TS_ROLLBACK      "rollback"
   #define RPL_LOG_OP_INVALIDATE_CATA  "invalidatecata"
   #define RPL_LOG_OP_CS_RENAME        "renamecs"
   #define RPL_LOG_OP_POP              "pop"

   CHAR* getOPName(UINT16 type);
}

#endif /* RPL_UTIL_HPP_ */

