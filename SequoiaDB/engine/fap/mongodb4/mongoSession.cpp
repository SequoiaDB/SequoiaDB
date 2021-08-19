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

   Source File Name = mongoSession.cpp

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
#include "util.hpp"
#include "mongodef.hpp"
#include "mongoConverter.hpp"
#include "mongoSession.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEDU.hpp"
#include "pmdEnv.hpp"
#include "monCB.hpp"
#include "msg.hpp"
#include "../../bson/bson.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "sdbInterface.hpp"
#include "mongoReplyHelper.hpp"
#include "../bson/lib/base64.h"
#include <sstream>

/////////////////////////////////////////////////////////////////
// implement for mongo processor

_mongoSession::_mongoSession( SOCKET fd, engine::IResource *resource )
   : engine::pmdSession( fd ), _masterRead( FALSE ), _resource( resource )
{
}

_mongoSession::~_mongoSession()
{
   _resource = NULL ;
}

void _mongoSession::_resetBuffers()
{
   // release buff context
   if ( 0 != _contextBuff.size() )
   {
      _contextBuff.release() ;
   }

   if ( !_inBuffer.empty() )
   {
      _inBuffer.zero() ;
   }

   if ( !_outBuffer.empty() )
   {
      _outBuffer.zero() ;
   }
}

INT32 _mongoSession::getServiceType() const
{
   return CMD_SPACE_SERVICE_LOCAL ;
}

engine::SDB_SESSION_TYPE _mongoSession::sessionType() const
{
   return engine::SDB_SESSION_PROTOCOL ;
}

