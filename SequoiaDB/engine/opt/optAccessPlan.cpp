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

   Source File Name = optAccessPlan.cpp

   Descriptive Name = Optimizer Access Plan

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for optimizer
   access plan creation. It will calculate based on rules and try to estimate
   a lowest cost plan to access data.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "optAccessPlan.hpp"
#include "../bson/ordering.h"
#include "rtn.hpp"
#include "ixm.hpp"
#include "optAPM.hpp"
#include "optStatUnit.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "pmd.hpp"

using namespace bson;
namespace engine
{

   /*
      _optAccessPlan implement
    */
   _optAccessPlan::_optAccessPlan ( optAccessPlanKey &planKey,
                                    const mthNodeConfig &config )
   : _utilHashTableItem(),
     _mthMatchTreeStackHolder(),
     _mthMatchRuntimeHolder(),
     _key( planKey ),
     _isInitialized( FALSE ),
     _hintFailed( FALSE ),
     _isAutoPlan( FALSE ),
     _activityID( OPT_INVALID_ACT_ID ),
     _refCount( 1 )
   {
      getMatchTree()->setMatchConfig( config ) ;

      // acquire access plan ID
      _accessPlanID = sdbGetRTNCB()->getAPM()->acquireAccessPlanID() ;
   }

   _optAccessPlan::~_optAccessPlan ()
   {
      UINT32 refCount = getRefCount() ;
      if ( refCount != 0 )
      {
         PD_LOG ( PDWARNING, "Plan[%s] is deleted when use count is "
                  "not 0: %d", toString().c_str(), refCount ) ;
      }
      getMatchTree()->clear() ;
      deleteMatchRuntime() ;

      /// must clear _scanPath in ~_optAccessPlan
      /// because _scanPath used the _planAllocator, but _planAllocator
      /// will destroyed out of the ~_optAccessPlan
      _scanPath.clearPath() ;
   }

   void _optAccessPlan::release ()
   {
      // If the plan is cached, decrease the reference count
      // If the plan is not cached, delete it when reference count is 1,
      // means it is the last reference
      if ( decRefCount() == 1 && !isCached() )
      {
         SDB_ASSERT( getRefCount() == 0, "Invalid ref count" ) ;
         SDB_OSS_DEL this ;
      }
   }

   string _optAccessPlan::toString () const
   {
      stringstream ss ;
      ss << "CollectionName:" << _key.getCLFullName()
         << ",IndexName:" << getIndexName()
         << ",OrderBy:" << _key.getOrderBy().toString().c_str()
         << ",Query:" << _key.getQuery().toString().c_str()
         << ",Hint:" << _key.getHint().toString().c_str()
         << ",HintFailed:" << ( _hintFailed ? "TRUE" : "FALSE" )
         << ",Direction:" << getDirection()
         << ",ScanType:" << ( TBSCAN == getScanType() ? "TBSCAN" : "IXSCAN" )
         << ",Valid:" << ( _key.isValid() ? "TRUE" : "FALSE" )
         << ",AutoPlan:" << ( _isAutoPlan ? "TRUE" : "FALSE" )
         << ",HashValue:" << _key.getKeyCode()
         << ",Count:" << getRefCount()
         << ",SortRequired:" << ( sortRequired() ? "TRUE" : "FALSE" ) ;
      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACPLAN_TOBSON, "_optAccessPlan::toBSON" )
   INT32 _optAccessPlan::toBSON ( BSONObjBuilder &builder,
                                  const rtnExplainOptions *expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTACPLAN_TOBSON ) ;

      BSONObjBuilderOption builderOption ;

      // copy builder option
      if ( NULL != expOptions )
      {
         builderOption = expOptions->getBuilderOption() ;
      }

      builder.append( OPT_FIELD_CACHE_LEVEL,
                      optAccessPlanKey::getCacheLevelName( getCacheLevel() ) ) ;

      // Selector, skip and limit are not used in cached
      builder.appendEx( OPT_FIELD_QUERY,
                        _key.getNormalizedQuery().isEmpty() ?
                              _key.getQuery() :
                              _key.getNormalizedQuery(),
                        builderOption ) ;
      builder.appendEx( OPT_FIELD_SORT, _key.getOrderBy(),
                        builderOption ) ;
      builder.appendEx( OPT_FIELD_HINT, _key.getHint(),
                        builderOption ) ;
      builder.appendBool( OPT_FIELD_SORTED_IDX_REQURED,
                          _key.isSortedIdxRequired() ) ;
      builder.appendBool( OPT_FIELD_IS_COUNT, _key.isCount() ) ;
      builder.appendBool( OPT_FIELD_EST_FROM_STAT, isEstimatedFromStat() ) ;

      builder.append( OPT_FIELD_HASH_CODE, getKeyCode() ) ;
      builder.append( OPT_FIELD_PLAN_SCORE, getScore() ) ;
      builder.append( OPT_FIELD_PLAN_REF_COUNT, (INT32)getRefCount() ) ;

