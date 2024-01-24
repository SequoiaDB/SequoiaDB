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

   Source File Name = fapMongoCommandDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/21  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_COMMAND_DEF_HPP_
#define _SDB_MONGO_COMMAND_DEF_HPP_

namespace fap
{
enum MONGO_CMD_TYPE
{
   CMD_INSERT    = 1,
   CMD_DELETE    = 2,
   CMD_UPDATE    = 3,

   CMD_QUERY     = 10,
   CMD_FIND      = 11,
   CMD_COUNT     = 12,
   CMD_AGGREGATE = 13,
   CMD_DISTINCT  = 14,
   CMD_FINDANDMODIFY    = 15,

   CMD_GETMORE          = 20,
   CMD_KILL_CURSORS     = 21,

   CMD_DATABASE_DROP    = 30,

   CMD_COLLECTION_CREATE = 40,
   CMD_COLLECTION_DROP   = 41,

   CMD_INDEX_CREATE     = 50,
   CMD_INDEX_DROP       = 51,
   CMD_INDEX_DELETE     = 52,

   CMD_LIST_DATABASE    = 60,
   CMD_LIST_COLLECTION  = 61,
   CMD_LIST_INDEX       = 62,
   CMD_LIST_USER        = 63,

   CMD_AUTHENTICATE     = 70,
   CMD_LOGOUT           = 71,
   CMD_SASL_START       = 72,
   CMD_SASL_CONTINUE    = 73,

   CMD_USER_CREATE      = 80,
   CMD_USER_DROP        = 81,

   CMD_IS_MASTER        = 90,
   CMD_ISMASTER         = 91,
   CMD_PING             = 92,
   CMD_WHATS_MY_URI     = 93,
   CMD_BUILD_INFO       = 94,
   CMD_BUILDINFO        = 95,
   CMD_GET_LOG          = 96,
   CMD_GET_LAST_ERROR   = 97,
   CMD_REPL_STAT        = 98,
   CMD_GET_CMD_LINE     = 99,

   CMD_UNKNOWN          = 65535
} ;

const CHAR* const MONGO_CMD_NAME_INSERT =            "insert" ;
const CHAR* const MONGO_CMD_NAME_DELETE =            "delete" ;
const CHAR* const MONGO_CMD_NAME_UPDATE =            "update" ;
const CHAR* const MONGO_CMD_NAME_QUERY =             "query" ;
const CHAR* const MONGO_CMD_NAME_FIND =              "find" ;
const CHAR* const MONGO_CMD_NAME_COUNT =             "count" ;
const CHAR* const MONGO_CMD_NAME_AGGREGATE =         "aggregate" ;
const CHAR* const MONGO_CMD_NAME_DISTINCT =          "distinct" ;
const CHAR* const MONGO_CMD_NAME_GETMORE =           "getMore" ;
const CHAR* const MONGO_CMD_NAME_KILL_CURSORS =      "killCursors" ;
const CHAR* const MONGO_CMD_NAME_DROP_DATABASE =     "dropDatabase" ;
const CHAR* const MONGO_CMD_NAME_CREATE_COLLECTION = "create" ;
const CHAR* const MONGO_CMD_NAME_DROP_COLLECTION =   "drop" ;
const CHAR* const MONGO_CMD_NAME_CREATE_INDEX =      "createIndexes" ;
const CHAR* const MONGO_CMD_NAME_DELETE_INDEX =      "deleteIndexes" ;
const CHAR* const MONGO_CMD_NAME_DROP_INDEX =        "dropIndexes" ;
const CHAR* const MONGO_CMD_NAME_CREATE_USER =       "createUser" ;
const CHAR* const MONGO_CMD_NAME_DROP_USER =         "dropUser" ;
const CHAR* const MONGO_CMD_NAME_USERS_INFO =        "usersInfo" ;
const CHAR* const MONGO_CMD_NAME_SASL_START =        "saslStart" ;
const CHAR* const MONGO_CMD_NAME_SASL_CONTINUE =     "saslContinue" ;
const CHAR* const MONGO_CMD_NAME_LOGOUT =            "logout" ;
const CHAR* const MONGO_CMD_NAME_LIST_DATABASE =     "listDatabases" ;
const CHAR* const MONGO_CMD_NAME_LIST_COLLECTION =   "listCollections" ;
const CHAR* const MONGO_CMD_NAME_LIST_INDEX =        "listIndexes" ;
const CHAR* const MONGO_CMD_NAME_GET_LAST_ERROR =    "getlasterror" ;
const CHAR* const MONGO_CMD_NAME_IS_MASTER =         "isMaster" ;
const CHAR* const MONGO_CMD_NAME_ISMASTER =          "ismaster" ;
const CHAR* const MONGO_CMD_NAME_WHATS_MY_URI =      "whatsmyuri" ;
const CHAR* const MONGO_CMD_NAME_BUILDINFO =         "buildinfo" ;
const CHAR* const MONGO_CMD_NAME_BUILD_INFO =        "buildInfo" ;
const CHAR* const MONGO_CMD_NAME_GET_LOG =           "getLog" ;
const CHAR* const MONGO_CMD_NAME_PING =              "ping" ;
const CHAR* const MONGO_CMD_NAME_LOGTOUT =           "logout" ;
const CHAR* const MONGO_CMD_NAME_REPL_STAT =         "replSetGetStatus" ;
const CHAR* const MONGO_CMD_NAME_GET_CMD_LINE =      "getCmdLineOpts" ;
const CHAR* const MONGO_CMD_NAME_FINDANDMODIFY =     "findandmodify" ;
const CHAR* const MONGO_CMD_NAME_FIND_AND_MODIFY =   "findAndModify" ;
}
#endif
