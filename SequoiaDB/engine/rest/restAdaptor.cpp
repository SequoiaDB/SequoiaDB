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
#include "restAdaptor.hpp"
#include "http_parser.hpp"
#include "ossMem.h"
#include "ossUtil.h"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "restTrace.hpp"
#include "dms.hpp"
#include "rtnContext.hpp"
#include "../util/url.h"

/* once recv size */
#define REST_ONCE_RECV_SIZE       1024

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
#define REST_STRING_TEXT_SVG      "image/svg+xml"
#define REST_STRING_TEXT_WOFF     "application/font-woff"
#define REST_STRING_TEXT_EOT      "application/vnd.ms-fontobject"
#define REST_STRING_TEXT_OCTET_STREAM  "application/octet-stream"
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

static const CHAR *responseHeader[] = {
      "200 Ok",   "302 Found",   "400 Bad Request",   "404 Not Found",
      "503 Service Unavailable",   "505 Http Version Not Supported"
} ;

static const CHAR *fileExtension[] = {
      "html",   "js",   "css",   "png",   "bmp",   "jpg",
      "gif",    "svg",  "woff",  "eot",   "otf",   "ttf",
      "jsp",    "php",  "asp"
} ;

#define REST_STRING_FILE_EX_SIZE ( sizeof( fileExtension ) \
                                 / sizeof( fileExtension[0] ) )

namespace engine
{
   INT32 restAdaptor::on_message_begin( void *pData )
   {
      return 0 ;
   }

   INT32 restAdaptor::on_headers_complete( void *pData )
   {
      http_parser *pParser = (http_parser *)pData ;
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

   INT32 restAdaptor::on_message_complete( void *pData )
   {
      return 0 ;
   }

   INT32 restAdaptor::on_url( void *pData,
                              const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)pData ;
      httpConnection *pHttpCon = (httpConnection *)pParser->data ;
      UINT32 i = 0 ;
      CHAR *pPath = NULL ;

      pHttpCon->_pPath = at ;
      for( ; i < length && at[i] != '?'; ++i ) ;

      pPath = pHttpCon->_pHeaderBuf +
         ( at - pHttpCon->_pHeaderBuf ) ;
      pPath[i] = 0 ;


      if( i + 1 < length )
      {
         ++i ;
         pHttpCon->_pQuery = pPath + i ;
         pHttpCon->_querySize = length - i ;
      }
      else
      {
         pHttpCon->_pQuery = NULL ;
         pHttpCon->_querySize = 0 ;
      }
      return 0 ;
   }

   INT32 restAdaptor::on_header_field( void *pData,
                                       const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)pData ;
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