      rc = _toBSONInternal( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for plan, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTACPLAN_TOBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACPLAN__PREMTHTREE, "_optAccessPlan::_prepareMatchTree" )
   INT32 _optAccessPlan::_prepareMatchTree ( optAccessPlanHelper &planHelper )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTACPLAN__PREMTHTREE ) ;

      planHelper.setMatchTree( getMatchTree() ) ;

      switch ( _key.getCacheLevel() )
      {
         case OPT_PLAN_NOCACHE :
         case OPT_PLAN_ORIGINAL :
         {
            // Load pattern directly
            rc = getMatchTree()->loadPattern( _key.getQuery(), FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load query, rc: %d", rc ) ;

            rc = getMatchTree()->calcPredicate( planHelper.getPredicateSet(),
                                                NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set predicate, rc: %d", rc ) ;

            break ;
         }
         case OPT_PLAN_NORMALIZED :
         case OPT_PLAN_PARAMETERIZED :
         case OPT_PLAN_FUZZYOPTR :
         {
            // Load pattern from normalizer
            rc = getMatchTree()->loadPattern( planHelper.getQuery(),
                                              planHelper.getNormalizer() ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to load query, rc: %d", rc ) ;

            rc = getMatchTree()->calcPredicate( planHelper.getPredicateSet(),
                                                NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set predicate, rc: %d",
                         rc ) ;

            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "Invalid cache level" ) ;
            break ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTACPLAN__PREMTHTREE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _optGeneralAccessPlan implement
    */
   _optGeneralAccessPlan::_optGeneralAccessPlan ( optAccessPlanKey &planKey,
                                                  const mthNodeConfig &config )
   : _optAccessPlan( planKey, config ),
     _cachedPlanMgr( NULL ),
     _searchPaths( NULL )
   {
   }

   _optGeneralAccessPlan::~_optGeneralAccessPlan ()
   {
      _deleteSearchPaths() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__CHKORDER, "_optGeneralAccessPlan::_checkOrderBy" )
   INT32 _optGeneralAccessPlan::_checkOrderBy ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__CHKORDER ) ;

      BSONObjIterator iter( _key.getOrderBy() ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         INT32 value ;
         if ( ossStrcasecmp( ele.fieldName(), "" ) == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's fieldName can't be empty:rc=%d", rc ) ;
            goto error ;
         }

         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's value must be numberic:rc=%d", rc ) ;
            goto error ;
         }

         value = ele.numberInt() ;
         if ( value != 1 && value != -1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's value must be 1 or -1:rc=%d", rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__CHKORDER, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTIXPLAN_NAME, "_optGeneralAccessPlan::_estimateIxScanPlan" )
   INT32 _optGeneralAccessPlan::_estimateIxScanPlan ( dmsStorageUnit *su,
                                                      dmsMBContext *mbContext,
                                                      optCollectionStat *collectionStat,
                                                      optAccessPlanHelper &planHelper,
                                                      const CHAR *pIndexName,
                                                      OPT_PLAN_PATH_PRIORITY priority,
                                                      optScanPath &ixScanPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTIXPLAN_NAME ) ;

      SDB_ASSERT( collectionStat, "collection is invalid" ) ;
      SDB_ASSERT( pIndexName, "pIndexName is invalid" ) ;

      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;

      // Search by index name
      rc = su->index()->getIndexCBExtent( mbContext, pIndexName,
                                          indexCBExtent ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index extent ID from "
                   "collection [%s], index [%s], rc: %d", _key.getCLFullName(),
                   pIndexName, rc ) ;

      rc = _estimateIxScanPlan( su, collectionStat, planHelper, indexCBExtent,
                                priority, ixScanPath ) ;
      if ( rc )
      {
         if ( SDB_OPTION_NOT_SUPPORT != rc )
         {
            PD_LOG( PDWARNING, "Failed to estimate ixscan plan for "
                    "collection [%s], index: [%s], rc: %d", _key.getCLFullName(),
                    pIndexName, rc ) ;
         }
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTIXPLAN_NAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTIXPLAN_OID, "_optGeneralAccessPlan::_estimateIxScanPlan" )
   INT32 _optGeneralAccessPlan::_estimateIxScanPlan ( dmsStorageUnit *su,
                                                      dmsMBContext *mbContext,
                                                      optCollectionStat *collectionStat,
                                                      optAccessPlanHelper &planHelper,
                                                      const OID &indexOID,
                                                      OPT_PLAN_PATH_PRIORITY priority,
                                                      optScanPath &ixScanPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTIXPLAN_OID ) ;

      SDB_ASSERT( collectionStat, "collection is invalid" ) ;

      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;

      // Search by OID
      rc = su->index()->getIndexCBExtent( mbContext, indexOID, indexCBExtent ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index extent ID from "
                   "collection [%s], index [%s], rc: %d", _key.getCLFullName(),
                   indexOID.toString().c_str(), rc ) ;

      rc = _estimateIxScanPlan( su, collectionStat, planHelper, indexCBExtent,
                                priority, ixScanPath ) ;
      if ( rc )
      {
         if ( SDB_OPTION_NOT_SUPPORT != rc )
         {
            PD_LOG( PDWARNING, "Failed to estimate ixscan plan for "
                    "collection [%s], index: [%s], rc: %d", _key.getCLFullName(),
                    indexOID.toString().c_str(), rc ) ;
         }
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTIXPLAN_OID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTIXPLAN, "_optGeneralAccessPlan::_estimateIxScanPlan" )
   INT32 _optGeneralAccessPlan::_estimateIxScanPlan ( dmsStorageUnit *su,
                                                      optCollectionStat *collectionStat,
                                                      optAccessPlanHelper &planHelper,
                                                      dmsExtentID indexCBExtent,
                                                      OPT_PLAN_PATH_PRIORITY priority,
                                                      optScanPath &ixScanPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTIXPLAN ) ;

      SDB_ASSERT( collectionStat, "collection is invalid" ) ;

      ixmIndexCB indexCB ( indexCBExtent, su->index(), NULL ) ;

      PD_CHECK( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error, PDWARNING,
                "Index [%d] in collection [%s] is invalid", indexCBExtent,
                _key.getCLFullName() ) ;

      PD_CHECK( indexCB.getFlag() == IXM_INDEX_FLAG_NORMAL,
                SDB_IXM_UNEXPECTED_STATUS, error, PDDEBUG,
                "Index is not normal status, skip" ) ;

      // Text index can not be used in query without text query condition.
      // So return an error to let this index to be skipped.
      if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT, indexCB.getIndexType() ) )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      try
      {
         optIndexStat indexStat( *collectionStat, indexCB ) ;

         rc = ixScanPath.createIxScan( _key.getCLFullName(), indexCB,
                                       _key, planHelper, priority,
                                       collectionStat, &indexStat ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to create index scan node, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR,
                 "Failed to estimate index scan, received unexpected error:%s",
                 e.what() );
         rc = SDB_INVALIDARG;
         goto error;
      }

      if ( ixScanPath.isCandidate() )
      {
         ixScanPath.evaluate( _key, planHelper.getSortBufferSize() ) ;
      }

   done :
       PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTIXPLAN, rc ) ;
      return rc ;
   error :
      ixScanPath.clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTTBPLAN, "_optGeneralAccessPlan::_estimateTbScanPlan" )
   INT32 _optGeneralAccessPlan::_estimateTbScanPlan ( optCollectionStat *collectionStat,
                                                      optAccessPlanHelper &planHelper,
                                                      optScanPath &tbScanPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTTBPLAN ) ;

      SDB_ASSERT( collectionStat, "collection is invalid" ) ;

      rc = tbScanPath.createTbScan( _key.getCLFullName(), _key, planHelper,
                                    collectionStat ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to create index scan node, rc: %d", rc ) ;

      tbScanPath.evaluate( _key, planHelper.getSortBufferSize() ) ;

   done :
       PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTTBPLAN, rc ) ;
      return rc ;
   error :
      tbScanPath.clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTHINTPLANS, "_optGeneralAccessPlan::_estimateHintPlans" )
   INT32 _optGeneralAccessPlan::_estimateHintPlans ( dmsStorageUnit *su,
                                                     dmsMBContext *mbContext,
                                                     optAccessPlanHelper &planHelper,
                                                     dmsStatCache *statCache,
                                                     BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTHINTPLANS ) ;

      optCollectionStat collectionStat( su->getPageSizeLog2(), mbContext,
                                        planHelper, statCache ) ;

      optScanPath bestPath ;

      BOOLEAN sortedIdxRequired = _key.isSortedIdxRequired() ;

      UINT32 hintCnt = 0 ;
      UINT32 validHints = 0 ;
      BSONObjIterator iter( _key.getHint() ) ;

      PD_LOG ( PDDEBUG, "Hint is provided: %s",
               _key.getHint().toString().c_str() ) ;

      rc = SDB_RTN_INVALID_HINT ;
      finished = FALSE ;

      // user can define more than one index name/oid in hint, it will pickup
      // the first valid one
      while ( iter.more() )
      {
         BSONElement hint = iter.next() ;
         switch ( hint.type() )
         {
            case String :
            {
               const CHAR *pIndexName = hint.valuestr() ;
               if ( '\0' != *( pIndexName ) )
               {
                  optScanPath ixScanPath ;

                  OPT_PLAN_PATH_PRIORITY priority = sortedIdxRequired ?
                                                    OPT_PLAN_SORTED_IDX_REQUIRED :
                                                    OPT_PLAN_IDX_REQUIRED ;

                  ++hintCnt ;

                  PD_LOG ( PDDEBUG, "Try to use index: %s", pIndexName ) ;

                  rc = _estimateIxScanPlan( su, mbContext, &collectionStat,
                                            planHelper, pIndexName, priority,
                                            ixScanPath ) ;
                  if ( SDB_OK != rc )
                  {
                     if ( SDB_OPTION_NOT_SUPPORT != rc )
                     {
                        PD_LOG( PDWARNING, "Failed to estimate index scan for "
                                "collection [%s], index [%s], rc: %d",
                                _key.getCLFullName(), pIndexName, rc ) ;
                        break ;
                     }
                     continue ;
                  }

                  ++validHints ;

                  if ( NULL != _searchPaths )
                  {
                     _addSearchPath( ixScanPath, planHelper ) ;
                  }

                  if ( ixScanPath.isBetterPath( bestPath ) )
                  {
                     bestPath.setPath( ixScanPath, TRUE ) ;
                  }
               }
               else if ( bestPath.isEmpty() )
               {
                  // First { "" : "" } goto auto hint
                  planHelper.setAutoHint( TRUE ) ;
                  rc = SDB_RTN_INVALID_HINT ;
                  goto error ;
               }
               break ;
            }
            case jstOID :
            {
               const OID &indexOID = hint.__oid() ;
               optScanPath ixScanPath ;

               OPT_PLAN_PATH_PRIORITY priority = sortedIdxRequired ?
                                                 OPT_PLAN_SORTED_IDX_REQUIRED :
                                                 OPT_PLAN_IDX_REQUIRED ;

               ++hintCnt ;

               PD_LOG ( PDDEBUG, "Try to use index: %s",
                        indexOID.toString().c_str() ) ;

               rc = _estimateIxScanPlan( su, mbContext, &collectionStat,
                                         planHelper, indexOID, priority,
                                         ixScanPath ) ;
               if ( SDB_OK != rc )
               {
                  if ( SDB_OPTION_NOT_SUPPORT != rc )
                  {
                     PD_LOG( PDWARNING, "Failed to estimate index scan for "
                             "collection [%s], index OID [%s], rc: %d",
                             _key.getCLFullName(), indexOID.toString().c_str(), rc ) ;
                     break ;
                  }
                  continue ;
               }

               ++validHints ;

               if ( NULL != _searchPaths )
               {
                  _addSearchPath( ixScanPath, planHelper ) ;
               }

               if ( ixScanPath.isBetterPath( bestPath ) )
               {
                  bestPath.setPath( ixScanPath, TRUE ) ;
               }

               break ;
            }
            case jstNULL :
            {
               optScanPath tbScanPath ;

               PD_LOG ( PDDEBUG, "Use Collection Scan by Hint" ) ;

               ++hintCnt ;

               // if we use null in the hint, we use tbscan
               rc = _estimateTbScanPlan( &collectionStat, planHelper,
                                         tbScanPath ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "Failed to estimate table scan for "
                          "collection [%s], rc: %d", _key.getCLFullName(),
                          rc ) ;
                  break ;
               }

               ++validHints ;

               if ( NULL != _searchPaths )
               {
                  _addSearchPath( tbScanPath, planHelper ) ;
               }

               if ( tbScanPath.isBetterPath( bestPath ) )
               {
                  bestPath.setPath( tbScanPath, TRUE ) ;
               }

               break ;
            }
            case Object :
            case Array :
            {
               break ;
            }
            default :
            {
               PD_LOG( PDWARNING, "Unknown hint type %d", hint.type() ) ;
               break ;
            }
         }
      }

      /// no auto hint, and force hint and have hints
      if ( !planHelper.isAutoHint() &&
           _key.testFlag( FLG_QUERY_FORCE_HINT ) &&
           hintCnt > 0 )
      {
         finished = TRUE ;
      }

      if ( sortedIdxRequired )
      {
         if ( !planHelper.isAutoHint() && validHints > 0 )
         {
            finished = TRUE ;
         }
         // Report the sort required earlier
         PD_CHECK ( bestPath.isIxScan(),
                    SDB_RTN_QUERYMODIFY_SORT_NO_IDX, error, PDWARNING,
                    "when query and modify, sorting must use index" ) ;
      }

      if ( bestPath.isEmpty() )
      {
         PD_LOG( PDINFO, "Failed to estimate hint plans" ) ;
         rc = SDB_RTN_INVALID_HINT ;
         pdSetLastError( rc ) ;
         goto error ;
      }

      rc = _usePath( su, planHelper, bestPath ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to use hint path, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTHINTPLANS, rc ) ;
      return rc ;
   error :
      if ( validHints > 0 )
      {
         _hintFailed = TRUE ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__ESTPLANS, "_optGeneralAccessPlan::_estimatePlans" )
   INT32 _optGeneralAccessPlan::_estimatePlans ( dmsStorageUnit *su,
                                                 dmsMBContext *mbContext,
                                                 optAccessPlanHelper &planHelper,
                                                 dmsStatCache *statCache )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__ESTPLANS ) ;

      dmsExtentID bestIdxExtID = DMS_INVALID_EXTENT ;

      optScanPath tbScanPath, bestPath ;

      optCollectionStat collectionStat( su->getPageSizeLog2(), mbContext,
                                        planHelper, statCache ) ;
      UINT32 candidateSearchedCount = 0 ;

      optScanType scanType = UNKNOWNSCAN ;
      OPT_PLAN_PATH_PRIORITY priority = OPT_PLAN_DEFAULT_PRIORITY ;

      if ( _key.isSortedIdxRequired() )
      {
         // Have order-by and with query flags to required sorted index
         priority = OPT_PLAN_SORTED_IDX_REQUIRED ;
      }
      else if ( planHelper.isAutoHint() &&
                ( !planHelper.isPredicateSetEmpty() ||
                  !_key.isOrderByEmpty() ) )
      {
         // Have hints, predicates or order-by, which could prefer index-scan
         if ( _key.testFlag( FLG_QUERY_FORCE_HINT ) )
         {
            // Must use index
            if ( !_key.isOrderByEmpty() )
            {
               priority = OPT_PLAN_SORTED_IDX_REQUIRED ;
            }
            else
            {
               priority = OPT_PLAN_IDX_REQUIRED ;
            }
         }
         else
         {
            // Index preferred
            priority = OPT_PLAN_IDX_PREFERRED ;
         }
      }

      if ( priority == OPT_PLAN_IDX_PREFERRED ||
           priority == OPT_PLAN_DEFAULT_PRIORITY )
      {
         // Estimate table scan
         rc = _estimateTbScanPlan( &collectionStat, planHelper, tbScanPath ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to estimate table scan for "
                      "collection [%s], rc: %d", _key.getCLFullName(), rc ) ;
      }

      if ( !planHelper.isPredicateSetEmpty() ||
           !_key.isOrderByEmpty() )
      {
         // We had found a best index earlier, check it first
         if ( collectionStat.getBestIndexName() )
         {
            const CHAR *pIndexName = collectionStat.getBestIndexName() ;
            optScanPath ixScanPath ;

            rc = _estimateIxScanPlan( su, mbContext, &collectionStat,
                                      planHelper, pIndexName, priority,
                                      ixScanPath ) ;

            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to estimate index scan for "
                       "collection [%s], index [%s], rc: %d",
                       _key.getCLFullName(), pIndexName, rc ) ;
            }
            else
            {
               bestIdxExtID = ixScanPath.getIndexExtID() ;

               if ( NULL != _searchPaths )
               {
                  _addSearchPath( ixScanPath, planHelper ) ;
               }

               if ( ixScanPath.isCandidate() )
               {
                  if ( ixScanPath.isBetterPath(bestPath) )
                  {
                     bestPath.setPath( ixScanPath, TRUE ) ;
                  }
                  if ( OPT_PLAN_DEFAULT_PRIORITY == priority ||
                       ixScanPath.isGoodCandidate() )
                  {
                     candidateSearchedCount ++ ;
                  }
               }
            }
         }

         // Go through indexes to find candidate plans
         for ( INT32 idx = 0 ; idx < DMS_COLLECTION_MAX_INDEX ; idx ++ )
         {
            dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;
            optScanPath ixScanPath ;

            rc = su->index()->getIndexCBExtent( mbContext, idx,
                                                indexCBExtent ) ;
            if ( SDB_IXM_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            if ( SDB_OK != rc )
            {
               // Continue to evaluate the rest of indexes
               PD_LOG( PDWARNING, "Failed to get index extent ID from "
                       "collection [%s], index [%d], rc: %d",
                       _key.getCLFullName(), idx, rc ) ;
               continue ;
            }
            if ( bestIdxExtID == indexCBExtent )
            {
               // Already evaluated
               continue ;
            }

            rc = _estimateIxScanPlan( su, &collectionStat, planHelper,
                                      indexCBExtent, priority, ixScanPath ) ;
            if ( SDB_OK != rc )
            {
               // Continue to evaluate the rest of indexes
               if ( SDB_OPTION_NOT_SUPPORT != rc )
               {
                  PD_LOG( PDWARNING, "Failed to estimate index scan for "
                          "collection [%s], index [%d], rc: %d",
                          _key.getCLFullName(), idx, rc ) ;
               }
               continue ;
            }

            if ( NULL != _searchPaths )
            {
               _addSearchPath( ixScanPath, planHelper ) ;
            }

            if ( ixScanPath.isCandidate() )
            {
               if ( ixScanPath.isBetterPath( bestPath ) )
               {
                  bestPath.setPath( ixScanPath, TRUE ) ;
               }
               if ( OPT_PLAN_DEFAULT_PRIORITY == priority ||
                    ixScanPath.isGoodCandidate() )
               {
                  candidateSearchedCount ++ ;
               }

               // Needn't to evaluate all indexes if we got enough
               // candidate plans
               if ( candidateSearchedCount >= OPT_MAX_CANDIDATE_COUNT )
               {
                  break ;
               }
            }
         }
      }

      // Check if sortedIdx is required
      if ( OPT_PLAN_SORTED_IDX_REQUIRED == priority )
      {
         PD_CHECK( bestPath.isIxScan(),
                   SDB_RTN_QUERYMODIFY_SORT_NO_IDX, error, PDWARNING,
                   "Failed to estimate plans: when query and modify, sorting "
                    "must use index" ) ;
      }
      else if ( OPT_PLAN_IDX_REQUIRED == priority )
      {
         PD_CHECK( bestPath.isIxScan(),
                   SDB_RTN_INVALID_HINT, error, PDWARNING,
                   "Failed to estimate plans: when hint is forced, must use "
                   "index" ) ;
      }

      if ( NULL != _searchPaths )
      {
         _addSearchPath( tbScanPath, planHelper ) ;
      }

      // Check the cost of tbScanPath after ixScanPath, so the ixScanPath
      // have higher priority when they have the same costs
      if ( ( ( OPT_PLAN_IDX_PREFERRED == priority &&
               DMS_INVALID_EXTENT == bestPath.getIndexExtID() ) ||
             OPT_PLAN_DEFAULT_PRIORITY == priority ) &&
           tbScanPath.isBetterPath( bestPath ) )
      {
         bestPath.setPath( tbScanPath, TRUE ) ;
      }

      scanType = bestPath.getScanType() ;
      rc = _usePath( su, planHelper, bestPath ) ;
      if ( SDB_OK != rc && IXSCAN == scanType )
      {
         PD_LOG( PDWARNING, "Failed to use index scan, rc: %d", rc ) ;
         if ( OPT_PLAN_SORTED_IDX_REQUIRED == priority )
         {
            rc = SDB_RTN_QUERYMODIFY_SORT_NO_IDX ;
         }
         else if ( OPT_PLAN_IDX_REQUIRED == priority )
         {
            rc = SDB_RTN_INVALID_HINT ;
         }
         else
         {
            if ( tbScanPath.isEmpty() )
            {
               PD_LOG( PDWARNING, "TblScan is not estimated" ) ;
            }
            rc = _usePath( su, planHelper, tbScanPath ) ;
         }
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to use scan path, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__ESTPLANS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__USEPATH, "_optGeneralAccessPlan::_usePath" )
   INT32 _optGeneralAccessPlan::_usePath ( dmsStorageUnit *su,
                                           optAccessPlanHelper &planHelper,
                                           optScanPath &path )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__USEPATH ) ;

      SDB_ASSERT( _matchRuntime, "matchRuntime is invalid" ) ;

      dmsExtentID idxExtID = path.getIndexExtID() ;
      optScanPath emptyPath ;

      // Clear earlier settings
      _matchRuntime->clearPredList() ;
      _scanPath.setPath( emptyPath, TRUE ) ;

      PD_CHECK( !path.isEmpty(), SDB_SYS, error, PDWARNING,
                "Try to use unknown path" ) ;

      rc = bindMatchRuntime( _matchRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind match runtime, rc: %d", rc ) ;

      if ( DMS_INVALID_EXTENT != idxExtID )
      {
         // Check the index
         ixmIndexCB indexCB ( idxExtID, su->index(), NULL ) ;
         PD_CHECK( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error, PDWARNING,
                   "Failed to use index at extent %d", idxExtID ) ;

         // Create predicate list
         rc = _matchRuntime->generatePredList( planHelper.getPredicateSet(),
                                               indexCB.keyPattern(),
                                               path.getDirection(),
                                               planHelper.getNormalizer() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate predicate list, rc: %d",
                      rc ) ;

         // Note: below processing should not be failed

         // Set if we need further match
         if ( !getMatchTree()->isMatchesAll() && path.isMatchAll() )
         {
            getMatchTree()->setMatchesAll( TRUE ) ;
         }
      }

      _scanPath.setPath( path, TRUE ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__USEPATH, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN__PREPARESUCACHES, "_optGeneralAccessPlan::_prepareSUCaches" )
   INT32 _optGeneralAccessPlan::_prepareSUCaches ( dmsStorageUnit *su,
                                                   dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN__PREPARESUCACHES ) ;

      dmsStatCache *statCache = NULL ;
      UINT16 mbID = _key.getCLMBID() ;
      BOOLEAN needCacheStat = FALSE, needCachedPlan = FALSE ;

      // Check if statistics need to be loaded
      statCache = su->getStatCache() ;
      if ( NULL != statCache &&
           UTIL_SU_CACHE_UNIT_STATUS_EMPTY == statCache->getStatus( mbID ) )
      {
         needCacheStat = TRUE ;
      }

      // Check if cached plan status need to be added
      _cachedPlanMgr = su->getCachedPlanMgr() ;
      if ( NULL != _cachedPlanMgr &&
           UTIL_SU_CACHE_UNIT_STATUS_EMPTY == _cachedPlanMgr->getStatus( mbID ) )
      {
         needCachedPlan = TRUE ;
      }

      if ( needCacheStat || needCachedPlan )
      {
         if ( SDB_OK == mbContext->mbLock( EXCLUSIVE ) )
         {
            // NOTE: should not goto error
            if ( needCacheStat )
            {
               // Reload statistics
               pmdEDUCB *cb = pmdGetThreadEDUCB() ;
               _SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
               rtnReloadCLStats ( su, mbContext, cb, dmsCB ) ;
            }

            if ( needCachedPlan )
            {
               // Create cached plan status
               _cachedPlanMgr->createCLCachedPlanUnit( mbID ) ;
            }
         }

         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock dms mb context SHARED failed, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN__PREPARESUCACHES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN_OPT, "_optGeneralAccessPlan::optimize" )
   INT32 _optGeneralAccessPlan::optimize ( dmsStorageUnit *su,
                                           dmsMBContext *mbContext,
                                           optAccessPlanHelper &planHelper )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__OPTGENACPLAN_OPT ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      dmsStatCache *statCache = su->getStatCache() ;

      BOOLEAN mbLocked = FALSE ;

      // Check order-by
      rc = _checkOrderBy() ;
      PD_RC_CHECK( rc, PDERROR, "failed to check orderby", rc ) ;

      // Prepare the match runtime for optimize
      rc = _prepareMatchTree( planHelper ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to load query, rc: %d", rc ) ;

      // Lock the mbContext, then we could access the statistics informations
      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock mbContext [%s], rc: %d",
                      _key.getCLFullName(), rc ) ;
         mbLocked = TRUE ;
      }

      // Prepare statistics and plan caches, etc
      rc = _prepareSUCaches( su, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare for optimize, rc: %d", rc ) ;

      if ( planHelper.isKeepPaths() )
      {
         rc = _createSearchPaths() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create search path list, "
                      "rc: %d", rc ) ;
      }

      // Check hints
      if ( _key.isHintEmpty() )
      {
         // No hints, evaluate the plans directly
         _isAutoPlan = TRUE ;
         rc = _estimatePlans( su, mbContext, planHelper, statCache ) ;
      }
      else
      {
         BOOLEAN finished = FALSE ;
         // Evaluate hints first
         rc = _estimateHintPlans( su, mbContext, planHelper, statCache,
                                  finished ) ;
         if ( SDB_OK != rc && !finished )
         {
            // could evaluate with all candidate plans again
            // Without sorted index should be reported
            _isAutoPlan = TRUE ;
            rc = _estimatePlans( su, mbContext, planHelper, statCache ) ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to create candidate plans, rc: %d", rc ) ;

      // Done evaluation, mark the plan valid
      _isInitialized = TRUE ;
      _key.setValid( TRUE ) ;

      // Clear hint failing marks
      if ( planHelper.isAutoHint() && _hintFailed && IXSCAN == getScanType() )
      {
         _hintFailed = FALSE ;
      }

      PD_LOG( PDDEBUG, "Optimizer: Use plan %s", toString().c_str() ) ;

      rc = SDB_OK ;

   done :
      if ( mbLocked )
      {
         mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN_OPT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN_BINDMTHRTM, "_optGeneralAccessPlan::bindMatchRuntime" )
   INT32 _optGeneralAccessPlan::bindMatchRuntime ( mthMatchRuntime *matchRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN_BINDMTHRTM ) ;

      matchRuntime->setMatchTree( getMatchTree() ) ;

      PD_TRACE_EXITRC( SDB__OPTGENACPLAN_BINDMTHRTM, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN_CRTSEARCHPATHS, "_optGeneralAccessPlan::_createSearchPaths" )
   INT32 _optGeneralAccessPlan::_createSearchPaths ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTGENACPLAN_CRTSEARCHPATHS ) ;

      _deleteSearchPaths() ;

      _searchPaths = new ( std::nothrow ) optScanPathList() ;
      PD_CHECK( NULL != _searchPaths, SDB_OOM, error, PDERROR,
                "Failed to allocate search list" ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTGENACPLAN_CRTSEARCHPATHS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTGENACPLAN_DELSEARCHPATHS, "_optGeneralAccessPlan::_deleteSearchPaths" )
   void _optGeneralAccessPlan::_deleteSearchPaths ()
   {
      PD_TRACE_ENTRY( SDB__OPTGENACPLAN_DELSEARCHPATHS ) ;

      SAFE_OSS_DELETE( _searchPaths ) ;

      PD_TRACE_EXIT( SDB__OPTGENACPLAN_DELSEARCHPATHS ) ;
   }

   INT32 _optGeneralAccessPlan::_addSearchPath ( optScanPath & path,
                                                 const optAccessPlanHelper & planHelper )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _searchPaths, "search path list is invalid" ) ;

      if ( IXSCAN == path.getScanType() )
      {
         const rtnExplainOptions *explainOptions =
                                             planHelper.getExplainOptions() ;
         BOOLEAN isAbbrev = NULL != explainOptions &&
                            explainOptions->isNeedAbbrev() ;
         rtnPredicateList predList ;
         UINT32 addedLevel = 0 ;
         rc = predList.initialize( planHelper.getPredicateSet(),
                                   path.getKeyPattern(), path.getDirection(),
                                   addedLevel ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate index bounds, "
                      "rc: %d", rc ) ;

         path.getScanNode()->setIXBound( predList.getBound( isAbbrev ) ) ;
      }

      _searchPaths->push_back( path ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   /*
      _optParamAccessPlan implement
    */
   _optParamAccessPlan::_optParamAccessPlan ( optAccessPlanKey &planKey,
                                              const mthNodeConfig &config )
   : _optGeneralAccessPlan( planKey, config ),
     _mthParamPredListStackHolder(),
     _isParamValid( FALSE ),
     _paramValidCount( 0 ),
     _score( 0.0 )
   {
      SDB_ASSERT( _key.getCacheLevel() >= OPT_PLAN_PARAMETERIZED,
                  "Invalid cache level" ) ;
   }

   _optParamAccessPlan::~_optParamAccessPlan ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN_VALIDPARAM, "_optParamAccessPlan::validateParameterized" )
   BOOLEAN _optParamAccessPlan::validateParameterized ( const _optAccessPlan &plan,
                                                        const BSONObj &parameters )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN_VALIDPARAM ) ;

      if ( plan.getScanType() == getScanType() &&
           plan.getIndexCBExtent() == getIndexCBExtent() &&
           plan.getIndexLID() == getIndexLID() &&
           plan.getDirection() == getDirection() )
      {
         _saveParam( parameters, plan.getScore() ) ;
         result = TRUE ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Plan generated is different from parameterized "
                 "plan: [%s] vs [%s]", toString().c_str(),
                 plan.toString().c_str() ) ;
         result = FALSE ;
      }

      PD_TRACE_EXIT( SDB__OPTPARAMACPLAN_VALIDPARAM ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN_CHKSAVEDPARAM, "_optParamAccessPlan::checkSavedParam" )
   BOOLEAN _optParamAccessPlan::checkSavedParam ( const BSONObj &parameters )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN_CHKSAVEDPARAM ) ;

      UINT32 savedCount = _paramValidCount.peek() ;

      savedCount = OSS_MIN( OPT_PARAM_VALID_PLAN_NUM, savedCount ) ;

      for ( UINT32 i = 0 ; i < savedCount ; i++ )
      {
         if ( parameters.shallowEqual( _records[ i ]._parameters ) )
         {
            res = TRUE ;
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTPARAMACPLAN_CHKSAVEDPARAM ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN_MARKINVALID, "_optParamAccessPlan::markParamInvalid" )
   INT32 _optParamAccessPlan::markParamInvalid ( dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN_MARKINVALID ) ;

      if ( NULL != _cachedPlanMgr )
      {
         UINT16 mbID = mbContext->mbID() ;
         BOOLEAN isSharedLocked = mbContext->isMBLock( SHARED ) ;

         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock mbContext [%s] exclusive, "
                      "rc: %d", getCLFullName(), rc ) ;

         dmsCLCachedPlanUnit *pCachedPlanUnit =
               (dmsCLCachedPlanUnit *)_cachedPlanMgr->getCacheUnit( mbID ) ;
         if ( NULL != pCachedPlanUnit )
         {
            pCachedPlanUnit->incParamInvalid() ;
            if ( pCachedPlanUnit->isParamInvalid() )
            {
               _cachedPlanMgr->setParamInvalidBit( mbID ) ;
               PD_LOG( PDDEBUG, "Mark collection [%s] parameterize invalid",
                       getCLFullName() ) ;
            }
         }

         if ( isSharedLocked )
         {
            rc = mbContext->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to lock mbContext [%s] shared, "
                         "rc: %d", getCLFullName(), rc ) ;
         }
         else
         {
            mbContext->mbUnlock() ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTPARAMACPLAN_MARKINVALID, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN_BINDMTHRTM, "_optParamAccessPlan::bindMatchRuntime" )
   INT32 _optParamAccessPlan::bindMatchRuntime ( mthMatchRuntime *matchRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN_BINDMTHRTM ) ;

      matchRuntime->setMatchTree( getMatchTree() ) ;
      matchRuntime->setParamPredList( getParamPredList() ) ;

      PD_TRACE_EXITRC( SDB__OPTPARAMACPLAN_BINDMTHRTM, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN__SAVEPARAM, "_optParamAccessPlan::_saveParam" )
   void _optParamAccessPlan::_saveParam ( const BSONObj &parameters, double score )
   {
      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN__SAVEPARAM ) ;

      UINT32 paramIndex = _paramValidCount.inc() ;
      if ( paramIndex < OPT_PARAM_VALID_PLAN_NUM )
      {
         // Save the specified plan for validation
         _records[ paramIndex ]._parameters = parameters.getOwned() ;
         _records[ paramIndex ]._score = score ;

         if ( paramIndex == OPT_PARAM_VALID_PLAN_NUM - 1 )
         {
            // The parameterized plan could be validated, so calculate the
            // score which is the standard difference of each specified plan
            // to validate the parameterized plan
            double avgScores = 0.0 ;
            double diffScores = 0.0 ;
            for ( UINT32 i = 0 ; i < OPT_PARAM_VALID_PLAN_NUM ; i++ )
            {
               avgScores += _records[i]._score ;
            }
            avgScores /= (double)OPT_PARAM_VALID_PLAN_NUM ;
            for ( UINT32 i = 0 ; i < OPT_PARAM_VALID_PLAN_NUM ; i++ )
            {
               double tmp = _records[i]._score - avgScores ;
               diffScores += ( tmp * tmp ) ;
            }
            _score = sqrt( diffScores / (double)OPT_PARAM_VALID_PLAN_NUM ) ;
            _isParamValid = TRUE ;

            PD_LOG( PDDEBUG, "Validate parameterized plan [%s]: score [%.6f]",
                    toString().c_str(), _score ) ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTPARAMACPLAN__SAVEPARAM ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN__TOBSONINT, "_optParamAccessPlan::_toBSONInternal" )
   INT32 _optParamAccessPlan::_toBSONInternal ( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN__TOBSONINT ) ;

      builder.appendBool( OPT_FIELD_PARAM_PLAN_VALID, isParamValid() ) ;

      // Output the validate parameters when the parameterized plan is
      // invalidate
      if ( !isParamValid() )
      {
         UINT32 paramValidNum = _paramValidCount.peek() ;
         if ( paramValidNum > 0 )
         {
            BSONArrayBuilder paramBuilder(
                  builder.subarrayStart( OPT_FIELD_VALID_PARAMS ) ) ;
            for ( UINT32 index = 0 ; index < paramValidNum ; index++ )
            {
               BSONObjBuilder subBuilder( paramBuilder.subobjStart() ) ;
               subBuilder.appendElements( _records[index]._parameters ) ;
               subBuilder.append( OPT_FIELD_PLAN_SCORE,
                                  _records[index]._score ) ;
               subBuilder.done() ;
            }
            paramBuilder.done() ;
         }
      }

      PD_TRACE_EXITRC( SDB__OPTPARAMACPLAN__TOBSONINT, rc ) ;

      return rc ;
   }

   /*
      _optMainCLAccessPlan implement
    */
   _optMainCLAccessPlan::_optMainCLAccessPlan ( optAccessPlanKey &planKey,
                                                const mthNodeConfig &config )
   : _optAccessPlan( planKey, config ),
     _mthParamPredListStackHolder(),
     _isMainCLValid( FALSE ),
     _mainCLValidCount( 0 ),
     _score( 0.0 )
   {
   }

   _optMainCLAccessPlan::~_optMainCLAccessPlan ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINACPLAN_PREPAREBINDSUB, "_optMainCLAccessPlan::prepareBindSubCL" )
   INT32 _optMainCLAccessPlan::prepareBindSubCL ( optAccessPlanHelper &planHelper )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINACPLAN_PREPAREBINDSUB ) ;

      // Prepare the match runtime for optimize
      rc = _prepareMatchTree( planHelper ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to load query, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTMAINACPLAN_PREPAREBINDSUB, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINACPLAN_BINDSUBACPLAN, "_optMainCLAccessPlan::bindSubCLAccessPlan" )
   INT32 _optMainCLAccessPlan::bindSubCLAccessPlan ( optAccessPlanHelper &planHelper,
                                                     optGeneralAccessPlan *subPlan,
                                                     dmsMBContext *mbContext,
                                                     const BSONObj &parameters )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINACPLAN_BINDSUBACPLAN ) ;

      SDB_ASSERT( _matchRuntime, "_matchRuntime is invalid" ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid" ) ;

      rc = _scanPath.copyPath( subPlan->getScanPath() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to copy scan path, rc: %d", rc ) ;

      rc = _scanPath.clearScanInfo() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to clear scan info, rc: %d", rc ) ;

      _hintFailed = subPlan->isHintFailed() ;
      _isAutoPlan = subPlan->isAutoGen() ;

      _saveSubCL( subPlan->getCLFullName(),
                  mbContext->mb()->_clUniqueID,
                  subPlan->getScore(), parameters ) ;

      rc = bindMatchRuntime( planHelper, subPlan ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind match runtime, rc: %d", rc ) ;

      // Bind finished, set the plan validated
      _isInitialized = TRUE ;
      _key.setValid( TRUE ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTMAINACPLAN_BINDSUBACPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN_VALIDSUBCLPLAN, "_optMainCLAccessPlan::validateSubCLPlan" )
   BOOLEAN _optMainCLAccessPlan::validateSubCLPlan ( const optGeneralAccessPlan *plan,
                                                     dmsMBContext *mbContext,
                                                     const BSONObj &parameters )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN_VALIDSUBCLPLAN ) ;

      SDB_ASSERT( plan, "plan is invalid" ) ;

      if ( getScanType() == plan->getScanType() &&
           sortRequired() == plan->sortRequired() )
      {
         if ( IXSCAN == getScanType() )
         {
            if ( getDirection() == plan->getDirection() &&
                 0 == ossStrncmp( getIndexName(), plan->getIndexName(),
                                  IXM_INDEX_NAME_SIZE ) &&
                 getKeyPattern().shallowEqual( plan->getKeyPattern() ) )
            {
               result = TRUE ;
            }
         }
         else
         {
            result = TRUE ;
         }
      }

      if ( result )
      {
         _saveSubCL( plan->getCLFullName(),
                     mbContext->mb()->_clUniqueID,
                     plan->getScore(),
                     parameters ) ;
      }

      PD_TRACE_EXIT( SDB__OPTMAINCLACPLAN_VALIDSUBCLPLAN ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN_VALIDSUBCL_SU, "_optMainCLAccessPlan::validateSubCL" )
   INT32 _optMainCLAccessPlan::validateSubCL ( dmsStorageUnit *su,
                                               dmsMBContext *mbContext,
                                               dmsExtentID &indexExtID,
                                               dmsExtentID &indexLID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN_VALIDSUBCL_SU ) ;

      BOOLEAN mbLocked = FALSE ;

      indexExtID = DMS_INVALID_EXTENT ;
      indexLID = DMS_INVALID_EXTENT ;

      if ( IXSCAN == getScanType() )
      {
         if ( !mbContext->isMBLock() )
         {
            rc = mbContext->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context, rc: %d", rc ) ;
            mbLocked = TRUE ;
         }

         rc = su->index()->getIndexCBExtent( mbContext, getIndexName(),
                                             indexExtID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s], rc: %d",
                      getIndexName(), rc ) ;

         {
            ixmIndexCB indexCB( indexExtID, su->index(), mbContext ) ;

            PD_CHECK( indexCB.isInitialized(),
                      SDB_DMS_INIT_INDEX, error, PDWARNING,
                      "Index [%s] is invalid", getIndexName() ) ;
            PD_CHECK( indexCB.getFlag() == IXM_INDEX_FLAG_NORMAL,
                      SDB_IXM_UNEXPECTED_STATUS, error, PDDEBUG,
                      "Index [%s] is not normal status", getIndexName() ) ;

            if ( !getKeyPattern().shallowEqual( indexCB.keyPattern() ) )
            {
               rc = SDB_IXM_NOTEXIST ;
               PD_LOG( PDERROR, "Index not exists" ) ;
               goto error ;
            }

            indexLID = indexCB.getLogicalID() ;
         }
      }

   done :
      if ( mbLocked )
      {
         mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__OPTMAINCLACPLAN_VALIDSUBCL_SU, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTPARAMACPLAN_CHKSAVEDSUBCL, "_optMainCLAccessPlan::checkSavedSubCL" )
   BOOLEAN _optMainCLAccessPlan::checkSavedSubCL ( utilCLUniqueID subCLUID,
                                                   const BSONObj & parameters )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__OPTPARAMACPLAN_CHKSAVEDSUBCL ) ;

      UINT32 savedCount = _mainCLValidCount.peek() ;

      savedCount = OSS_MIN( OPT_MAINCL_VALID_PLAN_NUM, savedCount ) ;

      for ( UINT32 i = 0 ; i < savedCount ; i++ )
      {
         if ( _records[ i ]._subCLUID == subCLUID &&
              parameters.shallowEqual( _records[ i ]._parameters ) )
         {
            res = TRUE ;
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTPARAMACPLAN_CHKSAVEDSUBCL ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN_MARKINVALID, "_optMainCLAccessPlan::markMainCLInvalid" )
   INT32 _optMainCLAccessPlan::markMainCLInvalid ( dmsCachedPlanMgr *pCachedPlanMgr,
                                                   dmsMBContext *mbContext,
                                                   BOOLEAN markInvalid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN_MARKINVALID ) ;

      if ( NULL != pCachedPlanMgr )
      {
         dmsCLCachedPlanUnit *pCachedPlanUnit = NULL ;
         UINT16 mbID = mbContext->mbID() ;
         BOOLEAN isSharedLocked = mbContext->isMBLock( SHARED ) ;

         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock mbContext [%s] exclusive, "
                      "rc: %d", getCLFullName(), rc ) ;

         pCachedPlanUnit = (dmsCLCachedPlanUnit *)
                           pCachedPlanMgr->getCacheUnit( mbID ) ;

         if ( markInvalid )
         {
            if ( NULL != pCachedPlanUnit )
            {
               pCachedPlanUnit->setMainCLInvalid() ;
            }
            pCachedPlanMgr->setMainCLInvalidBit( mbID ) ;
            PD_LOG( PDDEBUG, "Mark collection [%s] parameterize invalid",
                    getCLFullName() ) ;
         }
         else
         {
            if ( NULL != pCachedPlanUnit )
            {
               pCachedPlanUnit->incMainCLInvalid() ;
               if ( pCachedPlanUnit->isMainCLInvalid() )
               {
                  pCachedPlanMgr->setMainCLInvalidBit( mbID ) ;
                  PD_LOG( PDDEBUG, "Mark collection [%s] parameterize invalid",
                          getCLFullName() ) ;
               }
            }
         }

         if ( isSharedLocked )
         {
            rc = mbContext->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to lock mbContext [%s] shared, "
                         "rc: %d", getCLFullName(), rc ) ;
         }
         else
         {
            mbContext->mbUnlock() ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTMAINCLACPLAN_MARKINVALID, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN_BINDMTHRTM, "_optMainCLAccessPlan::bindMatchRuntime" )
   INT32 _optMainCLAccessPlan::bindMatchRuntime ( mthMatchRuntime *matchRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN_BINDMTHRTM ) ;

      matchRuntime->setMatchTree( getMatchTree() ) ;
      matchRuntime->setParamPredList( getParamPredList() ) ;

      PD_TRACE_EXITRC( SDB__OPTMAINCLACPLAN_BINDMTHRTM, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN_BINDMTHRTM_SUB, "_optMainCLAccessPlan::bindMatchRuntime" )
   INT32 _optMainCLAccessPlan::bindMatchRuntime ( optAccessPlanHelper & planHelper,
                                                  optGeneralAccessPlan * subPlan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN_BINDMTHRTM_SUB ) ;

      SDB_ASSERT( _matchRuntime, "_matchRuntime is invalid" ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid" ) ;

      rc = bindMatchRuntime( _matchRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind match runtime, rc: %d", rc ) ;

      // Only need to bind predicates for index-scan plan
      if ( IXSCAN == subPlan->getScanType() &&
           NULL == _matchRuntime->getPredList() )
      {
         mthMatchTree * matchTree = _matchRuntime->getMatchTree() ;
         mthMatchTree * subMatchTree = subPlan->getMatchTree() ;

         if ( NULL != matchTree && NULL != subMatchTree )
         {
            matchTree->setMatchesAll( subMatchTree->isMatchesAll() ) ;
         }

         rc = _matchRuntime->generatePredList( planHelper.getPredicateSet(),
                                               subPlan->getKeyPattern(),
                                               subPlan->getDirection(),
                                               planHelper.getNormalizer() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate predicate list, rc: %d",
                      rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTMAINCLACPLAN_BINDMTHRTM_SUB, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN__SAVESUBCL, "_optMainCLAccessPlan::_saveSubCL" )
   void _optMainCLAccessPlan::_saveSubCL ( const CHAR *pSubCLName,
                                           utilCLUniqueID subCLUID,
                                           double score,
                                           const BSONObj &parameters )
   {
      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN__SAVESUBCL ) ;

      UINT32 paramIndex = _mainCLValidCount.inc() ;
      if ( paramIndex < OPT_MAINCL_VALID_PLAN_NUM )
      {
         // Save the specified plan for validation
         try
         {
            if ( NULL != pSubCLName )
            {
               _records[ paramIndex ]._subCLName.assign( pSubCLName ) ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to save sub collection name, "
                    "occur exception %s", e.what() ) ;
         }
         _records[ paramIndex ]._subCLUID = subCLUID ;
         _records[ paramIndex ]._parameters = parameters ;
         _records[ paramIndex ]._score = score ;

         if ( paramIndex == OPT_MAINCL_VALID_PLAN_NUM - 1 )
         {
            // The parameterized plan could be validated, so calculate the
            // score which is the standard difference of each specified plan
            // to validate the parameterized plan
            double avgScores = 0.0 ;
            double diffScores = 0.0 ;
            for ( UINT32 i = 0 ; i < OPT_MAINCL_VALID_PLAN_NUM ; i++ )
            {
               avgScores += _records[i]._score ;
            }
            avgScores /= (double)OPT_MAINCL_VALID_PLAN_NUM ;
            for ( UINT32 i = 0 ; i < OPT_MAINCL_VALID_PLAN_NUM ; i++ )
            {
               double tmp = _records[i]._score - avgScores ;
               diffScores += ( tmp * tmp ) ;
            }
            _score = sqrt( diffScores / (double)OPT_MAINCL_VALID_PLAN_NUM ) ;
            _isMainCLValid = TRUE ;

            PD_LOG( PDDEBUG, "Validate main-collection plan [%s]: score [%.6f]",
                    toString().c_str(), _score ) ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTMAINCLACPLAN__SAVESUBCL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTMAINCLACPLAN__TOBSONINT, "_optMainCLAccessPlan::_toBSONInternal" )
   INT32 _optMainCLAccessPlan::_toBSONInternal ( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTMAINCLACPLAN__TOBSONINT ) ;

      builder.appendBool( OPT_FIELD_MAINCL_PLAN_VALID, isMainCLValid() ) ;

      if ( !isMainCLValid() )
      {
         UINT32 mainCLValidNum = _mainCLValidCount.peek() ;
         if ( mainCLValidNum > 0 )
         {
            BSONArrayBuilder subCLBuilder(
                  builder.subarrayStart( OPT_FIELD_VALID_SUBCLS ) ) ;
            for ( UINT32 index = 0 ; index < mainCLValidNum ; index++ )
            {
               BSONObjBuilder subBuilder( subCLBuilder.subobjStart() ) ;
               subBuilder.append( OPT_FIELD_COLLECTION,
                                  _records[index]._subCLName.c_str() ) ;
               if ( !_records[index]._parameters.isEmpty() )
               {
                  subBuilder.appendElements( _records[index]._parameters ) ;
               }
               subBuilder.append( OPT_FIELD_PLAN_SCORE,
                                  _records[index]._score ) ;
               subBuilder.done() ;
            }
            subCLBuilder.done() ;
         }
      }

      PD_TRACE_EXITRC( SDB__OPTMAINCLACPLAN__TOBSONINT, rc ) ;

      return rc ;
   }

}
