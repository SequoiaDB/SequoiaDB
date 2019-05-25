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

   Source File Name = restdefine.h

   Descriptive Name =

   When/how to use: parse Jsons util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2014  JW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REST_DEFINE_HPP__
#define REST_DEFINE_HPP__

#include "core.hpp"
#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__) && (!defined(_MSC_VER) || _MSC_VER<1600)
#include <BaseTsd.h>
#include <stddef.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif
#include <map>
#include <vector>
#include "ossUtil.h"

enum HTTP_PARSE_COMMON
{
   COM_CMD = 0,
   COM_GETFILE
} ;

enum HTTP_RESPONSE_CODE
{
   HTTP_OK = 0,                  /* 200 ok */
   HTTP_FOUND,                   /* 302 found */
   HTTP_BADREQ,                  /* 400 bad request */
   HTTP_NOTFOUND,                /* 404 not found */
   HTTP_SERVICUNAVA,             /* 503 service unavailable */
   HTTP_VERSION                  /* 505 http version not supported */
} ;

enum HTTP_FILE_TYPE
{
   HTTP_FILE_HTML = 0,
   HTTP_FILE_JS,
   HTTP_FILE_CSS,
   HTTP_FILE_PNG,
   HTTP_FILE_BMP,
   HTTP_FILE_JPG,
   HTTP_FILE_GIF,
   HTTP_FILE_SVG,
   HTTP_FILE_WOFF,
   HTTP_FILE_EOT,
   HTTP_FILE_OTF,
   HTTP_FILE_TTF,
   HTTP_FILE_JSP,
   HTTP_FILE_PHP,
   HTTP_FILE_ASP,
   HTTP_FILE_DEFAULT,        /* default file */
   HTTP_FILE_UNKNOW
} ;

struct http_parser {
  /** PRIVATE **/
  unsigned int type : 2;         /* enum http_parser_type */
  unsigned int flags : 6;        /* F_* values from 'flags' enum; semi-public */
  unsigned int state : 8;        /* enum state from http_parser.c */
  unsigned int header_state : 8; /* enum header_state from http_parser.c */
  unsigned int index : 8;        /* index into current matcher */

  uint32_t nread;          /* # bytes read in various scenarios */
  uint64_t content_length; /* # bytes in body (0 if no Content-Length header) */

  /** READ-ONLY **/
  unsigned short http_major;
  unsigned short http_minor;
  unsigned int status_code : 16; /* responses only */
  unsigned int method : 8;       /* requests only */
  unsigned int http_errno : 7;

  /* 1 = Upgrade header was present and the parser has exited because of that.
   * 0 = No upgrade header present.
   * Should be checked when http_parser_execute() returns in addition to
   * error checking.
   */
  unsigned int upgrade : 1;

  /** PUBLIC **/
  void *data; /* A pointer to get hook to the "connection" or "socket" object */
};

/* struct */
struct cmp_str
{
   bool operator() ( const char *a, const char *b )
   {
      int aLen = ossStrlen( a ) ;
      int bLen = ossStrlen( b ) ;
      return ossStrncasecmp( a, b, aLen > bLen ? aLen : bLen ) < 0 ;
   }
} ;

typedef std::map<const CHAR *,const CHAR *, cmp_str> COLNAME_MAP ;
#if defined(_WINDOWS)
typedef COLNAME_MAP::iterator COLNAME_MAP_IT ;
#else
typedef std::map<const CHAR *,const CHAR *>::iterator COLNAME_MAP_IT ;
#endif

struct httpResponse
{
   INT32 len ;
   const CHAR *pBuffer ;
} ;

struct httpConnection
{
/* request */

   INT32 _tempKeyLen ;
   INT32 _tempValueLen ;
   INT32 _CRLFNum ;
   INT32 _headerSize ;
   INT32 _bodySize ;
   INT32 _partSize ;
   INT32 _querySize ;

/* response */

   INT32 _firstRecordSize ;
   INT32 _responseSize ;
   BOOLEAN _isChunk ;
   BOOLEAN _isSendHttpHeader ;

/* request */

   BOOLEAN _isKey ;
   HTTP_PARSE_COMMON _common ;
   HTTP_FILE_TYPE _fileType ;
   CHAR *_pSourceHeaderBuf ;
   CHAR *_pHeaderBuf ;
   CHAR *_pPartBody ;
   CHAR *_pBodyBuf ;
   CHAR *_pSendBuffer ;
   CHAR *_pTempKey ;
   CHAR *_pTempValue ;
   CHAR *_pQuery ;
   const CHAR *_pPath ;

   http_parser _httpParser ;
   COLNAME_MAP _requestHeaders ;
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

#endif // REST_DEFINE_HPP__
