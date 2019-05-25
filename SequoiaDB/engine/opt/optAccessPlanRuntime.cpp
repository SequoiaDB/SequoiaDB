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

   Source File Name = optAccessPlanRuntime.cpp

   Descriptive Name = Optimizer Access Plan Runtime

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for runtime of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optAccessPlanRuntime.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "pmd.hpp"
#include "optAPM.hpp"
#include "rtnCB.hpp"
#include "rtnContext.hpp"
#include "rtnContextData.hpp"

using namespace bson ;

namespace engine
{

   /*
      _optQueryActivity implement
    */
   _optQueryActivity::_optQueryActivity ()
   : _optrType( MON_COUNTER_OPERATION_NONE ),
     _parameters(),
     _returnOptions(),
     _contextMonitor(),
     _hitEnd( FALSE )
   {
   }

   _optQueryActivity::_optQueryActivity ( MON_OPERATION_TYPES optrType,
                                          const rtnParamList &parameters,
                                          const monContextCB &monCtxCB,
                                          const rtnReturnOptions &returnOptions,
                                          BOOLEAN hitEnd )
   : _optrType( optrType),
     _returnOptions( returnOptions ),
     _contextMonitor( monCtxCB ),
     _hitEnd( hitEnd )
   {
      if ( !parameters.isEmpty() )
      {
         _parameters = parameters.toBSON() ;
      }
   }

   _optQueryActivity::~_optQueryActivity ()
   {
   }

   void _optQueryActivity::reset ()
   {
      _optrType = MON_COUNTER_OPERATION_NONE ;
      _parameters = BSONObj() ;
      _contextMonitor.reset() ;
      _returnOptions.reset() ;
      _hitEnd = FALSE ;
   }

   _optQueryActivity & _optQueryActivity::operator = (
                                       const _optQueryActivity & activity )
   {
      _optrType = activity._optrType ;
      _parameters = activity._parameters ;
      _contextMonitor = activity._contextMonitor ;
      _returnOptions = activity._returnOptions ;
      _hitEnd = activity._hitEnd ;
      return (*this) ;
   }

