/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilHttp.cpp

   Descriptive Name = Rest client.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilHttp.hpp"
#include "http_parser.hpp"
#include <sstream>
#include <algorithm>

/* http char */
#define REST_STRING_CR            13
#define REST_STRING_LF            10
#define REST_STRING_HTTP          "HTTP/1.1 "
#define REST_STRING_COLON         ":"

/* http header key */
#define REST_STRING_CONLEN        "Content-Length"
#define REST_STRING_CONNECTION    "Connection"
#define REST_STRING_CACHE_CONTROL "Cache-Control"
#define REST_STRING_PRAGMA        "Pragma"
#define REST_STRING_CONTENT_TYPE  "Content-Type"
#define REST_STRING_TRANSFER      "Transfer-Encoding"

/* http header value */
#define REST_STRING_CLOSE         "close"
#define REST_STRING_KEEP_ALIVE    "keep-alive"
#define REST_STRING_NO_STORE      "no-store, no-cache, must-revalidate, post-check=0, pre-check=0"
#define REST_STRING_NO_CACHE      "no-cache"
#define REST_STRING_TEXT_HTML     "text/html"
#define REST_STRING_TEXT_JS       "text/javascript"
#define REST_STRING_TEXT_CSS      "text/css"
#define REST_STRING_TEXT_PNG      "image/png"
#define REST_STRING_TEXT_BMP      "image/bmp"
#define REST_STRING_TEXT_JPG      "image/jpeg"
#define REST_STRING_TEXT_GIF      "image/gif"
#define REST_STRING_CONLEN_SIZE   "0"
#define REST_STRING_CHUNKED       "chunked"

/* http defalut file */
#define REST_STRING_INDEX         "/index.html"

/* http query */
#define REST_STRING_COMMON        "cmd"
#define REST_STRING_QUERY         "query"
#define REST_STRING_CS            "cs"
#define REST_STRING_CL            "cl"
#define REST_STRING_RECORD        "record"

#define REST_STRING_EMPTY         ""

/* http default body */
#define REST_RESULT_STRING_OK     "{ \"errno\": 0, \"description\": \"OK\" }"

#define REST_FUN_STRING( str ) str,ossStrlen( str )

/* string size */
#define REST_STRING_TRANSFER_SIZE (sizeof( REST_STRING_TRANSFER ) - 1)
#define REST_STRING_CHUNKED_SIZE (sizeof( REST_STRING_CHUNKED ) - 1)

/* chunk modal end */
#define REST_STRING_CHUNKED_END "0\r\n\r\n"
#define REST_STRING_CHUNKED_END_SIZE (sizeof( REST_STRING_CHUNKED_END ) - 1)

#define REST_OPRATION_TIMEOUT    10000

namespace seadapter
{
   _utilHttp::_utilHttp()
   : _init( FALSE ),
     _keepAlive( TRUE ),
     _port( 0 ),
     _socket( NULL ),
     _sendBuf( NULL ),
     _sendBufSize( 0 ),
     _recvBuf( NULL ),
     _recvBufSize( 0 ),
     _parserSetting( NULL )
   {
   }

   _utilHttp::~_utilHttp()
   {
      reset() ;
   }