INT32 _mongoSession::run()
{
   INT32 rc                     = SDB_OK ;
   BOOLEAN bigEndian            = FALSE ;
   UINT32 msgSize               = 0 ;
   UINT32  headerLen            = sizeof( mongoMsgHeader ) ;
   INT32 bodyLen                = 0 ;
   engine::pmdEDUMgr *pmdEDUMgr = NULL ;
   CHAR *pBuff                  = NULL ;
   const CHAR *pBody            = NULL ;
   const CHAR *pInMsg           = NULL ;
   INT32 orgOpCode              = 0 ;
   INT32 curOpType              = 0 ;
   engine::monDBCB *mondbcb     = engine::pmdGetKRCB()->getMonDBCB() ;

   if ( !_pEDUCB )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   pmdEDUMgr = _pEDUCB->getEDUMgr() ;
   bigEndian = checkBigEndian() ;
   while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
   {
      // clear interrupt flag
      _pEDUCB->resetInterrupt() ;
      _pEDUCB->resetInfo( engine::EDU_INFO_ERROR ) ;
      _pEDUCB->resetLsn() ;

      // recv msg
      rc = recvData( (CHAR*)&msgSize, sizeof(UINT32) ) ;
      if ( rc )
      {
         if ( SDB_APP_FORCED != rc )
         {
            PD_LOG( PDERROR, "Session[%s] failed to recv msg size, "
                    "rc: %d", sessionName(), rc ) ;
         }
         break ;
      }

      // if big endian, need to convert len to little endian
      if ( bigEndian )
      {
         // build an incompatible msg
         // UINT32 tmp = msgSize ;
         // ossEndianConvert4( tmp, msgSize) ;
      }

      if ( msgSize < headerLen || msgSize > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Session[%s] recv msg size[%d] is less than "
                 "mongoMsgHeader size[%d] or more than max msg size[%d]",
                 sessionName(), msgSize, sizeof( mongoMsgHeader ),
                 SDB_MAX_MSG_LENGTH ) ;
         rc = SDB_INVALIDARG ;
         break ;
      }
      // other msg
      else
      {
         pBuff = getBuff( msgSize + 1 ) ;
         if ( !pBuff )
         {
            rc = SDB_OOM ;
            break ;
         }
         *(UINT32*)pBuff = msgSize ;
         // recv the rest msg
         rc = recvData( pBuff + sizeof(UINT32), msgSize - sizeof(UINT32) ) ;
         if ( rc )
         {
            if ( SDB_APP_FORCED != rc )
            {
               PD_LOG( PDERROR, "Session[%s] failed to recv rest msg, rc: %d",
                       sessionName(), rc ) ;
            }
            break ;
         }
         pBuff[ msgSize ] = 0 ;
         {
            // make sure buffers are empty for coming msg
            _resetBuffers() ;
            // convert msg first
            _converter.loadFrom( pBuff, msgSize ) ;
            rc = _converter.convert( _inBuffer ) ;
            if ( SDB_OK != rc && SDB_OPTION_NOT_SUPPORT != rc)
            {
               goto error ;
            }

            _pEDUCB->incEventCount() ;
            mondbcb->addReceiveNum() ;
            // activate edu
            if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                       sessionName(), rc ) ;
               goto error ;
            }

            // handle commands before dispatched
            if ( _preProcessMsg( _converter.getParser(),
                                 _resource, _contextBuff ) )
            {
               goto reply ;
            }

            pInMsg = _inBuffer.data() ;
            orgOpCode = ((MsgHeader*)pInMsg)->opCode ;
            while ( NULL != pInMsg )
            {
               // process msg
               _processMsg( pInMsg ) ;
               curOpType = _converter.getOpType() ;

               // auto create cs/cl
               if ( SDB_DMS_CS_NOTEXIST == _replyHeader.flags )
               {
                  if ( SDB_OK == _autoCreateCS() )
                  {
                     ((MsgHeader*)pInMsg)->opCode = orgOpCode ;
                     continue ;
                  }
               }
               else if ( SDB_DMS_NOTEXIST == _replyHeader.flags )
               {
                  if ( OP_INSERT == curOpType || OP_CREATE_INDEX == curOpType ||
                       ( OP_UPDATE == curOpType &&
                         ( ((MsgOpUpdate*)_inBuffer.data())->flags &
                           FLG_UPDATE_UPSERT ) ) )
                  {
                     if ( SDB_OK == _autoCreateCL() )
                     {
                        ((MsgHeader*)pInMsg)->opCode = orgOpCode ;
                        continue ;
                     }
                  }
               }

               if ( ( OP_CMD_GET_INDEX == curOpType ||
                      OP_CMD_GET_CLS == curOpType ||
                      OP_CMD_AGGREGATE == curOpType ||
                      OP_CMD_DISTINCT == curOpType ||
                      OP_CMD_COUNT == curOpType ||
                      OP_CMD_GET_DBS == curOpType )
                     && SDB_OK == _replyHeader.flags )
               {
                  if ( 0 == _replyHeader.numReturned &&
                       -1 != _replyHeader.contextID )
                  {
                     _inBuffer.zero() ;
                     fap::mongo::buildGetMoreMsg( _inBuffer ) ;
                     MsgOpGetMore *msg = ( MsgOpGetMore *)_inBuffer.data() ;
                     msg->header.requestID = _replyHeader.header.requestID ;
                     msg->contextID = _replyHeader.contextID ;
                     msg->numToReturn = -1 ;
                     continue ;
                  }
               }

               if ( SDB_OK != _replyHeader.flags )
               {
                  goto reply ;
               }
               else
               {
                  // should exit while loop
                  _inBuffer.zero() ;
                  pInMsg = NULL ;
               }
            }
         reply:
            _handleResponse( _converter.getOpType(), _contextBuff ) ;
            pBody = _contextBuff.data() ;
            bodyLen = _contextBuff.size() ;
            // send response
            INT32 rcTmp = _reply( &_replyHeader, pBody, bodyLen ) ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Session[%s] failed to send response,"
                       "rc: %d", sessionName(), rcTmp ) ;
               goto error ;
            }
            pBody = NULL ;
            bodyLen = 0 ;
            _contextBuff.release() ;

            // wait edu
            if ( SDB_OK != ( rc = pmdEDUMgr->waitEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
                       sessionName(), rc ) ;
               goto error ;
            }
         }
      }
   } // end while
done:
   disconnect() ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_autoCreateCS()
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
   bson::BSONObj obj       = BSON( FIELD_NAME_NAME << packet.csName <<
                                   FIELD_NAME_PAGE_SIZE << 65536 ) ;
   bson::BSONObj empty ;

   _tmpBuffer.zero() ;
   _tmpBuffer.reverse( sizeof( MsgOpQuery ) ) ;
   _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )_tmpBuffer.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = 0 ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   query->nameLength = ossStrlen( cmdName ) ;

   _tmpBuffer.write( cmdName, query->nameLength + 1, TRUE ) ;
   _tmpBuffer.write( obj, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.write( empty, TRUE ) ;
   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)query ) ;

   return rc ;
}

