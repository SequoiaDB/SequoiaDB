/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = msgConvertorImpl.cpp

   Descriptive Name = Message convertor implementation

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains message structure for
   client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "msgConvertorImpl.hpp"
#include "pdTrace.hpp"
#include "msgTrace.hpp"
#include "ossUtil.hpp"

// The threshold to trigger buffer shrink in the message convertor: 64MB.
#define MSG_CVT_SHRINK_THRESHOLD       (64 * 1024 * 1024)

namespace engine
{
   _msgConvertorImpl::_msgConvertorImpl()
   : _action( CONVERT_ACTION_NONE ),
     _isReply( FALSE ),
     _isDone( TRUE ),
     _convertBuff( NULL ),
     _expectLen( 0 ),
     _pushLen( 0 ),
     _buffSize( 0 ),
     _writePos( 0 )
   {
   }

   _msgConvertorImpl::~_msgConvertorImpl()
   {
      if ( _convertBuff )
      {
         SDB_OSS_FREE( _convertBuff ) ;
         _convertBuff = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL_RESET, "_msgConvertorImpl::reset" )
   void _msgConvertorImpl::reset( BOOLEAN releaseBuff )
   {
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL_RESET ) ;
      _msgPieces.clear() ;
      _action = CONVERT_ACTION_NONE ;
      _isReply = FALSE ;
      _isDone = TRUE ;
      _expectLen = 0 ;
      _pushLen = 0 ;
      _writePos = 0 ;

      if ( _convertBuff )
      {
         if ( releaseBuff )
         {
            SDB_OSS_FREE( _convertBuff ) ;
            _convertBuff = NULL ;
            _buffSize = 0 ;
         }
         else
         {
            ossMemset( _convertBuff, 0, _buffSize ) ;
         }
      }
      PD_TRACE_EXIT( SDB__MSGCONVERTORIMPL_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL_PUSH, "_msgConvertorImpl::push" )
   INT32 _msgConvertorImpl::push( const CHAR *data, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL_PUSH ) ;

      if ( !data || ( 0 == size ) )
      {
         SDB_ASSERT( FALSE, "Data/size to push is invalid" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Data or size to be pushed into the message "
                 "convertor is invalid[%d]", rc ) ;
         goto error ;
      }

      // The system info message should not be pushed into the convertor.
      SDB_ASSERT( size != (UINT32)MSG_SYSTEM_INFO_LEN,
                  "Message size is invalid" ) ;

      // If peer version is the same with local, no conversion is required.
      // The first part being pushed is sure to be a message header. It has the
      // totoal message length at the beginning of the message.
      if ( _msgPieces.empty() )
      {
         rc = _pushHeader( data, size ) ;
         PD_RC_CHECK( rc, PDERROR, "Push message header into message convertor "
                      "failed[%d]", rc ) ;
         _expectLen = *(UINT32 *)data ;
      }
      else
      {
         rc = _pushBody( data, size ) ;
         PD_RC_CHECK( rc, PDERROR, "Push message body into message convertor "
                      "failed[%d]", rc ) ;
      }
      _pushLen += size ;

      SDB_ASSERT( _pushLen <= _expectLen,
                  "Length of pushed message is not as expected" ) ;

      // The total message has been pushed into convertor. Let's do the
      // conversion.
      if ( _pushLen == _expectLen )
      {
         rc = _doit() ;
         PD_RC_CHECK( rc, PDERROR, "Do message conversion failed[%d]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL_PUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL_OUTPUT, "_msgConvertorImpl::output" )
   INT32 _msgConvertorImpl::output( CHAR *&data, UINT32 &len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL_OUTPUT ) ;
      data = NULL ;
      len = 0 ;

      if ( !_isDone )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "The conversion of the message is not done[%d]",
                 rc ) ;
         goto error ;
      }

      if ( _msgPieces.size() > 0 )
      {
         data = (CHAR *)_msgPieces.front().iovBase ;
         len = _msgPieces.front().iovLen ;
         _msgPieces.pop_front() ;
         SDB_ASSERT( data && ( len > 0 ), "Message piece is invalid" ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL_OUTPUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__DOIT, "_msgConvertorImpl::_doit" )
   INT32 _msgConvertorImpl::_doit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__DOIT ) ;

      if ( _isReply )
      {
         rc = _convertReply() ;
         PD_RC_CHECK( rc, PDERROR, "Convert reply message failed[%d]", rc ) ;
      }
      else
      {
         rc = _convertRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Convert request message failed[%d]", rc ) ;
      }

      _isDone = TRUE ;
   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__PUSHHEADER, "_msgConvertorImpl::_pushHeader" )
   INT32 _msgConvertorImpl::_pushHeader( const CHAR *header, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__PUSHHEADER ) ;