   void _optQueryActivity::toBSON ( BSONObjBuilder &builder ) const
   {
      ossTickConversionFactor factor ;
      UINT32 seconds = 0, microseconds = 0 ;
      double queryTime = 0.0, executeTime = 0.0 ;
      CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestamp startTime( _contextMonitor.getStartTimestamp() ) ;

      _contextMonitor.getQueryTime().convertToTime( factor, seconds, microseconds ) ;
      queryTime = (double)( seconds ) +
                  (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;

      _contextMonitor.getExecuteTime().convertToTime( factor, seconds, microseconds ) ;
      executeTime = (double)( seconds ) +
                    (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;

      ossTimestampToString( startTime, timestampStr ) ;

      builder.append( OPT_FIELD_CONTEXT_ID, _contextMonitor.getContextID() ) ;

      switch ( _optrType )
      {
         case MON_UPDATE :
            builder.append( OPT_FIELD_QUERY_TYPE,
                            OPT_VALUE_QUERY_TYPE_UPDATE ) ;
            break ;
         case MON_DELETE :
            builder.append( OPT_FIELD_QUERY_TYPE,
                            OPT_VALUE_QUERY_TYPE_DELETE ) ;
            break ;
         default :
            builder.append( OPT_FIELD_QUERY_TYPE,
                            OPT_VALUE_QUERY_TYPE_SELECT ) ;
            break ;
      }

      if ( !_parameters.isEmpty() )
      {
         builder.append( _parameters.firstElement() ) ;
      }

      builder.append( OPT_FIELD_QUERY_START_TIME, timestampStr ) ;
      builder.append( OPT_FIELD_QUERY_TIME_SPENT, queryTime ) ;
      builder.append( OPT_FIELD_EXECUTE_TIME_SPENT, executeTime ) ;
      builder.append( OPT_FIELD_SELECTOR, _returnOptions.getSelector() ) ;
      builder.append( OPT_FIELD_SKIP, _returnOptions.getSkip() ) ;
      builder.append( OPT_FIELD_RETURN, _returnOptions.getLimit() ) ;
      builder.append( OPT_FIELD_FLAG, _returnOptions.getFlag() ) ;
      builder.append( OPT_FIELD_DATA_READ,
                      (INT64)_contextMonitor.getDataRead() );
      builder.append( OPT_FIELD_INDEX_READ,
                      (INT64)_contextMonitor.getIndexRead() ) ;
      builder.append( OPT_FIELD_GETMORES,
                      (INT32)_contextMonitor.getReturnBatches() ) ;
      builder.append( OPT_FIELD_RETURN_NUM,
                      (INT64)_contextMonitor.getReturnRecords() ) ;
      builder.appendBool( OPT_FIELD_HIT_END, _hitEnd ) ;
   }

   /*
      _optCLScanInfo implement
    */
   _optCLScanInfo::_optCLScanInfo ()
   : _optCollectionInfo(),
     _indexExtID( DMS_INVALID_EXTENT ),
     _indexLID( DMS_INVALID_EXTENT )
   {
      setCLFullName( NULL ) ;
   }

   _optCLScanInfo::_optCLScanInfo ( const _optCLScanInfo & info )
   : _optCollectionInfo( info ),
     _indexExtID( info._indexExtID ),
     _indexLID( info._indexLID )
   {
      setCLFullName( info._clFullName ) ;
   }

   _optCLScanInfo::~_optCLScanInfo ()
   {
   }

   void _optCLScanInfo::setCLFullName ( const CHAR *pCLFullName )
   {
      if ( NULL != pCLFullName )
      {
         ossStrncpy( _clFullName, pCLFullName,
                     DMS_COLLECTION_FULL_NAME_SZ ) ;
         _clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
      }
      else
      {
         _clFullName[0] = '\0' ;
      }
   }

   /*
      _optAccessPlanRuntime implement
    */
   _optAccessPlanRuntime::_optAccessPlanRuntime ()
   : _mthMatchRuntimeHolder(),
     _plan( NULL ),
     _apm( NULL ),
     _hasQueryActivity( FALSE ),
     _isNewPlan( FALSE ),
     _ownedPlanInfo( FALSE ),
     _clScanInfo( NULL )
   {
   }

   _optAccessPlanRuntime::~_optAccessPlanRuntime ()
   {
      reset() ;
   }

   void _optAccessPlanRuntime::reset ()
   {
      deleteMatchRuntime() ;
      deleteCLScanInfo() ;
      _isNewPlan = FALSE ;
      _hasQueryActivity = FALSE ;
      _apm = NULL ;
      releasePlan() ;
   }

   void _optAccessPlanRuntime::inheritRuntime ( _optAccessPlanRuntime *planRuntime )
   {
      SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;

      optAccessPlan * plan = planRuntime->_plan ;

      if ( NULL != plan )
      {
         plan->incRefCount() ;
         setPlan( plan, planRuntime->_apm, FALSE ) ;

         setMatchRuntime( planRuntime->getMatchRuntime() ) ;
         setCLScanInfo( planRuntime->getCLScanInfo() ) ;
      }
   }

   INT32 _optAccessPlanRuntime::createCLScanInfo ()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL == _clScanInfo, "_clScanInfo should be NULL" ) ;

      _clScanInfo = SDB_OSS_NEW optCLScanInfo() ;
      PD_CHECK( _clScanInfo, SDB_OOM, error, PDERROR,
                "Failed to allocate collection scan info" ) ;
      _ownedPlanInfo = TRUE ;

      done :
         return rc ;
      error :
         goto done ;
   }

   void _optAccessPlanRuntime::deleteCLScanInfo ()
   {
      if ( _ownedPlanInfo && NULL != _clScanInfo )
      {
         SDB_OSS_DEL _clScanInfo ;
      }
      _clScanInfo = NULL ;
      _ownedPlanInfo = FALSE ;
   }

   void _optAccessPlanRuntime::setCLScanInfo ( optCLScanInfo * clScanInfo )
   {
      deleteCLScanInfo() ;
      _clScanInfo = clScanInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPRTM_BINDPLANINFO, "_optAccessPlanRuntime::bindPlanInfo" )
   INT32 _optAccessPlanRuntime::bindPlanInfo ( const CHAR *pCLFullName,
                                               dmsStorageUnit *su,
                                               dmsMBContext *mbContext,
                                               dmsExtentID indexExtID,
                                               dmsExtentID indexLID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPRTM_BINDPLANINFO ) ;

