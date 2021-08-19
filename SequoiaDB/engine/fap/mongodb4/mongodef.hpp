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

   Source File Name = mongodef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_DEFINITION_HPP_
#define _SDB_MONGO_DEFINITION_HPP_

#include <vector>
#include <string>
#include "../../bson/bson.hpp"

#define FAP_CMD_NAME_INSERT                       "insert"
#define FAP_CMD_NAME_DELETE                       "delete"
#define FAP_CMD_NAME_UPDATE                       "update"
#define FAP_CMD_NAME_QUERY                        "query"
#define FAP_CMD_NAME_GETMORE                      "getMore"
#define FAP_CMD_NAME_KILLCURSORS                  "killCursors"
#define FAP_CMD_NAME_MSG                          "msg"

#define FAP_CMD_ADMIN_PREFIX                      "$"
#define FAP_UTIL_SYMBOL_COMMA                     ","
#define FAP_UTIL_SYMBOL_EQUAL                     "="

#define FAP_FIELD_NAME_OK                         "ok"
#define FAP_FIELD_NAME_CODE                       "code"
#define FAP_FIELD_NAME_ERRMSG                     "errmsg"
#define FAP_FIELD_NAME_DB                         "db"
#define FAP_FIELD_NAME_COLLECTION                 "collection"
#define FAP_FIELD_NAME_GROUP                      "group"
#define FAP_FIELD_NAME_SKIP                       "skip"
#define FAP_FIELD_NAME_LIMIT                      "limit"
#define FAP_FIELD_NAME_QUERY                      "query"
#define FAP_FIELD_NAME_HINT                       "hint"
#define FAP_FIELD_NAME_FIND                       "find"
#define FAP_FIELD_NAME_FILTER                     "filter"
#define FAP_FIELD_NAME_PROJECTTION                "projection"
#define FAP_FIELD_NAME_SORT                       "sort"
#define FAP_FIELD_NAME_ORDERED                    "ordered"
#define FAP_FIELD_NAME_DELETE                     "delete"
#define FAP_FIELD_NAME_Q                          "q"
#define FAP_FIELD_NAME_UPSERT                     "upsert"
#define FAP_FIELD_NAME_MULTI                      "multi"
#define FAP_FIELD_NAME_U                          "u"
#define FAP_FIELD_NAME_N                          "n"
#define FAP_FIELD_NAME_V                          "v"
#define FAP_FIELD_NAME_NS                         "ns"
#define FAP_FIELD_NAME_TYPE                       "type"
#define FAP_FIELD_NAME_NAME                       "name"
#define FAP_FIELD_NAME_UPDATE                     "update"
#define FAP_FIELD_NAME_GETMORE                    "getMore"
#define FAP_FIELD_NAME_CURSORS                    "cursors"
#define FAP_FIELD_NAME_PWD                        "pwd"
#define FAP_FIELD_NAME_PIPELINE                   "pipeline"
#define FAP_FIELD_NAME__ID                        "_id"
#define FAP_FIELD_NAME_ID                         "id"
#define FAP_FIELD_NAME_TMP_ID_FIELD               "tmp_id_field"
#define FAP_FIELD_NAME_INDEXES                    "indexes"
#define FAP_FIELD_NAME_INDEX                      "index"
#define FAP_FIELD_NAME_KEY                        "key"
#define FAP_FIELD_NAME_PAYLOAD                    "payload"
#define FAP_FIELD_NAME_DONE                       "done"
#define FAP_FIELD_NAME_DATABASES                  "databases"
#define FAP_FIELD_NAME_FIRSTBATCH                 "firstBatch"
#define FAP_FIELD_NAME_NEXTBATCH                  "nextBatch"
#define FAP_FIELD_NAME_CURSOR                     "cursor"
#define FAP_FIELD_NAME_N_MODIFIED                 "nModified"
#define FAP_FIELD_NAME_N_UPSERTED                 "nUpserted"
#define FAP_FIELD_NAME_ISMASTER                   "ismaster"
#define FAP_FIELD_NAME_MAXBSONOBJECTSIZE          "maxBsonObjectSize"
#define FAP_FIELD_NAME_MAXMESSAGESIZEBYTES        "maxMessageSizeBytes"
#define FAP_FIELD_NAME_MAXWIRE                    "maxWriteBatchSize"
#define FAP_FIELD_NAME_LOCALTIME                  "localTime"
#define FAP_FIELD_NAME_MAXWIREVERSION             "maxWireVersion"
#define FAP_FIELD_NAME_MINWIREVERSION             "minWireVersion"
#define FAP_FIELD_NAME_TOTALLINESWRITTEN          "totalLinesWritten"
#define FAP_FIELD_NAME_LOG                        "log"
#define FAP_FIELD_NAME_VERSION                    "version"
#define FAP_FIELD_NAME_VERSIONARRAY               "versionArray"
#define FAP_FIELD_NAME_VALUES                     "values"
#define FAP_FIELD_NAME_SASLSUPPORTEDMECHS         "saslSupportedMechs"
#define FAP_FIELD_NAME_ZERO                       "0"
#define FAP_FIELD_NAME_MECHANISM                  "mechanism"

#define FAP_AUTH_REPLY_MSG_SYMBOL_RANDOM          "r"
#define FAP_AUTH_REPLY_MSG_SYMBOL_SALT            "s"
#define FAP_AUTH_REPLY_MSG_SYMBOL_ITERATIONCOUNT  "i"
#define FAP_AUTH_REPLY_MSG_SYMBOL_VALUE           "v"

