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

   Source File Name = utilESClt.cpp

   Descriptive Name = Elasticsearch client.

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
#include "utilESClt.hpp"

#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

#include "pd.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

#define ES_HITS_KEY              "_hits"
#define ES_TOTAL_KEY             "total"
#define ES_SCROLL_ID_KEY         "_scroll_id"
#define ES_SOURCE_KEY            "_source"
#define ES_ERROR_FIELD_NAME      "errors"

#define ES_CLT_ARG_CHK1( index )                         \
do                                                       \
{                                                        \
   if ( !(index) )                                       \
   {                                                     \
      PD_LOG( PDERROR, "Index name is NULL" ) ;          \
      rc = SDB_INVALIDARG ;                              \
      goto error ;                                       \
   }                                                     \
}                                                        \
while ( 0 ) ;

#define ES_CLT_ARG_CHK2( index, type )                   \
do                                                       \
{                                                        \
   ES_CLT_ARG_CHK1( index ) ;                            \
   if ( !(type) )                                        \
   {                                                     \
      PD_LOG( PDERROR, "Type name is NULL" ) ;           \
      rc = SDB_INVALIDARG ;                              \
      goto error ;                                       \
   }                                                     \
}                                                        \
while ( 0 ) ;

#define ES_CLT_ARG_CHK3( index, type, id )               \
do                                                       \
{                                                        \
   ES_CLT_ARG_CHK2( index, type ) ;                      \
   if ( !(id) )                                          \
   {                                                     \
      PD_LOG( PDERROR, "Id is NULL" ) ;                  \
      rc = SDB_INVALIDARG ;                              \
      goto error ;                                       \
   }                                                     \
}                                                        \
while ( 0 ) ;

#define ES_CLT_ARG_CHK4( index, type, id, data )         \
do                                                       \
{                                                        \
   ES_CLT_ARG_CHK3( index, type, id ) ;                  \
   if ( !(id) )                                          \
   {                                                     \
      PD_LOG( PDERROR, "Data is NULL" ) ;                \
      rc = SDB_INVALIDARG ;                              \
      goto error ;                                       \
   }                                                     \
}                                                        \
while ( 0 ) ;

namespace seadapter
{
   _utilESClt::_utilESClt()
   : _readOnly( FALSE ),
     _errMsg( NULL )
   {
   }

   _utilESClt::~_utilESClt()
   {
   }

   INT32 _utilESClt::init( const string &uri, BOOLEAN readOnly, INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      if ( 0 == uri.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Search engine service address should not be empty, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      rc = _http.init( uri, TRUE, timeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init http connection with node: %s, "
                 "rc: %d", uri.c_str(), rc ) ;
         goto error ;
      }
      _readOnly = readOnly ;

   done:
      return rc  ;
   error:
      goto done ;
   }

   BOOLEAN _utilESClt::isActive()
   {
      return ( SDB_OK == _http.get( NULL, NULL ) ) ? TRUE : FALSE ;
   }

