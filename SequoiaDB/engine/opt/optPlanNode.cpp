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

   Source File Name = optPlanNode.cpp

   Descriptive Name = Optimizer Access Plan Node

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for optimizer
   access plan creation. It will calculate based on rules and try to estimate
   a lowest cost plan to access data.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          /14/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optPlanNode.hpp"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "msg.hpp"
#include "rtn.hpp"
#include "rtnContext.hpp"
#include "rtnContextData.hpp"
#include "rtnContextSort.hpp"
#include "rtnContextMain.hpp"
#include <cmath>

using namespace bson;

namespace engine
{

   /*
      _optPlanNode implement
    */
   _optPlanNode::_optPlanNode ()
   : _estStartCost( 0 ),
     _estRunCost( 0 ),
     _estTotalCost( 0 ),
     _outputRecords( 0 ),
     _outputRecordSize( 0 ),
     _outputNumFields( 0 ),
     _estIOCost( 0 ),
     _estCPUCost( 0 ),
     _sorted( FALSE )
   {
   }

   _optPlanNode::_optPlanNode ( const optPlanNode & node,
                                const rtnContext * context )
   : _estStartCost( node._estStartCost ),
     _estRunCost( node._estRunCost ),
     _estTotalCost( node._estTotalCost ),
     _outputRecords( node._outputRecords ),
     _outputRecordSize( node._outputRecordSize ),
     _outputNumFields( node._outputNumFields ),
     _estIOCost( node._estIOCost ),
     _estCPUCost( node._estCPUCost ),
     _sorted( node._sorted ),
     _returnOptions( node._returnOptions ),
     _runtimeMonitor( NULL == context ? node._runtimeMonitor :
                                        ( *( context->getMonCB() ) ) )
   {
   }

