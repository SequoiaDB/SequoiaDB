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

   Source File Name = pmdSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdSession.hpp"
#include "pmdEDU.hpp"
#include "pmdEnv.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "pmdTrace.hpp"

using namespace bson ;

namespace engine
{
   #define PMD_RECV_DATA_AFTER_LENGTH_TIMEOUT         ( 120 * OSS_ONE_SEC )

   /*
      _pmdLocalSession implement
   */
   _pmdLocalSession::_pmdLocalSession( SOCKET fd )
   :pmdSession( fd )
   {
      ossMemset( (void*)&_replyHeader, 0, sizeof(_replyHeader) ) ;
      _needReply = TRUE ;
      _needRollback = FALSE ;
   }

   _pmdLocalSession::~_pmdLocalSession()
   {
   }

   INT32 _pmdLocalSession::getServiceType () const
   {
      return CMD_SPACE_SERVICE_LOCAL ;
   }

   SDB_SESSION_TYPE _pmdLocalSession::sessionType() const
   {
      return SDB_SESSION_LOCAL ;
   }

   void _pmdLocalSession::_onAttach ()
   {
   }

   void _pmdLocalSession::_onDetach ()
   {
   }

   INT32 _pmdLocalSession::run()
   {
      INT32 rc                = SDB_OK ;
      UINT32 msgSize          = 0 ;
      CHAR *pBuff             = NULL ;
      INT32 buffSize          = 0 ;
      pmdEDUMgr *pmdEDUMgr    = NULL ;
      monDBCB *mondbcb        = pmdGetKRCB()->getMonDBCB () ;

      if ( !_pEDUCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      pmdEDUMgr = _pEDUCB->getEDUMgr() ;

      while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
      {
         _pEDUCB->resetInterrupt() ;
         _pEDUCB->resetInfo( EDU_INFO_ERROR ) ;
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

         if ( msgSize == (UINT32)MSG_SYSTEM_INFO_LEN )
         {
            rc = _recvSysInfoMsg( msgSize, &pBuff, buffSize ) ;
            if ( rc )
            {
               break ;
            }
            rc = _processSysInfoRequest( pBuff ) ;
            if ( rc )
            {
               break ;
            }

            _setHandshakeReceived() ;
         }
         else if ( msgSize < sizeof(MsgHeader) || msgSize > SDB_MAX_MSG_LENGTH )
         {
            PD_LOG( PDERROR, "Session[%s] recv msg size[%d] is less than "
                    "MsgHeader size[%d] or more than max msg size[%d]",
                    sessionName(), msgSize, sizeof(MsgHeader),
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
            buffSize = getBuffLen() ;
            *(UINT32*)pBuff = msgSize ;
            INT32 hasReceived = 0 ;
            rc = recvData( pBuff + sizeof(UINT32),
                           msgSize - sizeof(UINT32),
                           PMD_RECV_DATA_AFTER_LENGTH_TIMEOUT,
                           TRUE, &hasReceived ) ;
            if ( rc )
            {
               if ( SDB_APP_FORCED != rc )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to recv msg[len: %u, "
                          "recieved: %d], rc: %d",
                          sessionName(), msgSize - sizeof(UINT32),
                          hasReceived, rc ) ;
               }
               break ;
            }

            _pEDUCB->incEventCount() ;
            mondbcb->addReceiveNum() ;
            pBuff[ msgSize ] = 0 ;
            if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                       sessionName(), rc ) ;
               break ;
            }
            rc = _processMsg( (MsgHeader*)pBuff ) ;
            if ( rc )
            {
               break ;
            }
            if ( SDB_OK != ( rc = pmdEDUMgr->waitEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
                       sessionName(), rc ) ;
               break ;
            }
         }
      } // end while

