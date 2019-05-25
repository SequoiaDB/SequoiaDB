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
#include "mthSelector.hpp"
#include "optAPM.hpp"
#include "rtnIXScanner.hpp"
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

      rtnContext *context = NULL ;

      context = rtnCB->contextFind ( contextID, cb ) ;
      if ( !context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      rc = context->getMore( maxNumToReturn, buffObj, cb ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            PD_LOG( PDDEBUG, "Hit end of context" ) ;
            goto error ;
         }
         PD_LOG( PDERROR, "Failed to get more from context[%lld], rc: %d",
                 context->contextID(), rc ) ;
         context->getErrorInfo( rc, cb, buffObj ) ;
         goto error ;
      }

      if ( context->isWrite() && context->getDPSCB() && context->getW() > 1 )
      {
         context->getDPSCB()->completeOpr( cb, context->getW() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETMORE, rc ) ;
      return rc ;
   error :
      if ( context )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      goto done ;
   }

   static INT32 _rtnParseQueryModify( const BSONObj &hint,
                                      rtnQueryModifier** modifier )
   {
      BSONObjIterator iter( hint );
      BOOLEAN isUpdate = FALSE ;
      BOOLEAN isRemove = FALSE ;
      BSONObj updator ;
      BOOLEAN returnNew = FALSE ;
      rtnQueryModifier* queryModifier = NULL ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != modifier, "modifier can't be null" ) ;

      while ( iter.more() )
      {
         BSONElement elem = iter.next() ;

         if ( 0 == ossStrcmp( elem.fieldName(), FIELD_NAME_MODIFY ) )
         {
            BSONObj modify = elem.Obj() ;
            const CHAR* op = NULL ;

            rc = rtnGetStringElement( modify, FIELD_NAME_OP, &op ) ;
            PD_RC_CHECK( rc, PDERROR,
               "Query and modify has invalid field[%s] in hint: %s",
               FIELD_NAME_OP, hint.toString().c_str() ) ;

            if ( 0 == ossStrcmp( op, FIELD_OP_VALUE_UPDATE ) )
            {
               isUpdate = TRUE ;

               rc = rtnGetBooleanElement( modify, FIELD_NAME_RETURNNEW, returnNew ) ;
               if ( SDB_INVALIDARG == rc )
               {
                  PD_LOG( PDERROR,
                     "Query and modify has invalid field[%s] in hint: %s",
                     FIELD_NAME_RETURNNEW, hint.toString().c_str() ) ;
                  goto error ;
               }

               rc = rtnGetObjElement( modify, FIELD_NAME_OP_UPDATE, updator ) ;
               PD_RC_CHECK( rc, PDERROR,
                  "Query and modify has invalid field[%s] in hint: %s",
                  FIELD_NAME_OP_UPDATE, hint.toString().c_str() ) ;
            }
            else if ( 0 == ossStrcmp( op, FIELD_OP_VALUE_REMOVE ) )
            {
               isRemove = TRUE ;

               BOOLEAN remove = FALSE ;
               rc = rtnGetBooleanElement( modify, FIELD_NAME_OP_REMOVE, remove ) ;
               PD_RC_CHECK( rc, PDERROR,
                  "Query and modify has invalid field[%s] in hint: %s",
                  FIELD_NAME_OP_REMOVE, hint.toString().c_str() ) ;

               if ( TRUE != remove )
               {
                  PD_LOG( PDERROR,
                     "Query and modify has invalid field[%s] in hint: %s",
                     FIELD_NAME_OP_REMOVE, hint.toString().c_str() ) ;
                  goto error ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "Query and modify has invalid hint: %s",
                 hint.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }

      if ( !isUpdate && !isRemove )
      {
         PD_LOG( PDERROR, "Query and modify has no modify hint: %s",
                 hint.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      queryModifier = SDB_OSS_NEW rtnQueryModifier( isUpdate, isRemove, returnNew ) ;
      if ( NULL == queryModifier )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      if ( isUpdate )
      {
         rc = queryModifier->loadUpdator( updator ) ;
         PD_RC_CHECK( rc, PDERROR,
                  "Query and modify has invalid updator: %s",
                  updator.toString().c_str() ) ;
      }

      *modifier = queryModifier ;

   done:
      return rc ;
   error:
      if ( NULL != queryModifier )
      {
         SDB_OSS_DEL queryModifier ;
      }
      goto done ;
   }

   static INT32 _rtnParseQueryMeta( const BSONObj &meta, const CHAR *&scanType,
                                    const CHAR *&indexName, INT32 &indexLID,
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

         rc = rtnGetIntElement( meta, FIELD_NAME_INDEXLID, indexLID ) ;
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
                                 BOOLEAN &found )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__FINDFIELDINOBJ ) ;

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
                  rc = _findFieldInObj( ele.Obj(), fieldName, found ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
                  if ( found )
                  {
                     goto done ;
                  }
               }
               else if ( Array == ele.type() )
               {
                  BSONObjIterator subItr( ele.embeddedObject() ) ;
                  while ( subItr.more() )
                  {
                     BSONElement subEle = subItr.next() ;
                     if ( Object == subEle.type() )
                     {
                        rc = _findFieldInObj( subEle.Obj(), fieldName, found ) ;
                        if ( rc )
                        {
                           goto error ;
                        }
                        if ( found )
                        {
                           goto done ;
                        }
                     }
                  }
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
   static INT32 _getQueryType( const BSONObj &query, rtnQueryType &qType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__GETQUERYTYPE ) ;
      BOOLEAN textFound = FALSE ;

      rc = _findFieldInObj( query, FIELD_NAME_TEXT, textFound ) ;
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
                                rtnContextBase **ppContext, BOOLEAN enablePrefetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNQUERYWITHTS ) ;
      INT64 tmpContextID = -1 ;
      rtnContextTS *contextTS = NULL ;

      if ( options.testFlag( FLG_QUERY_EXPLAIN ) )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG( PDERROR, "Explain query with text search condition is not "
                 "support yet" ) ;
         goto error ;
      }
      else
      {
         rc = rtnCB->contextNew( RTN_CONTEXT_TS, (rtnContext **)&contextTS,
                                 tmpContextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create new text search context, "
                      "rc: %d", rc ) ;
         if ( options.canPrepareMore() )
         {
            contextTS->setPrepareMoreData( TRUE ) ;
         }

         rc = contextTS->open( options, cb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to open text search context, "
                      "rc: %d", rc ) ;
            }
            goto error ;
         }
      }

      if ( cb->getMonConfigCB()->timestampON )
      {
         contextTS->getMonCB()->recordStartTimestamp() ;
      }

      contextID = tmpContextID ;
      if ( ppContext )
      {
         *ppContext = contextTS ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNQUERYWITHTS, rc ) ;
      return rc ;
   error:
      if ( -1 != tmpContextID )
      {
         rtnCB->contextDelete( tmpContextID, cb ) ;
      }
      goto done ;
   }

   INT32 rtnSort ( rtnContext **ppContext,
                   const BSONObj &orderBy,
                   _pmdEDUCB *cb,
                   SINT64 numToSkip,
                   SINT64 numToReturn,
                   SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = NULL ;
      rtnContext *bkContext = NULL ;
      SINT64 old = contextID ;
      SINT64 sortContextID = -1 ;

      if ( NULL == ppContext ||
           NULL == *ppContext )
      {
         PD_LOG( PDERROR, "invalid src context" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bkContext = *ppContext ;

      rc = rtnCB->contextNew ( RTN_CONTEXT_SORT,
                               &context,
                               sortContextID,
                               cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to crt context:%d", rc ) ;
         goto error ;
      }

      rc = ((_rtnContextSort *)context)->open( orderBy,
                                               *ppContext,
                                               cb,
                                               numToSkip,
                                               numToReturn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open sort context:%d", rc ) ;
         goto error ;
      }

      contextID = sortContextID ;
      *ppContext = context ;
   done:
      return rc ;
   error:
      if ( -1 != sortContextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      contextID = old ;
      if ( NULL != ppContext )
      {
         *ppContext = bkContext ;
      }
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
                    rtnContextBase **ppContext,
                    BOOLEAN enablePrefetch )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNQUERY ) ;

      rtnQueryOptions options( matcher, selector, orderBy, hint,
                               pCollectionName, numToSkip, numToReturn, flags ) ;
      rc = rtnQuery( options, cb, dmsCB, rtnCB, contextID, ppContext,
                     enablePrefetch, FALSE ) ;

      PD_TRACE_EXITRC( SDB_RTNQUERY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNQUERY_OPTIONS, "rtnQuery" )
   INT32 rtnQuery ( rtnQueryOptions &options,
                    pmdEDUCB *cb,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextBase **ppContext,
                    BOOLEAN enablePrefetch,
                    BOOLEAN keepSearchPaths )
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
      rtnContextData *dataContext = NULL ;
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
      INT32 indexLID = DMS_INVALID_EXTENT ;
      INT32 direction = 0 ;
      rtnQueryType queryType = RTN_QUERY_NORMAL ;
      rtnRemoteMessenger* messenger = rtnCB->getRemoteMessenger() ;

      if ( messenger && messenger->isReady() )
      {
         rc = _getQueryType( options.getQuery(), queryType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get query type, rc: %d", rc ) ;
         if ( RTN_QUERY_TEXT == queryType )
         {
            rc = rtnQueryWithTS( options, cb, rtnCB, contextID,
                                 ppContext, enablePrefetch ) ;
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

      if ( options.testFlag( FLG_QUERY_MODIFY ) )
      {
         rc = _rtnParseQueryModify( hintTmp, &queryModifier ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to parse query and modify:%d", rc ) ;
            goto error ;
         }

         options.clearFlag( FLG_QUERY_PARALLED ) ;

         rc = dmsCB->writable( cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
            goto error;
         }
         writable = TRUE ;
      }

      rc = rtnResolveCollectionNameAndLock ( options.getCLFullName(), dmsCB,
                                             &su, &pCollectionShortName,
                                             suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s",
                   options.getCLFullName() ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      if ( options.testFlag( FLG_QUERY_MODIFY ) &&
           OSS_BIT_TEST( mbContext->mb()->_attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         PD_LOG( PDERROR, "Can not modify data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      rc = rtnCB->contextNew ( options.testFlag( FLG_QUERY_PARALLED ) ?
                               RTN_CONTEXT_PARADATA : RTN_CONTEXT_DATA,
                               (rtnContext**)&dataContext,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new data context" ) ;

      if ( options.canPrepareMore() )
      {
         dataContext->setPrepareMoreData( TRUE ) ;
      }

      if ( Object == options.getHint().getField( FIELD_NAME_META ).type() )
      {
         BSONObjBuilder build ;
         rc = _rtnParseQueryMeta(
               options.getHint().getField( FIELD_NAME_META ).embeddedObject(),
               scanType, indexName, indexLID, direction, blockObj ) ;
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
         hintTmp = build.obj () ;
      }

      options.setHint( hintTmp ) ;

      planRuntime = dataContext->getPlanRuntime() ;
      SDB_ASSERT( planRuntime, "plan runtime shouldn't be NULL" ) ;

      apm = rtnCB->getAPM() ;
      SDB_ASSERT( apm, "apm shouldn't be NULL" ) ;

      rc = apm->getAccessPlan( options, keepSearchPaths, su, mbContext,
                               (*planRuntime) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s, "
                   "context %lld, rc: %d", options.getCLFullName(), contextID,
                   rc ) ;

      if ( options.testFlag( FLG_QUERY_FORCE_HINT ) &&
           !options.isHintEmpty() &&
           planRuntime->isHintFailed() )
      {
         PD_LOG( PDERROR, "Query used force hint[%s] failed",
                 options.getHint().toString().c_str() ) ;
         rc = SDB_RTN_INVALID_HINT ;
         goto error ;
      }

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
            PD_LOG( PDERROR, "Scan type[%d] error or indexLID[%d] is the "
                    "same with [%d]", planRuntime->getScanType(),
                    planRuntime->getIndexLID(), indexLID ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }
      }

      if ( !planRuntime->sortRequired() )
      {
         rc = dataContext->open( su, mbContext, cb, options, pBlockObj,
                                 direction ) ;
         PD_RC_CHECK( rc, PDERROR, "Open data context failed, rc: %d", rc ) ;

         suID = DMS_INVALID_CS ;
         mbContext = NULL ;

         if ( options.testFlag( FLG_QUERY_MODIFY ) )
         {
            dataContext->setQueryModifier( queryModifier ) ;
            queryModifier = NULL ;
            writable = FALSE ;
         }

         if ( options.testFlag( FLG_QUERY_STRINGOUT ) )
         {
            dataContext->getSelector().setStringOutput( TRUE ) ;
         }
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
         PD_RC_CHECK( rc, PDERROR, "Open data context failed, rc: %d", rc ) ;

         suID = DMS_INVALID_CS ;
         mbContext = NULL ;

         if ( options.testFlag( FLG_QUERY_STRINGOUT ) )
         {
            dataContext->getSelector().setStringOutput( TRUE ) ;
         }

         dataContext->setEnableQueryActivity( FALSE ) ;

         rc = rtnSort ( (rtnContext**)&dataContext, options.getOrderBy(), cb,
                        options.getSkip(), options.getLimit(), contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

      if ( cb->getMonConfigCB()->timestampON )
      {
         dataContext->getMonCB()->recordStartTimestamp() ;
      }

      if ( ppContext )
      {
         *ppContext = dataContext ;
      }
      if ( enablePrefetch )
      {
         dataContext->enablePrefetch ( cb ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRAVERSALQUERY, "rtnTraversalQuery" )
   INT32 rtnTraversalQuery ( const CHAR *pCollectionName,
                             const BSONObj &key,
                             const CHAR *pIndexName,
                             INT32 dir,
                             pmdEDUCB *cb,
                             SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB,
                             SINT64 &contextID,
                             rtnContextData **ppContext,
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
      rtnContextData       *context              = NULL ;
      const CHAR           *pCollectionShortName = NULL ;
      optAccessPlanRuntime *planRuntime          = NULL ;
      dmsMBContext         *mbContext            = NULL ;
      rtnPredicateList     *predList             = NULL ;
      rtnIXScanner         *scanner              = NULL ;

      BSONObj hint ;
      BSONObj dummy ;

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name %s",
                    pCollectionName ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      rc = rtnCB->contextNew ( RTN_CONTEXT_DATA, (rtnContext**)&context,
                               contextID, cb ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to create new context, %d", rc ) ;
      SDB_ASSERT ( context, "context can't be NULL" ) ;

      planRuntime = context->getPlanRuntime() ;

      try
      {
         hint = BSON( "" << pIndexName ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Failed to construct hint object: %s",
                       e.what() ) ;
      }

      {
         rtnQueryOptions options( dummy, dummy, dummy, hint, pCollectionName,
                                  0, -1, 0 ) ;

         rc = rtnCB->getAPM()->getTempAccessPlan( options, su, mbContext,
                                                  *planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;

         PD_CHECK ( planRuntime->getScanType() == IXSCAN &&
                    !planRuntime->isAutoGen(),
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to generate access plan by index %s", pIndexName ) ;
      }

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      {
         dmsRecordID rid ;
         if ( -1 == dir )
         {
            rid.resetMax() ;
         }
         else
         {
            rid.resetMin () ;
         }
         ixmIndexCB indexCB ( planRuntime->getIndexCBExtent(), su->index(), NULL ) ;
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
         predList = planRuntime->getPredList() ;
         SDB_ASSERT ( predList, "predList can't be NULL" ) ;
         predList->setDirection ( dir ) ;

         scanner = SDB_OSS_NEW rtnIXScanner ( &indexCB, predList, su, cb ) ;
         PD_CHECK ( scanner, SDB_OOM, error, PDERROR,
                    "Unable to allocate memory for scanner" ) ;

         rc = scanner->relocateRID ( key, rid ) ;
         PD_CHECK ( SDB_OK == rc, rc, error, PDERROR,
                    "Failed to relocate key to the specified location: %s, "
                    "rc = %d", key.toString().c_str(), rc ) ;
      }
      mbContext->mbUnlock() ;

      rc = context->openTraversal( su, mbContext, scanner, cb, dummy, -1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context traversal faield, rc: %d", rc ) ;

      mbContext = NULL ;
      suID = DMS_INVALID_CS ;
      scanner = NULL ;
      su = NULL ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      if ( ppContext )
      {
         *ppContext = context ;
      }
      if ( enablePrefetch )
      {
         context->enablePrefetch ( cb ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNTRAVERSALQUERY, rc ) ;
      return rc ;
   error :
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( planRuntime->hasPlan() )
      {
         planRuntime->releasePlan() ;
      }
      if ( scanner )
      {
         SDB_OSS_DEL scanner ;
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
                     rtnContextBase **ppContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNEXPLAIN ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      rtnContextExplain * context = NULL ;
      rc = rtnCB->contextNew( RTN_CONTEXT_EXPLAIN,
                              ( rtnContext **)( &context ),
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

