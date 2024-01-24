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

   Source File Name = seAdptContext.cpp

   Descriptive Name = Context on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptContext.hpp"
#include "seAdptMgr.hpp"
#include "utilESKeywordDef.hpp"
#include "utilESUtil.hpp"

using namespace bson ;

namespace seadapter
{
   _seAdptQueryRebuilder::_seAdptQueryRebuilder()
   {
   }

   _seAdptQueryRebuilder::~_seAdptQueryRebuilder()
   {
   }

   // Initialize the rebuilder with the original message. It will store separate
   // parts of the query.
   INT32 _seAdptQueryRebuilder::init( const BSONObj &matcher,
                                      const BSONObj &selector,
                                      const BSONObj &orderBy,
                                      const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _query = matcher.copy() ;
         _selector = selector.copy() ;
         _orderBy = orderBy.copy() ;
         _hint = hint.copy() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // Rebuild the query message. The vectors of the second and third arguments
   // contain the items to be modified to the original.
   INT32 _seAdptQueryRebuilder::rebuild( REBUILD_ITEM_MAP &rebuildItems,
                                         utilCommObjBuff &objBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         const BSONObj *object ;
         REBUILD_ITEM_MAP_ITR itr ;

         if ( rebuildItems.find( SE_QUERY_REBLD_QUERY ) != rebuildItems.end() )
         {
            object = rebuildItems[SE_QUERY_REBLD_QUERY] ;
         }
         else
         {
            object = &_query ;
         }
         rc = objBuff.appendObj( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Append query to object buffer failed[ %d ]",
                      rc ) ;

         if ( rebuildItems.find( SE_QUERY_REBLD_SEL ) != rebuildItems.end() )
         {
            object = rebuildItems[SE_QUERY_REBLD_SEL] ;
         }
         else
         {
            object = &_selector ;
         }
         rc = objBuff.appendObj( object ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Append selector to object buffer failed[ %d ]", rc ) ;

         if ( rebuildItems.find( SE_QUERY_REBLD_ORD ) != rebuildItems.end() )
         {
            object = rebuildItems[SE_QUERY_REBLD_ORD] ;
         }
         else
         {
            object = &_orderBy ;
         }
         rc = objBuff.appendObj( object ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Append orderby to object buffer failed[ %d ]", rc ) ;

         {
            // The original hint is used to pass the text index name. In the
            // replay, we give an empty hint.
            // Any problem ?
            BSONObj object ;
            rc = objBuff.appendObj( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Append hint to object buffer failed[ %d ]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _seAdptContextBase::_seAdptContextBase()
   : _hitEnd( TRUE )
   {
   }

   _seAdptContextQuery::_seAdptContextQuery()
   : _imContext( NULL ),
     _esFetcher( NULL )
   {
   }

   _seAdptContextQuery::~_seAdptContextQuery()
   {
      if ( _esFetcher )
      {
         SDB_OSS_DEL _esFetcher ;
      }
      if ( _imContext )
      {
         sdbGetSeAdapterCB()->getIdxMetaMgr()->releaseIMContext( _imContext ) ;
      }
   }

   INT32 _seAdptContextQuery::open( const CHAR *clName,
                                    UINT16 indexID,
                                    const BSONObj &matcher,
                                    const BSONObj &selector,
                                    const BSONObj &orderBy,
                                    const BSONObj &hint,
                                    utilCommObjBuff &objBuff,
                                    pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj queryObj ;
      utilCommObjBuff searchResult ;
      BSONObj inCond ;
      BSONObj newQuery ;
      std::map< _seadptQueryRebldType, const BSONObj* > rebuildItems ;
      vector<_seadptQueryRebldType> rebuildType ;
      vector<BSONObj*> rebuildObjs ;
      rtnCondNode *textNode = NULL ;
      BSONObj textRootObj ;
      BSONObj textObj ;
      BOOLEAN validEmptyResult = FALSE ;

      objBuff.reset() ;
      _hitEnd = FALSE ;

      rc = sdbGetSeAdapterCB()->getIdxMetaMgr()->
            getIMContext( &_imContext, indexID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get index meta context for collection[%s] "
                                "failed[%d]", clName, rc ) ;

      // Check if the index meta still belongs to the collection.
      if ( 0 != ossStrcmp( clName, _imContext->meta()->getOrigCLName() ) )
      {
         PD_LOG( PDERROR, "Index for collection[%s] dose not exist", clName ) ;
         rc = SDB_RTN_INDEX_NOTEXIST ;
         goto error ;
      }

      _condTree.parse( matcher ) ;
      textNode = _condTree.getTextNode() ;
      if ( !textNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Text search condition not found: %s",
                 matcher.toString().c_str() ) ;
         goto error ;
      }

      try
      {
         // { "" : { "$Text" : { condition } } }
         textRootObj = textNode->toBson() ;
         PD_LOG( PDDEBUG, "Text query object: %s", textRootObj.toString().c_str() ) ;
         {
            BSONElement eleTmp = textRootObj.firstElement() ;
            if ( Object != eleTmp.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Text search condition is invalid: %s",
                       textRootObj.toString().c_str() ) ;
               goto error ;
            }

            // { "$Text" : { condition } }
            textObj = eleTmp.Obj() ;
            eleTmp = textObj.firstElement() ;
            if ( Object != eleTmp.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Text search condition is invalid: %s",
                       textRootObj.toString().c_str() ) ;
               goto error ;
            }

            // { condition }
            queryObj = eleTmp.Obj() ;
         }

         rc = _queryRebuilder.init(  matcher, selector, orderBy, hint ) ;
         PD_RC_CHECK( rc, PDERROR, "Initialize query rebuilder failed[ %d ]",
                      rc ) ;

         rc = searchResult.init() ;
         PD_RC_CHECK( rc, PDERROR, "Result buffer init failed[ %d ]", rc ) ;

         // If the text index query is inside a $not clause, try to fetch all
         // the documents which match the condition. Because if that is done in
         // more than one round, the result on SDB node will be wrong.
         // Otherwise, fetch one batch of records.
         if ( _condTree.textNodeInNot() )
         {
            rc = _fetchAll( queryObj, searchResult, SEADPT_FETCH_MAX_SIZE ) ;
         }
         else
         {
            rc = _prepareSearch( queryObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare search failed[ %d ]", rc ) ;
            rc = _getMore( searchResult ) ;
         }

         if ( SDB_DMS_EOC == rc &&
              ( _condTree.textNodeInNot() || _condTree.textNodeInOr() ) )
         {
            validEmptyResult = TRUE ;
         }
         else if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Get more result failed[ %d ]", rc ) ;
            }
            // If text condition is descendant of '$or' or '$not' clause, the
            // query should not end.
            goto error ;
         }

         if ( !validEmptyResult && ( 0 == searchResult.getObjNum() ) )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         rc = _buildInCond( searchResult, inCond ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Build the $in condition failed[ %d ]", rc ) ;

         PD_LOG( PDDEBUG, "The new in condition is: %s",
                 inCond.toString().c_str() ) ;

         rc = _condTree.updateNode( textNode, inCond.firstElement() ) ;
         PD_RC_CHECK( rc, PDERROR, "Update condition node failed[ %d ]", rc ) ;

         newQuery = _condTree.toBson() ;
         PD_LOG( PDDEBUG, "After transformation, the query is: %s",
                 newQuery.toString().c_str() ) ;

         rebuildItems[ SE_QUERY_REBLD_QUERY ] = &newQuery ;
         rc = _queryRebuilder.rebuild( rebuildItems, objBuff ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Rebuild the query message failed[ %d ]", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptContextQuery::getMore( INT32 returnNum, utilCommObjBuff &objBuff )
   {
      INT32 rc = SDB_OK ;
      utilCommObjBuff searchResult ;
      BSONObj inCond ;
      REBUILD_ITEM_MAP rebuildItems ;
      rtnCondNode *textNode = NULL ;
      BSONObj newQuery ;

      objBuff.reset() ;

      rc = searchResult.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init result buffer failed[ %d ]", rc ) ;

      rc = _getMore( searchResult ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get more failed[ %d ]", rc ) ;
         }
         goto error ;
      }

      if ( 0 == searchResult.getObjNum() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         rc = _buildInCond( searchResult, inCond ) ;
         PD_RC_CHECK( rc, PDERROR, "Build the $in condition failed[ %d ]", rc ) ;

         textNode = _condTree.getTextNode() ;
         SDB_ASSERT( textNode, "Text node pointer should not be NULL" ) ;
         rc = _condTree.updateNode( textNode, inCond.firstElement() ) ;
         PD_RC_CHECK( rc, PDERROR, "Update condition node failed[ %d ]", rc ) ;

         newQuery = _condTree.toBson() ;
         PD_LOG( PDDEBUG, "After transformation, the query is: %s",
                 newQuery.toString().c_str() ) ;

         rebuildItems[ SE_QUERY_REBLD_QUERY ] = &newQuery ;
         rc = _queryRebuilder.rebuild( rebuildItems, objBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Rebuild the query message failed[ %d ]", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // There are two ways to get data from ES.
   // 1. Using pagination by from + size.
   // 2. Using scrolling.
   // On SDB side, we only provide a query in the find condition. So we should
   // decide by which way we are going to fetch the data.
   // The strategy is as follows:
   // 1. If from or size is speicified in the query, use pagination.
   // 2. Otherwise, do it by scrolling.
   INT32 _seAdptContextQuery::_prepareSearch( const BSONObj &queryCond )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BOOLEAN rangeSet = FALSE ;
         BSONElement eleFrom ;
         BSONElement eleSize ;
         INT64 from = 0 ;
         INT64 size = 0 ;
         eleFrom = queryCond.getField( ES_KEYWORD_FROM ) ;
         if ( EOO != eleFrom.type() )
         {
            if ( !eleFrom.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Value of 'from' in full text search "
                           "condition is not number" ) ;
               goto error ;
            }

            from = eleFrom.numberLong() ;
            rangeSet = TRUE ;
         }

         eleSize = queryCond.getField( ES_KEYWORD_SIZE ) ;
         if ( EOO != eleSize.type() )
         {
            if ( !eleSize.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Value of 'size' in full text search "
                           "condition is not number" ) ;
               goto error ;
            }

            size = eleSize.numberLong() ;
            if ( !rangeSet )
            {
               rangeSet = TRUE ;
            }
         }

         if ( _esFetcher )
         {
            SDB_OSS_DEL _esFetcher ;
            _esFetcher = NULL ;
         }

         rc = _imContext->resume() ;
         PD_RC_CHECK( rc, PDERROR, "Lock index metadata failed[%d]", rc ) ;
         if ( rangeSet )
         {
            _esFetcher = SDB_OSS_NEW
                  utilESPageFetcher( _imContext->meta()->getESIdxName(),
                                     _imContext->meta()->getESTypeName() ) ;
         }
         else
         {
            _esFetcher = SDB_OSS_NEW
                  utilESScrollFetcher( _imContext->meta()->getESIdxName(),
                                       _imContext->meta()->getESTypeName() ) ;
         }
         _imContext->pause() ;

         if ( !_esFetcher )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for ES fetcher failed" ) ;
            goto error ;
         }

         if ( 0 != size )
         {
            _esFetcher->setSize( size ) ;
         }
         if ( 0 != from )
         {
            ((utilESPageFetcher *)_esFetcher)->setFrom( from ) ;
         }

         _esFetcher->setFilterPath( SEADPT_ES_ID_FILTER_PATH ) ;
         rc = _esFetcher->setCondition( queryCond ) ;
         PD_RC_CHECK( rc, PDERROR, "Set ES query condition failed[ %d ]", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( _esFetcher )
      {
         SDB_OSS_DEL _esFetcher ;
         _esFetcher = NULL ;
      }
      goto done ;
   }

   INT32 _seAdptContextQuery::_getMore( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;
      if ( eof() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_esFetcher )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "ES fetcher has not been initialized yet" ) ;
         goto error ;
      }

      // To be sure the index still exists when trying to get more data.
      rc = _imContext->resume() ;
      PD_RC_CHECK( rc, PDERROR, "Lock index metadata failed[%d]", rc ) ;
      rc = _esFetcher->fetch( result ) ;
      _imContext->pause() ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            if ( result.getObjNum() > 0 )
            {
               rc = SDB_OK ;
               goto done ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Fetch data from es failed[ %d ]", rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptContextQuery::_fetchAll( const BSONObj &queryCond,
                                         utilCommObjBuff &result,
                                         UINT32 limitNum )
   {
      INT32 rc = SDB_OK ;

      result.reset() ;

      rc = _prepareSearch( queryCond ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare search failed[ %d ]", rc ) ;
      do
      {
         rc = _getMore( result ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               _hitEnd = TRUE ;
               if ( result.getObjNum() > 0 )
               {
                  rc = SDB_OK ;
                  break ;
               }
               else
               {
                  goto error ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "Get more failed[ %d ]", rc ) ;
               goto error ;
            }
         }

         // Two conditions to terminate this loop:
         // 1. Number exceeds the limit.
         // 2. The result buffer is full.
         if ( result.getObjNum() > limitNum )
         {
            rc = SDB_OSS_UP_TO_LIMIT ;
            PD_LOG( PDERROR, "Record number[%u] too large for the operation",
                    result.getObjNum() ) ;
            goto error ;
         }
      } while ( TRUE ) ;

   done:
      return rc ;
   error:
      result.reset() ;
      goto done ;
   }

   /*
    * Build an $in condition with all the provided _id objects.
    * The _id objects in objBuff are like:
    * { "_id":"1234567890"} { "_id":"0987654321" } { "_id":"567890987654" }
    * Get them one by one and format to an $in clause.
    * The condition should be in the following format:
    * { "_id" : { $in : [ {"$oid" : "1234567890"}, {"$oid" : "0987654321" } ] } }
    */
   INT32 _seAdptContextQuery::_buildInCond( utilCommObjBuff &objBuff,
                                            BSONObj &condition )
   {
      INT32 rc = SDB_OK ;
      const CHAR *idValue = NULL ;
      BSONArrayBuilder arrayBuilder ;
      BSONArray idArray ;

      BSONObj obj ;
      UINT32 bufSize = SEADPT_MAX_ID_SZ + 1 ;
      CHAR idBuff[ SEADPT_MAX_ID_SZ + 1 ] = { 0 } ;

      try
      {
         while ( !objBuff.eof() )
         {
            BSONType type ;
            objBuff.nextObj( obj ) ;
            idValue = obj.getStringField( SEADPT_FIELD_NAME_ID ) ;
            SDB_ASSERT( idValue, "id value should not be NULL" ) ;
            // Skip the SDBCOMMIT mark record.
            if ( 0 == ossStrcmp( SEADPT_COMMIT_ID, idValue ) )
            {
               continue ;
            }

            bufSize = SEADPT_MAX_ID_SZ + 1 ;
            ossMemset( idBuff, 0, bufSize ) ;
            rc = decodeID( idValue, idBuff, bufSize, type ) ;
            PD_RC_CHECK( rc, PDERROR, "Decode id[ %s ] failed[ %d ]",
                         idValue, rc ) ;
            switch ( type )
            {
            case NumberDouble:
               arrayBuilder.append( *(FLOAT64 *)idBuff ) ;
               break ;
            case String:
               arrayBuilder.append( idBuff ) ;
               break ;
            case Object:
               arrayBuilder.append( BSONObj( (const CHAR *)idBuff ) ) ;
               break ;
            case jstOID:
               arrayBuilder.append( *(bson::OID *)idBuff ) ;
               break ;
            case NumberInt:
               arrayBuilder.append( *(INT32 *)idBuff ) ;
               break ;
            case NumberLong:
               arrayBuilder.append( *(INT64 *)idBuff ) ;
               break ;
            case Bool:
               arrayBuilder.append( (bool)(*idBuff) ) ;
               break ;
            case Date:
               arrayBuilder.append( *(Date_t *)idBuff ) ;
               break ;
            case Timestamp:
               arrayBuilder.appendTimestamp( *(UINT64 *)idBuff ) ;
               break ;
            case NumberDecimal:
            {
               bsonDecimal number ;
               rc = number.fromBsonValue( idBuff ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to parse decimal from bson value:"
                          "decimal=%s,rc=%d", number.toString().c_str(), rc ) ;
                  goto error ;
               }
               arrayBuilder.append( number ) ;
               break ;
            }
            default:
               PD_LOG( PDERROR, "Unsupported type of record[ %d ]",
                       obj.toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         idArray = arrayBuilder.arr() ;

         condition = BSON( "_id" << BSON( "$in" << idArray ) ) ;
         condition.getOwned() ;
         PD_LOG( PDDEBUG, "New in condition: %s",
                 condition.toString().c_str() ) ;
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

}
