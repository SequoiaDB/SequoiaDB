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

   INT32 _coordCmdWithLocation::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCtrlParam ctrlParam ;
      SET_RC ignoreRCList ;
      ROUTE_RC_MAP faileds ;
      rtnContextCoord *pContext = NULL ;

      contextID = -1 ;

      _preSet( cb, ctrlParam ) ;

      rc = _preExcute( pMsg, cb, ctrlParam, ignoreRCList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Pre-excute failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = executeOnNodes( pMsg, cb, ctrlParam, _getControlMask(),
                           faileds, _useContext() ? &pContext : NULL,
                           FALSE, &ignoreRCList, NULL ) ;
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
                                                    cb, &faileds ) ) ;
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
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
   }

   INT32 _coordCMDMonIntrBase::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
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
   }

   _coordCMDMonBase::~_coordCMDMonBase()
   {
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

      contextID = -1 ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract command failed, rc: %d", rc ) ;

      rc = coordParseControlParam( queryOption.getQuery(), ctrlParam,
                                   COORD_CTRL_MASK_RAWDATA, NULL, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse control param failed, rc: %d", rc ) ;

      rc = parseUserAggr( queryOption.getHint(), vecUserAggr ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse user define aggr[%s] failed, rc: %d",
                   queryOption.getHint().toString().c_str(), rc ) ;

      if ( ( !ctrlParam._rawData && getInnerAggrContent() ) ||
           vecUserAggr.size() > 0 )
      {
         BSONObj nodeMatcher ;
         BSONObj newMatcher ;
         rc = parseMatcher( queryOption.getQuery(), nodeMatcher, newMatcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse matcher failed, rc: %d", rc ) ;

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

         if ( !newMatcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << newMatcher ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append new matcher failed, rc: %d",
                         rc ) ;
         }

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

         rc = openContext( pOutBuff, buffObjNum, getIntrCMDName(),
                           queryOption.getSelector(), cb, contextID ) ;
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
      if ( !clName.empty() )
      {
         queryOpt.setCLFullName( clName.c_str() ) ;
      }
      queryOpt.clearFlag( FLG_QUERY_WITH_RETURNDATA ) ;

      rc = queryOnCatalog( queryOpt, cb, contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on catalog[%s] failed, rc: %d",
                 queryOpt.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( !outSelector.isEmpty() && -1 != contextID )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnContext *pContext = rtnCB->contextFind( contextID ) ;
         if ( pContext )
         {
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

}

