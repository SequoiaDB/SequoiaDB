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

   Source File Name = optPlanPath.cpp

   Descriptive Name = Optimizer Access Plan Path

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for optimizer
   access plan creation. It will calculate based on rules and try to estimate
   a lowest cost plan to access data.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optPlanPath.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "msg.hpp"
#include "rtn.hpp"
#include "rtnContext.hpp"
#include "rtnContextData.hpp"
#include "rtnContextSort.hpp"
#include "rtnContextMainCL.hpp"
#include "coordContext.hpp"
#include <cmath>

using namespace bson ;

namespace engine
{

   /*
      _optPlanPath implement
    */
   _optPlanPath::_optPlanPath ()
   : _ownedPath( TRUE ),
     _pRootNode( NULL )
   {
   }

   _optPlanPath::~_optPlanPath ()
   {
      _deleteNodes() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANPATH__DELNODES, "_optPlanPath::_deleteNodes" )
   void _optPlanPath::_deleteNodes ()
   {
      PD_TRACE_ENTRY( SDB_OPTPLANPATH__DELNODES ) ;

      if ( _pRootNode && _ownedPath )
      {
         _pRootNode->deleteChildNodes() ;
         SDB_OSS_DEL _pRootNode ;
      }
      _pRootNode = NULL ;
      _ownedPath = TRUE ;

      PD_TRACE_EXIT( SDB_OPTPLANPATH__DELNODES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANPATH__SETROOTNODE, "_optPlanPath::_setRootNode" )
   void _optPlanPath::_setRootNode ( optPlanNode * pNode )
   {
      PD_TRACE_ENTRY( SDB_OPTPLANPATH__SETROOTNODE ) ;

      if ( NULL != pNode && _ownedPath )
      {
         pNode->addChildNode( _pRootNode ) ;
         _pRootNode = pNode ;
      }

      PD_TRACE_EXIT( SDB_OPTPLANPATH__SETROOTNODE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANPATH__SETPATH, "_optPlanPath::_setPath" )
   void _optPlanPath::_setPath ( const optPlanPath &path, BOOLEAN takeOwned )
   {
      PD_TRACE_ENTRY( SDB_OPTPLANPATH__SETPATH ) ;

      clearPath() ;
      _pRootNode = path._pRootNode ;
      if ( takeOwned && path._ownedPath )
      {
         _ownedPath = TRUE ;
         path._ownedPath = FALSE ;
      }
      else
      {
         _ownedPath = FALSE ;
      }

      PD_TRACE_EXIT( SDB_OPTPLANPATH__SETPATH ) ;
   }

   /*
      _optScanPath implement
    */
   _optScanPath::_optScanPath ()
   : _optPlanPath(),
     _pScanNode( NULL ),
     _sortRequired( FALSE )
   {
   }

   _optScanPath::_optScanPath ( const _optScanPath & path )
   : _optPlanPath(),
     _pScanNode( NULL ),
     _sortRequired( FALSE )
   {
      setPath( path, TRUE ) ;
   }


   _optScanPath::~_optScanPath ()
   {
   }

   optScanPath & _optScanPath::operator = ( const optScanPath & path )
   {
      setPath( path, TRUE ) ;
      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_CRTTBSCAN, "_optScanPath::createTbScan" )
   INT32 _optScanPath::createTbScan ( const CHAR * pCollection,
                                      const rtnQueryOptions & queryOptions,
                                      optAccessPlanHelper & planHelper,
                                      optCollectionStat * collectionStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_CRTTBSCAN ) ;

      SDB_ASSERT( pCollection, "pCollection is invalid" ) ;
      SDB_ASSERT( collectionStat, "collectionStat is invalid" ) ;

      optTbScanNode *pTbScan = NULL ;

      if ( _pScanNode )
      {
         _deleteNodes() ;
         _pScanNode = NULL ;
      }

      pTbScan = SDB_OSS_NEW
                      optTbScanNode( pCollection,
                                     planHelper.getOptCostThreshold() ) ;
      PD_CHECK( pTbScan, SDB_OOM, error, PDWARNING,
                "Failed to allocate optTbScanNode" ) ;

      _setRootNode( pTbScan ) ;
      _pScanNode = pTbScan ;
      _sortRequired = FALSE ;

      pTbScan->preEvaluate( queryOptions, planHelper, collectionStat ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_CRTTBSCAN, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pTbScan ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_CRTIXSCAN, "_optScanPath::createIxScan" )
   INT32 _optScanPath::createIxScan ( const CHAR * pCollection,
                                      const ixmIndexCB & indexCB,
                                      const rtnQueryOptions & queryOptions,
                                      optAccessPlanHelper & planHelper,
                                      OPT_PLAN_PATH_PRIORITY priority,
                                      optCollectionStat * collectionStat,
                                      optIndexStat * indexStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_CRTIXSCAN ) ;

      SDB_ASSERT( pCollection, "pCollection is invalid" ) ;
      SDB_ASSERT( collectionStat, "collectionStat is invalid" ) ;
      SDB_ASSERT( indexStat, "indexStat is invalid" ) ;

      optIxScanNode *pIdxScan = NULL ;

      if ( _pScanNode )
      {
         _deleteNodes() ;
         _pScanNode = NULL ;
      }

      pIdxScan = SDB_OSS_NEW
                        optIxScanNode( pCollection, indexCB,
                                       planHelper.getOptCostThreshold() ) ;
      PD_CHECK( pIdxScan, SDB_OOM, error, PDWARNING,
                "Failed to allocate optIxScanNode" ) ;

      _setRootNode( pIdxScan ) ;
      _pScanNode = pIdxScan ;
      _sortRequired = FALSE ;

      pIdxScan->preEvaluate( queryOptions, planHelper, priority,
                             collectionStat, indexStat ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_CRTIXSCAN, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pIdxScan ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_CRTSORT, "_optScanPath::createSortNode" )
   INT32 _optScanPath::createSortNode ( const rtnQueryOptions & queryOptions,
                                        UINT64 sortBufferSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_CRTSORT ) ;

      optSortNode *pSort = NULL ;

      PD_CHECK( _ownedPath, SDB_INVALIDARG, error, PDDEBUG,
                "Failed to create sort node : path is not owned" ) ;

      if ( queryOptions.isOrderByEmpty() || !_pRootNode )
      {
         goto done ;
      }

      pSort = SDB_OSS_NEW optSortNode() ;
      PD_CHECK( pSort, SDB_OOM, error, PDWARNING,
                "Failed to allocate optSortNode" ) ;

      _setRootNode( pSort ) ;

      pSort->preEvaluate( queryOptions, sortBufferSize ) ;
      pSort->evaluate() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_CRTSORT, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pSort ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_EVAL, "_optScanPath::evaluate" )
   INT32 _optScanPath::evaluate ( const rtnQueryOptions & options,
                                  UINT64 sortBufferSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_EVAL ) ;

      if ( !_pScanNode || !_pScanNode->isCandidate() )
      {
         goto done ;
      }

      // Sort is required
      if ( !options.isOrderByEmpty() &&
           !_pScanNode->isSorted() )
      {
         _pScanNode->evaluate() ;

         rc = createSortNode( options, sortBufferSize ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to evaluate sort, rc: %d", rc ) ;

         _sortRequired = TRUE ;
      }
      else
      {
         _pScanNode->evaluate() ;
         _sortRequired = FALSE ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_EVAL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_SETPATH, "_optScanPath::setPath" )
   void _optScanPath::setPath ( const optScanPath & path, BOOLEAN takeOwned )
   {
      PD_TRACE_ENTRY( SDB_OPTSCANPATH_SETPATH ) ;

      _optPlanPath::_setPath( path, takeOwned ) ;
      _pScanNode = path._pScanNode ;
      _sortRequired = path._sortRequired ;

      PD_TRACE_EXIT( SDB_OPTSCANPATH_SETPATH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_COPYPATH, "_optScanPath::copyPath" )
   INT32 _optScanPath::copyPath ( const optScanPath & path )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_COPYPATH ) ;

      clearPath() ;

      optPlanNode *lastDstNode = NULL ;
      const optPlanNode *srcNode = path._pRootNode ;

      while ( srcNode != NULL )
      {
         optPlanNode *newNode = NULL ;
         const optPlanNodeList &childNodes = srcNode->getChildNodes() ;

         SDB_ASSERT( childNodes.size() <= 1, "Should be a single path" ) ;

         switch ( srcNode->getType() )
         {
            case OPT_PLAN_TB_SCAN :
            {
               const optTbScanNode *tbScanNode =
                     dynamic_cast<const optTbScanNode *>( srcNode ) ;
               PD_CHECK( NULL != tbScanNode, SDB_SYS, error, PDERROR,
                         "Failed to get table scan node" ) ;
               newNode = SDB_OSS_NEW optTbScanNode( *tbScanNode, NULL ) ;
               break ;
            }
            case OPT_PLAN_IX_SCAN :
            {
               const optIxScanNode *ixScanNode =
                     dynamic_cast<const optIxScanNode *>( srcNode ) ;
               PD_CHECK( NULL != ixScanNode, SDB_SYS, error, PDERROR,
                         "Failed to get index scan node" ) ;
               newNode = SDB_OSS_NEW optIxScanNode( *ixScanNode, NULL ) ;
               break ;
            }
            case OPT_PLAN_SORT :
            {
               const optSortNode *sortNode =
                     dynamic_cast<const optSortNode *>( srcNode ) ;
               PD_CHECK( NULL != sortNode, SDB_SYS, error, PDERROR,
                         "Failed to get sort node" ) ;
               newNode = SDB_OSS_NEW optSortNode( *sortNode, NULL ) ;
               break ;
            }
            default :
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Unknown node type %d", srcNode->getType() ) ;
               goto error ;
         }

         PD_CHECK( newNode, SDB_OOM, error, PDWARNING,
                   "Failed to allocate optPlanNode" ) ;

         if ( NULL == _pRootNode )
         {
            SDB_ASSERT( NULL == lastDstNode, "lastDstNode is invalid" ) ;
            _setRootNode( newNode ) ;
         }
         else
         {
            lastDstNode->addChildNode( newNode ) ;
         }
         lastDstNode = newNode ;

         if ( childNodes.size () == 1 )
         {
            srcNode = childNodes.front() ;
         }
         else
         {
            srcNode = NULL ;
         }
      }

      // Set scan node
      if ( NULL != lastDstNode )
      {
         rc = _setScanNode( lastDstNode ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set scan node, rc: %d", rc ) ;
      }

      _sortRequired = path._sortRequired ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_COPYPATH, rc ) ;
      return rc ;

   error :
      clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_CLRSCANINFO, "_optScanPath::clearScanInfo" )
   INT32 _optScanPath::clearScanInfo ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_CLRSCANINFO ) ;