   INT32 _utilHttp::init( const string &uri, BOOLEAN keepAlive )
   {
      INT32 rc = SDB_OK ;
      http_parser_settings *parserSettings = NULL ;

      if ( _init )
      {
         reset() ;
      }

      rc = _parseUri( uri ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse uri[ %s ] failed", uri.c_str() ) ;

      rc = _connectBySocket() ;
      PD_RC_CHECK( rc, PDERROR, "Prepare socket failed[ %d ]", rc ) ;

      _sendBuf = ( CHAR *)SDB_OSS_MALLOC( HTTP_DEF_BUF_SIZE ) ;
      if ( !_sendBuf )
      {
         PD_LOG( PDERROR, "Allocate send buffer memory of size[ %d ] failed",
                 HTTP_DEF_BUF_SIZE ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _sendBufSize = HTTP_DEF_BUF_SIZE ;

      _recvBuf = ( CHAR * )SDB_OSS_MALLOC( HTTP_DEF_BUF_SIZE ) ;
      if ( !_recvBuf )
      {
         PD_LOG( PDERROR, "Allocate receive buffer memory of size[ %d ] failed",
                 HTTP_DEF_BUF_SIZE ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _recvBufSize = HTTP_DEF_BUF_SIZE ;

      parserSettings =
         (http_parser_settings *)SDB_OSS_MALLOC(sizeof(http_parser_settings)) ;
      if ( !parserSettings )
      {
         PD_LOG( PDERROR, "Allocate parser settings memory of size[ %d ] "
                 "failed", sizeof( http_parser_settings) ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemset( parserSettings, 0, sizeof( http_parser_settings ) ) ;
      parserSettings->on_message_begin = _onMessageBegin ;
      parserSettings->on_url = _onUrl ;
      parserSettings->on_status = _onStatus ;
      parserSettings->on_header_field = _onHeaderField ;
      parserSettings->on_header_value = _onHeaderValue ;
      parserSettings->on_headers_complete = _onHeaderComplete ;
      parserSettings->on_body = _onBody ;
      parserSettings->on_message_complete = _onMessageComplete ;

      _parserSetting = parserSettings ;
      _keepAlive = keepAlive ;
      _init = TRUE ;

   done:
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   void _utilHttp::reset()
   {
      if ( !_init )
      {
         goto done ;
      }

      _init = FALSE ;
      _keepAlive = TRUE ;
      _url.clear() ;
      _urn.clear() ;
      _port = 0 ;

      if ( _socket )
      {
         _socket->close() ;
         SDB_OSS_DEL _socket ;
         _socket = NULL ;
      }

      if ( _sendBuf )
      {
         SDB_OSS_FREE( _sendBuf ) ;
         _sendBuf = NULL ;
         _sendBufSize = 0 ;
      }
      if ( _recvBuf )
      {
         SDB_OSS_FREE( _recvBuf ) ;
         _recvBuf = NULL ;
         _recvBufSize = 0 ;
      }
      if ( _parserSetting )
      {
         SDB_OSS_FREE( _parserSetting ) ;
         _parserSetting = NULL ;
      }

   done:
      return ;
   }

   INT32 _utilHttp::request( const CHAR *method, const CHAR *endUrl,
                             const CHAR *data,
                             HTTP_STATUS_CODE *statusCode,
                             const CHAR **ppReply,
                             INT32 *replyLen, const CHAR *contentType )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN onlyHead = 0 == ossStrcmp( method, HTTP_REQ_HEAD_STR ) ;

      if ( !_init )
      {
         PD_LOG( PDERROR, "Connection not initialized yet" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !method )
      {
         PD_LOG( PDERROR, "Method for request is empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_validMethod( method ) )
      {
         PD_LOG( PDERROR, "Invalid http method[ %s ]", method ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !isConnected() )
      {
         rc = _connect() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Reconnect to remote server failed[ %d ]", rc ) ;
            goto error ;
         }
      }

      rc = _sendMessage( method, endUrl, data, contentType ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to remote server failed[ %d ]",
                   rc ) ;

      rc = _getReply( statusCode, ppReply, replyLen, onlyHead ) ;
      PD_RC_CHECK( rc, PDERROR, "Request processed failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilHttp::_parseUri( const string &uri )
   {
      INT32 rc = SDB_OK ;
      size_t pos = 0 ;
      string uriStr = uri ;

      if ( uri.empty() )
      {
         PD_LOG( PDERROR, "Uri for creating http connection is empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pos = uri.find( HTTP_URI_PREFIX ) ;
      if( pos != string::npos )
      {
         uriStr = uri.substr( pos + HTTP_URI_PREFIX_LEN ) ;
      }

      pos = uriStr.find( "/" ) ;
      if( pos != string::npos )
      {
         _url = uriStr.substr( 0, pos ) ;
         _urn = uriStr.substr( pos ) ;
      }
      else
      {
         _url = uriStr ;
         _urn = "/" ;
      }

      pos = _url.find( ":" ) ;
      if ( pos != string::npos )
      {
         ossStrToInt( _url.substr( pos + 1 ).c_str(), &_port ) ;
         _url = _url.substr( 0, pos ) ;
      }
      else
      {
         PD_LOG( PDERROR, "The uri[ %s ] format is wrong", uri.c_str() ) ;
         rc = SDB_INVALIDARG ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilHttp::_connectBySocket()
   {
      INT32 rc = SDB_OK ;

      if ( _socket )
      {
         SDB_OSS_DEL _socket ;
         _socket = NULL ;
      }

      _socket = SDB_OSS_NEW ossSocket( _url.c_str(), _port ) ;
      if ( !_socket )
      {
         PD_LOG( PDERROR, "Allocate memory for socket of size[ %d ] failed",
                 sizeof( ossSocket ) ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _connect( TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Socket connect failed[ %d ]", rc ) ;
      PD_LOG( PDEVENT,
              "Connection with remote server establised successfully" ) ;

   done:
      return rc ;
   error:
      if ( _socket )
      {
         SDB_OSS_DEL _socket ;
         _socket = NULL ;
      }
      goto done ;
   }

   INT32 _utilHttp::_connect( BOOLEAN newSock )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _socket, "socket should not be NULL" ) ;
      if ( !newSock )
      {
         _socket->close() ;
      }

      rc = _socket->initSocket() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Initialize socket failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = _socket->connect() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Connect to remote failed, url: %s, port: %u, "
                 "rc: %d", _url.c_str(), _port, rc ) ;
         goto error ;
      }

      rc = _socket->disableNagle() ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Disable nable failed[ %d ]", rc ) ;
         rc = SDB_OK ;
      }

      rc = _socket->setKeepAlive( _keepAlive, OSS_SOCKET_KEEP_IDLE,
                                  OSS_SOCKET_KEEP_INTERVAL,
                                  OSS_SOCKET_KEEP_CONTER ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Set socket keep alive failed[ %d ]", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilHttp::_disconnect()
   {
      if ( _socket )
      {
         _socket->close() ;
      }
   }

   BOOLEAN _utilHttp::_validMethod( const CHAR *method )
   {
      BOOLEAN valid = TRUE ;
      if ( !method )
      {
         valid = FALSE ;
         goto done ;
      }

      if ( ossStrcmp( method, HTTP_REQ_HEAD_STR ) &&
           ossStrcmp( method, HTTP_REQ_GET_STR ) &&
           ossStrcmp( method, HTTP_REQ_PUT_STR ) &&
           ossStrcmp( method, HTTP_REQ_POST_STR ) &&
           ossStrcmp( method, HTTP_REQ_DELETE_STR ) )
      {
         valid = FALSE ;
      }

   done:
      return valid ;
   }

   void _utilHttp::_buildReqStr( const CHAR *method, const CHAR *endUrl,
                                 const CHAR *data, const CHAR *content_type,
                                 string &requestStr, BOOLEAN &chunked )
   {
      INT32 dataSize = 0 ;

      SDB_ASSERT( method, "method should not be NULL" ) ;

      if ( !requestStr.empty() )
      {
         requestStr.clear() ;
      }

      requestStr += string( method ) ;

      SDB_ASSERT( HTTP_REQ_POST_STR == requestStr ||
                  HTTP_REQ_DELETE_STR == requestStr ||
                  HTTP_REQ_GET_STR == requestStr ||
                  HTTP_REQ_PUT_STR == requestStr ||
                  HTTP_REQ_HEAD_STR == requestStr,
                  "Invalid method" ) ;

      requestStr += string(" ") ;
      requestStr += _urn ;

      if ( NULL != endUrl )
      {
         if ( !_urn.empty() && '/' != *_urn.rbegin() )
         {
            requestStr += string("/") ;
         }
         requestStr += string( endUrl );
      }

      requestStr += string( " HTTP/1.1\r\n" ) ;

      requestStr += string( "Host: " ) ;
      requestStr += _url ;
      requestStr += string( "\r\n" ) ;
      requestStr += string( "Accept: application/json\r\n" ) ;
      if( _keepAlive )
      {
         requestStr += string( "Connection: Keep-Alive\r\n" ) ;
      }

      if ( !data || ( 0 == ( dataSize = ossStrlen( data ) ) ) )
      {
         requestStr += string( "\r\n" ) ;
         goto done ;
      }

      requestStr += string( "Content-Type: " ) ;
      requestStr += string( content_type ) ;
      requestStr += string( "\r\n" ) ;

      if ( dataSize < HTTP_CHUNK_SIZE )
      {
         std::stringstream lengthStr ;
         lengthStr << dataSize ;
         requestStr += string( "Content-length: " ) ;
         requestStr += lengthStr.str() ;
         requestStr += string( "\r\n\r\n" ) ;
         requestStr += string( data ) ;
         chunked = FALSE ;
      }
      else
      {
         requestStr += string( "Transfer-Encoding: chunked\r\n\r\n" ) ;
         chunked = TRUE ;
      }

   done:
      return ;
   }

   INT32 _utilHttp::_sendMessage( const CHAR *method, const CHAR *endUrl,
                                  const CHAR *data, const CHAR *content_type )
   {

      INT32 rc = SDB_OK ;
      string requestStr ;
      BOOLEAN chunked = FALSE ;

      _buildReqStr( method, endUrl, data, content_type, requestStr, chunked ) ;
      PD_LOG( PDDEBUG, "Request string is: %s", requestStr.c_str() ) ;

      if ( !chunked )
      {
         rc = _send( requestStr.c_str(), requestStr.size() ) ;
         PD_RC_CHECK( rc, PDERROR, "Send request string failed[ %d ]" ) ;
      }
      else
      {
         rc = _sendInChunkMode( requestStr, data ) ;
         PD_RC_CHECK( rc, PDERROR, "Send request in chunk mode failed[ %d ]",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilHttp::_send( const CHAR *request, INT32 reqSize )
   {
      INT32 rc = SDB_OK ;
      INT32 offset = 0 ;

      if ( !isConnected() )
      {
         PD_LOG( PDWARNING, "Connection interrupted, try to connect again." ) ;
         rc = _connect() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to connect to remote, rc: %d", rc ) ;
            goto error ;
         }
      }

      while ( reqSize > offset )
      {
         INT32 sendSize = 0 ;
         rc = _socket->send( &request[offset], reqSize - offset, sendSize,
                             HTTP_REQ_TIMEOUT ) ;
         offset += sendSize ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send request failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilHttp::_sendInChunkMode( string &header, const CHAR *data )
   {
      INT32 rc = SDB_OK ;
      UINT32 totalSent = 0 ;
      string chunkEnd = "0\r\n\r\n" ;
      UINT32 dataSize = ossStrlen( data ) ;

      rc = _send( header.c_str(), header.size() ) ;
      PD_RC_CHECK( rc, PDERROR, "Send header failed[ %d ]", rc ) ;

      while ( totalSent < dataSize )
      {
         UINT32 chunkSize =  ( dataSize - totalSent ) < HTTP_CHUNK_SIZE ?
                             ( dataSize - totalSent ) : HTTP_CHUNK_SIZE ;
         std::stringstream chunkSizeString ;
         chunkSizeString << std::hex << chunkSize ;

         string chunk( chunkSizeString.str() ) ;
         chunk += "\r\n" ;
         chunk += string( data + totalSent, chunkSize ) ;
         chunk += "\r\n" ;

         PD_LOG( PDDEBUG, "chunk string: %s", chunk.c_str() ) ;
         rc = _send( chunk.c_str(), chunk.size() ) ;
         PD_RC_CHECK( rc, PDERROR, "Send chund data failed[ %d ]", rc ) ;
         totalSent += chunkSize;
      }

      rc = _send( chunkEnd.c_str(), chunkEnd.size() ) ;
      PD_RC_CHECK( rc, PDERROR, "Send final chunk message failed[ %d ]" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _utilHttp::_checkEndOfHeader( const CHAR *buff, UINT32 bufSize,
                                         INT32 &bodyOffset )
   {
      const CHAR *position = NULL ;
      position = ossStrstr( buff, HTTP_BLOCK_END_STR ) ;
      if ( !position )
      {
         return FALSE ;
      }
      else
      {
         bodyOffset = position - buff + HTTP_BLOCK_END_STR_LEN ;
         return TRUE ;
      }
   }

   INT32 _utilHttp::_chkStatusCode( HTTP_STATUS_CODE statusCode )
   {
      INT32 rc = SDB_OK ;
      switch( statusCode )
      {
         case HTTP_OK:
         case HTTP_CREATED:
         case HTTP_FOUND:
            break ;
         case HTTP_BAD_REQ:
            PD_LOG( PDERROR,
                    "Status: 400 Bad Request. Please reconsider the request" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         case HTTP_FORBID:
            PD_LOG( PDERROR,
                    "Status: 403 Forbidden. Please reconsider the request" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         case HTTP_NOT_FOUND:
            PD_LOG( PDERROR,
                    "Status: 404 Not Found. Please reconsider the request" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         case HTTP_SVC_UNAVAL:
            PD_LOG( PDERROR, "Status: 503 Service Unavailable. Please check "
                    "the server status" ) ;
            rc = SDB_SYS ;
            goto error ;
         default:
            PD_LOG( PDERROR, "Weired status code: %u", statusCode ) ;
            rc = SDB_SYS ;
            goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   INT32 _utilHttp::_getReply( HTTP_STATUS_CODE *statusCode, const CHAR **reply,
                               INT32 *replyLen, BOOLEAN onlyHead )
   {
      INT32 rc = SDB_OK ;
      INT32 headerSize = 0 ;
      INT32 remainSize = 0 ;
      INT32 curRecvSize = 0 ;
      BOOLEAN headFound = FALSE ;
      INT32 bodyOffset = 0 ;
      INT32 contentLen = 0 ;
      INT32 bodyPartLen = 0 ;
      INT32 bodyRemainLen = 0 ;
      INT32 bodyTotalLen = 0 ;
      INT32 totalRecv = 0 ;
      const CHAR *itemPtr = NULL ;
      http_parser *parser = _getHttpParser() ;

      remainSize = HTTP_MAX_HEADER_SIZE ;
      while ( headerSize < HTTP_MAX_HEADER_SIZE )
      {
         rc = _socket->recv( _recvBuf + headerSize, remainSize,
                             curRecvSize, REST_OPRATION_TIMEOUT, 0, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to receive data, rc: %d", rc ) ;
         _recvBuf[ headerSize + curRecvSize + 1 ] = '\0' ;
         headFound = _checkEndOfHeader( _recvBuf + headerSize,
                                        curRecvSize, bodyOffset ) ;
         if ( headFound )
         {
            SDB_ASSERT( bodyOffset >=0, "impossible" ) ;
            bodyPartLen = curRecvSize - bodyOffset ;
            headerSize += bodyOffset ;
            break ;
         }
         else
         {
            headerSize += curRecvSize ;
         }
      }

      if ( !headFound )
      {
         rc = SDB_REST_RECV_SIZE ;
         PD_LOG( PDERROR, "http header size %d greater than max: %d",
                 headerSize, HTTP_MAX_HEADER_SIZE ) ;
         goto error ;
      }

      rc = _parseHeader( _recvBuf, headerSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse http header failed[ %d ], header: %s",
                   rc, _recvBuf ) ;

      if ( statusCode )
      {
         *statusCode = parser->status_code ;
      }

      rc = _chkStatusCode( (HTTP_STATUS_CODE)parser->status_code ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "The status code is %u. Error has happened[ %d ]",
                 (HTTP_STATUS_CODE)parser->status_code ) ;
      }

      if ( onlyHead )
      {
         *reply = NULL ;
         *replyLen = 0 ;
         goto done ;
      }

      itemPtr = _getHeaderItemVal( REST_STRING_TRANSFER ) ;
      if ( itemPtr && ( 0 == ossStrcmp( itemPtr, REST_STRING_CHUNKED ) ) )
      {
         SDB_ASSERT( FALSE, "TODO" ) ;
      }
      else
      {
         itemPtr = _getHeaderItemVal( REST_STRING_CONLEN ) ;
         if ( !itemPtr )
         {
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            contentLen = ossAtoi( itemPtr ) ;
            bodyTotalLen = contentLen ;
            bodyRemainLen = contentLen - bodyPartLen ;
         }

         totalRecv = headerSize + bodyPartLen ;
         if ( (UINT32)( headerSize + bodyTotalLen ) > _recvBufSize )
         {
            _recvBuf = (CHAR *)SDB_OSS_REALLOC( _recvBuf,
                                                headerSize + bodyTotalLen ) ;
            if ( !_recvBuf )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Reallocate memory failed, required size: %u",
                       headerSize + bodyTotalLen ) ;
               goto error ;
            }
            _recvBufSize = headerSize + bodyTotalLen ;
         }
         while ( bodyRemainLen > 0 )
         {
            rc = _socket->recv( _recvBuf + totalRecv, bodyRemainLen,
                                curRecvSize, REST_OPRATION_TIMEOUT,
                                0, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to receive data, rc: %d", rc ) ;
            bodyRemainLen -= curRecvSize ;
            totalRecv += curRecvSize ;

            if ( 0 == curRecvSize && bodyRemainLen > 0 )
            {
               rc = SDB_REST_RECV_SIZE ;
               PD_LOG( PDERROR, "http body size is not the same with expected, "
                       "rc: %d", rc ) ;
               goto error ;
            }
         }
      }

      if ( reply )
      {
         *reply = _recvBuf + headerSize ;
      }
      if ( replyLen )
      {
         *replyLen = bodyTotalLen ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilHttp::_paraInit( httpConnection *pHttpCon )
   {
      pHttpCon->_tempKeyLen      = 0 ;
      pHttpCon->_tempValueLen    = 0 ;
      pHttpCon->_CRLFNum         = 0 ;
      pHttpCon->_headerSize      = 0 ;
      pHttpCon->_partSize        = 0 ;
      pHttpCon->_firstRecordSize = ( sizeof(REST_RESULT_STRING_OK) - 1 ) ;
      pHttpCon->_responseSize    = 0 ;
      pHttpCon->_isChunk         = FALSE ;
      pHttpCon->_isSendHttpHeader = FALSE ;
      pHttpCon->_isKey           = TRUE ;
      pHttpCon->_pHeaderBuf      = NULL ;
      pHttpCon->_pPartBody       = NULL ;
      pHttpCon->_pBodyBuf        = NULL ;
      pHttpCon->_pSendBuffer     = NULL ;
      pHttpCon->_pTempKey        = NULL ;
      pHttpCon->_pTempValue      = NULL ;
      pHttpCon->_pPath           = NULL ;
      httpResponse httpRe ;

      pHttpCon->_requestHeaders.clear() ;
      pHttpCon->_requestQuery.clear() ;
      pHttpCon->_responseHeaders.clear() ;
      pHttpCon->_responseHeaders.insert(
            std::make_pair( REST_STRING_CONNECTION, REST_STRING_CLOSE ) );
      pHttpCon->_responseHeaders.insert(
            std::make_pair( REST_STRING_CACHE_CONTROL, REST_STRING_NO_STORE ) );
      pHttpCon->_responseHeaders.insert(
            std::make_pair( REST_STRING_PRAGMA, REST_STRING_NO_CACHE ) ) ;
      pHttpCon->_responseHeaders.insert(
            std::make_pair( REST_STRING_CONTENT_TYPE, REST_STRING_TEXT_HTML ) );
      pHttpCon->_responseBody.clear() ;
      httpRe.pBuffer = REST_RESULT_STRING_OK ;
      httpRe.len = sizeof( REST_RESULT_STRING_OK ) - 1 ;
      pHttpCon->_responseBody.push_back( httpRe ) ;
   }

   INT32 _utilHttp::_parseHeader( CHAR *buff, INT32 len )
   {
      INT32 rc = SDB_OK ;
      http_parser *parser = &(_connection._httpParser) ;

      SDB_ASSERT( buff, "Data buffer should not be NULL" ) ;

      _paraInit( &_connection ) ;

      http_parser_init( parser, HTTP_RESPONSE ) ;
      if ( len != (INT32)http_parser_execute( parser,
                                              (http_parser_settings *)_parserSetting,
                                              buff, len ) )
      {
         if ( HTTP_PARSER_ERRNO( parser ) != HPE_PAUSED )
         {
            PD_LOG( PDERROR, "Parse http header failed[ %s ]",
                    http_errno_description( HTTP_PARSER_ERRNO( parser ) ) ) ;
            rc = SDB_REST_EHS ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   size_t _utilHttp::appendChunk(string& output, CHAR* msg, size_t msgSize)
   {
      return 0 ;
   }

   INT32 _utilHttp::_onMessageBegin( void *data )
   {
      return SDB_OK ;
   }

   INT32 _utilHttp::_onUrl( void *data, const CHAR* at, size_t length )
   {
      return SDB_OK ;
   }

   INT32 _utilHttp::_onStatus( void *data, const CHAR* at, size_t length )
   {
      return SDB_OK ;
   }

   INT32 _utilHttp::_onHeaderField( void *data, const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)data ;
      httpConnection *pHttpCon = (httpConnection *)pParser->data ;

      if ( pHttpCon->_isKey )
      {
         if ( pHttpCon->_pTempKey && pHttpCon->_pTempValue )
         {
            pHttpCon->_pTempKey[pHttpCon->_tempKeyLen] = 0 ;
            pHttpCon->_pTempValue[pHttpCon->_tempValueLen] = 0 ;
            pHttpCon->_requestHeaders.insert(
                  std::make_pair( pHttpCon->_pTempKey,pHttpCon->_pTempValue ) );
            pHttpCon->_pTempKey = NULL ;
            pHttpCon->_pTempValue = NULL ;
            pHttpCon->_tempKeyLen = 0 ;
            pHttpCon->_tempValueLen = 0 ;
         }

         pHttpCon->_tempKeyLen = length ;
         pHttpCon->_pTempKey = pHttpCon->_pHeaderBuf +
               ( at - pHttpCon->_pHeaderBuf ) ;
         pHttpCon->_isKey = FALSE ;
      }
      else
      {
         if(  pHttpCon->_pTempKey == NULL )
         {
            return 1 ;
         }
         pHttpCon->_tempKeyLen += length ;
      }
      return 0 ;
   }

   INT32 _utilHttp::_onHeaderValue( void *data, const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)data ;
      httpConnection *pHttpCon = (httpConnection *)pParser->data ;

      if ( !pHttpCon->_isKey )
      {
         if ( pHttpCon->_pTempKey == NULL )
         {
            return 1 ;
         }
         pHttpCon->_pTempKey[ pHttpCon->_tempKeyLen + 1 ] = 0 ;

         pHttpCon->_tempValueLen = length ;
         pHttpCon->_pTempValue = pHttpCon->_pHeaderBuf +
               ( at - pHttpCon->_pHeaderBuf ) ;
         pHttpCon->_isKey = TRUE ;
      }
      else
      {
         if(  pHttpCon->_pTempValue == NULL )
         {
            return 1 ;
         }
         pHttpCon->_tempValueLen += length ;
      }
      return 0 ;
   }

   INT32 _utilHttp::_onHeaderComplete( void *data )
   {
      http_parser *pParser = (http_parser *)data ;
      httpConnection *pHttpCon = (httpConnection *)pParser->data ;

      if ( pHttpCon->_pTempKey && pHttpCon->_pTempValue )
      {
         pHttpCon->_pTempKey[pHttpCon->_tempKeyLen] = 0 ;
         pHttpCon->_pTempValue[pHttpCon->_tempValueLen] = 0 ;
         pHttpCon->_requestHeaders.insert(
               std::make_pair( pHttpCon->_pTempKey, pHttpCon->_pTempValue ) ) ;
         pHttpCon->_pTempKey = NULL ;
         pHttpCon->_pTempValue = NULL ;
         pHttpCon->_tempKeyLen = 0 ;
         pHttpCon->_tempValueLen = 0 ;
      }
      http_parser_pause( pParser, 1 ) ;
      return 0 ;
   }

   INT32 _utilHttp::_onBody( void *data, const CHAR* at, size_t length )
   {
      return SDB_OK ;
   }

   INT32 _utilHttp::_onMessageComplete( void *data )
   {
      return SDB_OK ;
   }

   INT32 _utilHttp::_extendRecvBuff()
   {
      return SDB_OK ;
   }

   const CHAR* _utilHttp::_getHeaderItemVal( const CHAR *key )
   {
      COLNAME_MAP_IT it ;

      it = _connection._requestHeaders.find( key ) ;
      return ( it == _connection._requestHeaders.end() ) ? NULL : it->second ;
   }
}

