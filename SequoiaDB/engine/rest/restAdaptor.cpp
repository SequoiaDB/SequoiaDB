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

   Source File Name = restAdaptor.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/28/2014  JWH Initial Draft
          05/03/2015  JWH Refactor

   Last Changed =

*******************************************************************************/

#include "restAdaptor.hpp"
#include "ossMem.h"
#include "ossUtil.h"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "restTrace.hpp"
#include "dms.hpp"
#include "rtnContext.hpp"
#include "../util/url.h"

/* response body cache max size */
#define RESPONSE_MAX_CACHE_SIZE   (64*1024*1024)

/* once recv size */
#define REST_ONCE_RECV_SIZE       1024

/* http char */
#define REST_STRING_CR            13
#define REST_STRING_LF            10
#define REST_STRING_HTTP          "HTTP/1.1"
#define REST_STRING_COLON         ":"

/* http header key */
#define REST_STRING_CONLEN        "Content-Length"
#define REST_STRING_CONNECTION    "Connection"
#define REST_STRING_CACHE_CONTROL "Cache-Control"
#define REST_STRING_PRAGMA        "Pragma"
#define REST_STRING_CONTENT_TYPE  "Content-Type"
#define REST_STRING_TRANSFER      "Transfer-Encoding"
#define REST_STRING_ACCEPT        "Accept"

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
#define REST_STRING_TEXT_SVG      "image/svg+xml"
#define REST_STRING_IMAGE_ICON    "image/x-icon"

#define REST_STRING_APP_JSON      "application/json"
#define REST_STRING_APP_WOFF      "application/font-woff"
#define REST_STRING_APP_EOT       "application/vnd.ms-fontobject"
#define REST_STRING_TEXT_OCTET_STREAM  "application/octet-stream"

#define REST_STRING_CONLEN_SIZE   "0"

#define REST_STRING_CHUNKED       "chunked"
#define REST_STRING_IDENTITY      "identity"

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

/* chunk modal end */
#define REST_STRING_CHUNKED_END "0\r\n\r\n"
#define REST_STRING_CHUNKED_END_SIZE (sizeof( REST_STRING_CHUNKED_END ) - 1)

static const CHAR *responseHeader[] = {
      "200 Ok",   "302 Found",   "400 Bad Request",   "404 Not Found",
      "503 Service Unavailable",   "505 Http Version Not Supported"
} ;

static const CHAR *fileExtension[] = {
      "html",   "json", "js",    "css",   "png",   "bmp",   "jpg",
      "gif",    "svg",  "ico",   "woff",  "eot",   "otf",   "ttf",
      "jsp",    "php",  "asp"
} ;

#define REST_STRING_FILE_EX_SIZE ( sizeof( fileExtension ) \
                                 / sizeof( fileExtension[0] ) )

namespace engine
{

   restAdaptor::restAdaptor() : _maxHttpHeaderSize( 0 ),
                                _maxHttpBodySize( 0 ),
                                _timeout( 0 )
   {
   }

   restAdaptor::~restAdaptor()
   {
   }

   void restAdaptor::init( INT32 maxHttpHeaderSize, INT32 maxHttpBodySize,
                           INT32 timeout )
   {
      _maxHttpHeaderSize = maxHttpHeaderSize ;
      _maxHttpBodySize = maxHttpBodySize ;
      _timeout = timeout ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_RECVHEADER, "restAdaptor::recvHeader" )
   INT32 restAdaptor::recvHeader( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_RECVHEADER ) ;
      SDB_ASSERT( sock, "sock is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      INT32 recvSize = 0 ;
      CHAR *buffer   = pRest->_headerBuf ;
      http_parser *pParser = &(pRest->_httpParser) ;

      pRest->_write = FALSE ;

      pParser->data = pRest ;

      if ( NULL == buffer )
      {
         rc = pRest->_malloc( _maxHttpHeaderSize + 1, &pRest->_headerBuf ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     _maxHttpHeaderSize + 1, rc ) ;
            goto error ;
         }

         buffer = pRest->_headerBuf ;
         buffer[_maxHttpHeaderSize] = '\0' ;
      }

      while( TRUE )
      {
         INT32 tmpRecvSize = 0 ;
         INT32 offset = 0 ;

         rc = _recvData( sock, buffer + recvSize,
                         _maxHttpHeaderSize - recvSize,
                         FALSE, &tmpRecvSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to recv, rc=%d", rc ) ;
            goto error ;
         }

         buffer[ recvSize + tmpRecvSize ] = '\0' ;

         if ( _isEndOfHeader( pRest, buffer + recvSize, tmpRecvSize, offset ) )
         {
            if ( offset > 0 )
            {
               INT32 bodySize = tmpRecvSize - offset ;

               recvSize += offset ;

               pRest->_tmpBodySize = bodySize ;
               pRest->_tmpBodyBuf = buffer + recvSize ;
            }
            else
            {
               recvSize += tmpRecvSize ;
            }

            pRest->_headerSize = recvSize ;

            break ;
         }

         recvSize += tmpRecvSize ;
         if ( recvSize >= _maxHttpHeaderSize )
         {
            rc = SDB_REST_RECV_SIZE ;
            PD_LOG ( PDERROR, "http header size %d greater than %d",
                     recvSize, _maxHttpHeaderSize ) ;
            goto error ;
         }
      }

      http_parser_init( pParser, HTTP_BOTH ) ;

