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

   Source File Name = rtnContextExplain.cpp

   Descriptive Name = RunTime Explain Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/15/2017  HGM Split from rtnContextData.cpp

   Last Changed =

*******************************************************************************/

#include "rtnContextExplain.hpp"
#include "rtnContextMain.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"

namespace engine
{

   /*
      _rtnExplainBase implement
    */
   _rtnExplainBase::_rtnExplainBase ( optExplainPath * explainPath )
   : _rtnSubContextHolder(),
     _explainMask( OPT_NODE_EXPLAIN_MASK_NONE ),
     _needDetail( FALSE ),
     _needEstimate( FALSE ),
     _needRun( FALSE ),
     _needSearch( FALSE ),
     _needEvaluate( FALSE ),
     _needExpand( FALSE ),
     _needFlatten( FALSE ),
     _explainStarted( FALSE ),
     _explainRunned( FALSE ),
     _explainPrepared( FALSE ),
     _explained( FALSE ),
     _explainPath( explainPath )
   {
      SDB_ASSERT( NULL != explainPath, "explain path is invalid" ) ;
   }

   _rtnExplainBase::~_rtnExplainBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__OPENEXP, "_rtnExplainBase::_openExplain" )
   INT32 _rtnExplainBase::_openExplain ( const rtnQueryOptions & options,
                                         pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__OPENEXP ) ;

      BSONObj explainOptions, realHint ;
      rtnContext * queryContext = NULL ;
      rtnQueryOptions subOptions = options ;

      _queryOptions = options ;

      rc = _extractExplainOptions( options.getHint(), explainOptions,
                                   realHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract explain options, "
                   "rc: %d", rc ) ;

      rc = _parseExplainOptions( explainOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse explain options, "
                   "rc: %d", rc ) ;

      _queryOptions.setHint( realHint ) ;

      if ( _queryOptions.testFlag( FLG_QUERY_MODIFY ) &&
           !_queryOptions.isOrderByEmpty() )
      {
         _queryOptions.setFlag( FLG_QUERY_FORCE_IDX_BY_SORT ) ;
      }

      _queryOptions.clearFlag( FLG_QUERY_EXPLAIN ) ;

      rc = _queryOptions.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get query options owned, "
                   "rc: %d", rc ) ;

      if ( _needResetExplainOptions() )
      {
         subOptions = _queryOptions ;
         subOptions.clearFlag( FLG_QUERY_MODIFY ) ;
      }

      rc = _openSubContext( subOptions, cb, &queryContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open query context, rc: %d", rc ) ;

      SDB_ASSERT( NULL != queryContext, "query context is invalid" ) ;

      queryContext->setEnableQueryActivity( FALSE ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         queryContext->getMonCB()->recordStartTimestamp() ;
      }

      _setSubContext( queryContext, cb ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__OPENEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PREPAREEXP, "_rtnExplainBase::_prepareExplain" )
   INT32 _rtnExplainBase::_prepareExplain ( rtnContext * explainContext,
                                            pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PREPAREEXP ) ;

      SDB_ASSERT( NULL != explainContext, "explain context is invalid" ) ;

      rtnContext * queryContext = _getSubContext() ;
      pmdEDUCB * queryCB = _getSubContextCB() ;

      if ( _explained )
      {
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      PD_CHECK( NULL != queryContext && NULL != queryCB,
                SDB_SYS, error, PDERROR, "Failed to get query context" ) ;

      if ( !_explainStarted )
      {
         rc = _explainPath->setExplainStart( queryCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set explain start, "
                      "rc: %d", rc ) ;
         _explainStarted = TRUE ;
      }

      if ( !_explainRunned && ( _needRun || _needCollectSubExplains() ) )
      {
         for ( ; ; )
         {
            rtnContextBuf contextBuf ;
            rc = queryContext->getMore( -1, contextBuf, queryCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else if ( SDB_OK == rc )
            {
               if ( _needRun && _needReturnDataInRun() )
               {
                  rc = explainContext->appendObjs( contextBuf.data(),
                                                   contextBuf.size(),
                                                   contextBuf.recordNum() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed process data in run mode, "
                               "rc: %d", rc ) ;

                  goto done ;
               }
            }
            else
            {
               PD_RC_CHECK( rc, PDERROR, "Failed to get data from query "
                            "context [%lld], rc: %d",
                            queryContext->contextID(), rc ) ;
            }
         }

         _explainRunned = TRUE ;
      }

      if ( !_explainPrepared )
      {
         rc = _explainPath->setExplainEnd( queryContext, queryCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set explain end, rc: %d", rc ) ;

         rc = _finishSubContext( queryContext, queryCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to finish query context [%lld], "
                   "rc: %d", queryContext->contextID(), rc ) ;

         rc = _prepareExplainPath( queryContext, queryCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare explain path, rc: %d", rc ) ;

         _explainPrepared = TRUE ;
      }

      try
      {
         BOOLEAN hasMore = FALSE ;

         rc = _buildExplain( explainContext, hasMore ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build explain result, "
                      "rc: %d", rc ) ;

         if ( !hasMore )
         {
            _explained = TRUE ;
         }
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Failed to prepare explain, received unexpected "
                 "error: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _needResetTotalRecords() )
      {
         explainContext->_resetTotalRecords( RTN_CTX_EXPLAIN_PROCESSOR +
                                             explainContext->numRecords() ) ;
      }

      if ( _explained )
      {
         _deleteSubContext() ;
         explainContext->_hitEnd = TRUE ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PREPAREEXP, rc ) ;
      return rc ;

   error :
      _deleteSubContext() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__EXTRACTEXPOPTS, "_rtnExplainBase::_extractExplainOptions" )
   INT32 _rtnExplainBase::_extractExplainOptions ( const BSONObj & hint,
                                                   BSONObj & explainOptions,
                                                   BSONObj & realHint )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__EXTRACTEXPOPTS ) ;

      try
      {
         BSONElement element ;

         element = hint.getField( FIELD_NAME_OPTIONS ) ;
         if ( Object == element.type() )
         {
            explainOptions = element.embeddedObject() ;
         }

         element = hint.getField( FIELD_NAME_HINT ) ;
         if ( Object == element.type() )
         {
            realHint = element.embeddedObject() ;
         }
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__EXTRACTEXPOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PARSEEXPOPTS, "_rtnExplainBase::_parseExplainOptions" )
   INT32 _rtnExplainBase::_parseExplainOptions ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PARSEEXPOPTS ) ;

      BOOLEAN hasMask = TRUE ;
      BOOLEAN hasOption = FALSE ;

      rc = _parseBoolOption( options, FIELD_NAME_RUN, _needRun,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_RUN, rc ) ;

      rc = _parseBoolOption( options, FIELD_NAME_DETAIL, _needDetail,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_DETAIL, rc ) ;

      rc = _parseBoolOption( options, FIELD_NAME_EXPAND, _needExpand,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_EXPAND, rc ) ;

      if ( hasOption )
      {
         _needDetail = TRUE ;
      }

      rc = _parseBoolOption( options, FIELD_NAME_FLATTEN, _needFlatten,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_FLATTEN, rc ) ;

      if ( hasOption )
      {
         _needExpand = TRUE ;
         _needDetail = TRUE ;
      }

      rc = _parseBoolOption( options, FIELD_NAME_SEARCH, _needSearch,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_SEARCH, rc ) ;

      if ( hasOption )
      {
         _needDetail = TRUE ;
         _needExpand = TRUE ;
      }

      rc = _parseBoolOption( options, FIELD_NAME_EVALUATE, _needEvaluate,
                             hasOption, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_EVALUATE, rc ) ;

      if ( hasOption )
      {
         _needDetail = TRUE ;
         _needExpand = TRUE ;
         _needSearch = TRUE ;
      }

      rc = _parseMaskOption( options, FIELD_NAME_FILTER, hasMask,
                             _explainMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_FILTER, rc ) ;

      if ( hasMask )
      {
         _needDetail = TRUE ;
      }

      rc = _parseLocationOption ( options, hasOption ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_LOCATION, rc ) ;

      if ( hasOption )
      {
         _needDetail = TRUE ;
      }

      rc = _parseBoolOption ( options, FIELD_NAME_ESTIMATE, _needEstimate,
                              hasOption, _needDetail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse %s option, rc: %d",
                   FIELD_NAME_ESTIMATE, rc ) ;

      if ( hasOption )
      {
         _needDetail = TRUE ;
      }

      if ( _needDetail )
      {
         if ( _needEstimate )
         {
            OSS_BIT_SET( _explainMask, OPT_NODE_EXPLAIN_MASK_ESTIMATE ) ;
         }
         else
         {
            OSS_BIT_CLEAR( _explainMask, OPT_NODE_EXPLAIN_MASK_ESTIMATE ) ;
         }
         if ( _needRun )
         {
            OSS_BIT_SET( _explainMask, OPT_NODE_EXPLAIN_MASK_RUN ) ;
         }
         else
         {
            OSS_BIT_CLEAR( _explainMask, OPT_NODE_EXPLAIN_MASK_RUN ) ;
         }
         if ( !hasMask )
         {
            OSS_BIT_SET( _explainMask, OPT_NODE_EXPLAIN_MASK_INPUT |
                                       OPT_NODE_EXPLAIN_MASK_FILTER |
                                       OPT_NODE_EXPLAIN_MASK_OUTPUT ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PARSEEXPOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PARSELOCFILTER, "_rtnExplainBase::_parseLocationOption" )
   INT32 _rtnExplainBase::_parseLocationOption ( const BSONObj & explainOptions,
                                                 BOOLEAN & hasOption )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PARSELOCFILTER ) ;

      if ( explainOptions.hasField( FIELD_NAME_SUB_COLLECTIONS ) ||
           explainOptions.hasField( FIELD_NAME_LOCATION ) )
      {
         hasOption = TRUE ;
      }
      else
      {
         hasOption = FALSE ;
      }

      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PARSELOCFILTER, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PARSEBOOLOPT, "_rtnExplainBase::_parseBoolOption" )
   INT32 _rtnExplainBase::_parseBoolOption ( const BSONObj & options,
                                             const CHAR * optionName,
                                             BOOLEAN & option,
                                             BOOLEAN & hasOption,
                                             BOOLEAN defaultValue )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PARSEBOOLOPT ) ;

      hasOption = FALSE ;

      try
      {
         BSONElement e = options.getField( optionName ) ;

         if ( e.eoo() )
         {
            option = defaultValue ;
            goto done ;
         }
         else if ( e.isNumber() )
         {
            option = e.numberInt() == 0 ? FALSE : TRUE ;
         }
         else if ( e.isBoolean() )
         {
            option = e.booleanSafe() ;
         }
         else
         {
            option = defaultValue ;
         }

         hasOption = TRUE ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PARSEBOOLOPT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PARSEMASKOPT, "_rtnExplainBase::_parseMaskOption" )
   INT32 _rtnExplainBase::_parseMaskOption ( const BSONObj & options,
                                             const CHAR * optionName,
                                             BOOLEAN & hasMask,
                                             UINT16 & mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PARSEMASKOPT ) ;

      hasMask = FALSE ;
      mask = OPT_NODE_EXPLAIN_MASK_NONE ;

      try
      {
         BSONElement e = options.getField( optionName ) ;

         if ( e.eoo() )
         {
            mask = OPT_NODE_EXPLAIN_MASK_NONE ;
            goto done ;
         }
         else if ( e.isNull() )
         {
            mask = OPT_NODE_EXPLAIN_MASK_NONE ;
         }
         else if ( String == e.type() )
         {
            _parseMask( e.valuestrsafe(), mask ) ;
         }
         else if ( Array == e.type() )
         {
            BSONObjIterator iter( e.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement subElement = iter.next() ;
               if ( String == subElement.type() )
               {
                  UINT16 subMask = OPT_NODE_EXPLAIN_MASK_NONE ;
                  rc = _parseMask( subElement.valuestrsafe(), subMask ) ;
                  if ( SDB_OK != rc )
                  {
                     break ;
                  }
                  else if ( OPT_NODE_EXPLAIN_MASK_NONE == subMask ||
                            OPT_NODE_EXPLAIN_MASK_ALL == subMask )
                  {
                     mask = subMask ;
                     break ;
                  }
                  else
                  {
                     mask |= subMask ;
                  }
               }
               else
               {
                  break ;
               }
            }
         }
         else
         {
            mask = OPT_NODE_EXPLAIN_MASK_NONE ;
         }

         hasMask = TRUE ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PARSEMASKOPT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__PARSEMASK, "_rtnExplainBase::_parseMask" )
   INT32 _rtnExplainBase::_parseMask ( const CHAR * maskName,
                                       UINT16 & mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__PARSEMASK ) ;

      if ( 0 == ossStrcmp( maskName, OPT_EXPLAIN_MASK_NONE_NAME ) )
      {
         mask = OPT_NODE_EXPLAIN_MASK_NONE ;
      }
      else if ( 0 == ossStrcmp( maskName, OPT_EXPLAIN_MASK_INPUT_NAME ) )
      {
         mask = OPT_NODE_EXPLAIN_MASK_INPUT ;
      }
      else if ( 0 == ossStrcmp( maskName, OPT_EXPLAIN_MASK_FILTER_NAME ) )
      {
         mask = OPT_NODE_EXPLAIN_MASK_FILTER ;
      }
      else if ( 0 == ossStrcmp( maskName, OPT_EXPLAIN_MASK_OUTPUT_NAME ) )
      {
         mask = OPT_NODE_EXPLAIN_MASK_OUTPUT ;
      }
      else if ( 0 == ossStrcmp( maskName, OPT_EXPLAIN_MASK_ALL_NAME ) )
      {
         mask = ( OPT_NODE_EXPLAIN_MASK_INPUT |
                  OPT_NODE_EXPLAIN_MASK_FILTER |
                  OPT_NODE_EXPLAIN_MASK_OUTPUT ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }

      PD_TRACE_EXITRC( SDB_RTNEXPBASE__PARSEMASK, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__BLDBSONNODEINFO, "_rtnExplainBase::_buildBSONNodeInfo" )
   INT32 _rtnExplainBase::_buildBSONNodeInfo ( BSONObjBuilder & builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__BLDBSONNODEINFO ) ;

      const CHAR* hostName = NULL ;
      stringstream ss ;

      hostName = pmdGetKRCB()->getHostName() ;
      ss << hostName << ":" << pmdGetOptionCB()->getServiceAddr() ;

      builder.append( OPT_FIELD_NODE_NAME, ss.str() ) ;
      builder.append( OPT_FIELD_GROUP_NAME, pmdGetKRCB()->getGroupName() ) ;
      builder.append( OPT_FIELD_ROLE, pmdGetOptionCB()->dbroleStr() ) ;

      PD_TRACE_EXITRC( SDB_RTNEXPBASE__BLDBSONNODEINFO, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPBASE__BLDBSONOPTS, "_rtnExplainBase::_buildBSONQueryOptions" )
   INT32 _rtnExplainBase::_buildBSONQueryOptions ( BSONObjBuilder & builder,
                                                   BOOLEAN needDetail ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPBASE__BLDBSONOPTS ) ;

      builder.append( OPT_FIELD_COLLECTION,
                      NULL != _queryOptions.getCLFullName() ?
                      _queryOptions.getCLFullName() :
                      "" ) ;
      builder.append( OPT_FIELD_QUERY, _queryOptions.getQuery() ) ;

      if ( needDetail )
      {
         builder.append( OPT_FIELD_SORT, _queryOptions.getOrderBy() ) ;
         builder.append( OPT_FIELD_SELECTOR, _queryOptions.getSelector() ) ;
         builder.append( OPT_FIELD_HINT, _queryOptions.getHint() ) ;
         builder.append( OPT_FIELD_SKIP, _queryOptions.getSkip() ) ;
         builder.append( OPT_FIELD_RETURN, _queryOptions.getLimit() ) ;
         builder.append( OPT_FIELD_FLAG, _queryOptions.getFlag() ) ;
      }

      PD_TRACE_EXITRC( SDB_RTNEXPBASE__BLDBSONOPTS, rc ) ;

      return rc ;
   }

   /*
      _rtnContextExplain implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextExplain, RTN_CONTEXT_EXPLAIN, "EXPLAIN" )

   _rtnContextExplain::_rtnContextExplain ( INT64 contextID,
                                            UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _rtnExplainBase( &_explainScanPath ),
     _explainScanPath( &_planAllocator )
   {
   }

   _rtnContextExplain::~_rtnContextExplain ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTEXPLAIN_OPEN, "_rtnContextExplain::open" )
   INT32 _rtnContextExplain::open ( const rtnQueryOptions &options,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCONTEXTEXPLAIN_OPEN ) ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      rc = _openExplain( options, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open explain, rc: %d", rc ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB_RTNCONTEXTEXPLAIN_OPEN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTEXPLAIN__PREPAREDATA, "_rtnContextExplain::_prepareData" )
   INT32 _rtnContextExplain::_prepareData ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCONTEXTEXPLAIN__PREPAREDATA ) ;

      rc = _prepareExplain( this, cb ) ;
      if ( SDB_DMS_EOC != rc &&
           SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare explain, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCONTEXTEXPLAIN__PREPAREDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _rtnContextExplain::_toString ( stringstream & ss )
   {
      ss << ",NeedRun:" << _needRun ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTEXPLAIN__OPENSUBCTX, "_rtnContextExplain::_openSubContext" )
   INT32 _rtnContextExplain::_openSubContext ( rtnQueryOptions & options,
                                               pmdEDUCB * cb,
                                               rtnContext ** ppContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCONTEXTEXPLAIN__OPENSUBCTX ) ;

      SDB_ASSERT( ppContext, "context pointer is invalid" ) ;

      SDB_RTNCB * rtnCB = sdbGetRTNCB() ;
      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      INT64 queryContextID = -1 ;
      rtnContext * queryContext = NULL ;

      rc = rtnQuery( options, cb, dmsCB, rtnCB, queryContextID, &queryContext,
                     FALSE, _needSearch ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query data, rc: %d", rc ) ;

      PD_CHECK( NULL != queryContext, SDB_SYS, error, PDERROR,
                "Failed to get the context of query" ) ;

      _fromLocal = cb->isFromLocal() ;

   done :
      if ( NULL != ppContext )
      {
         ( *ppContext ) = queryContext ;
      }
      PD_TRACE_EXITRC( SDB_RTNCONTEXTEXPLAIN__OPENSUBCTX, rc ) ;
      return rc ;

   error :
      if ( -1 != queryContextID )
      {
         rtnCB->contextDelete( queryContextID, cb ) ;
      }
      queryContext = NULL ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTEXPLAIN__PREPAREEXPPATH, "_rtnContextExplain::_prepareExplainPath" )
   INT32 _rtnContextExplain::_prepareExplainPath ( rtnContext * context,
                                                   pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCONTEXTEXPLAIN__PREPAREEXPPATH ) ;

      SDB_ASSERT( NULL != context, "query context is invalid" ) ;

      SDB_ASSERT( RTN_CONTEXT_SORT == context->getType() ||
                  RTN_CONTEXT_DATA == context->getType() ||
                  RTN_CONTEXT_PARADATA == context->getType(),
                  "sub context is invalid" ) ;

      try
      {
         const optAccessPlanRuntime * planRuntime = NULL ;

         PD_CHECK( NULL != context, SDB_SYS, error, PDERROR,
                   "Failed to explain: data context should not be NULL" ) ;

         planRuntime = context->getPlanRuntime() ;
         PD_CHECK( NULL != planRuntime && planRuntime->hasPlan(),
                   SDB_SYS, error, PDERROR,
                   "Failed to explain: plan should not be NULL" ) ;

         rc = planRuntime->toExplainPath( _explainScanPath, context ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate explain path, "
                      "rc: %d", rc ) ;

         _explainScanPath.setSearchOptions( _needSearch, _needEvaluate ) ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Failed to build explain path, received unexpected "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCONTEXTEXPLAIN__PREPAREEXPPATH, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTEXPLAIN__BLDEXP, "_rtnContextExplain::_buildExplain" )
   INT32 _rtnContextExplain::_buildExplain ( rtnContext * explainContext,
                                             BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCONTEXTEXPLAIN__BLDEXP ) ;

      BSONObjBuilder builder ;

      rc = _buildBSONNodeInfo( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for node information, "
                   "rc: %d", rc ) ;

      if ( _needDetail )
      {
         rc = _buildBSONQueryOptions( builder, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for query options, "
                      "rc: %d", rc ) ;

         rc = _explainScanPath.toBSONExplainInfo( builder, OPT_EXPINFO_MASK_ALL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                      "rc: %d", rc ) ;

         rc = _explainScanPath.toBSON( builder, _needExpand, FALSE,
                                       _explainMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for explain details, "
                      "rc: %d", rc ) ;
      }
      else
      {
         rc = _explainScanPath.toBSONBasic( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for basic explain, "
                      "rc: %d", rc ) ;

         rc = _explainScanPath.toBSONExplainInfo( builder, OPT_EXPINFO_MASK_ALL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                      "rc: %d", rc ) ;
      }

      rc = explainContext->append( builder.obj() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed append explain result to "
                   "context [%lld], rc: %d",
                   explainContext->contextID(), rc ) ;

      hasMore = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB_RTNCONTEXTEXPLAIN__BLDEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnExplainMainBase implement
    */
   _rtnExplainMainBase::_rtnExplainMainBase ( optExplainMergePathBase * explainMergePath )
   : _rtnExplainBase( explainMergePath ),
     _IRtnCtxDataProcessor(),
     _tempTimestamp(),
     _mainExplainOutputted( FALSE ),
     _explainMergeBasePath( explainMergePath )
   {
      SDB_ASSERT( NULL != explainMergePath, "explain merge path is invalid" ) ;
   }

   _rtnExplainMainBase::~_rtnExplainMainBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE_PROCESSDATA, "_rtnExplainMainBase::processData" )
   INT32 _rtnExplainMainBase::processData ( INT64 dataID, const CHAR * data,
                                            INT32 dataSize, INT32 dataNum )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE_PROCESSDATA ) ;

      SDB_ASSERT( NULL != data, "data is invalid" ) ;
      SDB_ASSERT( dataSize >= 0, "data size is invalid" ) ;
      SDB_ASSERT( dataNum >= 0, "data number is invalid" ) ;

      rtnContextBuf contextBuf( data, dataSize, dataNum ) ;
      UINT32 explainNum = 0 ;

      while ( !contextBuf.eof() )
      {
         try
         {
            BSONObj explainResult ;
            ossTickDelta queryTime, waitTime ;
            BOOLEAN needExplain = FALSE ;
            BOOLEAN needParse = FALSE ;

            rc = contextBuf.nextObj( explainResult ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get explain from buffer, "
                         "rc: %d", rc ) ;

            needExplain = _needChildExplain( dataID, explainResult ) ;

            if ( _explainIDSet.end() == _explainIDSet.find( dataID ) )
            {
               ossTick startTimestamp, endTimestamp ;
               endTimestamp.sample() ;

               needParse = _needDetail ;

               rtnExplainTimestampList::iterator endIter =
                                          _endTimestampList.find( dataID ) ;
               if ( _endTimestampList.end() != endIter )
               {
                  endTimestamp = endIter->second ;
               }

               rtnExplainTimestampList::iterator startIter =
                                          _startTimestampList.find( dataID ) ;
               if ( _startTimestampList.end() != startIter &&
                    _needParallelProcess() )
               {
                  startTimestamp = startIter->second ;
               }
               else
               {
                  startTimestamp = _tempTimestamp ;
                  if ( !_needParallelProcess() )
                  {
                     _tempTimestamp.sample() ;
                  }
               }
               queryTime = endTimestamp - startTimestamp ;

               rtnExplainTimestampList::iterator waitIter =
                                          _waitTimestampList.find( dataID ) ;
               if ( _waitTimestampList.end() != waitIter )
               {
                  waitTime = waitIter->second - startTimestamp ;
               }
               else
               {
                  waitTime = endTimestamp - startTimestamp ;
               }

               PD_CHECK( NULL != _explainMergeBasePath, SDB_SYS, error, PDERROR,
                         "Failed to get merge explain path" ) ;

               rc = _explainMergeBasePath->addChildExplain(
                        explainResult, queryTime, waitTime, needParse,
                        needExplain, _explainMask ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add child explain, "
                            "rc: %d", rc ) ;

               _explainIDSet.insert( dataID ) ;
            }
            else
            {
               needParse = FALSE ;

               rc = _explainMergeBasePath->addChildExplain(
                        explainResult, queryTime, waitTime, needParse,
                        needExplain, _explainMask ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add child explain, "
                            "rc: %d", rc ) ;
            }

            explainNum ++ ;
         }
         catch ( std::exception & e )
         {
            PD_LOG( PDERROR, "Failed to process explain data, received "
                    "unexpected error: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      PD_CHECK( explainNum == (UINT32)dataNum, SDB_SYS, error, PDERROR,
                "Failed to process explain data" ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE_PROCESSDATA, rc ) ;
      return SDB_OK ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE_CHKDATA, "_rtnExplainMainBase::checkData" )
   INT32 _rtnExplainMainBase::checkData ( INT64 dataID, const CHAR * data,
                                          INT32 dataSize, INT32 dataNum )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE_CHKDATA ) ;

      if ( _waitTimestampList.end() == _waitTimestampList.find( dataID ) )
      {
         ossTick waitTimestamp ;
         waitTimestamp.sample() ;
         _waitTimestampList[ dataID ] = waitTimestamp ;
      }

      ossTick endTimestamp ;
      endTimestamp.sample() ;
      _endTimestampList[ dataID ] = endTimestamp ;

      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE_CHKDATA, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE_CHKSUBCTX, "_rtnExplainMainBase::checkSubContext" )
   INT32 _rtnExplainMainBase::checkSubContext ( INT64 dataID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE_CHKSUBCTX ) ;

      if ( _startTimestampList.end() == _startTimestampList.find( dataID ) )
      {
         ossTick startTimestamp ;
         startTimestamp.sample() ;
         _startTimestampList[ dataID ] = startTimestamp ;
      }

      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE_CHKSUBCTX, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__REGEXPPROC, "_rtnExplainMainBase::_registerExplainProcessor" )
   INT32 _rtnExplainMainBase::_registerExplainProcessor ( rtnContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__REGEXPPROC ) ;

      rtnContextMain * mainContext = dynamic_cast<rtnContextMain *>( context ) ;

      PD_CHECK( NULL != mainContext, SDB_SYS, error, PDERROR,
                "Failed to get main context" ) ;

      mainContext->registerProcessor( this ) ;

      _tempTimestamp.sample() ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__REGEXPPROC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__UNREGEXPPROC, "_rtnExplainMainBase::_unregisterExplainProcessor" )
   INT32 _rtnExplainMainBase::_unregisterExplainProcessor ( rtnContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__UNREGEXPPROC ) ;

      rtnContextMain * mainContext = dynamic_cast<rtnContextMain *>( context ) ;

      PD_CHECK( NULL != mainContext, SDB_SYS, error, PDERROR,
                "Failed to get main context" ) ;

      mainContext->unregisterProcessor( this ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__UNREGEXPPROC, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__FINISUBCTX, "_rtnExplainMainBase::_finishSubContext" )
   INT32 _rtnExplainMainBase::_finishSubContext ( rtnContext * context,
                                                  pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__FINISUBCTX ) ;

      rtnContextMain * mainContext = dynamic_cast<rtnContextMain *>( context ) ;

      PD_CHECK( NULL != mainContext, SDB_SYS, error, PDERROR,
                "Failed to get main-context" ) ;

      rc = mainContext->reopenForExplain( 0, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reopen main-context [%lld], "
                   "rc: %d", mainContext->contextID(), rc ) ;

      for ( ; ; )
      {
         rtnContextBuf contextBuffer ;
         rc = mainContext->getMore( -1, contextBuffer, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get more from main-context "
                      "[%lld], rc: %d", mainContext->contextID(), rc ) ;
      }

      rc = _unregisterExplainProcessor( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to unregister explain processor, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__FINISUBCTX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__BLDEXP, "_rtnExplainMainBase::_buildExplain" )
   INT32 _rtnExplainMainBase::_buildExplain ( rtnContext * explainContext,
                                              BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__BLDEXP ) ;

      SDB_ASSERT( explainContext, "explain context is invalid" ) ;

      PD_CHECK( NULL != _explainMergeBasePath, SDB_SYS, error, PDERROR,
                "Failed to get merge explain path" ) ;

      hasMore = FALSE ;

      if ( _needDetail && _needFlatten )
      {
         if ( !_mainExplainOutputted )
         {
            rc = _buildMainExplain( explainContext, hasMore ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build main explain, "
                         "rc: %d", rc ) ;
         }

         rc = _buildSubExplains( explainContext, TRUE, hasMore ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build sub explains, "
                      "rc: %d", rc ) ;
      }
      else if ( _needDetail )
      {
         rc = _buildMainExplain( explainContext, hasMore ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build main explain, "
                      "rc: %d", rc ) ;
      }
      else
      {
         rc = _buildSimpleExplain( explainContext, hasMore ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build simple explain, "
                      "rc: %d", rc ) ;
      }

      _mainExplainOutputted = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__BLDEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__BLDMAINEXP, "_rtnExplainMainBase::_buildMainExplain" )
   INT32 _rtnExplainMainBase::_buildMainExplain ( rtnContext * explainContext,
                                                  BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__BLDMAINEXP ) ;

      if ( NULL != _explainMergeBasePath )
      {
         BSONObjBuilder builder ;

         rc = _buildBSONNodeInfo( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for node info, "
                      "rc: %d", rc ) ;

         rc = _buildBSONQueryOptions( builder, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for query options, "
                      "rc: %d", rc ) ;

         rc = _explainMergeBasePath->toBSONExplainInfo( builder,
                                                        _getExplainInfoMask() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for run information, "
                      "rc: %d", rc ) ;

         rc = _explainMergeBasePath->toBSON( builder, _needExpand,
                                             _needFlatten, _explainMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to output BSON from explain path, "
                      "rc: %d", rc ) ;

         rc = explainContext->append( builder.obj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed append explain result to context "
                      "[%lld], rc: %d", explainContext->contextID(), rc ) ;

         hasMore = _needFlatten ;
      }
      else
      {
         PD_CHECK( NULL != _explainMergeBasePath, SDB_SYS, error, PDERROR,
                   "Failed to get merge explain path" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__BLDMAINEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNEXPLAINMAINBASE__BLDSIMPEXP, "_rtnExplainMainBase::_buildSimpleExplain" )
   INT32 _rtnExplainMainBase::_buildSubExplains ( rtnContext * explainContext,
                                                  BOOLEAN needSort,
                                                  BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNEXPLAINMAINBASE__BLDSIMPEXP ) ;

      PD_CHECK( NULL != _explainMergeBasePath, SDB_SYS, error, PDERROR,
                "Failed to get merge explain path" ) ;

      if ( needSort )
      {
         rc = _explainMergeBasePath->sortChildExplains() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort child explains, "
                      "rc: %d", rc ) ;
      }

      {
         optExplainResultList & childExplainList =
                                 _explainMergeBasePath->getChildExplains() ;

         while ( !childExplainList.empty() &&
                 explainContext->buffEndOffset() < RTN_MAX_EXPLAIN_BUFFER_SIZE &&
                 explainContext->buffEndOffset() + DMS_RECORD_MAX_SZ < RTN_RESULTBUFFER_SIZE_MAX )
         {
            rc = explainContext->append( childExplainList.front() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed append explain result to "
                         "context [%lld], rc: %d",
                         explainContext->contextID(), rc ) ;

            childExplainList.pop_front() ;
         }

         hasMore = !childExplainList.empty() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNEXPLAINMAINBASE__BLDSIMPEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }
}
