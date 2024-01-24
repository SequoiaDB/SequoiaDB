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

   Source File Name = mongoSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

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
#include "utilUniqueID.hpp"

/////////////////////////////////////////////////////////////////
// implement for mongo processor
_mongoSession::_mongoSession( SOCKET fd, engine::IResource *resource )
   : engine::pmdSession( fd ), _masterRead( FALSE ),
     _authed( FALSE ), _resource( resource )
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
   // reservedFlag should not included in msg header len
   UINT32  headerLen            = sizeof( mongoMsgHeader ) - sizeof( INT32 ) ;
   INT32 bodyLen                = 0 ;
   engine::pmdEDUMgr *pmdEDUMgr = NULL ;
   CHAR *pBuff                  = NULL ;
   const CHAR *pBody            = NULL ;
   const CHAR *pInMsg           = NULL ;
   engine::monDBCB *mondbcb     = engine::pmdGetKRCB()->getMonDBCB() ;
   BOOLEAN handled              = FALSE ;

   if ( !_pEDUCB )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   pmdEDUMgr = _pEDUCB->getEDUMgr() ;
   bigEndian = checkBigEndian() ;

   try
   {
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
                  PD_LOG( PDERROR, "Session[%s] failed to recv rest msg, "
                          "rc: %d", sessionName(), rc ) ;
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
                  PD_LOG( PDERROR, "Failed to convert the msg[opCode: %d] of "
                          "session[%s]", _converter.getParser().currentOption(),
                          sessionName() ) ;
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
               rc = _preProcessMsg( _converter.getParser(),
                                    _resource, _contextBuff, handled ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to preprocess msg, "
                          "rc: %d", sessionName(), rc ) ;
                  goto error ;
               }

               if ( handled )
               {
                  goto reply ;
               }

               pInMsg = _inBuffer.data() ;
               while ( NULL != pInMsg )
               {
                  // process msg
                  rc = _processMsg( pInMsg ) ;
                  if ( SDB_OK == rc )
                  {
                     _authed = TRUE ;
                  }

                  rc = _converter.reConvert( _inBuffer, &_replyHeader ) ;
                  if ( SDB_OK != rc )
                  {
                     goto reply ;
                  }
                  else
                  {
                     // when rc == SDB_OK && _inBuffer is not empty, should
                     // retry to process msg
                     if ( !_inBuffer.empty() )
                     {
                        _contextBuff.release() ;
                        pInMsg = _inBuffer.data() ;
                     }
                     else
                     {
                        // should exit while loop
                        pInMsg = NULL ;
                     }
                  }
               }
            reply:
               rc = _handleResponse( _converter.getOpType(), _contextBuff ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to handle Response, "
                          "rc: %d", sessionName(), rc ) ;
                  goto error ;
               }
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
      }
   }// end while
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Occur exception: %s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