      SDB_PROTOCOL_VERSION msgVersion =
         ( MSG_COMM_EYE_DEFAULT == ((MsgHeader *)header)->eye ) ?
         SDB_PROTOCOL_VER_2 : SDB_PROTOCOL_VER_1 ;

      if ( SDB_PROTOCOL_VER_2 == msgVersion )
      {
         _action = CONVERT_ACTION_DOWNGRADE ;
         _isReply = IS_REPLY_TYPE( ((MsgHeader *)header)->opCode ) ;
      }
      else
      {
         SDB_ASSERT( SDB_PROTOCOL_VER_1 == msgVersion,
                     "Message version is not as expected" ) ;
         _action = CONVERT_ACTION_UPGRADE ;
         _isReply = IS_REPLY_TYPE( ((MsgHeaderV1 *)header)->opCode ) ;
      }

      try
      {
         _msgPieces.push_back( netIOV( header, size ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__PUSHHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__PUSHBODY, "_msgConvertorImpl::_pushBody" )
   INT32 _msgConvertorImpl::_pushBody( const CHAR *data, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__PUSHBODY ) ;

      try
      {
         _msgPieces.push_back( netIOV( data, size ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__PUSHBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__CONVERTREQUEST, "_msgConvertorImpl::_convertRequest" )
   INT32 _msgConvertorImpl::_convertRequest()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__CONVERTREQUEST ) ;

      if ( CONVERT_ACTION_UPGRADE == _action )
      {
         rc = _upgradeRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Upgrade request message failed[%d]", rc ) ;
      }
      else if ( CONVERT_ACTION_DOWNGRADE == _action )
      {
         rc = _downgradeRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Downgrade request message failed[%d]",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__CONVERTREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__CONVERTREPLY, "_msgConvertorImpl::_convertReply" )
   INT32 _msgConvertorImpl::_convertReply()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__CONVERTREPLY ) ;

      if ( CONVERT_ACTION_UPGRADE == _action )
      {
         rc = _upgradeReply() ;
         PD_RC_CHECK( rc, PDERROR, "Upgrade reply message failed[%d]", rc ) ;
      }
      else if ( CONVERT_ACTION_DOWNGRADE == _action )
      {
         rc = _downgradeReply() ;
         PD_RC_CHECK( rc, PDERROR, "Downgrade reply message failed[%d]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__CONVERTREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__UPGRADEREQUEST, "_msgConvertorImpl::_upgradeRequest" )
   INT32 _msgConvertorImpl::_upgradeRequest()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__UPGRADEREQUEST ) ;

      // The first piece should always be the header.
      netIOV &piece = _msgPieces.front() ;
      INT32 opCode = ((const MsgHeaderV1 *)piece.iovBase)->opCode ;
      if ( MSG_PACKET == opCode )
      {
         rc = _upgradePacketRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Upgrade packet message failed[%d]", rc ) ;
      }
      else
      {
         rc = _upgradeSimpleRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Upgrade request message[opCode: %d] "
                      "failed[%d]", opCode, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__UPGRADEREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__DOWNGRADEREQUEST, "_msgConvertorImpl::_downgradeRequest" )
   INT32 _msgConvertorImpl::_downgradeRequest()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__DOWNGRADEREQUEST ) ;
      netIOV &piece = _msgPieces.front() ;
      INT32 opCode = ((const MsgHeader*)piece.iovBase)->opCode ;
      if ( MSG_PACKET == opCode )
      {
         rc = _downgradePacketRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Downgrade packet message failed[%d]", rc ) ;
      }
      else
      {
         rc = _downgradeSimpleRequest() ;
         PD_RC_CHECK( rc, PDERROR, "Downgrade request message[opCode: %d] "
                      "failed[%d]", opCode, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__DOWNGRADEREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__UPGRADESIMPLEREQUEST, "_msgConvertorImpl::_upgradeSimpleRequest" )
   INT32 _msgConvertorImpl::_upgradeSimpleRequest()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__UPGRADESIMPLEREQUEST ) ;

      MsgHeader header ;
      UINT32 writeLen = 0 ;
      UINT32 readOffset = 0 ;
      netIOV &piece = _msgPieces.front() ;
      const MsgHeaderV1 *oldHeader = (const MsgHeaderV1 *)piece.iovBase ;

      SDB_ASSERT( _msgPieces.size() == 1,
                  "Simple request should has only 1 piece of message" ) ;

      // Step 1: Upgrade the message header of old format into new.
      _msgHeaderUpgrade( oldHeader, header ) ;
      readOffset = sizeof(MsgHeaderV1) ;

      // Step 2: Make sure the buffer for the new converted message is enough.
      rc = _ensureMsgBuff( header.messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to upgrade request "
                   "message failed[%d]", header.messageLength, rc  ) ;

      // Step 3: Copy the new header into the conversion buffer.
      ossMemcpy( _convertBuff, &header, sizeof(MsgHeader) ) ;
      writeLen = sizeof(MsgHeader) ;

      // Step 4: Copy the remainning part of the original message into the
      //         conversion buffer.
      if ( piece.iovLen > readOffset )
      {
         ossMemcpy( _convertBuff + writeLen,
                    (CHAR *)oldHeader + readOffset,
                    piece.iovLen - readOffset ) ;
      }
      writeLen += piece.iovLen - readOffset ;

      piece.iovBase = _convertBuff ;
      piece.iovLen = writeLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__UPGRADESIMPLEREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__DOWNGRADESIMPLEREQUEST, "_msgConvertorImpl::_downgradeSimpleRequest" )
   INT32 _msgConvertorImpl::_downgradeSimpleRequest()
   {
      /**
       * In the scenario of downgrade simple request, it means we want to send
       * the message to a remote node of old version. The original message may
       * contain server pieces. We only need to convert the first piece, in
       * which the message header is contained. Others pieces(if any) do not
       * need to be converted. Just send them in the original format.
       */
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__DOWNGRADESIMPLEREQUEST ) ;

      MsgHeaderV1 header ;
      UINT32 writeLen = 0 ;
      UINT16 readOffset = 0 ;
      netIOV &piece = _msgPieces.front() ;
      const MsgHeader *oldHeader = (const MsgHeader *)piece.iovBase ;

      _msgHeaderDowngrade( oldHeader, header ) ;
      readOffset = sizeof(MsgHeader) ;

      rc = _ensureMsgBuff( header.messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to downgrade "
                   "request message failed[%d]", header.messageLength, rc ) ;

      ossMemcpy( _convertBuff, &header, sizeof(MsgHeaderV1) ) ;
      writeLen += sizeof(MsgHeaderV1) ;

      if ( piece.iovLen > readOffset )
      {
         ossMemcpy( _convertBuff + writeLen,
                    (CHAR *)oldHeader + readOffset,
                    piece.iovLen - readOffset ) ;
      }
      writeLen += piece.iovLen - readOffset ;
      // Only change the first piece. Other pieces are not changed.
      piece.iovBase = _convertBuff ;
      piece.iovLen = writeLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__DOWNGRADESIMPLEREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__UPGRADEPACKETREQUEST, "_msgConvertorImpl::_upgradePacketRequest" )
   INT32 _msgConvertorImpl::_upgradePacketRequest()
   {
      /**
       * Packet message should be unpacked and converted. The header of each
       * sub message should be upgraded.
       */
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__UPGRADEPACKETREQUEST ) ;
      INT32 readOffset = 0 ;
      UINT32 writeLen = 0 ;
      UINT32 subMsgNum = 0 ;
      UINT32 newMsgLen = 0 ;
      const INT16 origHeaderSize = sizeof(MsgHeaderV1) ;
      netIOV &piece = _msgPieces.front() ;
      MsgHeaderV1 *message = (MsgHeaderV1 *)piece.iovBase ;

      // Step 1: Count sub message number in the packet message.
      readOffset = origHeaderSize ;
      while ( readOffset < message->messageLength )
      {
         ++subMsgNum ;
         readOffset +=
            ((MsgHeaderV1 *)((CHAR *)message + readOffset))->messageLength ;
      }

      // Step 2: Calculate the final new message total length, based on the sub
      //         message number.
      newMsgLen = message->messageLength +
                  ( subMsgNum + 1 ) * ( sizeof(MsgHeader) - origHeaderSize ) ;

      rc = _ensureMsgBuff( newMsgLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to upgrade packet "
                   "request failed[%d]", newMsgLen, rc ) ;

      // Step 3: Upgrade the header of the outermost layer and copy into the
      //         conversion buffer.
      _msgHeaderUpgrade( (const MsgHeaderV1 *)message,
                         *(MsgHeader *)_convertBuff ) ;
      readOffset = origHeaderSize ;
      ((MsgHeader *)_convertBuff)->messageLength = newMsgLen ;
      writeLen = sizeof(MsgHeader) ;

      // Step 4: Upgrade each sub message in the packet.
      for ( UINT16 i = 0; i < subMsgNum; ++ i )
      {
         INT32 subMsgLen = *(INT32 *)( (CHAR *)message + readOffset ) ;
         _msgHeaderUpgrade( (const MsgHeaderV1 *)((CHAR *)message + readOffset),
                            *(MsgHeader *)(_convertBuff + writeLen) ) ;
         readOffset += origHeaderSize ;
         writeLen += sizeof(MsgHeader) ;
         if ( subMsgLen > origHeaderSize )
         {
            ossMemcpy( _convertBuff + writeLen, (CHAR *)message + readOffset,
                       subMsgLen - origHeaderSize ) ;
            readOffset += subMsgLen - origHeaderSize ;
            writeLen += subMsgLen - origHeaderSize ;
         }
      }

      piece.iovBase = _convertBuff ;
      piece.iovLen = newMsgLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__UPGRADEPACKETREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__DOWNGRADEPACKETREQUEST, "_msgConvertorImpl::_downgradePacketRequest" )
   INT32 _msgConvertorImpl::_downgradePacketRequest()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__DOWNGRADEPACKETREQUEST ) ;
      INT32 readOffset = 0 ;
      UINT32 writeLen = 0 ;
      UINT32 subMsgNum = 0 ;
      UINT32 newMsgLen = 0 ;
      const INT16 origHeaderSize = sizeof(MsgHeader) ;
      netIOV &piece = _msgPieces.front() ;
      MsgHeader *message = (MsgHeader *)piece.iovBase ;

      SDB_ASSERT( 1 == _msgPieces.size(),
                  "Packet request should have only 1 piece of message" ) ;

      readOffset = origHeaderSize ;
      while ( readOffset < message->messageLength )
      {
         ++subMsgNum ;
         readOffset +=
            ((MsgHeader *)((CHAR*)message + readOffset))->messageLength ;
      }

      newMsgLen = message->messageLength -
                  ( subMsgNum + 1 ) * ( origHeaderSize - sizeof(MsgHeaderV1) ) ;

      rc = _ensureMsgBuff( newMsgLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to downgrade packet "
                   "request failed[%d]", newMsgLen, rc ) ;

      _msgHeaderDowngrade( (const MsgHeader *)message,
                           *(MsgHeaderV1 *)_convertBuff ) ;
      readOffset = origHeaderSize ;
      ((MsgHeader *)_convertBuff)->messageLength = newMsgLen ;
      writeLen = sizeof(MsgHeaderV1) ;

      for ( UINT16 i = 0; i < subMsgNum; ++i )
      {
         INT32 subMsgLen = *(INT32 *)( (CHAR *)message + readOffset ) ;
         _msgHeaderDowngrade( (const MsgHeader *)((CHAR *)message + readOffset ),
                              *(MsgHeaderV1 *)(_convertBuff + writeLen) ) ;
         readOffset += origHeaderSize ;
         writeLen += sizeof(MsgHeaderV1) ;
         if ( subMsgLen > origHeaderSize )
         {
            ossMemcpy( _convertBuff + writeLen, (CHAR *)message + readOffset,
                       subMsgLen - origHeaderSize ) ;
            readOffset += subMsgLen - origHeaderSize ;
            writeLen += subMsgLen - origHeaderSize ;
         }
      }

      piece.iovBase = _convertBuff ;
      piece.iovLen = newMsgLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__DOWNGRADEPACKETREQUEST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__UPGRADEREPLY, "_msgConvertorImpl::_upgradeReply" )
   INT32 _msgConvertorImpl::_upgradeReply()
   {
      /**
       * For reply message, there are some special types, which are not using
       * the common reply header(MsgOpReply). They are called 'inner reply'.
       * The use the structure 'MsgInternalReplyHeader' as their header. This
       * affect the calculation of the header size change.
       */
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__UPGRADEREPLY ) ;
      MsgOpReply reply ;
      UINT32 readOffset = 0 ;
      UINT32 partLen = 0 ;
      UINT32 writeLen = 0 ;
      SDB_ASSERT( _msgPieces.size() > 0, "No message piece info" ) ;

      netIOV &piece = _msgPieces.front() ;

      if ( _isInnerOpReply( ((MsgHeaderV1 *)piece.iovBase)->opCode ) )
      {
         // For inner op reply, only upgrade the header
         _msgHeaderUpgrade( (const MsgHeaderV1 *)piece.iovBase, reply.header ) ;
         readOffset = sizeof( MsgHeaderV1 ) ;
         partLen = sizeof( MsgHeader ) ;
      }
      else
      {
         _msgReplyHeaderUpgrade( (const MsgOpReplyV1 *)piece.iovBase, reply ) ;
         readOffset = sizeof( MsgOpReplyV1 ) ;
         partLen = sizeof( MsgOpReply ) ;
      }

      rc = _ensureMsgBuff( reply.header.messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to upgrade reply "
                   "message failed[%d]", reply.header.messageLength, rc ) ;

      ossMemcpy( _convertBuff, &reply, partLen ) ;
      writeLen = partLen ;
      if ( piece.iovLen > readOffset )
      {
         ossMemcpy( _convertBuff + writeLen,
                    (CHAR *)piece.iovBase + readOffset,
                    piece.iovLen - readOffset ) ;
      }
      writeLen += piece.iovLen - readOffset ;
      piece.iovBase = _convertBuff ;
      piece.iovLen = writeLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__UPGRADEREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__DOWNGRADEREPLY, "_msgConvertorImpl::_downgradeReply" )
   INT32 _msgConvertorImpl::_downgradeReply()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__DOWNGRADEREPLY ) ;
      MsgOpReplyV1 reply ;
      UINT32 readOffset = 0 ;
      UINT32 partLen = 0 ;
      UINT32 writeLen = 0 ;
      SDB_ASSERT( _msgPieces.size() > 0, "No message piece info" ) ;

      netIOV &piece = _msgPieces.front() ;

      if ( _isInnerOpReply( ((MsgHeader *)piece.iovBase)->opCode ) )
      {
         _msgHeaderDowngrade( (const MsgHeader *)piece.iovBase, reply.header ) ;
         readOffset = sizeof( MsgHeader ) ;
         partLen = sizeof( MsgHeaderV1 ) ;
      }
      else
      {
         _msgReplyHeaderDowngrade( (const MsgOpReply *)piece.iovBase, reply ) ;
         readOffset = sizeof( MsgOpReply ) ;
         partLen = sizeof( MsgOpReplyV1 ) ;
      }

      rc = _ensureMsgBuff( reply.header.messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] to downgrade reply "
                   "message failed[%d]", reply.header.messageLength, rc ) ;

      ossMemcpy( _convertBuff, &reply, partLen ) ;
      writeLen = partLen ;
      // Copy the remainning part after the MsgOpReply of the reply message.
      if ( piece.iovLen > readOffset )
      {
         ossMemcpy( _convertBuff + writeLen,
                    (CHAR *)piece.iovBase + readOffset,
                    piece.iovLen - readOffset ) ;
      }
      writeLen += piece.iovLen - readOffset ;
      piece.iovBase = _convertBuff ;
      piece.iovLen = writeLen ;