   _optPlanNode::~_optPlanNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE_ADDCHILDNODE, "_optPlanNode::addChildNode" )
   void _optPlanNode::addChildNode ( _optPlanNode *pChildNode )
   {
      PD_TRACE_ENTRY( SDB_OPTPLANNODE_ADDCHILDNODE ) ;
      if ( NULL != pChildNode )
      {
         _childNodes.push_back( pChildNode ) ;
      }
      PD_TRACE_EXIT( SDB_OPTPLANNODE_ADDCHILDNODE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE_DELCHILDNODES, "_optPlanNode::deleteChildNodes" )
   void _optPlanNode::deleteChildNodes ()
   {
      PD_TRACE_ENTRY( SDB_OPTPLANNODE_DELCHILDNODES ) ;

      optPlanNodeList::iterator iter = _childNodes.begin() ;

      while ( iter != _childNodes.end() )
      {
         _optPlanNode *pChildNode = ( *iter ) ;
         iter = _childNodes.erase( iter ) ;

         pChildNode->deleteChildNodes() ;
         SDB_OSS_DEL pChildNode ;
      }

      PD_TRACE_EXIT( SDB_OPTPLANNODE_DELCHILDNODES ) ;
   }

   void _optPlanNode::_setReturnSelector ( const rtnContext * context )
   {
      if ( NULL != context )
      {
         if ( context->getSelector().isInitialized() )
         {
            _returnOptions.setSelector( context->getSelector().getPattern() ) ;
         }
         else
         {
            _returnOptions.setSelector( BSONObj() ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__EVALOUTREC, "_optPlanNode::_evaluateOutputRecords" )
   UINT64 _optPlanNode::_evaluateOutputRecords ( UINT64 inputRecords,
                                                 UINT64 &outputSkipRecords )
   {
      UINT64 outputRecords = 0 ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__EVALOUTREC ) ;

      if ( _returnOptions.getSkip() > 0 || _returnOptions.getLimit() >= 0 )
      {
         if ( _returnOptions.getLimit() == 0 )
         {
            // No return records
            outputRecords = 0 ;
            outputSkipRecords = 0 ;
         }
         else if ( inputRecords < (UINT64)_returnOptions.getSkip() )
         {
            // All input records are skipped
            outputRecords = 0 ;
            outputSkipRecords = inputRecords ;
         }
         else if ( inputRecords < (UINT64)( _returnOptions.getLimit() +
                                            _returnOptions.getSkip() ) )
         {
            // Input records could not cover limit and skip options
            outputRecords = inputRecords - _returnOptions.getSkip() ;
            outputSkipRecords = _returnOptions.getSkip() ;
         }
         else if ( _returnOptions.getLimit() > 0 )
         {
            // Limit options is given
            outputRecords = _returnOptions.getLimit() ;
            outputSkipRecords = _returnOptions.getSkip() ;
         }
         else
         {
            // Limit options is not given
            outputRecords = inputRecords - _returnOptions.getSkip() ;
            outputSkipRecords = _returnOptions.getSkip() ;
         }
      }
      else
      {
         // No limit or skip options
         outputRecords = inputRecords ;
         outputSkipRecords = 0 ;
      }

      PD_TRACE_EXIT( SDB_OPTPLANNODE__EVALOUTREC ) ;

      return outputRecords ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE_TOBSON, "_optPlanNode::toBSON" )
   INT32 _optPlanNode::toBSON ( BSONObjBuilder & builder,
                                const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE_TOBSON ) ;

      rc = _toBSONBasic( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for basic information, "
                   "rc: %d", rc ) ;

      if ( expOptions.isNeedEstimate() )
      {
         rc = _toBSONEstimate( builder, expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for estimate "
                      "information, rc: %d", rc ) ;
      }

      if ( expOptions.isNeedRun() )
      {
         rc = _toBSONRun( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                      "rc: %d", rc ) ;
      }

      if ( !expOptions.isNeedFlatten() )
      {
         rc = _toBSONChildNodes( builder, expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for child nodes, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE_TOBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE_FROMBSON, "_optPlanNode::fromBSON" )
   INT32 _optPlanNode::fromBSON ( const BSONObj & object,
                                  const rtnExplainOptions &expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE_FROMBSON ) ;

      rc = _fromBSONBasic( object ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for basic information, "
                   "rc: %d", rc ) ;

      if ( expOptions.isNeedEstimate() )
      {
         rc = _fromBSONEstimate( object, expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for estimate "
                      "information, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE_FROMBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONEST, "_optPlanNode::_toBSONEstimate" )
   INT32 _optPlanNode::_toBSONEstimate ( BSONObjBuilder &builder,
                                         const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONEST ) ;

      UINT16 showMask = expOptions.getShowMask() ;

      BSONObjBuilder estimateBuilder( builder.subobjStart( OPT_FIELD_ESTIMATE ) ) ;

      rc = _toBSONEstimateImpl( estimateBuilder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build estimate BSON, rc: %d", rc ) ;

      showMask &= _getExplainEstimateMask() ;

      if ( OSS_BIT_TEST( showMask, OPT_NODE_EXPLAIN_MASK_INPUT ) )
      {
         BSONObjBuilder subBuilder( estimateBuilder.subobjStart( OPT_FIELD_INPUT ) ) ;
         rc = _toBSONEstimateInput( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build estimate input BSON, "
                      "rc: %d", rc ) ;
         subBuilder.done() ;
      }
      if ( OSS_BIT_TEST( showMask, OPT_NODE_EXPLAIN_MASK_FILTER ) )
      {
         BSONObjBuilder subBuilder( estimateBuilder.subobjStart( OPT_FIELD_FILTER ) ) ;
         rc = _toBSONEstimateFilter( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build estimate filter BSON, "
                      "rc: %d", rc ) ;
         subBuilder.done() ;
      }
      if ( OSS_BIT_TEST( showMask, OPT_NODE_EXPLAIN_MASK_OUTPUT ) )
      {
         BSONObjBuilder subBuilder( estimateBuilder.subobjStart( OPT_FIELD_OUTPUT ) ) ;
         rc = _toBSONEstimateOutput( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build estimate output BSON, "
                      "rc: %d", rc ) ;
         subBuilder.done() ;
      }

      estimateBuilder.doneFast() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONEST, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__FROMBSONEST, "_optPlanNode::_fromBSONEstimate" )
   INT32 _optPlanNode::_fromBSONEstimate ( const BSONObj & object,
                                           const rtnExplainOptions &expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__FROMBSONEST ) ;

      try
      {
         BSONObj estimateObj ;
         UINT16 showMask = expOptions.getShowMask() ;

         rc = rtnGetObjElement( object, OPT_FIELD_ESTIMATE, estimateObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_ESTIMATE, rc ) ;

         rc = _fromBSONEstimateImpl( estimateObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rebuild from estimate BSON, "
                      "rc: %d", rc ) ;

         showMask &= _getExplainEstimateMask() ;

         if ( OSS_BIT_TEST( showMask, OPT_NODE_EXPLAIN_MASK_OUTPUT ) )
         {
            BSONObj estimateOutputObj ;

            rc = rtnGetObjElement( estimateObj, OPT_FIELD_OUTPUT,
                                   estimateOutputObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         OPT_FIELD_OUTPUT, rc ) ;

            rc = _fromBSONEstimateOutput( estimateOutputObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to rebuild from estimate "
                         "output BSON, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse plan node, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__FROMBSONEST, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONESTIMPL, "_optPlanNode::_toBSONEstimateImpl" )
   INT32 _optPlanNode::_toBSONEstimateImpl ( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONESTIMPL ) ;

      builder.append( OPT_FIELD_START_COST,
                      ( (double)_estStartCost ) * OPT_COST_TO_SEC ) ;
      builder.append( OPT_FIELD_RUN_COST,
                      ( (double)_estRunCost ) * OPT_COST_TO_SEC ) ;
      builder.append( OPT_FIELD_TOTAL_COST,
                      ( (double)_estTotalCost ) * OPT_COST_TO_SEC ) ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONESTIMPL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__FROMBSONESTIMPL, "_optPlanNode::_fromBSONEstimateImpl" )
   INT32 _optPlanNode::_fromBSONEstimateImpl ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__FROMBSONESTIMPL ) ;

      try
      {
         double result = 0.0 ;

         rc = rtnGetDoubleElement( object, OPT_FIELD_START_COST, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_START_COST, rc ) ;
         _estStartCost = (UINT64)DMS_STAT_ROUND_INT( result / OPT_COST_TO_SEC ) ;

         rc = rtnGetDoubleElement( object, OPT_FIELD_RUN_COST, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_RUN_COST, rc ) ;
         _estRunCost = (UINT64)DMS_STAT_ROUND_INT( result / OPT_COST_TO_SEC ) ;

         rc = rtnGetDoubleElement( object, OPT_FIELD_TOTAL_COST, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_TOTAL_COST, rc ) ;
         _estTotalCost = (UINT64)DMS_STAT_ROUND_INT( result / OPT_COST_TO_SEC ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse plan node, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__FROMBSONESTIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONESTOUTPUT, "_optPlanNode::_toBSONEstimateOutput" )
   INT32 _optPlanNode::_toBSONEstimateOutput ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONESTOUTPUT ) ;

      builder.append( OPT_FIELD_RECORDS, (INT64)_outputRecords ) ;
      builder.append( OPT_FIELD_RECORD_SIZE, (INT32)_outputRecordSize ) ;
      builder.appendBool( OPT_FIELD_SORTED, _sorted ) ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONESTOUTPUT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__FROMBSONESTOUTPUT, "_optPlanNode::_fromBSONEstimateOutput" )
   INT32 _optPlanNode::_fromBSONEstimateOutput ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__FROMBSONESTOUTPUT ) ;

      try
      {
         rc = rtnGetNumberLongElement( object, OPT_FIELD_RECORDS,
                                       (INT64 &)_outputRecords ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_RECORDS, rc ) ;

         rc = rtnGetIntElement( object, OPT_FIELD_RECORD_SIZE,
                                (INT32 &)_outputRecordSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_RECORD_SIZE, rc ) ;

         rc = rtnGetBooleanElement( object, OPT_FIELD_SORTED, _sorted ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_SORTED, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse plan node, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__FROMBSONESTOUTPUT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONRUN, "_optPlanNode::_toBSONRun" )
   INT32 _optPlanNode::_toBSONRun ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONRUN ) ;

      BSONObjBuilder runBuilder( builder.subobjStart( OPT_FIELD_RUN ) ) ;

      rc = _toBSONRunImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build run BSON, rc: %d", rc ) ;

      runBuilder.done() ;

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONRUN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONRUNIMPL, "_optPlanNode::_toBSONRunImpl" )
   INT32 _optPlanNode::_toBSONRunImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONRUNIMPL ) ;

      ossTickConversionFactor factor ;
      UINT32 seconds = 0, microseconds = 0 ;
      double queryTime = 0.0 ;
      CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestamp startTime( _runtimeMonitor.getStartTimestamp() ) ;

      _runtimeMonitor.getQueryTime().convertToTime( factor, seconds, microseconds ) ;
      queryTime = (double)( seconds ) +
                  (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;

      ossTimestampToString( startTime, timestampStr ) ;

      builder.append( OPT_FIELD_CONTEXT_ID, _runtimeMonitor.getContextID() ) ;
      builder.append( OPT_FIELD_QUERY_START_TIME, timestampStr ) ;
      builder.append( OPT_FIELD_QUERY_TIME_SPENT, queryTime ) ;
      // for COORD and MERGE node, one more batch for explain results
      builder.append( OPT_FIELD_GETMORES,
                      ( OPT_PLAN_MERGE == getType() ||
                        OPT_PLAN_COORD == getType() ) ?
                      (INT32)_runtimeMonitor.getReturnBatches() - 1 :
                      (INT32)_runtimeMonitor.getReturnBatches() ) ;
      builder.append( OPT_FIELD_RETURN_NUM,
                      (INT64)_runtimeMonitor.getReturnRecords() ) ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONRUNIMPL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONCHILDNODES, "_optPlanNode::_toBSONChildNodes" )
   INT32 _optPlanNode::_toBSONChildNodes ( BSONObjBuilder &builder,
                                           const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONCHILDNODES ) ;

      if ( hasChildNodes() )
      {
         BSONArrayBuilder subBuilder(
                     builder.subarrayStart( _getBSONChildArrayName() ) ) ;

         rc = _toBSONChildNodesImpl( subBuilder, expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for child nodes, "
                      "rc: %d", rc ) ;

         subBuilder.done() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONCHILDNODES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONRETOPTS, "_optPlanNode::_toBSONReturnOptions" )
   INT32 _optPlanNode::_toBSONReturnOptions ( BSONObjBuilder & builder,
                                              const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONRETOPTS ) ;

      builder.appendEx( OPT_FIELD_SELECTOR,
                        _returnOptions.getSelector(),
                        expOptions.getBuilderOption() ) ;

      builder.append( OPT_FIELD_SKIP, _returnOptions.getSkip() ) ;
      builder.append( OPT_FIELD_RETURN, _returnOptions.getLimit() ) ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONRETOPTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__FROMBSONRETOPTS, "_optPlanNode::_fromBSONReturnOptions" )
   INT32 _optPlanNode::_fromBSONReturnOptions ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__FROMBSONRETOPTS ) ;

      try
      {
         BSONObj selector ;
         INT64 numToSkip = 0, numToReturn = -1 ;

         rc = rtnGetObjElement( object, OPT_FIELD_SELECTOR,
                                selector ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_SELECTOR, rc ) ;
         _returnOptions.setSelector( selector ) ;

         rc = rtnGetNumberLongElement( object, OPT_FIELD_SKIP,
                                       numToSkip ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_SKIP, rc ) ;
         _returnOptions.setSkip( numToSkip ) ;

         rc = rtnGetNumberLongElement( object, OPT_FIELD_RETURN,
                                       numToReturn ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_RETURN, rc ) ;
         _returnOptions.setLimit( numToReturn ) ;

         rc = _returnOptions.getOwned() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return options owned, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to extract return options, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTPLANNODE__FROMBSONRETOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT32, "_optPlanNode::_toBSONFieldEval" )
   INT32 _optPlanNode::_toBSONFieldEval ( BSONObjBuilder & builder,
                                          const CHAR * outputName,
                                          const CHAR * outputFormula,
                                          const CHAR * outputEvaluate,
                                          INT32 outputResult ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT32 ) ;

      BSONArrayBuilder subBuilder( builder.subarrayStart( outputName ) ) ;

      subBuilder.append( outputFormula ) ;
      subBuilder.append( outputEvaluate ) ;
      subBuilder.append( outputResult ) ;

      subBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT32, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT64, "_optPlanNode::_toBSONFieldEval" )
   INT32 _optPlanNode::_toBSONFieldEval ( BSONObjBuilder & builder,
                                          const CHAR * outputName,
                                          const CHAR * outputFormula,
                                          const CHAR * outputEvaluate,
                                          INT64 outputResult ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT64 ) ;

      BSONArrayBuilder subBuilder( builder.subarrayStart( outputName ) ) ;

      subBuilder.append( outputFormula ) ;
      subBuilder.append( outputEvaluate ) ;
      subBuilder.append( outputResult ) ;

      subBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTPLANNODE__TOBSONFLDEVAL_INT64, rc ) ;

      return rc ;
   }

   INT32 _optPlanNode::_toBSONTotalCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      formBuilder << OPT_FIELD_START_COST << " + "
                  << OPT_FIELD_RUN_COST ;

      evalBuilder << _estStartCost << " + " << _estRunCost ;

      return _toBSONFieldEval( builder, OPT_FIELD_TOTAL_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estTotalCost ) ;
   }

   /*
      _optScanNode implement
    */
   _optScanNode::_optScanNode ()
   : _optPlanNode(),
     _pCollection( NULL ),
     _pageSizeLog2( DMS_PAGE_SIZE_LOG2_DFT ),
     _estCacheSize( OPT_RANDOM_SCAN_IO_COST * 2 ),
     _mthSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _mthCPUCost( OPT_MTH_DEFAULT_CPU_COST ),
     _needMatch( TRUE ),
     _inputRecords( 0 ),
     _inputPages( 0 ),
     _inputNumFields( 0 ),
     _inputRecordSize( 0 ),
     _readRecords( 0 ),
     _readPages( 0 ),
     _clFromStat( FALSE ),
     _clStatTime( 0 ),
     _isCandidate( FALSE ),
     _matchedOrders( 0 )
   {
   }

   _optScanNode::_optScanNode ( const CHAR * pCollection,
                                INT32 estCacheSize )
   : _optPlanNode(),
     _pCollection( pCollection ),
     _pageSizeLog2( DMS_PAGE_SIZE_LOG2_DFT ),
     _estCacheSize( estCacheSize ),
     _mthSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _mthCPUCost( OPT_MTH_DEFAULT_CPU_COST ),
     _needMatch( TRUE ),
     _inputRecords( 0 ),
     _inputPages( 0 ),
     _inputNumFields( 0 ),
     _inputRecordSize( 0 ),
     _readRecords( 0 ),
     _readPages( 0 ),
     _clFromStat( FALSE ),
     _clStatTime( 0 ),
     _isCandidate( FALSE ),
     _matchedOrders( 0 )
   {
   }

   _optScanNode::_optScanNode ( const optScanNode & node,
                                const rtnContext * context )
   : _optPlanNode( node, context ),
     _pCollection( node._pCollection ),
     _pageSizeLog2( node._pageSizeLog2 ),
     _estCacheSize( node._estCacheSize ),
     _mthSelectivity( node._mthSelectivity ),
     _mthCPUCost( node._mthCPUCost ),
     _needMatch( node._needMatch ),
     _inputRecords( node._inputRecords ),
     _inputPages( node._inputPages ),
     _inputNumFields( node._inputNumFields ),
     _inputRecordSize( node._inputRecordSize ),
     _readRecords( node._readRecords ),
     _readPages( node._readPages ),
     _clFromStat( node._clFromStat ),
     _clStatTime( node._clStatTime ),
     _isCandidate( node._isCandidate ),
     _runtimeMatcher( node._runtimeMatcher ),
     _matchedOrders( node._matchedOrders )
   {
      if ( NULL != context )
      {
         const rtnContextData * dataContext =
                           dynamic_cast<const rtnContextData *>( context ) ;
         SDB_ASSERT( dataContext, "data context is invalid" ) ;

         const optAccessPlanRuntime *planRuntime =
                                             dataContext->getPlanRuntime() ;
         SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;

         _returnOptions = dataContext->getReturnOptions() ;
         _setReturnSelector( dataContext ) ;

         // Reset matcher by runtime
         _runtimeMatcher = planRuntime->getParsedMatcher() ;
      }
   }

   _optScanNode::~_optScanNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE__PREEVAL, "_optScanNode::_preEvaluate" )
   void _optScanNode::_preEvaluate ( const rtnQueryOptions & queryOptions,
                                     optAccessPlanHelper & planHelper,
                                     optCollectionStat * collectionStat )
   {
      PD_TRACE_ENTRY( SDB_OPTSCANNODE__PREEVAL ) ;

      SDB_ASSERT( collectionStat, "collectionStat is invalid" ) ;

      _inputRecords = OPT_ROUND_NUM_DEF( collectionStat->getTotalRecords(),
                                         DMS_STAT_DEF_TOTAL_RECORDS ) ;
      _inputPages = OPT_ROUND_NUM( collectionStat->getTotalDataPages() ) ;
      _inputRecordSize = OPT_ROUND_NUM(
                  (UINT32)ceil( (double)collectionStat->getTotalDataSize() /
                                (double)_inputRecords ) ) ;

      _pageSizeLog2 = collectionStat->getPageSizeLog2() ;
      _inputNumFields = collectionStat->getAvgNumFields() ;

      if ( _needMatch ) {
         planHelper.getEstimation( collectionStat, _mthSelectivity, _mthCPUCost ) ;
      }

      if ( collectionStat->isValid() )
      {
         _clFromStat = TRUE ;
         _clStatTime = collectionStat->getCreateTime() ;
      }

      _returnOptions = queryOptions ;
      _returnOptions.getOwned() ;

      PD_TRACE_EXIT( SDB_OPTSCANNODE__PREEVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE_EVALOUTRECSIZE, "_optScanNode::_evalOutRecordSize" )
   void _optScanNode::_evalOutRecordSize ()
   {
      PD_TRACE_ENTRY( SDB_OPTSCANNODE_EVALOUTRECSIZE ) ;

      _outputNumFields = (UINT32)( _returnOptions.getSelectorFieldNum() ) ;

      if ( _outputNumFields > 0 && _inputNumFields > 0 )
      {
         _outputRecordSize = _outputNumFields * _inputRecordSize / _inputNumFields ;
      }
      else
      {
         _outputNumFields = _inputNumFields ;
         _outputRecordSize = _inputRecordSize ;
      }

      PD_TRACE_EXIT( SDB_OPTSCANNODE_EVALOUTRECSIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE_TOBSONCLSTAT, "_optScanNode::toBSONCLStatInfo" )
   INT32 _optScanNode::toBSONCLStatInfo ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANNODE_TOBSONCLSTAT ) ;

      builder.appendBool( OPT_FIELD_CL_STAT_EST, _clFromStat ) ;
      if ( _clFromStat )
      {
         ossTimestamp statTimestamp( _clStatTime ) ;
         CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
         ossTimestampToString( statTimestamp, timestampStr ) ;
         builder.append( OPT_FIELD_CL_STAT_TIME, timestampStr ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTSCANNODE_TOBSONCLSTAT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE__TOBSONESTIMPL, "_optScanNode::_toBSONEstimateImpl" )
   INT32 _optScanNode::_toBSONEstimateImpl ( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANNODE__TOBSONESTIMPL ) ;

      rc = _optPlanNode::_toBSONEstimateImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for estimate "
                   "information, rc: %d", rc ) ;

      rc = toBSONCLStatInfo( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for collection "
                   "statistics information, rc: %d", rc ) ;

      rc = toBSONIXStatInfo( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for index statistics "
                   "information, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANNODE__TOBSONESTIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE__TOBSONESTINPUT, "_optScanNode::_toBSONEstimateInput" )
   INT32 _optScanNode::_toBSONEstimateInput ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANNODE__TOBSONESTINPUT ) ;

      builder.append( OPT_FIELD_PAGES, (INT32)_inputPages ) ;
      builder.append( OPT_FIELD_RECORDS, (INT64)_inputRecords ) ;
      builder.append( OPT_FIELD_RECORD_SIZE, (INT32)_inputRecordSize ) ;

      PD_TRACE_EXITRC( SDB_OPTSCANNODE__TOBSONESTINPUT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE__TOBSONESTFILTER, "_optScanNode::_toBSONEstimateFilter" )
   INT32 _optScanNode::_toBSONEstimateFilter ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANNODE__TOBSONESTFILTER ) ;

      builder.append( OPT_FIELD_MTH_SEL, _mthSelectivity ) ;

      PD_TRACE_EXITRC( SDB_OPTSCANNODE__TOBSONESTFILTER, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSCANNODE__TOBSONRUNIMPL, "_optScanNode::_toBSONRunImpl" )
   INT32 _optScanNode::_toBSONRunImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSCANNODE__TOBSONRUNIMPL ) ;

      rc = _optPlanNode::_toBSONRunImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                   "rc: %d", rc ) ;

      builder.append( OPT_FIELD_READ_RECORDS,
                      (INT64)_runtimeMonitor.getDataRead() ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSCANNODE__TOBSONRUNIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _optScanNode::_toBSONRunCostEval ( BSONObjBuilder & builder,
                                            BOOLEAN needIOCost ) const
   {
      StringBuilder formBuilder, evalBuilder ;
      if ( needIOCost )
      {
         formBuilder << OPT_FIELD_IO_CPU_RATE << " * "
                     << OPT_FIELD_IO_COST << " + "
                     << OPT_FIELD_CPU_COST ;
         evalBuilder << OPT_IO_CPU_RATE << " * "
                     << _estIOCost << " + "
                     << _estCPUCost ;
      }
      else
      {
         formBuilder << OPT_FIELD_CPU_COST ;
         evalBuilder << _estCPUCost ;
      }

      return _toBSONFieldEval( builder, OPT_FIELD_RUN_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estRunCost ) ;
   }

   INT32 _optScanNode::_toBSONPagesEval ( BSONObjBuilder & builder,
                                          const CHAR * outputName,
                                          const CHAR * inputName,
                                          const CHAR * selName,
                                          UINT32 inputValue,
                                          double selectivity,
                                          UINT32 outputValue ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // max( 1, ceil( input * selectivity ) )
      formBuilder << "max( 1, ceil( " << inputName << " * "
                  << selName << " ) )" ;
      evalBuilder << "max( 1, ceil( " << inputValue << " * "
                  << selectivity << " ) )";

      return _toBSONFieldEval( builder, outputName, formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT32)outputValue ) ;
   }

   INT32 _optScanNode::_toBSONRecordsEval ( BSONObjBuilder & builder,
                                            const CHAR * outputName,
                                            const CHAR * inputName,
                                            const CHAR * selName,
                                            UINT64 inputValue,
                                            double selectivity,
                                            UINT64 outputValue ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // max( 1, ceil( input * selectivity ) )
      formBuilder << "max( 1, ceil( " << inputName << " * "
                  << selName << " ) )" ;
      evalBuilder << "max( 1, ceil( " << inputValue << " * "
                  << selectivity << " ) )" ;

      return _toBSONFieldEval( builder, outputName,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)outputValue ) ;
   }

   BOOLEAN _optScanNode::_checkEqualOrder( BSONElement &orderElement,
                                           RTN_PREDICATE_MAP &predicates )
   {
      BOOLEAN isEqual = FALSE ;

      const CHAR *orderField = orderElement.fieldName() ;
      RTN_PREDICATE_MAP::iterator iterPred = predicates.find( orderField ) ;
      if ( predicates.end() != iterPred &&
           iterPred->second.isEquality() )
      {
         isEqual = TRUE ;
      }

      return isEqual ;
   }

   /*
      _optTbScanNode implement
    */
   _optTbScanNode::_optTbScanNode ()
   : _optScanNode()
   {
   }

   _optTbScanNode::_optTbScanNode ( const CHAR * pCollection,
                                    INT32 estCacheSize )
   : _optScanNode( pCollection, estCacheSize )
   {
   }

   _optTbScanNode::_optTbScanNode ( const optTbScanNode & node,
                                    const rtnContext * context )
   : _optScanNode( node, context )
   {
      if ( NULL != context )
      {
         const rtnContextData * dataContext =
                           dynamic_cast<const rtnContextData *>( context ) ;
         SDB_ASSERT( dataContext, "data context is invalid" ) ;

         const optAccessPlanRuntime *planRuntime =
                                             dataContext->getPlanRuntime() ;
         SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;
         SDB_ASSERT( TBSCAN == planRuntime->getScanType(),
                     "scan type is invalid" ) ;
      }
   }

   _optTbScanNode::~_optTbScanNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN_PREEVAL, "_optTbScanNode::preEvaluate" )
   void _optTbScanNode::preEvaluate ( const rtnQueryOptions & queryOptions,
                                      optAccessPlanHelper & planHelper,
                                      optCollectionStat * collectionStat )
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN_PREEVAL ) ;

      SDB_ASSERT( collectionStat, "collectionStat is invalid" ) ;

      mthMatchTree *matcher = planHelper.getMatchTree() ;

      // prepare evaluate
      _preEvaluate( queryOptions, planHelper, collectionStat ) ;

      // evaluate order
      _evalOrder( queryOptions, planHelper ) ;

      if ( matcher->isInitialized() && matcher->isMatchesAll() &&
           !matcher->hasExpand() && !matcher->hasReturnMatch() )
      {
         _needMatch = FALSE ;
      }

      // TBSCAN is always candidate plan
      _isCandidate = TRUE ;

      PD_TRACE_EXIT( SDB_OPTTBSCAN_PREEVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN_EVAL, "_optTbScanNode::evaluate" )
   void _optTbScanNode::evaluate ()
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN_EVAL ) ;

      UINT64 noLimitRecords = OPT_ROUND_NUM(
                  (UINT64)ceil( (double)_inputRecords * _mthSelectivity ) ) ;
      UINT64 returnSkipRecords = 0 ;

      _outputRecords = _evaluateOutputRecords( noLimitRecords,
                                               returnSkipRecords ) ;

      if ( 0 == _outputRecords || 0 == noLimitRecords )
      {
         _evalNoReturnRecords() ;
      }
      else if ( _outputRecords != noLimitRecords )
      {
         _evalWithReturnOptions( noLimitRecords, returnSkipRecords ) ;
      }
      else
      {
         _evalNoReturnOptions() ;
      }

      _evalOutRecordSize() ;

      PD_TRACE_EXIT( SDB_OPTTBSCAN_EVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__EVALNORET, "_optTbScanNode::_evalNoReturnRecords" )
   void _optTbScanNode::_evalNoReturnRecords ()
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN__EVALNORET ) ;

      _readPages = 0 ;
      _readRecords = 0 ;

      _estStartCost = OPT_TBSCAN_DEFAULT_START_COST ;
      _estRunCost = 0 ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTTBSCAN__EVALNORET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__EVALNORETOPTS, "_optTbScanNode::_evalNoReturnOptions" )
   void _optTbScanNode::_evalNoReturnOptions ()
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN__EVALNORETOPTS ) ;

      _readPages = _inputPages ;
      _readRecords = _inputRecords ;

      _estIOCost = _evalScanIOCost( OPT_SEQ_SCAN_IO_COST, _readPages ) ;

      // Need to extract every records and evaluate matchers
      _estCPUCost = ( OPT_RECORD_CPU_COST + _mthCPUCost ) * _readRecords ;

      _estStartCost = OPT_TBSCAN_DEFAULT_START_COST ;
      _estRunCost = OPT_IO_CPU_RATE * _estIOCost + _estCPUCost ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTTBSCAN__EVALNORETOPTS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__EVALRETOPTS, "_optTbScanNode::_evalWithReturnOptions" )
   void _optTbScanNode::_evalWithReturnOptions ( UINT64 noLimitRecords,
                                                 UINT64 returnSkipRecords )
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN__EVALRETOPTS ) ;

      UINT64 scanSkipIOCost = 0, scanReturnIOCost = 0 ;
      UINT64 scanSkipCPUCost = 0, scanReturnCPUCost = 0 ;
      UINT32 readSkipPages = 0, readReturnPages = 0 ;
      UINT64 readSkipRecords = 0, readReturnRecords = 0 ;
      double skipRate = 0.0, returnRate = 1.0, scanRate = 1.0 ;

      skipRate = (double)returnSkipRecords / (double)noLimitRecords ;
      returnRate = (double)_outputRecords / (double)noLimitRecords ;
      scanRate = skipRate + returnRate ;

      readSkipPages = (UINT32)ceil( (double)_inputPages * skipRate ) ;
      readSkipRecords = (UINT64)ceil( (double)_inputRecords * skipRate ) ;

      _readPages = (UINT32)ceil( (double)_inputPages * scanRate ) ;
      _readRecords = (UINT64)ceil( (double)_inputRecords * scanRate ) ;

      readReturnPages = _readPages - readSkipPages ;
      readReturnRecords = _readRecords - readSkipRecords ;

      scanSkipIOCost = _evalScanIOCost( OPT_SEQ_SCAN_IO_COST, readSkipPages ) ;
      scanReturnIOCost = _evalScanIOCost( OPT_SEQ_SCAN_IO_COST, readReturnPages ) ;

      // Need to extract every records and evaluate matchers
      scanSkipCPUCost = ( OPT_RECORD_CPU_COST + _mthCPUCost ) *
                        readSkipRecords ;
      scanReturnCPUCost = ( OPT_RECORD_CPU_COST + _mthCPUCost ) *
                          readReturnRecords ;

      _estStartCost = OPT_TBSCAN_DEFAULT_START_COST +
                      OPT_IO_CPU_RATE * scanSkipIOCost +
                      scanSkipCPUCost ;
      _estRunCost = OPT_IO_CPU_RATE * scanReturnIOCost + scanReturnCPUCost ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTTBSCAN__EVALRETOPTS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__EVALORDER, "_optTbScanNode::_evalOrder" )
   void _optTbScanNode::_evalOrder( const rtnQueryOptions &queryOptions,
                                    optAccessPlanHelper &planHelper )
   {
      PD_TRACE_ENTRY( SDB_OPTTBSCAN__EVALORDER ) ;

      const BSONObj &boOrder = queryOptions.getOrderBy() ;
      RTN_PREDICATE_MAP &predicates = planHelper.getPredicates() ;

      // we could check order against predicates to find out whether
      // we could skip sorting
      // if all order fields are all equal predicates, the sorting could be
      // skipped, since in this case, only one distinct value of order fields
      // will be returned
      // e.g. predicates are { a: 10, b: 10, c: 1 }, order is { a: 1, b: 1 },
      //      obviously, only one distinct value { a: 10, b: 10 } is returned
      //      so no need to have extra sorting for order fields
      if ( !boOrder.isEmpty() && !predicates.empty() )
      {
         UINT32 matchedOrders = 0 ;
         BSONObjIterator iterOrder( boOrder ) ;

         while ( iterOrder.more() )
         {
            BSONElement beOrder = iterOrder.next() ; ;
            if ( _checkEqualOrder( beOrder, predicates ) )
            {
                // predicate is equal, it can be considered as order matched
               ++ matchedOrders ;
            }
            else
            {
               // not equal, no need to check further
               break ;
            }
         }

         // if all order fields are equal predicates, the sorting could be
         // skipped
         if ( matchedOrders == (UINT32)( boOrder.nFields() ) )
         {
            _sorted = TRUE ;
         }
         _matchedOrders = matchedOrders ;
      }

      PD_TRACE_EXIT( SDB_OPTTBSCAN__EVALORDER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__TOBSONBASIC, "_optTbScanNode::_toBSONBasic" )
   INT32 _optTbScanNode::_toBSONBasic ( BSONObjBuilder & builder,
                                        const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTTBSCAN__TOBSONBASIC ) ;

      builder.append( OPT_FIELD_OPERATOR, getName() ) ;
      builder.append( OPT_FIELD_COLLECTION,
                      NULL == _pCollection ? "" : _pCollection ) ;
      builder.appendEx( OPT_FIELD_QUERY,
                        _runtimeMatcher,
                        expOptions.getBuilderOption() ) ;

      rc = _toBSONReturnOptions( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for return options, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTTBSCAN__TOBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN__FROMBSONBASIC, "_optTbScanNode::_fromBSONBasic" )
   INT32 _optTbScanNode::_fromBSONBasic ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTTBSCAN__FROMBSONBASIC ) ;

      try
      {
         const CHAR *nodeName = NULL ;

         rc = rtnGetStringElement( object, OPT_FIELD_OPERATOR,
                                   &nodeName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_OPERATOR, rc ) ;

         PD_CHECK( 0 == ossStrcmp( nodeName, getName() ),
                   SDB_SYS, error, PDERROR, "Failed to get field [%s], "
                   "operator names [%s] and [%s] are different",
                   OPT_FIELD_OPERATOR, nodeName, getName() ) ;

         rc = rtnGetStringElement( object, OPT_FIELD_COLLECTION,
                                   &_pCollection ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_COLLECTION, rc ) ;

         rc = rtnGetObjElement( object, OPT_FIELD_QUERY, _runtimeMatcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_QUERY, rc ) ;

         rc = _fromBSONReturnOptions( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for return options, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for node info, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTTBSCAN__FROMBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTTBSCAN_TOBSONEVAL, "_optTbScanNode::toBSONEvaluation" )
   INT32 _optTbScanNode::toBSONEvaluation ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTTBSCAN_TOBSONEVAL ) ;

      BOOLEAN needIOCost = needEvalIOCost() ;

      builder.append( OPT_FIELD_MTH_SEL, _mthSelectivity ) ;
      builder.append( OPT_FIELD_MTH_COST, (INT32)_mthCPUCost ) ;

      if ( needIOCost )
      {
         rc = _toBSONIOCostEval( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for IO cost "
                      "evaluation, rc: %d", rc ) ;
      }
      else
      {
         builder.append( OPT_FIELD_IO_COST, (INT64)_estIOCost ) ;
      }

      rc = _toBSONCPUCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for CPU cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONStartCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for start cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONRunCostEval( builder, needIOCost ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONTotalCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for total cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONOutputRecordsEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for output records "
                   "evaluation, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTTBSCAN_TOBSONEVAL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _optTbScanNode::_toBSONIOCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // SeqReadIOCostUnit * ( Pages * ( PageSize / PageUnit ) )
      formBuilder << OPT_FIELD_SEQ_IO_COST << " * "
                  << OPT_FIELD_PAGES << " * ( "
                  << OPT_FIELD_PAGE_SIZE << " / "
                  << OPT_FIELD_PAGE_UINT << " )" ;

      evalBuilder << OPT_SEQ_SCAN_IO_COST << " * "
                  << _inputPages << " * ( "
                  << getPageSize() << " / "
                  << DMS_PAGE_SIZE_BASE << " )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_IO_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estIOCost ) ;
   }

   INT32 _optTbScanNode::_toBSONCPUCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // TotalRecords * ( RecExtractCPUCost + MthCPUCost )
      formBuilder << OPT_FIELD_RECORDS << " * ( "
                  << OPT_FIELD_REC_CPU_COST << " + "
                  << OPT_FIELD_MTH_COST << " )" ;

      evalBuilder << _inputRecords << " * ( "
                  << OPT_RECORD_CPU_COST << " + "
                  << _mthCPUCost << " )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_CPU_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estCPUCost ) ;
   }

   INT32 _optTbScanNode::_toBSONStartCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;
      formBuilder << OPT_FIELD_TB_START_COST ;
      evalBuilder << OPT_IXSCAN_DEFAULT_START_COST ;

      return _toBSONFieldEval( builder, OPT_FIELD_START_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estStartCost ) ;
   }

   INT32 _optTbScanNode::_toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const
   {
      return _toBSONRecordsEval( builder,
                                 OPT_FIELD_OUTPUT_RECORDS,
                                 OPT_FIELD_RECORDS,
                                 OPT_FIELD_MTH_SEL,
                                 _inputRecords,
                                 _mthSelectivity,
                                 _outputRecords ) ;
   }

   /*
      _optIxScanNode implement
    */
   _optIxScanNode::_optIxScanNode ()
   : _optScanNode(),
     _direction( 1 ),
     _matchAll( FALSE ),
     _indexCover( FALSE ),
     _notArray( FALSE ),
     _readIndexOnly( FALSE ),
     _matchedFields( 0 ),
     _indexExtID( DMS_INVALID_EXTENT ),
     _indexLID( DMS_INVALID_EXTENT ),
     _scanSelectivity( OPT_PRED_DEFAULT_SELECTIVITY ),
     _predSelectivity( OPT_PRED_DEFAULT_SELECTIVITY ),
     _predCPUCost( OPT_PRED_DEFAULT_CPU_COST ),
     _indexPages( 0 ),
     _indexLevels( 0 ),
     _idxReadRecords( 0 ),
     _idxReadPages( 0 ),
     _ixFromStat( FALSE ),
     _ixStatTime( 0 )
   {
   }

   _optIxScanNode::_optIxScanNode ( const CHAR * pCollection,
                                    const ixmIndexCB & indexCB,
                                    INT32 estCacheSize )
   : _optScanNode ( pCollection, estCacheSize ),
     _direction( 1 ),
     _matchAll( FALSE ),
     _indexCover( FALSE ),
     _notArray( FALSE ),
     _readIndexOnly( FALSE ),
     _matchedFields( 0 ),
     _indexExtID( DMS_INVALID_EXTENT ),
     _indexLID( DMS_INVALID_EXTENT ),
     _scanSelectivity( OPT_PRED_DEFAULT_SELECTIVITY ),
     _predSelectivity( OPT_PRED_DEFAULT_SELECTIVITY ),
     _predCPUCost( OPT_PRED_DEFAULT_CPU_COST ),
     _indexPages( 0 ),
     _indexLevels( 0 ),
     _idxReadRecords( 0 ),
     _idxReadPages( 0 ),
     _ixFromStat( FALSE ),
     _ixStatTime( 0 )
   {
      if ( indexCB.isInitialized() )
      {
         _pIndexName.append( indexCB.getName() ) ;
         _indexExtID = indexCB.getExtentID() ;
         _indexLID = indexCB.getLogicalID() ;
         _keyPattern = indexCB.keyPattern().getOwned() ;
         _notArray = indexCB.notArray() ;
      }
   }

   _optIxScanNode::_optIxScanNode ( const optIxScanNode & node,
                                    const rtnContext * context )
   : _optScanNode( node, context ),
     _direction( node._direction ),
     _matchAll( node._matchAll ),
     _indexCover( node._indexCover ),
     _readIndexOnly( node._readIndexOnly ),
     _matchedFields( node._matchedFields ),
     _indexExtID( DMS_INVALID_EXTENT ),
     _indexLID( DMS_INVALID_EXTENT ),
     _scanSelectivity( node._scanSelectivity ),
     _predSelectivity( node._predSelectivity ),
     _predCPUCost( node._predCPUCost ),
     _indexPages( node._indexPages ),
     _indexLevels( node._indexLevels ),
     _idxReadRecords( node._idxReadRecords ),
     _idxReadPages( node._idxReadPages ),
     _ixFromStat( node._ixFromStat ),
     _ixStatTime( node._ixStatTime ),
     _runtimeIXBound( node._runtimeIXBound )
   {
      if ( node._pIndexName.len() > 0 )
      {
         _pIndexName.append( node._pIndexName.str() ) ;
         _indexExtID = node._indexExtID ;
         _indexLID = node._indexLID ;
         _keyPattern = node._keyPattern.getOwned() ;
         _notArray = node.notArray() ;
      }

      if ( NULL != context )
      {
         const rtnContextData * dataContext =
                           dynamic_cast<const rtnContextData *>( context ) ;
         SDB_ASSERT( dataContext, "data context is invalid" ) ;

         const optAccessPlanRuntime *planRuntime =
                                             dataContext->getPlanRuntime() ;
         SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;
         SDB_ASSERT( IXSCAN == planRuntime->getScanType(),
                     "scan type is invalid" ) ;

         // Reset need match by runtime
         if ( NULL != planRuntime->getMatchTree() )
         {
            setNeedMatch( !planRuntime->getMatchTree()->isMatchesAll() ) ;
         }
         else
         {
            setNeedMatch( FALSE ) ;
         }

         _indexCover = dataContext->isIndexCover() ;

         // Reset index bound by runtime
         setIXBound( planRuntime->getPredIXBound() ) ;
      }
   }

   _optIxScanNode::~_optIxScanNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_PREEVAL, "_optIxScanNode::preEvaluate" )
   void _optIxScanNode::preEvaluate ( const rtnQueryOptions & queryOptions,
                                      optAccessPlanHelper & planHelper,
                                      OPT_PLAN_PATH_PRIORITY priority,
                                      optCollectionStat * collectionStat,
                                      optIndexStat * indexStat )
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN_PREEVAL ) ;

      SDB_ASSERT( collectionStat, "collectionStat is invalid" ) ;
      SDB_ASSERT( indexStat, "indexStat is invalid" ) ;

      _preEvaluate( queryOptions, planHelper, collectionStat ) ;

      _indexPages = indexStat->getIndexPages() ;
      _indexLevels = indexStat->getIndexLevels() ;

      BOOLEAN isBestIndex = collectionStat->isBestIndex( indexStat ) ;
      BOOLEAN canReadIndexOnly = queryOptions.testInternalFlag( RTN_INTERNAL_QUERY_COUNT_FLAG ) ;

      _evalPredEstimation( planHelper, queryOptions.getOrderBy(),
                           canReadIndexOnly, isBestIndex, indexStat ) ;

      switch ( priority )
      {
         case OPT_PLAN_IDX_REQUIRED :
         {
            _isCandidate = TRUE ;
            break ;
         }
         case OPT_PLAN_SORTED_IDX_REQUIRED :
         {
            // Must be sorted index
            if ( _sorted )
            {
               _isCandidate = TRUE ;
            }
            break ;
         }
         case OPT_PLAN_IDX_PREFERRED :
         {
            // Either be sorted or matched predicates
            if ( _sorted || _matchedFields > 0 )
            {
               _isCandidate = TRUE ;
            }
            break ;
         }
         default :
         {
            // Either be sorted, or can read index only, or scan selectivity
            // smaller than threshold
            if ( ( _readIndexOnly && _matchedFields > 0 ) ||
                 ( _scanSelectivity <= OPT_PRED_THRESHOLD_SELECTIVITY ) ||
                 ( _sorted ) )
            {
               _isCandidate = TRUE ;
            }
            break;
         }
      }

      if ( indexStat->isValid() )
      {
         _ixFromStat = TRUE ;
         _ixStatTime = indexStat->getCreateTime() ;
      }

      PD_TRACE_EXIT( SDB_OPTIXSCAN_PREEVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_EVAL, "_optIxScanNode::evaluate" )
   void _optIxScanNode::evaluate ()
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN_EVAL ) ;

      UINT64 noLimitRecords = 0, returnSkipRecords ;

      if ( _predSelectivity < _mthSelectivity )
      {
         noLimitRecords = OPT_ROUND_NUM(
                          (UINT64)ceil( (double)_inputRecords * _predSelectivity ) ) ;
      }
      else
      {
         noLimitRecords = OPT_ROUND_NUM(
                          (UINT64)ceil( (double)_inputRecords * _mthSelectivity ) ) ;
      }

      _outputRecords = _evaluateOutputRecords( noLimitRecords,
                                               returnSkipRecords ) ;

      if ( 0 == _outputRecords || 0 == noLimitRecords )
      {
         _evalNoReturnRecords() ;
      }
      else if ( _outputRecords != noLimitRecords )
      {
         _evalWithReturnOptions( noLimitRecords, returnSkipRecords ) ;
      }
      else
      {
         _evalNoReturnOptions() ;
      }

      _evalOutRecordSize() ;

      PD_TRACE_EXIT( SDB_OPTIXSCAN_EVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__EVALNORET, "_optIxScanNode::_evalNoReturnRecords" )
   void _optIxScanNode::_evalNoReturnRecords ()
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN__EVALNORET ) ;

      _idxReadPages = 0 ;
      _idxReadRecords = 0 ;
      _readPages = 0 ;
      _readRecords = 0 ;

      _estStartCost = OPT_IXSCAN_DEFAULT_START_COST ;
      _estRunCost = 0 ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTIXSCAN__EVALNORET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__EVALNORETOPTS, "_optIxScanNode::_evalNoReturnOptions" )
   void _optIxScanNode::_evalNoReturnOptions ()
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN__EVALNORETOPTS ) ;

      // Number of index pages and records will be read ( based on _scanSelectivity )
      _idxReadPages = OPT_ROUND_NUM( (UINT32)ceil( (double)_indexPages *
                                                   _scanSelectivity ) ) ;
      _idxReadRecords = OPT_ROUND_NUM( (UINT64)ceil( (double)_inputRecords *
                                                     _scanSelectivity ) ) ;

      if ( _readIndexOnly )
      {
         _readPages = 0 ;
         _readRecords = 0 ;
      }
      else
      {
         // Number of data pages and records will be read ( based on _predSelectivity )
         // which is also the number of items output from index
         _readPages = OPT_ROUND_NUM( (UINT32)ceil( (double)_inputPages *
                                                   _predSelectivity ) ) ;
         _readRecords = OPT_ROUND_NUM( (UINT64)ceil( (double)_inputRecords *
                                                     _predSelectivity ) ) ;
      }

      _estIOCost = _evalScanIOCost( OPT_RANDOM_SCAN_IO_COST,
                                    _idxReadPages + _readPages ) ;

      // For each index read records, need to be extracted from index page and
      // evaluated against predicates
      // For each index output records, need to be extracted from data page and
      // evaluated against matchers
      _estCPUCost = ( _idxReadRecords *
                      ( OPT_IDX_CPU_COST + _predCPUCost ) ) +
                    ( _readRecords *
                      ( OPT_RECORD_CPU_COST +
                        ( _needMatch ? _mthCPUCost : 0 ) ) ) ;

      _estStartCost = OPT_IXSCAN_DEFAULT_START_COST + _predCPUCost * _indexLevels ;
      _estRunCost = OPT_IO_CPU_RATE * _estIOCost + _estCPUCost ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTIXSCAN__EVALNORETOPTS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__EVALRETOPTS, "_optIxScanNode::_evalWithReturnOptions" )
   void _optIxScanNode::_evalWithReturnOptions ( UINT64 noLimitRecords,
                                                 UINT64 returnSkipRecords )
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN__EVALRETOPTS ) ;

      UINT32 idxNoLimitReadPages, noLimitReadPages = 0 ;
      UINT32 idxReadSkipPages = 0, readSkipPages = 0 ;
      UINT32 idxReadReturnPages = 0, readReturnPages = 0 ;

      UINT64 idxNoLimitReadRecords = 0, noLimitReadRecords = 0 ;
      UINT64 idxReadSkipRecords = 0, readSkipRecords = 0 ;
      UINT64 idxReadReturnRecords, readReturnRecords = 0 ;
      double skipRate = 0.0, returnRate = 1.0, scanRate = 1.0 ;
      UINT64 scanSkipIOCost = 0, scanReturnIOCost = 0 ;
      UINT64 scanSkipCPUCost = 0, scanReturnCPUCost = 0 ;

      skipRate = (double)returnSkipRecords / (double)noLimitRecords ;
      returnRate = (double)_outputRecords / (double)noLimitRecords ;
      scanRate = skipRate + returnRate ;

      // Number of index pages and records will be read ( based on _scanSelectivity )
      idxNoLimitReadPages = OPT_ROUND_NUM( (UINT32)ceil( (double)_indexPages *
                                                         _scanSelectivity ) ) ;
      idxNoLimitReadRecords = OPT_ROUND_NUM( (UINT64)ceil( (double)_inputRecords *
                                                           _scanSelectivity ) ) ;

      idxReadSkipPages = (UINT32)ceil( (double)idxNoLimitReadPages * skipRate ) ;
      idxReadSkipRecords = (UINT64)ceil( (double)idxNoLimitReadRecords * skipRate ) ;

      idxReadReturnPages = _idxReadPages - idxReadSkipPages ;
      idxReadReturnRecords = _idxReadRecords - idxReadSkipRecords ;

      _idxReadPages = (UINT32)ceil( (double)idxNoLimitReadPages * scanRate ) ;
      _idxReadRecords = (UINT64)ceil( (double)idxNoLimitReadRecords * scanRate ) ;

      if ( _readIndexOnly )
      {
         _readPages = 0 ;
         _readRecords = 0 ;
      }
      else
      {
         // Number of data pages and records will be read ( based on _predSelectivity )
         // which is also the number of items output from index
         noLimitReadPages = OPT_ROUND_NUM( (UINT32)ceil( (double)_inputPages *
                                                         _predSelectivity ) ) ;
         noLimitReadRecords = OPT_ROUND_NUM( (UINT64)ceil( (double)_inputRecords *
                                                           _predSelectivity ) ) ;

         readSkipPages = (UINT32)ceil( (double)noLimitReadPages * skipRate ) ;
         readSkipRecords = (UINT64)ceil( (double)noLimitReadRecords * skipRate ) ;

         _readPages = (UINT32)ceil( (double)noLimitReadPages * scanRate ) ;
         _readRecords = (UINT64)ceil( (double)noLimitReadRecords * scanRate ) ;

         readReturnPages = _readPages - readSkipPages ;
         readReturnRecords = _readRecords - readSkipRecords ;
      }

      scanSkipIOCost = _evalScanIOCost( OPT_RANDOM_SCAN_IO_COST,
                                        idxReadSkipPages + readSkipPages ) ;
      scanReturnIOCost = _evalScanIOCost( OPT_RANDOM_SCAN_IO_COST,
                                          idxReadReturnPages + readReturnPages ) ;

      // For each index read records, need to be extracted from index page and
      // evaluated against predicates
      // For each index output records, need to be extracted from data page and
      // evaluated against matchers
      scanSkipCPUCost = ( idxReadSkipRecords ) *
                        ( OPT_IDX_CPU_COST + _predCPUCost ) +
                        ( readSkipRecords *
                        ( OPT_RECORD_CPU_COST + ( _needMatch ? _mthCPUCost : 0 ) ) ) ;
      scanReturnCPUCost = ( idxReadReturnRecords ) *
                          ( OPT_IDX_CPU_COST + _predCPUCost ) +
                          ( readReturnRecords *
                          ( OPT_RECORD_CPU_COST + ( _needMatch ? _mthCPUCost : 0 ) ) ) ;

      _estStartCost = OPT_IXSCAN_DEFAULT_START_COST +
                      _predCPUCost * _indexLevels +
                      OPT_IO_CPU_RATE * scanSkipIOCost + scanSkipCPUCost ;
      _estRunCost = OPT_IO_CPU_RATE * scanReturnIOCost + scanReturnCPUCost ;
      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTIXSCAN__EVALRETOPTS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_EVALPREDEST, "_optIxScanNode::_evalPredEstimation" )
   void _optIxScanNode::_evalPredEstimation ( optAccessPlanHelper & planHelper,
                                              const BSONObj & boOrder,
                                              BOOLEAN canReadIndexOnly,
                                              BOOLEAN isBestIndex,
                                              const optIndexStat * indexStat )
   {
      PD_TRACE_ENTRY( SDB_OPTIXSCAN_EVALPREDEST ) ;

      SDB_ASSERT( planHelper.getMatchTree(), "matchTree is invalid" ) ;
      SDB_ASSERT( indexStat, "indexStat is invalid" ) ;

      UINT32 iterIdx = 0,
             matchedFields = 0,
             matchedOrders = 0 ;
      const BSONObj &keyPattern = indexStat->getKeyPattern() ;
      UINT32 keyNum = (UINT32)keyPattern.nFields() ;

      optIndexPathEncoder encoder ;
      rtnStatPredList predicateList ;
      BOOLEAN needMatchOrder = TRUE ;
      INT32 direction = 1 ;

      double predSelectivity = 1.0 ;
      double scanSelectivity = 1.0 ;
      UINT32 savedCPUCost = 0 ;

      const CHAR *pFirstField = NULL ;

      // The statistics of index are invalid, we need to evaluate each predicate
      // in the predicate set
      BOOLEAN isStatValid = indexStat->isValid() ;
      BOOLEAN needCalcScanSel = TRUE ;
      BOOLEAN isAllEqual = TRUE ;
      UINT32 predPathNum = 1 ;

      mthMatchTree *matcher = planHelper.getMatchTree() ;
      RTN_PREDICATE_MAP &predicates = planHelper.getPredicates() ;

      if ( !planHelper.isPredEstimated() )
      {
         // The matcher has not been estimated, which could not be used
         // to evaluate predicates
         isBestIndex = FALSE ;
      }

      BSONObjIterator iterKey( keyPattern ) ;
      BSONObjIterator iterOrder( boOrder ) ;

      while ( iterKey.more() )
      {
         BSONElement beKey = iterKey.next() ;
         const CHAR *pFieldName = beKey.fieldName() ;
         BOOLEAN isOrderField = FALSE ;
         BOOLEAN needRecheckOrder = FALSE ;

         // Set the first name
         if ( !pFirstField )
         {
            pFirstField = pFieldName ;
         }

         // Try to match order first
         while ( iterOrder.more() && needMatchOrder )
         {
            BSONElement beOrder = *iterOrder ;
            if ( 0 == ossStrcmp( pFieldName, beOrder.fieldName() ) )
            {
               // found order field, check direction
               BOOLEAN orderMatched = ( ( ( beKey.number() * direction ) > 0 ) ==
                                          ( beOrder.number() > 0 ) ) ;
               isOrderField = TRUE ;
               if ( matchedOrders == 0 )
               {
                  // first matched order field, set initial direction
                  direction = orderMatched ? 1 : -1 ;
                  ++ matchedOrders ;
                  ++ iterOrder ;
               }
               else if ( orderMatched )
               {
                  ++ matchedOrders ;
                  ++ iterOrder ;
               }
               else
               {
                  // order is not matched, but we could recheck later
                  // if the index field is equal predicate, we could skip
                  // this one
                  needRecheckOrder = TRUE ;
               }
            }
            else if ( !predicates.empty() &&
                      _checkEqualOrder( beOrder, predicates ) )
            {
               // corresponding predicate is equal, it can be considered as
               // order matched
               ++ matchedOrders ;
               ++ iterOrder ;

               // move to next order field
               continue ;
            }
            else
            {
               // order is not matched, but we could recheck later
               // if the index field is equal predicate, we could skip
               // this one
               needRecheckOrder = TRUE ;
            }

            break ;
         }

         if ( predicates.empty() )
         {
            // no predicates, if the order requires recheck, the order will not
            // matched since we don't have predicates
            if ( needRecheckOrder )
            {
               needMatchOrder = FALSE ;
            }
            iterIdx ++ ;
            isAllEqual = FALSE ;
            continue ;
         }

         RTN_PREDICATE_MAP::iterator iterPred = predicates.find( pFieldName ) ;
         BOOLEAN isCurSingleEqual = TRUE, isCurAllEqual = TRUE ;

         if ( iterPred == predicates.end() )
         {
            // The key is not included in the predicates
            // Cover all values in this key
            if ( isStatValid && !isBestIndex )
            {
               predicateList.push_back( NULL ) ;
               encoder.append( pFieldName, TRUE ) ;
            }
            isCurSingleEqual = FALSE ;
            isCurAllEqual = FALSE ;
            needCalcScanSel = FALSE ;
         }
         else
         {
            rtnPredicate &curPredicate = iterPred->second ;

            predPathNum *= curPredicate._startStopKeys.size() ;

            if ( !isStatValid )
            {
               // Evaluate the predicate for this field only
               BOOLEAN curIsAllRange = FALSE ;
               double curSelectivity =  indexStat->evalPredicate(
                        pFieldName, curPredicate, planHelper.mthEnabledMixCmp(),
                        curIsAllRange ) ;

               predSelectivity *= curSelectivity ;
               if ( needCalcScanSel )
               {
                  if ( curPredicate.isAllRange() )
                  {
                     needCalcScanSel = FALSE ;
                  }
                  else if ( ( curPredicate.isAllEqual() ) ||
                            ( 0 == iterIdx ) )
                  {
                     scanSelectivity *= curSelectivity ;
                  }
                  else
                  {
                     scanSelectivity *= OPT_ROUND_SELECTIVITY( curSelectivity * OPT_IDX_SCAN_FAN_OUT ) ;
                  }
               }
            }
            else if ( !isBestIndex )
            {
               // Need to evaluate the whole predicate set together, add the
               // predicates into list, and evaluate them later
               predicateList.push_back( &curPredicate ) ;
               encoder.append( pFieldName, FALSE ) ;
            }

            isCurSingleEqual = curPredicate.isEquality() ;
            isCurAllEqual = curPredicate.isAllEqual() ;
            savedCPUCost += curPredicate.getSavedCPUCost() ;
            matchedFields ++ ;
         }

         if ( isAllEqual && !isCurAllEqual )
         {
            isAllEqual = FALSE ;
         }

         if ( needRecheckOrder )
         {
            // if the order requires recheck, and the predicate is equal,
            // we could skip this field for order matching, otherwise, we could
            // mark the order is not matched
            if ( isCurSingleEqual )
            {
               if ( isOrderField )
               {
                  ++ matchedOrders ;
                  ++ iterOrder ;
               }
            }
            else
            {
               // key is order field, be not equal, so we could not
               // consider as order matched
               needMatchOrder = FALSE ;
            }
         }

         iterIdx ++ ;
      }

      // Matched key in index
      if ( matchedFields > 0 )
      {
         if ( !isStatValid )
         {
            // scanSelectivity is estimated by the first field
            // round by unique index
            if ( isAllEqual && indexStat->isUniqux() )
            {
               FLOAT64 uniqSelectivity =
                     OPT_ROUND_SELECTIVITY(
                           OSS_MIN( OPT_PRED_EQ_UNIQ_DEF_SELECTIVITY,
                                    OPT_PRED_EQ_DEF_NUM_KEYS *
                                    OPT_PRED_EQ_UNIQ_DEF_SELECTIVITY /
                                    indexStat->getTotalRecords() ) *
                           FLOAT64( predPathNum ) ) ;
               predSelectivity = OSS_MIN( predSelectivity, uniqSelectivity ) ;
               scanSelectivity = OSS_MIN( scanSelectivity, uniqSelectivity ) ;
            }
         }
         else if ( isBestIndex )
         {
            // The best index has been evaluated and cached in matcher
            planHelper.getPredSelectivity( predSelectivity, scanSelectivity ) ;
         }
         else
         {
            ossPoolString indexPath = encoder.getPath() ;
            if ( !( planHelper.getSelectivityFromCache( indexPath,
                                                        predSelectivity,
                                                        scanSelectivity) ) )
            {
               // The predicates contain multiple start stop key-pairs, evaluate
               // each of them
               predSelectivity = indexStat->evalPredicateList(
                     pFirstField, predicateList, planHelper.mthEnabledMixCmp(),
                     scanSelectivity ) ;

               // fix by match selectivity
               if ( NULL != planHelper.getMatchTree() )
               {
                  if ( predSelectivity < getMthSelctivity() )
                  {
                     predSelectivity = getMthSelctivity() ;
                  }
               }

               planHelper.saveSelectivityToCache( indexPath,
                                                  predSelectivity,
                                                  scanSelectivity ) ;
            }
         }
      }

      if ( !boOrder.isEmpty() )
      {
         // Set the scan direction
         _direction = direction ;

         if ( needMatchOrder &&
              matchedOrders != (UINT32)boOrder.nFields() &&
              !predicates.empty() )
         {
            // let's check the remaining orders, if all remaining order fields
            // are equal predicates, the sorting could be skipped
            // e.g. index is { a: 1, b: 1 }, predicate is { a: 1, c: 1 },
            //      order is { a: 1, c: 1 }, since the "c" is not matched in
            //      index, so the output of index could not be considered to
            //      match the order fields, but the field "c" is equal
            //      predicate
            while ( iterOrder.more() )
            {
               BSONElement beOrder = *iterOrder ;
               if ( _checkEqualOrder( beOrder, predicates ) )
               {
                  // if the predicate is equal, it can be considered as order
                  // matched
                  ++ matchedOrders ;
                  ++ iterOrder ;
               }
               else
               {
                  // it is not equal, no need to check further
                  break ;
               }
            }
         }

         // if the order could be matched, the sorting could be skipped
         if ( matchedOrders == (UINT32)( boOrder.nFields() ) )
         {
            _sorted = TRUE ;
         }
      }

      // we try to set matchall only when all fields converted into predicates
      if ( matcher->totallyConverted() )
      {
         _matchAll = ( 0 == predicates.size() ) ||
                     ( ( 0 != matchedFields ) &&
                       ( matchedFields == predicates.size() ) &&
                         matchedFields <= keyNum ) ;
      }

      if ( matcher->isInitialized() && _matchAll &&
           !matcher->hasExpand() && !matcher->hasReturnMatch() )
      {
         _needMatch = FALSE ;
      }

      _evalIndexCover( _keyPattern, canReadIndexOnly, iterOrder, matcher ) ;

      _matchedFields = matchedFields ;
      _matchedOrders = matchedOrders ;

      _predSelectivity = OPT_ROUND_SELECTIVITY( predSelectivity ) ;
      _scanSelectivity = OPT_ROUND_SELECTIVITY( scanSelectivity ) ;
      _predCPUCost = OPT_OPTR_BASE_CPU_COST * keyNum ;

      // Using predicates will save CPU cost for matcher
      _mthCPUCost =
            _mthCPUCost > savedCPUCost ? _mthCPUCost - savedCPUCost : 0 ;

      PD_TRACE_EXIT( SDB_OPTIXSCAN_EVALPREDEST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_EVALINDEXCOVER, "_optIxScanNode::_evalIndexCover" )
   void _optIxScanNode::_evalIndexCover( const BSONObj &keyPattern,
                                         BOOLEAN canReadIndexOnly,
                                         BSONObjIterator &restOrder,
                                         mthMatchTree *matcher )
   {
      INT32 rc = SDB_OK ;
      ixmIndexCover index( keyPattern ) ;
      PD_TRACE_ENTRY( SDB_OPTIXSCAN_EVALINDEXCOVER ) ;

      BOOLEAN indexCoverOn = pmdGetOptionCB()->isIndexCoverOn() ;

      _indexCover = FALSE ;
      _readIndexOnly = FALSE ;

      if( ( !canReadIndexOnly ) &&
          ( ( !_notArray ) || ( !indexCoverOn ) ) )
      {
         // if index support array, then can't be indexCover
         goto done ;
      }
      // the reset of order
      while ( restOrder.more() )
      {
         BSONElement beOrder = restOrder.next() ;
         if( FALSE == index.cover( beOrder.fieldName() ) )
         {
            // not indexCover
            goto done ;
         }
      }

      if( _matchAll )
      {
         // we kown index covered matcher
      }
      else if( matcher->totallyConverted() )
      {
         // all matcher fields is converted to predicates, but not matchall
         goto done ;
      }
      else
      {
         IXM_FIELD_NAME_SET matchNameSet ;
         // some matcher fileds is not coverted to predicates as follows
         // matcher is { a:  { $gt: { "$field": "b" } } }
         // matcher is { $or:[ { a: { $gt:3 } }, { a: { $lt:10 } } ] }
         rc = matcher->getName( matchNameSet ) ;
         if( rc )
         {
             PD_LOG( PDWARNING, "Get matcher name failed rc: %d", rc ) ;
             rc = SDB_OK ;
             goto done ;
         }
         if( FALSE == index.cover( matchNameSet ) )
         {
            goto done ;
         }
      }

      if ( canReadIndexOnly )
      {
         _readIndexOnly = TRUE ;
      }
      if ( _notArray && indexCoverOn )
      {
         _indexCover = TRUE ;
      }

   done :
      PD_TRACE_EXIT( SDB_OPTIXSCAN_EVALINDEXCOVER ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_TOBSONEVAL, "_optIxScanNode::toBSONEvaluation" )
   INT32 _optIxScanNode::toBSONEvaluation ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN_TOBSONEVAL ) ;

      BOOLEAN needIOCost = needEvalIOCost() ;

      builder.append( OPT_FIELD_INDEX_PAGES, (INT32)_indexPages ) ;
      builder.append( OPT_FIELD_INDEX_LEVELS, (INT32)_indexLevels ) ;
      builder.append( OPT_FIELD_MTH_SEL, _mthSelectivity ) ;
      builder.append( OPT_FIELD_MTH_COST, (INT32)_mthCPUCost ) ;
      builder.append( OPT_FIELD_SCAN_SEL, _scanSelectivity ) ;
      builder.append( OPT_FIELD_PRED_SEL, _predSelectivity ) ;
      builder.append( OPT_FIELD_PRED_COST, (INT32)_predCPUCost ) ;

      if ( needIOCost )
      {
         rc = _toBSONPagesEval( builder,
                                OPT_FIELD_INDEX_READ_PAGES,
                                OPT_FIELD_INDEX_PAGES,
                                OPT_FIELD_SCAN_SEL,
                                _indexPages,
                                _scanSelectivity,
                                _idxReadPages ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for %s evaluation, "
                      "rc: %d", OPT_FIELD_INDEX_READ_PAGES, rc ) ;
      }

      rc = _toBSONRecordsEval( builder,
                               OPT_FIELD_INDEX_READ_RECORDS,
                               OPT_FIELD_RECORDS,
                               OPT_FIELD_SCAN_SEL,
                               _inputRecords,
                               _scanSelectivity,
                               _idxReadRecords ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for %s evaluation, "
                   "rc: %d", OPT_FIELD_INDEX_READ_RECORDS, rc ) ;

      if ( needIOCost )
      {
         rc = _toBSONPagesEval( builder,
                                OPT_FIELD_READ_PAGES,
                                OPT_FIELD_PAGES,
                                OPT_FIELD_PRED_SEL,
                                _indexPages,
                                _predSelectivity,
                                _readPages ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for %s evaluation, "
                      "rc: %d", OPT_FIELD_READ_PAGES, rc ) ;
      }

      rc = _toBSONRecordsEval( builder,
                               OPT_FIELD_READ_RECORDS,
                               OPT_FIELD_RECORDS,
                               OPT_FIELD_PRED_SEL,
                               _inputRecords,
                               _predSelectivity,
                               _readRecords ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for %s evaluation, "
                   "rc: %d", OPT_FIELD_READ_RECORDS, rc ) ;

      if ( needIOCost )
      {
         rc = _toBSONIOCostEval( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for IO cost "
                      "evaluation, rc: %d", rc ) ;
      }
      else
      {
         builder.append( OPT_FIELD_IO_COST, (INT64)_estIOCost ) ;
      }

      rc = _toBSONCPUCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for CPU cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONStartCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for start cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONRunCostEval( builder, needIOCost ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONTotalCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for total cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONOutputRecordsEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for output records "
                   "evaluation, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCAN_TOBSONEVAL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN_TOBSONIXSTAT, "_optIxScanNode::toBSONIXStatInfo" )
   INT32 _optIxScanNode::toBSONIXStatInfo ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN_TOBSONIXSTAT ) ;

      builder.appendBool( OPT_FIELD_IX_STAT_EST, _ixFromStat ) ;
      if ( _ixFromStat )
      {
         ossTimestamp statTimestamp( _ixStatTime ) ;
         CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
         ossTimestampToString( statTimestamp, timestampStr ) ;
         builder.append( OPT_FIELD_IX_STAT_TIME, timestampStr ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTIXSCAN_TOBSONIXSTAT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__TOBSONBASIC, "_optIxScanNode::_toBSONBasic" )
   INT32 _optIxScanNode::_toBSONBasic ( BSONObjBuilder & builder,
                                        const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN__TOBSONBASIC ) ;

      builder.append( OPT_FIELD_OPERATOR, getName() ) ;
      builder.append( OPT_FIELD_COLLECTION,
                      NULL == _pCollection ? "" : _pCollection ) ;
      builder.append( OPT_FIELD_INDEX, _pIndexName.str() ) ;
      if ( _runtimeIXBound.isEmpty() )
      {
         builder.appendNull( OPT_FIELD_IX_BOUND ) ;
      }
      else
      {
         // already readably
         builder.append( OPT_FIELD_IX_BOUND, _runtimeIXBound ) ;
      }
      builder.appendEx( OPT_FIELD_QUERY,
                        _runtimeMatcher,
                        expOptions.getBuilderOption() ) ;
      builder.appendBool( OPT_FIELD_NEED_MATCH, _needMatch ) ;
      builder.appendBool( OPT_FIELD_INDEX_COVER, _indexCover ) ;

      rc = _toBSONReturnOptions( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for return options, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCAN__TOBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__FROMBSONBASIC, "_optIxScanNode::_fromBSONBasic" )
   INT32 _optIxScanNode::_fromBSONBasic ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN__FROMBSONBASIC ) ;

      try
      {
         const CHAR *nodeName = NULL ;
         const CHAR *indexName = NULL ;

         rc = rtnGetStringElement( object, OPT_FIELD_OPERATOR,
                                   &nodeName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_OPERATOR, rc ) ;

         PD_CHECK( 0 == ossStrcmp( nodeName, getName() ),
                   SDB_SYS, error, PDERROR, "Failed to get field [%s], "
                   "operator names [%s] and [%s] are different",
                   OPT_FIELD_OPERATOR, nodeName, getName() ) ;

         rc = rtnGetStringElement( object, OPT_FIELD_COLLECTION,
                                   &_pCollection ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_COLLECTION, rc ) ;

         rc = rtnGetStringElement( object, OPT_FIELD_INDEX,
                                   &indexName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_INDEX, rc ) ;

         _pIndexName.clear() ;
         _pIndexName.append( indexName ) ;

         if ( object.getField( OPT_FIELD_IX_BOUND ).isNull() )
         {
            _runtimeIXBound = BSONObj() ;
         }
         else if ( object.getField( OPT_FIELD_IX_BOUND ).isABSONObj() )
         {
            rc = rtnGetObjElement( object, OPT_FIELD_IX_BOUND,
                                   _runtimeIXBound ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         OPT_FIELD_IX_BOUND, rc ) ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get field [%s], "
                    "BSONObj or NULL is expected", OPT_FIELD_IX_BOUND ) ;
            goto error ;
         }

         rc = rtnGetObjElement( object, OPT_FIELD_QUERY, _runtimeMatcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_QUERY, rc ) ;

         rc = rtnGetBooleanElement( object, OPT_FIELD_NEED_MATCH,
                                    _needMatch ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_NEED_MATCH, rc ) ;

         rc = rtnGetBooleanElement( object, OPT_FIELD_INDEX_COVER,
                                    _indexCover ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_INDEX_COVER, rc ) ;

         rc = _fromBSONReturnOptions( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for return options, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for node info, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCAN__FROMBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__TOBSONESTINPUT, "_optIxScanNode::_toBSONEstimateInput" )
   INT32 _optIxScanNode::_toBSONEstimateInput ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN__TOBSONESTINPUT ) ;

      rc = _optScanNode::_toBSONEstimateInput( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build estimate input BSON, "
                   "rc: %d", rc ) ;

      builder.append( OPT_FIELD_INDEX_PAGES, (INT32)_indexPages ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCAN__TOBSONESTINPUT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCAN__TOBSONESTFILTER, "_optIxScanNode::_toBSONEstimateFilter" )
   INT32 _optIxScanNode::_toBSONEstimateFilter ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCAN__TOBSONESTFILTER ) ;

      rc = _optScanNode::_toBSONEstimateFilter( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build estimate filter BSON, "
                   "rc: %d", rc ) ;

      builder.append( OPT_FIELD_SCAN_SEL, _scanSelectivity ) ;
      builder.append( OPT_FIELD_PRED_SEL, _predSelectivity ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCAN__TOBSONESTFILTER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTIXSCANNODE__TOBSONRUNIMPL, "_optIxScanNode::_toBSONRunImpl" )
   INT32 _optIxScanNode::_toBSONRunImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTIXSCANNODE__TOBSONRUNIMPL ) ;

      rc = _optScanNode::_toBSONRunImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                   "rc: %d", rc ) ;

      builder.append( OPT_FIELD_INDEX_READ_RECORDS,
                      (INT64)_runtimeMonitor.getIndexRead() ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTIXSCANNODE__TOBSONRUNIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _optIxScanNode::_toBSONIOCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // RandomReadIOCostUnit * ( IndexReadPages + ReadPages ) *
      // ( PageSize / PageUnit )
      formBuilder << OPT_FIELD_RAN_IO_COST << " * ( "
                  << OPT_FIELD_INDEX_READ_PAGES << " + "
                  << OPT_FIELD_READ_PAGES << " ) * ( "
                  << OPT_FIELD_PAGE_SIZE << " / "
                  << OPT_FIELD_PAGE_UINT << " )" ;

      evalBuilder << OPT_RANDOM_SCAN_IO_COST << " * ( "
                  << _idxReadPages << " + "
                  << _readPages << " ) * ( "
                  << getPageSize() << " / "
                  << DMS_PAGE_SIZE_BASE << " )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_IO_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estIOCost ) ;
   }

   INT32 _optIxScanNode::_toBSONCPUCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // 1. If match is needed
      //    IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) +
      //    ReadRecords * ( RecExtractCPUCost + MthCPUCost )
      // 2. If match is not needed
      //    IndexReadRecords * ( IXExtractCPUCost + PredCPUCost ) +
      //    ReadRecords * RecExtractCPUCost
      formBuilder << OPT_FIELD_INDEX_READ_RECORDS << " * ( "
                  << OPT_FIELD_IDX_CPU_COST << " + "
                  << OPT_FIELD_PRED_COST << " ) + " ;

      evalBuilder << _idxReadRecords << " * ( " <<
                  OPT_IDX_CPU_COST << " + " <<
                  _predCPUCost << " ) + " ;

      if ( _needMatch )
      {
         formBuilder << OPT_FIELD_READ_RECORDS << " * ( " <<
                     OPT_FIELD_REC_CPU_COST << " + " <<
                     OPT_FIELD_MTH_COST << " )" ;
         evalBuilder << _readRecords << " * ( " <<
                     OPT_RECORD_CPU_COST << " + " <<
                     _mthCPUCost << " )" ;
      }
      else
      {
         formBuilder << OPT_FIELD_READ_RECORDS << " * " <<
                     OPT_FIELD_REC_CPU_COST ;
         evalBuilder << _readRecords << " * " <<
                     OPT_RECORD_CPU_COST ;
      }

      return _toBSONFieldEval( builder, OPT_FIELD_CPU_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estCPUCost ) ;
   }

   INT32 _optIxScanNode::_toBSONStartCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // IXScanStartCost + PredCPUCost * IndexLevels
      formBuilder << OPT_FIELD_IX_START_COST << " + "
                  << OPT_FIELD_PRED_COST << " * "
                  << OPT_FIELD_INDEX_LEVELS ;

      evalBuilder << OPT_IXSCAN_DEFAULT_START_COST << " + "
                  << _predCPUCost << " * "
                  << _indexLevels ;

      return _toBSONFieldEval( builder, OPT_FIELD_START_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estStartCost ) ;
   }

   INT32 _optIxScanNode::_toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // max( 1, ceil( Records * min( IXPredSelectivity, MthSelectivity ) ) )
      formBuilder << "max( 1, ceil( " << OPT_FIELD_RECORDS << " * min( "
                  << OPT_FIELD_PRED_SEL << ", " << OPT_FIELD_MTH_SEL << " ) ) )" ;
      evalBuilder << "max( 1, ceil( " << _inputRecords << " * min( "
                  << _predSelectivity << ", " << _mthSelectivity << " ) ) )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_OUTPUT_RECORDS,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_outputRecords ) ;
   }

   /*
      _optSortNode implement
    */
   _optSortNode::_optSortNode ()
   : _optPlanNode(),
     _estSortType( OPT_PLAN_IN_MEM_SORT ),
     _runtimeSortType( OPT_PLAN_IN_MEM_SORT ),
     _sortBufferSize( 0 )
   {
   }

   _optSortNode::_optSortNode ( const optSortNode & node,
                                const rtnContext * context )
   : _optPlanNode( node, context ),
     _estSortType( node._estSortType ),
     _orderBy( node._orderBy ),
     _sortBufferSize( node._sortBufferSize )
   {
      if ( NULL == context )
      {
         _runtimeSortType = node._runtimeSortType ;
      }
      else
      {
         const rtnContextSort *sortContext =
                           dynamic_cast<const rtnContextSort *>( context ) ;
         SDB_ASSERT( sortContext, "sortContext is invalid" ) ;

         _returnOptions = sortContext->getReturnOptions() ;
         _setReturnSelector( sortContext ) ;

         _runtimeSortType = sortContext->isInMemorySort() ?
                            OPT_PLAN_IN_MEM_SORT : OPT_PLAN_EXT_SORT ;
      }
   }

   _optSortNode::~_optSortNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT_PREEVAL, "_optSortNode::preEvaluate" )
   void _optSortNode::preEvaluate ( const rtnQueryOptions & queryOptions,
                                    UINT64 sortBufferSize )
   {
      PD_TRACE_ENTRY( SDB_OPTSORT_PREEVAL ) ;

      _orderBy = queryOptions.getOrderBy() ;
      _sortBufferSize = sortBufferSize ;
      _returnOptions = queryOptions ;

      _orderBy = _orderBy.getOwned() ;
      _returnOptions.getOwned() ;

      PD_TRACE_EXIT( SDB_OPTSORT_PREEVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT_EVAL, "_optSortNode::evaluate" )
   void _optSortNode::evaluate ()
   {
      PD_TRACE_ENTRY( SDB_OPTSORT_EVAL ) ;

      UINT32 numOrderByFields = (UINT32)_orderBy.nFields() ;

      SDB_ASSERT( numOrderByFields > 0, "orderBy is invalid" ) ;
      SDB_ASSERT( _childNodes.size() == 1, "Should have only one child node" ) ;

      const _optPlanNode *childNode = _childNodes.front() ;

      UINT64 inputSize = ( childNode->getOutputRecords() *
                           childNode->getOutputRecordSize() ) ;
      UINT64 inputRecords = childNode->getOutputRecords() ;
      double comparisionCPUCost = 2.0 * (double)OPT_OPTR_BASE_CPU_COST *
                                  (double)numOrderByFields ;
      UINT64 outputSkipRecords = 0 ;

      // At least 2 records to be compared
      double roundInputRecords = (double)( OSS_MAX( 2, inputRecords ) ) ;
      _estCPUCost = (UINT64)ceil( comparisionCPUCost *
                                  roundInputRecords *
                                  OPT_LOG2( roundInputRecords ) ) ;

      // Check input size against sort buffer size
      if ( inputSize > _sortBufferSize )
      {
         // Use external merge-sort
         // 1. Write data to disk
         // 2. Read data from disk and merge
         //    75% is sequential read
         //    25% is random read
         _estIOCost = (UINT64)ceil( (double)_getInputPages( inputSize ) *
                                    ( (double)OPT_SEQ_WRITE_IO_COST +
                                      (double)OPT_SEQ_SCAN_IO_COST * 0.75 +
                                      (double)OPT_RANDOM_SCAN_IO_COST * 0.25 ) ) ;
         _estSortType = OPT_PLAN_EXT_SORT ;
      }
      else
      {
         // Use in-memory sort
         _estIOCost = 0 ;
         _estSortType = OPT_PLAN_IN_MEM_SORT ;
      }

      _outputRecords = _evaluateOutputRecords( inputRecords,
                                               outputSkipRecords ) ;
      _outputNumFields = childNode->getOutputNumFields() ;
      _outputRecordSize = childNode->getOutputRecordSize() ;

      _estStartCost = childNode->getEstTotalCost() +
                      OPT_IO_CPU_RATE * _estIOCost +
                      _estCPUCost +
                      OPT_OPTR_BASE_CPU_COST * outputSkipRecords ;

      _estRunCost = OPT_OPTR_BASE_CPU_COST * _outputRecords ;
      _estTotalCost = _estStartCost + _estRunCost ;

      _sorted = TRUE ;

      PD_TRACE_EXIT( SDB_OPTSORT_EVAL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT_TOBSONEVAL, "_optSortNode::toBSONEvaluation" )
   INT32 _optSortNode::toBSONEvaluation ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT_TOBSONEVAL ) ;

      const _optPlanNode *childNode = _childNodes.front() ;

      UINT32 numOrderByFields = (UINT32)_orderBy.nFields() ;
      UINT64 inputRecords = childNode->getOutputRecords() ;
      UINT32 inputRecordSize = childNode->getOutputRecordSize() ;
      UINT64 inputSize = inputRecords * inputRecordSize ;
      UINT32 inputPages = _getInputPages( inputSize ) ;

      builder.append( OPT_FIELD_RECORDS, (INT64)inputRecords ) ;
      builder.append( OPT_FIELD_SORT_FIELDS, (INT32)numOrderByFields ) ;

      rc = _toBSONInputSizeEval( builder, inputRecords, inputRecordSize,
                                 inputSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for input size "
                      "evaluation, rc: %d", rc ) ;

      rc = _toBSONInputPageEval( builder, inputSize, inputPages ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for input size "
                   "evaluation, rc: %d", rc ) ;

      builder.append( OPT_FIELD_SORT_TYPE,
                      ( _estSortType == OPT_PLAN_IN_MEM_SORT ?
                        OPT_VALUE_SORT_IN_MEM :
                        OPT_VALUE_SORT_EXTERNAL ) ) ;

      if ( OPT_PLAN_EXT_SORT == _estSortType )
      {
         rc = _toBSONIOCostEval( builder, inputPages ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for IO cost "
                      "evaluation, rc: %d", rc ) ;
      }
      else
      {
         builder.append( OPT_FIELD_IO_COST, (INT64)_estIOCost ) ;
      }

      rc = _toBSONCPUCostEval( builder, inputRecords ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for CPU cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONStartCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for start cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONRunCostEval( builder, inputRecords ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONTotalCostEval( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for total cost "
                   "evaluation, rc: %d", rc ) ;

      rc = _toBSONOutputRecordsEval( builder  ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for output records "
                   "evaluation, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSORT_TOBSONEVAL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT__TOBSONBASIC, "_optSortNode::_toBSONBasic" )
   INT32 _optSortNode::_toBSONBasic ( BSONObjBuilder & builder,
                                      const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT__TOBSONBASIC ) ;

      builder.append( OPT_FIELD_OPERATOR, getName() ) ;

      builder.appendEx( OPT_FIELD_SORT,
                        _orderBy,
                        expOptions.getBuilderOption() ) ;

      _toBSONReturnOptions( builder, expOptions ) ;

      PD_RC_CHECK( rc, PDERROR, "Failed to generate BSON for return options, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSORT__TOBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT__FROMBSONBASIC, "_optSortNode::_fromBSONBasic" )
   INT32 _optSortNode::_fromBSONBasic ( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT__FROMBSONBASIC ) ;

      try
      {
         const CHAR *nodeName = NULL ;

         rc = rtnGetStringElement( object, OPT_FIELD_OPERATOR,
                                   &nodeName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_OPERATOR, rc ) ;

         PD_CHECK( 0 == ossStrcmp( nodeName, getName() ),
                   SDB_SYS, error, PDERROR, "Failed to get field [%s], "
                   "operator names [%s] and [%s] are different",
                   OPT_FIELD_OPERATOR, nodeName, getName() ) ;

         rc = rtnGetObjElement( object, OPT_FIELD_SORT, _orderBy ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_SORT, rc ) ;

         rc = _fromBSONReturnOptions( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for return options, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for node info, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTSORT__FROMBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT__TOBSONESTIMPL, "_optSortNode::_toBSONEstimateImpl" )
   INT32 _optSortNode::_toBSONEstimateImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT__TOBSONESTIMPL ) ;

      rc = _optPlanNode::_toBSONEstimateImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for estimate "
                   "information, rc: %d", rc ) ;

      builder.append( OPT_FIELD_SORT_TYPE,
                      ( _estSortType == OPT_PLAN_IN_MEM_SORT ?
                        OPT_VALUE_SORT_IN_MEM :
                        OPT_VALUE_SORT_EXTERNAL ) ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSORT__TOBSONESTIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT__TOBSONRUNIMPL, "_optSortNode::_toBSONRunImpl" )
   INT32 _optSortNode::_toBSONRunImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT__TOBSONRUNIMPL ) ;

      rc = _optPlanNode::_toBSONRunImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                   "rc: %d", rc ) ;

      builder.append( OPT_FIELD_SORT_TYPE,
                      ( _runtimeSortType == OPT_PLAN_IN_MEM_SORT ?
                        OPT_VALUE_SORT_IN_MEM :
                        OPT_VALUE_SORT_EXTERNAL ) ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTSORT__TOBSONRUNIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTSORT__TOBSONCHILDNODESIMPL, "_optSortNode::_toBSONChildNodesImpl" )
   INT32 _optSortNode::_toBSONChildNodesImpl ( BSONArrayBuilder & builder,
                                               const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTSORT__TOBSONCHILDNODESIMPL ) ;

      SDB_ASSERT( _childNodes.size() == 1, "child node is invalid" ) ;

      const optPlanNode *pChildNode = _childNodes.front() ;
      const optScanNode *pScanNode =
                  dynamic_cast<const optScanNode *>( pChildNode ) ;

      // copy options
      rtnExplainOptions tempOptions( expOptions ) ;

      SDB_ASSERT( NULL != pScanNode, "child node is invalid" ) ;

      if ( !expOptions.isNeedExpand() )
      {
         // no need to build run and estimate to output BSON
         tempOptions.setShowMask( OPT_NODE_EXPLAIN_MASK_NONE ) ;
         tempOptions.setNeedRun( FALSE ) ;
         tempOptions.setNeedEstimate( FALSE ) ;
      }
      // no need to be flatten, scan node and sort node should be in
      // one record in flatten mode
      tempOptions.setNeedFlatten( FALSE ) ;

      BSONObjBuilder subBuilder( builder.subobjStart() ) ;
      pScanNode->toBSON( subBuilder, tempOptions ) ;
      subBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTSORT__TOBSONCHILDNODESIMPL, rc ) ;

      return rc ;
   }

   INT32 _optSortNode::_toBSONInputSizeEval ( BSONObjBuilder & builder,
                                              UINT64 records,
                                              UINT32 recordSize,
                                              UINT64 totalSize ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // Records * RecordSize
      formBuilder << OPT_FIELD_RECORDS << " * "
                  << OPT_FIELD_RECORD_SIZE ;

      evalBuilder << records << " * " << recordSize ;

      return _toBSONFieldEval( builder, OPT_FIELD_RECORD_TOTAL_SIZE,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)totalSize ) ;
   }

   INT32 _optSortNode::_toBSONInputPageEval ( BSONObjBuilder & builder,
                                              UINT64 totalSize,
                                              UINT32 totalPages ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // max( 1, ceil( RecordTotalSize / PageUnit ) )
      formBuilder << "max( 1, ceil( "
                  << OPT_FIELD_RECORD_TOTAL_SIZE << " / "
                  << OPT_FIELD_PAGE_UINT << ") )" ;

      evalBuilder << "max( 1, ceil( "
                  << totalSize << " / " << DMS_PAGE_SIZE_BASE << ") )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_PAGES,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)totalPages ) ;
   }

   INT32 _optSortNode::_toBSONIOCostEval ( BSONObjBuilder & builder,
                                           UINT32 totalPages ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // ceil( Pages * ( SeqWrtIOCostUnit + SeqReadIOCostUnit * 0.75 +
      //                 RandomReadIOCostUnit * 0.25 ) )
      formBuilder << "ceil( " << OPT_FIELD_PAGES << " * ( "
                  << OPT_FIELD_SEQ_WRITE_IO_COST << " + "
                  << OPT_FIELD_SEQ_IO_COST << " * 0.75 + "
                  << OPT_FIELD_RAN_IO_COST << " * 0.25 ) )" ;

      evalBuilder << "ceil( " << totalPages << " * ( "
                  << OPT_SEQ_WRITE_IO_COST << " + "
                  << OPT_SEQ_SCAN_IO_COST << " * 0.75 + "
                  << OPT_RANDOM_SCAN_IO_COST << " * 0.25 ) )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_IO_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estIOCost ) ;
   }

   INT32 _optSortNode::_toBSONCPUCostEval ( BSONObjBuilder & builder,
                                            UINT64 records ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // ceil( 2 * OptrCPUCost * SortFields * max( 2, Records ) *
      //       log2( max( 2, Records ) ) )
      formBuilder << "ceil( 2 * " << OPT_FIELD_OPTR_CPU_COST << " * "
                  << OPT_FIELD_SORT_FIELDS << " * "
                  << "max( 2, " << OPT_FIELD_RECORDS << " ) * log2( "
                  << "max( 2, " << OPT_FIELD_RECORDS << " ) ) )" ;

      evalBuilder << "ceil( 2 * " << OPT_OPTR_BASE_CPU_COST << " * "
                  << _orderBy.nFields() << " * "
                  << "max( 2, " << records << " ) * log2( "
                  << "max( 2, " << records << " ) ) )" ;

      return _toBSONFieldEval( builder, OPT_FIELD_CPU_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estCPUCost ) ;
   }

   INT32 _optSortNode::_toBSONStartCostEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // ChildTotalCost + IOCPURate * IOCost + CPUCost
      formBuilder << OPT_FIELD_CHILD_TOTAL_COST << " + "
                  << OPT_FIELD_IO_CPU_RATE << " * "
                  << OPT_FIELD_IO_COST << " + "
                  << OPT_FIELD_CPU_COST ;

      evalBuilder << _getInputTotalCost() << " + "
                  << OPT_IO_CPU_RATE << " * "
                  << _estIOCost << " + "
                  << _estCPUCost ;

      return _toBSONFieldEval( builder, OPT_FIELD_START_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estStartCost ) ;
   }

   INT32 _optSortNode::_toBSONRunCostEval ( BSONObjBuilder & builder,
                                            UINT64 records ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // OptrCPUCost * Records
      formBuilder << OPT_FIELD_OPTR_CPU_COST << " * "
                  << OPT_FIELD_RECORDS ;
      evalBuilder << OPT_OPTR_BASE_CPU_COST << " * "
                  << records ;

      return _toBSONFieldEval( builder, OPT_FIELD_RUN_COST,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_estRunCost ) ;
   }

   INT32 _optSortNode::_toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const
   {
      StringBuilder formBuilder, evalBuilder ;

      // Records
      formBuilder << OPT_FIELD_RECORDS ;
      evalBuilder << _outputRecords ;

      return _toBSONFieldEval( builder, OPT_FIELD_OUTPUT_RECORDS,
                               formBuilder.str().c_str(),
                               evalBuilder.str().c_str(),
                               (INT64)_outputRecords ) ;
   }

   /*
      _optMergeNodeBase implement
    */
   _optMergeNodeBase::_optMergeNodeBase ()
   : _optPlanNode(),
     _needReorder( FALSE ),
     _outputSkipRecords( 0 ),
     _inputRecords( 0 )
   {
   }

   _optMergeNodeBase::_optMergeNodeBase ( const rtnContext * context )
   : _optPlanNode(),
     _outputSkipRecords( 0 ),
     _inputRecords( 0 )
   {
      const rtnContextMain *mainContext =
            dynamic_cast<const rtnContextMain *>( context ) ;
      SDB_ASSERT( NULL != mainContext, "context is invalid" ) ;

      _needReorder = mainContext->requireOrder() ;
      _orderBy = mainContext->getQueryOptions().getOrderBy() ;

      _returnOptions = mainContext->getQueryOptions() ;
      _returnOptions.setSkip( mainContext->getNumToSkip() ) ;
      _returnOptions.setLimit( mainContext->getNumToReturn() ) ;
      _setReturnSelector( mainContext ) ;
   }

   _optMergeNodeBase::~_optMergeNodeBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE_ADDCHILDEXP, "_optMergeNodeBase::addChildExplain" )
   INT32 _optMergeNodeBase::addChildExplain ( const BSONObj & childExplain,
                                              const ossTickDelta & queryTime,
                                              const ossTickDelta & waitTime,
                                              BOOLEAN needParse,
                                              BOOLEAN needChildExplain,
                                              const rtnExplainOptions &expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE_ADDCHILDEXP ) ;

      optPlanNode * newNode = NULL ;

      try
      {
         // Make sure result is owned
         BSONObj explainResult = childExplain.getOwned() ;

         if ( needChildExplain )
         {
            _childExplainList.push_back( explainResult ) ;
         }
         _childBackupList.push_back( explainResult ) ;

         if ( needParse )
         {
            BSONObj pathObject ;
            const CHAR * optrName = NULL ;
            const CHAR * childNodeName = NULL ;
            ossTickConversionFactor factor ;
            UINT32 seconds = 0, microseconds = 0 ;
            double realQueryTime = 0.0 ;
            optChildNodeSummary childSummary ;

            rc = rtnGetStringElement( explainResult, _getChildExplainName(),
                                      &childNodeName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         _getChildExplainName(), rc ) ;

            rc = rtnGetDoubleElement( explainResult, OPT_FIELD_ELAPSED_TIME,
                                      realQueryTime ) ;
            if ( SDB_OK != rc )
            {
               queryTime.convertToTime( factor, seconds, microseconds ) ;
               realQueryTime = (double)( seconds ) +
                               (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;
               rc = SDB_OK ;
            }

            rc = rtnGetObjElement( explainResult, OPT_FIELD_PLAN_PATH,
                                   pathObject ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         OPT_FIELD_PLAN_PATH, rc ) ;

            rc = rtnGetStringElement( pathObject, OPT_FIELD_OPERATOR,
                                      &optrName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         OPT_FIELD_OPERATOR, rc ) ;

            if ( 0 == ossStrcmp( optrName, OPT_PLAN_NODE_NAME_TBSCAN ) )
            {
               newNode = SDB_OSS_NEW optTbScanNode() ;
            }
            else if ( 0 == ossStrcmp( optrName, OPT_PLAN_NODE_NAME_IXSCAN ) )
            {
               newNode = SDB_OSS_NEW optIxScanNode() ;
            }
            else if ( 0 == ossStrcmp( optrName, OPT_PLAN_NODE_NAME_SORT ) )
            {
               newNode = SDB_OSS_NEW optSortNode() ;
            }
            else if ( 0 == ossStrcmp( optrName, OPT_PLAN_NODE_NAME_MERGE ) )
            {
               newNode = SDB_OSS_NEW optMainCLMergeNode() ;
            }
            else if ( 0 == ossStrcmp( optrName, OPT_PLAN_NODE_NAME_COORD ) )
            {
               newNode = SDB_OSS_NEW optCoordMergeNode() ;
            }
            else
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to rebuild from BSON, "
                       "unknown operator [%s]", optrName ) ;
               goto error ;
            }

            PD_CHECK( newNode, SDB_OOM, error, PDERROR,
                      "Failed to allocate %s node", optrName ) ;

            rc = newNode->fromBSON( pathObject, expOptions ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for %s node, "
                         "rc: %d", optrName, rc ) ;

            _childNodes.push_back( newNode ) ;

            // Construct summary
            childSummary._name = childNodeName ;
            childSummary._estTotalCost = newNode->getEstTotalCost() ;
            childSummary._queryTime = realQueryTime ;
            waitTime.convertToTime( factor, seconds, microseconds ) ;
            childSummary._waitTime = (double)( seconds ) +
                                     (double)( microseconds ) /
                                     (double)( OSS_ONE_MILLION ) ;

            _childSummary.push_back( childSummary ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to extract node info, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE_ADDCHILDEXP, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( newNode ) ;
      // Ignore errors
      rc = SDB_OK ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE_EVALUATE, "_optMergeNodeBase::evaluate" )
   void _optMergeNodeBase::evaluate ()
   {
      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE_EVALUATE ) ;

      _evaluateOutput() ;

      if ( 0 == _outputRecords )
      {
         _evaluateEmptyRunCost() ;
      }
      else if ( _needReorder )
      {
         _evaluateOrderedRunCost() ;
      }
      else
      {
         _evaluateNormalRunCost() ;
      }

      _estTotalCost = _estStartCost + _estRunCost ;

      PD_TRACE_EXIT( SDB_OPTMERGENODEBASE_EVALUATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE__EVALOUTPUT, "_optMergeNodeBase::_evaluateOutput" )
   void _optMergeNodeBase::_evaluateOutput ()
   {
      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE__EVALOUTPUT ) ;

      UINT32 outputRecordSize = 0 ;
      UINT32 outputNumFields = 0 ;

      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         UINT64 curOutputRecords = node->getOutputRecords() ;
         UINT32 curOutputRecordSize = node->getOutputRecordSize() ;
         UINT32 curOutputNumFields = node->getOutputNumFields() ;

         if ( 0 == _inputRecords )
         {
            outputRecordSize = curOutputRecordSize ;
            outputNumFields = curOutputNumFields ;
         }
         else if ( 0 != curOutputRecords )
         {
            double rate = (double)_inputRecords /
                          (double)( curOutputRecords + _inputRecords ) ;
            outputRecordSize = (UINT32)ceil( (double)outputRecordSize * rate +
                                             (double)curOutputRecordSize * ( 1.0 - rate ) ) ;
            outputNumFields = (UINT32)ceil( (double)outputNumFields * rate +
                                            (double)curOutputNumFields * ( 1.0 - rate ) ) ;
         }

         _inputRecords += curOutputRecords ;
      }

      _outputRecords = _evaluateOutputRecords( _inputRecords,
                                               _outputSkipRecords ) ;
      _outputRecordSize = outputRecordSize ;
      _outputNumFields = outputNumFields ;

      _sorted = !_orderBy.isEmpty() ;

      // No child node or single child node, no need to reorder
      if ( _sorted && _needReorder && _childNodes.size() <= 1 )
      {
         _needReorder = FALSE ;
      }

      PD_TRACE_EXIT( SDB_OPTMERGENODEBASE__EVALOUTPUT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE_TOSIMPLEBSON, "_optMergeNodeBase::toSimpleBSON" )
   INT32 _optMergeNodeBase::toSimpleBSON ( BSONObjBuilder & builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE_TOSIMPLEBSON ) ;

      rtnExplainOptions expOptions;
      expOptions.setNeedExpand( TRUE ) ;

      rc = _toBSONChildNodes( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate BSON for child nodes, "
                   "rc: %d" ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE_TOSIMPLEBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE__TOBSONBASIC, "_optMergeNodeBase::_toBSONBasic" )
   INT32 _optMergeNodeBase::_toBSONBasic ( BSONObjBuilder & builder,
                                           const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE__TOBSONBASIC ) ;

      builder.append( OPT_FIELD_OPERATOR, getName() ) ;
      builder.appendEx( OPT_FIELD_SORT,
                        _orderBy,
                        expOptions.getBuilderOption() ) ;
      builder.appendBool( OPT_FIELD_NEED_REORDER, _needReorder ) ;

      rc = _toBSONChildList( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for child list, "
                   "rc: %d", rc ) ;

      rc = _toBSONReturnOptions( builder, expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for return options, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE__TOBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE__FROMBSONBASIC, "_optMergeNodeBase::_fromBSONBasic" )
   INT32 _optMergeNodeBase::_fromBSONBasic ( const BSONObj & object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE__FROMBSONBASIC ) ;

      try
      {
         const CHAR *nodeName = NULL ;

         rc = rtnGetStringElement( object, OPT_FIELD_OPERATOR,
                                   &nodeName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_OPERATOR, rc ) ;

         PD_CHECK( 0 == ossStrcmp( nodeName, getName() ),
                   SDB_SYS, error, PDERROR, "Failed to get field [%s], "
                   "node names [%s] and [%s] are different",
                   OPT_FIELD_OPERATOR, nodeName, getName() ) ;

         rc = rtnGetObjElement( object, OPT_FIELD_SORT, _orderBy ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_SORT, rc ) ;

         rc = rtnGetBooleanElement( object, OPT_FIELD_NEED_REORDER,
                                    _needReorder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      OPT_FIELD_NEED_REORDER, rc ) ;

         rc = _fromBSONReturnOptions( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON for return options, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for node info, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE__FROMBSONBASIC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE__TOBSONCHILDLST, "_optMergeNodeBase::_toBSONChildList" )
   INT32 _optMergeNodeBase::_toBSONChildList ( BSONObjBuilder & builder,
                                               const rtnExplainOptions &expOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE__TOBSONCHILDLST ) ;

      builder.append( _getChildNumName(), (INT32)_childSummary.size() ) ;
      BSONArrayBuilder subBuilder(
            builder.subarrayStart( _getChildListName() ) ) ;

      for ( optChildSummaryList::const_iterator iter = _childSummary.begin() ;
            iter != _childSummary.end() ;
            iter ++ )
      {
         BSONObjBuilder summaryBuilder( subBuilder.subobjStart() ) ;

         summaryBuilder.append( OPT_FIELD_SUMMARY_NAME, iter->_name ) ;

         if ( expOptions.isNeedEstimate() )
         {
            summaryBuilder.append( OPT_FIELD_SUMMARY_EST_COST,
                                   (double)( iter->_estTotalCost *
                                             OPT_COST_TO_SEC ) ) ;
         }

         if ( expOptions.isNeedRun() )
         {
            summaryBuilder.append( OPT_FIELD_QUERY_TIME_SPENT, iter->_queryTime ) ;
            summaryBuilder.append( OPT_FIELD_WAIT_TIME_SPENT, iter->_waitTime ) ;
         }

         summaryBuilder.done() ;
      }

      subBuilder.done() ;

      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE__TOBSONCHILDLST, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODEBASE__TOBSONMERGECHILDNODES, "_optMergeNodeBase::_toBSONMergeChildNodes" )
   INT32 _optMergeNodeBase::_toBSONMergeChildNodes ( BSONArrayBuilder & builder,
                                                     BOOLEAN needPlanPath,
                                                     BOOLEAN needNodeInfo ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTMERGENODEBASE__TOBSONMERGECHILDNODES ) ;

      for ( optExplainResultList::const_iterator iter = _childExplainList.begin() ;
            iter != _childExplainList.end() ;
            iter ++ )
      {
         BSONObjBuilder subBuilder( builder.subobjStart() ) ;

         const BSONObj &subExplain = ( *iter ) ;
         BSONObjIterator subIter( subExplain ) ;

         while( subIter.more() )
         {
            BSONElement subElement = subIter.next() ;

            if ( ( 0 == ossStrcmp( subElement.fieldName(),
                                   OPT_FIELD_NODE_NAME ) ||
                   0 == ossStrcmp( subElement.fieldName(),
                                   OPT_FIELD_GROUP_NAME ) ||
                   0 == ossStrcmp( subElement.fieldName(),
                                   OPT_FIELD_ROLE ) ) &&
                 !needNodeInfo )
            {
               // Skip node info
               continue ;
            }
            else if ( 0 == ossStrcmp( subElement.fieldName(),
                                      OPT_FIELD_PLAN_PATH ) &&
                      Object == subElement.type() &&
                      !needPlanPath )
            {
               // Skip plan path
               continue ;
            }
            else
            {
               subBuilder.append( subElement ) ;
            }
         }

         subBuilder.done() ;
      }

      PD_TRACE_EXITRC( SDB_OPTMERGENODEBASE__TOBSONMERGECHILDNODES, rc ) ;

      return rc ;
   }

   /*
      _optMainCLMergeNode implement
    */
   _optMainCLMergeNode::_optMainCLMergeNode ()
   : _optMergeNodeBase()
   {
   }

   _optMainCLMergeNode::_optMainCLMergeNode ( const rtnContext * context )
   : _optMergeNodeBase( context )
   {
   }

   _optMainCLMergeNode::~_optMainCLMergeNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODE__EVALEMPRUNCOST, "_optMainCLMergeNode::_evaluateEmptyRunCost" )
   void _optMainCLMergeNode::_evaluateEmptyRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTMERGENODE__EVALEMPRUNCOST ) ;

      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         _estStartCost += node->getEstStartCost() ;
         _estRunCost += node->getEstRunCost() ;
      }

      PD_TRACE_EXIT( SDB_OPTMERGENODE__EVALEMPRUNCOST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODE__EVALORDRUNCOST, "_optMainCLMergeNode::_evaluateOrderedRunCost" )
   void _optMainCLMergeNode::_evaluateOrderedRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTMERGENODE__EVALORDRUNCOST ) ;

      UINT32 numOrderByFields = (UINT32)_orderBy.nFields() ;
      UINT64 comparisionCPUCost = 2 * OPT_OPTR_BASE_CPU_COST * numOrderByFields ;

      BOOLEAN allDataNeeded = ( ( _outputSkipRecords + _outputRecords ) ==
                                _inputRecords ) ;
      double skipRate = (double)_outputSkipRecords  / (double)_inputRecords ;
      double returnRate = 1.0 - skipRate ;

      if ( !allDataNeeded )
      {
         returnRate = (double)_outputRecords  / (double)_inputRecords ;
      }

      // Iterate all input nodes
      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         _estStartCost += ( node->getEstStartCost() +
                            (UINT64)ceil( (double)node->getEstRunCost() * skipRate ) ) ;
         _estRunCost += (UINT64)ceil( (double)node->getEstRunCost() * returnRate ) ;
      }

      // CPU cost
      _estStartCost += ( ( comparisionCPUCost + OPT_OPTR_BASE_CPU_COST ) *
                         _outputSkipRecords ) ;
      _estRunCost += ( ( comparisionCPUCost + OPT_OPTR_BASE_CPU_COST ) *
                       _outputRecords ) ;

      PD_TRACE_EXIT( SDB_OPTMERGENODE__EVALORDRUNCOST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTMERGENODE__EVALNMLRUNCOST, "_optMainCLMergeNode::_evaluateNormalRunCost" )
   void _optMainCLMergeNode::_evaluateNormalRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTMERGENODE__EVALNMLRUNCOST ) ;

      UINT64 skipRecords = _outputSkipRecords ;
      UINT64 returnRecords = _outputRecords ;

      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() && returnRecords > 0 ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         if ( skipRecords > 0 ||
              returnRecords == _outputRecords )
         {
            _estStartCost += node->getEstStartCost() ;
         }
         else
         {
            _estRunCost += node->getEstStartCost() ;
         }

         if ( 0 == node->getOutputRecords() )
         {
            if ( returnRecords == _outputRecords )
            {
               _estStartCost += node->getEstRunCost() ;
            }
            else
            {
               _estRunCost += node->getEstRunCost() ;
            }
         }
         else if ( skipRecords > node->getOutputRecords() )
         {
            _estStartCost += node->getEstRunCost() ;
            _estStartCost += ( OPT_OPTR_BASE_CPU_COST *
                               node->getOutputRecords() ) ;
            skipRecords -= node->getOutputRecords() ;
         }
         else if ( skipRecords > 0 )
         {
            double avgCost = (double)node->getEstRunCost() /
                             (double)node->getOutputRecords() ;
            UINT64 remainRecords = node->getOutputRecords() - skipRecords ;
            if ( remainRecords > returnRecords )
            {
               remainRecords = returnRecords ;
            }
            _estStartCost += (UINT64)ceil( (double)skipRecords * avgCost ) ;
            _estStartCost += OPT_OPTR_BASE_CPU_COST * skipRecords ;
            _estRunCost += (UINT64)ceil( (double)remainRecords * avgCost ) ;
            _estRunCost += ( OPT_OPTR_BASE_CPU_COST * remainRecords ) ;
            returnRecords -= remainRecords ;
            skipRecords = 0 ;
         }
         else if ( node->getOutputRecords() > returnRecords )
         {
            double avgCost = (double)node->getEstRunCost() /
                             (double)node->getOutputRecords() ;
            _estRunCost += (UINT64)ceil( (double)returnRecords * avgCost ) ;
            _estRunCost += ( OPT_OPTR_BASE_CPU_COST * returnRecords ) ;
            returnRecords = 0 ;
         }
         else
         {
            _estRunCost += node->getEstRunCost() ;
            _estRunCost += ( OPT_OPTR_BASE_CPU_COST *
                             node->getOutputRecords() ) ;
            returnRecords -= node->getOutputRecords() ;
         }
      }

      PD_TRACE_EXIT( SDB_OPTMERGENODE__EVALNMLRUNCOST ) ;
   }

   /*
      _optCoordMergeNode implement
    */
   _optCoordMergeNode::_optCoordMergeNode ()
   : _optMergeNodeBase()
   {
   }

   _optCoordMergeNode::_optCoordMergeNode ( const rtnContext * context )
   : _optMergeNodeBase( context )
   {
   }

   _optCoordMergeNode::~_optCoordMergeNode ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCOORDNODE__EVALEMPRUNCOST, "_optCoordMergeNode::_evaluateEmptyRunCost" )
   void _optCoordMergeNode::_evaluateEmptyRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTCOORDNODE__EVALEMPRUNCOST ) ;

      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         if ( iter == _childNodes.begin() )
         {
            _estStartCost = node->getEstStartCost() ;
            _estRunCost = node->getEstRunCost() ;
         }
         else
         {
            if ( _estStartCost > node->getEstStartCost() )
            {
               _estStartCost = node->getEstStartCost() ;
            }
            if ( _estRunCost < node->getEstRunCost() )
            {
               _estRunCost = node->getEstRunCost() ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_OPTCOORDNODE__EVALEMPRUNCOST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCOORDNODE__EVALORDRUNCOST, "_optCoordMergeNode::_evaluateOrderedRunCost" )
   void _optCoordMergeNode::_evaluateOrderedRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTCOORDNODE__EVALORDRUNCOST ) ;

      UINT32 numOrderByFields = (UINT32)_orderBy.nFields() ;
      UINT64 comparisionCPUCost = 2 * OPT_OPTR_BASE_CPU_COST * numOrderByFields ;

      BOOLEAN allDataNeeded = ( ( _outputSkipRecords + _outputRecords ) ==
                                _inputRecords ) ;
      double skipRate = (double)_outputSkipRecords  / (double)_inputRecords ;
      double returnRate = 1.0 - skipRate ;

      if ( !allDataNeeded )
      {
         returnRate = (double)_outputRecords  / (double)_inputRecords ;
      }

      // Iterate all input nodes
      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         UINT64 curStartCost = ( node->getEstStartCost() +
                                 (UINT64)ceil( (double)node->getEstRunCost() *
                                               skipRate ) ) ;
         UINT64 curRunCost = (UINT64)ceil( (double)node->getEstRunCost() *
                                           returnRate ) ;

         if ( iter == _childNodes.begin() )
         {
            _estStartCost = curStartCost ;
            _estRunCost = curRunCost ;
         }
         else
         {
            if ( _estStartCost < curStartCost )
            {
               _estStartCost = curStartCost ;
            }
            if ( _estRunCost < curRunCost )
            {
               _estRunCost = curRunCost ;
            }
         }
      }

      // CPU cost
      _estStartCost += ( ( comparisionCPUCost + OPT_OPTR_BASE_CPU_COST ) *
                         _outputSkipRecords ) ;
      _estRunCost += ( ( comparisionCPUCost + OPT_OPTR_BASE_CPU_COST ) *
                       _outputRecords ) ;

      PD_TRACE_EXIT( SDB_OPTCOORDNODE__EVALORDRUNCOST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCOORDNODE__EVALNMLRUNCOST, "_optCoordMergeNode::_evaluateNormalRunCost" )
   void _optCoordMergeNode::_evaluateNormalRunCost ()
   {
      PD_TRACE_ENTRY( SDB_OPTCOORDNODE__EVALNMLRUNCOST ) ;

      BOOLEAN allDataNeeded = ( ( _outputSkipRecords + _outputRecords ) ==
                                _inputRecords ) ;
      double skipRate = (double)_outputSkipRecords  / (double)_inputRecords ;
      double returnRate = 1.0 - skipRate ;

      if ( !allDataNeeded )
      {
         returnRate = (double)_outputRecords  / (double)_inputRecords ;
      }

      // Iterate all input nodes
      for ( optPlanNodeList::const_iterator iter = _childNodes.begin() ;
            iter != _childNodes.end() ;
            iter ++ )
      {
         const optPlanNode *node = ( *iter ) ;
         SDB_ASSERT( NULL != node, "node is invalid" ) ;

         UINT64 curStartCost = ( node->getEstStartCost() +
                                 (UINT64)ceil( (double)node->getEstRunCost() *
                                               skipRate ) ) ;
         UINT64 curRunCost = (UINT64)ceil( (double)node->getEstRunCost() *
                                           returnRate ) ;

         if ( iter == _childNodes.begin() )
         {
            _estStartCost = curStartCost ;
            _estRunCost = curRunCost ;
         }
         else
         {
            if ( _estStartCost > curStartCost )
            {
               _estStartCost = curStartCost ;
            }
            if ( _estRunCost < curRunCost )
            {
               _estRunCost = curRunCost ;
            }
         }
      }

      // CPU cost
      _estStartCost += ( OPT_OPTR_BASE_CPU_COST * _outputSkipRecords ) ;
      _estRunCost += ( OPT_OPTR_BASE_CPU_COST * _outputRecords ) ;

      PD_TRACE_EXIT( SDB_OPTCOORDNODE__EVALNMLRUNCOST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCOORDNODE__TOBSONRUNIMPL, "_optCoordMergeNode::_toBSONRunImpl" )
   INT32 _optCoordMergeNode::_toBSONRunImpl ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTCOORDNODE__TOBSONRUNIMPL ) ;

      ossTickConversionFactor factor ;
      UINT32 seconds = 0, microseconds = 0 ;
      double waitTime = 0.0 ;

      rc = _optMergeNodeBase::_toBSONRunImpl( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                   "rc: %d", rc ) ;

      _runtimeMonitor.getWaitTime().convertToTime( factor, seconds, microseconds ) ;
      waitTime = (double)( seconds ) +
                 (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;

      builder.append( OPT_FIELD_WAIT_TIME_SPENT, waitTime ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTCOORDNODE__TOBSONRUNIMPL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