      if ( (UINT32)recvSize != http_parser_execute( pParser, &pRest->_settings,
                                                    buffer, (UINT32)recvSize ) )
      {
         if ( HTTP_PARSER_ERRNO( pParser ) != 28 )
         {
            rc = SDB_REST_EHS ;
            PD_LOG ( PDERROR, "Failed to parse http, %s, rc=%d",
                     http_errno_description( HTTP_PARSER_ERRNO( pParser ) ),
                     rc ) ;
            goto error ;
         }
      }

      if ( pRest->type() == SDB_REST_REQUEST )
      {
         restRequest *request = (restRequest *)pRest ;

         request->_method = pParser->method ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_RECVHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_RECVBODY, "restAdaptor::recvBody" )
   INT32 restAdaptor::recvBody( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_RECVBODY ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      string chunk ;

      chunk = pRest->getHeader( REST_STRING_TRANSFER ) ;
      if ( chunk.empty() )
      {
         rc = _recvRestBody( sock, pRest ) ;
      }
      else if( SDB_REST_RESPONSE == pRest->type() &&
               REST_STRING_CHUNKED == chunk )
      {
         rc = _recvRestChunk( sock, pRest ) ;
      }
      else if( SDB_REST_RESPONSE == pRest->type() &&
               REST_STRING_IDENTITY == chunk )
      {
         rc = _recvRestIdentity( sock, pRest ) ;
      }
      else
      {
         rc = SDB_REST_EHS ;
      }

      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to recv rest body: rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_REST_REQUEST == pRest->type() && pRest->_bodySize > 0 )
      {
         restRequest *request = (restRequest *)pRest ;

         rc = request->_parse_http_query( pRest->_bodyBuf, pRest->_bodySize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to parse request body: rc=%d", rc ) ;
            goto error ;
         }

         request->_parseDataType() ;
      }

