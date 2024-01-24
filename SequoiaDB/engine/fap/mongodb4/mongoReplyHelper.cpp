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

   Source File Name = mongoReplyHelper.cpp

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
#include "mongoReplyHelper.hpp"
#include "sdbInterface.hpp"
#include "../../bson/bson.hpp"
#include "../../bson/lib/nonce.h"

namespace fap
{
   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bob.append( FAP_FIELD_NAME_ISMASTER, TRUE ) ;
         bob.append( FAP_FIELD_NAME_MAXBSONOBJECTSIZE, 16*1024*1024 ) ;
         bob.append( FAP_FIELD_NAME_MAXMESSAGESIZEBYTES, SDB_MAX_MSG_LENGTH ) ;
         bob.append( FAP_FIELD_NAME_MAXWIRE, 1000 ) ;
         bob.append( FAP_FIELD_NAME_LOCALTIME, 100 ) ;
         bob.append( FAP_FIELD_NAME_MAXWIREVERSION, FAP_MAX_WIRE_VERSION ) ;
         bob.append( FAP_FIELD_NAME_MINWIREVERSION, FAP_MIN_WIRE_VERSION ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
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
            bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         }
         if ( SDB_OK != rc && !err.isEmpty() )
         {
            bob.append( FAP_FIELD_NAME_OK, 0 ) ;
            bob.append( FAP_FIELD_NAME_CODE, rc ) ;
            bob.append( FAP_FIELD_NAME_ERRMSG,
                        err.getStringField( OP_ERRDESP_FIELD) ) ;
         }
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const CHAR *cmdName )
      {
         bson::BSONObjBuilder bob ;
         std::string err = "no such cmd:";
         err += cmdName ;
         bob.append( FAP_FIELD_NAME_OK, 0 ) ;
         bob.append( FAP_FIELD_NAME_CODE, 59 ) ;
         bob.append( FAP_FIELD_NAME_ERRMSG, err.c_str() ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildPingReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob;
         bob.append( FAP_FIELD_NAME_OK, 1 );
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

      void buildWhatsmyuriReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         bob.append( FAP_FIELD_NAME_ERRMSG, "" ) ;
         bob.append( FAP_FIELD_NAME_CODE, 0 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildGetLogReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bson::BSONArrayBuilder sub( bob.subarrayStart( FAP_FIELD_NAME_LOG ) ) ;
         sub.done() ;
         bob.append( FAP_FIELD_NAME_TOTALLINESWRITTEN, 0 ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      void buildBuildinfoReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         bob.append( FAP_FIELD_NAME_VERSION, FAP_MONGODB_VERSION ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         bson::BSONArrayBuilder sub(
            bob.subarrayStart( FAP_FIELD_NAME_VERSIONARRAY ) ) ;
         sub.append( 4 ) ;
         sub.append( 2 ) ;
         sub.append( 2 ) ;
         sub.append( 0 ) ;
         sub.done() ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      /** \fn buildAuthStep3ReplyMsg( engine::rtnContextBuf &buff )
          \brief Build a response message for the third authentication of
                 authentication.
          \param [out] buff Message we will build.
          \retval SDB_OK Operation Success
          \retval Others Operation Fail
      */
      void buildAuthStep3ReplyMsg( engine::rtnContextBuf &buff )
      {
         bson::BSONObjBuilder bob ;
         string payload = "" ;
         bob.append( FAP_FIELD_NAME_DONE, true ) ;
         bob.appendBinData( FAP_FIELD_NAME_PAYLOAD, payload.length(),
                            bson::BinDataGeneral,
                            payload.c_str() ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

   }
}
