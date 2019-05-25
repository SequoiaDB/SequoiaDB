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

#define HTTP_DEF_BUF_SIZE        ( 1024 * 1024 )

#endif /* UTIL_HTTP_DEF_HPP_ */