INT32 _mongoSession::_autoCreateCL()
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   bson::BSONObj obj       = BSON( FIELD_NAME_NAME << packet.fullName.c_str() );
   bson::BSONObj empty ;

   while( TRUE )
   {
      _tmpBuffer.zero() ;
      _tmpBuffer.reverse( sizeof( MsgOpQuery ) ) ;
      _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;

      query = ( MsgOpQuery * )_tmpBuffer.data() ;
      query->header.opCode = MSG_BS_QUERY_REQ ;
      query->header.TID = 0 ;
      query->header.routeID.value = 0 ;
      query->header.requestID = 0 ;
      query->version = 0 ;
      query->w = 0 ;
      query->padding = 0 ;
      query->flags = 0 ;
      query->nameLength = ossStrlen( cmdName ) ;
      query->numToSkip = 0 ;
      query->numToReturn = -1 ;

      _tmpBuffer.write( cmdName, query->nameLength + 1, TRUE ) ;
      _tmpBuffer.write( obj, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.write( empty, TRUE ) ;
      _tmpBuffer.doneLen() ;

      rc = _processMsg( (CHAR*)query ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = _autoCreateCS() ;
         if ( rc )
         {
            break ;
         }
      }
      else
      {
         break ;
      }
   }

   return rc ;
}

INT32 _mongoSession::_processMsg( const CHAR *pMsg )
{
   INT32 rc  = SDB_OK ;
   INT32 errCode = SDB_OK ;
   INT32 bodyLen = 0 ;
   BOOLEAN needReply = FALSE ;
   BOOLEAN needRollback = FALSE ;
   bson::BSONObjBuilder bob ;
   bson::BSONObjBuilder retBuilder ;

   _contextBuff.release() ;

   rc = _onMsgBegin( (MsgHeader *) pMsg ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   {
      rc = getProcessor()->processMsg( (MsgHeader *) pMsg,
                                       _contextBuff, _replyHeader.contextID,
                                       needReply,
                                       needRollback,
                                       retBuilder ) ;
      _errorInfo = engine::utilGetErrorBson( rc,
                   _pEDUCB->getInfo( engine::EDU_INFO_ERROR ) ) ;

      if ( SDB_OK != rc )
      {
         if ( needRollback )
         {
            PD_LOG( PDDEBUG, "Session rolling back operation "
                    "(opCode=%d, rc=%d)", ((MsgHeader*)pMsg)->opCode, rc ) ;

            INT32 rcTmp = getProcessor()->doRollback() ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Session failed to rollback trans "
                       "info, rc: %d", rcTmp ) ;
            }
         }

         errCode = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
         // build error msg
         bob.append( FAP_FIELD_NAME_OK, FALSE ) ;
         if ( SDB_IXM_DUP_KEY == errCode )
         {
            // for assert in testcase of c driver for mongodb
            errCode = 11000 ;
         }
         bob.append( FAP_FIELD_NAME_CODE,  errCode ) ;
         bob.append( FAP_FIELD_NAME_ERRMSG,
                     _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
         _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
      }
      bodyLen = _contextBuff.size() ;
      _replyHeader.numReturned = _contextBuff.recordNum() ;
      _replyHeader.startFrom = (INT32)_contextBuff.getStartFrom() ;
      _replyHeader.flags = rc ;
   }

   if ( 0 == bodyLen && -1 != _replyHeader.contextID &&
        _converter.getOpType() == OP_FIND )
   {
      goto done ;
   }

   if ( 0 == bodyLen )
   {
      errCode = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
      if ( SDB_OK != rc )
      {
         bob.append( FAP_FIELD_NAME_OK, FALSE ) ;
         bob.append( FAP_FIELD_NAME_CODE,  errCode ) ;
         bob.append( FAP_FIELD_NAME_ERRMSG,
                     _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
      }
      else
      {
         bob.append( FAP_FIELD_NAME_OK, TRUE ) ;
      }

      _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
      _replyHeader.flags = rc ;
   }

done:
   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *msg )
{
   // set reply header ( except flags, length )
   _replyHeader.contextID          = -1 ;
   _replyHeader.numReturned        = 0 ;
   _replyHeader.startFrom          = 0 ;
   _replyHeader.header.opCode      = MAKE_REPLY_TYPE(msg->opCode) ;
   _replyHeader.header.requestID   = msg->requestID ;
   _replyHeader.header.TID         = msg->TID ;
   _replyHeader.header.routeID     = engine::pmdGetNodeID() ;

   // start operator
   MON_START_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
}

INT32 _mongoSession::_onMsgEnd( INT32 result, MsgHeader *msg )
{
   // release buff context
   //_contextBuff.release() ;

   if ( result && SDB_DMS_EOC != result )
   {
      PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
              "TID: %d, requestID: %llu] failed, rc: %d",
              sessionName(), msg->opCode, msg->messageLength, msg->TID,
              msg->requestID, result ) ;
   }

   // end operator
   MON_END_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
}

