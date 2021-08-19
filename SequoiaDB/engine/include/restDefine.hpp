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
#include <map>
#include <vector>
#include <string>
#include "ossUtil.h"

enum HTTP_PARSE_COMMAND
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

enum HTTP_DATA_TYPE
{
   HTTP_FILE_HTML = 0,
   HTTP_FILE_JSON,
   HTTP_FILE_JS,
   HTTP_FILE_CSS,
   HTTP_FILE_PNG,
   HTTP_FILE_BMP,
   HTTP_FILE_JPG,
   HTTP_FILE_GIF,
   HTTP_FILE_SVG,
   HTTP_FILE_ICON,
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

/* struct */
struct cmp_str
{
   bool operator() ( const std::string &a, const std::string &b )
   {
      int aLen = a.length() ;
      int bLen = b.length() ;

      return ossStrncasecmp( a.c_str(), b.c_str(),
                             aLen > bLen ? aLen : bLen ) < 0 ;
   }
} ;

typedef std::map<const std::string, std::string, cmp_str> REST_COLNAME_MAP ;
#if defined(_WINDOWS)
typedef REST_COLNAME_MAP::iterator REST_COLNAME_MAP_IT ;
#else
typedef std::map<const std::string, std::string>::iterator REST_COLNAME_MAP_IT ;
#endif

struct httpResponse
{
   INT32 len ;
   const CHAR *pBuffer ;
} ;


//struct httpConnection
//{
/* request */

   //key size
   //INT32 _tempKeyLen ;

   //value size
   //INT32 _tempValueLen ;

   // \r\n number
   //INT32 _CRLFNum ;

   //http header buffer size
   //INT32 _headerSize ;

   //http body buffer size
   //INT32 _bodySize ;

   //recv temp a part of the body size
   //INT32 _partSize ;

   //temp query size
   //INT32 _querySize ;

/* response */

   //response first record size
   //INT32 _firstRecordSize ;
   //response body size
   //INT32 _responseSize ;

   //chunk model
   //BOOLEAN _isChunk ;

   //is send http header(chunk model)
   //BOOLEAN _isSendHttpHeader ;

/* request */

   //flag is parser key or value, true: key, false: value
   //BOOLEAN _isKey ;

   //client send common type
   //HTTP_PARSE_COMMON _common ;

   //get file's type
   //HTTP_FILE_TYPE _fileType ;

   //source header buffer
   //CHAR *_pSourceHeaderBuf ;

   //recv header buffer
   //CHAR *_pHeaderBuf ;

   //recv temp a part of the body
   //CHAR *_pPartBody ;

   //recv body buffer
   //CHAR *_pBodyBuf ;

   //send buffer
   //CHAR *_pSendBuffer ;

   //temp key buffer
   //CHAR *_pTempKey ;

   //temp value buffer ;
   //CHAR *_pTempValue ;

   //temp query
   //CHAR *_pQuery ;

   //path
   //const CHAR *_pPath ;

   //http parser
   //http_parser _httpParser ;

   //header list
   //COLNAME_MAP _headerList ;

   //query list
   //COLNAME_MAP _queryList ;

/* response */

   //std::map<const CHAR *,const CHAR *, cmp_str> _responseHeaders ;

   //std::vector<httpResponse> _responseBody ;

/* public */

   //httpConnection() : //_tempKeyLen(0),
                      //_tempValueLen(0),
                      //_CRLFNum(0),
                      //_headerSize(0),
                      //_bodySize(0),
                      //_partSize(0),
                      //_querySize(0),
                      //_firstRecordSize(0),
                      //_responseSize(0),
                      //_isChunk(FALSE),
                      //_isSendHttpHeader(FALSE),
                      //_isKey(TRUE),
                      //_common(COM_CMD),
                      //_fileType(HTTP_FILE_DEFAULT),
                      //_pSourceHeaderBuf(NULL),
                      //_pHeaderBuf(NULL),
                      //_pPartBody(NULL),
                      //_pBodyBuf(NULL),
                      //_pSendBuffer(NULL),
                      //_pTempKey(NULL),
                      //_pTempValue(NULL),
                      //_pQuery(NULL),
                      //_pPath(NULL)
//   {
 //     _httpParser.data = this ;
 //  }

//} ;

#endif // REST_DEFINE_HPP__