   INT32 _utilESClt::active()
   {
      INT32 rc = SDB_OK ;

      rc = _http.reconnect() ;
      PD_RC_CHECK( rc, PDERROR, "Activate client failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::getSEInfo( BSONObj &infoObj )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;

      rc = _http.get( NULL, NULL, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, infoObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::indexExist( const CHAR *index, BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      BSONObj replyInfo ;

      ES_CLT_ARG_CHK1( index ) ;

      rc = _http.head( index, NULL, &status, &reply, &replyLen ) ;
      if ( SDB_OK == rc )
      {
         exist = TRUE ;
         goto done ;
      }
      else if ( HTTP_NOT_FOUND == status )
      {
         exist = FALSE ;
         rc = SDB_OK ;
         goto done ;
      }

      rc = _processReply( rc, reply, replyLen, replyInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::createIndex( const CHAR *index, const CHAR *data )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      BSONObj replyInfo ;

      ES_CLT_ARG_CHK1( index ) ;

      rc = _http.put( index, data, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, replyInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::dropIndex( const CHAR *index )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      BSONObj replyInfo ;

      ES_CLT_ARG_CHK1( index ) ;

      rc = _http.remove( index, NULL, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, replyInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::indexDocument( const CHAR *index, const CHAR *type,
                                    const CHAR *id, const CHAR *jsonData )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj bsonObj ;
      HTTP_STATUS_CODE status = 0 ;
      CHAR url[ UTIL_SE_MAX_URL_SIZE ] = { 0 } ;

      ES_CLT_ARG_CHK4( index, type, id, jsonData ) ;

      if ( strlen( index ) + strlen( type ) + strlen( id ) + 2
           > UTIL_SE_MAX_URL_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Url length is too long, max: %d, actual: %d",
                 UTIL_SE_MAX_URL_SIZE,
                 strlen( index ) + strlen( type ) + strlen( id ) + 2 ) ;
         goto error ;
      }

      ossSnprintf( url, UTIL_SE_MAX_URL_SIZE, "%s/%s/%s", index, type, id ) ;
      rc = _http.put( url, jsonData, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, bsonObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::updateDocument( const CHAR *index, const CHAR *type,
                                     const CHAR *id, const CHAR *newData )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj bsonObj ;
      HTTP_STATUS_CODE status = 0 ;
      CHAR url[ UTIL_SE_MAX_URL_SIZE ] = { 0 } ;

      ES_CLT_ARG_CHK4( index, type, id, newData ) ;

      if ( strlen( index ) + strlen( type ) + strlen( id ) + 2
           > UTIL_SE_MAX_URL_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Url length is too long, max: %d, actual: %d",
                 UTIL_SE_MAX_URL_SIZE,
                 strlen( index ) + strlen( type ) + strlen( id ) + 2 ) ;
         goto error ;
      }

      ossSnprintf( url, UTIL_SE_MAX_URL_SIZE, "%s/%s/%s", index, type, id ) ;
      rc = _http.put( url, newData, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, bsonObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::deleteDocument( const CHAR *index, const CHAR *type,
                                     const CHAR *id )
   {
      INT32 rc = SDB_OK ;
      std::string result ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      BSONObj resultObj ;
      std::ostringstream oss ;

      ES_CLT_ARG_CHK3( index, type, id ) ;

      SDB_ASSERT( index && type && id, "Invalid arguments for deleteDocument" ) ;

      oss << index << "/" << type << "/" << id ;

      rc = _http.remove( oss.str().c_str(), NULL, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, resultObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::getDocument( const CHAR *index, const CHAR *type,
                                  const CHAR *id, BSONObj &result,
                                  BOOLEAN withMeta )
   {
      INT32 rc = SDB_OK ;
      BSONObj fullObj ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      std::ostringstream oss ;
      utilCommObjBuff objBuff ;

      ES_CLT_ARG_CHK3( index, type, id ) ;

      oss << index << "/" << type << "/" << id ;
      rc = _http.get( oss.str().c_str(), NULL, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, fullObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

      if ( withMeta )
      {
         result = fullObj ;
      }
      else
      {
         rc = _removeDocMeta( fullObj, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Remove document meta failed[ %d ]", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::getDocument( const CHAR *index, const CHAR *type,
                                  const CHAR *key, const CHAR *value,
                                  utilCommObjBuff &objBuff, BOOLEAN withMeta )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      std::ostringstream endUrl ;
      std::ostringstream query ;
      BSONObj fullObj ;

      ES_CLT_ARG_CHK4( index, type, key, value ) ;

      endUrl << index << "/" << type << "/_search" ;
      query << "{\"query\":{\"match\":{\"" << key << "\":\""
            << value << "\"}}}" ;

      rc = objBuff.reset() ;
      PD_RC_CHECK( rc, PDERROR, "Reset object buffer failed[ %d ]", rc ) ;

      rc = _http.get( endUrl.str().c_str(), query.str().c_str(),
                      &status, &reply, &replyLen ) ;
      if ( withMeta )
      {
         rc = _processReply( rc, reply, replyLen, fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;
         rc = objBuff.appendObj( fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Append object to buffer failed[ %d ]", rc ) ;
      }
      else
      {
         rc = _processReply( rc, reply,replyLen, fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

         rc = _getResultObjs( fullObj, objBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Get result objects failed[ %d ]" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::getDocument( const CHAR *index, const CHAR *type,
                                  const CHAR *query, utilCommObjBuff &objBuff,
                                  BOOLEAN withMeta )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      HTTP_STATUS_CODE status = 0 ;
      std::ostringstream endUrl ;
      BSONObj fullObj ;

      ES_CLT_ARG_CHK2( index, type ) ;

      if ( !query )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Query condition is NULL" ) ;
         goto error ;
      }

      endUrl << index << "/" << type << "/_search" ;

      rc = objBuff.reset() ;
      PD_RC_CHECK( rc, PDERROR, "Reset object buffer failed[ %d ]", rc ) ;

      rc = _http.get( endUrl.str().c_str(), query, &status, &reply, &replyLen ) ;
      if ( withMeta )
      {
         rc = _processReply( rc, reply, replyLen, fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;
         rc = objBuff.appendObj( fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Append object to buffer failed[ %d ]", rc ) ;
      }
      else
      {
         rc = _processReply( rc, reply,replyLen, fullObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

         rc = _getResultObjs( fullObj, objBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Get result objects failed[ %d ]" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::getDocCount( const CHAR *index, const CHAR *type,
                                  UINT64 &count )
   {
      INT32 rc = SDB_OK ;
      std::ostringstream oss ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj bsonObj ;
      HTTP_STATUS_CODE status = 0 ;

      ES_CLT_ARG_CHK2( index, type ) ;

      oss << index << "/" << type << "/_count" ;

      rc = _http.get( oss.str().c_str(), NULL, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, bsonObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

      count = bsonObj.getIntField( "count" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::documentExist( const CHAR *index, const CHAR *type,
                                    const CHAR *id, BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      HTTP_STATUS_CODE status = 0 ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      std::ostringstream oss ;
      BSONObj result ;

      ES_CLT_ARG_CHK3( index, type, id ) ;

      oss << index << "/" << type << "/" << id ;
      rc = _http.head( oss.str().c_str(), NULL, &status, &reply, &replyLen ) ;
      if ( SDB_OK == rc )
      {
         exist = ( HTTP_OK == status ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         exist = FALSE ;
         rc = SDB_OK ;
         goto done ;
      }
      else
      {
         PD_LOG( PDERROR, "HEAD request[ %s ] failed[ %d ]", oss.str().c_str(),
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::documentExist( const CHAR *index, const CHAR *type,
                                    const CHAR *key, const CHAR *value,
                                    BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      std::ostringstream url ;
      std::ostringstream query ;
      HTTP_STATUS_CODE status = 0 ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj result ;
      INT64 hitNum = 0 ;

      ES_CLT_ARG_CHK4( index, type, key, val ) ;

      url << index << "/" << type ;
      query << "{\"query\":{\"match\":{\"" << key << "\":\""
            << value << "\"}}}" ;

      rc = _http.get( url.str().c_str(), query.str().c_str(), &status,
                      &reply, &replyLen ) ;
      if ( rc )
      {
         exist = FALSE ;
      }
      else
      {
         rc = _processReply( rc, reply, replyLen, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;
         _getHitNum( result, hitNum ) ;
         exist = ( hitNum >= 1 ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::deleteAllByType( const CHAR *index, const CHAR *type )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj resultObj ;
      HTTP_STATUS_CODE status = 0 ;
      std::ostringstream uri, data ;

      ES_CLT_ARG_CHK2( index, type ) ;

      uri << index << "/" << type << "/_delete_by_query" ;
      data << "{\"query\":{\"match_all\":{}}}" ;

      rc = _http.post( uri.str().c_str(), data.str().c_str(),
                       &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, resultObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::refresh( const CHAR *index )
   {
      INT32 rc = SDB_OK ;
      BSONObj bsonObj ;
      std::string result ;
      std::ostringstream oss ;

      ES_CLT_ARG_CHK1( index ) ;

      oss << index << "/_refresh" ;
      rc = _http.get( oss.str().c_str(), NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Process refresh request failed[ %d ]" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::initScroll( std::string& scrollId,
                                 const CHAR* index,
                                 const CHAR* type,
                                 const std::string& query,
                                 utilCommObjBuff &result,
                                 int scrollSize,
                                 const CHAR *filterPath )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj replyObj ;
      HTTP_STATUS_CODE status = 0 ;
      std::ostringstream oss;

      ES_CLT_ARG_CHK2( index, type ) ;

      oss << index << "/" << type << "/_search?scroll=1m&size="
         << scrollSize;
      if ( filterPath )
      {
         oss << "&" << filterPath ;
      }

      rc = result.reset() ;
      PD_RC_CHECK( rc, PDERROR, "Reset object buffer failed[ %d ]", rc ) ;

      rc = _http.post( oss.str().c_str(), query.c_str(), &status,
                       &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, replyObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;
      PD_LOG( PDDEBUG, "Reply for init scroll: %s",
              replyObj.toString().c_str() ) ;

      rc = _getResultObjs( replyObj, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Get result objects from reply failed[ %d ]",
                   rc ) ;

      scrollId = string( replyObj.getStringField( ES_SCROLL_ID_KEY ) ) ;
      PD_LOG( PDDEBUG, "scroll id returned: %s", scrollId.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESClt::scrollNext( std::string& scrollId, utilCommObjBuff &result,
                                 const CHAR *filterPath )
   {
      INT32 rc = SDB_OK ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj replyObj ;
      HTTP_STATUS_CODE status = 0 ;
      string endUrl = "_search/scroll?scroll=1m" ;
      string scrollIdStr = "{ \"scroll_id\" : \"" + scrollId + "\" }" ;

      if ( filterPath )
      {
         endUrl = endUrl + "&" + filterPath ;
      }

      rc = _http.post( endUrl.c_str(), scrollIdStr.c_str(),
                       &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, replyObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

      rc = _getResultObjs( replyObj, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Get objects from reply failed[ %d ]", rc ) ;

      scrollId = string( replyObj.getStringField( ES_SCROLL_ID_KEY ) ) ;
      PD_LOG( PDDEBUG, "scroll id returned: %s", scrollId.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESClt::clearScroll( const std::string& scrollId )
   {
      string endUrl = "_search/scroll/" + scrollId ;
      _http.remove( endUrl.c_str(), NULL, NULL, NULL, NULL );
   }

   INT32 _utilESClt::bulk( const CHAR *index, const CHAR *type,
                           const CHAR *data, const CHAR *filterPath )
   {
      INT32 rc = SDB_OK ;
      string endUrl ;
      const CHAR *reply = NULL ;
      INT32 replyLen = 0 ;
      BSONObj replyObj ;
      HTTP_STATUS_CODE status = 0 ;

      if ( !data )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "No valid data for _bulk operation" ) ;
         goto error ;
      }

      if ( index )
      {
         endUrl = string( index ) ;
         if ( type )
         {
            endUrl += string( "/" ) + type ;
         }
      }
      endUrl += "/_bulk" ;
      if ( filterPath )
      {
         endUrl = endUrl + "?" + filterPath ;
      }

      rc = _http.post( endUrl.c_str(), data, &status, &reply, &replyLen ) ;
      rc = _processReply( rc, reply, replyLen, replyObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Process request reply failed[ %d ]", rc ) ;

      try
      {
         if ( replyObj.getBoolField( ES_ERROR_FIELD_NAME ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Operation on remote failed. Reply: %s", reply ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _utilESClt::getLastErrMsg() const
   {
      return _errMsg ;
   }

   INT32 _utilESClt::_getResultObjs( const BSONObj &replyObj,
                                     utilCommObjBuff &resultObjs )
   {
      INT32 rc = SDB_OK ;
      BSONElement sourceObj ;

      PD_RC_CHECK( rc, PDERROR, "Initialize result buffer failed[ %d ]", rc ) ;

      try
      {
         PD_LOG( PDDEBUG, "Result object: %s", replyObj.toString( FALSE, TRUE ).c_str() ) ;
         {
            BSONElement hitsObj = replyObj.getField( "hits" ) ;
            if ( Object == hitsObj.type() )
            {
               BSONObj hitsObj2 = hitsObj.embeddedObject() ;
               BSONElement hitsEle2 = hitsObj2.getField( "hits" ) ;
               if ( Array == hitsEle2.type() )
               {
                  vector<BSONElement> hitObjs = hitsEle2.Array() ;
                  for ( vector<BSONElement>::iterator itr = hitObjs.begin();
                        itr != hitObjs.end(); ++itr )
                  {
                     BSONObj sourceObj = itr->embeddedObject() ;
                     rc = resultObjs.appendObj( sourceObj ) ;
                     PD_RC_CHECK( rc, PDERROR, "Append object failed[ %d ]",
                                  rc ) ;
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESClt::_getHitNum( const BSONObj &fullResult, INT64 &hitNum )
   {
      BSONObj hitsObj = fullResult.getObjectField( ES_HITS_KEY ) ;
      BSONElement totalEle = hitsObj.getField( ES_TOTAL_KEY ) ;
      if ( NumberInt == totalEle.type() ||
           NumberLong == totalEle.type() )
      {
         hitNum = totalEle.numberLong() ;
      }
      else
      {
         hitNum = 0 ;
      }
   }

   INT32 _utilESClt::_removeDocMeta( const BSONObj &fullObj, BSONObj &newObj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele = fullObj.getField( ES_SOURCE_KEY ) ;
         newObj = ele.Obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

