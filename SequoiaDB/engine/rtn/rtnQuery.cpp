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

   Source File Name = rtnQuery.cpp

   Descriptive Name = Runtime Query

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for query
   and getmore request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "mthSelector.hpp"
#include "optAPM.hpp"
#include "rtnScannerFactory.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnContextData.hpp"
#include "rtnContextSort.hpp"
#include "rtnContextExplain.hpp"
#include "rtnContextTS.hpp"
#include "rtnQueryModifier.hpp"

using namespace bson ;

namespace engine
{
   enum _rtnQueryType
   {
      RTN_QUERY_NORMAL = 1,
      RTN_QUERY_TEXT
   } ;
   typedef _rtnQueryType rtnQueryType ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETMORE, "rtnGetMore" )
   INT32 rtnGetMore ( SINT64 contextID,         // input, context id
                      SINT32 maxNumToReturn,    // input, max record to read
                      rtnContextBuf &buffObj,   // output
                      pmdEDUCB *cb,             // input educb
                      SDB_RTNCB *rtnCB          // input runtimecb
                      )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETMORE ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      rtnContextPtr context ;

      // retrieve the context pointer
      rc = rtnCB->contextFind ( contextID, context, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist, rc: %d", contextID, rc ) ;
         goto error ;
      }

      rc = rtnGetMore( context, maxNumToReturn, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETMORE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETMORE1, "rtnGetMore" )
   INT32 rtnGetMore ( rtnContextPtr &pContext,  // input, context
                      SINT32 maxNumToReturn,    // input, max record to read
                      rtnContextBuf &buffObj,   // output
                      pmdEDUCB *cb,             // input educb
                      SDB_RTNCB *rtnCB          // input runtimecb
                      )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETMORE1 ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      if ( !pContext )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pContext->getMore( maxNumToReturn, buffObj, cb ) ;
      if ( SDB_OK != rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDERROR, "Failed to get more from context[%lld], rc: %d",
                 pContext->contextID(), rc ) ;
         /// get detial information
         pContext->getErrorInfo( rc, cb, buffObj ) ;
         goto error ;
      }

      /// wait for sync
      if ( pContext->isWrite() && pContext->getDPSCB() && pContext->getW() > 1 )
      {
         cb->setOrgReplSize( pContext->getW() ) ;
         // For now we don't report error, since the user will not be able to
         // due with the situation, in which case primary node is done but the
         // secondary nodes are not synchronized
         pContext->getDPSCB()->completeOpr( cb, pContext->getW() ) ;
      }

      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDDEBUG, "Hit end of context" ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETMORE1, rc ) ;
      return rc ;
   error :
      if ( pContext )
      {
         rtnCB->contextDelete ( pContext->contextID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNADVANCE, "rtnAdvance" )
   INT32 rtnAdvance( SINT64 contextID,
                     const BSONObj &arg,
                     const CHAR *pBackData,
                     INT32 backDataSize,
                     pmdEDUCB *cb,
                     SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNADVANCE ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      rtnContextPtr context ;

      // retrieve the context pointer
      rc = rtnCB->contextFind ( contextID, context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context %lld does not exist, rc: %d",
                   contextID, rc ) ;

      rc = context->advance( arg, pBackData, backDataSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Advance context[%lld] failed, rc: %d",
                 contextID, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNADVANCE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static INT32 _rtnParseQueryMeta( const BSONObj &meta, const CHAR *&scanType,
                                    const CHAR *&indexName, dmsExtentID &indexLID,
                                    INT32 &direction, BSONObj &blockObj )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;

      rc = rtnGetStringElement( meta, FIELD_NAME_SCANTYPE, &scanType ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_SCANTYPE, rc ) ;

      if ( 0 == ossStrcmp( scanType, VALUE_NAME_IXSCAN ) )
      {
         ele = meta.getField( FIELD_NAME_INDEXBLOCKS ) ;

         rc = rtnGetStringElement( meta, FIELD_NAME_INDEXNAME, &indexName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_INDEXNAME, rc ) ;

         rc = rtnGetIntElement( meta, FIELD_NAME_INDEXLID, (INT32 &)indexLID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_INDEXLID, rc ) ;

         rc = rtnGetIntElement( meta, FIELD_NAME_DIRECTION, direction ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_DIRECTION, rc ) ;
      }
      else if ( 0 == ossStrcmp( scanType, VALUE_NAME_TBSCAN ) )
      {
         ele = meta.getField( FIELD_NAME_DATABLOCKS ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Query meta[%s] scan type error",
                 meta.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( Array != ele.type() )
      {
         PD_LOG( PDERROR, "Block field[%s] type error",
                 ele.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      blockObj = ele.embeddedObject() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__FINDFIELDINOBJ, "_findFieldInObj" )
   static INT32 _findFieldInObj( const BSONObj &object, const CHAR *fieldName,
                                 BOOLEAN &found, BOOLEAN &ridFilterRequired )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__FINDFIELDINOBJ ) ;

      found = FALSE ;

      try
      {
         if ( object.hasField( fieldName ) )
         {
            found = TRUE ;
            goto done ;
         }
         else
         {
            BSONObjIterator itr( object ) ;
            while ( itr.more() )
            {
               BSONElement ele = itr.next() ;
               if ( Object == ele.type() )
               {
                  rc = _findFieldInObj( ele.Obj(), fieldName, found,
                                        ridFilterRequired) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }
               // for combined condition by $and, $or, $not
               else if ( Array == ele.type() )
               {
                  BSONObjIterator subItr( ele.embeddedObject() ) ;
                  while ( subItr.more() )
                  {
                     BSONElement subEle = subItr.next() ;
                     if ( Object == subEle.type() )
                     {
                        rc = _findFieldInObj( subEle.Obj(), fieldName,
                                              found, ridFilterRequired ) ;
                        if ( rc )
                        {
                           goto error ;
                        }
                        if ( found )
                        {
                           // Text query condition is found in sub level of this
                           // one.
                           if ( !ridFilterRequired &&
                                ( 0 == ossStrcmp( MTH_OPERATOR_STR_OR,
                                                  ele.fieldName() )
                                  ||
                                  0 == ossStrcmp( MTH_OPERATOR_STR_NOT,
                                                  ele.fieldName() ) ) )
                           {
                              ridFilterRequired = TRUE ;
                           }
                           break ;
                        }
                     }
                  }
               }

               if ( found )
               {
                  goto done ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__FINDFIELDINOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__GETQUERYTYPE, "_getQueryType" )
   static INT32 _getQueryType( const BSONObj &query, rtnQueryType &qType,
                               BOOLEAN &ridFilterRequired )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__GETQUERYTYPE ) ;
      BOOLEAN textFound = FALSE ;

      rc = _findFieldInObj( query, FIELD_NAME_TEXT, textFound,
                            ridFilterRequired ) ;
      PD_RC_CHECK( rc, PDERROR, "Find field[ %s ] in query[ %s ]failed[ %d ]",
                   FIELD_NAME_TEXT, query.toString().c_str(), rc ) ;
      qType = textFound ? RTN_QUERY_TEXT : RTN_QUERY_NORMAL ;

   done:
      PD_TRACE_EXITRC( SDB__GETQUERYTYPE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNQUERYWITHTS, "rtnQueryWithTS" )
   static INT32 rtnQueryWithTS( const rtnQueryOptions &options, pmdEDUCB *cb,
                                SDB_RTNCB *rtnCB, SINT64 &contextID,
                                rtnContextPtr *ppContext,
                                BOOLEAN enablePrefetch,
                                BOOLEAN ridFilterRequired )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNQUERYWITHTS ) ;
      rtnContextTS::sharePtr contextTS ;
      rtnContextPtr context ;

      contextID = -1 ;

      if ( options.testFlag( FLG_QUERY_EXPLAIN )
           || options.testFlag( FLG_QUERY_MODIFY ) )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG( PDERROR, "Operation with text search condition is not "
                 "support yet" ) ;
         goto error ;
      }
      else
      {
         rc = rtnCB->contextNew( RTN_CONTEXT_TS, contextTS,
                                 contextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create new text search context, "
                      "rc: %d", rc ) ;
         if ( options.canPrepareMore() )
         {
            contextTS->setPrepareMoreData( TRUE ) ;
         }

         if ( ridFilterRequired )
         {
            contextTS->enableRIDFilter() ;
         }

         if ( options.isOrderByEmpty() )
         {
            rc = contextTS->open( options, cb ) ;
         }
         else
         {
            rtnQueryOptions subOption( options ) ;
            subOption.setSkip( 0 ) ;
            subOption.setLimit( -1 ) ;
            rc = contextTS->open( subOption, cb ) ;
         }
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to open text search context, "
                      "rc: %d", rc ) ;
            }
            goto error ;
         }

         if ( !options.isOrderByEmpty() )
         {
            rc = rtnSort( contextTS, options.getOrderBy(), cb,
                          options.getSkip(), options.getLimit(), contextID,
                          &context ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
         }
         else
         {
            context = contextTS ;
         }
      }

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      if ( ppContext )
      {
         *ppContext = context ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNQUERYWITHTS, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSORT, "rtnSort" )
   INT32 rtnSort ( rtnContextPtr &pContext,
                   const BSONObj &orderBy,
                   _pmdEDUCB *cb,
                   SINT64 numToSkip,
                   SINT64 numToReturn,
                   SINT64 &contextID,
                   rtnContextPtr *ppContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNSORT ) ;

      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContextSort::sharePtr context ;
      SINT64 old = contextID ;
      SINT64 sortContextID = -1 ;

      if ( !pContext )
      {
         PD_LOG( PDERROR, "invalid src context" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnCB->contextNew ( RTN_CONTEXT_SORT,
                               context,
                               sortContextID,
                               cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to crt context:%d", rc ) ;
         goto error ;
      }

      rc = context->open( orderBy,
                          pContext,
                          cb,
                          numToSkip,
                          numToReturn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open sort context:%d", rc ) ;
         goto error ;
      }

      contextID = sortContextID ;
      if ( ppContext )
      {
         *ppContext = context ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNSORT, rc ) ;
      return rc ;
   error:
      if ( -1 != sortContextID )
      {
         rtnCB->contextDelete ( sortContextID, cb ) ;
         sortContextID = -1 ;
      }
      contextID = old ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNQUERY, "rtnQuery" )
   INT32 rtnQuery ( const CHAR *pCollectionName,
                    const BSONObj &selector,
                    const BSONObj &matcher,
                    const BSONObj &orderBy,
                    const BSONObj &hint,
                    SINT32 flags,
                    pmdEDUCB *cb,
                    SINT64 numToSkip,
                    SINT64 numToReturn,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextPtr *ppContext,
                    BOOLEAN enablePrefetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNQUERY ) ;

      // matcher, selector, order, hint, collection, skip, limit, flag
      rtnQueryOptions options( matcher, selector, orderBy, hint,
                               pCollectionName, numToSkip, numToReturn, flags ) ;
      rc = rtnQuery( options, cb, dmsCB, rtnCB, contextID, ppContext,
                     enablePrefetch, NULL ) ;

      PD_TRACE_EXITRC( SDB_RTNQUERY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNQUERY_OPTIONS, "rtnQuery" )
   INT32 rtnQuery ( rtnQueryOptions &options,
                    pmdEDUCB *cb,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextPtr *ppContext,
                    BOOLEAN enablePrefetch,
                    const rtnExplainOptions *expOptions )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNQUERY_OPTIONS ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      contextID             = -1 ;

      SDB_ASSERT ( options.getCLFullName(), "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      rtnContextData::sharePtr dataContext ;
      rtnContextPtr context ;
      const CHAR *pCollectionShortName = NULL ;
      optAccessPlanManager *apm = NULL ;
      optAccessPlanRuntime *planRuntime = NULL ;
      rtnQueryModifier *queryModifier = NULL ;
      BOOLEAN writable = FALSE ;

      BSONObj hintTmp = options.getHint() ;
      BSONObj blockObj, emptyObj ;
      BSONObj *pBlockObj = NULL ;
      const CHAR *indexName = NULL ;
      const CHAR *scanType  = NULL ;
      dmsExtentID indexLID = DMS_INVALID_EXTENT ;
      INT32 direction = 1 ;
      rtnQueryType queryType = RTN_QUERY_NORMAL ;
      rtnRemoteMessenger* messenger = rtnCB->getRemoteMessenger() ;

      UINT32 scannerRetryTime = 0 ;

      // check if the adapter is registered.
      if ( messenger && messenger->isReady() )
      {
         BOOLEAN ridFilterRequired = FALSE ;
         rc = _getQueryType( options.getQuery(), queryType,
                             ridFilterRequired ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get query type, rc: %d", rc ) ;
         if ( RTN_QUERY_TEXT == queryType )
         {
            rc = rtnQueryWithTS( options, cb, rtnCB, contextID,
                                 ppContext, enablePrefetch,
                                 ridFilterRequired ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG( PDERROR, "Query with text search condition "
                          "failed[ %d ]", rc ) ;
               }
               goto error ;
            }
            goto done ;
         }
      }

      if ( options.testFlag( FLG_QUERY_EXPLAIN ) )
      {
         rc = rtnExplain( options, cb, dmsCB, rtnCB, contextID, ppContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to explain query:%d", rc ) ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }

      /// When in transaction, can't enable prefetch and paralled query
      if ( DPS_INVALID_TRANS_ID != cb->getTransID() )
      {
         enablePrefetch = FALSE ;
         options.clearFlag( FLG_QUERY_PARALLED ) ;
      }

      if ( options.testFlag( FLG_QUERY_MODIFY ) )
      {
         queryModifier = SDB_OSS_NEW rtnQueryModifier ;
         if ( !queryModifier )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for query modifier failed[%d]",
                    rc) ;
            goto error ;
         }
         rc = queryModifier->init( hintTmp ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to parse query and modify:%d", rc ) ;
            goto error ;
         }

         // disallow parallel query
         options.clearFlag( FLG_QUERY_PARALLED ) ;

         // writeable judge
         rc = dmsCB->writable( cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
            goto error;
         }
         writable = TRUE ;
      }

      // This prevents other sessions drop the collectionspace during accessing
      rc = rtnResolveCollectionNameAndLock ( options.getCLFullName(), dmsCB,
                                             &su, &pCollectionShortName,
                                             suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s",
                   options.getCLFullName() ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      /// if collection don't have $id index, can't modify( update or remove )
      if ( options.testFlag( FLG_QUERY_MODIFY ) &&
           OSS_BIT_TEST( mbContext->mb()->_attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         PD_LOG( PDERROR, "Can not modify data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      try
      {
      BSONElement eMeta  = hintTmp.getField( FIELD_NAME_META ) ;
      BSONElement ePos   = hintTmp.getField( FIELD_NAME_POSITION ) ;
      BSONElement eRange = hintTmp.getField( FIELD_NAME_RANGE ) ;

      if ( !ePos.eoo() && !eRange.eoo() )
      {
         PD_LOG( PDERROR, "Field[%s] and Field[%s] cannot be specified at the "
                 "same time", FIELD_NAME_POSITION, FIELD_NAME_RANGE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !ePos.eoo() && Object != ePos.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid", FIELD_NAME_POSITION ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !eRange.eoo() && Object != eRange.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid", FIELD_NAME_RANGE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // create a new context
      rc = rtnCB->contextNew ( options.testFlag( FLG_QUERY_PARALLED ) ?
                               RTN_CONTEXT_PARADATA : RTN_CONTEXT_DATA,
                               dataContext,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new data context" ) ;

      if ( options.canPrepareMore() )
      {
         dataContext->setPrepareMoreData( TRUE ) ;
      }

      // Adjust hint for meta-query
      if ( Object == eMeta.type() )
      {
         BSONObjBuilder build ;
         BSONObjIterator itrHint( hintTmp ) ;

         /// parse $Meta
         rc = _rtnParseQueryMeta( eMeta.embeddedObject(), scanType,
                                  indexName, indexLID, direction, blockObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse query meta[%s], rc: %d",
                      options.getHint().toString().c_str(), rc ) ;

         pBlockObj = &blockObj ;

         if ( indexName )
         {
            build.append( "", indexName ) ;
         }
         else
         {
            build.appendNull( "" ) ;
         }

         /// append other hints
         while ( itrHint.more() )
         {
            BSONElement e = itrHint.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), FIELD_NAME_META ) )
            {
               build.append( e ) ;
            }
         }
         hintTmp = build.obj () ;
      }

      // Reassign hint
      options.setHint( hintTmp ) ;

      planRuntime = dataContext->getPlanRuntime() ;
      SDB_ASSERT( planRuntime, "plan runtime shouldn't be NULL" ) ;

      apm = rtnCB->getAPM() ;
      SDB_ASSERT( apm, "apm shouldn't be NULL" ) ;

retry:
      // plan is released in context destructor
      // selector, numToSkip and numToReturn are not considered in plan cache
      // now, so put dummy ones to find the plan
      rc = apm->getAccessPlan( options, su, mbContext, (*planRuntime),
                               expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s, "
                   "context %lld, rc: %d", options.getCLFullName(), contextID,
                   rc ) ;

      // used force hint, but hint failed
      if ( options.testFlag( FLG_QUERY_FORCE_HINT ) &&
           planRuntime->isHintFailed() )
      {
         PD_LOG( PDERROR, "Query used force hint[%s] failed",
                 options.getHint().toString().c_str() ) ;
         rc = SDB_RTN_INVALID_HINT ;
         goto error ;
      }

      // check
      if ( pBlockObj )
      {
         if ( !indexName && TBSCAN != planRuntime->getScanType() )
         {
            PD_LOG( PDERROR, "Scan type[%d] must be TBSCAN",
                    planRuntime->getScanType() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( indexName && ( IXSCAN != planRuntime->getScanType() ||
                   indexLID != planRuntime->getIndexLID() ) )
         {
            PD_LOG( PDERROR, "Scan type[%d] error or indexLID[%d] is not the "
                    "same with [%d]", planRuntime->getScanType(),
                    planRuntime->getIndexLID(), indexLID ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }
      }

      if ( !planRuntime->sortRequired() )
      {
         // open context
         rc = dataContext->open( su, mbContext, cb, options, pBlockObj,
                                 direction ) ;
         if ( SDB_IXM_NOTEXIST == rc && scannerRetryTime < 1 )
         {
            // Maybe in the process of scanning the index,
            // the index is deleted
            planRuntime->reset() ;
            scannerRetryTime++ ;
            // We only need to try to scan once. In most cases,
            // the next scan is normal
            goto retry ;
         }
         PD_RC_CHECK( rc, PDERROR, "Open data context failed, rc: %d", rc ) ;

         /// when open succeed, plan and mbcontext and su is take over
         /// by context
         suID = DMS_INVALID_CS ;
         mbContext = NULL ;

         if ( options.testFlag( FLG_QUERY_MODIFY ) )
         {
            dataContext->setQueryModifier( queryModifier ) ;
            // queryModifier will be released by dataContext
            queryModifier = NULL ;
            // dmsCB will be writedown by dataContext
            writable = FALSE ;
         }

         if ( options.testFlag( FLG_QUERY_STRINGOUT ) )
         {
            dataContext->getSelector().setStringOutput( TRUE ) ;
         }

         if ( Object == ePos.type() )
         {
            rc = dataContext->locate( ePos.embeddedObject(), cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Do context locate failed, rc: %d", rc ) ;
               goto error ;
            }
         }
         else if ( Object == eRange.type() )
         {
            rc = dataContext->setAdvanceSection( eRange.embeddedObject() ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Do context set advance condition failed "
                                ", rc: %d", rc ) ;
               goto error ;
            }
         }

         context = dataContext ;
      }
      else
      {
         rtnReturnOptions returnOptions( options ) ;

         returnOptions.setSkip( 0 ) ;
         returnOptions.setLimit( -1 ) ;

         if ( options.testFlag( FLG_QUERY_MODIFY ) )
         {
            PD_LOG( PDERROR, "when query and modify, sorting must use index");
            rc = SDB_RTN_QUERYMODIFY_SORT_NO_IDX ;
            goto error ;
         }

         rc = dataContext->open( su, mbContext, cb, returnOptions, pBlockObj,
                                 direction ) ;
         if ( SDB_IXM_NOTEXIST == rc && scannerRetryTime < 1 )
         {
            // Maybe in the process of scanning the index,
            // the index is deleted
            planRuntime->reset() ;
            scannerRetryTime++ ;
            // We only need to try to scan once. In most cases,
            // the next scan is normal
            goto retry ;
         }
         PD_RC_CHECK( rc, PDERROR, "Open data context failed, rc: %d", rc ) ;

         /// when open succeed, plan and mbcontext and su is take over
         /// by context
         suID = DMS_INVALID_CS ;
         mbContext = NULL ;

         if ( options.testFlag( FLG_QUERY_STRINGOUT ) )
         {
            dataContext->getSelector().setStringOutput( TRUE ) ;
         }

         dataContext->setEnableQueryActivity( FALSE ) ;

         if ( Object == ePos.type() )
         {
            rc = dataContext->locate( ePos.embeddedObject(), cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Do context locate failed, rc: %d", rc ) ;
               goto error ;
            }
         }
         else if ( Object == eRange.type() )
         {
            rc = dataContext->setAdvanceSection( eRange.embeddedObject() ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Do context set advance condition "
                       "failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         rc = rtnSort ( dataContext, options.getOrderBy(), cb,
                        options.getSkip(), options.getLimit(), contextID,
                        &context ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      // sample timetamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      if ( enablePrefetch )
      {
         context->enablePrefetch ( cb ) ;
      }

      if ( ppContext )
      {
         *ppContext = context ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNQUERY_OPTIONS, rc ) ;
      return rc ;
   error :
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      if ( queryModifier )
      {
         SDB_OSS_DEL queryModifier ;
      }
      goto done ;
   }

   // given a collection name, a key ( without field name ), an index name, and
   // a direction, this function will create a context and build an index
   // scanner
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRAVERSALQUERY, "rtnTraversalQuery" )
   INT32 rtnTraversalQuery ( const CHAR *pCollectionName,
                             const BSONObj &key,
                             const CHAR *pIndexName,
                             INT32 dir,
                             pmdEDUCB *cb,
                             SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB,
                             SINT64 &contextID,
                             rtnContextPtr *ppContext,
                             BOOLEAN enablePrefetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTRAVERSALQUERY ) ;
      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( pIndexName, "index name can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;
      SDB_ASSERT ( dir == 1 || dir == -1, "dir must be 1 or -1" ) ;

      dmsStorageUnitID      suID                 = DMS_INVALID_CS ;
      dmsStorageUnit       *su                   = NULL ;
      rtnContextData::sharePtr context ;
      const CHAR           *pCollectionShortName = NULL ;
      optAccessPlanRuntime *planRuntime          = NULL ;
      dmsMBContext         *mbContext            = NULL ;
      rtnPredicateList     *predList             = NULL ;
      rtnIXScanner         *scanner              = NULL ;
      rtnScannerFactory    f ;

      BSONObj hint ;
      BSONObj dummy ;

      // collection in dmsCB lock is released when context is freed
      // This prevents other sessions drop the collectionspace during accessing
      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name %s",
                    pCollectionName ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      // create a new context
      rc = rtnCB->contextNew ( RTN_CONTEXT_DATA, context,
                               contextID, cb ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to create new context, %d", rc ) ;
      SDB_ASSERT ( context, "context can't be NULL" ) ;

      planRuntime = context->getPlanRuntime() ;

      try
      {
         // build hint
         hint = BSON( "" << pIndexName ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Failed to construct hint object: %s",
                       e.what() ) ;
      }

      {
         // matcher, selector, order, hint, collection, skip, limit, flag
         rtnQueryOptions options( dummy, dummy, dummy, hint, pCollectionName,
                                  0, -1, 0 ) ;

         rc = rtnCB->getAPM()->getTempAccessPlan( options, su, mbContext,
                                                  *planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;

         // Must apply the hint to find index-scan plan
         PD_CHECK ( planRuntime->getScanType() == IXSCAN &&
                    !planRuntime->isAutoGen(),
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to generate access plan by index %s", pIndexName ) ;
      }

      // lock
      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // start building scanner
      {
         rtnScannerType scannerType = ( DPS_INVALID_TRANS_ID !=
                                       cb->getTransID() ) ?
                                       SCANNER_TYPE_MERGE :
                                       SCANNER_TYPE_DISK ;
         dmsRecordID      rid ;
         if ( -1 == dir )
         {
            rid.resetMax() ;
         }
         else
         {
            rid.resetMin () ;
         }
         // get the index control block we want
         ixmIndexCB indexCB ( planRuntime->getIndexCBExtent(),
                              su->index(), NULL ) ;
         PD_CHECK ( indexCB.isInitialized(), SDB_SYS, error, PDERROR,
                    "unable to get proper index control block" ) ;
         if ( indexCB.getLogicalID() != planRuntime->getIndexLID() )
         {
            PD_LOG( PDERROR, "Index[extent id: %d] logical id[%d] is not "
                    "expected[%d]", planRuntime->getIndexCBExtent(),
                    indexCB.getLogicalID(), planRuntime->getIndexLID() ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }
         // get the predicate list
         predList = planRuntime->getPredList() ;
         SDB_ASSERT ( predList, "predList can't be NULL" ) ;
         // set the traversal direction
         predList->setDirection ( dir ) ;

         rc = f.createIXScanner( scannerType, &indexCB, predList, su, mbContext, FALSE, cb, scanner ) ;
         if ( rc )
         {
            goto error ;
         }

         // reloate RID to the key that we want
         rc = scanner->relocateRID ( key, rid ) ;
         PD_CHECK ( SDB_OK == rc, rc, error, PDERROR,
                    "Failed to relocate key to the specified location: %s, "
                    "rc = %d", key.toString().c_str(), rc ) ;
      }
      mbContext->mbUnlock() ;

      // open context
      rc = context->openTraversal( su, mbContext, scanner, cb, dummy, -1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context traversal faield, rc: %d", rc ) ;

      mbContext = NULL ;
      suID = DMS_INVALID_CS ;
      scanner = NULL ;
      su = NULL ;

      // sample timestamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      /// In transaction, can't use prefetch
      if ( enablePrefetch && DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         context->enablePrefetch ( cb ) ;
      }

      if ( ppContext )
      {
         *ppContext = context ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNTRAVERSALQUERY, rc ) ;
      return rc ;
   error :
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( NULL != planRuntime && planRuntime->hasPlan() )
      {
         planRuntime->reset() ;
      }
      if ( scanner )
      {
         f.releaseScanner( scanner ) ;
         scanner = NULL ;
      }
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAIN, "rtnExplain" )
   INT32 rtnExplain( rtnQueryOptions &options,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_RTNCB *rtnCB, INT64 &contextID,
                     rtnContextPtr *ppContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNEXPLAIN ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      rtnContextExplain::sharePtr context ;
      rc = rtnCB->contextNew( RTN_CONTEXT_EXPLAIN,
                              context,
                              contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create explain context:%d", rc ) ;
         goto error ;
      }

      rc = context->open( options, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open explain context:%d", rc ) ;
         goto error ;
      }

      if ( ppContext )
      {
         *ppContext = context ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAIN, rc ) ;
      return rc ;

   error :
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }
}

