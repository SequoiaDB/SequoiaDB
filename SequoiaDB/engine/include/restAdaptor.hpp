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

   Source File Name = restAdaptor.hpp

   Descriptive Name =

   When/how to use: parse rest

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/28/2014  JWH Initial Draft
          05/03/2015  JWH Refactor

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
#include "restDefine.hpp"
#include "http_parser.hpp"
#include <map>

//recv and send timeout
#define REST_TIMEOUT ( 30 * OSS_ONE_SEC )

namespace engine
{
   enum SDB_REST_TYPE
   {
      SDB_REST_REQUEST  = 0,
      SDB_REST_RESPONSE = 1,

      SDB_REST_UNKNOW
   } ;

   #define SDB_REST_HEADER_MAX_SIZE 4096
   #define SDB_REST_BODY_MAX_SIZE   (64 * 1024 * 1024)

   class restBase ;
   class restRequest ;
   class restResponse ;

   class restAdaptor : public SDBObject
   {
   public:
      restAdaptor() ;

      ~restAdaptor() ;

      void init( INT32 maxHttpHeaderSize = SDB_REST_HEADER_MAX_SIZE,
                 INT32 maxHttpBodySize = SDB_REST_BODY_MAX_SIZE,
                 INT32 timeout = REST_TIMEOUT ) ;

      INT32 recvHeader( ossSocket *sock, restBase *pRest ) ;

      INT32 recvBody( ossSocket *sock, restBase *pRest ) ;

      INT32 sendRest( ossSocket *sock, restBase *pRest ) ;

      INT32 sendHeader( ossSocket *sock, restBase *pRest ) ;

      INT32 sendBody( ossSocket *sock, restBase *pRest ) ;

      INT32 sendChunk( ossSocket *sock, const CHAR *pBuffer, INT32 length,
                       INT32 number = 0, BOOLEAN isObjBuffer = TRUE ) ;

      INT32 setResBody( ossSocket *sock, restResponse *response,
                        const CHAR *pBuffer, INT32 length,
                        INT32 number = 0, BOOLEAN isObjBuffer = TRUE ) ;

      INT32 setResBodyEnd( ossSocket *sock, restResponse *response ) ;

   protected:
      INT32 _sendBodyWithChunk( ossSocket *sock, restResponse *response ) ;

      BOOLEAN _isEndOfHeader( restBase *pRest, CHAR *buffer,
                              INT32 size, INT32 &offset ) ;

      BOOLEAN _isEndOfChunk( restBase *pRest, CHAR *buffer,
                             INT32 size ) ;

      INT32 _recvRestBody( ossSocket *sock, restBase *pRest ) ;

      INT32 _recvRestChunk( ossSocket *sock, restBase *pRest ) ;

      INT32 _recvRestIdentity( ossSocket *sock, restBase *pRest ) ;

      INT32 _sendChunkData( ossSocket *sock, const CHAR *pBuffer,
                            INT32 length ) ;

      INT32 _sendData( ossSocket *sock, const CHAR* pData, INT32 size,
                       BOOLEAN block = TRUE, INT32 *pSentLen = NULL ) ;

      INT32 _recvData( ossSocket *sock, CHAR * pData, INT32 size,
                       BOOLEAN block = TRUE, INT32 *pRecvLen = NULL ) ;

   protected:
      INT32 _maxHttpHeaderSize ;
      INT32 _maxHttpBodySize ;
      INT32 _timeout ;
   } ;

   class restBase : public SDBObject
   {
   public:
      friend class restAdaptor ;

   public:
      restBase( IExecutor *cb ) ;

      virtual ~restBase() ;

      virtual INT32 init() = 0 ;

      virtual SDB_REST_TYPE type() = 0 ;

      CHAR *headerBuf(){ return _headerBuf ; } ;
      INT32 getHeaderSize(){ return _headerSize ; }

      CHAR *bodyBuf(){ return _bodyBuf ; } ;
      INT32 getBodySize(){ return _bodySize ; }

      void forceWrite(){ _write = TRUE; }

      /* read */
      string getHeader( const string &key ) ;

      BOOLEAN isHeaderExist( const string &key ) ;

      HTTP_DATA_TYPE getDataType() ;

      BOOLEAN isKeepAlive() ;

      /* write */
      virtual void setDataType( HTTP_DATA_TYPE type = HTTP_FILE_HTML ) = 0 ;

      virtual void putHeader( const string &key, string value ) ;

      virtual INT32 appendBody( const CHAR *pBuffer, INT32 length,
                                INT32 number = 0,
                                BOOLEAN isObjBuffer = TRUE ) ;

