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

   Source File Name = utilESClt.hpp

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
#ifndef UTIL_ESCLT_HPP_
#define UTIL_ESCLT_HPP_

#include <string>
#include <sstream>
#include <list>
#include <vector>

#include "oss.hpp"
#include "utilHttp.hpp"
#include "cJSON.h"
#include "../bson/bsonobj.h"
#include "../util/fromjson.hpp"
#include "utilESClt.hpp"
#include "utilCommObjBuff.hpp"

#define UTIL_SE_MAX_URL_SIZE              2048
#define UTIL_ES_DFT_SCROLL_SIZE           1000
#define UTIL_SE_MAX_TYPE_SZ               255
#define UTIL_SE_BULK_DFT_FILTERPATH       "filter_path=errors,took,items.index._id,items.index.status"
using std::string ;

namespace seadapter
{
   class _utilESClt : public SDBObject
   {
      public:
         _utilESClt();
         ~_utilESClt();

         INT32 init( const string &uri, BOOLEAN readOnly = FALSE ) ;
         BOOLEAN isActive() ;
         INT32 active() ;
         INT32 getSEInfo( BSONObj &infoObj ) ;
         INT32 indexExist( const CHAR *index, BOOLEAN &exist ) ;
         INT32 createIndex( const CHAR *index, const CHAR *data = NULL ) ;
         INT32 dropIndex( const CHAR *index ) ;
         INT32 indexDocument( const CHAR *index, const CHAR *type,
                              const CHAR *id, const CHAR *jsonData ) ;
         INT32 updateDocument( const CHAR *index, const CHAR *type,
                               const CHAR *id, const CHAR *newData ) ;
         INT32 deleteDocument( const CHAR *index, const CHAR *type,
                               const CHAR *id ) ;

         INT32 getDocument( const CHAR *index, const CHAR *type, const CHAR *id,
                            BSONObj &result, BOOLEAN withMeta = TRUE ) ;

         INT32 getDocument( const CHAR *index, const CHAR *type,
                            const CHAR *key, const CHAR *value,
                            utilCommObjBuff &objBuff, BOOLEAN withMeta = TRUE ) ;

         INT32 getDocument( const CHAR *index, const CHAR *type,
                            const CHAR *query, utilCommObjBuff &objBuff,
                            BOOLEAN withMeta = TRUE ) ;

         INT32 documentExist( const CHAR *index, const CHAR *type,
                              const CHAR *id, BOOLEAN &exist ) ;
         INT32 documentExist( const CHAR *index, const CHAR *type,
                              const CHAR *key, const CHAR *val,
                              BOOLEAN &exist ) ;
         INT32 deleteAllByType( const CHAR *index, const CHAR *type ) ;
         INT32 getDocCount( const CHAR *index, const CHAR *type,
                            UINT64 &count ) ;
         INT32 refresh( const CHAR *index ) ;

         INT32 initScroll( string& scrollId,
                           const CHAR* index,
                           const CHAR* type,
                           const string& query,
                           utilCommObjBuff &result,
                           int scrollSize = UTIL_ES_DFT_SCROLL_SIZE,
                           const CHAR *filterPath = NULL ) ;
         INT32 scrollNext( string& scrollId, utilCommObjBuff &result,
                           const CHAR *filterPath = NULL ) ;
         void clearScroll( const string& scrollId ) ;

         INT32 bulk( const CHAR *index, const CHAR *type, const CHAR *data,
                     const CHAR *filterPath = UTIL_SE_BULK_DFT_FILTERPATH) ;

         const CHAR* getLastErrMsg() const ;

      private:
         OSS_INLINE INT32 _processReply( INT32 returnCode, const CHAR *reply,
                                         INT32 replyLen, BSONObj &resultObj,
                                         BOOLEAN transform = TRUE ) ;
         INT32 _getResultObjs( const BSONObj &replyObj,
                               utilCommObjBuff &resultObjs ) ;

         INT32 _searchByUri( const CHAR *index, const CHAR *type,
                             const string &query,
                             string &result, UINT64 *totalNum ) ;
         INT32 _searchByDSL( const CHAR *index, const CHAR *type,
                             const string &query,
                             const CHAR **ppReply, INT32 *replyLen ) ;
         void _getHitNum( const BSONObj &fullResult, INT64 &hitNum ) ;
         INT32 _removeDocMeta( const BSONObj &fullObj, BSONObj &newObj ) ;

      private:
         utilHttp    _http;
         BOOLEAN     _readOnly;
         const CHAR *_errMsg ;
   };
   typedef _utilESClt utilESClt ;

   OSS_INLINE INT32 _utilESClt::_processReply( INT32 returnCode,
                                               const CHAR *reply,
                                               INT32 replyLen,
                                               BSONObj &resultObj,
                                               BOOLEAN transform )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK == returnCode )
      {
         if ( _errMsg )
         {
            _errMsg = NULL ;
         }
         if ( transform && reply && replyLen > 0 )
         {
            rc = fromjson( reply, resultObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Convert respond to BSONObj "
                         "failed[ %d ], respond: %s",
                         rc, string( reply, replyLen ).c_str() ) ;
         }
      }
      else
      {
         rc = returnCode ;
         _errMsg = reply ;
         if ( reply && replyLen > 0 )
         {
            PD_LOG( PDERROR, "Request processed failed[ %d ], respond "
                    "message: %s", rc, string( reply, replyLen ).c_str() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Request processed failed[ %d ]", rc ) ;
         }

         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

#endif /* UTIL_ESCLT_HPP_ */

