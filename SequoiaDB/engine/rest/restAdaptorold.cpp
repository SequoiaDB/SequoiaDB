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

   Source File Name = restAdaptor.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "restAdaptorold.hpp"
#include "ossMem.h"
#include "ossUtil.h"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "restTrace.hpp"

namespace engine
{
   _restAdaptoro::_restAdaptoro ()
   {
      CRLF[0] = 0x0D ;
      CRLF[1] = 0x0A ;
      CRLF[2] = 0 ;
      CRLF2[0] = 0x0D ;
      CRLF2[1] = 0x0A ;
      CRLF2[2] = 0x0D ;
      CRLF2[3] = 0x0A ;
      CRLF2[4] = 0 ;
   }
   _restAdaptoro::~_restAdaptoro ()
   {
   }
   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP_GETREQ, "_restAdaptor::getRequest" )
   INT32 _restAdaptoro::getRequest ( ossSocket &sock, CHAR **ppReceiveBuffer,
                                    INT32 *receiveBufferSize,
                                    BOOLEAN &needFetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP_GETREQ );
      INT32 buffLen = REST_ADAPTOR_RECV_BUFF_SIZE ;
      CHAR *buffer = (CHAR *)SDB_OSS_MALLOC ( buffLen ) ;
      if ( !buffer )
      {
         PD_LOG ( PDERROR, "Failed to malloc memory" ) ;
         goto error ;
      }
      ossMemset ( buffer, 0, buffLen ) ;
      rc = _receive ( sock, buffer, buffLen, REST_SOCKET_DFT_TIMEOUT ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call receive" ) ;
         goto error ;
      }
      needFetch = FALSE ;
      rc = _httpConvertMsg ( sock, buffer,
                             ppReceiveBuffer,
                             receiveBufferSize,
                             needFetch ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call httpConvertMsg" ) ;
         goto error ;
      }
   done :
      if ( buffer )
      {
         SDB_OSS_FREE ( buffer ) ;
      }
      PD_TRACE_EXITRC ( SDB__RESTADP_GETREQ, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP_SNDREP, "_restAdaptor::sendReply" )
   INT32 _restAdaptoro::sendReply ( MsgOpReply &replyHeader,
                                   const CHAR *pExtraBuffer,
                                   INT32 extraSize,
                                   ossSocket &sock,
                                   BOOLEAN needFetch,
                                   BOOLEAN &isSendHeader )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP_SNDREP );
      INT32 len = 0 ;
      string str ;
      INT32 sentLen = 0 ;
      if ( !isSendHeader )
      {
         isSendHeader = TRUE ;
         len = ossSnprintf ( httpHeader, REST_ADAPTOR_RECV_HEAD_SIZE,
                             "HTTP/1.1 200 OK%s\
Content-Type: text/html;charset=utf-8%s\
Transfer-Encoding: chunked%s\
Connection: close%s", CRLF, CRLF, CRLF, CRLF2 ) ;
         if ( 0 > len )
         {
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "Failed to formatting" ) ;
            goto error ;
         }
         rc = sock.send ( httpHeader, len, sentLen,
                          REST_SOCKET_DFT_TIMEOUT ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to sock.send" ) ;
            goto error ;
         }
         if ( !needFetch && !pExtraBuffer )
         {
            len = ossSnprintf ( httpHeader, REST_ADAPTOR_RECV_HEAD_SIZE,
                                "B%s{ errno: 0}%s0%s", CRLF, CRLF, CRLF2 ) ;
            if ( 0 > len )
            {
               rc = SDB_REST_EHS ;
               PD_LOG ( PDERROR, "Failed to formatting" ) ;
               goto error ;
            }
            rc = sock.send ( httpHeader, len, sentLen,
                             REST_SOCKET_DFT_TIMEOUT ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to sock.send" ) ;
               goto error ;
            }
         }
      }
      if ( pExtraBuffer )
      {
         INT32 _offset  = 0 ;
         BSONObj bson ;
         CHAR *pNote = NULL ;
         for ( INT32 i = 0; i < replyHeader.numReturned; ++i )
         {
            bson = BSONObj ( pExtraBuffer + _offset ) ;
            str = string ( bson.toString ( FALSE, FALSE ) ) ;
            pNote = (CHAR *)str.c_str () ;
            if ( replyHeader.flags != 0 )
            {
               if ( SDB_DMS_EOC == replyHeader.flags )
               {
                  len = ossSnprintf ( httpHeader,
                                      REST_ADAPTOR_RECV_HEAD_SIZE,
                                      "0%s", CRLF2 ) ;
                  if ( 0 > len )
                  {
                     rc = SDB_REST_EHS ;
                     PD_LOG ( PDERROR, "Failed to formatting" ) ;
                     goto error ;
                  }
                  rc = sock.send ( httpHeader, len, sentLen,
                                   REST_SOCKET_DFT_TIMEOUT ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to sock.send" ) ;
                     goto error ;
                  }
               }
               else
               {
                  len = ossStrlen ( pNote ) ;
                  len = ossSnprintf ( httpHeader,
                                      REST_ADAPTOR_RECV_HEAD_SIZE,
                                      "%X%s%s%s0%s",
                                      len, CRLF,
                                      pNote, CRLF,
                                      CRLF2 ) ;
                  if ( 0 > len )
                  {
                     rc = SDB_REST_EHS ;
                     PD_LOG ( PDERROR, "Failed to formatting" ) ;
                     goto error ;
                  }
                  rc = sock.send ( httpHeader, len, sentLen,
                                   REST_SOCKET_DFT_TIMEOUT ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to sock.send" ) ;
                     goto error ;
                  }
               }
            }
            else
            {
               len = ossStrlen ( pNote ) ;
               len = ossSnprintf ( httpBody,
                                   REST_ADAPTOR_RECV_BODY_SIZE,
                                   "%X%s%s%s", len, CRLF, pNote, CRLF ) ;
               rc = sock.send ( httpBody, len, sentLen,
                                REST_SOCKET_DFT_TIMEOUT ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to sock.send" ) ;
                  goto error ;
               }
            }
            _offset += ossRoundUpToMultipleX ( *(INT32*)&pExtraBuffer[_offset],
                                               4 ) ;
            if ( _offset > extraSize )
            {
               goto done ;
            }
         }//for
      }//if
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP_SNDREP, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__RECV, "_restAdaptor::_receive" )
   INT32 _restAdaptoro::_receive ( ossSocket &sock, CHAR *buffer,
                                  INT32 &len, INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      INT32 receivedLen = 0 ;
      PD_TRACE_ENTRY ( SDB__RESTADP__RECV );
      CHAR *readBuf = buffer ;
      INT32 tempLen = 0 ;
      INT32 bufferLen = len ;
      len = 0 ;

      while ( len < bufferLen )
      {
         tempLen = (len + REST_ADAPTOR_RECV_BUFF_SIZE) > bufferLen ?
                   bufferLen - len : REST_ADAPTOR_RECV_BUFF_SIZE ;
         rc = sock.recv ( buffer + len,
                          tempLen,
                          receivedLen,
                          timeout,
                          FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to call ossSocket recv" ) ;
            goto error ;
         }
         if ( ossStrstr ( readBuf + len - (len?REST_ADAPTOR_NUMBER_3:0),
                          CRLF2 ) )
         {
            goto done ;
         }
         len += tempLen ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__RECV, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__HTTPCVMSG, "_restAdaptor::_httpConvertMsg" )
   INT32 _restAdaptoro::_httpConvertMsg ( ossSocket &sock,
                                         CHAR *pBuffer,
                                         CHAR **ppReceiveBuffer,
                                         INT32 *receiveBufferSize,
                                         BOOLEAN &needFetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__HTTPCVMSG );
      INT32 operators = 0 ;
      INT32 len      = 0 ;

      CHAR indexBuf[REST_ADAPTOR_INDEX_SIZE] = {0} ;
      CHAR csName[REST_ADAPTOR_NAME_SIZE] = {0} ;
      CHAR *pCsName = csName ;
      CHAR *pClName = csName ;

      CHAR *pParameterBuf = NULL ;
      CHAR *pLenTemp = NULL ;

      if ( !ossStrstr ( pBuffer, CRLF2 ) )
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "Error http structure" ) ;
         goto error ;
      }
      if ( pBuffer[0] == 'P' && pBuffer[1] == 'O' &&
           pBuffer[2] == 'S' && pBuffer[3] == 'T' &&
           ( pBuffer[4] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[4] == REST_ADAPTOR_RECV_TABLE ) )
      {
         operators = REST_ADAPTOR_RECV_POST ;
         pBuffer += REST_ADAPTOR_STR_POST_LEN ;
      }
      else if ( pBuffer[0] == 'G' && pBuffer[1] == 'E' &&
                pBuffer[2] == 'T' &&
                ( pBuffer[3] == REST_ADAPTOR_RECV_SPACE ||
                  pBuffer[3] == REST_ADAPTOR_RECV_TABLE ) )
      {
         operators = REST_ADAPTOR_RECV_GET ;
         pBuffer += REST_ADAPTOR_STR_GET_LEN ;
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "Error http structure" ) ;
         goto error ;
      }
      pBuffer = _skip ( pBuffer ) ;
      while ( REST_ADAPTOR_STR_SLASH == pBuffer[0] )
      {
         ++pBuffer ;
         len = 0 ;
         if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_SNAPSHOT,
                            REST_ADAPTOR_STR_SNAPSHOT_LEN ) &&
         ( pBuffer[REST_ADAPTOR_STR_SNAPSHOT_LEN] == REST_ADAPTOR_STR_SLASH ||
           pBuffer[REST_ADAPTOR_STR_SNAPSHOT_LEN] == REST_ADAPTOR_RECV_SPACE ||
           pBuffer[REST_ADAPTOR_STR_SNAPSHOT_LEN] == REST_ADAPTOR_RECV_TABLE ) )
         {
            needFetch = TRUE ;
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_SNAPSHOT_LEN ;
            rc = _snapshotCommand ( ppReceiveBuffer, receiveBufferSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _snapshotCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_CREATE,
                                 REST_ADAPTOR_STR_CREATE_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_CREATE_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_CREATE_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_CREATE_LEN] == REST_ADAPTOR_RECV_TABLE ||
             pBuffer[REST_ADAPTOR_STR_CREATE_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_CREATE_LEN ;
            rc = _createCommand ( sock,
                                  ppReceiveBuffer,
                                  receiveBufferSize,
                                  pBuffer,
                                  pParameterBuf,
                                  csName,
                                  operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _createCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_DROP,
                                 REST_ADAPTOR_STR_DROP_LEN ) &&
             ( pBuffer[REST_ADAPTOR_STR_DROP_LEN] == REST_ADAPTOR_STR_SLASH ||
               pBuffer[REST_ADAPTOR_STR_DROP_LEN] == REST_ADAPTOR_RECV_SPACE ||
               pBuffer[REST_ADAPTOR_STR_DROP_LEN] == REST_ADAPTOR_RECV_TABLE ||
               pBuffer[REST_ADAPTOR_STR_DROP_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_DROP_LEN ;
            rc = _dropCommand ( sock,
                                ppReceiveBuffer,
                                receiveBufferSize,
                                pBuffer,
                                pParameterBuf,
                                csName,
                                operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _dropCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_SESSION,
                                 REST_ADAPTOR_STR_SESSION_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_SESSION_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_SESSION_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_SESSION_LEN] == REST_ADAPTOR_RECV_TABLE )
                 )
         {
            needFetch = TRUE ;
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_SESSION_LEN ;
            rc = _sessionCommand ( ppReceiveBuffer,
                                   receiveBufferSize,
                                   pParameterBuf ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _sessionCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_CONTEXT,
                                 REST_ADAPTOR_STR_CONTEXT_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_CONTEXT_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_CONTEXT_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_CONTEXT_LEN] == REST_ADAPTOR_RECV_TABLE )
                 )
         {
            needFetch = TRUE ;
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_CONTEXT_LEN ;
            rc = _contextCommand ( ppReceiveBuffer,
                                   receiveBufferSize,
                                   pParameterBuf ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _contextCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_QUERY,
                                 REST_ADAPTOR_STR_QUERY_LEN ) &&
            ( pBuffer[REST_ADAPTOR_STR_QUERY_LEN] == REST_ADAPTOR_STR_SLASH ||
              pBuffer[REST_ADAPTOR_STR_QUERY_LEN] == REST_ADAPTOR_RECV_SPACE ||
              pBuffer[REST_ADAPTOR_STR_QUERY_LEN] == REST_ADAPTOR_RECV_TABLE ||
              pBuffer[REST_ADAPTOR_STR_QUERY_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            needFetch = TRUE ;
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_QUERY_LEN ;
            rc =  _queryCommand ( sock,
                                  ppReceiveBuffer,
                                  receiveBufferSize,
                                  pBuffer,
                                  pParameterBuf,
                                  csName,
                                  operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _queryCommand" ) ;
               goto error ;
            }
            goto done ;
         }//if
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_UPDATE,
                                 REST_ADAPTOR_STR_UPDATE_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_UPDATE_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_UPDATE_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_UPDATE_LEN] == REST_ADAPTOR_RECV_TABLE ||
             pBuffer[REST_ADAPTOR_STR_UPDATE_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_UPDATE_LEN ;
            rc =  _updateCommand ( sock,
                                   ppReceiveBuffer,
                                   receiveBufferSize,
                                   pBuffer,
                                   pParameterBuf,
                                   csName,
                                   operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _updateCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_INSERT,
                                 REST_ADAPTOR_STR_INSERT_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_INSERT_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_INSERT_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_INSERT_LEN] == REST_ADAPTOR_RECV_TABLE ||
             pBuffer[REST_ADAPTOR_STR_INSERT_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_INSERT_LEN ;
            rc =  _insertCommand ( sock,
                                   ppReceiveBuffer,
                                   receiveBufferSize,
                                   pBuffer,
                                   pParameterBuf,
                                   csName,
                                   operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _insertCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_DELETE,
                                 REST_ADAPTOR_STR_DELETE_LEN ) &&
           ( pBuffer[REST_ADAPTOR_STR_DELETE_LEN] == REST_ADAPTOR_STR_SLASH ||
             pBuffer[REST_ADAPTOR_STR_DELETE_LEN] == REST_ADAPTOR_RECV_SPACE ||
             pBuffer[REST_ADAPTOR_STR_DELETE_LEN] == REST_ADAPTOR_RECV_TABLE ||
             pBuffer[REST_ADAPTOR_STR_DELETE_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_DELETE_LEN ;
            rc = _deleteCommand ( sock,
                                  ppReceiveBuffer,
                                  receiveBufferSize,
                                  pBuffer,
                                  pParameterBuf,
                                  csName,
                                  operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _deleteCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else if ( !ossStrncmp ( pBuffer, REST_ADAPTOR_STR_COUNT,
                                 REST_ADAPTOR_STR_COUNT_LEN ) &&
            ( pBuffer[REST_ADAPTOR_STR_COUNT_LEN] == REST_ADAPTOR_STR_SLASH ||
              pBuffer[REST_ADAPTOR_STR_COUNT_LEN] == REST_ADAPTOR_RECV_SPACE ||
              pBuffer[REST_ADAPTOR_STR_COUNT_LEN] == REST_ADAPTOR_RECV_TABLE ||
              pBuffer[REST_ADAPTOR_STR_COUNT_LEN] == REST_ADAPTOR_STR_QUESTION )
                 )
         {
            needFetch = TRUE ;
            pParameterBuf = pBuffer + REST_ADAPTOR_STR_COUNT_LEN ;
            rc = _countCommand ( sock,
                                 ppReceiveBuffer,
                                 receiveBufferSize,
                                 pBuffer,
                                 pParameterBuf,
                                 csName,
                                 operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _countCommand" ) ;
               goto error ;
            }
            goto done ;
         }
         else
         {
            if ( REST_ADAPTOR_RECV_SPACE == pBuffer[0] ||
                 REST_ADAPTOR_RECV_TABLE == pBuffer[0] )
            {
               break ;
            }
            if ( !pCsName[0] )
            {
               pLenTemp = _strEndTag ( pBuffer ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               len = pLenTemp - pBuffer ;
               if ( len > REST_ADAPTOR_NAME_SIZE )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               ossStrncpy ( csName, pBuffer, len ) ;
               len = _strReplace ( pCsName ) ;
               if ( 0 > len )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               pClName += len ;
            }
            else if ( !pClName[0] )
            {
               pClName[0] = REST_ADAPTOR_STR_POINT ;
               ++pClName ;
               pLenTemp = _strEndTag ( pBuffer ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               len = pLenTemp - pBuffer ;
               if ( len > REST_ADAPTOR_NAME_SIZE )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               ossStrncpy ( pClName, pBuffer, len ) ;
               len = _strReplace ( pClName ) ;
               if ( 0 > len )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            else if ( !indexBuf[0] )
            {
               pLenTemp = _strEndTag ( pBuffer ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               len = pLenTemp - pBuffer ;
               if ( len > REST_ADAPTOR_NAME_SIZE )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
               ossStrncpy ( indexBuf, pBuffer, len ) ;
               len = _strReplace ( indexBuf ) ;
               if ( 0 > len )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            else
            {
               rc = SDB_REST_EHS ;
               PD_LOG ( PDERROR, "error http struct" ) ;
               goto error ;
            }
            pBuffer += len ;
            pBuffer = _skip ( pBuffer ) ;
         }
      }//while

      if ( ossStrstr ( pBuffer, REST_ADAPTOR_STR_HTTP ) )
      {
         rc = _defaultCommand ( ppReceiveBuffer,
                                receiveBufferSize,
                                csName,
                                pCsName,
                                pClName,
                                indexBuf,
                                needFetch ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Faile to call _defaultCommand" ) ;
            goto error ;
         }
         goto done ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__HTTPCVMSG, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__STRET, "_restAdaptor::_strEndTag" )
   CHAR *_restAdaptoro::_strEndTag ( CHAR *in )
   {
      PD_TRACE_ENTRY ( SDB__RESTADP__STRET );
       while ( in && *in &&
               (UINT32)*in != REST_ADAPTOR_STR_SLASH &&
               (UINT32)*in != REST_ADAPTOR_RECV_SPACE &&
               (UINT32)*in != REST_ADAPTOR_RECV_TABLE )
      {
         ++in ;
      }
      if ( !in )
      {
         PD_TRACE_EXIT ( SDB__RESTADP__STRET );
         return NULL ;
      }
      else
      {
         PD_TRACE_EXIT ( SDB__RESTADP__STRET );
         return in ;
      }
   }

   CHAR *_restAdaptoro::_skip( CHAR *in )
   {
      while ( in && *in &&
              ( (UINT32)*in == REST_ADAPTOR_RECV_SPACE ||
              (UINT32)*in == REST_ADAPTOR_RECV_TABLE ) )
      {
         ++in ;
      }
      return in ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__SKIP2PM, "_restAdaptor::_skipToParameter" )
   INT32 _restAdaptoro::_skipToParameter( ossSocket &sock,
                                         CHAR *buffer,
                                         CHAR **temp,
                                         INT32 operators,
                                         BOOLEAN &isMalloc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__SKIP2PM );
      if ( REST_ADAPTOR_RECV_POST == operators )
      {
         rc = _getPostBody ( sock, &buffer, temp, isMalloc ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to call _getPostBody" ) ;
            goto error ;
         }
      }
      else if ( REST_ADAPTOR_RECV_GET == operators )
      {
         if ( REST_ADAPTOR_STR_QUESTION == (*temp)[0] )
         {
            *temp = _skip ( *temp + 1 ) ;
         }
         else
         {
            if ( REST_ADAPTOR_STR_SLASH == (*temp)[0] )
            {
               ++(*temp) ;
               if ( REST_ADAPTOR_STR_QUESTION == (*temp)[0] )
               {
                  *temp = _skip ( (*temp) + 1 ) ;
               }
               else
               {
                  *temp = _skip ( (*temp) ) ;
               }
            }
            else
            {
               *temp = _skip ( *temp ) ;
            }
         }
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
done :
   PD_TRACE_EXITRC ( SDB__RESTADP__SKIP2PM, rc );
   return rc ;
error :
   goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__GETPSTBODY, "_restAdaptor::_getPostBody" )
   INT32 _restAdaptoro::_getPostBody( ossSocket &sock,
                                     CHAR **buffer,
                                     CHAR **temp,
                                     BOOLEAN &isMalloc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__GETPSTBODY );
      INT32 postLength = 0 ;
      INT32 len = 0 ;
      INT32 bodyLen = 0 ;
      CHAR *bodyTemp = NULL ;

      *temp = ossStrstr ( *buffer, REST_ADAPTOR_STR_CONLEN ) ;
      if ( !*temp )
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
      *temp += REST_ADAPTOR_STR_CONLEN_LEN ;
      *temp = _skip ( *temp ) ;
      if ( REST_ADAPTOR_STR_COLON != *temp[0] )
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
      *temp = _skip ( *temp + 1 ) ;
      postLength = ossAtoi ( *temp ) ;
      *temp = ossStrstr ( *buffer, CRLF2 ) ;
      if ( !*temp )
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
      *temp += REST_ADAPTOR_NUMBER_4 ;
      len = ossStrlen ( *temp ) ;
      if ( postLength > len )
      {
         bodyTemp = (CHAR *)SDB_OSS_MALLOC ( postLength ) ;
         if ( !bodyTemp )
         {
            SDB_OSS_FREE ( bodyTemp ) ;
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "error http struct" ) ;
            goto error ;
         }
         if ( len > 0 )
         {
            ossStrncpy ( bodyTemp, *temp, len ) ;
         }
         bodyLen = postLength -len ;
         rc = _receive ( sock,
                         bodyTemp + len,
                         bodyLen,
                         REST_SOCKET_DFT_TIMEOUT ) ;
         if ( rc )
         {
            SDB_OSS_FREE ( bodyTemp ) ;
            PD_LOG ( PDERROR, "failed to call _receive" ) ;
            goto error ;
         }
         if ( bodyLen != postLength - len )
         {
            SDB_OSS_FREE ( bodyTemp ) ;
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "error http struct" ) ;
            goto error ;
         }
         *temp = bodyTemp ;
         isMalloc = TRUE ;
      }
      else if ( postLength == len )
      {
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__GETPSTBODY, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__GETPM ,"_restAdaptor::_getParameter" )
   INT32 _restAdaptoro::_getParameter( CHAR **temp,
                                      CHAR **parameter,
                                      INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__GETPM );
      CHAR *lenTemp = NULL ;
      if ( REST_ADAPTOR_STR_EQUALSIGN != *temp[0] )
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
      *temp = _skip ( *temp + 1 ) ;
      *parameter = *temp ;
      lenTemp = ossStrchr ( *temp, REST_ADAPTOR_STR_AMPERSAND ) ;
      if ( !lenTemp )
      {
         lenTemp = ossStrchr ( *temp, REST_ADAPTOR_RECV_SPACE ) ;
         if ( !lenTemp )
         {
            lenTemp = ossStrchr ( *temp, REST_ADAPTOR_RECV_TABLE ) ;
            if ( !lenTemp )
            {
               lenTemp = ossStrchr ( *temp, 0 ) ;
               if ( !lenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }//(table)
         }//(space)
      }//'&'

      lenTemp[0] = 0 ;
      *temp = lenTemp + 1 ;
      if ( !ossIsUTF8 ( *parameter ) )
      {
         rc = SDB_OSS_CCE ;
         PD_LOG ( PDERROR, "character coding error" ) ;
         goto error ;
      }
      if ( REST_ADAPTOR_RECV_GET == operators )
      {
         rc = _strReplace ( *parameter ) ;
         if ( 0 > rc )
         {
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "error http struct" ) ;
            goto error ;
         }
         rc = SDB_OK ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__GETPM, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__RUNCMD, "_restAdaptor::_runCommand" )
   INT32 _restAdaptoro::_runCommand ( CHAR **ppReceiveBuffer,
                                     INT32 *receiveBufferSize,
                                     CHAR *pSring,
                                     INT32 numToReturn,
                                     CHAR *arg1,
                                     CHAR *arg2,
                                     CHAR *arg3,
                                     CHAR *arg4 )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__RUNCMD );
      BSONObj *Barg1 = NULL ;
      BSONObj *Barg2 = NULL ;
      BSONObj *Barg3 = NULL ;
      BSONObj *Barg4 = NULL ;
      try
      {
         if ( arg1 )
         {
            Barg1 = SDB_OSS_NEW BSONObj () ;
            if ( !Barg1 )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( arg1, *Barg1 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s", arg1 ) ;
               goto error ;
            }
         }
         if ( arg2 )
         {
            Barg2 = SDB_OSS_NEW BSONObj () ;
            if ( !Barg2 )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( arg2, *Barg2 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to parse line into BSONObj: %s", arg2 ) ;
               goto error ;
            }
         }
         if ( arg3 )
         {
            Barg3 = SDB_OSS_NEW BSONObj () ;
            if ( !Barg3 )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }

            rc = fromjson ( arg3, *Barg3 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to parse line into BSONObj: %s", arg3 ) ;
               goto error ;
            }
         }
         if ( arg4 )
         {
            Barg4 = SDB_OSS_NEW BSONObj () ;
            if ( !Barg4 )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }

            rc = fromjson ( arg4, *Barg4 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to parse line into BSONObj: %s", arg4 ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct, %s", e.what() ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg ( ppReceiveBuffer, receiveBufferSize,
                              pSring,
                              0,
                              0,
                              -1,
                              numToReturn,
                              Barg1,
                              Barg2,
                              Barg3,
                              Barg4 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to call msgBuildQueryMsg" ) ;
         goto error ;
      }
   done :
      if ( Barg1 )
         SDB_OSS_DEL Barg1 ;
      if ( Barg2 )
         SDB_OSS_DEL Barg2 ;
      if ( Barg3 )
         SDB_OSS_DEL Barg3 ;
      if ( Barg4 )
         SDB_OSS_DEL Barg4 ;
      PD_TRACE_EXITRC ( SDB__RESTADP__RUNCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__SNAPCMD, "_restAdaptor::_snapshotCommand" )
   INT32 _restAdaptoro::_snapshotCommand ( CHAR **ppReceiveBuffer,
                                          INT32 *receiveBufferSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__SNAPCMD );
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      ossStrcpy ( command, "$snapshot database" ) ;
      rc = _runCommand ( ppReceiveBuffer, receiveBufferSize, command ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Faile to call _runCommand" ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__SNAPCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__CRTCMD, "_restAdaptor::_createCommand" )
   INT32 _restAdaptoro::_createCommand ( ossSocket &sock,
                                        CHAR **ppReceiveBuffer,
                                        INT32 *receiveBufferSize,
                                        CHAR *pBuffer,
                                        CHAR *pParameterBuf,
                                        CHAR *csFullName,
                                        INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__CRTCMD );
      CHAR *pTemporary = NULL ;
      CHAR *pLenTemp   = NULL ;
      CHAR *pCsName    = NULL ;
      CHAR *pClName    = NULL ;
      CHAR *pKey       = NULL ;
      CHAR *pIndexName = NULL ;
      CHAR *pUnique    = NULL ;
      BOOLEAN isMalloc = FALSE ;
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Faile to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CSNAME,
                             REST_ADAPTOR_STR_CSNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CSNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCsName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CLNAME,
                                 REST_ADAPTOR_STR_CLNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CLNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pClName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_KEY,
                                 REST_ADAPTOR_STR_KEY_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_KEY_LEN ) ;
            rc = _getParameter ( &pTemporary, &pKey, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_INDEXNAME,
                                 REST_ADAPTOR_STR_INDEXNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_INDEXNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pIndexName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_UNIQUE,
                                 REST_ADAPTOR_STR_UNIQUE_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_UNIQUE_LEN ) ;
            rc = _getParameter ( &pTemporary, &pUnique, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for
      if ( pCsName && !csFullName[0] && !pKey && !pIndexName && !pUnique )
      {
         ossStrcpy ( command, "$create collectionspace" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{name:\"%s\",pagesize:%d}",
                       pCsName,
                       REST_PAGESIZE_DEFAULT ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Faile to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pClName && csFullName[0] && !pKey && !pIndexName && !pUnique )
      {
         ossStrcpy ( command, "$create collection" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{name:\"%s.%s\"}",
                       csFullName,
                       pClName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Faile to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pKey && pIndexName &&
                !pCsName && !pClName && csFullName[0] )
      {
         ossStrcpy ( command, "$create index" ) ;
         if ( pUnique )
         {
            ossSnprintf ( Strings,
                          REST_ADAPTOR_MSG_COMMAND,
                          "{collection:\"%s\",\
index:{key:%s,name:\"%s\",unique:%s}}",
                          csFullName,
                          pKey,
                          pIndexName,
                          pUnique ) ;
         }
         else
         {
            ossSnprintf ( Strings,
                          REST_ADAPTOR_MSG_COMMAND,
                          "{collection:\"%s\",\
index:{key:%s,name:\"%s\",unique:false}}",
                          csFullName,
                          pKey,
                          pIndexName ) ;
         }
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      PD_TRACE_EXITRC ( SDB__RESTADP__CRTCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__DROPCMD, "_restAdaptor::_dropCommand" )
   INT32 _restAdaptoro::_dropCommand ( ossSocket &sock,
                                      CHAR **ppReceiveBuffer,
                                      INT32 *receiveBufferSize,
                                      CHAR *pBuffer,
                                      CHAR *pParameterBuf,
                                      CHAR *csFullName,
                                      INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__DROPCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR *pTemporary = NULL ;
      CHAR *pLenTemp   = NULL ;
      CHAR *pCsName    = NULL ;
      CHAR *pClName    = NULL ;
      CHAR *pIndexName = NULL ;

      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;

      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CSNAME,
                            REST_ADAPTOR_STR_CSNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CSNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCsName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CLNAME,
                                 REST_ADAPTOR_STR_CLNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CLNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pClName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_INDEXNAME,
                                 REST_ADAPTOR_STR_INDEXNAME_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_INDEXNAME_LEN ) ;
            rc = _getParameter ( &pTemporary, &pIndexName, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Faile to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for
      if ( pCsName && !csFullName[0] && !pIndexName )
      {
         ossStrcpy ( command, "$drop collectionspace" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{name:\"%s\"}",
                       pCsName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pClName && csFullName[0] && !pIndexName )
      {
         ossStrcpy ( command, "$drop collection" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{name:\"%s.%s\"}",
                       csFullName,
                       pClName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pIndexName &&
                !pCsName && !pClName && csFullName[0] )
      {
         ossStrcpy ( command, "$drop index" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{collection:\"%s\",\
index:{\"\":\"%s\"}}",
                       csFullName,
                       pIndexName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }
   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      PD_TRACE_EXITRC ( SDB__RESTADP__DROPCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__SNCMD, "_restAdaptor::_sessionCommand" )
   INT32 _restAdaptoro::_sessionCommand ( CHAR **ppReceiveBuffer,
                                         INT32 *receiveBufferSize,
                                         CHAR *pParameterBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__SNCMD );
      CHAR *pLenTemp = NULL ;
      INT32 len = 0 ;
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      if ( REST_ADAPTOR_STR_SLASH == pParameterBuf[0] )
      {
         ++pParameterBuf ;
         if ( REST_ADAPTOR_RECV_SPACE == pParameterBuf[0] ||
              REST_ADAPTOR_RECV_TABLE == pParameterBuf[0] )
         {
            ossStrcpy ( command, "$list sessions" ) ;
            rc = _runCommand ( ppReceiveBuffer, receiveBufferSize, command ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = _strEndTag ( pParameterBuf ) ;
            len = pLenTemp - pParameterBuf ;
            pParameterBuf[len] = 0 ;
            ossStrcpy ( command, "$snapshot sessions" ) ;
            ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{SessionID:%s}",
                       pParameterBuf ) ;
            rc = _runCommand ( ppReceiveBuffer,
                               receiveBufferSize,
                               command,
                               -1,
                               Strings ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
      }
      else
      {
         if ( REST_ADAPTOR_RECV_SPACE == pParameterBuf[0] ||
              REST_ADAPTOR_RECV_TABLE == pParameterBuf[0] )
         {
            ossStrcpy ( command, "$list sessions" ) ;
            rc = _runCommand ( ppReceiveBuffer, receiveBufferSize, command ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "error http struct" ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__SNCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__CONTXCMD, "_restAdaptor::_contextCommand" )
   INT32 _restAdaptoro::_contextCommand ( CHAR **ppReceiveBuffer,
                                         INT32 *receiveBufferSize,
                                         CHAR *pParameterBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__CONTXCMD );
      CHAR *pLenTemp = NULL ;
      INT32 len = 0 ;
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      if ( REST_ADAPTOR_STR_SLASH == pParameterBuf[0] )
      {
         ++pParameterBuf ;
         if ( REST_ADAPTOR_RECV_SPACE == pParameterBuf[0] ||
              REST_ADAPTOR_RECV_TABLE == pParameterBuf[0] )
         {
            ossStrcpy ( command, "$list contexts" ) ;
            rc = _runCommand ( ppReceiveBuffer, receiveBufferSize, command ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = _strEndTag ( pParameterBuf ) ;
            len = pLenTemp - pParameterBuf ;
            pParameterBuf[len] = 0 ;
            ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{SessionID:%s}",
                       pParameterBuf ) ;
            ossStrcpy ( command, "$snapshot contexts" ) ;
            rc = _runCommand ( ppReceiveBuffer,
                               receiveBufferSize,
                               command,
                               -1,
                               Strings ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
      }
      else
      {
         if ( REST_ADAPTOR_RECV_SPACE == pParameterBuf[0] ||
              REST_ADAPTOR_RECV_TABLE == pParameterBuf[0] )
         {
            ossStrcpy ( command, "$list contexts" ) ;
            rc = _runCommand ( ppReceiveBuffer, receiveBufferSize, command ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "error http struct" ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__CONTXCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__QCMD, "_restAdaptor::_queryCommand" )
   INT32 _restAdaptoro::_queryCommand ( ossSocket &sock,
                                       CHAR **ppReceiveBuffer,
                                       INT32 *receiveBufferSize,
                                       CHAR *pBuffer,
                                       CHAR *pParameterBuf,
                                       CHAR *csName,
                                       INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__QCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR *pLenTemp = NULL ;

      CHAR *pTemporary = NULL ;
      CHAR *pCondition = NULL ;
      CHAR *pSelector  = NULL ;
      CHAR *pOrderby   = NULL ;
      CHAR *pHint      = NULL ;
      CHAR *pSkiprows  = NULL ;
      CHAR *pLimit     = NULL ;

      BSONObj *BsCondition = NULL ;
      BSONObj *BsSelector = NULL ;
      BSONObj *BsOrderby = NULL ;
      BSONObj *BsHint = NULL ;

      SINT32 flag = 0 ;
      UINT64 reqID = 0 ;
      INT64 numToskip = 0 ;
      INT64 numToReturn = -1 ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CONDITION,
                            REST_ADAPTOR_STR_CONDITION_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CONDITION_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCondition, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_SELECTOR,
                                 REST_ADAPTOR_STR_SELECTOR_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_SELECTOR_LEN ) ;
            rc = _getParameter ( &pTemporary, &pSelector, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_ORDERBY,
                                 REST_ADAPTOR_STR_ORDERBY_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_ORDERBY_LEN ) ;
            rc = _getParameter ( &pTemporary, &pOrderby, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_HINT,
                                 REST_ADAPTOR_STR_HINT_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_HINT_LEN ) ;
            rc = _getParameter ( &pTemporary, &pHint, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_SKIPROWS,
                                 REST_ADAPTOR_STR_SKIPROWS_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_SKIPROWS_LEN ) ;
            rc = _getParameter ( &pTemporary, &pSkiprows, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_LIMIT,
                                 REST_ADAPTOR_STR_LIMIT_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_LIMIT_LEN ) ;
            rc = _getParameter ( &pTemporary, &pLimit, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for

      if ( pSkiprows )
         numToskip = ossAtoi ( pSkiprows ) ;
      if ( pLimit )
         numToReturn = ossAtoi ( pLimit ) ;
      try
      {
         if ( pCondition )
         {
            BsCondition = SDB_OSS_NEW BSONObj () ;
            if ( !BsCondition )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pCondition, *BsCondition ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pCondition ) ;
               goto error ;
            }
         }
         if ( pSelector )
         {
            BsSelector = SDB_OSS_NEW BSONObj () ;
            if ( !BsSelector )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pSelector, *BsSelector ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pSelector ) ;
               goto error ;
            }
         }
         if ( pOrderby )
         {
            BsOrderby = SDB_OSS_NEW BSONObj () ;
            if ( !BsOrderby )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pOrderby, *BsOrderby ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pOrderby ) ;
               goto error ;
            }
         }
         if ( pHint )
         {
            BsHint = SDB_OSS_NEW BSONObj () ;
            if ( !BsHint )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pHint, *BsHint ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pHint ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct, %s", e.what() ) ;
         goto error ;
      }
      flag = 0 ;
      reqID = 0 ;
      rc = msgBuildQueryMsg ( ppReceiveBuffer, receiveBufferSize,
                              csName,
                              flag,
                              reqID,
                              numToskip,
                              numToReturn,
                              BsCondition,
                              BsSelector,
                              BsOrderby,
                              BsHint ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call msgBuildQueryMsg" ) ;
         goto error ;
      }

   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      if ( BsCondition )
         SDB_OSS_DEL BsCondition ;
      if ( BsSelector )
         SDB_OSS_DEL BsSelector ;
      if ( BsOrderby )
         SDB_OSS_DEL BsOrderby ;
      if ( BsHint )
         SDB_OSS_DEL BsHint ;
      PD_TRACE_EXITRC ( SDB__RESTADP__QCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__UPCMD, "_restAdaptor::_updateCommand" )
   INT32 _restAdaptoro::_updateCommand ( ossSocket &sock,
                                        CHAR **ppReceiveBuffer,
                                        INT32 *receiveBufferSize,
                                        CHAR *pBuffer,
                                        CHAR *pParameterBuf,
                                        CHAR *csName,
                                        INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__UPCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR *pTemporary = NULL ;
      CHAR *pLenTemp = NULL ;

      CHAR *pCondition = NULL ;
      CHAR *pRule      = NULL ;
      CHAR *pHint      = NULL ;

      BSONObj *BsCondition = NULL ;
      BSONObj *BsHint = NULL ;
      BSONObj *BsRule = NULL ;

      SINT32 flag = 0 ;
      UINT64 reqID = 0 ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CONDITION,
                            REST_ADAPTOR_STR_CONDITION_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CONDITION_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCondition, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_RULE,
                                 REST_ADAPTOR_STR_RULE_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_RULE_LEN ) ;
            rc = _getParameter ( &pTemporary, &pRule, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_HINT,
                                 REST_ADAPTOR_STR_HINT_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_HINT_LEN ) ;
            rc = _getParameter ( &pTemporary, &pHint, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for
      if ( !pRule )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct" ) ;
         goto error ;
      }
      try
      {
         BsRule = SDB_OSS_NEW BSONObj () ;
         if ( !BsRule )
         {
            PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
            rc = SDB_OOM ;
         }
         rc = fromjson ( pRule, *BsRule ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to parse line into BSONObj: %s",
                     pRule ) ;
            goto error ;
         }
         if ( pCondition )
         {
            BsCondition = SDB_OSS_NEW BSONObj () ;
            if ( !BsCondition )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pCondition, *BsCondition ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pCondition ) ;
               goto error ;
            }
         }
         if ( pHint )
         {
            BsHint = SDB_OSS_NEW BSONObj () ;
            if ( !BsHint )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pHint, *BsHint ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pHint ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct, %s", e.what() ) ;
         goto error ;
      }
      flag = 0 ;
      reqID = 0 ;
      rc = msgBuildUpdateMsg ( ppReceiveBuffer, receiveBufferSize,
                               csName,
                               flag,
                               reqID,
                               BsCondition,
                               BsRule,
                               BsHint ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call msgBuildUpdateMsg" ) ;
         goto error ;
      }
      goto done ;

   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      if ( BsCondition )
         SDB_OSS_DEL BsCondition ;
      if ( BsHint )
         SDB_OSS_DEL BsHint ;
      if ( BsRule )
         SDB_OSS_DEL BsRule ;
      PD_TRACE_EXITRC ( SDB__RESTADP__UPCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__INSCMD, "_restAdaptor::_insertCommand" )
   INT32 _restAdaptoro::_insertCommand ( ossSocket &sock,
                                        CHAR **ppReceiveBuffer,
                                        INT32 *receiveBufferSize,
                                        CHAR *pBuffer,
                                        CHAR *pParameterBuf,
                                        CHAR *csName,
                                        INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__INSCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR *pTemporary = NULL ;
      CHAR *pRecord    = NULL ;
      CHAR *pLenTemp   = NULL ;

      BSONObj *BsRecord = NULL ;

      SINT32 flag = 0 ;
      UINT64 reqID = 0 ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_RECORD,
                            REST_ADAPTOR_STR_RECORD_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_RECORD_LEN ) ;
            rc = _getParameter ( &pTemporary, &pRecord, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for

      if ( !pRecord )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct" ) ;
         goto error ;
      }
      try
      {
         BsRecord = SDB_OSS_NEW BSONObj () ;
         if ( !BsRecord )
         {
            PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
            rc = SDB_OOM ;
         }
         rc = fromjson ( pRecord, *BsRecord ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to parse line into BSONObj: %s",
                     pRecord ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct, %s", e.what() ) ;
         goto error ;
      }
      flag = 0 ;
      reqID = 0 ;
      rc = msgBuildInsertMsg ( ppReceiveBuffer,
                               receiveBufferSize,
                               csName,
                               flag,
                               reqID,
                               BsRecord ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call msgBuildInsertMsg" ) ;
         goto error ;
      }
      goto done ;

   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      if ( BsRecord )
         SDB_OSS_DEL BsRecord ;
      PD_TRACE_EXITRC ( SDB__RESTADP__INSCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__DELCMD, "_restAdaptor::_deleteCommand" )
   INT32 _restAdaptoro::_deleteCommand ( ossSocket &sock,
                                        CHAR **ppReceiveBuffer,
                                        INT32 *receiveBufferSize,
                                        CHAR *pBuffer,
                                        CHAR *pParameterBuf,
                                        CHAR *csName,
                                        INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__DELCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR *pTemporary = NULL ;
      CHAR *pLenTemp   = NULL ;

      CHAR *pCondition = NULL ;
      CHAR *pHint      = NULL ;

      BSONObj *BsCondition = NULL ;
      BSONObj *BsHint = NULL ;

      SINT32 flag = 0 ;
      UINT64 reqID = 0 ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CONDITION,
                            REST_ADAPTOR_STR_CONDITION_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CONDITION_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCondition, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_HINT,
                                 REST_ADAPTOR_STR_HINT_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_HINT_LEN ) ;
            rc = _getParameter ( &pTemporary, &pHint, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for
      try
      {
         if ( pCondition )
         {
            BsCondition = SDB_OSS_NEW BSONObj () ;
            if ( !BsCondition )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pCondition, *BsCondition ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pCondition ) ;
               goto error ;
            }
         }
         if ( pHint )
         {
            BsHint = SDB_OSS_NEW BSONObj () ;
            if ( !BsHint )
            {
               PD_LOG ( PDERROR, "Failed to allocate bsonobj" ) ;
               rc = SDB_OOM ;
            }
            rc = fromjson ( pHint, *BsHint ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                       "Failed to parse line into BSONObj: %s",
                       pHint ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "error json struct, %s", e.what() ) ;
         goto error ;
      }
      flag = 0 ;
      reqID = 0 ;
      rc = msgBuildDeleteMsg ( ppReceiveBuffer, receiveBufferSize,
                               csName,
                               flag,
                               reqID,
                               BsCondition,
                               BsHint ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call msgBuildDeleteMsg" ) ;
         goto error ;
      }
   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      if ( BsCondition )
         SDB_OSS_DEL BsCondition ;
      if ( BsHint )
         SDB_OSS_DEL BsHint ;
      PD_TRACE_EXITRC ( SDB__RESTADP__DELCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__CNTCMD, "_restAdaptor::_countCommand" )
   INT32 _restAdaptoro::_countCommand ( ossSocket &sock,
                                       CHAR **ppReceiveBuffer,
                                       INT32 *receiveBufferSize,
                                       CHAR *pBuffer,
                                       CHAR *pParameterBuf,
                                       CHAR *csName,
                                       INT32 operators )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__CNTCMD );
      BOOLEAN isMalloc = FALSE ;
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;

      CHAR *pTemporary = NULL ;
      CHAR *pLenTemp   = NULL ;

      CHAR *pCondition = NULL ;

      rc = _skipToParameter ( sock,
                              pBuffer,
                              &pParameterBuf,
                              operators,
                              isMalloc ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _skipToParameter" ) ;
         goto error ;
      }
      pTemporary = pParameterBuf ;
      while ( 0 != pTemporary[0] )
      {
         if ( !ossStrncmp ( pTemporary, REST_ADAPTOR_STR_CONDITION,
                            REST_ADAPTOR_STR_CONDITION_LEN ) )
         {
            pTemporary = _skip ( pTemporary + REST_ADAPTOR_STR_CONDITION_LEN ) ;
            rc = _getParameter ( &pTemporary, &pCondition, operators ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _getParameter" ) ;
               goto error ;
            }
         }
         else
         {
            pLenTemp = ossStrchr ( pTemporary,
                                   REST_ADAPTOR_STR_AMPERSAND ) ;
            if ( !pLenTemp )
            {
               pLenTemp = ossStrchr( pTemporary, 0 ) ;
               if ( !pLenTemp )
               {
                  rc = SDB_REST_EHS ;
                  PD_LOG ( PDERROR, "error http struct" ) ;
                  goto error ;
               }
            }
            pTemporary = pLenTemp ;
         }//if
      }//for

      ossStrcpy ( command, "$get count" ) ;
      ossSnprintf ( Strings,
                    REST_ADAPTOR_MSG_COMMAND,
                    "{collection:\"%s\"}",
                    csName ) ;
      rc = _runCommand ( ppReceiveBuffer,
                         receiveBufferSize,
                         command,
                         -1,
                         pCondition,
                         NULL,
                         NULL,
                         Strings ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _runCommand" ) ;
         goto error ;
      }
      goto done ;
   done :
      if ( isMalloc )
      {
         SDB_OSS_FREE ( pParameterBuf ) ;
      }
      PD_TRACE_EXITRC ( SDB__RESTADP__CNTCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__DFCMD, "_restAdaptor::_defaultCommand" )
   INT32 _restAdaptoro::_defaultCommand ( CHAR **ppReceiveBuffer,
                                         INT32 *receiveBufferSize,
                                         CHAR *csFullName,
                                         CHAR *pCsName,
                                         CHAR *pClName,
                                         CHAR *indexBuf,
                                         BOOLEAN &needFetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RESTADP__DFCMD );

      INT64 numToskip = 0 ;
      INT64 numToReturn = -1 ;
      SINT32 flag = 0 ;
      UINT64 reqID = 0 ;

      BSONObj *BsCondition = NULL ;
      BSONObj *BsSelector = NULL ;
      BSONObj *BsOrderby = NULL ;
      BSONObj *BsHint = NULL ;

      CHAR Strings[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      CHAR command[REST_ADAPTOR_MSG_COMMAND] = {0} ;
      if ( !pCsName[0] && !pClName[0] && !indexBuf[0] )
      {
         needFetch = TRUE ;
         ossStrcpy ( command, "$list collectionspaces" ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pCsName[0] && !pClName[0] && !indexBuf[0] )
      {
         needFetch = TRUE ;
         ossStrcpy ( command, "$list collections" ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{\"Name\":{$regex:\"^%s.\",$options:\"i\"}}",
                       pCsName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to call _runCommand" ) ;
            goto error ;
         }
      }
      else if ( pCsName[0] && pClName[0] && !indexBuf[0] )
      {
         needFetch = TRUE ;
         flag = 0 ;
         reqID = 0 ;
         rc = msgBuildQueryMsg ( ppReceiveBuffer, receiveBufferSize,
                                 csFullName,
                                 flag,
                                 reqID,
                                 numToskip,
                                 numToReturn,
                                 BsCondition,
                                 BsSelector,
                                 BsOrderby,
                                 BsHint ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to call msgBuildQueryMsg" ) ;
            goto error ;
         }
      }
      else if ( pCsName[0] && pClName[0] && indexBuf[0] )
      {
         CHAR pName[REST_ADAPTOR_MSG_COMMAND] = {0} ;
         needFetch = TRUE ;
         ossStrcpy ( command, "$get indexes" ) ;
         ossSnprintf ( pName,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{Name:\"%s\"}",
                       indexBuf ) ;
         ossSnprintf ( Strings,
                       REST_ADAPTOR_MSG_COMMAND,
                       "{collection:\"%s\"}",
                       csFullName ) ;
         rc = _runCommand ( ppReceiveBuffer,
                            receiveBufferSize,
                            command,
                            -1,
                            pName,
                            NULL,
                            NULL,
                            Strings ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to call _runCommand" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_REST_EHS ;
         PD_LOG ( PDERROR, "error http struct" ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RESTADP__DFCMD, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RESTADP__STRRLC, "_restAdaptor::_strReplace" )
   INT32 _restAdaptoro::_strReplace ( CHAR *pStr )
   {
      PD_TRACE_ENTRY ( SDB__RESTADP__STRRLC );
      INT32 ret = -1 ;
      INT32 i = 0 ;
      CHAR *pStrW = pStr ;
      INT32 strLen = ossStrlen ( pStr ) ;
      INT32 cutDownnum = 0 ;
      INT32 hex1 = 0 ;
      INT32 hex2 = 0 ;
      while ( i < strLen )
      {
         if ( '%' == pStr[i] )
         {
            ++i ;
            if ( !( ( pStr[i] > 47 && pStr[i] < 58 ) ||
                    ( pStr[i] > 64 && pStr[i] < 91 ) ||
                    ( pStr[i] > 96 && pStr[i] < 123 ) ) )
            {
               goto done ;
            }
            if ( pStr[i] > 47 && pStr[i] < 58 )
            {
               hex1 = pStr[i] - 48 ;
            }
            else if ( pStr[i] > 64 && pStr[i] < 91 )
            {
               hex1 = pStr[i] - 55 ;
            }
            else
            {
               hex1 = pStr[i] - 87 ;
            }
            ++i ;
            if ( !( ( pStr[i] > 47 && pStr[i] < 58 ) ||
                    ( pStr[i] > 64 && pStr[i] < 91 ) ||
                    ( pStr[i] > 96 && pStr[i] < 123 ) ) )
            {
               goto done ;
            }
            if ( pStr[i] > 47 && pStr[i] < 58 )
            {
               hex2 = pStr[i] - 48 ;
            }
            else if ( pStr[i] > 64 && pStr[i] < 91 )
            {
               hex2 = pStr[i] - 55 ;
            }
            else
            {
               hex2 = pStr[i] - 87 ;
            }
            pStrW[0] = hex1 * 16 + hex2 ;
            ++pStrW ;
            ++i ;
         }
         else if ( '+' == pStr[i] )
         {
            pStrW[0] = ' ' ;
            ++pStrW ;
            ++i ;
         }
         else
         {
            if ( pStrW != pStr + i )
            {
               pStrW[0] = pStr[i] ;
            }
            ++pStrW ;
            ++i ;
        }// % +
      }//while
      if ( pStrW != pStr + i )
      {
         for ( ; pStrW < pStr + i; ++pStrW )
         {
            pStrW[0] = 0 ;
         }
      }
      ret = strLen - cutDownnum ;
   done :
      PD_TRACE1 ( SDB__RESTADP__STRRLC, PD_PACK_INT(ret) );
      PD_TRACE_EXIT ( SDB__RESTADP__STRRLC );
      return ret ;
   }
}
