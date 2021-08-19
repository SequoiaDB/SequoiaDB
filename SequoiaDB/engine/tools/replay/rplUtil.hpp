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
#include "ossUtil.hpp"
#include <string>

using namespace std ;

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

   void getCurrentTime( string &timeStr ) ;

   void getCurrentDate( string &dateStr ) ;

   BOOLEAN isSameDay( UINT64 microSecondLeft, UINT64 microSecondRight ) ;

   UINT64 replaceAndGetTime( UINT64 currentTime, INT32 newHour, INT32 newMinite,
                             INT32 newSecond ) ;

   void rplTimestampToString( ossTimestamp &timestamp, string &timeStr ) ;

   void rplTimeIncToString( time_t &timer, UINT32 inc, string &timeStr ) ;
}

#endif /* RPL_UTIL_HPP_ */