      if ( NULL != _pScanNode )
      {
         _pScanNode->setCollection( NULL ) ;
         if ( OPT_PLAN_IX_SCAN == _pScanNode->getType() )
         {
            optIxScanNode *ixScanNode =
                  dynamic_cast<optIxScanNode *>( _pScanNode ) ;
            PD_CHECK( ixScanNode, SDB_SYS, error, PDERROR,
                      "Failed to get index scan node" ) ;
            ixScanNode->setIndexExtID( DMS_INVALID_EXTENT ) ;
            ixScanNode->setIndexLID( DMS_INVALID_EXTENT ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_CLRSCANINFO, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH_TOBSONSEARCHPATH, "_optScanPath::toBSONSearchPath" )
   INT32 _optScanPath::toBSONSearchPath ( BSONArrayBuilder & builder,
                                          BOOLEAN isUsed,
                                          BOOLEAN needEvaluate ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH_TOBSONSEARCHPATH ) ;

      BSONObjBuilder pathBuilder( builder.subobjStart() ) ;

      pathBuilder.appendBool( OPT_FIELD_IS_USED, isUsed ) ;
      pathBuilder.appendBool( OPT_FIELD_IS_CANDIDATE, isCandidate() ) ;
      pathBuilder.append( OPT_FIELD_PLAN_SCORE, getSelectivity() ) ;
      if ( isCandidate() )
      {
         pathBuilder.append( OPT_FIELD_TOTAL_COST,
                             (INT64)getEstTotalCost() ) ;
      }
      pathBuilder.append( OPT_FIELD_SCAN_TYPE,
                          IXSCAN == getScanType() ? OPT_VALUE_IXSCAN :
                                                    OPT_VALUE_TBSCAN ) ;
      pathBuilder.append( OPT_FIELD_INDEX_NAME, getIndexName() ) ;
      pathBuilder.appendBool( OPT_FIELD_USE_EXT_SORT, isSortRequired() ) ;

      if ( IXSCAN == getScanType() )
      {
         pathBuilder.append( OPT_FIELD_DIRECTION, getDirection() ) ;
         pathBuilder.append( OPT_FIELD_IX_BOUND, getIXBound() ) ;
         pathBuilder.appendBool( OPT_FIELD_NEED_MATCH, isNeedMatch() ) ;
         pathBuilder.appendBool( OPT_FIELD_INDEX_COVER, isIndexCover() ) ;
         if ( NULL != _pScanNode )
         {
            rc = _pScanNode->toBSONIXStatInfo( pathBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for index "
                         "statistics information, rc: %d", rc ) ;
         }
      }

      if ( needEvaluate && isCandidate() )
      {
         if ( NULL != _pScanNode )
         {
            BSONObjBuilder scanBuilder(
                  pathBuilder.subobjStart( OPT_FIELD_SCAN_NODE ) ) ;
            rc = _pScanNode->toBSONEvaluation( scanBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for scan node "
                         "evaluation, rc: %d", rc ) ;
            scanBuilder.done() ;
         }
         if ( isSortRequired() && NULL != _pRootNode )
         {
            BSONObjBuilder scanBuilder(
                  pathBuilder.subobjStart( OPT_FIELD_SORT_NODE ) ) ;
            rc = _pRootNode->toBSONEvaluation( scanBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for sort node "
                         "evaluation, rc: %d", rc ) ;
            scanBuilder.done() ;
         }
      }

      pathBuilder.done() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH_TOBSONSEARCHPATH, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANPATH__SETSCANNODE, "_optScanPath::_setScanNode" )
   INT32 _optScanPath::_setScanNode ( optPlanNode * pNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANPATH__SETSCANNODE ) ;

      SDB_ASSERT( NULL != pNode, "pNode is invalid" ) ;

      PD_CHECK( OPT_PLAN_TB_SCAN == pNode->getType() ||
                OPT_PLAN_IX_SCAN == pNode->getType(),
                SDB_SYS, error, PDERROR, "No scan node is found" ) ;

      _pScanNode = dynamic_cast<optScanNode *>( pNode ) ;
      PD_CHECK( _pScanNode, SDB_SYS, error, PDERROR,
                "Failed to get scan node" ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANPATH__SETSCANNODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _optExplainPath implement
    */
   _optExplainPath::_optExplainPath ()
   : _optPlanPath(),
     _beginDataRead( 0 ),
     _beginIndexRead( 0 ),
     _beginUsrCpu( 0.0 ),
     _beginSysCpu( 0.0 ),
     _endDataRead( 0 ),
     _endIndexRead( 0 ),
     _endUsrCpu( 0.0 ),
     _endSysCpu( 0.0 )
   {
      _clFullName[ 0 ] = '\0' ;
   }

   _optExplainPath::~_optExplainPath ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_SETCLNAME, "_optExplainPath::setCollectionName" )
   void _optExplainPath::setCollectionName ( const CHAR * clFullName )
   {
      PD_TRACE_ENTRY( SDB_OPTEXPPATH_SETCLNAME ) ;

      if ( NULL != clFullName )
      {
         ossStrncpy( _clFullName, clFullName,
                     DMS_COLLECTION_FULL_NAME_SZ ) ;
         _clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
      }
      else
      {
         _clFullName[ 0 ] = '\0' ;
      }

      PD_TRACE_EXIT( SDB_OPTEXPPATH_SETCLNAME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_CLEARPATH, "_optExplainPath::clearPath" )
   void _optExplainPath::clearPath ()
   {
      PD_TRACE_ENTRY( SDB_OPTEXPPATH_CLEARPATH ) ;

      _optPlanPath::clearPath() ;

      PD_TRACE_EXIT( SDB_OPTEXPPATH_CLEARPATH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_TOBSON, "_optExplainPath::toBSON" )
   INT32 _optExplainPath::toBSON ( BSONObjBuilder & builder,
                                   const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPPATH_TOBSON ) ;

      if ( NULL != _pRootNode )
      {
         BSONObjBuilder pathBuilder( builder.subobjStart( OPT_FIELD_PLAN_PATH ) ) ;
         _pRootNode->toBSON( builder, expOptions ) ;
         pathBuilder.done() ;
      }
      else
      {
         PD_CHECK( _pRootNode, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get root node of explain path" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPPATH_TOBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_SETEXPSTART, "_optExplainPath::setExplainStart" )
   INT32 _optExplainPath::setExplainStart ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPPATH_SETEXPSTART ) ;

      ossTime userTime, sysTime ;

      /// get some info before explain
      _beginDataRead = cb->getMonAppCB()->totalDataRead ;
      _beginIndexRead = cb->getMonAppCB()->totalIndexRead ;
      ossGetCurrentTime( _beginTime ) ;

      /// get begin cpu time info
      ossGetCPUUsage( cb->getThreadHandle(), userTime, sysTime ) ;
      _beginUsrCpu = userTime.seconds + (FLOAT64)userTime.microsec / 1000000 ;
      _beginSysCpu = sysTime.seconds + (FLOAT64)sysTime.microsec / 1000000 ;

      PD_TRACE_EXITRC( SDB_OPTEXPPATH_SETEXPSTART, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_SETEXPEND, "_optExplainPath::setExplainEnd" )
   INT32 _optExplainPath::setExplainEnd ( rtnContext * context, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPPATH_SETEXPEND ) ;

      ossTime userTime, sysTime ;

      if ( NULL != _pRootNode )
      {
         _pRootNode->setRuntimeMonitor( *( context->getMonCB() ) ) ;
      }

      ossGetCurrentTime( _endTime ) ;
      _endDataRead = cb->getMonAppCB()->totalDataRead ;
      _endIndexRead = cb->getMonAppCB()->totalIndexRead ;

      /// get end cpu time info
      ossGetCPUUsage( cb->getThreadHandle(), userTime, sysTime ) ;
      _endUsrCpu = userTime.seconds + (FLOAT64)userTime.microsec / 1000000 ;
      _endSysCpu = sysTime.seconds + (FLOAT64)sysTime.microsec / 1000000 ;

      PD_TRACE_EXITRC( SDB_OPTEXPPATH_SETEXPEND, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPPATH_TOBSONEXPINFO, "_optExplainPath::toBSONExplainInfo" )
   INT32 _optExplainPath::toBSONExplainInfo ( BSONObjBuilder & builder,
                                              UINT16 mask ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPPATH_TOBSONEXPINFO ) ;

      // ReturnNum
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_RETURN_NUM ) )
      {
         if ( NULL != _pRootNode )
         {
            builder.appendNumber(
                  OPT_FIELD_RETURN_NUM,
                  (INT64)_pRootNode->getRuntimeMonitor().getReturnRecords() ) ;
         }
         else
         {
            builder.appendNumber( OPT_FIELD_RETURN_NUM, (INT64)0 ) ;
         }
      }

      // ElapsedTime
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_ELAPSED_TIME ) )
      {
         UINT64 beginTime = _beginTime.time * 1000000 + _beginTime.microtm  ;
         UINT64 endTime = _endTime.time * 1000000 + _endTime.microtm  ;
         builder.append( OPT_FIELD_ELAPSED_TIME,
                         FLOAT64( ( endTime - beginTime ) / 1000000.0 ) ) ;
      }

