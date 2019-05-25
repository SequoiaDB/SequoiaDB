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
#ifndef UTIL_HTTP_HPP_
#define UTIL_HTTP_HPP_

#include "ossSocket.hpp"

#include "utilHttpDef.hpp"
#include "http_parser.hpp"
#include <string>

using std::string ;

namespace seadapter
{
   typedef UINT32 HTTP_STATUS_CODE ;
   enum HTTP_STATUS
   {
      HTTP_OK         = 200,
      HTTP_CREATED    = 201,
      HTTP_FOUND      = 302,
      HTTP_BAD_REQ    = 400,
      HTTP_FORBID     = 403,
      HTTP_NOT_FOUND  = 404,
      HTTP_SVC_UNAVAL = 503
   } ;

   class _utilHttp : public SDBObject
   {
      public:
         _utilHttp() ;
         ~_utilHttp() ;

         INT32 init( const string &uri, BOOLEAN keepAlive = TRUE ) ;
         void reset() ;

         OSS_INLINE BOOLEAN isConnected() const
         {
            return ( _socket && _socket->isConnected() ) ;
         }

         OSS_INLINE INT32 reconnect()
         {
            INT32 rc = SDB_OK ;

            if ( !_socket )
            {
               PD_LOG( PDEVENT, "Not initialized yet" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = _connectBySocket() ;
            PD_RC_CHECK( rc, PDERROR, "Create connection by socket "
                         "failed[ %d ]", rc ) ;
         done:
            return rc ;
         error:
            goto done ;
         }

         INT32 request( const CHAR *method,
                        const CHAR *endUrl,
                        const CHAR *data = NULL,
                        HTTP_STATUS_CODE *statusCode = NULL,
                        const CHAR **ppReply = NULL,
                        INT32 *replyLen = NULL,
                        const CHAR *contentType = _APPLICATION_JSON ) ;

         OSS_INLINE INT32 head( const CHAR *endUrl, const CHAR *data,
                                HTTP_STATUS_CODE *statusCode = NULL,
                                const CHAR **ppReply = NULL,
                                INT32 *replyLen = NULL )
         {
            return request( HTTP_REQ_HEAD_STR, endUrl, data,
                            statusCode, ppReply, replyLen ) ;
         }

         OSS_INLINE INT32 get( const CHAR *endUrl, const CHAR *data,
                               HTTP_STATUS_CODE *statusCode = NULL,
                               const CHAR **ppReply = NULL,
                               INT32 *replyLen = NULL )
         {
            return request( HTTP_REQ_GET_STR, endUrl, data,
                            statusCode, ppReply, replyLen ) ;
         }

         OSS_INLINE INT32 put( const CHAR* endUrl, const CHAR* data,
                               HTTP_STATUS_CODE *statusCode = NULL,
                               const CHAR **ppReply = NULL,
                               INT32 *replyLen = NULL )
         {
            return request( HTTP_REQ_PUT_STR, endUrl, data,
                            statusCode, ppReply, replyLen ) ;
         }

         OSS_INLINE INT32 post( const CHAR* endUrl, const CHAR* data,
                                HTTP_STATUS_CODE *statusCode = NULL,
                                const CHAR **ppReply = NULL,
                                INT32 *replyLen = NULL )
         {
            return request( HTTP_REQ_POST_STR, endUrl, data,
                            statusCode, ppReply, replyLen ) ;
         }

         OSS_INLINE INT32 remove( const CHAR *endUrl, const CHAR *data,
                                  HTTP_STATUS_CODE *statusCode = NULL,
                                  const CHAR **ppReply = NULL,
                                  INT32 *replyLen = NULL )
         {
            return request( HTTP_REQ_DELETE_STR, endUrl, data,
                            statusCode, ppReply, replyLen ) ;
         }

      private:
         INT32 _parseUri( const string &uri ) ;
         INT32 _connectBySocket() ;
         INT32 _connect( BOOLEAN newSock = FALSE ) ;
         void  _disconnect() ;
         BOOLEAN _validMethod( const CHAR *method ) ;
         void _buildReqStr( const CHAR *method, const CHAR *endUrl,
                            const CHAR *data, const CHAR *content_type,
                            string &requestStr, BOOLEAN &chunked  ) ;
         INT32 _sendMessage( const CHAR *method, const CHAR *endUrl,
                             const CHAR *data, const CHAR *content_type ) ;
         INT32 _send( const CHAR *request, INT32 reqSize ) ;
         INT32 _sendInChunkMode( string &header, const CHAR *data ) ;
         INT32 _chkStatusCode( HTTP_STATUS_CODE statusCode ) ;
         INT32 _getReply( HTTP_STATUS_CODE *statusCode,
                          const CHAR **reply, INT32 *replyLen,
                          BOOLEAN onlyHead = FALSE ) ;

         void _paraInit( httpConnection *pHttpCon ) ;
         http_parser* _getHttpParser() { return &(_connection._httpParser) ; }

         BOOLEAN _checkEndOfHeader( const CHAR *buff, UINT32 bufSize,
                                    INT32 &bodyOffset ) ;
         INT32 _parseHeader( CHAR *buff, INT32 len ) ;

         static INT32 _onMessageBegin( void *data ) ;
         static INT32 _onUrl( void *data, const CHAR* at, size_t length ) ;
         static INT32 _onStatus( void *data, const CHAR* at, size_t length ) ;
         static INT32 _onHeaderField( void *data, const CHAR* at, size_t length ) ;
         static INT32 _onHeaderValue( void *data, const CHAR* at, size_t length ) ;
         static INT32 _onHeaderComplete( void *data ) ;
         static INT32 _onBody( void *data, const CHAR* at, size_t length ) ;
         static INT32 _onMessageComplete( void *data ) ;


         size_t appendChunk(string& output, CHAR* msg, size_t msgSize);

         /*
         inline bool mustReconnect() const
         {
            return (_keepAliveTimeout <= time(NULL) - _lastRequest);
         }
         */

         INT32 _extendRecvBuff() ;
         const CHAR* _getHeaderItemVal( const CHAR *key ) ;

   private:
      BOOLEAN        _init ;
      BOOLEAN        _keepAlive ;
      string         _url ;       // Include the address and port.
      string         _urn ;
      INT32          _port ;
      ossSocket      *_socket ;
      CHAR           *_sendBuf ;
      UINT32         _sendBufSize ;
      CHAR           *_recvBuf ;
      UINT32         _recvBufSize ;

      void           *_parserSetting ;
      httpConnection _connection ;
   };
   typedef _utilHttp utilHttp ;
}
#endif /* UTIL_HTTP_HPP_ */