   INT32 restAdaptor::on_header_value( void *pData,
                                       const CHAR* at, size_t length )
   {
      http_parser *pParser = (http_parser *)pData ;
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

   INT32 restAdaptor::on_body( void *pData,
                               const CHAR* at, size_t length )
   {
      return 0 ;
   }

   restAdaptor::restAdaptor() : _maxHttpHeaderSize(0),
                                _maxHttpBodySize(0),
                                _timeout(0),
                                _pSettings(NULL)
   {
   }

   restAdaptor::~restAdaptor()
   {
      if ( _pSettings )
      {
         SDB_OSS_FREE( _pSettings ) ;
         _pSettings = NULL ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_PARQUERY, "restAdaptor::_parse_http_query" )
   INT32 restAdaptor::_parse_http_query( httpConnection *pHttpConnection,
                                         CHAR *pBuffer, INT32 length,
                                         CHAR *pDecodeBuff, INT32 decodeLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_PARQUERY ) ;
      INT32 keyLen      = 0 ;
      INT32 valueLen    = 0 ;
      INT32 keyOffset   = 0 ;
      INT32 valueOffset = 0 ;
      INT32 useLen      = 0 ;
      INT32 tempDecodeLen = 0 ;
      CHAR *pKeyBuf = NULL ;
      CHAR *pValueBuf = NULL ;
      CHAR *pDecodeKeyBuf = NULL ;
      CHAR *pDecodeValueBuf = NULL ;

      for ( INT32 i = 0; i < length; ++i )
      {
         if ( pBuffer[i] == '=' && pValueBuf == NULL )
         {
            valueOffset = i + 1 ;
            pValueBuf = pBuffer + valueOffset ;
         }
         else if ( pBuffer[i] == '&' || ( i + 1 == length ) )
         {
            pKeyBuf = pBuffer + keyOffset ;
            keyLen = valueOffset - keyOffset - 1 ;
            valueLen = i - valueOffset ;
            if( i + 1 == length )
            {
               ++valueLen ;
            }
            pDecodeKeyBuf = pDecodeBuff + useLen ;
            tempDecodeLen = urlDecodeSize( pKeyBuf, keyLen ) ;
            urlDecode( pKeyBuf, keyLen,
                       &pDecodeKeyBuf, tempDecodeLen ) ;
            useLen += tempDecodeLen ;
            pDecodeBuff[useLen] = 0 ;
            ++useLen ;
            if( pValueBuf )
            {
               pDecodeValueBuf = pDecodeBuff + useLen ;
               tempDecodeLen = urlDecodeSize( pValueBuf, valueLen ) ;
               urlDecode( pValueBuf, valueLen,
                          &pDecodeValueBuf, tempDecodeLen ) ;
               useLen += tempDecodeLen ;
               pDecodeBuff[useLen] = 0 ;
               ++useLen ;
            }
            pHttpConnection->_requestQuery.insert(
                     std::make_pair( pDecodeKeyBuf, pDecodeValueBuf ) ) ;
            pValueBuf = NULL ;
            pDecodeValueBuf = NULL ;
            keyOffset = i + 1 ;
         }
      }
      PD_TRACE_EXITRC ( SDB__RESTADP_PARQUERY, rc ) ;
      return rc ;
   }

   OSS_INLINE const CHAR *restAdaptor::_getResourceFileName( const CHAR *pPath )
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

   OSS_INLINE const CHAR *restAdaptor::_getFileExtension(
         const CHAR *pFileName )
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

   OSS_INLINE BOOLEAN restAdaptor::_checkEndOfHeader( httpConnection * pHttpCon,
                                                      CHAR *pBuffer,
                                                      INT32 bufferSize,
                                                      INT32 &bodyOffset )
   {
      BOOLEAN rc = TRUE ;
      for ( INT32 i = 0; i < bufferSize; ++i )
      {
         if ( pBuffer[i] == REST_STRING_CR )
         {
            switch( pHttpCon->_CRLFNum )
            {
            case 2: // x  \r \n \r
               pHttpCon->_CRLFNum = 3 ;
               break ;
            case 0: // x  x  x  \r
            case 1: // x  x  \r \r
            case 3: // \r \n \r \r
            default:
               pHttpCon->_CRLFNum = 1 ;
               break ;
            }
         }
         else if ( pBuffer[i] == REST_STRING_LF )
         {
            switch( pHttpCon->_CRLFNum )
            {
            case 1: // x  x  \r \n
               pHttpCon->_CRLFNum = 2 ;
               break ;
            case 3: // \r \n \r \n
               pHttpCon->_CRLFNum = 4 ;
               break ;
            case 2: // x  \r \n \n
            case 0: // x  x  x  \n
            default:
               pHttpCon->_CRLFNum = 0 ;
               break ;
            }
         }
         else
         {
            pHttpCon->_CRLFNum = 0 ;
         }

         if ( pHttpCon->_CRLFNum == 4 )
         {
            rc = TRUE ;
            if ( i + 1 == bufferSize )
            {
               bodyOffset = 0 ;
            }
            else
            {
               bodyOffset = i + 1 ;
            }
            goto done ;
         }
      }
      rc = FALSE ;

   done:
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_CONVERTMSG, "restAdaptor::_convertMsg" )
   INT32 restAdaptor::_convertMsg( pmdRestSession *pSession,
                                   HTTP_PARSE_COMMON &common,
                                   CHAR **ppMsg,
                                   INT32 &msgSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_CONVERTMSG );
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      SDB_ASSERT ( ppMsg, "pMsg is NULL" ) ;
      UINT32 pathSize = 0 ;
      UINT32 extenSize = 0 ;
      const CHAR *pFileName = NULL ;
      const CHAR *pExtension = NULL ;
      CHAR *pMsg = NULL ;
      httpConnection *pHttpCon = pSession->getRestConn() ;

      if( pHttpCon->_pPath )
      {
         pFileName = _getResourceFileName( pHttpCon->_pPath ) ;
      }

      if ( pFileName )
      {
         pExtension = _getFileExtension( pFileName ) ;
         if ( NULL == pExtension )
         {
            common = COM_CMD ;
            pHttpCon->_fileType = HTTP_FILE_HTML ;
         }
         else
         {
            common = COM_GETFILE ;
            pHttpCon->_fileType = HTTP_FILE_UNKNOW ;
            extenSize = ossStrlen( pExtension ) ;

            for( UINT32 i = 0; i < REST_STRING_FILE_EX_SIZE; ++i )
            {
               if ( 0 == ossStrncasecmp( pExtension,
                                         fileExtension[i], extenSize ) )
               {
                  pHttpCon->_fileType = (HTTP_FILE_TYPE)i ;
                  break ;
               }
            }

            if( HTTP_FILE_JSP == pHttpCon->_fileType ||
                HTTP_FILE_PHP == pHttpCon->_fileType ||
                HTTP_FILE_ASP == pHttpCon->_fileType )
            {
               common = COM_CMD ;
               pHttpCon->_fileType = HTTP_FILE_HTML ;
            }
         }
      }
      else
      {
         if ( pHttpCon->_requestQuery.size() > 0 )
         {
            common = COM_CMD ;
            pHttpCon->_fileType = HTTP_FILE_HTML ;
         }
         else
         {
            common = COM_GETFILE ;
            pHttpCon->_fileType = HTTP_FILE_DEFAULT ;
         }
      }

      if( pHttpCon->_pPath )
      {
         pathSize = ossStrlen( pHttpCon->_pPath ) ;
         rc = pSession->allocBuff( pathSize + 1, &pMsg, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %u bytes memory, rc=%d",
                     pathSize, rc ) ;
            goto error ;
         }
         ossMemcpy( pMsg, pHttpCon->_pPath, pathSize ) ;
         pMsg[pathSize] = 0 ;
      }
      else
      {
         pathSize = 1 ;
         rc = pSession->allocBuff( pathSize + 1, &pMsg, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %u bytes memory, rc=%d",
                     pathSize, rc ) ;
            goto error ;
         }
         pMsg[0] = '/' ;
         pMsg[1] = 0 ;
      }

      *ppMsg = pMsg ;
      msgSize = pathSize ;

   done:
      PD_TRACE_EXITRC ( SDB__RESTADP_CONVERTMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_INIT, "restAdaptor::init" )
   INT32 restAdaptor::init( INT32 maxHttpHeaderSize,
                            INT32 maxHttpBodySize,
                            INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_INIT );

      http_parser_settings *pSettings = NULL ;

      pSettings = (http_parser_settings *)SDB_OSS_MALLOC(
            sizeof( http_parser_settings ) ) ;
      if ( !pSettings )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Unable to allocate %d bytes memory",
                  sizeof( http_parser_settings ) ) ;
         goto error ;
      }

      ossMemset( pSettings, 0, sizeof( http_parser_settings ) ) ;
      pSettings->on_message_begin    = restAdaptor::on_message_begin ;
      pSettings->on_url              = restAdaptor::on_url ;
      pSettings->on_header_field     = restAdaptor::on_header_field ;
      pSettings->on_header_value     = restAdaptor::on_header_value ;
      pSettings->on_headers_complete = restAdaptor::on_headers_complete ;
      pSettings->on_body             = restAdaptor::on_body ;
      pSettings->on_message_complete = restAdaptor::on_message_complete ;

      _maxHttpHeaderSize = maxHttpHeaderSize ;
      _maxHttpBodySize = maxHttpBodySize ;
      _timeout = timeout ;
      _pSettings = pSettings ;

   done:
      PD_TRACE_EXITRC ( SDB__RESTADP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_PARAINIT, "restAdaptor::_paraInit" )
   void restAdaptor::_paraInit( httpConnection *pHttpCon )
   {
      PD_TRACE_ENTRY( SDB__RESTADP_PARAINIT ) ;
      pHttpCon->_tempKeyLen       = 0 ;
      pHttpCon->_tempValueLen     = 0 ;
      pHttpCon->_CRLFNum          = 0 ;
      pHttpCon->_headerSize       = 0 ;
      pHttpCon->_bodySize         = 0 ;
      pHttpCon->_partSize         = 0 ;
      pHttpCon->_firstRecordSize  = ( sizeof(REST_RESULT_STRING_OK) - 1 ) ;
      pHttpCon->_responseSize     = 0 ;
      pHttpCon->_isChunk          = FALSE ;
      pHttpCon->_isSendHttpHeader = FALSE ;
      pHttpCon->_isKey            = TRUE ;
      pHttpCon->_pSourceHeaderBuf = NULL ;
      pHttpCon->_pHeaderBuf       = NULL ;
      pHttpCon->_pPartBody        = NULL ;
      pHttpCon->_pBodyBuf         = NULL ;
      pHttpCon->_pSendBuffer      = NULL ;
      pHttpCon->_pTempKey         = NULL ;
      pHttpCon->_pTempValue       = NULL ;
      pHttpCon->_pPath            = NULL ;
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
      PD_TRACE_EXIT( SDB__RESTADP_PARAINIT ) ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_RECVREQHE, "restAdaptor::recvRequestHeader" )
   INT32 restAdaptor::recvRequestHeader( pmdRestSession *pSession )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_RECVREQHE ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      CHAR *pBuffer = pSession->getFixBuff() ;
      INT32 bufSize = pSession->getFixBuffSize() ;
      http_parser *pParser = &(pHttpCon->_httpParser) ;
      CHAR *pUrl = NULL ;
      INT32 curRecvSize  = 0 ;
      INT32 receivedSize = 0 ;
      INT32 bodyOffset = 0 ;
      INT32 urlSize = 0 ;
      UINT32 recvSize = 0 ;

      _paraInit( pHttpCon ) ;

      _maxHttpHeaderSize = _maxHttpHeaderSize > bufSize ?
            bufSize : _maxHttpHeaderSize ;

      pHttpCon->_pHeaderBuf = pBuffer ;

      while( true )
      {
         recvSize = _maxHttpHeaderSize - receivedSize - 1 ;
         rc = pSession->recvData( pBuffer + receivedSize,
                                  recvSize,
                                  _timeout,
                                  FALSE,
                                  &curRecvSize,
                                  0 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to recv, rc=%d", rc ) ;
            goto error ;
         }

         pBuffer[ receivedSize + curRecvSize + 1 ] = '\0' ;

         if ( _checkEndOfHeader( pHttpCon, pBuffer + receivedSize,
                                 curRecvSize, bodyOffset ) )
         {
            if ( bodyOffset > 0 )
            {
               pHttpCon->_partSize  = curRecvSize - bodyOffset ;
               pHttpCon->_pPartBody = pBuffer + receivedSize + bodyOffset ;
               receivedSize += bodyOffset ;
            }
            else
            {
               receivedSize += curRecvSize ;
            }
            pHttpCon->_headerSize = receivedSize ;

            rc = pSession->allocBuff( pHttpCon->_headerSize + 1,
                                      &pHttpCon->_pSourceHeaderBuf, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                        pHttpCon->_headerSize + 1, rc ) ;
               goto error ;
            }

            ossMemcpy( pHttpCon->_pSourceHeaderBuf, pHttpCon->_pHeaderBuf,
                       pHttpCon->_headerSize ) ;
            pHttpCon->_pSourceHeaderBuf[pHttpCon->_headerSize] = 0 ;

            break ;
         }
         else
         {
            receivedSize += curRecvSize ;
            if ( receivedSize >= _maxHttpHeaderSize )
            {
               rc = SDB_REST_RECV_SIZE ;
               PD_LOG ( PDERROR, "http header size %d greater than %d",
                        receivedSize,
                        _maxHttpHeaderSize ) ;
               goto error ;
            }
         }
      }

      http_parser_init( pParser, HTTP_BOTH ) ;
      if( http_parser_execute( pParser, (http_parser_settings *)_pSettings,
                               pBuffer, (UINT32)receivedSize )
                != (UINT32)receivedSize )
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

      if( pHttpCon->_pQuery != NULL )
      {
         urlSize = urlDecodeSize( pHttpCon->_pQuery, pHttpCon->_querySize ) ;
         rc = pSession->allocBuff( urlSize + 1, &pUrl, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     urlSize + 1, rc ) ;
            goto error ;
         }
         pUrl[ urlSize ] = 0 ;
         _parse_http_query( pHttpCon,
                            pHttpCon->_pQuery,
                            pHttpCon->_querySize,
                            pUrl,
                            urlSize ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_RECVREQHE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_RECVREQBO, "restAdaptor::recvRequestBody" )
   INT32 restAdaptor::recvRequestBody( pmdRestSession *pSession,
                                       HTTP_PARSE_COMMON &common,
                                       CHAR **ppPath,
                                       INT32 &pathSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_RECVREQBO ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      SDB_ASSERT ( ppPath, "ppPath is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      CHAR *pBuffer = NULL ;
      const CHAR *pContentLength = NULL ;
      CHAR *pUrl = NULL ;
      INT32 bodySize = 0 ;
      INT32 curRecvSize  = 0 ;
      INT32 receivedSize = 0 ;
      INT32 urlSize = 0 ;

      rc = getHttpHeader( pSession, REST_STRING_CONLEN, &pContentLength ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get http header, rc=%d", rc ) ;
         goto error ;
      }

      if ( pContentLength )
      {
         bodySize = ossAtoi( pContentLength ) ;
         if ( bodySize > 0 )
         {
            if ( bodySize > _maxHttpBodySize )
            {
               rc = SDB_REST_RECV_SIZE ;
               PD_LOG ( PDERROR, "http body size %d greater than %d",
                        bodySize,
                        _maxHttpBodySize ) ;
               goto error ;
            }

            rc = pSession->allocBuff( bodySize + 1,
                                      &(pHttpCon->_pBodyBuf),
                                      NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                        bodySize, rc ) ;
               goto error ;
            }
            pBuffer = pHttpCon->_pBodyBuf ;
            pBuffer[bodySize] = 0 ;

            pHttpCon->_bodySize = bodySize ;

            if ( pHttpCon->_pPartBody )
            {
               ossMemcpy( pHttpCon->_pBodyBuf,
                          pHttpCon->_pPartBody,
                          pHttpCon->_partSize ) ;
               receivedSize = pHttpCon->_partSize ;
            }

            rc = pSession->recvData( pBuffer + receivedSize,
                                     bodySize - receivedSize,
                                     _timeout,
                                     TRUE,
                                     &curRecvSize,
                                     0 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to recv, rc=%d", rc ) ;
               goto error ;
            }
            receivedSize += curRecvSize ;

            urlSize = urlDecodeSize( pBuffer, receivedSize ) ;
            rc = pSession->allocBuff( urlSize + 1, &pUrl, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                        urlSize + 1, rc ) ;
               goto error ;
            }
            pUrl[ urlSize ] = 0 ;
            _parse_http_query( pHttpCon, pBuffer, receivedSize,
                               pUrl, urlSize ) ;
         }
      }

      rc = _convertMsg( pSession, common, ppPath, pathSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to build msg, rc=%d", rc ) ;
         goto error ;
      }
      pHttpCon->_common = common ;
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_RECVREQBO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_SETOPR, "restAdaptor::setOPResult" )
   INT32 restAdaptor::setOPResult( pmdRestSession *pSession,
                                   INT32 result,
                                   const BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_SETOPR ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;

      if( COM_GETFILE != pHttpCon->_common )
      {
         std::string str = info.toString( FALSE, TRUE ) ;
         INT32 bufferSize = ossStrlen( str.c_str() ) ;
         if( TRUE == pHttpCon->_isChunk )
         {
            if( FALSE == pHttpCon->_isSendHttpHeader )
            {
               pHttpCon->_isSendHttpHeader = TRUE ;
               rc = _setResponseType( pSession ) ;
               if( rc )
               {
                  PD_LOG ( PDERROR, "Failed to set respone type, rc=%d",
                           rc ) ;
                  goto error ;
               }
               rc = _sendHttpHeader( pSession, HTTP_OK ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to send http header, rc=%d", rc ) ;
                  goto error ;
               }
            }
            rc = _sendHttpChunk( pSession, str.c_str(), bufferSize ) ;
            if( rc )
            {
               PD_LOG ( PDERROR, "Failed to send http chunk, rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            CHAR *pBuffer = NULL ;
            httpResponse httpRe ;
            rc = pSession->allocBuff( bufferSize + 1, &pBuffer, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                        bufferSize + 1, rc ) ;
               goto error ;
            }
            ossMemcpy( pBuffer, str.c_str(), bufferSize ) ;
            pBuffer[ bufferSize ] = 0 ;
            pHttpCon->_firstRecordSize = bufferSize ;
            httpRe.pBuffer = pBuffer ;
            httpRe.len = bufferSize ;
            pHttpCon->_responseBody[0] = httpRe ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_SETOPR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_SENDRE, "restAdaptor::sendResponse" )
   INT32 restAdaptor::sendResponse( pmdRestSession *pSession,
                                    HTTP_RESPONSE_CODE rspCode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_SENDRE ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      CHAR httpBodySize[256] = { 0 } ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      std::vector<httpResponse>::iterator it ;
      httpResponse httpRe ;

      if( TRUE == pHttpCon->_isChunk )
      {
         rc = pSession->sendData( REST_STRING_CHUNKED_END,
                                  REST_STRING_CHUNKED_END_SIZE,
                                  _timeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if( HTTP_OK == rspCode )
         {
            ossSnprintf( httpBodySize, 255, "%d",
                         pHttpCon->_firstRecordSize + pHttpCon->_responseSize );
            rc = appendHttpHeader( pSession, REST_STRING_CONLEN, httpBodySize );
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call append http header, rc=%d",
                        rc ) ;
               goto error ;
            }
            rc = _setResponseType( pSession ) ;
            if( rc )
            {
               PD_LOG ( PDERROR, "Failed to set respone type, rc=%d",
                        rc ) ;
               goto error ;
            }
         }
         else
         {
            ossSnprintf( httpBodySize, 255, "0" ) ;
            rc = appendHttpHeader( pSession, REST_STRING_CONLEN, httpBodySize );
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call append http header, rc=%d",
                        rc ) ;
               goto error ;
            }
         }
         rc = _sendHttpHeader( pSession, rspCode ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send http header, rc=%d", rc ) ;
            goto error ;
         }

         if( HTTP_OK == rspCode )
         {
            for( it = pHttpCon->_responseBody.begin();
                 it != pHttpCon->_responseBody.end(); ++it )
            {
               httpRe = (*(it)) ;
               rc = pSession->sendData( httpRe.pBuffer, httpRe.len,
                                        _timeout ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_SENDRE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_APPENDHEADER, "restAdaptor::appendHttpHeader" )
   INT32 restAdaptor::appendHttpHeader( pmdRestSession *pSession,
                                        const CHAR *pKey,
                                        const CHAR *pValue )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_APPENDHEADER ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      INT32 keySize = ossStrlen( pKey ) ;
      INT32 valueSize = ossStrlen( pValue ) ;
      INT32 newHeaderSize = keySize + valueSize + 2 ;
      CHAR *pNewHeaderBuf = NULL ;
      CHAR *pNewKey = NULL ;
      CHAR *pNewValue = NULL ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      COLNAME_MAP_IT it ;

      if( REST_STRING_TRANSFER_SIZE == ossStrlen( pKey ) && 
          0 == ossStrncmp( pKey, REST_STRING_TRANSFER,
                           REST_STRING_TRANSFER_SIZE ) &&
          REST_STRING_CHUNKED_SIZE == ossStrlen( pValue ) &&
          0 == ossStrncmp( pValue, REST_STRING_CHUNKED,
                           REST_STRING_CHUNKED_SIZE ) )
      {
         pHttpCon->_isChunk = TRUE ;
      }

      it = pHttpCon->_responseHeaders.find( pKey ) ;
      if ( it == pHttpCon->_responseHeaders.end() )
      {
         rc = pSession->allocBuff( newHeaderSize, &pNewHeaderBuf, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     newHeaderSize, rc ) ;
            goto error ;
         }

         pNewKey = pNewHeaderBuf ;
         pNewValue = pNewHeaderBuf + keySize + 1 ;
         ossStrncpy( pNewKey, pKey, keySize ) ;
         ossStrncpy( pNewValue, pValue, valueSize ) ;
         pNewKey[ keySize ] = 0 ;
         pNewValue[ valueSize ] = 0 ;
         pHttpCon->_responseHeaders.insert(
               std::make_pair( pNewKey, pNewValue ) ) ;
      }
      else
      {
         rc = pSession->allocBuff( valueSize + 1, &pNewValue, NULL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                     valueSize, rc ) ;
            goto error ;
         }
         ossStrncpy( pNewValue, pValue, valueSize ) ;
         pNewValue[ valueSize ] = 0 ;
         it->second = pNewValue ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RESTADP_APPENDHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void restAdaptor::_getQuery( httpConnection *pHttpCon,
                                 const CHAR *pKey,
                                 const CHAR **ppValue )
   {
      SDB_ASSERT ( pHttpCon, "pSession is NULL" ) ;
      SDB_ASSERT ( pKey, "pKey is NULL" ) ;
      SDB_ASSERT ( ppValue, "ppValue is NULL" ) ;
      COLNAME_MAP_IT it ;

      it = pHttpCon->_requestQuery.find( pKey ) ;
      if ( it == pHttpCon->_requestQuery.end() )
      {
         *ppValue = NULL ;
      }
      else if( NULL == it->second )
      {
         *ppValue = REST_STRING_EMPTY ;
      }
      else
      {
         *ppValue = it->second ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_GETHEADER, "restAdaptor::getHttpHeader" )
   INT32 restAdaptor::getHttpHeader( pmdRestSession *pSession,
                                     const CHAR *pKey,
                                     const CHAR **ppValue )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      SDB_ASSERT ( pKey, "pKey is NULL" ) ;
      SDB_ASSERT ( ppValue, "ppValue is NULL" ) ;
      PD_TRACE_ENTRY( SDB__RESTADP_GETHEADER ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      COLNAME_MAP_IT it ;

      it = pHttpCon->_requestHeaders.find( pKey ) ;
      if ( it == pHttpCon->_requestHeaders.end() )
      {
         *ppValue = NULL ;
      }
      else
      {
         *ppValue = it->second ;
      }
      PD_TRACE_EXITRC( SDB__RESTADP_GETHEADER, rc ) ;
      return rc ;
   }

   const CHAR *restAdaptor::getRequestHeader( pmdRestSession *pSession )
   {
      httpConnection *pHttpCon = pSession->getRestConn() ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      return pHttpCon->_pSourceHeaderBuf ;
   }

   INT32 restAdaptor::getRequestHeaderSize( pmdRestSession *pSession )
   {
      httpConnection *pHttpCon = pSession->getRestConn() ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      return pHttpCon->_headerSize ;
   }

   const CHAR *restAdaptor::getRequestBody( pmdRestSession *pSession )
   {
      httpConnection *pHttpCon = pSession->getRestConn() ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      return pHttpCon->_pBodyBuf ;
   }

   INT32 restAdaptor::getRequestBodySize( pmdRestSession *pSession )
   {
      httpConnection *pHttpCon = pSession->getRestConn() ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      return pHttpCon->_bodySize ;
   }

   BOOLEAN restAdaptor::isKeepAlive( pmdRestSession *pSession )
   {
      BOOLEAN isKeepAlive = FALSE ;
      const CHAR *pValue = NULL ;

      getHttpHeader( pSession, REST_STRING_CONNECTION, &pValue ) ;
      if( pValue &&
          ossStrncasecmp( pValue, REST_STRING_KEEP_ALIVE,
                          sizeof( REST_STRING_KEEP_ALIVE ) - 1 ) == 0 )
      {
         isKeepAlive = TRUE ;
      }

      return isKeepAlive ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_SENDHTTPHEADER, "restAdaptor::_sendHttpHeader" )
   INT32 restAdaptor::_sendHttpHeader( pmdRestSession *pSession,
                                       HTTP_RESPONSE_CODE rspCode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_SENDHTTPHEADER ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      CHAR CRLF[3] = { REST_STRING_CR, REST_STRING_LF, 0 } ;
      COLNAME_MAP_IT it ;

      rc = pSession->sendData( REST_FUN_STRING( REST_STRING_HTTP ),
                               _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
      rc = pSession->sendData( REST_FUN_STRING( responseHeader[ rspCode ] ),
                               _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
      rc = pSession->sendData( REST_FUN_STRING( CRLF ),
                               _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }

      for( it = pHttpCon->_responseHeaders.begin();
            it != pHttpCon->_responseHeaders.end(); ++it )
      {
         rc = pSession->sendData( REST_FUN_STRING( it->first ),
                                  _timeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
         rc = pSession->sendData( REST_FUN_STRING( REST_STRING_COLON ),
                                  _timeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
         rc = pSession->sendData( REST_FUN_STRING( it->second ),
                                  _timeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
         rc = pSession->sendData( REST_FUN_STRING( CRLF ),
                                  _timeout ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
            goto error ;
         }
      }
      rc = pSession->sendData( REST_FUN_STRING( CRLF ),
                               _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_SENDHTTPHEADER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 restAdaptor::_setResponseType( pmdRestSession *pSession )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      const CHAR *pFileType = NULL ;

      if( COM_GETFILE == pHttpCon->_common )
      {
         switch( pHttpCon->_fileType )
         {
         case HTTP_FILE_PNG:
            pFileType = REST_STRING_TEXT_PNG ;
            break ;
         case HTTP_FILE_BMP:
            pFileType = REST_STRING_TEXT_BMP ;
            break ;
         case HTTP_FILE_JPG:
            pFileType = REST_STRING_TEXT_JPG ;
            break ;
         case HTTP_FILE_GIF:
            pFileType = REST_STRING_TEXT_GIF ;
            break ;
         case HTTP_FILE_SVG:
            pFileType = REST_STRING_TEXT_SVG ;
            break ;
         case HTTP_FILE_WOFF:
            pFileType = REST_STRING_TEXT_WOFF ;
            break ;
         case HTTP_FILE_EOT:
            pFileType = REST_STRING_TEXT_EOT ;
            break ;
         case HTTP_FILE_OTF:
         case HTTP_FILE_TTF:
            pFileType = REST_STRING_TEXT_OCTET_STREAM ;
            break ;
         case HTTP_FILE_JS:
            pFileType = REST_STRING_TEXT_JS ;
            break ;
         case HTTP_FILE_CSS:
            pFileType = REST_STRING_TEXT_CSS ;
            break ;
         case HTTP_FILE_HTML:
         case HTTP_FILE_DEFAULT:
         case HTTP_FILE_UNKNOW:
         default:
            pFileType = REST_STRING_TEXT_HTML ;
            break ;
         }
         rc = appendHttpHeader( pSession, REST_STRING_CONTENT_TYPE,
                                pFileType ) ;
         if( rc )
         {
            PD_LOG ( PDERROR, "Failed to call append http header, rc=%d",
                     rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 restAdaptor::_sendHttpChunk( pmdRestSession *pSession,
                                      const CHAR *pBuffer,
                                      INT32 length )
   {
      INT32 rc = SDB_OK ;
      CHAR CRLF[3] = { REST_STRING_CR, REST_STRING_LF, 0 } ;
      CHAR chunkSize[255] = { 0 } ;

      ossSnprintf( chunkSize, 255, "%x\r\n", length ) ;
      rc = pSession->sendData( REST_FUN_STRING( chunkSize ), _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
      rc = pSession->sendData( pBuffer, length, _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
      rc = pSession->sendData( REST_FUN_STRING( CRLF ), _timeout ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send data, rc=%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_APPENDBODY, "restAdaptor::appendHttpBody" )
   INT32 restAdaptor::appendHttpBody( pmdRestSession *pSession,
                                      const CHAR *pBuffer,
                                      INT32 length,
                                      INT32 number, BOOLEAN isObjBuffer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RESTADP_APPENDBODY ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      SDB_ASSERT ( pBuffer, "pBuffer is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      httpResponse httpRe ;

      if( TRUE == pHttpCon->_isChunk )
      {
         if( FALSE == pHttpCon->_isSendHttpHeader )
         {
            pHttpCon->_isSendHttpHeader = TRUE ;
            rc = _setResponseType( pSession ) ;
            if( rc )
            {
               PD_LOG ( PDERROR, "Failed to set respone type, rc=%d",
                        rc ) ;
               goto error ;
            }
            rc = _sendHttpHeader( pSession, HTTP_OK ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to send http header, rc=%d", rc ) ;
               goto error ;
            }
         }

         if( isObjBuffer )
         {
            INT32 jsonSize = 0 ;
            BSONObj record ;
            std::string str ;
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
               jsonSize = ossStrlen( str.c_str() ) ;
               rc = _sendHttpChunk( pSession, str.c_str(), jsonSize ) ;
               if( rc )
               {
                  PD_LOG ( PDERROR, "Failed to send http chunk, rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
         else
         {
            rc = _sendHttpChunk( pSession, pBuffer, length ) ;
            if( rc )
            {
               PD_LOG ( PDERROR, "Failed to send http chunk, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         if( isObjBuffer )
         {
            CHAR *pJson = NULL ;
            INT32 jsonSize = 0 ;
            BSONObj record ;
            std::string str ;
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
               jsonSize = ossStrlen( str.c_str() ) ;
               rc = pSession->allocBuff( jsonSize + 1, &pJson, NULL ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                           jsonSize + 1, rc ) ;
                  goto error ;
               }
               ossMemcpy( pJson, str.c_str(), jsonSize ) ;
               pJson[ jsonSize ] = 0 ;
               pHttpCon->_responseSize += jsonSize ;
               httpRe.pBuffer = pJson ;
               httpRe.len = jsonSize ;
               pHttpCon->_responseBody.push_back( httpRe ) ;
               pBuffer = NULL ;
               jsonSize = 0 ;
            }
         }
         else
         {
            CHAR *pFileText = NULL ;
            rc = pSession->allocBuff( length + 1, &pFileText, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to allocate %d bytes memory, rc=%d",
                        length + 1, rc ) ;
               goto error ;
            }
            ossMemcpy( pFileText, pBuffer, length ) ;
            pFileText[ length ] = 0 ;
            pHttpCon->_firstRecordSize = length ;
            httpRe.pBuffer = pFileText ;
            httpRe.len = length ;
            pHttpCon->_responseBody[0] = httpRe ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RESTADP_APPENDBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 restAdaptor::setChunkModal( pmdRestSession *pSession )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      rc = appendHttpHeader( pSession,
                             REST_STRING_TRANSFER,
                             REST_STRING_CHUNKED ) ;
      if( rc )
      {
         PD_LOG ( PDERROR, "Failed to set chunk modal, rc=%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   HTTP_FILE_TYPE restAdaptor::getFileType( pmdRestSession *pSession )
   {
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      return pHttpCon->_fileType ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_GETQUERY, "restAdaptor::getQuery" )
   void restAdaptor::getQuery( pmdRestSession *pSession,
                               const CHAR *pKey,
                               const CHAR **ppValue )
   {
      PD_TRACE_ENTRY( SDB__RESTADP_GETQUERY ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      SDB_ASSERT ( pKey, "pKey is NULL" ) ;
      SDB_ASSERT ( ppValue, "ppValue is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      _getQuery( pHttpCon, pKey, ppValue ) ;
      PD_TRACE_EXIT( SDB__RESTADP_GETQUERY ) ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__RESTADP_CLEARHTTPBODY, "restAdaptor::clearHtttpBody" )
   void restAdaptor::clearHtttpBody( pmdRestSession *pSession )
   {
      PD_TRACE_ENTRY( SDB__RESTADP_CLEARHTTPBODY ) ;
      SDB_ASSERT ( pSession, "pSession is NULL" ) ;
      httpConnection *pHttpCon = pSession->getRestConn() ;
      httpResponse httpRe ;
      pHttpCon->_responseBody.clear() ;
      httpRe.pBuffer = REST_RESULT_STRING_OK ;
      httpRe.len = sizeof( REST_RESULT_STRING_OK ) - 1 ;
      pHttpCon->_responseBody.push_back( httpRe ) ;
      PD_TRACE_EXIT( SDB__RESTADP_CLEARHTTPBODY ) ;
   }
}