#if defined (_DEBUG)
      SDB_ASSERT( TRUE == _validateMsgPieces(),
                  "Message piece info is not correct" ) ;
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__DOWNGRADEREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGCONVERTORIMPL__ENSUREBUFF, "_msgConvertorImpl::_ensureMsgBuff" )
   INT32 _msgConvertorImpl::_ensureMsgBuff( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MSGCONVERTORIMPL__ENSUREBUFF ) ;

      BOOLEAN notEnough = ( _buffSize < size ) ;

      // To avoid that the buffer becomes too big because of one big message,
      // try to shrink the buffer. When the actual size of the buffer is greater
      // than the threshold and the requested size is less than half of the
      // threshold, the shrinking will be done.
      if ( notEnough ||
           ( _buffSize > MSG_CVT_SHRINK_THRESHOLD &&
             ( size < MSG_CVT_SHRINK_THRESHOLD / 2 ) ) )
      {
         CHAR *newBuff = (CHAR *)SDB_OSS_MALLOC( size ) ;
         if ( !newBuff )
         {
            if ( notEnough )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate memory[size: %u] in message convertor"
                       " failed[%d]", size, rc ) ;
               goto error ;
            }
            else
            {
               // Current buffer is enough, no need to return error.
               goto done ;
            }
         }
         ossMemset( newBuff, 0, size ) ;

         SAFE_OSS_FREE( _convertBuff ) ;
         _convertBuff = newBuff ;
         _buffSize = size ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MSGCONVERTORIMPL__ENSUREBUFF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _msgConvertorImpl::_msgHeaderUpgrade( const MsgHeaderV1 *msgHeader,
                                              MsgHeader &newMsgHeader )
   {
      SDB_ASSERT( msgHeader, "Message to be upgrade is null" ) ;
      newMsgHeader.messageLength = msgHeader->messageLength +
                                   ( sizeof(MsgHeader) - sizeof(MsgHeaderV1) ) ;
      newMsgHeader.eye = MSG_COMM_EYE_DEFAULT ;
      newMsgHeader.version = SDB_PROTOCOL_VER_2 ;
      newMsgHeader.flags = 0 ;
      newMsgHeader.opCode = msgHeader->opCode ;
      newMsgHeader.TID = msgHeader->TID ;
      newMsgHeader.routeID = msgHeader->routeID ;
      newMsgHeader.requestID = msgHeader->requestID ;
      ossMemset( &(newMsgHeader.globalID), 0, sizeof( newMsgHeader.globalID) ) ;
      ossMemset( newMsgHeader.reserve, 0, sizeof( newMsgHeader.reserve) ) ;
   }

   void _msgConvertorImpl::_msgHeaderDowngrade( const MsgHeader *msgHeader,
                                                MsgHeaderV1 &newMsgHeader )
   {
      newMsgHeader.messageLength = msgHeader->messageLength -
                                   ( sizeof(MsgHeader) - sizeof(MsgHeaderV1) ) ;
      newMsgHeader.opCode = msgHeader->opCode ;
      newMsgHeader.TID = msgHeader->TID ;
      newMsgHeader.routeID = msgHeader->routeID ;
      newMsgHeader.requestID = msgHeader->requestID ;
   }

   void _msgConvertorImpl::_msgReplyHeaderUpgrade( const MsgOpReplyV1 *replyHeader,
                                                   MsgOpReply &newReplyHeader )
   {
      _msgHeaderUpgrade( &replyHeader->header, newReplyHeader.header ) ;
      newReplyHeader.header.messageLength = replyHeader->header.messageLength +
         sizeof(MsgOpReply) - sizeof(MsgOpReplyV1);
      newReplyHeader.contextID = replyHeader->contextID ;
      newReplyHeader.flags = replyHeader->flags ;
      newReplyHeader.startFrom = replyHeader->startFrom ;
      newReplyHeader.numReturned = replyHeader->numReturned ;
      newReplyHeader.returnMask = 0 ;
   }

   void _msgConvertorImpl::_msgReplyHeaderDowngrade( const MsgOpReply *replyHeader,
                                                     MsgOpReplyV1 &newReplyHeader )
   {
      _msgHeaderDowngrade( &replyHeader->header, newReplyHeader.header ) ;
      newReplyHeader.header.messageLength = replyHeader->header.messageLength -
         ( sizeof(MsgOpReply) - sizeof(MsgOpReplyV1) ) ;
      newReplyHeader.contextID = replyHeader->contextID ;
      newReplyHeader.flags = replyHeader->flags ;
      newReplyHeader.startFrom = replyHeader->startFrom ;
      newReplyHeader.numReturned = replyHeader->numReturned ;
   }

   BOOLEAN _msgConvertorImpl::_isInnerOpReply( INT32 opCode )
   {
      BOOLEAN result = FALSE ;
      switch ( opCode )
      {
         case MSG_CLS_BEAT_RES:
         case MSG_CLS_BALLOT_RES:
         case MSG_CLS_SYNC_RES:
         case MSG_CLS_CONSULTATION_RES:
         case MSG_CLS_FULL_SYNC_BEGIN_RES:
         case MSG_CLS_FULL_SYNC_LEND_RES:
         case MSG_CLS_FULL_SYNC_END_RES:
         case MSG_CLS_FULL_SYNC_META_RES:
         case MSG_CLS_FULL_SYNC_INDEX_RES:
         case MSG_CLS_FULL_SYNC_NOTIFY_RES:
         case MSG_CLS_FULL_SYNC_TRANS_RES:
            result = TRUE ;
            break ;
         default:
            result = FALSE ;
      }

      return result ;
   }

#if defined (_DEBUG)
   BOOLEAN _msgConvertorImpl::_validateMsgPieces()
   {
      // Validate the total message length.
      BOOLEAN result = FALSE ;
      INT32 totalLen = 0 ;
      MSG_PIECE_LIST_CITR itr = _msgPieces.begin() ;
      if ( _msgPieces.end() == itr )
      {
         goto done ;
      }

      totalLen = *(INT32 *)itr->iovBase ;
      do
      {
         totalLen -= (INT32)itr->iovLen ;
         ++itr ;
      }
      while ( itr != _msgPieces.end() ) ;

      result = ( 0 == totalLen ) ;
   done:
      return result ;
   }
#endif /* _DEBUG */

}
