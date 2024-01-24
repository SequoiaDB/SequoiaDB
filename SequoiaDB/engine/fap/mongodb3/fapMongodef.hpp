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

   Source File Name = fapMongodef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/07/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_DEFINITION_HPP_
#define _SDB_MONGO_DEFINITION_HPP_

#include "../../bson/bson.hpp"
#include "fapMongoMessageDef.hpp"
#include "fapMongoUtil.hpp"

using namespace bson ;

namespace fap
{

enum MONGO_CLIENT_TYPE
{
   NODEJS_DRIVER = 1,
   JAVA_DRIVER   = 2,
   MONGO_SHELL   = 3,
   OTHER         = 255
} ;

struct mongoAuthInfo
{
   string nonce ;
   string identify ;
   string clientProof ;
   INT32 type ;
} ;

struct mongoClientInfo
{
   INT32 version ;
   INT32 subVersion ;
   INT32 fixVersion ;
   MONGO_CLIENT_TYPE type ;
} ;

struct mongoSessionCtx
{
   mongoClientInfo clientInfo ;
   BOOLEAN hasParsedClientInfo ;
   BSONObj errorObj ;
   BSONObj lastErrorObj ;
   string userName ;
   mongoAuthInfo authInfo ;
   const CHAR* sessionName ;
   UINT64 eduID ;

   mongoSessionCtx() : clientInfo(), hasParsedClientInfo( FALSE ) 
   {
      sessionName = NULL ;
   }

   void setError( INT32 errCode, const CHAR* pErrMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      try
      {
         builder.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         builder.append( FAP_MONGO_FIELD_NAME_ERRMSG, pErrMsg ) ;
         builder.append( FAP_MONGO_FIELD_NAME_CODE, errCode ) ;
         errorObj = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when set error obj : %s, "
                 "rc: %d", e.what(), rc ) ;
         errorObj = mongoGetErrorBson( rc ) ;
      }
   }

   void resetError()
   {
      /*
        errorObj can't reset to empty here.
        After each command is executed( we call this command A command ),
        the client will actively execute the isMater command.

        eg: We reset errorObj to empty here.

            1. If we fail to execute A command,
               errorObj = { ok: 0, errmsg: Invalid Argument, code: -6 }
            2. Before executing the isMaster command, we will call
               resetError()
               {
                  // Now errorObj = { ok: 0, errmsg: Invalid Argument, code: -6 }
                  lastErrorObj = errorObj.getOwned()
                  Next we reset errorObj to empty
               }
            3. Executing the isMaster command, but the errorObj doesn't change.
            4. Before executing the next command, we will call
               resetError()
               {
                  // Now errorObj = BSONObj()
                  lastErrorObj = errorObj.getOwned()
                  Next we reset errorObj to empty
               }

            When we execute the getLastError command,
            the result is always empty.
            So we can't reset errorObj to empty here.
      */
      lastErrorObj = errorObj.getOwned() ;
   }

} ;

/* The id of cursor, Mongo client name it "cusorId",
 * Sequoiadb name it "contextId".
 */
#define MONGO_INVALID_CURSORID ( 0 )
#define SDB_INVALID_CONTEXTID  ( -1 )

/* Mongo cursorId: regards 0 as invalid value.
 * Sequoiadb contextId: regards -1 as invalid value.
 * In order to prevent the first context( contextId = 0 ) from being treated as
 * invalid by Mongo client, we convert cursorId = contextId -1.
 */
#define MGCURSOID_TO_SDBCTXID( mongoCursorId ) \
        ( mongoCursorId - 1 )
#define SDBCTXID_TO_MGCURSOID( sdbCtxId ) \
        ( sdbCtxId + 1 )
}
#endif
