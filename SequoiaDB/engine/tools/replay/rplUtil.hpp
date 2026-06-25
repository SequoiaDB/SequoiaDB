/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
#include "../bson/bsontypes.h"
#include <string>

namespace bson { class BSONElement; }

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
   #define RPL_LOG_OP_RETURN           "return"

   CHAR* getOPName(UINT16 type);

   void getCurrentTime( string &timeStr ) ;

   void getCurrentDate( string &dateStr ) ;

   BOOLEAN isSameDay( UINT64 microSecondLeft, UINT64 microSecondRight ) ;

   UINT64 replaceAndGetTime( UINT64 currentTime, INT32 newHour, INT32 newMinite,
                             INT32 newSecond ) ;

   void rplTimestampToString( ossTimestamp &timestamp, string &timeStr ) ;

   void rplDateToString( INT64 millis, string &dateStr ) ;

   INT32 rplCheckFloatFmt( const CHAR *pFloatFmt ) ;

   void rplDoubleToString( FLOAT64 val, BOOLEAN fullPrecision,
                           const CHAR *floatFmt, string &outStr ) ;

   INT32 rplCastNumberToString( const bson::BSONElement &ele,
                                bson::BSONType targetType,
                                string &outStr ) ;

   void rplTimeIncToString( time_t &timer, UINT32 inc, string &timeStr ) ;
}

#endif /* RPL_UTIL_HPP_ */

