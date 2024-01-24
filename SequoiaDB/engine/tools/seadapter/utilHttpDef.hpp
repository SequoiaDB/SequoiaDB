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

   Source File Name = utilHttp.hpp

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
#ifndef UTIL_HTTP_DEF_HPP_
#define UTIL_HTTP_DEF_HPP_

// Maximum size of the http header and body.
// Note: HTTP_MAX_HEADER_SIZE should be defined before including http_parser.hpp
//       with exactly this name. It will be used in http_parser.
//       If not defined here, the default value of 80k defined in http_parser
//       itself will be used.
#define HTTP_MAX_HEADER_SIZE        ( 64 * 1024 )
#define HTTP_MAX_BODY_SIZE          ( 64 * 1024 * 1024 - HTTP_MAX_HEADER_SIZE )
#define HTTP_CHUNK_SIZE             1024

#define _TEXT_PLAIN              "text/plain"
#define _APPLICATION_JSON        "application/json"
#define _APPLICATION_URLENCODED  "application/x-www-form-urlencoded"

#define HTTP_URI_PREFIX          "http://"
#define HTTP_LINE_END_STR        "\r\n"
#define HTTP_BLOCK_END_STR       "\r\n\r\n"
#define HTTP_BLOCK_END_STR_LEN   ossStrlen( HTTP_BLOCK_END_STR )
#define HTTP_URI_PREFIX_LEN      ossStrlen( HTTP_URI_PREFIX )
#define HTTP_DEFAULT_PORT        80
#define HTTP_REQ_TIMEOUT         ( 10 * 1000 )
#define HTTP_REQ_HEAD_STR        "HEAD"
#define HTTP_REQ_GET_STR         "GET"
#define HTTP_REQ_PUT_STR         "PUT"
#define HTTP_REQ_POST_STR        "POST"
#define HTTP_REQ_DELETE_STR      "DELETE"

// Default send and receive buffer size: 1MB.
#undef   HTTP_DEF_BUF_SIZE
#define  HTTP_DEF_BUF_SIZE        ( 1024 * 1024 )

#define HTTP_OPRATION_TIMEOUT    10000

#include "http_parser.hpp"
#include "restDefine.hpp"

typedef std::map<const CHAR *,const CHAR *, cmp_str> COLNAME_MAP ;
#if defined(_WINDOWS)
typedef COLNAME_MAP::iterator COLNAME_MAP_IT ;
#else
typedef std::map<const CHAR *,const CHAR *>::iterator COLNAME_MAP_IT ;
#endif

struct httpConnection
{
/* request */

   //key size
   INT32 _tempKeyLen ;
   //value size
   INT32 _tempValueLen ;
   // \r\n number
   INT32 _CRLFNum ;
   //http header buffer size
   INT32 _headerSize ;
   //http body buffer size
   INT32 _bodySize ;
   //recv temp a part of the body size
   INT32 _partSize ;
   //temp query size
   INT32 _querySize ;

/* response */

   //response first record size
   INT32 _firstRecordSize ;
   //response body size
   INT32 _responseSize ;
   //chunk model
   BOOLEAN _isChunk ;
   //is send http header(chunk model)
   BOOLEAN _isSendHttpHeader ;

/* request */

   //flag is parser key or value, true: key, false: value
   BOOLEAN _isKey ;
   //client send common type
   HTTP_PARSE_COMMAND _common ;
   //get file's type
   HTTP_DATA_TYPE _fileType ;
   //source header buffer
   CHAR *_pSourceHeaderBuf ;
   //recv header buffer
   CHAR *_pHeaderBuf ;
   //recv temp a part of the body
   CHAR *_pPartBody ;
   //recv body buffer
   CHAR *_pBodyBuf ;
   //send buffer
   CHAR *_pSendBuffer ;
   //temp key buffer
   CHAR *_pTempKey ;
   //temp value buffer ;
   CHAR *_pTempValue ;
   //temp query
   CHAR *_pQuery ;
   //path
   const CHAR *_pPath ;

   //http parser
   http_parser _httpParser ;
   //header list
   COLNAME_MAP _requestHeaders ;
   //query list
   COLNAME_MAP _requestQuery ;

/* response */

   std::map<const CHAR *,const CHAR *, cmp_str> _responseHeaders ;
   std::vector<httpResponse> _responseBody ;

/* public */

   httpConnection() : _tempKeyLen(0),
                      _tempValueLen(0),
                      _CRLFNum(0),
                      _headerSize(0),
                      _bodySize(0),
                      _partSize(0),
                      _querySize(0),
                      _firstRecordSize(0),
                      _responseSize(0),
                      _isChunk(FALSE),
                      _isSendHttpHeader(FALSE),
                      _isKey(TRUE),
                      _common(COM_CMD),
                      _fileType(HTTP_FILE_DEFAULT),
                      _pSourceHeaderBuf(NULL),
                      _pHeaderBuf(NULL),
                      _pPartBody(NULL),
                      _pBodyBuf(NULL),
                      _pSendBuffer(NULL),
                      _pTempKey(NULL),
                      _pTempValue(NULL),
                      _pQuery(NULL),
                      _pPath(NULL)
   {
      _httpParser.data = this ;
   }

} ;

#endif /* UTIL_HTTP_DEF_HPP_ */