INT32 _mongoSession::_reply( MsgOpReply *replyHeader,
                             const CHAR *pBody,
                             const INT32 len )
{
   INT32 rc         = SDB_OK ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;

   if ( dbMsg == packet.opCode )
   {
      rc = _replyOpMsg( replyHeader, pBody, len ) ;
   }
   else
   {
      rc = _replyOpQuery( replyHeader, pBody, len ) ;
   }

   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_replyOpQuery( MsgOpReply *replyHeader,
                                    const CHAR *pBody,
                                    const INT32 len )
{
   INT32 rc = SDB_OK ;
   INT32 offset     = 0 ;
   mongoMsgReply reply ;
   bson::BSONObjBuilder bob ;
   bson::BSONObj bsonBody ;
   bson::BSONObj objToSend ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;

   reply.header.opCode = dbReply ;
   reply.header.requestId = 0 ;
   // responseTo, cast UINT64 to INT32
   reply.header.responseTo = packet.requestId ;
   reply.responseFlags = 0 ;

   // startingFrom
   if ( -1 != replyHeader->contextID )
   {
      _cursorStartFrom.cursorId = reply.cursorId ;
      reply.startingFrom = replyHeader->startFrom ;
      _cursorStartFrom.startFrom = reply.startingFrom
                                   + replyHeader->numReturned ;
   }
   else
   {
      if ( replyHeader->contextID == _cursorStartFrom.cursorId )
      {
         reply.startingFrom = _cursorStartFrom.startFrom ;
      }
      else
      {
         reply.startingFrom = 0;
      }
      // reset cursorStartFrom
      _cursorStartFrom.cursorId = 0 ;
      _cursorStartFrom.startFrom = 0 ;
   }
   //cursorID
   if ( SDB_OK != replyHeader->flags )
   {
      reply.cursorId = 0 ;
   }
   else
   {
      reply.cursorId = replyHeader->contextID + 1 ;
   }

   // nReturn
   if ( packet.with( OPTION_CMD ) &&
        OP_GETMORE != _converter.getOpType() )
   {
      reply.nReturned = ( replyHeader->numReturned > 0 ?
                          replyHeader->numReturned : 1 ) ;
   }
   else
   {
      reply.nReturned = replyHeader->numReturned ;
   }

   if ( reply.nReturned > 1 )
   {
      while ( offset < len )
      {
         bsonBody.init( pBody + offset ) ;
         _outBuffer.write( bsonBody.objdata(), bsonBody.objsize() ) ;
         offset += ossRoundUpToMultipleX( bsonBody.objsize(), 4 ) ;
      }
   }
   else
   {
      if ( pBody )
      {
         if ( 0 == reply.cursorId &&
             ( SDB_OK == _replyHeader.flags &&
               OP_QUERY != _converter.getOpType() ) )
         {
            // error or command
            bsonBody.init( pBody ) ;
            if ( !bsonBody.hasField( FAP_FIELD_NAME_OK ) )
            {
               bob.append( FAP_FIELD_NAME_OK,
                           0 == replyHeader->flags ? TRUE : FALSE ) ;
               bob.append( FAP_FIELD_NAME_CODE, replyHeader->flags ) ;
               bob.appendElements( bsonBody ) ;
               objToSend = bob.obj() ;
               _outBuffer.write( objToSend ) ;
            }
            else
            {
               _outBuffer.write( bsonBody ) ;
            }
         }
         else
         {
            bsonBody.init( pBody ) ;
            _outBuffer.write( bsonBody ) ;
         }
      }
      else
      {
         if ( OP_GETMORE != _converter.getOpType() &&
              OP_CMD_GET_INDEX == _converter.getOpType()  )
         {
            bob.append( FAP_FIELD_NAME_OK, 1 ) ;
            objToSend = bob.obj() ;
            _outBuffer.write( objToSend ) ;
         }
      }
   }

   if ( !_outBuffer.empty() )
   {
      pBody = _outBuffer.data() ;
   }
   reply.header.msgLen = sizeof( mongoMsgReply ) + _outBuffer.size() ;

   rc = sendData( (CHAR *)&reply, sizeof( mongoMsgReply ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Session[%s] failed to send response header, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

   if ( pBody )
   {
      rc = sendData( pBody, reply.header.msgLen - sizeof( mongoMsgReply ) ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to send response body, rc: %d",
                          sessionName(), rc ) ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}


INT32 _mongoSession::_replyOpMsg( MsgOpReply *replyHeader,
                                  const CHAR *pBody,
                                  const INT32 len )
{
   INT32 rc = SDB_OK ;
   bson::BSONObjBuilder bob ;
   bson::BSONObj bsonBody ;
   bson::BSONObj objToSend ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;
   mongoOpMsgReply msgReply ;

   msgReply.header.requestId = 0 ;
   msgReply.header.responseTo = packet.requestId ;
   msgReply.header.opCode = packet.opCode ;
   msgReply.flags = 0 ;

   msgReply.header.msgLen = sizeof( mongoOpMsgReply ) ;

   msgReply.sectionType = SECTION_BODY ;
   if ( pBody )
   {
      objToSend.init( pBody ) ;
   }
   msgReply.header.msgLen += ( objToSend.objsize() ) ;

   _outBuffer.zero() ;
   _outBuffer.write( (CHAR *)&msgReply, sizeof( mongoOpMsgReply ) ) ;
   _outBuffer.write( objToSend ) ;

   rc = sendData( _outBuffer.data(), msgReply.header.msgLen ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Session[%s] failed to send Msg msg, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN _mongoSession::_preProcessMsg( msgParser &parser,
                                       engine::IResource *resource,
                                       engine::rtnContextBuf &buff )
{
   BOOLEAN handled = FALSE ;
   INT32 curOp = parser.currentOperation() ;
   mongoDataPacket &packet = parser.dataPacket() ;

   if ( OP_CMD_ISMASTER == curOp )
   {
      handled = TRUE ;
      fap::mongo::buildIsMasterReplyMsg( buff ) ;
   }
   else if ( OP_CMD_GETLASTERROR == curOp )
   {
      handled = TRUE ;
      fap::mongo::buildGetLastErrorReplyMsg( _errorInfo, buff ) ;
   }
   else if ( OP_CMD_NOT_SUPPORTED == curOp )
   {
      handled = TRUE ;
      fap::mongo::buildNotSupportReplyMsg( _contextBuff,
                                packet.all.firstElementFieldName() ) ;
   }
   else if ( OP_CMD_PING == curOp )
   {
       handled = TRUE ;
       fap::mongo::buildPingReplyMsg( _contextBuff ) ;
   }
   else if ( OP_CMD_WHATSMYURI == curOp )
   {
       handled = TRUE ;
       fap::mongo::buildWhatsmyuriReplyMsg( buff ) ;
   }
   else if ( OP_CMD_GETLOG == curOp )
   {
      handled = TRUE ;
      fap::mongo::buildGetLogReplyMsg( buff ) ;
   }
   else if ( OP_CMD_BUILDINFO == curOp )
   {
       handled = TRUE ;
       fap::mongo::buildBuildinfoReplyMsg( buff ) ;
   }
   else if ( OP_CMD_AUTH_STEP3 == curOp )
   {
       handled = TRUE ;
       fap::mongo::buildAuthStep3ReplyMsg( buff ) ;
   }

   if ( handled )
   {
      // make _relpyHeader
      _replyHeader.contextID            = -1 ;
      _replyHeader.numReturned          = 1 ;
      _replyHeader.startFrom            = 0 ;
      _replyHeader.header.opCode        = MAKE_REPLY_TYPE(packet.opCode) ;
      _replyHeader.header.requestID     = packet.requestId ;
      _replyHeader.header.TID           = 0 ;
      _replyHeader.header.routeID.value = 0 ;
      _replyHeader.flags         = SDB_OK ;
   }

   return handled ;
}

void _mongoSession::_handleResponse( const INT32 opType,
                                     engine::rtnContextBuf &buff )
{
   if ( OP_CMD_COUNT == opType )
   {
      bson::BSONObjBuilder bob ;
      bson::BSONObj obj( buff.data() ) ;
      bob.append( FAP_FIELD_NAME_N, obj.getIntField( FIELD_NAME_TOTAL ) ) ;
      bob.append( FAP_FIELD_NAME_OK, 1 ) ;
      buff = engine::rtnContextBuf( bob.obj() ) ;
      _replyHeader.contextID = -1 ;
      _replyHeader.startFrom = 0 ;
   }

   else if ( OP_CMD_DISTINCT == opType )
   {
      // reply: { values: [ 1, 3, 4 ], ok: 1 }
      bson::BSONObjBuilder bob ;
      if ( SDB_OK == _replyHeader.flags )
      {
         bob.appendElements( BSONObj( buff.data() ) ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
         _replyHeader.contextID = -1 ;
      }
      else if ( SDB_DMS_EOC == _replyHeader.flags )
      {
         bson::BSONArrayBuilder arr(
            bob.subarrayStart( FAP_FIELD_NAME_VALUES ) ) ;
         arr.done() ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
         _replyHeader.contextID = -1 ;
      }
   }

   else if ( OP_CMD_GET_DBS == opType )
   {
      bson::BSONObjBuilder bob ;
      if ( SDB_OK == _replyHeader.flags )
      {
         bson::BSONArrayBuilder arr(
            bob.subarrayStart( FAP_FIELD_NAME_DATABASES ) ) ;
         INT32 offset = 0 ;
         while ( offset < buff.size() )
         {
            bson::BSONObj obj( buff.data() + offset ) ;
            // { Name: "cs" } => { name: "cs" }
            arr.append( BSON( FAP_FIELD_NAME_NAME <<
                        obj.getStringField( FIELD_NAME_NAME ) ) ) ;
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         }
         arr.done() ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;

         buff = engine::rtnContextBuf( bob.obj() ) ;
         _replyHeader.contextID = -1 ;
         _replyHeader.numReturned = 1 ;
      }
      else if ( SDB_DMS_EOC == _replyHeader.flags )
      {
         bson::BSONArrayBuilder arr(
            bob.subarrayStart( FAP_FIELD_NAME_DATABASES ) ) ;
         arr.done() ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
         _replyHeader.contextID = -1 ;
         _replyHeader.numReturned = 1 ;
      }
   }

   else if ( ( OP_FIND == opType || OP_GETMORE == opType ||
             OP_CMD_AGGREGATE == opType || OP_CMD_GET_INDEX == opType
             || OP_CMD_GET_CLS == opType ) &&
             ( SDB_OK == _replyHeader.flags ||
               SDB_DMS_EOC == _replyHeader.flags ) )
   {
      if ( OP_GETMORE != opType )
      {
         _buildFirstBatch( _converter.getParser(), _replyHeader.flags, buff ) ;
      }
      else
      {
         _buildNextBatch( _converter.getParser(), _replyHeader.flags, buff ) ;
      }
   }

   else if ( ( OP_INSERT == opType || OP_REMOVE == opType ||
             OP_UPDATE == opType ) && SDB_OK == _replyHeader.flags )
   {
      bson::BSONObjBuilder bob ;
      bson::BSONObj returnNum( buff.data() ) ;

      bob.append( FAP_FIELD_NAME_OK, 1 ) ;

      if ( OP_INSERT == opType && returnNum.hasField( FIELD_NAME_INSERT_NUM ) )
      {
         bob.append( FAP_FIELD_NAME_N,
                     returnNum.getIntField( FIELD_NAME_INSERT_NUM ) ) ;
      }
      else if ( OP_REMOVE == opType &&
                returnNum.hasField( FIELD_NAME_DELETE_NUM ) )
      {
         bob.append( FAP_FIELD_NAME_N,
                     returnNum.getIntField( FIELD_NAME_DELETE_NUM ) ) ;
      }
      else if ( OP_UPDATE == opType &&
                returnNum.hasField( FIELD_NAME_UPDATE_NUM ) &&
                returnNum.hasField( FIELD_NAME_MODIFIED_NUM ) &&
                returnNum.hasField( FIELD_NAME_INSERT_NUM ) )
      {
         bob.append( FAP_FIELD_NAME_N,
                     returnNum.getIntField( FIELD_NAME_UPDATE_NUM ) ) ;
         bob.append( FAP_FIELD_NAME_N_MODIFIED,
                     returnNum.getIntField( FIELD_NAME_MODIFIED_NUM ) ) ;
         bob.append( FAP_FIELD_NAME_N_UPSERTED,
                     returnNum.getIntField( FIELD_NAME_INSERT_NUM ) ) ;
      }

      buff = engine::rtnContextBuf( bob.obj() ) ;
   }

   else if ( OP_CMD_AUTH_STEP1 == opType && SDB_OK == _replyHeader.flags )
   {
      std::stringstream ss ;
      std::string payload ;
      bson::BSONObjBuilder bob ;
      bson::BSONObj authStep1Reply( buff.data() ) ;
      const CHAR* saltBase64 = NULL ;
      UINT32 iterationCount = 0 ;
      const CHAR* nonceBase64 = NULL ;

      if ( authStep1Reply.isEmpty() )
      {
         bob.append( FAP_FIELD_NAME_CODE, SDB_AUTH_AUTHORITY_FORBIDDEN ) ;
         bob.append( FAP_FIELD_NAME_ERRMSG,
                     getErrDesp( SDB_AUTH_AUTHORITY_FORBIDDEN ) ) ;
         bob.append( FAP_FIELD_NAME_OK, 0 ) ;
      }
      else
      {
         saltBase64 = authStep1Reply.getStringField( SDB_AUTH_SALT ) ;
         iterationCount = authStep1Reply.getIntField( SDB_AUTH_ITERATIONCOUNT ) ;
         nonceBase64 = authStep1Reply.getStringField( SDB_AUTH_NONCE ) ;

         ss << FAP_AUTH_REPLY_MSG_SYMBOL_RANDOM << FAP_UTIL_SYMBOL_EQUAL
            << nonceBase64
            << FAP_UTIL_SYMBOL_COMMA << FAP_AUTH_REPLY_MSG_SYMBOL_SALT
            << FAP_UTIL_SYMBOL_EQUAL << saltBase64
            << FAP_UTIL_SYMBOL_COMMA << FAP_AUTH_REPLY_MSG_SYMBOL_ITERATIONCOUNT
            << FAP_UTIL_SYMBOL_EQUAL << iterationCount ;

         payload = ss.str() ;
         bob.append( FAP_FIELD_NAME_DONE, false ) ;
         bob.append( FAP_FIELD_NAME_MECHANISM, SDB_AUTH_MECHANISM_SS256 ) ;
         bob.appendBinData( FAP_FIELD_NAME_PAYLOAD, payload.length(),
                            bson::BinDataGeneral,
                            payload.c_str() ) ;
         bob.append( FAP_FIELD_NAME_OK, 1 ) ;
      }

      buff = engine::rtnContextBuf( bob.obj() ) ;
   }

   else if ( OP_CMD_AUTH_STEP2 == opType && SDB_OK == _replyHeader.flags )
   {
      std::stringstream ss ;
      std::string payload ;
      bson::BSONObjBuilder bob ;
      bson::BSONObj authStep2Reply( buff.data() ) ;
      const CHAR* serverProofBase64 =
         authStep2Reply.getStringField( SDB_AUTH_PROOF ) ;

      ss << FAP_AUTH_REPLY_MSG_SYMBOL_VALUE << FAP_UTIL_SYMBOL_EQUAL
         << serverProofBase64 ;

      payload = ss.str() ;
      bob.append( FAP_FIELD_NAME_DONE, false ) ;
      bob.appendBinData( FAP_FIELD_NAME_PAYLOAD, payload.length(),
                         bson::BinDataGeneral,
                         payload.c_str() ) ;
      bob.append( FAP_FIELD_NAME_OK, 1 ) ;

      buff = engine::rtnContextBuf( bob.obj() ) ;
   }

}

INT32 _mongoSession::_setSeesionAttr()
{
   INT32 rc = SDB_OK ;
   engine::pmdEDUMgr *pmdEDUMgr = _pEDUCB->getEDUMgr() ;
   const CHAR *cmd = CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR ;
   MsgOpQuery *set = NULL ;

   bson::BSONObj obj ;
   bson::BSONObj emptyObj ;

   msgBuffer msgSetAttr ;
   if ( _masterRead )
   {
      goto done ;
   }

   msgSetAttr.reverse( sizeof( MsgOpQuery ) ) ;
   msgSetAttr.advance( sizeof( MsgOpQuery ) - 4 ) ;
   obj = BSON( FIELD_NAME_PREFERED_INSTANCE << PREFER_REPL_MASTER ) ;
   set = (MsgOpQuery *)msgSetAttr.data() ;

   set->header.opCode = MSG_BS_QUERY_REQ ;
   set->header.TID = 0 ;
   set->header.routeID.value = 0 ;
   set->header.requestID = 0 ;
   set->version = 0 ;
   set->w = 0 ;
   set->padding = 0 ;
   set->flags = 0 ;
   set->nameLength = ossStrlen(cmd) ;
   set->numToSkip = 0 ;
   set->numToReturn = -1 ;

   msgSetAttr.write( cmd, set->nameLength + 1, TRUE ) ;
   msgSetAttr.write( obj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.write( emptyObj, TRUE ) ;
   msgSetAttr.doneLen() ;

   // activate edu
   if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
   {
      PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

   rc = _processMsg( msgSetAttr.data() ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   _masterRead = TRUE ;

   // wait edu
   if ( SDB_OK != ( rc = pmdEDUMgr->waitEDU( _pEDUCB ) ) )
   {
      PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
              sessionName(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

void _mongoSession::_formatConversion ( INT32 opType, BSONObj &obj )
{
   if ( OP_CMD_GET_INDEX == opType )
   {
      bson::BSONObjBuilder formatConversion ;
      obj = obj.getObjectField( IXM_FIELD_NAME_INDEX_DEF ) ;
      formatConversion.append( FAP_FIELD_NAME_V,
                               obj.getIntField( IXM_FIELD_NAME_V ) ) ;

      if ( obj.getBoolField( IXM_FIELD_NAME_UNIQUE ) &&
           obj.getBoolField( IXM_FIELD_NAME_ENFORCED ) )
      {
         formatConversion.append( IXM_FIELD_NAME_UNIQUE,
                                  obj.getBoolField( IXM_FIELD_NAME_UNIQUE ) ) ;
      }

      formatConversion.append( IXM_FIELD_NAME_KEY,
                               obj.getObjectField( IXM_FIELD_NAME_KEY ) ) ;
      formatConversion.append( IXM_FIELD_NAME_NAME,
                               obj.getStringField( IXM_FIELD_NAME_NAME ) ) ;
      formatConversion.append( FAP_FIELD_NAME_NS,
               _converter.getParser().dataPacket().fullName.c_str() ) ;
      obj = formatConversion.obj() ;
   }

   else if ( OP_CMD_GET_CLS == opType )
   {
      // { Name: "foo.bar" } => { name: "bar", type: "collection" }
      const CHAR* clFullName = obj.getStringField( FIELD_NAME_NAME ) ;
      const CHAR* clShortName = ossStrstr( clFullName, "." ) + 1 ;
      obj = BSON( FAP_FIELD_NAME_NAME << clShortName <<
                  FAP_FIELD_NAME_TYPE << FAP_FIELD_NAME_COLLECTION ) ;
   }

}

void  _mongoSession::_buildFirstBatch( msgParser &parser, INT32 errCode,
                                       engine::rtnContextBuf &replyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { firstBatch: [ {xxx}, {xxx}... ], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   mongoDataPacket packet = parser.dataPacket() ;
   BSONObjBuilder resultBuilder ;
   BSONObjBuilder cursorBuilder ;
   INT32 opType = parser.currentOperation() ;
   string ns ;

   bson::BSONArrayBuilder arr(
      cursorBuilder.subarrayStart( FAP_FIELD_NAME_FIRSTBATCH ) ) ;
   if ( SDB_DMS_EOC == errCode )
   {
      // do nothing
   }
   else
   {
      INT32 offset = 0 ;
      while ( offset < replyBuf.size() )
      {
         BSONObj obj( replyBuf.data() + offset ) ;
         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         _formatConversion( opType, obj ) ;
         arr.append( obj ) ;
      }
   }
   arr.done() ;

   if ( OP_CMD_GET_INDEX == opType )
   {
      // listIndexes reply:   { ... ns: "foo.$cmd.listIndexes.bar" ... }
      ns = packet.csName ;
      ns += ".$cmd.listIndexes." ;
      ns += packet.clName ;
   }
   else if ( OP_CMD_GET_CLS == opType )
   {
      // listCL reply:   { ... ns: "foo.$cmd.listCollections" ... }
      ns = packet.csName ;
      ns += ".$cmd.listCollections" ;
   }
   else
   {
      // real query reply:   { ... ns: "foo.bar" ... }
      ns = packet.fullName ;
   }

   cursorBuilder.append( FAP_FIELD_NAME_NS, ns.c_str() ) ;
   cursorBuilder.append( FAP_FIELD_NAME_ID,
                         (INT64)( _replyHeader.contextID + 1 ) ) ;
   resultBuilder.append( FAP_FIELD_NAME_CURSOR, cursorBuilder.obj() ) ;
   resultBuilder.append( FAP_FIELD_NAME_OK, 1 ) ;
   replyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;

}

void  _mongoSession::_buildNextBatch( msgParser &parser, INT32 errCode,
                                      engine::rtnContextBuf &replyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { nextBatch: [{xxx}, {xxx}...], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   mongoDataPacket packet = parser.dataPacket() ;
   bson::BSONObjBuilder resultBuilder ;
   bson::BSONObjBuilder cursorBuilder ;

   bson::BSONArrayBuilder arr( cursorBuilder.subarrayStart( "nextBatch" ) ) ;
   INT32 offset = 0 ;
   if ( SDB_DMS_EOC == errCode )
   {
      // do nothing
   }
   else
   {
      while ( offset < replyBuf.size() )
      {
         bson::BSONObj obj( replyBuf.data() + offset ) ;
         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         arr.append( obj ) ;
      }
   }
   arr.done() ;

   cursorBuilder.append( FAP_FIELD_NAME_NS, packet.fullName.c_str() ) ;
   cursorBuilder.append( FAP_FIELD_NAME_ID,
                         (INT64)( _replyHeader.contextID + 1 ) ) ;
   resultBuilder.append( FAP_FIELD_NAME_CURSOR, cursorBuilder.obj() ) ;
   resultBuilder.append( FAP_FIELD_NAME_OK, 1 ) ;
   replyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;

}