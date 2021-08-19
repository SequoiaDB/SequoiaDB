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

   Source File Name = sptRemote.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/


#include "sptRemote.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "ossTypes.h"
#include "omagentDef.hpp"
#include "network.h"
#include "common.h"
#include "sptCommon.hpp"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#define SDB_CLIENT_DFT_NETWORK_TIMEOUT 10000

namespace engine
{

/*
 define macro
*/
#define HANDLE_CHECK( handle, interhandle, handletype ) \
do                                                      \
{                                                       \
   if ( SDB_INVALID_HANDLE == handle )                  \
   {                                                    \
      rc = SDB_INVALIDARG ;                             \
      goto error ;                                      \
   }                                                    \
   if ( !interhandle ||                                 \
        handletype != interhandle->_handleType )        \
   {                                                    \
      rc = SDB_CLT_INVALID_HANDLE ;                     \
      goto error ;                                      \
   }                                                    \
}while( FALSE )


#define CHECK_RET_MSGHEADER( pSendBuf, pRecvBuf, connHandle )               \
do                                                                          \
{                                                                           \
   sdbConnectionStruct *db = (sdbConnectionStruct*)connHandle ;             \
   HANDLE_CHECK( connHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;             \
   rc = clientCheckRetMsgHeader( pSendBuf, pRecvBuf, db->_endianConvert ) ; \
   if ( SDB_OK != rc )                                                      \
   {                                                                        \
      if ( SDB_UNEXPECTED_RESULT == rc )                                    \
      {                                                                     \
         sdbDisconnect( connHandle ) ;                                      \
      }                                                                     \
      goto error ;                                                          \
   }                                                                        \
}while( FALSE )

   /*
      Local function define
   */

   static INT32 _extractErrorObj( const CHAR *pErrorBuf,
                                  INT32 *pFlag,
                                  const CHAR **ppErr,
                                  const CHAR **ppDetail )
   {
      INT32 rc = SDB_OK ;
      bson localobj ;
      bson_iterator it ;

      bson_init( &localobj ) ;

      if ( !pErrorBuf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = bson_init_finished_data( &localobj, pErrorBuf ) ;
      if ( rc )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      if ( pFlag && BSON_INT == bson_find( &it, &localobj, OP_ERRNOFIELD ) )
      {
         *pFlag = bson_iterator_int( &it ) ;
      }
      if ( ppErr && BSON_STRING == bson_find( &it, &localobj,
                                              OP_ERRDESP_FIELD ) )
      {
         *ppErr = bson_iterator_string( &it ) ;
      }
      if ( ppDetail && BSON_STRING == bson_find( &it, &localobj,
                                                 OP_ERR_DETAIL ) )
      {
         *ppDetail = bson_iterator_string( &it ) ;
      }

   done:
      bson_destroy( &localobj ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      define member functions
   */
   _sptRemote::_sptRemote()
   {
   }

   _sptRemote::~_sptRemote()
   {
   }

   INT32 _sptRemote::runCommand( ossValuePtr handle,
                                 const CHAR *pString,
                                 SINT32 flag,
                                 UINT64 reqID,
                                 SINT64 numToSkip,
                                 SINT64 numToReturn,
                                 const CHAR *arg1,
                                 const CHAR *arg2,
                                 const CHAR *arg3,
                                 const CHAR *arg4,
                                 CHAR **ppRetBuffer,
                                 INT32 &retCode,
                                 BOOLEAN needRecv )
   {
      SDB_ASSERT( handle, "handle can't be 0" ) ;
      SDB_ASSERT( pString, "pString can't be null" ) ;
      INT32 rc          = SDB_OK ;
      BOOLEAN extracted = FALSE ;
      SINT64 contextID  = 0 ;
      sdbConnectionStruct *connection = 0;
      retCode = SDB_OK ;

      if( 0 == handle )
      {
         rc = SDB_NETWORK ;
         PD_LOG( PDERROR, "network is closed, rc = %d", rc ) ;
         goto error ;
      }
      connection = (sdbConnectionStruct *) handle ;

      // build message
      rc = clientBuildQueryMsgCpp ( &( connection->_pSendBuffer ),
                                    &( connection->_sendBufferSize ),
                                    pString, flag, reqID,
                                    numToSkip, numToReturn,
                                    arg1, arg2, arg3, arg4,
                                    connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build query msg, rc = %d", rc ) ;


      // send and recv msg
      rc = _sendAndRecv( handle,
                         ( const MsgHeader* )connection->_pSendBuffer,
                         ( MsgHeader** )&( connection->_pReceiveBuffer ),
                         &( connection->_receiveBufferSize ),
                         needRecv, connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build send and recv msg, rc = %d", rc ) ;

      if ( needRecv )
      {
         // extract message
         rc = _extract( (MsgHeader *)connection->_pReceiveBuffer,
                        connection->_receiveBufferSize,
                        &contextID, extracted,
                        connection->_endianConvert ) ;

         if ( SDB_OK != rc )
         {
            if ( !extracted )
            {
               PD_LOG( PDERROR, "Failed to extract msg in client, rc = %d",
                       rc ) ;
               goto error ;
            }
            else
            {
               retCode = rc ;
               rc = SDB_OK ;
               PD_LOG( PDINFO, "Failed to run command in engine, rc = %d",
                       retCode ) ;
            }
         }

         // check whether the return message is what we want or not
         CHECK_RET_MSGHEADER( connection->_pSendBuffer,
                              connection->_pReceiveBuffer,
                              handle ) ;

         // try to get retObj
         rc = _getRetBuffer( connection->_pReceiveBuffer, ppRetBuffer ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get retObjArray, rc = %d", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_sendAndRecv( ossValuePtr handle,
                                   const MsgHeader *sendMsg,
                                   MsgHeader **recvMsg, INT32 *size,
                                   BOOLEAN needRecv,
                                   BOOLEAN endianConvert )
   {
      SDB_ASSERT( handle, "handle can't be null" ) ;
      SDB_ASSERT( sendMsg, "sendMsg can't be null" ) ;
      SDB_ASSERT( recvMsg, "recvMsg can't be null" ) ;
      SDB_ASSERT( size , "size can't be null" ) ;

      INT32 rc          = SDB_OK ;
      BOOLEAN hasLock   = FALSE ;
      sdbConnectionStruct *connection = (sdbConnectionStruct*)handle ;

      // check arguments
      if( NULL == connection->_sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if( NULL == sendMsg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMutexLock( &connection->_sockMutex ) ;
      hasLock = TRUE ;

      // send
      rc = _sendMsg ( handle, sendMsg, endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send msg, rc: %d", rc ) ;

      // recv
      if ( TRUE == needRecv )
      {
         rc = _recvMsg ( handle, recvMsg, size, endianConvert ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recv msg, rc: %d", rc ) ;
      }

   done:
      if ( TRUE == hasLock )
      {
         ossMutexUnlock( &connection->_sockMutex ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_sendMsg( ossValuePtr handle,
                               const MsgHeader *msg,
                               BOOLEAN endianConvert )
   {
      SDB_ASSERT( handle, "handle can't be null" ) ;
      SDB_ASSERT( msg, "msg can't be null" ) ;

      INT32 rc          = SDB_OK ;
      INT32 msgLength   = 0 ;
      ossEndianConvertIf4 ( msg->messageLength, msgLength, endianConvert ) ;

      rc = _send ( handle, (const CHAR*)msg, msgLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send msg, rc: %d", rc) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_recvMsg( ossValuePtr handle,
                               MsgHeader **msg,
                               INT32 *msgLength,
                               BOOLEAN endianConvert )
   {
      SDB_ASSERT( handle, "handle can't be null" ) ;
      SDB_ASSERT( msg, "msg can't be null" ) ;

      INT32 rc         = SDB_OK ;
      INT32 recvLength = 0 ;
      INT32 realLength = 0 ;
      INT32 receivedLen = 0 ;
      INT32 totalReceivedLen = 0 ;
      CHAR **ppBuffer  = (CHAR**)msg ;
      sdbConnectionStruct *connection = (sdbConnectionStruct*)handle ;

      if ( NULL == connection->_sock )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      while ( TRUE )
      {
         // get length first
         rc = clientRecv ( connection->_sock, 
                           ((CHAR*)&recvLength) + totalReceivedLen,
                           sizeof( recvLength ) - totalReceivedLen,
                           &receivedLen,
                           SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get length, rc: %d", rc) ;

#if defined( _LINUX ) || defined (_AIX)
      #if defined (_AIX)
         #define TCP_QUICKACK TCP_NODELAYACK
      #endif
      // quick ack
      {
         INT32 i = 1 ;
         setsockopt( clientGetRawSocket ( connection->_sock ),
                     IPPROTO_TCP, TCP_QUICKACK, (void*)&i, sizeof(i) ) ;
      }
#endif
         break ;
      }
      ossEndianConvertIf4 ( recvLength, realLength, endianConvert ) ;
      rc = _reallocBuffer ( ppBuffer, msgLength , realLength+1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to realloc buffer, rc: %d", rc) ;

      // use the original recvLength before convert
      *(SINT32*)(*ppBuffer) = recvLength ;
      totalReceivedLen = 0 ;
      receivedLen = 0 ;
      while ( TRUE )
      {
         // get residual message
         rc = clientRecv ( connection->_sock,
                           &( *ppBuffer )[sizeof( realLength ) + totalReceivedLen],
                           realLength - sizeof( realLength ) - totalReceivedLen,
                           &receivedLen,
                           SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc ) 
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get residual message, rc: %d", rc) ;
         break ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_send( ossValuePtr handle,
                            const CHAR *pMsg,
                            INT32 msgLength )
   {
      SDB_ASSERT( handle, "handle can't be null" ) ;
      SDB_ASSERT( pMsg, "pMsg can't be null" ) ;
      INT32 rc = SDB_OK ;
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      sdbConnectionStruct *connection = (sdbConnectionStruct*)handle ;

      if ( NULL == connection->_sock )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to get valid sock, rc: %d", rc ) ;
         goto error ;
      }

      while ( msgLength > totalSentSize )
      {
         rc = clientSend ( connection->_sock, pMsg + totalSentSize, 
                           msgLength - totalSentSize, &sentSize,
                           SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to send, rc: %d", rc) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_reallocBuffer( CHAR **ppBuffer, INT32 *bufferSize,
                                     INT32 newSize)
   {
      INT32 rc              = SDB_OK ;
      CHAR *pOriginalBuffer = NULL ;

      if ( *bufferSize < newSize )
      {
         pOriginalBuffer = *ppBuffer ;
         *ppBuffer = (CHAR*)SDB_OSS_REALLOC( *ppBuffer,
                                             sizeof(CHAR) *newSize ) ;
         if( !*ppBuffer )
         {
            *ppBuffer = pOriginalBuffer ;
            rc = SDB_OOM ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to realloc ppBuffer, rc: %d", rc) ;
         *bufferSize = newSize ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_extract( MsgHeader * msg, INT32 msgSize,
                               SINT64 * contextID,
                               BOOLEAN &extracted,
                               BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;
      INT32 replyFlag = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom = -1 ;
      CHAR *pBuffer = ( CHAR* )msg ;

      extracted = FALSE ;
      rc = clientExtractReply ( pBuffer, &replyFlag, contextID,
                                &startFrom, &numReturned, endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = replyFlag ;
      extracted = TRUE ;

      if ( SDB_OK != replyFlag && SDB_DMS_EOC != replyFlag )
      {
         INT32 dataOff     = 0 ;
         INT32 dataSize    = 0 ;
         const CHAR *pErr  = NULL ;
         const CHAR *pDetail = NULL ;
         const CHAR *pErrorBuf = NULL ;

         dataOff = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
         dataSize = msg->messageLength - dataOff ;
         /// save error info
         if ( dataSize > 0 )
         {
            pErrorBuf = ( const CHAR* )msg + dataOff ;
            if ( SDB_OK == _extractErrorObj( pErrorBuf,
                                             NULL, &pErr, &pDetail ) )
            {
               sdbErrorCallback( pErrorBuf, (UINT32)dataSize,
                                 replyFlag, pErr, pDetail ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_getRetBuffer( CHAR *pRetMsg, CHAR **ppRetBuffer )
   {
      INT32 rc     = SDB_OK ;
      INT32 offset = ossRoundUpToMultipleX( sizeof( MsgOpReply ), 4 ) ;

      MsgOpReply * msgReply = (MsgOpReply*)pRetMsg ;

      if ( NULL == ppRetBuffer )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get ppRetBuffer, rc: %d", rc) ;

      // init retBuffer
      if ( offset < msgReply->header.messageLength )
      {
         *ppRetBuffer = &pRetMsg[offset] ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to init ppRetBuffer, rc: %d", rc) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