      // DataRead
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_DATA_READ ) )
      {
         builder.appendNumber( OPT_FIELD_DATA_READ,
                               (INT64)( _endDataRead - _beginDataRead ) ) ;
      }

      // IndexRead
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_INDEX_READ ) )
      {
         builder.appendNumber( OPT_FIELD_INDEX_READ,
                               (INT64)( _endIndexRead - _beginIndexRead ) ) ;
      }

      // UserCPU
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_USERCPU ) )
      {
         builder.append( OPT_FIELD_USERCPU, _endUsrCpu - _beginUsrCpu ) ;
      }

      // SysCPU
      if ( OSS_BIT_TEST( mask, OPT_EXPINFO_MASK_SYSCPU ) )
      {
         builder.append( OPT_FIELD_SYSCPU, _endSysCpu - _beginSysCpu ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTEXPPATH_TOBSONEXPINFO, rc ) ;

      return rc ;
   }

   /*
      _optExplainScanPath implement
    */
   _optExplainScanPath::_optExplainScanPath ()
   : _optExplainPath(),
     _mthMatchConfigHolder(),
     _optAccessPlanConfigHolder(),
     _pScanNode( NULL ),
     _isNewPlan( FALSE ),
     _isMainCLPlan( FALSE ),
     _cacheLevel( OPT_PLAN_NOCACHE ),
     _searchPaths( NULL ),
     _needSearch( FALSE ),
     _needEvaluate( FALSE )
   {
   }

   _optExplainScanPath::~_optExplainScanPath ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH_CLEARPATH, "_optExplainScanPath::clearPath" )
   void _optExplainScanPath::clearPath ()
   {
      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH_CLEARPATH ) ;

      _optExplainPath::clearPath() ;

      _pScanNode = NULL ;
      _isNewPlan = FALSE ;
      _cacheLevel = OPT_PLAN_NOCACHE ;
      _parameters = BSONObj() ;

      _searchPaths = NULL ;

      PD_TRACE_EXIT( SDB_OPTEXPSCANPATH_CLEARPATH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH_COPYSCANPATH, "_optExplainScanPath::copyScanPath" )
   INT32 _optExplainScanPath::copyScanPath ( const optScanPath & scanPath,
                                             const rtnContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH_COPYSCANPATH ) ;

      optPlanNode *lastDstNode = NULL ;
      const optPlanNode *srcNode = NULL ;
      const rtnContext *curContext = NULL ;

      clearPath() ;

      srcNode = scanPath.getRootNode() ;
      curContext = context ;

      while ( srcNode != NULL )
      {
         optPlanNode *newNode = NULL ;
         const optPlanNodeList &childNodes = srcNode->getChildNodes() ;

         SDB_ASSERT( childNodes.size() <= 1, "Should be a single path" ) ;

         switch ( srcNode->getType() )
         {
            case OPT_PLAN_TB_SCAN :
            {
               rc = _copyTbScanNode( srcNode, curContext, &newNode ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create TBSCAN node, "
                            "rc: %d", rc ) ;
               break ;
            }
            case OPT_PLAN_IX_SCAN :
            {
               rc = _copyIxScanNode( srcNode, curContext, &newNode ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create IXSCAN node, "
                            "rc: %d", rc ) ;
               break ;
            }
            case OPT_PLAN_SORT :
            {
               rc = _copySortNode( srcNode, curContext, &newNode, &curContext ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create SORT node, "
                            "rc: %d", rc ) ;
               break ;
            }
            default :
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Unknown node type %d", srcNode->getType() ) ;
               goto error ;
         }

         PD_CHECK( newNode, SDB_OOM, error, PDWARNING,
                   "Failed to allocate optPlanNode" ) ;

         if ( NULL == _pRootNode )
         {
            SDB_ASSERT( NULL == lastDstNode, "lastDstNode is invalid" ) ;
            _setRootNode( newNode ) ;
         }
         else
         {
            lastDstNode->addChildNode( newNode ) ;
            if ( lastDstNode->needReEvaluate() )
            {
               lastDstNode->evaluate() ;
            }
         }
         lastDstNode = newNode ;

         srcNode = childNodes.size() == 1 ? childNodes.front() : NULL ;
      }

      if ( lastDstNode && lastDstNode->needReEvaluate() )
      {
         lastDstNode->evaluate() ;
      }

      rc = _setScanNode( lastDstNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set scan node, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH_COPYSCANPATH, rc ) ;
      return rc ;

   error :
      clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__COPYTBSCAN, "_optExplainScanPath::_copyTbScanNode" )
   INT32 _optExplainScanPath::_copyTbScanNode ( const optPlanNode * srcNode,
                                                const rtnContext * srcContext,
                                                optPlanNode ** dstNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__COPYTBSCAN ) ;

      SDB_ASSERT( dstNode, "dstNode is invalid" ) ;

      optPlanNode *newNode = NULL ;
      const optTbScanNode *tbScanNode = NULL ;
      const rtnContextData *dataContext = NULL ;

      tbScanNode = dynamic_cast<const optTbScanNode *>( srcNode ) ;
      PD_CHECK( NULL != tbScanNode, SDB_SYS, error, PDERROR,
                "Failed to get table scan node" ) ;

      dataContext = dynamic_cast<const rtnContextData *>( srcContext ) ;
      PD_CHECK( NULL != dataContext, SDB_SYS, error, PDERROR,
                "Failed to get data context" ) ;

      newNode = SDB_OSS_NEW optTbScanNode( *tbScanNode, srcContext ) ;
      PD_CHECK( NULL != newNode, SDB_OOM, error, PDERROR,
                "Failed to allocate node" ) ;

      (*dstNode) = newNode ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__COPYTBSCAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__COPYIXSCAN, "_optExplainScanPath::_copyIxScanNode" )
   INT32 _optExplainScanPath::_copyIxScanNode ( const optPlanNode * srcNode,
                                                const rtnContext * srcContext,
                                                optPlanNode ** dstNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__COPYIXSCAN ) ;

      SDB_ASSERT( dstNode, "dstNode is invalid" ) ;

      optPlanNode *newNode = NULL ;
      const optIxScanNode *ixScanNode = NULL ;
      const rtnContextData *dataContext = NULL ;

      ixScanNode = dynamic_cast<const optIxScanNode *>( srcNode ) ;
      PD_CHECK( NULL != ixScanNode, SDB_SYS, error, PDERROR,
                "Failed to get index scan node" ) ;

      dataContext = dynamic_cast<const rtnContextData *>( srcContext ) ;
      PD_CHECK( NULL != dataContext, SDB_SYS, error, PDERROR,
                "Failed to get data context" ) ;

      newNode = SDB_OSS_NEW optIxScanNode( *ixScanNode, srcContext ) ;
      PD_CHECK( NULL != newNode, SDB_OOM, error, PDERROR,
                "Failed to allocate node" ) ;

      (*dstNode) = newNode ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__COPYIXSCAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__COPYSORT, "_optExplainScanPath::_copySortNode" )
   INT32 _optExplainScanPath::_copySortNode ( const optPlanNode * srcNode,
                                              const rtnContext * srcContext,
                                              optPlanNode ** dstNode,
                                              const rtnContext ** subContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__COPYSORT ) ;

      SDB_ASSERT( dstNode, "dstNode is invalid" ) ;
      SDB_ASSERT( subContext, "subContext is invalid" ) ;

      optPlanNode *newNode = NULL ;
      const optSortNode *sortNode = NULL ;
      const rtnContextSort *sortContext = NULL ;

      sortNode = dynamic_cast<const optSortNode *>( srcNode ) ;
      PD_CHECK( NULL != sortNode, SDB_SYS, error, PDERROR,
                "Failed to get sort node" ) ;
      sortContext = dynamic_cast<const rtnContextSort *>( srcContext ) ;
      PD_CHECK( NULL != sortContext, SDB_SYS, error, PDERROR,
                "Failed to get sort context" ) ;
      newNode = SDB_OSS_NEW optSortNode( *sortNode, srcContext ) ;
      PD_CHECK( NULL != newNode, SDB_OOM, error, PDERROR,
                "Failed to allocate node" ) ;

      (*dstNode) = newNode ;
      (*subContext) = sortContext->getSubContext() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__COPYSORT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__SETSCANNODE, "_optExplainScanPath::_setScanNode" )
   INT32 _optExplainScanPath::_setScanNode ( optPlanNode * pNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__SETSCANNODE ) ;

      SDB_ASSERT( NULL != pNode, "pNode is invalid" ) ;

      PD_CHECK( OPT_PLAN_TB_SCAN == pNode->getType() ||
                OPT_PLAN_IX_SCAN == pNode->getType(),
                SDB_SYS, error, PDERROR, "No merge node is found" ) ;

      _pScanNode = dynamic_cast<optScanNode *>( pNode ) ;
      PD_CHECK( _pScanNode, SDB_SYS, error, PDERROR,
                "Failed to set MERGE node" ) ;

      _pScanNode->setCollection( _clFullName ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__SETSCANNODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH_TOBSON, "_optExplainScanPath::toBSON" )
   INT32 _optExplainScanPath::toBSON ( BSONObjBuilder & builder,
                                       const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH_TOBSON ) ;

      rtnExplainOptions tempOptions( expOptions ) ;
      tempOptions.setNeedFlatten( FALSE ) ;

      rc = _toBSONPlanInfo( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for plan information, "
                   "rc: %d", rc ) ;

      rc = confToBSON( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for match configuration, "
                   "rc: %d", rc ) ;

      rc = _optExplainPath::toBSON( builder, tempOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to output BSON from explain path, "
                      "rc: %d", rc ) ;

      if ( _needSearch && NULL != _searchPaths && !_searchPaths->empty() )
      {
         BSONObjBuilder searchBuilder( builder.subobjStart( OPT_FIELD_SEARCH ) ) ;

         rc = _toBSONSearchParameters( searchBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for search "
                      "parameters, rc: %d", rc ) ;

         BSONArrayBuilder pathsBuilder(
               searchBuilder.subarrayStart( OPT_FIELD_SEARCH_PATHS ) ) ;

         for ( optScanPathList::const_iterator iter = _searchPaths->begin() ;
               iter != _searchPaths->end() ; iter ++ )
         {
            const optScanPath &scanPath = ( *iter ) ;
            BOOLEAN isUsed =
                  ( scanPath.getScanType() == _pScanNode->getScanType() &&
                    ( ( _pScanNode->getScanType() == IXSCAN &&
                        0 == ossStrcmp( scanPath.getIndexName(),
                                        _pScanNode->getIndexName() ) ) ||
                      ( _pScanNode->getScanType() == TBSCAN ) ) ) ;

            rc = scanPath.toBSONSearchPath( pathsBuilder, isUsed,
                                            _needEvaluate ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for search path, "
                         "rc: %d", rc ) ;
         }

         pathsBuilder.done() ;

         searchBuilder.done() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH_TOBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH_TOBSONBASIC, "_optExplainScanPath::toBSONBasic" )
   INT32 _optExplainScanPath::toBSONBasic ( BSONObjBuilder & builder,
                                            const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH_TOBSONBASIC ) ;

      if ( NULL != _pScanNode )
      {
         optScanType scanType = _pScanNode->getScanType() ;

         builder.append( OPT_FIELD_NAME, _pScanNode->getCollection() ) ;
         builder.append( OPT_FIELD_SCAN_TYPE,
                         IXSCAN == scanType ? OPT_VALUE_IXSCAN :
                                              OPT_VALUE_TBSCAN ) ;
         builder.append( OPT_FIELD_INDEX_NAME, _pScanNode->getIndexName() ) ;
         builder.appendBool( OPT_FIELD_USE_EXT_SORT,
                             OPT_PLAN_SORT == _pRootNode->getType() ) ;

         builder.appendEx( OPT_FIELD_QUERY,
                           _pScanNode->getMatcher(),
                           expOptions.getBuilderOption() ) ;

         if ( IXSCAN == scanType &&
              !_pScanNode->getIXBound().isEmpty() )
         {
            builder.append( OPT_FIELD_IX_BOUND, _pScanNode->getIXBound() ) ;
         }
         else
         {
            builder.appendNull( OPT_FIELD_IX_BOUND ) ;
         }

         builder.appendBool( OPT_FIELD_NEED_MATCH, _pScanNode->isNeedMatch() ) ;
         builder.appendBool( OPT_FIELD_INDEX_COVER, _pScanNode->isIndexCover() ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH_TOBSONBASIC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__TOBSONPLANINFO, "_optExplainScanPath::_toBSONPlanInfo" )
   INT32 _optExplainScanPath::_toBSONPlanInfo ( BSONObjBuilder & builder,
                                                const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__TOBSONPLANINFO ) ;

      if ( _cacheLevel > OPT_PLAN_NOCACHE )
      {
         builder.append( OPT_FIELD_CACHE_STATUS,
                         _isNewPlan ? OPT_VALUE_CACHE_STATUS_NEWCACHE :
                                      OPT_VALUE_CACHE_STATUS_HITCACHE ) ;
         builder.appendBool( OPT_FIELD_MAINCL_PLAN, _isMainCLPlan ) ;
         builder.append( OPT_FIELD_CACHE_LEVEL,
                         optAccessPlanKey::getCacheLevelName( _cacheLevel ) ) ;
         if ( !_parameters.isEmpty() )
         {
            builder.appendEx( _parameters.firstElement(),
                              expOptions.getBuilderOption() ) ;
         }
      }
      else
      {
         builder.append( OPT_FIELD_CACHE_STATUS,
                         OPT_VALUE_CACHE_STATUS_NOCACHE ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__TOBSONPLANINFO, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__TOBSONSEARCHPARAMS, "_optExplainScanPath::_toBSONSearchParameters" )
   INT32 _optExplainScanPath::_toBSONSearchParameters ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__TOBSONSEARCHPARAMS ) ;

      rc = _toBSONSearchOptions( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for options of "
                   "search path, rc: %d", rc ) ;

      if ( _needEvaluate )
      {
         rc = _toBSONSearchConstants( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for constants of "
                      "search path, rc: %d", rc ) ;

         rc = _toBSONSearchInputs( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for inputs of "
                      "search path, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__TOBSONSEARCHPARAMS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__TOBSONSEARCHCONSTS, "_optExplainScanPath::_toBSONSearchConstants" )
   INT32 _optExplainScanPath::_toBSONSearchConstants ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__TOBSONSEARCHCONSTS ) ;

      BSONObjBuilder constantBuilder(
                        builder.subobjStart( OPT_FIELD_CONSTANTS ) ) ;

      constantBuilder.append( OPT_FIELD_RAN_IO_COST,
                              OPT_RANDOM_SCAN_IO_COST ) ;
      constantBuilder.append( OPT_FIELD_SEQ_IO_COST,
                              OPT_SEQ_SCAN_IO_COST ) ;
      constantBuilder.append( OPT_FIELD_SEQ_WRITE_IO_COST,
                              OPT_SEQ_WRITE_IO_COST ) ;
      constantBuilder.append( OPT_FIELD_PAGE_UINT,
                              DMS_PAGE_SIZE_BASE ) ;
      constantBuilder.append( OPT_FIELD_REC_CPU_COST,
                              OPT_RECORD_CPU_COST ) ;
      constantBuilder.append( OPT_FIELD_IDX_CPU_COST,
                              OPT_IDX_CPU_COST ) ;
      constantBuilder.append( OPT_FIELD_OPTR_CPU_COST,
                              OPT_OPTR_BASE_CPU_COST ) ;
      constantBuilder.append( OPT_FIELD_IO_CPU_RATE,
                              OPT_IO_CPU_RATE ) ;
      constantBuilder.append( OPT_FIELD_TB_START_COST,
                              OPT_TBSCAN_DEFAULT_START_COST ) ;
      constantBuilder.append( OPT_FIELD_IX_START_COST,
                              OPT_IXSCAN_DEFAULT_START_COST ) ;

      constantBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__TOBSONSEARCHCONSTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__TOBSONSEARCHOPTIONS, "_optExplainScanPath::_toBSONSearchOptions" )
   INT32 _optExplainScanPath::_toBSONSearchOptions ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__TOBSONSEARCHOPTIONS ) ;

      BSONObjBuilder optionBuilder(
                        builder.subobjStart( OPT_FIELD_OPTIONS ) ) ;

      optionBuilder.append( OPT_FIELD_SORT_BUFF_SIZE,
                            getSortBufferSizeMB() ) ;
      optionBuilder.append( OPT_FIELD_OPT_COST_THRESHOLD,
                            getOptCostThreshold() ) ;

      optionBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__TOBSONSEARCHOPTIONS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPSCANPATH__TOBSONSEARCHINPUTS, "_optExplainScanPath::_toBSONSearchInputs" )
   INT32 _optExplainScanPath::_toBSONSearchInputs ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPSCANPATH__TOBSONSEARCHINPUTS ) ;

      if ( NULL != _pScanNode )
      {
         BSONObjBuilder inputBuilder(
                           builder.subobjStart( OPT_FIELD_INPUT ) ) ;

         inputBuilder.append( OPT_FIELD_PAGES,
                              (INT32)_pScanNode->getInputPages() ) ;
         inputBuilder.append( OPT_FIELD_RECORDS,
                              (INT64)_pScanNode->getInputRecords() ) ;
         inputBuilder.append( OPT_FIELD_RECORD_SIZE,
                              (INT32)_pScanNode->getInputRecordSize() ) ;
         inputBuilder.appendBool( OPT_FIELD_NEED_EVAL_IO,
                                  _pScanNode->needEvalIOCost() ) ;

         rc = _pScanNode->toBSONCLStatInfo( inputBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for collection "
                      "statistics information, rc: %d", rc ) ;

         inputBuilder.done() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPSCANPATH__TOBSONSEARCHINPUTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _optExplainMergePathBase implement
    */
   _optExplainMergePathBase::_optExplainMergePathBase ()
   : _optExplainPath(),
     _pMergeNode( NULL )
   {
   }

   _optExplainMergePathBase::~_optExplainMergePathBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATHBASE_CLEARPATH, "_optExplainMergePathBase::clearPath" )
   void _optExplainMergePathBase::clearPath ()
   {
      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATHBASE_CLEARPATH ) ;

      _optExplainPath::clearPath() ;

      _pMergeNode = NULL ;

      PD_TRACE_EXIT( SDB_OPTEXPMERGEPATHBASE_CLEARPATH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATHBASE_EVALUATE, "_optExplainMergePathBase::evaluate" )
   INT32 _optExplainMergePathBase::evaluate ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATHBASE_EVALUATE ) ;

      PD_CHECK( NULL != _pMergeNode, SDB_SYS, error, PDERROR,
                "Failed to evaluate MERGE node" ) ;

      _pMergeNode->evaluate() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPMERGEPATHBASE_EVALUATE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATHBASE__SETMERGENODE, "_optExplainMergePathBase::_setMergeNode" )
   INT32 _optExplainMergePathBase::_setMergeNode ( optPlanNode *pNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATHBASE__SETMERGENODE ) ;

      SDB_ASSERT( NULL != pNode, "pNode is invalid" ) ;

      PD_CHECK( OPT_PLAN_MERGE == pNode->getType() ||
                OPT_PLAN_COORD == pNode->getType(),
                SDB_SYS, error, PDERROR, "No merge node is found" ) ;

      _pMergeNode = dynamic_cast<optMergeNodeBase *>( pNode ) ;
      PD_CHECK( _pMergeNode, SDB_SYS, error, PDERROR,
                "Failed to set MERGE node" ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPMERGEPATHBASE__SETMERGENODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _optExplainMergePath implement
    */
   _optExplainMergePath::_optExplainMergePath ()
   : _optExplainMergePathBase()
   {
   }

   _optExplainMergePath::~_optExplainMergePath ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATH_CRTMERGEPATH, "_optExplainMergePath::createMergePath" )
   INT32 _optExplainMergePath::createMergePath ( const rtnContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATH_CRTMERGEPATH ) ;

      optPlanNode *newNode = NULL ;
      const rtnContextMainCL *mainCLContext = NULL ;

      clearPath() ;

      mainCLContext = dynamic_cast<const rtnContextMainCL *>( context ) ;
      PD_CHECK( mainCLContext, SDB_SYS, error, PDERROR,
                "Failed to get main-collection context" ) ;

      rc = _createMergeNode( context, &newNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create MERGE node, rc: %d", rc ) ;

      rc = _setMergeNode( newNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set MERGE node, rc: %d", rc ) ;

      _setRootNode( newNode ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPMERGEPATH_CRTMERGEPATH, rc ) ;
      return rc ;

   error :
      clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATH__CRTMERGENODE, "_optExplainMergePath::_createMergeNode" )
   INT32 _optExplainMergePath::_createMergeNode ( const rtnContext * srcContext,
                                                  optPlanNode ** dstNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATH__CRTMERGENODE ) ;

      SDB_ASSERT( dstNode, "dstNode is invalid" ) ;

      optPlanNode *newNode = NULL ;
      const rtnContextMainCL *mainCLContext = NULL ;

      mainCLContext = dynamic_cast<const rtnContextMainCL *>( srcContext ) ;
      PD_CHECK( NULL != mainCLContext, SDB_SYS, error, PDERROR,
                "Failed to get main-collection context" ) ;
      newNode = SDB_OSS_NEW optMainCLMergeNode( srcContext ) ;
      PD_CHECK( NULL != newNode, SDB_OOM, error, PDERROR,
                "Failed to allocate MERGE node" ) ;

      (*dstNode) = newNode ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPMERGEPATH__CRTMERGENODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPMERGEPATH_TOSIMPLEBSON, "_optExplainMergePath::toSimpleBSON" )
   INT32 _optExplainMergePath::toSimpleBSON ( BSONObjBuilder & builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPMERGEPATH_TOSIMPLEBSON ) ;

      builder.append( OPT_FIELD_NAME, getCollectionName() ) ;

      rc = _pMergeNode->toSimpleBSON( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for simple explain, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPMERGEPATH_TOSIMPLEBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _optExplainCoordPath implement
    */
   _optExplainCoordPath::_optExplainCoordPath ()
   : _optExplainMergePathBase()
   {
   }

   _optExplainCoordPath::~_optExplainCoordPath ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPCOORDPATH_CRTCOORDPATH, "_optExplainCoordPath::createCoordPath" )
   INT32 _optExplainCoordPath::createCoordPath ( const rtnContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPCOORDPATH_CRTCOORDPATH ) ;

      optPlanNode *newNode = NULL ;
      const rtnContextCoord *coordContext = NULL ;

      clearPath() ;

      coordContext = dynamic_cast<const rtnContextCoord *>( context ) ;
      PD_CHECK( coordContext, SDB_SYS, error, PDERROR,
                "Failed to get coord context" ) ;

      rc = _createCoordNode( context, &newNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create COORD node, rc: %d", rc ) ;

      rc = _setMergeNode( newNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set MERGE node, rc: %d", rc ) ;

      _setRootNode( newNode ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPCOORDPATH_CRTCOORDPATH, rc ) ;
      return rc ;

   error :
      clearPath() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPCOORDPATH_SORTCHILDEXPS, "_optExplainCoordPath::sortChildExplains" )
   INT32 _optExplainCoordPath::sortChildExplains ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPCOORDPATH_SORTCHILDEXPS ) ;

      SDB_ASSERT( NULL != _pMergeNode, "merge node is invalid" ) ;

      optExplainResultList tmpExplainList ;
      optExplainResultList & childExplainList = _pMergeNode->getChildExplains() ;

      while ( !childExplainList.empty() )
      {
         try
         {
            BSONElement nodeElement ;
            const CHAR * nodeName = NULL ;

            BSONObj explainResult = childExplainList.front() ;
            tmpExplainList.push_back( explainResult ) ;
            childExplainList.pop_front() ;

            nodeName = explainResult.getStringField( OPT_FIELD_NODE_NAME ) ;

            if ( NULL != nodeName && nodeName[0] != '\0' )
            {
               optExplainResultList::iterator iter = childExplainList.begin() ;

               while ( iter != childExplainList.end() )
               {
                  BSONObj subExplainResult = ( *iter ) ;
                  const CHAR * subNodeName =
                        subExplainResult.getStringField( OPT_FIELD_NODE_NAME ) ;

                  if ( NULL != subNodeName && subNodeName[0] != '\0' &&
                       0 == ossStrcmp( nodeName, subNodeName ) )
                  {
                     tmpExplainList.push_back( subExplainResult ) ;
                     iter = childExplainList.erase( iter ) ;
                  }
                  else
                  {
                     iter ++ ;
                  }
               }
            }
         }
         catch ( std::exception & e )
         {
            PD_LOG( PDERROR, "Failed to prepare explain, received unexpected "
                    "error: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      while ( !tmpExplainList.empty() )
      {
         childExplainList.push_back( tmpExplainList.front() ) ;
         tmpExplainList.pop_front() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPCOORDPATH_SORTCHILDEXPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTEXPCOORDPATH__CRTCOORDNODE, "_optExplainCoordPath::_createCoordNode" )
   INT32 _optExplainCoordPath::_createCoordNode ( const rtnContext * srcContext,
                                                  optPlanNode ** dstNode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTEXPCOORDPATH__CRTCOORDNODE ) ;

      SDB_ASSERT( dstNode, "dstNode is invalid" ) ;

      optPlanNode *newNode = NULL ;
      const rtnContextCoord *coordContext = NULL ;

      coordContext = dynamic_cast<const rtnContextCoord *>( srcContext ) ;
      PD_CHECK( NULL != coordContext, SDB_SYS, error, PDERROR,
                "Failed to get coord context" ) ;
      newNode = SDB_OSS_NEW optCoordMergeNode( srcContext ) ;
      PD_CHECK( NULL != newNode, SDB_OOM, error, PDERROR,
                "Failed to allocate COORD-MERGE node" ) ;

      (*dstNode) = newNode ;

   done :
      PD_TRACE_EXITRC( SDB_OPTEXPCOORDPATH__CRTCOORDNODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
