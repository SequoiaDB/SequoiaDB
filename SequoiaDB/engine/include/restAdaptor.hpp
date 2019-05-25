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

   Source File Name = restAdaptor.hpp

   Descriptive Name =

   When/how to use: parse rest

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/28/2014  JWH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RESTADAPTOR_HPP__
#define RESTADAPTOR_HPP__
#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "msgMessage.hpp"
#include "pmdRestSession.hpp"
#include "restDefine.hpp"
#include <map>

#define REST_TIMEOUT             ( 30 * OSS_ONE_SEC )

namespace engine
{
   class restAdaptor : public SDBObject
   {
   private:
      INT32 _maxHttpHeaderSize ;
      INT32 _maxHttpBodySize ;
      INT32 _timeout ;
      void *_pSettings ;
   private:
      static INT32 on_message_begin( void *pData ) ;
      static INT32 on_headers_complete( void *pData ) ;
      static INT32 on_message_complete( void *pData ) ;
      static INT32 on_url( void *pData, const CHAR* at, size_t length ) ;
      static INT32 on_header_field( void *pData, const CHAR* at,
                                    size_t length ) ;
      static INT32 on_header_value( void *pData, const CHAR* at,
                                    size_t length ) ;
      static INT32 on_body( void *pData, const CHAR* at,
                            size_t length ) ;
      static INT32 _parse_http_query( httpConnection *pHttpConnection,
                                      CHAR *pBuffer, INT32 length,
                                      CHAR *pDecodeBuff, INT32 decodeLen ) ;
      OSS_INLINE const CHAR *_getResourceFileName( const CHAR *pPath ) ;
      OSS_INLINE const CHAR *_getFileExtension( const CHAR *pFileName ) ;
      OSS_INLINE BOOLEAN _checkEndOfHeader( httpConnection *pHttpCon,
                                            CHAR *pBuffer,
                                            INT32 bufferSize,
                                            INT32 &bodyOffset ) ;
      INT32 _convertMsg( pmdRestSession *pSession,
                         HTTP_PARSE_COMMON &common,
                         CHAR **ppMsg,
                         INT32 &msgSize ) ;
      void _getQuery( httpConnection *pHttpCon,
                      const CHAR *pKey,
                      const CHAR **ppValue ) ;
      void _paraInit( httpConnection *pHttpCon ) ;
      INT32 _sendHttpHeader( pmdRestSession *pSession,
                             HTTP_RESPONSE_CODE rspCode ) ;
      INT32 _sendHttpChunk( pmdRestSession *pSession,
                            const CHAR *pBuffer,
                            INT32 length ) ;
      INT32 _setResponseType( pmdRestSession *pSession ) ;
   public:
      restAdaptor() ;
      ~restAdaptor() ;
      INT32 init( INT32 maxHttpHeaderSize,
                  INT32 maxHttpBodySize,
                  INT32 timeout = REST_TIMEOUT ) ;

      INT32 recvRequestHeader( pmdRestSession *pSession ) ;
      INT32 recvRequestBody( pmdRestSession *pSession,
                             HTTP_PARSE_COMMON &common,
                             CHAR **ppPath,
                             INT32 &pathSize ) ;
      INT32 setOPResult( pmdRestSession *pSession,
                         INT32 result,
                         const BSONObj &info ) ;
      INT32 sendResponse( pmdRestSession *pSession,
                          HTTP_RESPONSE_CODE rspCode ) ;

      INT32 appendHttpHeader( pmdRestSession *pSession,
                              const CHAR *pKey,
                              const CHAR *pValue ) ;
      INT32 getHttpHeader( pmdRestSession *pSession,
                           const CHAR *pKey,
                           const CHAR **ppValue ) ;
      const CHAR *getRequestHeader( pmdRestSession *pSession ) ;
      INT32 getRequestHeaderSize( pmdRestSession *pSession ) ;
      const CHAR *getRequestBody( pmdRestSession *pSession ) ;
      INT32 getRequestBodySize( pmdRestSession *pSession ) ;
      BOOLEAN isKeepAlive( pmdRestSession *pSession ) ;
      void getQuery( pmdRestSession *pSession,
                     const CHAR *pKey,
                     const CHAR **ppValue ) ;
      INT32 appendHttpBody( pmdRestSession *pSession,
                            const CHAR *pBuffer,
                            INT32 length,
                            INT32 number = 0,
                            BOOLEAN isObjBuffer = TRUE ) ;
      INT32 setChunkModal( pmdRestSession *pSession ) ;
      HTTP_FILE_TYPE getFileType( pmdRestSession *pSession ) ;
      void clearHtttpBody( pmdRestSession *pSession ) ;
   } ;
}

#endif // RESTADAPTOR_HPP__