   private:
      restBase( const restBase & ) ;
      restBase &operator=( const restBase & ) ;

   protected:
      INT32 _malloc( INT32 size, CHAR **buffer ) ;

      void _releaseBuff( CHAR *buffer ) ;

      /* read */
      virtual string _generateHeader() = 0 ;

      /* write */
      static INT32 on_message_begin( void *pData ) ;

      static INT32 on_message_complete( void *pData ) ;

      static INT32 on_header_field( void *pData, const CHAR* at,
                                    size_t length ) ;

      static INT32 on_header_value( void *pData, const CHAR* at,
                                    size_t length ) ;

      static INT32 on_headers_complete( void *pData ) ;

      static INT32 on_body( void *pData, const CHAR* at, size_t length ) ;

   protected:
      //request data type or response data type
      HTTP_DATA_TYPE _dataType ;

      //tmp key is set value
      BOOLEAN        _isSetKey ;

      //tmp value is set value
      BOOLEAN        _isSetVal ;

      //write permission
      BOOLEAN        _write ;

      //http end of header sign
      INT32          _CRLFNum ;

      //chunk end of body sign
      INT32          _ZCRLF ;

      //http header size
      INT32          _headerSize ;

      //http body size
      INT32          _bodySize ;

      //http body buffer size
      INT32          _bodyBufSize ;

      /*
      if recv header and part of body,
      _tmpBodyBuf will point to part of body
      */
      INT32          _tmpBodySize ;

      //http header buffer
      CHAR          *_headerBuf ;

      //http body buffer
      CHAR          *_bodyBuf ;

      /*
      if recv header and part of body,
      _tmpBodyBuf will point to part of body
      */
      CHAR          *_tmpBodyBuf ;

      //executor
      IExecutor     *_cb ;

      //http parser callback
      http_parser_settings _settings ;

      //http header
      REST_COLNAME_MAP    _headerList ;

      //http parser object
      http_parser    _httpParser ;

      //tmp key & tmp value
      string         _tmpKey ;
      string         _tmpVal ;

      //rest body
      std::vector<string> _bodyContent ;
   } ;

   class restRequest : public restBase
   {
   public:
      friend class restAdaptor ;

   public:
      restRequest( IExecutor *cb ) ;

      virtual ~restRequest() ;

      INT32 init() ;

      SDB_REST_TYPE type(){ return SDB_REST_REQUEST ; }

      void buildResponse( restResponse &response ) ;

      /* read */
      HTTP_PARSE_COMMAND getCommand() ;

      string getRequestPath() ;

      string getQuery( const string &key ) ;

      BOOLEAN isQueryArgExist( const string &key ) ;

      /* write */
      void setUrlPath( string path ) ;

      void setDataType( HTTP_DATA_TYPE type = HTTP_FILE_HTML ) ;

   private:
      static INT32 on_url( void *pData, const CHAR* at, size_t length ) ;
      
      INT32 _parse_http_query( const CHAR *pBuffer, INT32 length ) ;

      virtual string _generateHeader() ;

      void _parseDataType() ;

      OSS_INLINE const CHAR *_getResourceFileName( const CHAR *pPath ) ;

      OSS_INLINE const CHAR *_getFileExtension( const CHAR *pFileName ) ;

   private:
      //request method
      UINT32 _method ;

      //request command
      HTTP_PARSE_COMMAND _command ;

      //request query
      REST_COLNAME_MAP _queryList ;

      //request path
      string _path ;
   } ;

   class restResponse : public restBase
   {
   public:
      friend class restAdaptor ;

   public:
      restResponse( IExecutor *cb ) ;

      virtual ~restResponse() ;

      INT32 init() ;

      SDB_REST_TYPE type(){ return SDB_REST_RESPONSE ; }

      /* read */
      BOOLEAN isChunkModal(){ return _isChunk ; }

      /* write */
      void setDataType( HTTP_DATA_TYPE type = HTTP_FILE_HTML ) ;

      void putHeader( const string &key, string value ) ;

      INT32 appendBody( const CHAR *pBuffer, INT32 length,
                        INT32 number = 0, BOOLEAN isObjBuffer = TRUE ) ;

      void setResponse( HTTP_RESPONSE_CODE rspCode ) ;

      void setOPResult( INT32 result, const BSONObj &info ) ;

      void setKeepAlive() ;

      void setConnectionClose() ;

      void setChunkModal() ;

   private:
      virtual string _generateHeader() ;

   private:
      BOOLEAN _isChunk ;
      HTTP_RESPONSE_CODE _rspCode ;
   } ;
}

#endif // RESTADAPTOR_HPP__