      if ( NULL == _clScanInfo )
      {
         rc = createCLScanInfo() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create sub-collection scan info, "
                      "rc: %d", rc ) ;
      }

      _clScanInfo->setCSInfo( su ) ;
      _clScanInfo->setCLInfo( mbContext ) ;
      _clScanInfo->setCLFullName( pCLFullName ) ;
      _clScanInfo->setIndexExtID( indexExtID ) ;
      _clScanInfo->setIndexLID( indexLID ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPRTM_BINDPLANINFO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPRTM_BINDPARAMPLAN, "_optAccessPlanRuntime::bindParamPlan" )
   INT32 _optAccessPlanRuntime::bindParamPlan ( optAccessPlanHelper &planHelper,
                                                optAccessPlan *plan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPRTM_BINDPARAMPLAN ) ;

      SDB_ASSERT ( NULL != plan &&
                   plan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED,
                   "plan is invalid" ) ;

      mthMatchRuntime *matchRuntime = _matchRuntime ;
      if ( NULL == matchRuntime )
      {
         matchRuntime = plan->getMatchRuntime() ;
      }
      SDB_ASSERT( matchRuntime, "matchRuntime is invalid" ) ;

      rc = plan->bindMatchRuntime( matchRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind match runtime, rc: %d", rc ) ;

      if ( IXSCAN == plan->getScanType() &&
           NULL == matchRuntime->getPredList() &&
           !plan->getMatchRuntime()->isFixedPredList() )
      {
         rc = matchRuntime->generatePredList( planHelper.getPredicateSet(),
                                              plan->getKeyPattern(),
                                              plan->getDirection(),
                                              planHelper.getNormalizer() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate predicate list, rc: %d",
                      rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPRTM_BINDPARAMPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _optAccessPlanRuntime::releasePlan ()
   {
      if ( NULL != _plan )
      {
         _plan->release() ;
         _plan = NULL ;
      }
   }

   void _optAccessPlanRuntime::setQueryActivity ( MON_OPERATION_TYPES optrType,
                                                  const monContextCB &monCtxCB,
                                                  const rtnReturnOptions &returnOptions,
                                                  BOOLEAN hitEnd )
   {
      if ( canSetQueryActivity() )
      {
         optQueryActivity queryActivity( optrType, getParameters(),
                                         monCtxCB, returnOptions, hitEnd ) ;
         _apm->setQueryActivity( _plan->getActivityID(), queryActivity ) ;
         _hasQueryActivity = TRUE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPRTM_TOEXPPATH, "_optAccessPlanRuntime::toExplainPath" )
   INT32 _optAccessPlanRuntime::toExplainPath ( optExplainScanPath &expPath,
                                                const rtnContext *context ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPRTM_TOEXPPATH ) ;

      rc = expPath.copyScanPath( _plan->getScanPath(), context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to copy scan path, rc: %d", rc ) ;

      if ( NULL != _clScanInfo )
      {
         expPath.setCollectionName( _clScanInfo->getCLFullName() ) ;
         expPath.setIsMainCLPlan( TRUE ) ;
      }
      else
      {
         expPath.setCollectionName( _plan->getCLFullName() ) ;
      }

      expPath.setCacheLevel( _plan->isCached() ? _plan->getCacheLevel() :
                                                 OPT_PLAN_NOCACHE ) ;
      expPath.setIsNewPlan( _isNewPlan ) ;
      expPath.setSearchPaths( _plan->getSearchPaths() ) ;
      if ( NULL != getMatchTree() )
      {
         expPath.setMatchConfig( getMatchTree()->getMatchConfig() ) ;
      }
      if ( NULL != _apm )
      {
         expPath.setPlanConfig( _apm->getPlanConfig() ) ;
      }

      if ( !getParameters().isEmpty() )
      {
         expPath.setParameters( getParameters().toBSON() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPRTM_TOEXPPATH, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   const mthMatchRuntime * _optAccessPlanRuntime::getMatchRuntime () const
   {
      return _matchRuntime ? _matchRuntime :
                             ( _plan ? _plan->getMatchRuntime() :
                                       NULL ) ;
   }

   mthMatchRuntime * _optAccessPlanRuntime::getMatchRuntime ()
   {
      return _matchRuntime ? _matchRuntime :
                             ( _plan ? _plan->getMatchRuntime() :
                                       NULL ) ;
   }

   mthMatchRuntime * _optAccessPlanRuntime::getMatchRuntime ( BOOLEAN checkValid )
   {
      mthMatchRuntime *matchRuntime = getMatchRuntime() ;
      if ( checkValid && NULL != matchRuntime )
      {
         if ( !_plan->getMatchTree()->isInitialized() ||
              _plan->getMatchTree()->isMatchesAll() )
         {
            matchRuntime = NULL ;
         }
      }
      return matchRuntime ;
   }

   mthMatchTree * _optAccessPlanRuntime::getMatchTree ()
   {
      mthMatchRuntime * matchRuntime = getMatchRuntime() ;
      return matchRuntime ? matchRuntime->getMatchTree() : NULL ;
   }

   const mthMatchTree * _optAccessPlanRuntime::getMatchTree () const
   {
      const mthMatchRuntime * matchRuntime = getMatchRuntime() ;
      return matchRuntime ? matchRuntime->getMatchTree() : NULL ;
   }

   const rtnPredicateList * _optAccessPlanRuntime::getPredList () const
   {
      SDB_ASSERT ( _plan && _plan->isInitialized(),
                   "optAccessPlan must be optimized before start using" ) ;

      const mthMatchRuntime * planMatchRuntime = _plan->getMatchRuntime() ;
      const mthMatchRuntime * matchRuntime = getMatchRuntime() ;

      if ( NULL != planMatchRuntime && planMatchRuntime->isFixedPredList() )
      {
         return planMatchRuntime->getPredList() ;
      }
      else if ( NULL != matchRuntime )
      {
         return matchRuntime->getPredList() ;
      }

      return NULL ;
   }

   rtnPredicateList * _optAccessPlanRuntime::getPredList ()
   {
      SDB_ASSERT ( _plan && _plan->isInitialized(),
                   "optAccessPlan must be optimized before start using" ) ;

      mthMatchRuntime * planMatchRuntime = _plan->getMatchRuntime() ;
      mthMatchRuntime * matchRuntime = getMatchRuntime() ;

      if ( NULL != planMatchRuntime && planMatchRuntime->isFixedPredList() )
      {
         return planMatchRuntime->getPredList() ;
      }
      else if ( NULL != matchRuntime )
      {
         return matchRuntime->getPredList() ;
      }

      return NULL ;
   }

   rtnParamList & _optAccessPlanRuntime::getParameters ()
   {
      static rtnParamList s_emptyParameters ;
      mthMatchRuntime * matchRuntime = getMatchRuntime() ;
      return matchRuntime ? matchRuntime->getParameters() :
                            s_emptyParameters ;
   }

   const rtnParamList & _optAccessPlanRuntime::getParameters () const
   {
      static rtnParamList s_emptyParameters ;
      const mthMatchRuntime * matchRuntime = getMatchRuntime() ;
      return matchRuntime ? matchRuntime->getParameters() :
                            s_emptyParameters ;
   }

   BSONObj _optAccessPlanRuntime::getEqualityQueryObject ()
   {
      mthMatchRuntime * matchRuntime = getMatchRuntime() ;
      return matchRuntime ? matchRuntime->getEqualityQueryObject() :
                            BSONObj() ;
   }

   BSONObj _optAccessPlanRuntime::getParsedMatcher () const
   {
      const mthMatchTree * matchTree = getMatchTree() ;
      return matchTree ? matchTree->getParsedMatcher( getParameters() ) :
                         BSONObj() ;
   }

   BSONObj _optAccessPlanRuntime::getPredIXBound () const
   {
      const rtnPredicateList * predList = getPredList() ;
      return predList ? predList->getBound() : BSONObj() ;
   }

}
