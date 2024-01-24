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

   Source File Name = coordCommandCommon.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandCommon.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "../util/fromjson.hpp"
#include "rtnQueryOptions.hpp"
#include "coordUtil.hpp"
#include "aggrDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "msgMessage.hpp"
#include "rtn.hpp"
#include "monDump.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCmdWithLocation implement
   */
   _coordCmdWithLocation::_coordCmdWithLocation()
   {
   }

   _coordCmdWithLocation::~_coordCmdWithLocation()
   {
   }

   BOOLEAN _coordCmdWithLocation::isOpenEmptyContext()
   {
      IRtnMonProcessorPtr ptr;
      _getMonProcessor( ptr ) ;
      return ptr.get() ? TRUE : FALSE ;
   }

   BOOLEAN _coordCmdWithLocation::ignoreFailedNodes()
   {
      return TRUE ;
   }

   const CHAR* _coordCmdWithLocation::pushdownCommandName()
   {
      return NULL ;
   }

   INT32 _coordCmdWithLocation::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCtrlParam ctrlParam ;
      SET_RC ignoreRCList ;
      ROUTE_RC_MAP faileds ;
      SET_ROUTEID sucNodes ;
      rtnContextCoord::sharePtr pContext ;
      IRtnMonProcessorPtr monProcessorPtr ;

      contextID = -1 ;

      _preSet( cb, ctrlParam ) ;

      rc = _getMonProcessor( monProcessorPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Get aggr mon-processor failed, rc: %d", rc ) ;

      rc = _preExcute( pMsg, cb, ctrlParam, ignoreRCList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Pre-excute failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = executeOnNodes( pMsg, cb, ctrlParam, _getControlMask(),
                           faileds, _useContext() ? &pContext : NULL,
                           this, &ignoreRCList, &sucNodes ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = _onLocalMode( rc ) ;
         }

         if ( rc )
         {
            if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
            {
               PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
            }
            goto error ;
         }
      }

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }

      rc = _onExecuteOnNodes( pMsg, cb, contextID, faileds, sucNodes, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to do after executing on nodes, rc: %d", rc ) ;
         goto error ;
      }

      if ( -1 != contextID )
      {
         if ( monProcessorPtr.get() )
         {
            rc = _processWithProcessor( pMsg, monProcessorPtr, cb, contextID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Process with processor failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         if ( ignoreFailedNodes() )
         {
            rc = _processFailedNodes( pMsg, cb, contextID, faileds ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Process failed nodes failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

      rc = _posExcute( pMsg, cb, faileds ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Post-excute failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || faileds.size() > 0 ) && -1 == contextID && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &faileds,
                                                    sucNodes.size() ) ) ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCmdWithLocation::_preExcute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            coordCtrlParam &ctrlParam,
                                            SET_RC &ignoreRCList )
   {
      return SDB_OK ;
   }

   INT32 _coordCmdWithLocation::_posExcute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            ROUTE_RC_MAP &faileds )
   {
      return SDB_OK ;
   }

   INT32 _coordCmdWithLocation::_onExecuteOnNodes( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   INT64 &contextID,
                                                   ROUTE_RC_MAP &faileds,
                                                   SET_ROUTEID &sucNodes,
                                                   rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   INT32 _coordCmdWithLocation::_getMonProcessor( IRtnMonProcessorPtr & ptr )
   {
      ptr = IRtnMonProcessorPtr() ;
      return SDB_OK ;
   }

   INT32 _coordCmdWithLocation::_processFailedNodes( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     INT64 &contextID,
                                                     ROUTE_RC_MAP &faileds )
   {
      INT32 rc = SDB_OK ;
      COORD_SHOWERROR_TYPE showError = _getDefaultShowErrorType() ;
      COORD_SHOWERRORMODE_TYPE showErrorMode = _getDefaultShowErrorModeType() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextPtr pContext ;
      const CHAR *pHint = NULL ;

      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj hint( pHint ) ;
         rc = coordParseShowErrorHint( hint, COORD_MASK_SHOWERROR_ALL,
                                       showError, showErrorMode ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Parse show error parameter failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      if ( COORD_SHOWERROR_ONLY == showError )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;

         rtnContextDump::sharePtr pDumpContext ;

         /// create new context
         rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, pDumpContext, contextID, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = pDumpContext->open( BSONObj(), BSONObj(), -1, 0 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
            goto error ;
         }
         pContext = pDumpContext ;
      }
      else
      {
         rc = rtnCB->contextFind( contextID, pContext, cb ) ;
         if ( !pContext )
         {
            PD_LOG( PDERROR, "Context(%lld) is not found", contextID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( COORD_SHOWERROR_IGNORE != showError )
      {
         rc = _buildFailedNodeReply( faileds, pContext, showErrorMode ) ;
         PD_RC_CHECK( rc, PDERROR, "Build failed node reply failed, rc: %d",
                     rc ) ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCmdWithLocation::_processWithProcessor( MsgHeader *pMsg,
                                                       IRtnMonProcessorPtr processorPtr,
                                                       pmdEDUCB *cb,
                                                       INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextDump::sharePtr pContext ;
      INT64 newContextID = -1 ;
      rtnQueryOptions queryOption ;
      monDataSetFetch *dsFetch = NULL ;
      BSONObj nodesMatcher, newMatcher ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnParseCmdLocationMatcher( queryOption.getQuery(), nodesMatcher, newMatcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse cmd location matcher, rc: %d", rc ) ;

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, pContext,
                              newContextID, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pContext->open( queryOption.getSelector(),
                           newMatcher,
                           queryOption.getLimit(),
                           queryOption.getSkip() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open dump context failed, rc: %d", rc ) ;
         goto error ;
      }

      dsFetch = (monDataSetFetch *)( getRtnFetchBuilder()->create(
                                     RTN_FETCH_DATASET ) ) ;
      PD_CHECK( NULL != dsFetch, SDB_OOM, error, PDERROR,
                "Failed to allocate fetcher for context" ) ;

      rc = dsFetch->attachContext( contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to attach context, rc: %d", rc ) ;
      contextID = newContextID ;
      newContextID = -1 ;

      pContext->setMonFetch( dsFetch, TRUE ) ;
      dsFetch = NULL ;
      pContext->setMonProcessor( processorPtr ) ;

      if ( !queryOption.getOrderBy().isEmpty() )
      {
         rc = rtnSort( pContext, queryOption.getOrderBy(), cb, queryOption.getSkip(),
                       queryOption.getLimit(), contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      if ( -1 != newContextID )
      {
         rtnCB->contextDelete ( newContextID, cb ) ;
         newContextID = -1 ;
      }
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      SAFE_OSS_DELETE( dsFetch ) ;
      goto done ;
   }

   INT32 _coordCmdWithLocation::_getCSGrps ( const CHAR * collectionSpace,
                                             pmdEDUCB * cb,
                                             coordCtrlParam & ctrlParam )
   {
      INT32 rc = SDB_OK ;

      CHAR * catMessage = NULL ;
      INT32 catMessageSize = 0 ;

      CoordGroupList grpLst ;
      rtnQueryOptions queryOpt ;

      PD_CHECK( !dmsIsSysCSName( collectionSpace ), SDB_INVALIDARG, error,
                PDERROR, "Could not analyze SYS collection space [%s]",
                collectionSpace ) ;

      queryOpt.setCLFullName( "CAT" ) ;
      queryOpt.setQuery( BSON( CAT_COLLECTION_SPACE_NAME <<
                               collectionSpace ) ) ;
      rc = queryOpt.toQueryMsg( &catMessage, catMessageSize, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Alloc query msg failed, rc: %d", rc ) ;

      /// change the opCode
      ((MsgHeader*)catMessage)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
      rc = executeOnCataGroup( (MsgHeader*)catMessage, cb, &grpLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collectionspace[%s] info from catalog "
                   "failed, rc: %d", collectionSpace, rc ) ;

      ctrlParam._useSpecialGrp = TRUE ;
      ctrlParam._specialGrps = grpLst ;

   done :
      if ( catMessage )
      {
         msgReleaseBuffer( catMessage, cb ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCmdWithLocation::_getCLGrps ( MsgHeader * message,
                                             const CHAR * collection,
                                             pmdEDUCB * cb,
                                             coordCtrlParam & ctrlParam )
   {
      INT32 rc = SDB_OK ;

      coordCataSel cataSel ;
      CoordGroupList grpLst, exceptLst ;
      MsgOpQuery *request = (MsgOpQuery *)message ;

      CHAR collectionSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

      rc = rtnResolveCollectionName( collection,
                                     ossStrlen( collection ),
                                     collectionSpace,
                                     DMS_COLLECTION_SPACE_NAME_SZ,
                                     clShortName,
                                     DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name [%s], "
                    "rc: %d", collection, rc ) ;

      PD_CHECK( !dmsIsSysCSName( collectionSpace ), SDB_INVALIDARG, error,
                PDERROR, "Could not analyze SYS collection space [%s]",
                collectionSpace ) ;

      PD_CHECK( !dmsIsSysCLName( clShortName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection [%s]", clShortName ) ;

      rc = cataSel.bind( _pResource, collection, cb, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update collection[%s]'s catalog info failed, "
                   "rc: %d", collection, rc ) ;


      rc = cataSel.getGroupLst( cb, exceptLst, grpLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s]'s group list failed, "
                   "rc: %d", collection, rc ) ;

      // Set the version for verify in data-groups
      request->version = cataSel.getCataPtr()->getVersion() ;

      ctrlParam._useSpecialGrp = TRUE ;
      ctrlParam._specialGrps = grpLst ;

   done :
      return rc ;

   error :
      goto done ;
   }

   /*
      _coordCMDMonIntrBase implement
   */
   _coordCMDMonIntrBase::_coordCMDMonIntrBase()
   {
   }

   _coordCMDMonIntrBase::~_coordCMDMonIntrBase()
   {
   }

   void _coordCMDMonIntrBase::_preSet( pmdEDUCB *cb,
                                       coordCtrlParam &ctrlParam )
   {
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   INT32 _coordCMDMonIntrBase::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   INT32 _coordCMDMonIntrBase::_preExcute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           coordCtrlParam &ctrlParam,
                                           SET_RC &ignoreRCList )
   {
      // Return data during QUERY to reduce GETMORE operation.
      ((MsgOpQuery*)pMsg)->flags |= FLG_QUERY_WITH_RETURNDATA ;
      return SDB_OK ;
   }

   /*
      _coordCMDMonCurIntrBase implement
   */
   _coordCMDMonCurIntrBase::_coordCMDMonCurIntrBase()
   {
   }

   _coordCMDMonCurIntrBase::~_coordCMDMonCurIntrBase()
   {
   }

   void _coordCMDMonCurIntrBase::_preSet( pmdEDUCB *cb,
                                          coordCtrlParam &ctrlParam )
   {
      SET_NODEID tmpNodes ;
      MsgRouteID nodeID ;
      pmdRemoteSessionSite *pSite = NULL ;

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;

      ctrlParam._useSpecialNode = TRUE ;
      if ( pSite )
      {
         pSite->getAllNodeID( tmpNodes ) ;
      }

      SET_NODEID::iterator it = tmpNodes.begin() ;
      while( it != tmpNodes.end() )
      {
         nodeID.value = *it ;
         ++it ;
         if ( MSG_ROUTE_SHARD_SERVCIE == nodeID.columns.serviceID )
         {
            ctrlParam._specialNodes.insert( nodeID.value ) ;
         }
      }
   }

   /*
      _coordAggrCmdBase implement
   */
   _coordAggrCmdBase::_coordAggrCmdBase()
   {
   }

   _coordAggrCmdBase::~_coordAggrCmdBase()
   {
   }

   INT32 _coordAggrCmdBase::appendObjs( const CHAR *pInputBuffer,
                                        CHAR *&pOutputBuffer,
                                        INT32 &bufferSize,
                                        INT32 &bufUsed,
                                        INT32 &buffObjNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pEnd = pInputBuffer ;
      string strline ;
      BSONObj obj ;

      while ( *pEnd != '\0' )
      {
         strline.clear() ;
         while( *pEnd && *pEnd != '\r' && *pEnd != '\n' )
         {
            strline += *pEnd ;
            ++pEnd ;
         }

         if ( strline.empty() )
         {
            ++pEnd ;
            continue ;
         }

         rc = fromjson( strline, obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse string[%s] to json failed, rc: %d",
                      strline.c_str(), rc ) ;

         rc = appendObj( obj, pOutputBuffer, bufferSize,
                         bufUsed, buffObjNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Append obj failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDMonBase implement
   */
   _coordCMDMonBase::_coordCMDMonBase()
   {
      _showError = _getDefaultShowErrorType() ;
      _showErrorMode = _getDefaultShowErrorModeType() ;
   }

   _coordCMDMonBase::~_coordCMDMonBase()
   {
   }

   INT32 _coordCMDMonBase::_getAggrMonProcessor( IRtnMonProcessorPtr &ptr )
   {
      ptr = IRtnMonProcessorPtr() ;
      return SDB_OK ;
   }

   INT32 _coordCMDMonBase::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = NULL ;
      coordOperator *pOperator = NULL ;
      CHAR *pOutBuff = NULL ;
      INT32 buffSize = 0 ;
      INT32 buffUsedSize = 0 ;
      INT32 buffObjNum = 0 ;

      rtnQueryOptions queryOption ;
      coordCtrlParam ctrlParam ;
      vector< BSONObj > vecUserAggr ;
      BSONObj newHint ;
      IRtnMonProcessorPtr monPtr ;

      contextID = -1 ;

      rc = _getAggrMonProcessor( monPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Get aggr mon-processor failed, rc: %d", rc ) ;

      // Return data during QUERY to reduce GETMORE operation.
      ((MsgOpQuery*)pMsg)->flags |= FLG_QUERY_WITH_RETURNDATA ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract command failed, rc: %d", rc ) ;

      rc = coordParseControlParam( queryOption.getQuery(), ctrlParam,
                                   COORD_CTRL_MASK_RAWDATA, NULL, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse control param failed, rc: %d", rc ) ;

      rc = parseUserAggr( queryOption.getHint(), vecUserAggr, newHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse user define aggr[%s] failed, rc: %d",
                   queryOption.getHint().toString().c_str(), rc ) ;

      rc = _handleHints( newHint, _getShowErrorMask() ) ;
      PD_RC_CHECK( rc, PDERROR, "Handle hints failed, rc: %d", rc ) ;

      if ( ( !ctrlParam._rawData && getInnerAggrContent() ) ||
           vecUserAggr.size() > 0 )
      {
         /// add aggr operators
         BSONObj nodeMatcher ;
         BSONObj newMatcher ;
         rc = rtnParseCmdLocationMatcher( queryOption.getQuery(), nodeMatcher, newMatcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse matcher failed, rc: %d", rc ) ;

         /// add nodes matcher to the botton
         if ( !nodeMatcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << nodeMatcher ),
                            pOutBuff, buffSize, buffUsedSize,
                            buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append node matcher failed, rc: %d",
                         rc ) ;
         }

         if ( !ctrlParam._rawData )
         {
            rc = appendObjs( getInnerAggrContent(), pOutBuff, buffSize,
                             buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append objs[%s] failed, rc: %d",
                         getInnerAggrContent(), rc ) ;
         }

         /// add new matcher
         if ( !newMatcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << newMatcher ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append new matcher failed, rc: %d",
                         rc ) ;
         }

         /// order by
         if ( !queryOption.isOrderByEmpty() )
         {
            rc = appendObj( BSON( AGGR_SORT_PARSER_NAME <<
                                  queryOption.getOrderBy() ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append order by failed, rc: %d",
                         rc ) ;
         }

         for ( UINT32 i = 0 ; i < vecUserAggr.size() ; ++i )
         {
            rc = appendObj( vecUserAggr[ i ], pOutBuff, buffSize,
                            buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append user define aggr[%s] failed, "
                         "rc: %d", vecUserAggr[ i ].toString().c_str(),
                         rc ) ;
         }

         /// offset
         if ( queryOption.getSkip() > 0 )
         {
            rc = appendObj( BSON( AGGR_SKIP_PARSER_NAME <<
                                  queryOption.getSkip() ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append skip failed, rc: %d", rc ) ;
         }

         /// limit
         if ( queryOption.getLimit() != -1 )
         {
            rc = appendObj( BSON( AGGR_LIMIT_PARSER_NAME <<
                                  queryOption.getLimit() ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append limit failed, rc: %d", rc ) ;
         }

         /// open context
         rc = openContext( pOutBuff, buffObjNum, getIntrCMDName(),
                           queryOption.getSelector(),
                           newHint,
                           0, -1,
                           cb, contextID,
                           monPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;
      }
      else
      {
         pFactory = coordGetFactory() ;
         rc = pFactory->create( getIntrCMDName() + 1, pOperator ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                    getIntrCMDName(), rc ) ;
            goto error ;
         }
         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOperator->getName(), rc ) ;
            goto error ;
         }
         rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pOutBuff )
      {
         SDB_OSS_FREE( pOutBuff ) ;
         pOutBuff = NULL ;
         buffSize = 0 ;
         buffUsedSize = 0 ;
      }
      if ( -1 != contextID && !_useContext() )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done;
   }

   INT32 _coordCMDMonBase::_handleHints( BSONObj &hint, UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      rc = coordParseShowErrorHint( hint, mask, _showError, _showErrorMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse show error hint, rc: %d" ) ;

      try
      {
         // rebuild hint according to the mask
         BSONObjBuilder builder ;
         BSONObjIterator itr2 ( hint ) ;
         while ( itr2.more() )
         {
            BSONElement elem = itr2.next() ;
            if ( 0 == ossStrcasecmp( elem.fieldName(), "$Options" ) )
            {
               BSONObjBuilder sub( builder.subobjStart( "$Options" ) ) ;
               BSONObj tmp( elem.value() );
               BSONObjIterator itr ( tmp ) ;
               BSONElement elem2 ;
               while ( itr.more() )
               {
                  elem2 = itr.next() ;
                  if ( 0 != ossStrcasecmp( elem2.fieldName(),
                                           COORD_SHOWERROR ) &&
                       0 != ossStrcasecmp( elem2.fieldName(),
                                           COORD_SHOWERRORMODE ) )
                  {
                     sub.append( elem2 ) ;
                  }
               }

               switch ( _getShowErrorType() )
               {
                  case COORD_SHOWERROR_SHOW :
                     if ( COORD_MASK_SHOWERROR_SHOW & mask )
                     {
                        sub.append( COORD_SHOWERROR,
                                    COORD_SHOWERROR_VALUE_SHOW ) ;
                     }
                     break ;
                  case COORD_SHOWERROR_ONLY :
                     if ( COORD_MASK_SHOWERROR_ONLY & mask )
                     {
                        sub.append( COORD_SHOWERROR,
                                    COORD_SHOWERROR_VALUE_ONLY ) ;
                     }
                     break ;
                  case COORD_SHOWERROR_IGNORE :
                     if ( COORD_MASK_SHOWERROR_IGNORE & mask )
                     {
                        sub.append( COORD_SHOWERROR,
                                    COORD_SHOWERROR_VALUE_IGNORE ) ;
                     }
                     break ;
                  default :
                     break ;
               }

               switch ( _getShowErrorModeType() )
               {
                  case COORD_SHOWERRORMODE_AGGR :
                     if ( COORD_MASK_SHOWERRORMODE_AGGR & mask )
                     {
                        sub.append( COORD_SHOWERRORMODE,
                                    COORD_SHOWERRORMODE_VALUE_AGGR ) ;
                     }
                     break ;
                  case COORD_SHOWERRORMODE_FLAT :
                     if ( COORD_MASK_SHOWERRORMODE_FLAT & mask )
                     {
                        sub.append( COORD_SHOWERRORMODE,
                                    COORD_SHOWERRORMODE_VALUE_FLAT ) ;
                     }
                     break ;
                  default :
                     break ;
               }
               sub.done() ;
            }
            else
            {
               builder.append( elem ) ;
            }
         }

         hint = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to change hint, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   /*
      _coordCMDQueryBase implement
   */
   _coordCMDQueryBase::_coordCMDQueryBase()
   {
   }

   _coordCMDQueryBase::~_coordCMDQueryBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDQUERYBASE_EXE, "_coordCMDQueryBase::execute" )
   INT32 _coordCMDQueryBase::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMDQUERYBASE_EXE ) ;

      contextID                        = -1 ;
      string clName ;
      BSONObj outSelector ;
      rtnQueryOptions queryOpt ;
      BSONElement ele ;

      // parse msg
      rc = queryOpt.fromQueryMsg( (CHAR*)pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract query message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _preProcess( queryOpt, clName, outSelector ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "PreProcess[%s] failed, rc: %d",
                 queryOpt.toString().c_str(), rc ) ;
         goto error ;
      }

      ele = queryOpt._query.getField( FIELD_NAME_NAME ) ;
      if ( String == ele.type() &&
           0 == ossStrncmp( ele.valuestr(),
                            CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         rc = _processQueryVCS( queryOpt, ele.valuestr(),
                                cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Process query VCS failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( !clName.empty() )
         {
            queryOpt.setCLFullName( clName.c_str() ) ;
         }
         queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;

         // query on catalog
         rc = queryOnCatalog( queryOpt, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query on catalog[%s] failed, rc: %d",
                    queryOpt.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( !outSelector.isEmpty() && -1 != contextID )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnContextPtr pContext ;
         if ( SDB_OK == rtnCB->contextFind( contextID, pContext ) )
         {
            /// re-load pattern
            pContext->getSelector().clear() ;
            pContext->getSelector().loadPattern( outSelector ) ;
         }
      }

   done:
      PD_TRACE_EXITRC ( COORD_CMDQUERYBASE_EXE, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDQueryBase::_processQueryVCS( rtnQueryOptions &queryOpt,
                                               const CHAR *pName,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnContextCoord::sharePtr pContext ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_COORD,
                              pContext,
                              contextID,
                              cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pContext->open( queryOpt, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _processVCS( queryOpt, pName, pContext ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Process VCS failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDQueryBase::_processVCS( rtnQueryOptions &queryOpt,
                                          const CHAR *pName,
                                          rtnContext *pContext )
   {
      return SDB_INVALIDARG ;
   }

}

