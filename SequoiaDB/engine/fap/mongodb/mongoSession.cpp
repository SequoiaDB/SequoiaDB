/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
   UINT32  headerLen            = sizeof( mongoMsgHeader ) - sizeof( INT32 ) ;
   INT32 bodyLen                = 0 ;
   engine::pmdEDUMgr *pmdEDUMgr = NULL ;
   CHAR *pBuff                  = NULL ;
   const CHAR *pBody            = NULL ;
   const CHAR *pInMsg           = NULL ;
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
      _pEDUCB->resetInterrupt() ;
      _pEDUCB->resetInfo( engine::EDU_INFO_ERROR ) ;
      _pEDUCB->resetLsn() ;

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

      if ( bigEndian )
      {
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
      else
      {
         pBuff = getBuff( msgSize + 1 ) ;
         if ( !pBuff )
         {
            rc = SDB_OOM ;
            break ;
         }
         *(UINT32*)pBuff = msgSize ;
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
            _resetBuffers() ;
            _converter.loadFrom( pBuff, msgSize ) ;
            rc = _converter.convert( _inBuffer ) ;
            if ( SDB_OK != rc && SDB_OPTION_NOT_SUPPORT != rc)
            {
               goto error ;
            }

            _pEDUCB->incEventCount() ;
            mondbcb->addReceiveNum() ;
            if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                       sessionName(), rc ) ;
               goto error ;
            }

            if ( _preProcessMsg( _converter.getParser(),
                                 _resource, _contextBuff ) )
            {
               goto reply ;
            }

            pInMsg = _inBuffer.data() ;
            while ( NULL != pInMsg )
            {
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
                  if ( !_inBuffer.empty() )
                  {
                     _contextBuff.release() ;
                     pInMsg = _inBuffer.data() ;
                  }
                  else
                  {
                     pInMsg = NULL ;
                  }
               }
            }
         reply:
            _handleResponse( _converter.getOpType(), _contextBuff ) ;
            pBody = _contextBuff.data() ;
            bodyLen = _contextBuff.size() ;
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

INT32 _mongoSession::_processMsg( const CHAR *pMsg )
{
   INT32 rc  = SDB_OK ;
   INT32 tmp = SDB_OK ;
   INT32 bodyLen = 0 ;
   BOOLEAN needReply = FALSE ;
   bson::BSONObjBuilder bob ;
   mongoDataPacket &packet = _converter.getParser().dataPacket() ;

   rc = _onMsgBegin( (MsgHeader *) pMsg ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   {
      rc = getProcessor()->processMsg( (MsgHeader *) pMsg,
                                       _contextBuff, _replyHeader.contextID,
                                       needReply ) ;
      _errorInfo = engine::utilGetErrorBson( rc,
                   _pEDUCB->getInfo( engine::EDU_INFO_ERROR ) ) ;
      if ( SDB_OK != rc )
      {
         tmp = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
         bob.append( "ok", FALSE ) ;
         if ( SDB_IXM_DUP_KEY == tmp )
         {
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
   }

   if ( packet.with( OPTION_CMD ) )
   {
      if ( 0 == bodyLen )
      {
         tmp = _errorInfo.getIntField( OP_ERRNOFIELD ) ;
         if ( SDB_OK != rc )
         {
            bob.append( "ok", FALSE ) ;
            bob.append( "code",  tmp ) ;
            bob.append( "errmsg", _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
            bob.append( "err", _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
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

   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *msg )
{
   _replyHeader.contextID          = -1 ;
   _replyHeader.numReturned        = 0 ;
   _replyHeader.startFrom          = 0 ;
   _replyHeader.header.opCode      = MAKE_REPLY_TYPE(msg->opCode) ;
   _replyHeader.header.requestID   = msg->requestID ;
   _replyHeader.header.TID         = msg->TID ;
   _replyHeader.header.routeID     = engine::pmdGetNodeID() ;

   MON_START_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
}

INT32 _mongoSession::_onMsgEnd( INT32 result, MsgHeader *msg )
{

   if ( result && SDB_DMS_EOC != result )
   {
      PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
              "TID: %d, requestID: %llu] failed, rc: %d",
              sessionName(), msg->opCode, msg->messageLength, msg->TID,
              msg->requestID, result ) ;
   }

   MON_END_OP( _pEDUCB->getMonAppCB() ) ;

   return SDB_OK ;
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

   if ( OP_KILLCURSORS == _converter.getOpType() ||
        dbInsert == packet.opCode ||
        dbUpdate == packet.opCode ||
        dbDelete == packet.opCode )
   {
      goto done;
   }
   reply.header.requestId = 0 ;//replyHeader->header.requestID ;
   reply.header.responseTo = packet.requestId ;
   reply.header.opCode = dbReply ;
   reply.header.flags = 0 ;
   reply.header.version = 0 ;
   reply.header.reservedFlags = 0 ;
   if ( SDB_AUTH_AUTHORITY_FORBIDDEN == replyHeader->flags )
   {
      reply.header.reservedFlags |= 2 ;
   }
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
      _cursorStartFrom.cursorId = 0 ;
      _cursorStartFrom.startFrom = 0 ;
   }
   if ( SDB_OK != replyHeader->flags )
   {
      reply.cursorId = 0 ;
   }
   else
   {
      reply.cursorId = replyHeader->contextID + 1 ;
   }

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
            bsonBody.init( pBody ) ;
            if ( !bsonBody.hasField( "ok" ) )
            {
               bob.append( "ok", 0 == replyHeader->flags ? TRUE : FALSE ) ;
               bob.append( "code", replyHeader->flags ) ;
               bob.append( "err", _errorInfo.getStringField( OP_ERRDESP_FIELD) ) ;
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

BOOLEAN _mongoSession::_preProcessMsg( msgParser &parser,
                                       engine::IResource *resource,
                                       engine::rtnContextBuf &buff )
{
   BOOLEAN handled = FALSE ;
   mongoDataPacket &packet = parser.dataPacket() ;

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

   if ( handled )
   {
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