#define FAP_MONGODB_VERSION                       "4.2.2"

#define FAP_AUTH_MSG_PAYLOADD_MAX_SIZE            128

enum mongoOption
{
   dbReply       = 1,
   dbUpdate      = 2001,
   dbInsert      = 2002,
   dbQuery       = 2004,
   dbGetMore     = 2005,
   dbDelete      = 2006,
   dbKillCursors = 2007,
   dbMsg         = 2013,
} ;

enum insertOption
{
   INSERT_CONTINUE_ON_ERROR = 1 << 0,
} ;

enum removeOption
{
   REMOVE_JUSTONE   = 1 << 0,
   REMOVE_BROADCASE = 1 << 1,
} ;

enum updateOption
{
   UPDATE_UPSERT    = 1 << 0,
   UPDATE_MULTI     = 1 << 1,
   UPDATE_BROADCAST = 1 << 2,
} ;

enum queryOption
{
   QUERY_CURSOR_TAILABLE   = 1 << 1,
   QUERY_SLAVE_OK          = 1 << 2,
   QUERY_OPLOG_REPLAY      = 1 << 3,
   QUERY_NO_CURSOR_TIMEOUT = 1 << 4,
   QUERY_AWAIT_DATA        = 1 << 5,
   QUERY_EXHAUST           = 1 << 6,
   QUERY_PARTIAL_RESULTS   = 1 << 7,
   QUERY_ALL_SUPPORTED     = QUERY_CURSOR_TAILABLE | QUERY_SLAVE_OK |
                             QUERY_OPLOG_REPLAY | QUERY_NO_CURSOR_TIMEOUT |
                             QUERY_AWAIT_DATA | QUERY_EXHAUST |
   QUERY_PARTIAL_RESULTS,
} ;

enum ResultFlag
{
   RESULT_CURSOR_NOT_FOUND   = 1,
   RESULT_ERRSET             = 2,
   RESULT_SHARD_CONFIG_STALE = 4,
   RESULT_AWAIT_CAPABLE      = 8,
};

enum authState
{
   AUTH_NONE  = 0,
   AUTH_NONCE = 1,

   AUTH_FINISHED = 1 << 31,
} ;

enum SECTION_TYPE
{
   SECTION_BODY     = 0,
   SECTION_SEQUENCE = 1,
} ;

#pragma pack(1)
struct mongoMsgHeader
{
   INT32  msgLen ;
   INT32  requestId ;
   INT32  responseTo ;
   INT32  opCode ;
};
#pragma pack()

#pragma pack(1)
struct mongoMsgReply
{
   mongoMsgHeader header ;
   INT32 responseFlags ;
   SINT64 cursorId ;
   INT32 startingFrom ;
   INT32 nReturned ;
   // After nReturned, there is a bson::BSONObj document
};
#pragma pack()

struct section
{
   CHAR sectionType ;
   // size = sizeof( UINT32 ) + ( name.length() + 1 ) + documents's size()
   // size and name must be empty if sectionType is 0
   INT32 size ;
   std::string name ;
   std::vector<bson::BSONObj> documents ;
};

#pragma pack(1)
struct mongoOpMsgReply
{
   mongoMsgHeader header ;
   // If the first bit of flags is 0, we don't need to send checksum.
   INT32 flags ;
   CHAR sectionType ;
   // After sectionType, there are a body(BSONObj) or sequences(vector) and
   // checksum(UINT32)
};
#pragma pack()

#define CS_NAME_SIZE       DMS_COLLECTION_SPACE_NAME_SZ
#define CL_FULL_NAME_SIZE  DMS_COLLECTION_SPACE_NAME_SZ +   \
                           DMS_COLLECTION_NAME_SZ + 1
enum
{
   OP_INVALID           = -1,

   OP_INSERT            = 0,
   OP_REMOVE            = 1,
   OP_UPDATE            = 2,
   OP_QUERY             = 3,
   OP_FIND              = 4,
   OP_MSG               = 5,
   OP_GETMORE           = 6,
   OP_KILLCURSORS       = 7,
   OP_CREATE_INDEX      = 8,
   OP_CMD_CREATE        = 9,    // create collection
   OP_CMD_CREATE_CS     = 10,
   OP_CMD_DROP          = 11,   // drop collection
   OP_CMD_DROP_DATABASE = 12,
   OP_CMD_GETLASTERROR  = 13,
   OP_CMD_DROP_INDEX    = 14,
   OP_CMD_GET_INDEX     = 15,
   OP_CMD_GET_CLS       = 16,
   OP_CMD_COUNT         = 17,
   OP_CMD_AGGREGATE     = 18,
   OP_CMD_DISTINCT      = 19,
   OP_CMD_LISTUSER      = 20,
   OP_CMD_ISMASTER      = 21,
   OP_CMD_PING          = 22,
   OP_CMD_NOT_SUPPORTED = 23,
   OP_CMD_WHATSMYURI    = 24,
   OP_CMD_BUILDINFO     = 25,
   OP_CMD_GETLOG        = 26,
   OP_CMD_GET_DBS       = 27,
   OP_CMD_AUTH_STEP1    = 28,
   OP_CMD_AUTH_STEP2    = 29,
   OP_CMD_AUTH_STEP3    = 30,
   OP_CMD_CRTUSER       = 31,
   OP_CMD_DELUSER       = 32,
};

struct cursorStartFrom
{
   INT64 cursorId ;
   INT32 startFrom ;
};

#endif