  done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_RECVBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 restAdaptor::sendRest( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;

      rc = sendHeader( sock, pRest ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sendBody( sock, pRest ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_SENDHEADER, "restAdaptor::sendHeader" )
   INT32 restAdaptor::sendHeader( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_SENDHEADER ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      CHAR CRLF[3] = { REST_STRING_CR, REST_STRING_LF, 0 } ;
      string restHeader = pRest->_generateHeader() ;
      REST_COLNAME_MAP_IT it ;

      if ( pRest->getBodySize() > 0 &&
           pRest->isHeaderExist( REST_STRING_CONLEN ) == FALSE )
      {
         INT32 totalSize = pRest->getBodySize() ;
         BOOLEAN isJson = ( HTTP_FILE_JSON == pRest->getDataType() &&
                            SDB_REST_RESPONSE == pRest->type() ) ;
         CHAR bodySizeTmp[256] = { 0 } ;

         if ( isJson )
         {
            INT32 recordNum = pRest->_bodyContent.size() ;

            totalSize += recordNum + 1 ;
         }

         ossSnprintf( bodySizeTmp, 256, "%d", totalSize ) ;

         pRest->putHeader( REST_STRING_CONLEN, bodySizeTmp ) ;
      }

      rc = _sendData( sock, restHeader.c_str(), restHeader.length() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }

      //CRLF
      rc = _sendData( sock, REST_FUN_STRING( CRLF ) ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }

      for( it = pRest->_headerList.begin();
            it != pRest->_headerList.end(); ++it )
      {
         string key = it->first ;
         string val = it->second ;

         //key
         rc = _sendData( sock, key.c_str(), key.length() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }

         //:
         rc = _sendData( sock, REST_FUN_STRING( REST_STRING_COLON ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }

         //value
         rc = _sendData( sock, val.c_str(), val.length() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }

         //CRLF
         rc = _sendData( sock, REST_FUN_STRING( CRLF ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }

      //CRLF
      rc = _sendData( sock, REST_FUN_STRING( CRLF ) ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_SENDHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_SENDBODY, "restAdaptor::sendBody" )
   INT32 restAdaptor::sendBody( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_SENDBODY ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      std::vector<string>::iterator it ;

      if ( pRest->_bodyContent.size() > 0 )
      {
         BOOLEAN isJson = ( HTTP_FILE_JSON == pRest->getDataType() &&
                            SDB_REST_RESPONSE == pRest->type() ) ;

         if ( isJson )
         {
            rc = _sendData( sock, "[", 1 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
               goto error ;
            }
         }

         for( it = pRest->_bodyContent.begin();
              it != pRest->_bodyContent.end(); ++it )
         {
            string context = *it ;

            if ( isJson && it != pRest->_bodyContent.begin() )
            {
               rc = _sendData( sock, ",", 1 ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
                  goto error ;
               }
            }

            rc = _sendData( sock, context.c_str(), context.length() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
               goto error ;
            }
         }

         if ( isJson )
         {
            rc = _sendData( sock, "]", 1 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      else if ( pRest->_bodyBuf && pRest->_bodySize > 0 )
      {
         rc = _sendData( sock, pRest->_bodyBuf, pRest->_bodySize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_SENDBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_SENDCHUNK, "restAdaptor::sendChunk" )
   INT32 restAdaptor::sendChunk( ossSocket *sock, const CHAR *pBuffer,
                                 INT32 length, INT32 number,
                                 BOOLEAN isObjBuffer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_SENDCHUNK ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;

      if( isObjBuffer )
      {
         BSONObj record ;
         string str ;
         _rtnObjBuff rtnObj( pBuffer, length, number ) ;

         while( TRUE )
         {
            rc = rtnObj.nextObj( record ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else if ( SDB_SYS == rc )
            {
               PD_LOG ( PDERROR, "Failed to get nextObj:rc=%d", rc ) ;
               goto error ;
            }
            
            str = record.toString( FALSE, TRUE ) ;

            rc = _sendChunkData( sock, str.c_str(), str.length() ) ;
            if( rc )
            {
               PD_LOG ( PDERROR, "Failed to send chunk data, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         rc = _sendChunkData( sock, pBuffer, length ) ;
         if( rc )
         {
            PD_LOG ( PDERROR, "Failed to send chunk data, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_SENDCHUNK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR_SETRESBODY, "restAdaptor::setResBody" )
   INT32 restAdaptor::setResBody( ossSocket *sock, restResponse *response,
                                  const CHAR *pBuffer, INT32 length,
                                  INT32 number, BOOLEAN isObjBuffer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR_SETRESBODY ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( response, "response is NULL" ) ;

      if ( !response->isChunkModal() )
      {
         if ( response->getBodySize() + length > RESPONSE_MAX_CACHE_SIZE )
         {
            response->setChunkModal() ;

            //send http header
            rc = sendHeader( sock, response ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to send rest header: rc=%d", rc ) ;
               goto error ;
            }

            //send http body
            rc = _sendBodyWithChunk( sock, response ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to send body with chunk: rc=%d", rc ) ;
               goto error ;
            }

            rc = setResBody( sock, response, pBuffer, length,
                             number, isObjBuffer ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to send response: rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = response->appendBody( pBuffer, length, number, isObjBuffer ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to append response body: rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         rc = sendChunk( sock, pBuffer, length, number, isObjBuffer ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send chunk, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR_SETRESBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 restAdaptor::setResBodyEnd( ossSocket *sock, restResponse *response )
   {
      if ( !response->isChunkModal() )
      {
         return sendRest( sock, response ) ;
      }
      else
      {
         return sendChunk( sock, NULL, 0, 0, FALSE ) ;
      }
   }

   INT32 restAdaptor::_sendBodyWithChunk( ossSocket *sock,
                                          restResponse *response )
   {
      INT32 rc = SDB_OK ;
      std::vector<string>::iterator it ;

      for( it = response->_bodyContent.begin();
           it != response->_bodyContent.end(); ++it )
      {
         string context = *it ;

         rc = _sendChunkData( sock, context.c_str(), context.length() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN restAdaptor::_isEndOfHeader( restBase *pRest, CHAR *buffer,
                                        INT32 size, INT32 &offset )
   {
      BOOLEAN rc = TRUE ;

      for ( INT32 i = 0; i < size; ++i )
      {
         if ( REST_STRING_CR == buffer[i] )
         {
            switch( pRest->_CRLFNum )
            {
            case 2: // x  \r \n \r
               pRest->_CRLFNum = 3 ;
               break ;
            case 0: // x  x  x  \r
            case 1: // x  x  \r \r
            case 3: // \r \n \r \r
            default:
               pRest->_CRLFNum = 1 ;
               break ;
            }
         }
         else if ( REST_STRING_LF == buffer[i] )
         {
            switch( pRest->_CRLFNum )
            {
            case 1: // x  x  \r \n
               pRest->_CRLFNum = 2 ;
               break ;
            case 3: // \r \n \r \n
               pRest->_CRLFNum = 4 ;
               break ;
            case 2: // x  \r \n \n
            case 0: // x  x  x  \n
            default:
               pRest->_CRLFNum = 0 ;
               break ;
            }
         }
         else
         {
            pRest->_CRLFNum = 0 ;
         }

         if ( pRest->_CRLFNum == 4 )
         {
            pRest->_CRLFNum = 0 ;
            rc = TRUE ;
            if ( i + 1 == size )
            {
               offset = 0 ;
            }
            else
            {
               offset = i + 1 ;
            }
            goto done ;
         }
      }

      rc = FALSE ;

   done:
      return rc ;
   }

   BOOLEAN restAdaptor::_isEndOfChunk( restBase *pRest, CHAR *buffer,
                                       INT32 size )
   {
      BOOLEAN rc = TRUE ;

      for ( INT32 i = 0; i < size; ++i )
      {
         if ( '0' == buffer[i] )
         {
            pRest->_ZCRLF = 1 ;
         }
         else if ( REST_STRING_CR == buffer[i] && 1 == pRest->_ZCRLF )
         {
           pRest->_ZCRLF = 2 ;
         }
         else if ( REST_STRING_LF == buffer[i] && 2 == pRest->_ZCRLF )
         {
            pRest->_ZCRLF = 3 ;
         }
         else
         {
            pRest->_ZCRLF = 0 ;
         }

         if ( pRest->_ZCRLF == 3 )
         {
            pRest->_ZCRLF = 0 ;
            rc = TRUE ;
            goto done ;
         }
      }

      rc = FALSE ;

   done:
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__RECVRESTBODY, "restAdaptor::_recvRestBody" )
   INT32 restAdaptor::_recvRestBody( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__RECVRESTBODY ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      INT32 bodySize = 0 ;
      INT32 curRecvSize  = 0 ;
      INT32 receivedSize = 0 ;
      CHAR *pBuffer = NULL ;
      string contentLength ;

      contentLength = pRest->getHeader( REST_STRING_CONLEN ) ;

      //if http body size > 0,than Content-Length must exist
      if ( FALSE == contentLength.empty() )
      {
         bodySize = ossAtoi( contentLength.c_str() ) ;
         if ( bodySize > 0 )
         {
            BOOLEAN isFullSpace = FALSE ;

            if ( bodySize > _maxHttpBodySize )
            {
               isFullSpace = TRUE ;

               if ( SDB_REST_REQUEST == pRest->type() )
               {
                  rc = SDB_REST_RECV_SIZE ;
                  PD_LOG ( PDERROR, "http body size %d greater than %d",
                           bodySize, _maxHttpBodySize ) ;
                  goto error ;
               }
               else if ( SDB_REST_RESPONSE == pRest->type() )
               {
                  bodySize = _maxHttpBodySize ;
               }
            }

            if ( NULL == pRest->_bodyBuf )
            {
               rc = pRest->_malloc( bodySize + 1, &pRest->_bodyBuf ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                           bodySize, rc ) ;
                  goto error ;
               }
            }

            pBuffer = pRest->_bodyBuf ;
            pBuffer[bodySize] = 0 ;

            pRest->_bodySize = bodySize ;
            pRest->_bodyBufSize = bodySize ;

            if ( pRest->_tmpBodyBuf )
            {
               INT32 tmpBodySize = pRest->_tmpBodySize ;

               if ( tmpBodySize > bodySize )
               {
                  tmpBodySize = bodySize ;
               }

               ossMemcpy( pRest->_bodyBuf, pRest->_tmpBodyBuf, tmpBodySize ) ;

               receivedSize = tmpBodySize ;

               pRest->_tmpBodyBuf = NULL ;
               pRest->_tmpBodySize = 0 ;
            }

            if ( bodySize - receivedSize > 0 )
            {
               rc = _recvData( sock, pBuffer + receivedSize,
                               bodySize - receivedSize, TRUE, &curRecvSize ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to recv, rc=%d", rc ) ;
                  goto error ;
               }
            }

            if ( isFullSpace )
            {
               rc = SDB_REST_RECV_SIZE ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__RECVRESTBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__RECVRESTCHUNK, "restAdaptor::_recvRestChunk" )
   INT32 restAdaptor::_recvRestChunk( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__RECVRESTCHUNK ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      INT32 receivedSize = 0 ;
      INT32 curRecvSize = 0 ;
      INT32 bufSize = pRest->_bodyBufSize ;
      CHAR *pBuffer = pRest->_bodyBuf ;

      //allocate space
      if ( NULL == pRest->_bodyBuf )
      {
         bufSize = _maxHttpBodySize ;

         rc = pRest->_malloc( bufSize + 1, &pRest->_bodyBuf ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     bufSize, rc ) ;
            goto error ;
         }

         pRest->_bodyBufSize = bufSize ;
      }

      pBuffer = pRest->_bodyBuf ;
      pBuffer[bufSize] = 0 ;

      //replicate the multiple body received
      if ( pRest->_tmpBodyBuf )
      {
         ossMemcpy( pRest->_bodyBuf, pRest->_tmpBodyBuf,
                    pRest->_tmpBodySize ) ;

         receivedSize = pRest->_tmpBodySize ;

         pRest->_tmpBodyBuf = NULL ;
         pRest->_tmpBodySize = 0 ;
      }

      pRest->_bodySize += receivedSize ;

      while( TRUE )
      {
         rc = _recvData( sock, pBuffer + receivedSize,
                         bufSize - receivedSize, FALSE, &curRecvSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to recv, rc=%d", rc ) ;
            goto error ;
         }

         pRest->_bodySize += curRecvSize ;

         if( _isEndOfChunk( pRest, pBuffer + receivedSize, curRecvSize ) )
         {
            break ;
         }

         receivedSize += curRecvSize ;

         if ( bufSize == receivedSize )
         {
            rc = SDB_REST_RECV_SIZE ;
            break ;
         }

         SDB_ASSERT( receivedSize > bufSize, "memory-access errors" ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__RECVRESTCHUNK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__RECVRESTIDENTITY, "restAdaptor::_recvRestIdentity" )
   INT32 restAdaptor::_recvRestIdentity( ossSocket *sock, restBase *pRest )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__RECVRESTIDENTITY ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pRest, "pRest is NULL" ) ;
      INT32 receivedSize = 0 ;
      INT32 curRecvSize = 0 ;
      INT32 bufSize = pRest->_bodyBufSize ;
      CHAR *pBuffer = pRest->_bodyBuf ;

      //allocate space
      if ( NULL == pRest->_bodyBuf )
      {
         bufSize = _maxHttpBodySize ;

         rc = pRest->_malloc( bufSize + 1, &pRest->_bodyBuf ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     bufSize, rc ) ;
            goto error ;
         }

         pRest->_bodyBufSize = bufSize ;
      }

      pBuffer = pRest->_bodyBuf ;
      pBuffer[bufSize] = 0 ;

      //replicate the multiple body received
      if ( pRest->_tmpBodyBuf )
      {
         ossMemcpy( pRest->_bodyBuf, pRest->_tmpBodyBuf,
                    pRest->_tmpBodySize ) ;

         receivedSize = pRest->_tmpBodySize ;

         pRest->_tmpBodyBuf = NULL ;
         pRest->_tmpBodySize = 0 ;
      }

      pRest->_bodySize += receivedSize ;

      while( TRUE )
      {
         rc = _recvData( sock, pBuffer + receivedSize,
                         bufSize - receivedSize, FALSE, &curRecvSize ) ;
         if ( rc )
         {
            rc = SDB_OK ;
            break ;
         }

         pRest->_bodySize += curRecvSize ;

         receivedSize += curRecvSize ;

         if ( bufSize == receivedSize )
         {
            rc = SDB_REST_RECV_SIZE ;
            break ;
         }

         SDB_ASSERT( receivedSize > bufSize, "memory-access errors" ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__RECVRESTIDENTITY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__SENDCHUNKDATA, "restAdaptor::_sendChunkData" )
   INT32 restAdaptor::_sendChunkData( ossSocket *sock, const CHAR *pBuffer,
                                      INT32 length )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__SENDCHUNKDATA ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;

      if ( pBuffer )
      {
         CHAR CRLF[3] = { REST_STRING_CR, REST_STRING_LF, 0 } ;
         CHAR chunkSize[255] = { 0 } ;

         ossSnprintf( chunkSize, 255, "%x\r\n", length ) ;

         // chunk size
         rc = _sendData( sock, REST_FUN_STRING( chunkSize ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }

         // chunk body
         rc = _sendData( sock, pBuffer, length ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }

         // CRLF
         rc = _sendData( sock, REST_FUN_STRING( CRLF ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         // 0 CRLF CRLF
         rc = _sendData( sock, REST_STRING_CHUNKED_END,
                         REST_STRING_CHUNKED_END_SIZE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__SENDCHUNKDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__SENDDATA, "restAdaptor::_sendData" )
   INT32 restAdaptor::_sendData( ossSocket *sock, const CHAR* pData, INT32 size,
                                 BOOLEAN block, INT32 *pSentLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__SENDDATA ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pData, "data is NULL" ) ;
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;

      while( TRUE )
      {
         rc = sock->send ( &pData[totalSentSize], size-totalSentSize,
                           sentSize, _timeout, 0, block ) ;
         totalSentSize += sentSize ;
         if ( _timeout < 0 && SDB_TIMEOUT == rc )
         {
            continue ;
         }
         break ;
      }

      if ( pSentLen )
      {
         *pSentLen = totalSentSize ;
      }

      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__SENDDATA, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RESTADAPTOR__RECVDATA, "restAdaptor::_recvData" )
   INT32 restAdaptor::_recvData( ossSocket *sock, CHAR* pData, INT32 size,
                                 BOOLEAN block, INT32 *pRecvLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RESTADAPTOR__RECVDATA ) ;
      SDB_ASSERT( sock, "socket is NULL" ) ;
      SDB_ASSERT( pData, "data is NULL" ) ;
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;

      while( TRUE )
      {
         rc = sock->recv( &pData[totalReceivedSize], size - totalReceivedSize,
                          receivedSize, _timeout, 0, block ) ;
         totalReceivedSize += receivedSize ;
         if ( _timeout < 0 && SDB_TIMEOUT == rc )
         {
            continue ;
         }
         break ;
      }

      if ( pRecvLen )
      {
         *pRecvLen = totalReceivedSize ;
      }

      PD_TRACE_EXITRC ( SDB_RESTADAPTOR__RECVDATA, rc ) ;
      return rc ;
   }

   restBase::restBase( IExecutor *cb ) : _dataType( HTTP_FILE_DEFAULT ),
                                         _isSetKey( FALSE ),
                                         _isSetVal( FALSE ),
                                         _write( TRUE ),
                                         _CRLFNum( 0 ),
                                         _ZCRLF( 0 ),
                                         _headerSize( 0 ),
                                         _bodySize( 0 ),
                                         _bodyBufSize( 0 ),
                                         _tmpBodySize( 0 ),
                                         _headerBuf( NULL ),
                                         _bodyBuf( NULL ),
                                         _tmpBodyBuf( NULL ),
                                         _cb( cb )
   {
   }

   restBase::~restBase()
   {
      if( _headerBuf )
      {
         _releaseBuff( _headerBuf ) ;

         _headerBuf = NULL ;
      }

      if( _bodyBuf )
      {
         _releaseBuff( _bodyBuf ) ;

         _bodyBuf = NULL ;
      }
   }

   /* read */
   string restBase::getHeader( const string &key )
   {
      REST_COLNAME_MAP_IT it ;
      string value ;

      it = _headerList.find( key ) ;
      if ( it != _headerList.end() )
      {
         value = it->second ;
      }

      return value ;
   }

   BOOLEAN restBase::isHeaderExist( const string &key )
   {
      REST_COLNAME_MAP_IT it = _headerList.find( key ) ;

      return ( it != _headerList.end() ) ;
   }

   HTTP_DATA_TYPE restBase::getDataType()
   {
      return _dataType ;
   }

   BOOLEAN restBase::isKeepAlive()
   {
      BOOLEAN isKeepAlive = FALSE ;
      string value ;

      value = getHeader( REST_STRING_CONNECTION ) ;

      if( REST_STRING_KEEP_ALIVE == value )
      {
         isKeepAlive = TRUE ;
      }

      return isKeepAlive ;
   }

   /* write */
   void restBase::putHeader( const string &key, string value )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;
      REST_COLNAME_MAP_IT it ;

      it = _headerList.find( key ) ;
      if ( it == _headerList.end() )
      {
         _headerList.insert( std::make_pair( key, value ) ) ;
      }
      else
      {
         it->second = value ;
      }
   }

   INT32 restBase::appendBody( const CHAR *pBuffer, INT32 length,
                               INT32 number, BOOLEAN isObjBuffer )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( _write, "rest is read only" ) ;
      SDB_ASSERT( pBuffer, "pBuffer is NULL" ) ;

      if( isObjBuffer )
      {
         _rtnObjBuff rtnObj( pBuffer, length, number ) ;
         BSONObj record ;
         string json ;

         while( TRUE )
         {
            rc = rtnObj.nextObj( record ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else if ( SDB_SYS == rc )
            {
               PD_LOG ( PDERROR, "Failed to get nextObj:rc=%d", rc ) ;
               goto error ;
            }

            json = record.toString( FALSE, TRUE ) ;

            _bodyContent.push_back( json ) ;

            _bodySize += json.length() ;
         }
      }
      else
      {
         string content( pBuffer, length ) ;

         _bodyContent.push_back( content ) ;

         _bodySize += content.length() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 restBase::_malloc( INT32 size, CHAR **buffer )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( buffer, "buffer can't be NULL" ) ;

      *buffer = ( CHAR* )SDB_THREAD_ALLOC( size ) ;
      if ( !(*buffer) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void restBase::_releaseBuff( CHAR *buffer )
   {
      SDB_ASSERT( buffer, "buffer can't be NULL" ) ;

      SDB_THREAD_FREE( buffer ) ;
   }

   INT32 restBase::on_message_begin( void *pData )
   {
      return 0 ;
   }

   INT32 restBase::on_message_complete( void *pData )
   {
      return 0 ;
   }

   INT32 restBase::on_header_field( void *pData, const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)pData ;
      restBase *pRest = (restBase *)pParser->data ;

      if ( pRest->_isSetKey && pRest->_isSetVal )
      {
         pRest->_headerList.insert(
               std::make_pair( pRest->_tmpKey, pRest->_tmpVal ) ) ;

         pRest->_tmpKey = "" ;
         pRest->_tmpVal = "" ;
         pRest->_isSetKey = FALSE ;
         pRest->_isSetVal = FALSE ;
      }

      if ( FALSE == pRest->_isSetKey )
      {
         pRest->_tmpKey = string( at, length ) ;
         pRest->_isSetKey = TRUE ;
      }
      else
      {
         pRest->_tmpKey += string( at, length ) ;
      }

      return 0 ;
   }

   INT32 restBase::on_header_value( void *pData, const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)pData ;
      restBase *pRest = (restBase *)pParser->data ;

      if ( FALSE == pRest->_isSetVal )
      {
         pRest->_tmpVal = string( at, length ) ;
         pRest->_isSetVal = TRUE ;
      }
      else
      {
         pRest->_tmpVal += string( at, length ) ;
      }

      return 0 ;
   }

   INT32 restBase::on_headers_complete( void *pData )
   {
      http_parser *pParser = (http_parser *)pData ;
      restBase *pRest = (restBase *)pParser->data ;

      if ( pRest->_isSetKey && pRest->_isSetVal )
      {
         pRest->_headerList.insert(
               std::make_pair( pRest->_tmpKey, pRest->_tmpVal ) ) ;

         pRest->_tmpKey = "" ;
         pRest->_tmpVal = "" ;
         pRest->_isSetKey = FALSE ;
         pRest->_isSetVal = FALSE ;
      }

      http_parser_pause( pParser, 1 ) ;

      return 0 ;
   }

   INT32 restBase::on_body( void *pData, const CHAR* at, size_t length )
   {
      return 0 ;
   }

   /*
   restRequest
   */
   restRequest::restRequest( IExecutor *cb ) : restBase( cb ),
                                               _method( 1 ),
                                               _command( COM_GETFILE )
   {
   }

   restRequest::~restRequest()
   {
   }

   INT32 restRequest::init()
   {
      INT32 rc = SDB_OK ;

      ossMemset( &_settings, 0, sizeof( http_parser_settings ) ) ;

      _settings.on_message_begin    = restBase::on_message_begin ;
      _settings.on_url              = restRequest::on_url ;
      _settings.on_header_field     = restBase::on_header_field ;
      _settings.on_header_value     = restBase::on_header_value ;
      _settings.on_headers_complete = restBase::on_headers_complete ;
      _settings.on_body             = restBase::on_body ;
      _settings.on_message_complete = restBase::on_message_complete ;

      return rc ;
   }

   /* write */
   void restRequest::setUrlPath( string path )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      _path = path ;
   }

   void restRequest::setDataType( HTTP_DATA_TYPE type )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      _dataType = type ;
   }

   void restRequest::buildResponse( restResponse &response )
   {
      response.putHeader( REST_STRING_CONNECTION, REST_STRING_CLOSE ) ;
      response.putHeader( REST_STRING_CACHE_CONTROL, REST_STRING_NO_STORE ) ;
      response.putHeader( REST_STRING_PRAGMA, REST_STRING_NO_CACHE ) ;
      response.putHeader( REST_STRING_CONTENT_TYPE, REST_STRING_TEXT_HTML ) ;

      if ( COM_CMD == _command )
      {
         string accept = getHeader( REST_STRING_ACCEPT ) ;

         if ( NULL == ossStrstr( accept.c_str(), REST_STRING_APP_JSON ) )
         {
            response.setDataType( getDataType() ) ;
         }
         else
         {
            response.setDataType( HTTP_FILE_JSON ) ;
         }
      }
      else
      {
         response.setDataType( getDataType() ) ;
      }

      if ( isKeepAlive() )
      {
         response.setKeepAlive() ;
      }
   }

   HTTP_PARSE_COMMAND restRequest::getCommand()
   {
      return _command ;
   }

   string restRequest::getRequestPath()
   {
      return _path ;
   }

   string restRequest::getQuery( const string &key )
   {
      REST_COLNAME_MAP_IT it ;
      string value ;

      it = _queryList.find( key ) ;
      if ( it != _queryList.end() )
      {
         value = it->second ;
      }

      return value ;
   }

   BOOLEAN restRequest::isQueryArgExist( const string &key )
   {
      REST_COLNAME_MAP_IT it = _queryList.find( key ) ;

      return ( it != _queryList.end() ) ;
   }

   INT32 restRequest::on_url( void *pData, const CHAR* at, size_t length )
   {
      INT32 rc = SDB_OK ;
      UINT32 pathLength = 0 ;
      http_parser *pParser = (http_parser *)pData ;
      restRequest *request = (restRequest *)pParser->data ;

      for( UINT32 i = 0; i < length; ++i )
      {
         if ( '?' == at[i] )
         {
            pathLength = i ;
            break ;
         }
      }

      if( 0 == pathLength )
      {
         pathLength = (UINT32)length ;
      }

      request->_path = string( at, pathLength ) ;

      if( pathLength < (UINT32)length )
      {
         rc = request->_parse_http_query( at + pathLength + 1,
                                          length - pathLength - 1 ) ;
         if ( rc )
         {
            return rc ;
         }
      }

      request->_parseDataType() ;

      return 0 ;
   }

   INT32 restRequest::_parse_http_query( const CHAR *pBuffer, INT32 length )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pKeyBuf   = pBuffer ;
      const CHAR *pValueBuf = NULL ;
      string key ;
      string value ;

      for ( INT32 i = 0; i < length; ++i )
      {
         if ( pBuffer[i] == '=' && key.empty() )
         {
            INT32 keyLength = pBuffer + i - pKeyBuf ;

            if( keyLength > 0 )
            {
               INT32 tempDecodeLen = urlDecodeSize( pKeyBuf, keyLength ) ;
               CHAR *pDecodeBuf = NULL ;

               rc = _malloc( tempDecodeLen + 1, &pDecodeBuf ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, "
                                    "rc=%d",
                           tempDecodeLen, rc ) ;
                  goto error ;
               }

               pDecodeBuf[tempDecodeLen] = 0 ;

               urlDecode( pKeyBuf, keyLength, &pDecodeBuf, tempDecodeLen ) ;

               key = string( pDecodeBuf, tempDecodeLen ) ;

               _releaseBuff( pDecodeBuf ) ;
            }
            else
            {
               key = "" ;
            }

            pValueBuf = pBuffer + i + 1 ;
         }

         if ( pBuffer[i] == '&' || ( i + 1 >= length ) )
         {
            INT32 valueLength = pBuffer + i - pValueBuf ;

            if ( i + 1 >= length )
            {
               ++valueLength ;
            }

            if( key.length() > 0 && valueLength > 0 )
            {
               INT32 tempDecodeLen = urlDecodeSize( pValueBuf, valueLength ) ;
               CHAR *pDecodeBuf = NULL ;

               rc = _malloc( tempDecodeLen + 1, &pDecodeBuf ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, "
                                    "rc=%d",
                           tempDecodeLen, rc ) ;
                  goto error ;
               }

               pDecodeBuf[tempDecodeLen] = 0 ;

               urlDecode( pValueBuf, valueLength,
                          &pDecodeBuf, tempDecodeLen ) ;

               value = string( pDecodeBuf, tempDecodeLen ) ;

               _releaseBuff( pDecodeBuf ) ;
            }
            else
            {
               value = "" ;
            }

            pKeyBuf = pBuffer + i + 1 ;

            if ( FALSE == key.empty() )
            {
               _queryList.insert( std::make_pair( key, value ) ) ;
               key = "" ;
               value = "" ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string restRequest::_generateHeader()
   {
      string header = http_method_str( (enum http_method)_method ) ;

      header += " " + _path + " "REST_STRING_HTTP ;

      return header ;
   }

   void restRequest::_parseDataType()
   {
      const CHAR *pFileName = NULL ;
      const CHAR *pExtension = NULL ;

      if( FALSE == _path.empty() )
      {
         pFileName = _getResourceFileName( _path.c_str() ) ;
      }

      if ( pFileName )
      {
         pExtension = _getFileExtension( pFileName ) ;
         if ( NULL == pExtension )
         {
            _command = COM_CMD ;
            _dataType = HTTP_FILE_HTML ;
         }
         else
         {
            UINT32 extenSize = ossStrlen( pExtension ) ;

            _command = COM_GETFILE ;
            _dataType = HTTP_FILE_UNKNOW ;

            for( UINT32 i = 0; i < REST_STRING_FILE_EX_SIZE; ++i )
            {
               if ( ossStrlen( fileExtension[i] ) == extenSize &&
                    0 == ossStrncasecmp( pExtension, fileExtension[i],
                                         extenSize ) )
               {
                  _dataType = (HTTP_DATA_TYPE)i ;
                  break ;
               }
            }

            if( HTTP_FILE_JSP == _dataType ||
                HTTP_FILE_PHP == _dataType ||
                HTTP_FILE_ASP == _dataType )
            {
               _command = COM_CMD ;
               _dataType = HTTP_FILE_HTML ;
            }
         }
      }
      else
      {
         if ( _queryList.size() > 0 )
         {
            _command = COM_CMD ;
            _dataType = HTTP_FILE_HTML ;
         }
         else
         {
            _command = COM_GETFILE ;
            _dataType = HTTP_FILE_DEFAULT ;
         }
      }
   }

   OSS_INLINE const CHAR *restRequest::_getResourceFileName( const CHAR *pPath )
   {
      INT32 pathLen = ossStrlen( pPath ) ;
      for ( INT32 i = pathLen - 1; i >= 0; --i )
      {
         if( pPath[i] == '/' )
         {
            if ( i + 1  >= pathLen )
            {
               return NULL ;
            }
            else
            {
               return ( pPath + i + 1 ) ;
            }
         }
      }
      return NULL ;
   }

   OSS_INLINE const CHAR *restRequest::_getFileExtension( const CHAR *pFileName )
   {
      INT32 fileNameLen = ossStrlen( pFileName ) ;
      for ( INT32 i = fileNameLen - 1; i >= 0; --i )
      {
         if( pFileName[i] == '.' )
         {
            if ( i + 1  >= fileNameLen )
            {
               return NULL ;
            }
            else
            {
               return ( pFileName + i + 1 ) ;
            }
         }
      }
      return NULL ;
   }

   /*
   restResponse
   */
   restResponse::restResponse( IExecutor *cb ) : restBase( cb ),
                                                 _isChunk( FALSE ),
                                                 _rspCode( HTTP_OK )
   {
   }

   restResponse::~restResponse()
   {
   }

   INT32 restResponse::init()
   {
      INT32 rc = SDB_OK ;

      ossMemset( &_settings, 0, sizeof( http_parser_settings ) ) ;

      _settings.on_message_begin    = restBase::on_message_begin ;
      _settings.on_header_field     = restBase::on_header_field ;
      _settings.on_header_value     = restBase::on_header_value ;
      _settings.on_headers_complete = restBase::on_headers_complete ;
      _settings.on_body             = restBase::on_body ;
      _settings.on_message_complete = restBase::on_message_complete ;

      return rc ;
   }

   void restResponse::setDataType( HTTP_DATA_TYPE type )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;
      string fileType ;

      switch( type )
      {
      case HTTP_FILE_JSON:
         fileType = REST_STRING_APP_JSON ;
         break ;
      case HTTP_FILE_PNG:
         fileType = REST_STRING_TEXT_PNG ;
         break ;
      case HTTP_FILE_BMP:
         fileType = REST_STRING_TEXT_BMP ;
         break ;
      case HTTP_FILE_JPG:
         fileType = REST_STRING_TEXT_JPG ;
         break ;
      case HTTP_FILE_GIF:
         fileType = REST_STRING_TEXT_GIF ;
         break ;
      case HTTP_FILE_SVG:
         fileType = REST_STRING_TEXT_SVG ;
         break ;
      case HTTP_FILE_ICON:
         fileType = REST_STRING_IMAGE_ICON ;
         break ;
      case HTTP_FILE_WOFF:
         fileType = REST_STRING_APP_WOFF ;
         break ;
      case HTTP_FILE_EOT:
         fileType = REST_STRING_APP_EOT ;
         break ;
      case HTTP_FILE_OTF:
      case HTTP_FILE_TTF:
         fileType = REST_STRING_TEXT_OCTET_STREAM ;
         break ;
      case HTTP_FILE_JS:
         fileType = REST_STRING_TEXT_JS ;
         break ;
      case HTTP_FILE_CSS:
         fileType = REST_STRING_TEXT_CSS ;
         break ;
      case HTTP_FILE_HTML:
      case HTTP_FILE_DEFAULT:
      case HTTP_FILE_UNKNOW:
      default:
         fileType = REST_STRING_TEXT_HTML ;
         break ;
      }

      _dataType = type ;
      putHeader( REST_STRING_CONTENT_TYPE, fileType ) ;
   }

   void restResponse::putHeader( const string &key, string value )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      if( REST_STRING_TRANSFER == key &&
          REST_STRING_CHUNKED == value )
      {
         _isChunk = TRUE ;
      }

      restBase::putHeader( key, value ) ;
   }

   INT32 restResponse::appendBody( const CHAR *pBuffer, INT32 length,
                                   INT32 number, BOOLEAN isObjBuffer )
   {
      SDB_ASSERT( !_isChunk, "chunk mode can not call appendBody" ) ;

      if( isObjBuffer && _bodyContent.size() == 0 )
      {
         string result = REST_RESULT_STRING_OK ;

         _bodyContent.push_back( result ) ;

         _bodySize += result.length() ;
      }

      if ( FALSE == isObjBuffer && HTTP_FILE_JSON == _dataType )
      {
         setDataType( HTTP_FILE_HTML ) ;
      }

      return restBase::appendBody( pBuffer, length, number, isObjBuffer ) ;
   }

   void restResponse::setResponse( HTTP_RESPONSE_CODE rspCode )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      _rspCode = rspCode ;
   }

   void restResponse::setOPResult( INT32 result, const BSONObj &info )
   {
      SDB_ASSERT( _write, "rest is read only" ) ;
      string json = info.toString( FALSE, TRUE ) ;

      if( _bodyContent.size() == 0 )
      {
         _bodyContent.push_back( json ) ;
      }
      else
      {
         _bodySize -= _bodyContent[0].length() ;

         _bodyContent[0] = json ;
      }

      _bodySize += json.length() ;
   }

   void restResponse::setKeepAlive()
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      _headerList[ REST_STRING_CONNECTION ] = REST_STRING_KEEP_ALIVE ;
   }

   void restResponse::setConnectionClose()
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      _headerList[ REST_STRING_CONNECTION ] = REST_STRING_CLOSE ;
   }

   void restResponse::setChunkModal()
   {
      SDB_ASSERT( _write, "rest is read only" ) ;

      putHeader( REST_STRING_TRANSFER, REST_STRING_CHUNKED ) ;
   }

   string restResponse::_generateHeader()
   {
      // HTTP/1.1 200 OK
      string header = REST_STRING_HTTP" " ;

      header += responseHeader[ _rspCode ] ;

      return header ;
   }
}

