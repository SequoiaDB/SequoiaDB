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

   Source File Name = mongoReplyHelper.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/05/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoReplyHelper.hpp"
#include "sdbInterface.hpp"
#include "../../bson/bson.hpp"
#include "../../bson/lib/nonce.h"
namespace fap
{
   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::IResource *resource,
                                  engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bob.append( "ismaster", TRUE ) ;
         bob.append("msg", "isdbgrid");
         // build
         // config at last
         bob.append( "maxBsonObjectSize", 16*1024*1024 ) ;
         bob.append( "maxMessageSizeBytes", SDB_MAX_MSG_LENGTH ) ;
         bob.append( "maxWriteBatchSize", 1000 ) ;
         bob.append( "localTime", 100 ) ;
         bob.append( "maxWireVersion", 2 ) ;
         bob.append( "minWireVersion", 2 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetNonceReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         static Nonce::Security security ;
         UINT64 nonce = security.getNonce() ;

         std::stringstream ss ;
         ss << std::hex << nonce ;
         bob.append( "nonce", ss.str() ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetLastErrorReplyMsg( const bson::BSONObj &err,
                                      engine::rtnContextBuf &buff )
      {
         INT32 rc = SDB_OK ;
         bson::BSONObjBuilder bob ;
         rc = err.getIntField( OP_ERRNOFIELD ) ;
         if ( SDB_OK == rc )
         {
            bob.append( "ok", 1.0 ) ;
            bob.appendNull( "err" ) ;
         }
         if ( SDB_OK != rc && !err.isEmpty() )
         {
            bob.append( "ok", 0.0 ) ;
            bob.append( "code", rc ) ;
            bob.append( "errmsg", err.getStringField( OP_ERRDESP_FIELD) ) ;
            bob.append( "err", err.getStringField( OP_ERRDESP_FIELD) ) ;
         }
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const CHAR *cmdName )
      {
         bson::BSONObjBuilder bob ;
         std::string err = "no such cmd:";
         err += cmdName ;
         bob.append( "ok", 0 ) ;
         bob.append( "code", 59 ) ;
         bob.append( "errmsg", err.c_str() ) ;
         bob.append( "err", err.c_str() ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildPingReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob;
         bob.append( "ok", 1 );
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetMoreMsg( msgBuffer &out )
      {
         if ( !out.empty() )
         {
            out.zero() ;
         }
         out.reverse( sizeof( MsgOpGetMore ) ) ;
         out.advance( sizeof( MsgOpGetMore ) ) ;

         MsgOpGetMore *getmore = (MsgOpGetMore *)out.data() ;
         getmore->header.messageLength = sizeof( MsgOpGetMore ) ;
         getmore->header.opCode = MSG_BS_GETMORE_REQ ;
         getmore->header.requestID = 0 ;
         getmore->header.routeID.value = 0 ;
         getmore->header.TID = 0 ;
         getmore->contextID = -1 ;
         getmore->numToReturn = -1 ;
      }
   }
}