   done:
      disconnect() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdLocalSession::_recvSysInfoMsg( UINT32 msgSize,
                                            CHAR **ppBuff,
                                            INT32 &buffLen )
   {
      INT32 rc = SDB_OK ;
      UINT32 recvSize = sizeof(MsgSysInfoRequest) ;

      *ppBuff = getBuff( recvSize ) ;
      if ( !*ppBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      buffLen = getBuffLen() ;
      *(INT32*)(*ppBuff) = msgSize ;

      rc = recvData( *ppBuff + sizeof(UINT32), recvSize - sizeof( UINT32 ),
                     PMD_RECV_DATA_AFTER_LENGTH_TIMEOUT ) ;
      if ( rc )
      {
         if ( SDB_APP_FORCED != rc )
         {
            PD_LOG( PDERROR, "Session[%s] failed to recv sys info req rest "
                    "msg, rc: %d", sessionName(), rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdLocalSession::_processSysInfoRequest( const CHAR * msg )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN endianConvert = FALSE ;
      MsgSysInfoReply reply ;
      reply.header.specialSysInfoLen      = MSG_SYSTEM_INFO_LEN ;
      reply.header.eyeCatcher             = MSG_SYSTEM_INFO_EYECATCHER ;
      reply.header.realMessageLength      = sizeof(MsgSysInfoReply) ;
      reply.osType                        = OSS_OSTYPE ;
      ossMemset( reply.pad, 0, sizeof(reply.pad ) ) ;

      rc = msgExtractSysInfoRequest ( (CHAR*)msg, endianConvert ) ;
      PD_RC_CHECK ( rc, PDERROR, "Session[%s] failed to extract sys info "
                    "request, rc = %d", sessionName(), rc ) ;

      rc = sendData ( (const CHAR*)&reply, sizeof( MsgSysInfoReply ) ) ;
      PD_RC_CHECK ( rc, PDERROR, "Session[%s] failed to send packet, rc = %d",
                    sessionName(), rc ) ;

   done :
      return rc ;
   error :
      disconnect() ;
      goto done ;
   }

   INT32 _pmdLocalSession::_onMsgBegin( MsgHeader *msg )
   {
      _replyHeader.contextID          = -1 ;
      _replyHeader.numReturned        = 0 ;
      _replyHeader.startFrom          = 0 ;
      _replyHeader.header.opCode      = MAKE_REPLY_TYPE(msg->opCode) ;
      _replyHeader.header.requestID   = msg->requestID ;
      _replyHeader.header.TID         = msg->TID ;
      _replyHeader.header.routeID     = pmdGetNodeID() ;

      if ( MSG_BS_INTERRUPTE == msg->opCode ||
           MSG_BS_INTERRUPTE_SELF == msg->opCode ||
           MSG_BS_DISCONNECT == msg->opCode )
      {
         _needReply = FALSE ;
      }
      else
      {
         _needReply = TRUE ;
      }

      if ( MSG_BS_UPDATE_REQ == msg->opCode ||
           MSG_BS_INSERT_REQ == msg->opCode ||
           MSG_BS_DELETE_REQ == msg->opCode ||
           MSG_BS_TRANS_COMMIT_REQ == msg->opCode )
      {
         _needRollback = TRUE ;
      }
      else
      {
         _needRollback = FALSE ;
      }

      MON_START_OP( _pEDUCB->getMonAppCB() ) ;
      _pEDUCB->getMonAppCB()->setLastOpType( msg->opCode ) ;

      return SDB_OK ;
   }

   void _pmdLocalSession::_onMsgEnd( INT32 result, MsgHeader *msg )
   {
      if ( result && SDB_DMS_EOC != result )
      {
         PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
                 "TID: %d, requestID: %llu] failed, rc: %d",
                 sessionName(), msg->opCode, msg->messageLength, msg->TID,
                 msg->requestID, result ) ;
      }

      if ( result != SDB_OK )
      {
         pmdIncErrNum( result ) ;
      }

      MON_END_OP( _pEDUCB->getMonAppCB() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOCALSN_PROMSG, "_pmdLocalSession::_processMsg" )
   INT32 _pmdLocalSession::_processMsg( MsgHeader * msg )
   {
      INT32 rc          = SDB_OK ;
      const CHAR *pBody = NULL ;
      INT32 bodyLen     = 0 ;
      rtnContextBuf contextBuff ;
      INT32 opCode      = msg->opCode ;

      PD_TRACE_ENTRY( SDB_PMDLOCALSN_PROMSG );
      rc = _onMsgBegin( msg ) ;
      if ( SDB_OK == rc )
      {
         rc = _processor->processMsg( msg, contextBuff,
                                      _replyHeader.contextID,
                                      _needReply ) ;
         pBody     = contextBuff.data() ;
         bodyLen   = contextBuff.size() ;
         _replyHeader.numReturned = contextBuff.recordNum() ;
         _replyHeader.startFrom = (INT32)contextBuff.getStartFrom() ;
         if ( SDB_OK != rc )
         {
            if ( _needRollback )
            {
               INT32 rcTmp = rtnTransRollback( eduCB(), getDPSCB() ) ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to rollback trans "
                          "info, rc: %d", sessionName(), rcTmp ) ;
               }
               _needRollback = FALSE ;
            }
         }
      }

      if ( _needReply )
      {
         if ( rc && bodyLen == 0 )
         {
            _errorInfo = utilGetErrorBson( rc, _pEDUCB->getInfo(
                                           EDU_INFO_ERROR ) ) ;
            pBody = _errorInfo.objdata() ;
            bodyLen = (INT32)_errorInfo.objsize() ;
            _replyHeader.numReturned = 1 ;
         }
         _replyHeader.header.opCode = MAKE_REPLY_TYPE(opCode) ;
         _replyHeader.flags         = rc ;
         _replyHeader.header.messageLength = sizeof( _replyHeader ) +
                                             bodyLen ;

         INT32 rcTmp = _reply( &_replyHeader, pBody, bodyLen ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s] failed to send response, rc: %d",
                    sessionName(), rcTmp ) ;
            disconnect() ;
         }
      }

      _onMsgEnd( rc, msg ) ;
      rc = SDB_OK ;
      PD_TRACE_EXITRC ( SDB_PMDLOCALSN_PROMSG, rc );
      return rc ;
   }

   INT32 _pmdLocalSession::_reply( MsgOpReply *responseMsg,
                                   const CHAR *pBody,
                                   INT32 bodyLen )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( responseMsg->header.messageLength ==
                  (SINT32)(sizeof(MsgOpReply) + bodyLen),
                  "Invalid msg" ) ;

      rc = sendData( (const CHAR*)responseMsg, sizeof(MsgOpReply) ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to send response header, rc: %d",
                 sessionName(), rc ) ;
         goto error ;
      }
      if ( pBody )
      {
         rc = sendData( pBody, bodyLen ) ;
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


}


