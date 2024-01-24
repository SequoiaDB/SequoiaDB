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

   Source File Name = rtnRemoteExec.cpp

   Descriptive Name = Remote Excuting Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for process op.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/27/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnRemoteExec.hpp"
#include "ossSocket.hpp"
#include "ossProc.hpp"
#include "msgMessage.hpp"
#include "omagentDef.hpp"
#include "utilParam.hpp"
#include "pmdEDU.hpp"
#include "msgDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "msgConvertorImpl.hpp"
#include "coordRemoteSession.hpp"

using namespace bson ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_REMOTEEXEC, "rtnRemoteExec" )
   INT32 rtnRemoteExec ( SINT32 remoCode,
                         const CHAR * hostname,
                         SINT32 *retCode,
                         const BSONObj *arg1,
                         const BSONObj *arg2,
                         const BSONObj *arg3,
                         const BSONObj *arg4,
                         std::vector<BSONObj> *retObjs )
   {
      SDB_ASSERT ( retCode && hostname , "Invalid input" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_REMOTEEXEC ) ;
      PD_TRACE1 ( SDB_REMOTEEXEC,
                  PD_PACK_INT ( remoCode ) ) ;
      CHAR *pCMRequest = NULL ;
      CHAR *pReceiveBuffer = NULL ;
      CHAR *replyPtr = NULL ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      const CHAR *svcname = NULL ;
      UINT16 port = SDBCM_DFT_PORT ;
      INT32 reqSize = 0 ;
      SINT32 packetLength = 0 ;
      CHAR *finalRequest = NULL ;
      msgConvertorImpl *msgConvertor = NULL ;

      pmdEDUCB *eduCB = pmdGetThreadEDUCB() ;
      INT32 execTimeout = -1 ;

      po::options_description desc ( "Config options" ) ;
      po::variables_map vm ;
      CHAR hostname2[OSS_MAX_HOSTNAME + 6] = { 0 } ;
      if ( ossStrlen(hostname) > OSS_MAX_HOSTNAME )
      {
         PD_LOG ( PDERROR, "Invalid host name: %s", hostname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossMemcpy( hostname2, hostname, ossStrlen( hostname ) );
      ossStrncat ( hostname2, SDBCM_CONF_PORT, ossStrlen(SDBCM_CONF_PORT) ) ;

      desc.add_options()
         (SDBCM_CONF_DFTPORT, po::value<string>(), "sdbcm default "
         "listening port")
         (hostname2, po::value<string>(), "sdbcm specified listening port")
      ;

      // try to use operator timeout of session
      if ( NULL != eduCB &&
           NULL != eduCB->getRemoteSite() )
      {
         coordSessionPropSite *propSite =
               (coordSessionPropSite *)eduCB->getRemoteSite()->getUserData() ;
         if ( NULL != propSite )
         {
            execTimeout = propSite->getOperationTimeout() ;
         }
      }

      rc = ossGetEWD ( conf, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get excutable file's working "
                  "directory" ) ;
         goto error ;
      }
      if ( ( ossStrlen ( conf ) + ossStrlen ( SDBCM_CONF_PATH_FILE ) + 2 ) >
           OSS_MAX_PATHSIZE )
      {
         PD_LOG ( PDERROR, "Working directory too long" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncat( conf, OSS_FILE_SEP, 1 );
      ossStrncat( conf, SDBCM_CONF_PATH_FILE,
                  ossStrlen( SDBCM_CONF_PATH_FILE ) );
      rc = utilReadConfigureFile ( conf, desc, vm ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to read configure file[%s], rc = %d", conf, rc ) ;
         goto error ;
      }
      else if ( vm.count(hostname2) )
      {
         svcname = vm[hostname2].as<string>().c_str() ;
      }
      else if ( vm.count(SDBCM_CONF_DFTPORT) )
      {
         svcname = vm[SDBCM_CONF_DFTPORT].as<string>().c_str() ;
      }
      if ( svcname != NULL )
      {
         rc = ossGetPort( svcname, port ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Bad sdbcm listening service name: %s",
                     svcname ) ;
            goto error ;
         }
      }

      {
         UINT32 reserveSize = 0 ;
         UINT32 buffSize = 0 ;
         ossSocket sock ( hostname, port, OSS_SOCKET_DFT_TIMEOUT ) ;
         rc = sock.initSocket () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed initialize socket, rc=%d", rc ) ;
            goto error ;
         }
         rc = sock.connect () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed connect remote server[%s:%d], rc=%d",
                     hostname, port, rc ) ;
            goto error ;
         }
         rc = sock.disableNagle() ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to disable nagle, rc: %d", rc ) ;
         }
         // set keep alive
         rc = sock.setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                                 OSS_SOCKET_KEEP_INTERVAL,
                                 OSS_SOCKET_KEEP_CONTER ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to set keep alive, rc=%d", rc ) ;
         }

         // build message
         rc = msgBuildCMRequest ( &pCMRequest, &reqSize, remoCode,
                                   arg1, arg2, arg3, arg4 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to build cm request message, rc=%d",
                     rc ) ;
            goto error ;
         }

         finalRequest = pCMRequest ;
   reSend:
         // send message
         rc = pmdSend ( finalRequest, ((MsgHeader*)finalRequest)->messageLength,
                        &sock, eduCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send cm request message, rc=%d", rc ) ;
            goto error ;
         }

         // receive message
         rc = pmdRecv ( (CHAR*)&packetLength, sizeof (SINT32), &sock,
                        eduCB, OSS_SOCKET_DFT_TIMEOUT,
                        execTimeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to receive cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         else if ( (UINT32)packetLength < sizeof(MsgHeaderV1) ||
                   (UINT32)packetLength > SDB_MAX_MSG_LENGTH )
         {
            PD_LOG( PDERROR, "Recv msg size[%d] is less than "
                    "MsgHeader size[%d] or more than max msg size[%d]",
                    packetLength, sizeof(MsgHeaderV1), SDB_MAX_MSG_LENGTH ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         // free at the end of this function
         if ( buffSize < packetLength + reserveSize )
         {
            CHAR *newBuff =
                  (CHAR *)SDB_OSS_REALLOC( pReceiveBuffer,
                                           packetLength + reserveSize ) ;
            if ( !newBuff )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate memory[size: %u] failed, rc=%d",
                       packetLength + reserveSize, rc ) ;
               goto error ;
            }
            pReceiveBuffer = newBuff ;
            buffSize = packetLength + reserveSize ;
         }

         *(SINT32*)(pReceiveBuffer) = packetLength ;
         rc = pmdRecv ( &pReceiveBuffer[sizeof (SINT32)],
                        packetLength-sizeof (SINT32), &sock,
                        eduCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to receive cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         replyPtr = pReceiveBuffer ;

         if ( MSG_COMM_EYE_DEFAULT != ((MsgHeader*)pReceiveBuffer)->eye )
         {
            // The eye is not as expected, so the peer node is most likely to be
            // using old protocol. The first reply should report unknown message
            // as the original request has not been converted. So convert the
            // request and send again. And when the reply is received, it also
            // needs to be converted.
            MsgOpReplyV1 *reply = (MsgOpReplyV1 *)pReceiveBuffer ;
            INT32 result = reply->flags ;
            UINT32 finalSize = 0 ;
            BOOLEAN newConvertor = FALSE ;

            if ( !msgConvertor )
            {
               msgConvertor = SDB_OSS_NEW msgConvertorImpl ;
               if ( !msgConvertor )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Allocate memory for message "
                          "convertor[size: %d] failed, rc=%d",
                          sizeof(msgConvertorImpl), rc ) ;
                  goto error ;
               }
               newConvertor = TRUE ;
            }

            // If the result is not unknown message, it failed for some other
            // reason. No need to retry and just convert the reply.
            if ( newConvertor &&
                 ( SDB_UNKNOWN_MESSAGE == result ||
                   SDB_CLS_UNKNOW_MSG == result ) )
            {
               rc = msgConvertor->push( (CHAR *)pCMRequest,
                                        ((MsgHeader *)pCMRequest)->messageLength ) ;
               PD_RC_CHECK( rc, PDERROR, "Push cm request message into "
                            "message convertor failed, rc=%d", rc ) ;
               rc = msgConvertor->output( finalRequest, finalSize ) ;
               PD_RC_CHECK( rc, PDERROR, "Get converted cm request message "
                            "from the message convertor failed, rc=%d",
                            rc ) ;
               reserveSize = sizeof(MsgOpReply) - sizeof(MsgOpReplyV1) ;

               SDB_ASSERT( ((MsgHeaderV1 *)finalRequest)->messageLength
                           == (INT32)finalSize,
                           "Message length is invalid") ;
               goto reSend ;
            }
            else
            {
               // The converted message is process successfully by remote node.
               // Convert the reply.
               msgConvertor->reset( FALSE ) ;
               rc = msgConvertor->push( pReceiveBuffer, packetLength ) ;
               PD_RC_CHECK( rc, PDERROR, "Push cm reply message into message "
                            "convertor failed, rc=%d", rc ) ;
               rc = msgConvertor->output( replyPtr, finalSize ) ;
               PD_RC_CHECK( rc, PDERROR, "Get converted cm reply message from "
                            "message convertor failed, rc=%d", rc ) ;
            }
         }
         else if ( (UINT32)packetLength < sizeof(MsgHeader) )
         {
            PD_LOG( PDERROR, "Recv msg size[%d] is less than MsgHeader size[%d]",
                    packetLength, sizeof(MsgHeader) ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // process reply
      {
         SINT64 contextID  = 0 ;
         SINT32 startFrom = 0 ;
         SINT32 numReturned = 0 ;
         vector<BSONObj> objLst ;

         // extract message
         rc = msgExtractReply ( replyPtr, retCode, &contextID,
                                &startFrom, &numReturned, objLst ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to extract cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         SDB_ASSERT( contextID == -1, "Context id must be invalid" ) ;

         for ( UINT32 i = 0 ; i < objLst.size() ; ++i )
         {
            PD_LOG( PDEVENT, "RemoteExec recv obj: %s",
                    objLst[i].toString().c_str() ) ;
            if ( NULL != retObjs )
            {
               retObjs->push_back( objLst[i].copy() ) ;
            }
         }
      }

   done:
      if ( pCMRequest )
      {
         SDB_OSS_FREE ( pCMRequest ) ;
      }
      if ( pReceiveBuffer )
      {
         SDB_OSS_FREE ( pReceiveBuffer ) ;
      }
      if ( msgConvertor )
      {
         SDB_OSS_DEL msgConvertor ;
      }
      PD_TRACE_EXITRC ( SDB_REMOTEEXEC, rc ) ;
      return rc ;
   error:
      if ( NULL != retObjs )
      {
         retObjs->clear() ;
      }
      goto done ;
   }

}