done:
   disconnect() ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_processMsg( const CHAR *pMsg )
{
   INT32 rc  = SDB_OK ;
   INT32 tmp = SDB_OK ;
   INT32 bodyLen = 0 ;
   BOOLEAN needReply = FALSE ;
   BOOLEAN needRollback = FALSE ;
   bson::BSONObjBuilder bob ;
   bson::BSONObjBuilder retBuilder ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;
   const CHAR* commandName = _converter.getParser().command()->name() ;

   rc = _onMsgBegin( (MsgHeader *) pMsg ) ;

   try
   {
      if ( SDB_OK == rc )
      {
         rc = getProcessor()->processMsg( (MsgHeader *) pMsg,
                                          _contextBuff, _replyHeader.contextID,
                                          needReply,
                                          needRollback,
                                          retBuilder ) ;
      }

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

         tmp = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
         // build error msg
         bob.append( "ok", FALSE ) ;
         if ( SDB_IXM_DUP_KEY == tmp )
         {
            // for assert in testcase of c driver for mongodb
            tmp = 11000 ;
         }
         bob.append( "code",  tmp ) ;
         bob.append( "errmsg", _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
         bob.append( "err", _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
         _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
      }
      bodyLen = _contextBuff.size() ;
      _replyHeader.numReturned = _contextBuff.recordNum() ;
      _replyHeader.startFrom = (INT32)_contextBuff.getStartFrom() ;
      _replyHeader.flags = rc ;

      // when msg is with $cmd, need to reply
      // so value of bodyLen cannot be 0
      if ( packet.with( OPTION_CMD ) )
      {
         if ( 0 == bodyLen )
         {
            tmp = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
            if ( SDB_OK != rc )
            {
               // build error msg
               bob.append( "ok", FALSE ) ;
               bob.append( "code",  tmp ) ;
               bob.append( "errmsg",
                           _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
               bob.append( "err",
                           _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
            }
            else
            {
               bob.append( "ok", TRUE ) ;
               bob.appendNull( "err" ) ;
            }

            _contextBuff = engine::rtnContextBuf( bob.obj() ) ;
            _replyHeader.flags = rc ;
         }
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Process command[%s] exception: %s, rc: %d",
               commandName, ex.what(), rc ) ;
      goto error ;
   }

   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *msg )
{
   INT32 rc = SDB_OK ;
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

done:
   return rc ;
error:
   goto done ;
}

void _mongoSession::_onMsgEnd( INT32 result, MsgHeader *msg )
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
}

INT32 _mongoSession::_reply( MsgOpReply *replyHeader,
                             const CHAR *pBody,
                             const INT32 len )
{
   INT32 rc         = SDB_OK ;
   INT32 offset     = 0 ;
   mongoMsgReply reply ;
   bson::BSONObjBuilder bob ;
   bson::BSONObj bsonBody ;
   bson::BSONObj objToSend ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;
   baseCommand *&cmd       = _converter.getParser().command() ;
   const CHAR* commandName = NULL ;

   if ( NULL != cmd )
   {
      commandName = cmd->name() ;
   }

   if ( OP_KILLCURSORS == _converter.getOpType() ||
        dbInsert == packet.opCode ||
        dbUpdate == packet.opCode ||
        dbDelete == packet.opCode )
   {
      // should not send any msg
      goto done;
   }
   // id
   reply.header.requestId = 0 ;//replyHeader->header.requestID ;
   // responseTo, cast UINT64 to INT32
   reply.header.responseTo = packet.requestId ;
   // opCode
   reply.header.opCode = dbReply ;
   // _flags
   reply.header.flags = 0 ;
   // _version
   reply.header.version = 0 ;
   // reservedFlag
   reply.header.reservedFlags = 0 ;
   if ( SDB_AUTH_AUTHORITY_FORBIDDEN == replyHeader->flags )
   {
      reply.header.reservedFlags |= 2 ;
   }
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

   try
   {
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
               if ( !bsonBody.hasField( "ok" ) )
               {
                  bob.append( "ok", 0 == replyHeader->flags ? TRUE : FALSE ) ;
                  bob.append( "code", replyHeader->flags ) ;
                  bob.append( "err",
                              _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
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
               bob.append( "ok", 1.0 ) ;
               objToSend = bob.obj() ;
               _outBuffer.write( objToSend ) ;
            }
         }
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of command[%s] exception: %s, rc: %d",
               commandName ? commandName : "", ex.what(), rc ) ;
      goto error ;
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

INT32 _mongoSession::_preProcessMsg( msgParser &parser,
                                     engine::IResource *resource,
                                     engine::rtnContextBuf &buff,
                                     BOOLEAN &handled )
{
   INT32           rc          = SDB_OK ;
   const CHAR*     commandName = NULL ;
   baseCommand     *&cmd       = parser.command() ;
   mongoDataPacket &packet     = parser.dataPacket() ;
   handled                     = FALSE ;

   if ( NULL != cmd )
   {
      commandName = cmd->name() ;
   }

   try
   {
      if ( OP_CMD_ISMASTER == parser.currentOption() )
      {
         handled = TRUE ;
         fap::mongo::buildIsMasterReplyMsg( resource, buff ) ;
      }
      else if ( OP_CMD_GETNONCE == parser.currentOption() )
      {
         handled = TRUE ;
         bson::BSONObj obj ;
         obj.init( _inBuffer.data() ) ;
         buff = engine::rtnContextBuf( obj ) ;
      }
      else if ( OP_CMD_GETLASTERROR == parser.currentOption() )
      {
         handled = TRUE ;
         fap::mongo::buildGetLastErrorReplyMsg( _errorInfo, buff ) ;
      }
      else if ( OP_CMD_NOT_SUPPORTED == parser.currentOption() )
      {
         handled = TRUE ;
         fap::mongo::buildNotSupportReplyMsg( _contextBuff,
                                   packet.all.firstElementFieldName() ) ;
      }
      else if ( OP_CMD_PING == parser.currentOption() )
      {
          handled = TRUE ;
          fap::mongo::buildPingReplyMsg( _contextBuff ) ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Preprocess command[%s] exception: %s, rc: %d",
               commandName ? commandName : "", ex.what(), rc ) ;
      goto error ;
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

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_handleResponse( const INT32 opType,
                                      engine::rtnContextBuf &buff )
{
   INT32       rc          = SDB_OK ;
   baseCommand *&cmd       = _converter.getParser().command() ;
   const CHAR* commandName = NULL ;

   if ( NULL != cmd )
   {
      commandName = cmd->name() ;
   }

   try
   {
      if ( SDB_AUTH_AUTHORITY_FORBIDDEN == _replyHeader.flags )
      {
         bson::BSONObjBuilder bob ;
         bson::BSONObj obj( buff.data() ) ;
         bob.append( "ok", obj.getIntField( "ok" ) ) ;
         bob.append( "$err", obj.getStringField( "err" ) ) ;
         bob.append( "code", obj.getIntField( "code" ) ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
      }

      if ( OP_CMD_COUNT_MORE == opType )
      {
         bson::BSONObjBuilder bob ;
         bson::BSONObj obj( buff.data() ) ;
         bob.append( "n", obj.getIntField( "Total" ) ) ;
         buff = engine::rtnContextBuf( bob.obj() ) ;
         _replyHeader.contextID = -1 ;
         _replyHeader.startFrom = 0 ;
      }

      if ( OP_CMD_GET_CLS == opType &&
           SDB_DMS_EOC != _replyHeader.flags &&
           _replyHeader.numReturned > 0 )
      {
         INT32 offset = 0 ;
         INT32 len = buff.size() ;
         const CHAR *pBody = buff.data() ;
         INT32 recordNum = buff.recordNum() ;
         bson::BSONObj obj, tmp ;
         _tmpBuffer.zero() ;
         while ( offset < len )
         {
            tmp.init( pBody + offset ) ;
            obj = BSON( "name" << tmp.getStringField( "Name" ) ) ;
            _tmpBuffer.write( obj.objdata(), obj.objsize(), TRUE ) ;
            offset += ossRoundUpToMultipleX( tmp.objsize(), 4 ) ;
         }
         const CHAR *pBuffer = _tmpBuffer.data() ;
         INT32 size = _tmpBuffer.size() ;
         engine::rtnContextBuf ctx( pBuffer, size, recordNum ) ;
         buff = ctx ;
      }

      if ( OP_CMD_GET_INDEX == opType )
      {
         INT32 rNum = buff.recordNum() ;
         INT32 len = buff.size() ;
         const CHAR *pBody = buff.data() ;
         INT32 offset = 0 ;
         bson::BSONObj obj, tmp, indexObj, value ;
         _tmpBuffer.zero() ;
         while ( offset < len )
         {
            tmp.init( pBody + offset ) ;
            obj = tmp.getObjectField( "IndexDef" ) ;
            _tmpBuffer.write( obj.objdata(), obj.objsize(), TRUE ) ;
            offset += ossRoundUpToMultipleX( tmp.objsize(), 4 ) ;
         }
         const CHAR *pBuffer = _tmpBuffer.data() ;
         INT32 size = _tmpBuffer.size() ;
         engine::rtnContextBuf ctx( pBuffer, size, rNum ) ;
         buff = ctx ;
      }

      if ( SDB_DMS_EOC == _replyHeader.flags )
      {
         buff = engine::rtnContextBuf() ;
         _replyHeader.numReturned = 0 ;
         _replyHeader.startFrom = _cursorStartFrom.startFrom ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Handle the response of command[%s] exception: %s, "
               "rc: %d", commandName ? commandName : "", ex.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

