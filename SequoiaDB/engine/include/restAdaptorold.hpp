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

   Source File Name = restAdptorold.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RESTADAPTOROLD_HPP__
#define RESTADAPTOROLD_HPP__
#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include "msg.hpp"
#include "../bson/bson.h"
#include "msgMessage.hpp"

#define REST_ADAPTOR_NUMBER_3 3
#define REST_ADAPTOR_NUMBER_4 4
#define REST_ADAPTOR_NUMBER_5 5
#define REST_ADAPTOR_NUMBER_6 6
#define REST_ADAPTOR_NUMBER_7 7
#define REST_ADAPTOR_NUMBER_8 8
#define REST_ADAPTOR_NUMBER_9 9
#define REST_ADAPTOR_NUMBER_14 14
#define REST_ADAPTOR_STR_KEY              "key"
#define REST_ADAPTOR_STR_KEY_LEN                      REST_ADAPTOR_NUMBER_3
#define REST_ADAPTOR_STR_INDEXNAME        "indexname"
#define REST_ADAPTOR_STR_INDEXNAME_LEN                REST_ADAPTOR_NUMBER_9
#define REST_ADAPTOR_STR_UNIQUE           "unique"
#define REST_ADAPTOR_STR_UNIQUE_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_CLNAME           "clname"
#define REST_ADAPTOR_STR_CLNAME_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_CSNAME           "csname"
#define REST_ADAPTOR_STR_CSNAME_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_SNAPSHOT         "snapshot"
#define REST_ADAPTOR_STR_SNAPSHOT_LEN                 REST_ADAPTOR_NUMBER_8
#define REST_ADAPTOR_STR_CREATE           "create"
#define REST_ADAPTOR_STR_CREATE_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_DROP             "drop"
#define REST_ADAPTOR_STR_DROP_LEN                     REST_ADAPTOR_NUMBER_4
#define REST_ADAPTOR_STR_SESSION          "session"
#define REST_ADAPTOR_STR_SESSION_LEN                  REST_ADAPTOR_NUMBER_7
#define REST_ADAPTOR_STR_CONTEXT          "context"
#define REST_ADAPTOR_STR_CONTEXT_LEN                  REST_ADAPTOR_NUMBER_7
#define REST_ADAPTOR_STR_QUERY            "query"
#define REST_ADAPTOR_STR_QUERY_LEN                    REST_ADAPTOR_NUMBER_5
#define REST_ADAPTOR_STR_SELECTOR         "selector"
#define REST_ADAPTOR_STR_SELECTOR_LEN                 REST_ADAPTOR_NUMBER_8
#define REST_ADAPTOR_STR_ORDERBY          "orderby"
#define REST_ADAPTOR_STR_ORDERBY_LEN                  REST_ADAPTOR_NUMBER_7
#define REST_ADAPTOR_STR_SKIPROWS         "skiprows"
#define REST_ADAPTOR_STR_SKIPROWS_LEN                 REST_ADAPTOR_NUMBER_8
#define REST_ADAPTOR_STR_LIMIT            "limit"
#define REST_ADAPTOR_STR_LIMIT_LEN                    REST_ADAPTOR_NUMBER_5
#define REST_ADAPTOR_STR_UPDATE           "update"
#define REST_ADAPTOR_STR_UPDATE_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_CONDITION        "condition"
#define REST_ADAPTOR_STR_CONDITION_LEN                REST_ADAPTOR_NUMBER_9
#define REST_ADAPTOR_STR_RULE             "rule"
#define REST_ADAPTOR_STR_RULE_LEN                     REST_ADAPTOR_NUMBER_4
#define REST_ADAPTOR_STR_HINT             "hint"
#define REST_ADAPTOR_STR_HINT_LEN                     REST_ADAPTOR_NUMBER_4
#define REST_ADAPTOR_STR_INSERT           "insert"
#define REST_ADAPTOR_STR_INSERT_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_RECORD           "record"
#define REST_ADAPTOR_STR_RECORD_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_DELETE           "delete"
#define REST_ADAPTOR_STR_DELETE_LEN                   REST_ADAPTOR_NUMBER_6
#define REST_ADAPTOR_STR_COUNT            "count"
#define REST_ADAPTOR_STR_COUNT_LEN                    REST_ADAPTOR_NUMBER_5
#define REST_ADAPTOR_STR_HTTP             "HTTP"
#define REST_ADAPTOR_STR_HTTP_LEN                     REST_ADAPTOR_NUMBER_4
#define REST_ADAPTOR_STR_CONLEN           "Content-Length"
#define REST_ADAPTOR_STR_CONLEN_LEN                   REST_ADAPTOR_NUMBER_14
#define REST_ADAPTOR_STR_POST_LEN         REST_ADAPTOR_NUMBER_4
#define REST_ADAPTOR_STR_GET_LEN          REST_ADAPTOR_NUMBER_3
#define REST_ADAPTOR_STR_POINT            '.'
#define REST_ADAPTOR_STR_QUESTION         '?'
#define REST_ADAPTOR_STR_COLON            ':'
#define REST_ADAPTOR_STR_EQUALSIGN        '='
#define REST_ADAPTOR_STR_AMPERSAND        '&'
#define REST_ADAPTOR_STR_SLASH            '/'
#define REST_ADAPTOR_RECV_SPACE           32
#define REST_ADAPTOR_RECV_TABLE           9
#define REST_ADAPTOR_RECV_BUFF_SIZE       4096
#define REST_ADAPTOR_RECV_POST            1
#define REST_ADAPTOR_RECV_GET             2
#define REST_ADAPTOR_RECV_HEAD_SIZE       4096
#define REST_ADAPTOR_RECV_BODY_SIZE       4096
#define REST_ADAPTOR_INDEX_SIZE           128
#define REST_ADAPTOR_NAME_SIZE            256
#define REST_ADAPTOR_MSG_COMMAND          4096
#define REST_PAGESIZE_DEFAULT             4096
#define REST_SOCKET_DFT_TIMEOUT           10000
namespace engine
{
   class _restAdaptoro : public SDBObject
   {
   private :
      CHAR CRLF[3] ;
      CHAR CRLF2[REST_ADAPTOR_NUMBER_5] ;
      CHAR httpHeader [REST_ADAPTOR_RECV_HEAD_SIZE] ;
      CHAR httpBody   [REST_ADAPTOR_RECV_BODY_SIZE] ;
      INT32 _receive ( ossSocket &sock, CHAR *buffer,
                       INT32 &len, INT32 timeout ) ;
      INT32 _httpConvertMsg ( ossSocket &sock,
                              CHAR *pBuffer,
                              CHAR **ppReceiveBuffer,
                              INT32 *receiveBufferSize,
                              BOOLEAN &needFetch ) ;
      CHAR *_strEndTag ( CHAR *in ) ;
      CHAR *_skip( CHAR *in ) ;
      INT32 _skipToParameter( ossSocket &sock,
                              CHAR *buffer,
                              CHAR **temp,
                              INT32 operators,
                              BOOLEAN &isMalloc ) ;
      INT32 _getPostBody( ossSocket &sock,
                          CHAR **buffer,
                          CHAR **temp,
                          BOOLEAN &isMalloc ) ;
      INT32 _getParameter( CHAR **temp, CHAR **parameter, INT32 operators ) ;
      INT32 _runCommand ( CHAR **ppReceiveBuffer,
                          INT32 *receiveBufferSize,
                          CHAR *pSring,
                          INT32 numToReturn = -1,
                          CHAR *arg1 = NULL,
                          CHAR *arg2 = NULL,
                          CHAR *arg3 = NULL,
                          CHAR *arg4 = NULL ) ;
      INT32 _snapshotCommand ( CHAR **ppReceiveBuffer,
                               INT32 *receiveBufferSize ) ;
      INT32 _createCommand ( ossSocket &sock,
                             CHAR **ppReceiveBuffer,
                             INT32 *receiveBufferSize,
                             CHAR *pBuffer,
                             CHAR *pParameterBuf,
                             CHAR *csFullName,
                             INT32 operators ) ;
      INT32 _dropCommand ( ossSocket &sock,
                           CHAR **ppReceiveBuffer,
                           INT32 *receiveBufferSize,
                           CHAR *pBuffer,
                           CHAR *pParameterBuf,
                           CHAR *csFullName,
                           INT32 operators ) ;
      INT32 _sessionCommand ( CHAR **ppReceiveBuffer,
                              INT32 *receiveBufferSize,
                              CHAR *pParameterBuf ) ;
      INT32 _contextCommand ( CHAR **ppReceiveBuffer,
                              INT32 *receiveBufferSize,
                              CHAR *pParameterBuf ) ;
      INT32 _queryCommand ( ossSocket &sock,
                            CHAR **ppReceiveBuffer,
                            INT32 *receiveBufferSize,
                            CHAR *pBuffer,
                            CHAR *pParameterBuf,
                            CHAR *csName,
                            INT32 operators ) ;
      INT32 _updateCommand ( ossSocket &sock,
                             CHAR **ppReceiveBuffer,
                             INT32 *receiveBufferSize,
                             CHAR *pBuffer,
                             CHAR *pParameterBuf,
                             CHAR *csName,
                             INT32 operators ) ;
      INT32 _insertCommand ( ossSocket &sock,
                             CHAR **ppReceiveBuffer,
                             INT32 *receiveBufferSize,
                             CHAR *pBuffer,
                             CHAR *pParameterBuf,
                             CHAR *csName,
                             INT32 operators ) ;
      INT32 _deleteCommand ( ossSocket &sock,
                             CHAR **ppReceiveBuffer,
                             INT32 *receiveBufferSize,
                             CHAR *pBuffer,
                             CHAR *pParameterBuf,
                             CHAR *csName,
                             INT32 operators ) ;
      INT32 _countCommand ( ossSocket &sock,
                            CHAR **ppReceiveBuffer,
                            INT32 *receiveBufferSize,
                            CHAR *pBuffer,
                            CHAR *pParameterBuf,
                            CHAR *csName,
                            INT32 operators ) ;
      INT32 _defaultCommand ( CHAR **ppReceiveBuffer,
                              INT32 *receiveBufferSize,
                              CHAR *csFullName,
                              CHAR *pCsName,
                              CHAR *pClName,
                              CHAR *indexBuf,
                              BOOLEAN &needFetch ) ;
      INT32 _strReplace ( CHAR *pStr ) ;
   public :
      _restAdaptoro () ;
      ~_restAdaptoro () ;
      INT32 getRequest ( ossSocket &sock, CHAR **ppReceiveBuffer,
                         INT32 *receiveBufferSize,
                         BOOLEAN &needFetch ) ;
      INT32 sendReply ( MsgOpReply &replyHeader,
                        const CHAR *pExtraBuffer,
                        INT32 extraSize,
                        ossSocket &sock,
                        BOOLEAN needFetch,
                        BOOLEAN &isSendHeader ) ;
   } ;
   typedef class _restAdaptoro restAdaptoro ;
}


#endif // RESTADAPTOROLD_HPP__
