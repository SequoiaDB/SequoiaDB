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

   Source File Name = msgConvertorImpl.hpp

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
#ifndef MSG_CONVERTOR_IMPL_HPP__
#define MSG_CONVERTOR_IMPL_HPP__

#include "msgConvertor.hpp"
#include "msg.hpp"
#include "ossMemPool.hpp"
#include "netDef.hpp"

namespace engine
{
   /**
    * Message convertor for protocol conversion, both upgrade and downgrade.
    *
    * Note: No lock in the convertor for now. The user should take care of the
    *       concurrency control.
    */
   class _msgConvertorImpl : public _IMsgConvertor
   {
      typedef ossPoolList< netIOV > MSG_PIECE_LIST ;
      typedef MSG_PIECE_LIST::const_iterator MSG_PIECE_LIST_CITR ;
      enum _CONVERT_ACTION
      {
         CONVERT_ACTION_NONE = 0,
         CONVERT_ACTION_UPGRADE,
         CONVERT_ACTION_DOWNGRADE,
      } ;

   public:
      _msgConvertorImpl() ;
      ~_msgConvertorImpl() ;

      void reset( BOOLEAN releaseBuff ) ;
      INT32 push( const CHAR *data, UINT32 size ) ;
      INT32 output( CHAR *&data, UINT32 &len ) ;

   private:
      INT32 _doit() ;
      INT32 _pushHeader( const CHAR *header, UINT32 size ) ;
      INT32 _pushBody( const CHAR *data, UINT32 size ) ;

      INT32 _convertRequest() ;
      INT32 _convertReply() ;

      INT32 _upgradeRequest() ;
      INT32 _downgradeRequest() ;

      // Simple request is a request that is not in a packet message.
      INT32 _upgradeSimpleRequest() ;
      INT32 _downgradeSimpleRequest() ;

      INT32 _upgradePacketRequest() ;
      INT32 _downgradePacketRequest() ;

      INT32 _upgradeReply() ;
      INT32 _downgradeReply() ;

      INT32 _ensureMsgBuff( UINT32 size ) ;

      /**
       * Upgrade message header from version 1 to current version.
       * @param msgHeader Original message header of version 1.
       * @param newMsgHeader Message of new version converted form old version.
       */
      static void _msgHeaderUpgrade( const MsgHeaderV1 *msgHeader,
                                     MsgHeader &newMsgHeader ) ;

      /**
       * Downgrade message header from current version to version 1.
       * @param msgHeader Message header of current version
       * @param newMsgHeader Message of version 1 converted from new version.
       */
      static void _msgHeaderDowngrade( const MsgHeader *msgHeader,
                                       MsgHeaderV1 &newMsgHeader ) ;

      static void _msgReplyHeaderUpgrade( const MsgOpReplyV1 *replyHeader,
                                          MsgOpReply &newReplyHeader ) ;

      static void _msgReplyHeaderDowngrade( const MsgOpReply *replyHeader,
                                            MsgOpReplyV1 &newReplyHeader ) ;

      static BOOLEAN _isInnerOpReply( INT32 opCode ) ;

#if defined (_DEBUG)
      BOOLEAN _validateMsgPieces() ;
#endif /* _DEBUG */

   private:
      MSG_PIECE_LIST                _msgPieces ;
      _CONVERT_ACTION               _action ;
      BOOLEAN                       _isReply ;
      BOOLEAN                       _isDone ;
      CHAR                          *_convertBuff ;
      UINT32                        _expectLen ;
      UINT32                        _pushLen ;
      UINT32                        _buffSize ;
      UINT32                        _writePos ;
   } ;
   typedef _msgConvertorImpl msgConvertorImpl ;
}

#endif /* MSG_CONVERTOR_IMPL_HPP__ */
