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
         // clear interrupt flag
         _pEDUCB->resetInterrupt() ;
         _pEDUCB->resetInfo( EDU_INFO_ERROR ) ;
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
         /// update conf should here
         _pEDUCB->updateTransConf() ;

         // if system info msg
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
#ifdef SDB_ENTERPRISE

#ifdef SDB_SSL
         else if ( _isAwaitingHandshake() )
         {
            if ( pmdGetOptionCB()->useSSL() )
            {
               rc = _socket.doSSLHandshake ( (CHAR*)&msgSize, sizeof(UINT32) ) ;
               if ( rc )
               {
                  break ;
               }

               _setHandshakeReceived() ;
            }
            else
            {
               PD_LOG( PDERROR, "SSL handshake received but server is started "
                       "without SSL support" ) ;
               rc = SDB_NETWORK ;
               break ;
            }

            /*continue;

            PD_LOG( PDERROR, "SSL feature not available in this build" ) ;
            rc = SDB_NETWORK ;
            break ;*/
         }
#endif /* SDB_SSL */

#endif /* SDB_ENTERPRISE */
         // error msg
         else if ( msgSize < sizeof(MsgHeader) || msgSize > SDB_MAX_MSG_LENGTH )
         {
            PD_LOG( PDERROR, "Session[%s] recv msg size[%d] is less than "
                    "MsgHeader size[%d] or more than max msg size[%d]",
                    sessionName(), msgSize, sizeof(MsgHeader),
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
            buffSize = getBuffLen() ;
            *(UINT32*)pBuff = msgSize ;
            INT32 hasReceived = 0 ;
            // recv the rest msg, need timeout
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

            // increase process event count
            _pEDUCB->incEventCount() ;
            mondbcb->addReceiveNum() ;
            pBuff[ msgSize ] = 0 ;
            // activate edu
            if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
            {
               PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                       sessionName(), rc ) ;
               break ;
            }
            // process msg
            rc = _processMsg( (MsgHeader*)pBuff ) ;
            if ( rc )
            {
               break ;
            }
            // wait edu
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

      // recv recvSize1
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
      reply.authVersion                   = AUTH_SCRAM_SHA256 ;
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
      // set reply header ( except flags, length )
      _replyHeader.contextID          = -1 ;
      _replyHeader.numReturned        = 0 ;
      _replyHeader.startFrom          = 0 ;
      _replyHeader.header.opCode      = MAKE_REPLY_TYPE(msg->opCode) ;
      _replyHeader.header.requestID   = msg->requestID ;
      _replyHeader.header.TID         = msg->TID ;
      _replyHeader.header.routeID     = pmdGetNodeID() ;

      if ( isNoReplyMsg( msg->opCode ) )
      {
         _needReply = FALSE ;
      }
      else
      {
         _needReply = TRUE ;
      }

      // start operator
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

      // end operator
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
      BOOLEAN hasException = FALSE ;

      BOOLEAN needRollback = FALSE ;
      BOOLEAN isAutoCommit = FALSE ;
      BOOLEAN isDoCommit   = FALSE ;

      BSONObjBuilder retBuilder( PMD_RETBUILDER_DFT_SIZE ) ;

      PD_TRACE_ENTRY( SDB_PMDLOCALSN_PROMSG ) ;

      UINT64 bTime = ossGetCurrentMicroseconds() ;

      // prepare
      rc = _onMsgBegin( msg ) ;
      if ( SDB_OK == rc )
      {
         if ( MSG_BS_TRANS_COMMIT_REQ == msg->opCode )
         {
            isDoCommit = TRUE ;
         }

         try
         {
            rc = _processor->processMsg( msg, contextBuff,
                                         _replyHeader.contextID,
                                         _needReply,
                                         needRollback,
                                         retBuilder ) ;
            pBody     = contextBuff.data() ;
            bodyLen   = contextBuff.size() ;
            _replyHeader.numReturned = contextBuff.recordNum() ;
            _replyHeader.startFrom = (INT32)contextBuff.getStartFrom() ;

            if ( eduCB()->isAutoCommitTrans() &&
                 -1 == eduCB()->getCurAutoTransCtxID() )
            {
               isAutoCommit = TRUE ;
               if ( SDB_OK == rc || SDB_DMS_EOC == rc )
               {
                  INT32 rcTmp = _processor->doCommit() ;
                  rc = rcTmp ? rcTmp : rc ;
               }
            }

            if ( SDB_OK != rc && SDB_RTN_ALREADY_IN_AUTO_TRANS != rc &&
                 eduCB()->isTransaction() &&
                 ( isAutoCommit || isDoCommit ||
                   ( needRollback &&
                     eduCB()->getTransExecutor()->isTransAutoRollback() )
                 )
               )
            {
               PD_LOG( PDDEBUG, "Session[%s] rolling back operation "
                       "(opCode=%d, rc=%d)", sessionName(), msg->opCode, rc ) ;

               INT32 rcTmp = _processor->doRollback() ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR, "Session[%s] failed to rollback trans "
                          "info, rc: %d", sessionName(), rcTmp ) ;
               }
            }
         }
         catch( std::bad_alloc &e )
         {
            hasException = TRUE ;
            PD_LOG_MSG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
         }
         catch( std::exception &e )
         {
            hasException = TRUE ;
            PD_LOG_MSG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
         }
      }

      if ( _needReply )
      {
         if ( rc )
         {
            if ( SDB_APP_INTERRUPT == rc &&
                 SDB_OK != _pEDUCB->getInterruptRC() )
            {
               rc = _pEDUCB->getInterruptRC() ;
               PD_LOG ( PDDEBUG, "Interrupted EDU [%llu] with return code %d",
                        _pEDUCB->getID(), rc ) ;
            }

            if ( 0 == bodyLen )
            {
               utilBuildErrorBson( retBuilder, rc,
                                   _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
               _errorInfo = retBuilder.obj() ;
               pBody = _errorInfo.objdata() ;
               bodyLen = (INT32)_errorInfo.objsize() ;
               _replyHeader.numReturned = 1 ;
            }
            else
            {
               SDB_ASSERT( 1 == _replyHeader.numReturned,
                           "Record number must be 1" ) ;

               BSONObj errObj( pBody ) ;
               retBuilder.appendElements( errObj ) ;
               _errorInfo = retBuilder.obj() ;
               pBody = _errorInfo.objdata() ;
               bodyLen = (INT32)_errorInfo.objsize() ;
               _replyHeader.numReturned = 1 ;
            }
         }
         /// succeed and has result info
         else if ( !retBuilder.isEmpty() && 0 == bodyLen )
         {
            _errorInfo = retBuilder.obj() ;
            pBody = _errorInfo.objdata() ;
            bodyLen = (INT32)_errorInfo.objsize() ;
            _replyHeader.numReturned = 1 ;
         }

         // fill the return opCode
         _replyHeader.header.opCode = MAKE_REPLY_TYPE(opCode) ;
         _replyHeader.flags         = rc ;
         _replyHeader.header.messageLength = sizeof( _replyHeader ) +
                                             bodyLen ;

         // send response
         INT32 rcTmp = _reply( &_replyHeader, pBody, bodyLen ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s] failed to send response, rc: %d",
                    sessionName(), rcTmp ) ;
            disconnect() ;
         }
         else if ( hasException )
         {
            disconnect() ;
         }
      }

      // end
      _onMsgEnd( rc, msg ) ;

      UINT64 eTime = ossGetCurrentMicroseconds() ;
      if ( eTime > bTime )
      {
         monSvcTaskInfo *pInfo = NULL ;
         pInfo = eduCB()->getMonAppCB()->getSvcTaskInfo() ;
         if ( pInfo )
         {
            /// it doesn't matter wether type is MON_TOTAL_WRITE_TIME
            /// or MON_TOTAL_READ_TIME
            pInfo->monOperationTimeInc( MON_TOTAL_WRITE_TIME,
                                        eTime - bTime ) ;
         }
      }

      rc = SDB_OK ;
      PD_TRACE_EXITRC ( SDB_PMDLOCALSN_PROMSG, rc ) ;
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

      // response header
      rc = sendData( (const CHAR*)responseMsg, sizeof(MsgOpReply) ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to send response header, rc: %d",
                 sessionName(), rc ) ;
         goto error ;
      }
      // response body
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


