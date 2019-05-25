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

   Source File Name = coordCommandBase.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCommandBase.hpp"
#include "coordUtil.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "coordQueryOperator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordCommandBase impelement
   */
   _coordCommandBase::_coordCommandBase()
   {
   }

   _coordCommandBase::~_coordCommandBase()
   {
   }

   INT32 _coordCommandBase::_processSucReply( ROUTE_REPLY_MAP &okReply,
                                              rtnContextCoord *pContext )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      BOOLEAN takeOver = FALSE ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;
      ROUTE_REPLY_MAP::iterator it = okReply.begin() ;
      while( it != okReply.end() )
      {
         takeOver = FALSE ;
         pReply = (MsgOpReply*)(it->second) ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( pContext )
            {
               rcTmp = pContext->addSubContext( pReply, takeOver ) ;
               if ( rcTmp )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          pContext->toString().c_str(), rcTmp ) ;
                  rc = rcTmp ;
               }
            }
            else
            {
               SDB_ASSERT( pReply->contextID == -1, "Context leak" ) ;
            }
         }
         else
         {
            PD_LOG( PDWARNING,
                    "Error reply flag in data[node: %s, opCode: %d], rc: %d",
                    routeID2String( nodeID ).c_str(),
                    pReply->header.opCode, pReply->flags ) ;
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
         }
         ++it ;
      }
      okReply.clear() ;

      return rc ;
   }

   INT32 _coordCommandBase::_processNodesReply( pmdRemoteSession *pSession,
                                                ROUTE_RC_MAP &faileds,
                                                rtnContextCoord *pContext,
                                                SET_RC *pIgnoreRC,
                                                SET_ROUTEID *pSucNodes )
   {
      INT32 rc = SDB_OK ;

      MsgRouteID nodeID ;
      MsgOpReply *pReply = NULL ;
      BOOLEAN takeOver = FALSE ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;

      while( itr.more() )
      {
         pSub = itr.next() ;
         pReply = ( MsgOpReply *)pSub->getRspMsg( TRUE ) ;

         takeOver = FALSE ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( pSucNodes )
            {
               pSucNodes->insert( nodeID.value ) ;
            }

            if ( pContext )
            {
               rc = pContext->addSubContext( pReply, takeOver ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          pContext->toString().c_str(), rc ) ;
               }
            }
            else
            {
               SDB_ASSERT( pReply->contextID == -1, "Context leak" ) ;
            }
         }
         else if ( pIgnoreRC && pIgnoreRC->end() !=
                   pIgnoreRC->find( pReply->flags ) )
         {
         }
         else
         {
            coordErrorInfo errInfo( pReply ) ;
            PD_LOG( ( pContext ? PDINFO : PDWARNING ),
                    "Failed to process reply[node: %s, flag: %d, obj: %s]",
                    routeID2String( nodeID ).c_str(), pReply->flags,
                    errInfo._obj.toString().c_str() ) ;
            faileds[ nodeID.value ] = errInfo ;
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
            pReply = NULL ;
         }
      }

      return rc ;
   }

   INT32 _coordCommandBase::_buildFailedNodeReply( ROUTE_RC_MAP &failedNodes,
                                                   rtnContextCoord *pContext )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( pContext != NULL, "pContext can't be NULL!" ) ;

      if ( failedNodes.size() > 0 )
      {
         BSONObjBuilder builder ;
         coordBuildFailedNodeReply( _pResource, failedNodes, builder ) ;

         rc = pContext->append( builder.obj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append obj, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCommandBase::_executeOnGroups( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              const CoordGroupList &groupLst,
                                              MSG_ROUTE_SERVICE_TYPE type,
                                              BOOLEAN onPrimary,
                                              SET_RC *pIgnoreRC,
                                              CoordGroupList *pSucGrpLst,
                                              rtnContextCoord **ppContext,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      pmdKRCB *pKrcb                   = pmdGetKRCB() ;
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB() ;

      coordSendMsgIn inMsg( pMsg ) ;
      coordSendOptions sendOpt ;
      coordProcessResult result ;
      rtnContextCoord *pTmpContext = NULL ;
      INT64 contextID = -1 ;

      BOOLEAN preRead = _flagCoordCtxPreRead() ;

      sendOpt._groupLst = groupLst ;
      sendOpt._svcType = type ;
      sendOpt._primary = onPrimary ;
      sendOpt._pIgnoreRC = pIgnoreRC ;

      ROUTE_REPLY_MAP okReply ;
      ROUTE_REPLY_MAP nokReply ;
      result._pOkReply = &okReply ;
      result._pNokReply = &nokReply ;

      ROUTE_RC_MAP nokRC ;

      if ( ppContext )
      {
         if ( NULL == *ppContext )
         {
            rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                                     (rtnContext **)ppContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*ppContext)->contextID() ;
         }
         pTmpContext = *ppContext ;

         if ( !pTmpContext->isOpened() )
         {
            rtnQueryOptions defaultOptions ;
            rc = pTmpContext->open( defaultOptions, preRead ) ;
            PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
         }
      }

      rc = doOnGroups( inMsg, sendOpt, cb, result ) ;
      rcTmp = _processSucReply( okReply, pTmpContext ) ;
      if ( nokReply.size() > 0 )
      {
         ROUTE_REPLY_MAP::iterator itNok = nokReply.begin() ;
         while( itNok != nokReply.end() )
         {
            MsgOpReply *pReply = (MsgOpReply*)itNok->second ;
            nokRC[ itNok->first ] = coordErrorInfo( pReply ) ;
            SDB_OSS_FREE( pReply ) ;
            ++itNok ;
         }
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Do command[%d] on groups failed, rc: %d",
                 pMsg->opCode, rc ) ;
         goto error ;
      }
      else if ( rcTmp )
      {
         rc = rcTmp ;
         goto error ;
      }

      if ( preRead && pTmpContext )
      {
         pTmpContext->addSubDone( cb ) ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID  )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
         *ppContext = NULL ;
      }
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                   cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 _coordCommandBase::executeOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 const CoordGroupList &groupLst,
                                                 BOOLEAN onPrimary,
                                                 SET_RC *pIgnoreRC,
                                                 CoordGroupList *pSucGrpLst,
                                                 rtnContextCoord **ppContext,
                                                 rtnContextBuf *buf )
   {
      return _executeOnGroups( pMsg, cb, groupLst, MSG_ROUTE_SHARD_SERVCIE,
                               onPrimary, pIgnoreRC, pSucGrpLst, ppContext,
                               buf ) ;
   }

   INT32 _coordCommandBase::executeOnCataGroup( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                BOOLEAN onPrimary,
                                                SET_RC *pIgnoreRC,
                                                rtnContextCoord **ppContext,
                                                rtnContextBuf *buf )
   {
      CoordGroupList grpList ;
      grpList[ CATALOG_GROUPID ] = CATALOG_GROUPID ;
      return _executeOnGroups( pMsg, cb, grpList, MSG_ROUTE_CAT_SERVICE,
                               onPrimary, pIgnoreRC, NULL, ppContext,
                               buf ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDBASE_EXEONCATA, "_coordCommandBase::executeOnCataGroup" )
   INT32 _coordCommandBase::executeOnCataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 CoordGroupList *pGroupList,
                                                 vector<BSONObj> *pReplyObjs,
                                                 BOOLEAN onPrimary,
                                                 SET_RC *pIgnoreRC,
                                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMDBASE_EXEONCATA ) ;

      rtnContextBuf buffObj ;
      rtnContextCoord *pContext = NULL ;

      rc = executeOnCataGroup( pMsg, cb, onPrimary, pIgnoreRC,
                               &pContext, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command[%d] on "
                   "catalog, rc: %d", pMsg->opCode, rc ) ;

      while ( pContext )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get more from context [%lld], rc: %d",
                    pContext->contextID(), rc ) ;
            goto error ;
         }

         try
         {
            BSONObj obj( buffObj.data() ) ;

            if ( pGroupList )
            {
               rc = coordGetGroupsFromObj( obj, *pGroupList ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get groups from catalog "
                            "reply [%s], rc: %d", obj.toString().c_str(),
                            rc ) ;
            }

            if ( pReplyObjs )
            {
               pReplyObjs->push_back( obj.getOwned() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Extrace catalog reply obj occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      PD_TRACE_EXITRC ( COORD_CMDBASE_EXEONCATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _coordCommandBase::executeOnCataCL( MsgOpQuery *pMsg,
                                             pmdEDUCB *cb,
                                             const CHAR *pCLName,
                                             BOOLEAN onPrimary,
                                             SET_RC *pIgnoreRC,
                                             rtnContextCoord **ppContext,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCataSel cataSel ;

      rc = cataSel.bind( _pResource, pCLName, cb, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", pCLName, rc ) ;
         goto error ;
      }

   retry:
      pMsg->version = cataSel.getCataPtr()->getVersion() ;

      rc = executeOnCataGroup( (MsgHeader*)pMsg, cb, onPrimary, pIgnoreRC,
                               ppContext, buf ) ;
      if ( rc )
      {
         if ( checkRetryForCLOpr( rc, NULL, cataSel, &(pMsg->header),
                                  cb, rc, NULL, TRUE ) )
         {
            _groupSession.getGroupCtrl()->incRetry() ;
            goto retry ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCommandBase::executeOnCL( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         const CHAR *pCLName,
                                         BOOLEAN firstUpdateCata,
                                         const CoordGroupList *pSpecGrpLst,
                                         SET_RC *pIgnoreRC,
                                         CoordGroupList *pSucGrpLst,
                                         rtnContextCoord **ppContext,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordQueryOperator queryOpr( isReadOnly() ) ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;

      queryConf._allCataGroups = TRUE ;
      if ( pCLName )
      {
         queryConf._realCLName = pCLName ;
      }
      queryConf._updateAndGetCata = firstUpdateCata ;
      queryConf._openEmptyContext = TRUE ;

      sendOpt._primary = TRUE ;
      sendOpt._pIgnoreRC = pIgnoreRC ;
      if ( pSpecGrpLst )
      {
         sendOpt._groupLst = *pSpecGrpLst ;
         sendOpt._useSpecialGrp = TRUE ;
      }

      rc = queryOpr.init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init query operator failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( !pSucGrpLst )
      {
         rc = queryOpr.queryOrDoOnCL( pMsg, cb, ppContext,
                                      sendOpt, &queryConf, buf ) ;
      }
      else
      {
         rc = queryOpr.queryOrDoOnCL( pMsg, cb, ppContext,
                                      sendOpt, *pSucGrpLst, &queryConf,
                                      buf ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCommandBase::queryOnCL( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       const CHAR *pCLName,
                                       rtnContextCoord **ppContext,
                                       BOOLEAN onPrimary,
                                       const CoordGroupList *pSpecGrpLst,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordQueryOperator queryOpr( FALSE ) ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;

      if ( pCLName )
      {
         queryConf._realCLName = pCLName ;
      }

      sendOpt._primary = onPrimary ;
      if ( pSpecGrpLst )
      {
         sendOpt._groupLst = *pSpecGrpLst ;
         sendOpt._useSpecialGrp = TRUE ;
      }

      rc = queryOpr.init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init query operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = queryOpr.queryOrDoOnCL( pMsg, cb, ppContext,
                                   sendOpt, &queryConf, buf ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDBASE_QUERYONCATA, "_coordCommandBase::queryOnCatalog" )
   INT32 _coordCommandBase::queryOnCatalog( MsgHeader *pMsg,
                                            INT32 requestType,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDBASE_QUERYONCATA ) ;
      rtnContextCoord *pContext        = NULL ;

      contextID = -1 ;

      pMsg->opCode                     = requestType ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, &pContext, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Query[%d] on catalog group failed, rc = %d",
                  requestType, rc ) ;
         goto error ;
      }

   done :
      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }
      PD_TRACE_EXITRC ( COORD_CMDBASE_QUERYONCATA, rc ) ;
      return rc ;
   error :
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDBASE_QUERYONCATA2, "_coordCommandBase::queryOnCatalog" )
   INT32 _coordCommandBase::queryOnCatalog( const rtnQueryOptions &options,
                                            pmdEDUCB *cb,
                                            SINT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_CMDBASE_QUERYONCATA2 ) ;

      CHAR *msgBuf = NULL ;
      INT32 msgBufLen = 0 ;
      contextID = -1 ;

      rc = options.toQueryMsg( &msgBuf, msgBufLen, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build query msg:%d", rc ) ;
         goto error ;
      }

      rc = queryOnCatalog( (MsgHeader*)msgBuf, MSG_BS_QUERY_REQ, cb,
                           contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on catalog group failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != msgBuf )
      {
         msgReleaseBuffer( msgBuf, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_CMDBASE_QUERYONCATA2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDBASE_QUERYONCATA3, "_coordCommandBase::queryOnCataAndPushToVec" )
   INT32 _coordCommandBase::queryOnCataAndPushToVec( const rtnQueryOptions &options,
                                                     pmdEDUCB *cb,
                                                     vector< BSONObj > &objs,
                                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_CMDBASE_QUERYONCATA3 ) ;
      SINT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = queryOnCatalog( options, cb, contextID, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to query on catalog:%d", rc ) ;
         goto error ;
      }

      do
      {
         rc = rtnGetMore( contextID, -1, bufObj, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            contextID = -1 ;
            PD_LOG( PDERROR, "Failed to getmore from context, rc: %d", rc ) ;
            goto error ;
         }
         else
         {
            while ( !bufObj.eof() )
            {
               BSONObj obj ;
               rc = bufObj.nextObj( obj ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to get obj from obj buf, rc: %d",
                          rc ) ;
                  goto error ;
               }

               objs.push_back( obj.getOwned() ) ;
            }
         }
      } while( TRUE ) ;

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_CMDBASE_QUERYONCATA3, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordCommandBase::_printDebug ( const CHAR *pReceiveBuffer,
                                         const CHAR *pFuncName )
   {
   #if defined (_DEBUG)
      PD_LOG( PDDEBUG, "%s: %s", pFuncName,
              msg2String( (const MsgHeader*)pReceiveBuffer,
                          MSG_MASK_ALL,
                          MSG_MASK_ALL ).c_str() ) ;
   #endif // _DEBUG
   }

   INT32 _coordCommandBase::executeOnNodes( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            SET_ROUTEID &nodes,
                                            ROUTE_RC_MAP &faileds,
                                            SET_ROUTEID *pSucNodes,
                                            SET_RC *pIgnoreRC,
                                            rtnContextCoord *pContext )
   {
      INT32 rc                      = SDB_OK ;
      INT32 rcTmp                   = SDB_OK ;
      pmdRemoteSession *pRemote     = _groupSession.getSession() ;
      pmdSubSession *pSub           = NULL ;
      SET_ROUTEID::iterator it ;

      _groupSession.clear() ;

      it = nodes.begin() ;
      while( it != nodes.end() )
      {
         pSub = pRemote->addSubSession( *it ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pRemote->sendMsg( pSub ) ;
         if ( rcTmp )
         {
            faileds[ *it ] = rcTmp ;
            pRemote->delSubSession( *it ) ;
         }
         ++it ;
      }

      rcTmp = pRemote->waitReply1( TRUE ) ;
      rc = rc ? rc : rcTmp ;

      rcTmp = _processNodesReply( pRemote, faileds, pContext,
                                  pIgnoreRC, pSucNodes ) ;
      rc = rc ? rc : rcTmp ;

      _groupSession.clear() ;

      return rc ;
   }

   INT32 _coordCommandBase::executeOnNodes( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            coordCtrlParam &ctrlParam,
                                            UINT32 mask,
                                            ROUTE_RC_MAP &faileds,
                                            rtnContextCoord **ppContext,
                                            BOOLEAN openEmptyContext,
                                            SET_RC *pIgnoreRC,
                                            SET_ROUTEID *pSucNodes )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;
      SDB_RTNCB *pRtncb = pKrcb->getRTNCB() ;

      rtnQueryOptions queryOption ;
      const CHAR *pSrcFilterObjData = NULL ;
      BSONObj *pFilterObj = NULL ;
      BOOLEAN hasNodeOrGroupFilter = FALSE ;

      CoordGroupList allGroupLst ;
      CoordGroupList expectGrpLst ;
      CoordGroupList groupLst ;
      SET_ROUTEID sendNodes ;
      BSONObj newFilterObj ;
      BOOLEAN hasParseRetry = FALSE ;

      CHAR *pNewMsg = NULL ;
      INT32 newMsgSize = 0 ;
      INT64 contextID = -1 ;
      rtnContextCoord *pTmpContext = NULL ;
      BOOLEAN needReset = FALSE ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extrace query msg failed, rc: %d", rc ) ;
      pFilterObj = coordGetFilterByID( ctrlParam._filterID, queryOption ) ;
      pSrcFilterObjData = pFilterObj->objdata() ;
      rc = coordParseControlParam( *pFilterObj, ctrlParam, mask,
                                   &newFilterObj, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "prase control param failed, rc: %d", rc ) ;
      *pFilterObj = newFilterObj ;

      rc = _pResource->updateGroupList( allGroupLst, cb, NULL,
                                        FALSE, FALSE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d",
                   rc ) ;
      {
         CoordGroupList::iterator itGrp = allGroupLst.begin() ;
         while( itGrp != allGroupLst.end() )
         {
            if ( ( !ctrlParam._role[ SDB_ROLE_DATA ] &&
                   itGrp->second >= DATA_GROUP_ID_BEGIN &&
                   itGrp->second <= DATA_GROUP_ID_END ) ||
                 ( !ctrlParam._role[ SDB_ROLE_CATALOG ] &&
                   CATALOG_GROUPID == itGrp->second ) ||
                 ( !ctrlParam._role[ SDB_ROLE_COORD ] &&
                   COORD_GROUPID == itGrp->second ) )
            {
               ++itGrp ;
               continue ;
            }
            expectGrpLst[ itGrp->first ] = itGrp->second ;
            ++itGrp ;
         }
      }

      if ( !pFilterObj->isEmpty() )
      {
         rc = coordParseGroupList( _pResource, cb, *pFilterObj,
                                   groupLst, &newFilterObj,
                                   ppContext ? FALSE : TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse groups, rc: %d", rc  ) ;
         if ( pFilterObj->objdata() != newFilterObj.objdata() )
         {
            hasNodeOrGroupFilter = TRUE ;
         }
         else if ( ctrlParam._useSpecialGrp )
         {
            CoordGroupList::iterator itGrp = expectGrpLst.begin() ;
            while( itGrp != expectGrpLst.end() )
            {
               if ( ctrlParam._specialGrps.find( itGrp->first ) ==
                    ctrlParam._specialGrps.end() )
               {
                 itGrp = expectGrpLst.erase( itGrp ) ;
               }
               else
               {
                  ++itGrp ;
               }
            }
         }
         *pFilterObj = newFilterObj ;
      }

   parseNode:
      rc = coordGetGroupNodes( _pResource, cb, *pFilterObj,
                               ctrlParam._emptyFilterSel,
                               ( 0 == groupLst.size() ? ( hasParseRetry ?
                                 expectGrpLst : allGroupLst ) : groupLst ),
                               sendNodes, &newFilterObj,
                               ppContext ? FALSE : TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( sendNodes.size() == 0 && !hasParseRetry )
      {
         PD_LOG( PDWARNING, "Node specfic nodes[%s]",
                 pFilterObj->toString().c_str() ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
      else if ( pFilterObj->objdata() != newFilterObj.objdata() )
      {
         hasNodeOrGroupFilter = TRUE ;
      }
      else if ( 0 == groupLst.size() )
      {
         if ( ctrlParam._useSpecialNode )
         {
            sendNodes = ctrlParam._specialNodes ;
         }
         else if ( !hasParseRetry &&
                   allGroupLst.size() != expectGrpLst.size() )
         {
            hasParseRetry = TRUE ;
            goto parseNode ;
         }
      }
      *pFilterObj = newFilterObj ;

      if ( !ctrlParam._isGlobal )
      {
         if ( !hasNodeOrGroupFilter )
         {
            rc = SDB_RTN_CMD_IN_LOCAL_MODE ;
            goto error ;
         }
         ctrlParam._isGlobal = TRUE ;
      }

      if ( ppContext )
      {
         if ( NULL == *ppContext )
         {
            rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                                     (rtnContext **)ppContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*ppContext)->contextID() ;
         }
         pTmpContext = *ppContext ;
      }
      if ( pTmpContext && !pTmpContext->isOpened() )
      {
         if ( openEmptyContext )
         {
            rtnQueryOptions defaultOptions ;
            rc = pTmpContext->open( defaultOptions ) ;
         }
         else
         {
            rtnQueryOptions contextOptions ;
            BSONObj srcSelector = queryOption.getSelector() ;
            INT64 srcLimit = queryOption.getLimit() ;
            INT64 srcSkip = queryOption.getSkip() ;

            if ( sendNodes.size() > 1 )
            {
               if ( srcLimit > 0 && srcSkip > 0 )
               {
                  queryOption.setLimit( srcLimit + srcSkip ) ;
               }
               queryOption.setSkip( 0 ) ;
            }
            else
            {
               srcLimit = -1 ;
               srcSkip = 0 ;
            }

            rtnNeedResetSelector( srcSelector, queryOption.getOrderBy(),
                                  needReset ) ;
            if ( needReset )
            {
               queryOption.setSelector( BSONObj() ) ;
            }

            contextOptions = queryOption ;
            contextOptions.setSelector( needReset ? srcSelector : BSONObj() ) ;
            contextOptions.setLimit( srcLimit ) ;
            contextOptions.setSkip( srcSkip ) ;

            rc = pTmpContext->open( contextOptions ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Open context failed(rc=%d)", rc ) ;
      }

      if ( pSrcFilterObjData == pFilterObj->objdata() && !needReset )
      {
         pNewMsg = (CHAR*)pMsg ;
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )pNewMsg ;
         pQueryMsg->numToReturn = queryOption.getLimit() ;
         pQueryMsg->numToSkip = queryOption.getSkip() ;
      }
      else
      {
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )pMsg ;
         INT32 version = pQueryMsg->version ;

         rc = queryOption.toQueryMsg( &pNewMsg, newMsgSize, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build new query message failed, rc: %d",
                      rc ) ;

         pQueryMsg = ( MsgOpQuery* )pNewMsg ;
         pQueryMsg->version = version ;
      }

      rc = executeOnNodes( (MsgHeader*)pNewMsg, cb, sendNodes,
                           faileds, pSucNodes, pIgnoreRC,
                           pTmpContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;

      if ( pTmpContext )
      {
         rc = _buildFailedNodeReply( faileds, pTmpContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Build failed node reply failed, rc: %d",
                      rc ) ;
      }

   done:
      if ( pNewMsg && pNewMsg != (CHAR*)pMsg )
      {
         cb->releaseBuff( pNewMsg ) ;
         pNewMsg = NULL ;
         newMsgSize = 0 ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         *ppContext = NULL ;
      }
      goto done ;
   }

}

