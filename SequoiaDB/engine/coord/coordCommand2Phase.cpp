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

   Source File Name = coordCommand2Phase.cpp

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

#include "coordCommand2Phase.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "utilCommon.hpp"
#include "msgMessage.hpp"
#include "coordUtil.hpp"
#include "utilString.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_CMD_RETRY_TIMES ( 10 )

   /*
      _coordCMDEventHandler implement
    */

   /*
      _coordCMD2Phase implement
   */
   _coordCMD2Phase::_coordCMD2Phase()
   {
   }

   _coordCMD2Phase::~_coordCMD2Phase()
   {
   }

   coordCMDArguments* _coordCMD2Phase::_getArguments()
   {
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_EXE, "_coordCMD2Phase::execute" )
   INT32 _coordCMD2Phase::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      PD_TRACE_ENTRY ( COORD_CMD2PHASE_EXE ) ;

      CoordGroupList groupLst ;
      CoordGroupList sucGroupLst ;
      vector<BSONObj> cataObjs, cataP2Objs ;
      rtnContextCoord::sharePtr pCoordCtxForCata ;
      rtnContextCoord::sharePtr pCoordCtxForData ;

      _utilString< DMS_COLLECTION_FULL_NAME_SZ > lastProcessName ;

      CHAR *pCataMsgBuf = NULL ;
      INT32 cataMsgSize = 0 ;
      BOOLEAN isCataMsgRewritten = FALSE ;

      CHAR *pDataMsgBuf = NULL ;
      INT32 dataMsgSize = 0 ;
      BOOLEAN isDataMsgRewritten = FALSE ;

      coordCMDArguments arguments ;
      coordCMDArguments *pArguments = NULL ;

      contextID = -1 ;
      INT32 retryCount = 0 ;

      lastProcessName.append( cb->getCurProcessName() ) ;

      /************************************************************************
       * Prepare phase
       * 1. Parse message
       * 2. Sanity check for arguments
       ************************************************************************/
      pArguments = _getArguments() ;
      if ( NULL == pArguments )
      {
         pArguments = &arguments ;
      }

      pArguments->_pBuf = buf ;
      // Extract message
      rc = _extractMsg ( pMsg, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message for command[%s], "
                   "rc: %d", getName(), rc ) ;

      // Parse and check message
      rc = _parseMsg ( pMsg, pArguments ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to parse arguments for command[%s], "
                   "rc: %d", getName(), rc ) ;

      cb->setCurProcessName( pArguments->_targetName.c_str() ) ;

      if ( !_allowInTransaction() && cb->isTransaction() )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "Operation(%s) in transaction is not supported",
                     getName() ) ;
         goto error ;
      }

      rc = _regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register event handlers for "
                   "command [%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

   retryCata :
      rc = _onBeginEvent( pArguments, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call begin events, rc: %d", rc ) ;

      /************************************************************************
       * Phase 1
       * 1. Generate P1 message to Catalog
       * 2. Execute P1 on Catalog
       * 3. Generate P1 message to Data Groups
       * 4. Execute P1 on Data Groups
       ************************************************************************/
      // Generate P1 message to Catalog
      rc = _generateCataMsg( pMsg, cb, pArguments,
                             &pCataMsgBuf, &cataMsgSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate message to catalog failed for "
                   "command[%s, target:%s], rc: %d",
                   getName(), pArguments->_targetName.c_str(), rc ) ;
      SDB_ASSERT( NULL != pCataMsgBuf,
                  "catalog message buffer should be valid" ) ;
      PD_CHECK( NULL != pCataMsgBuf, SDB_SYS, error, PDERROR,
                "Failed to generate catalog message for command[%s, target:%s]",
                getName(), pArguments->_targetName.c_str() ) ;
      if ( pMsg != (MsgHeader *)pCataMsgBuf )
      {
         isCataMsgRewritten = TRUE ;
      }

      // Execute P1 on Catalog
      rc = _doOnCataGroup( (MsgHeader*)pCataMsgBuf, cb, &pCoordCtxForCata,
                           pArguments,
                           _flagGetGrpLstFromCata() ? &groupLst : NULL,
                           &cataObjs ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         PD_LOG( PDWARNING, "Empty group list, done anyway" ) ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Do phase 1 on catalog failed for command[%s, "
                 "target:%s], rc: %d", getName(),
                 pArguments->_targetName.c_str(), rc ) ;
         goto error ;
      }

      pArguments->_groupList = groupLst ;

      rc = _parseCatReturn( pArguments, cataObjs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse CATALOG return objects "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 1 on catalog done for command[%s, target:%s], "
              "get %u target groups back", getName(),
              pArguments->_targetName.c_str(), groupLst.size() ) ;

      // Generate P1 message to Data Groups
      rc = _generateDataMsg( pMsg, cb, pArguments, cataObjs,
                             &pDataMsgBuf, &dataMsgSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate message to data failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;
      SDB_ASSERT( NULL != pDataMsgBuf,
                  "data message buffer should be valid" ) ;
      PD_CHECK( NULL != pDataMsgBuf, SDB_SYS, error, PDERROR,
                "Failed to generate data message for command[%s, target:%s]",
                getName(), pArguments->_targetName.c_str() ) ;

      if ( (MsgHeader *)pDataMsgBuf != pMsg )
      {
         isDataMsgRewritten = TRUE ;
      }

      if ( _needRewriteDataMsg() )
      {
         CHAR *pNewDataMsgBuf = NULL ;
         INT32 newDataMsgSize = 0 ;

         rc = _rewriteDataMsg( (MsgHeader *)pDataMsgBuf,
                               pArguments,
                               cb,
                               &pNewDataMsgBuf,
                               &newDataMsgSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rewrite data message for "
                      "command[%s, target:%s], rc: %d", getName(),
                      pArguments->_targetName.c_str(), rc ) ;

         if ( isDataMsgRewritten )
         {
            msgReleaseBuffer( pDataMsgBuf, cb ) ;
            isDataMsgRewritten = FALSE ;
         }
         pDataMsgBuf = pNewDataMsgBuf ;
         dataMsgSize = newDataMsgSize ;

         SDB_ASSERT( NULL != pDataMsgBuf,
                     "data message buffer should be valid" ) ;
         PD_CHECK( NULL != pDataMsgBuf, SDB_SYS, error, PDERROR,
                   "Failed to generate data message for command[%s, target:%s]",
                   getName(), pArguments->_targetName.c_str() ) ;

         isDataMsgRewritten = TRUE ;
      }

   retryData :
      // Execute P1 on Data Groups
      rc = _onDataP1Event( SDB_EVT_OCCUR_BEFORE, pArguments, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call before data P1 events "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      rc = _doOnDataGroup( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                           pArguments, groupLst, cataObjs, sucGroupLst ) ;
      if ( retryCount < COORD_CMD_RETRY_TIMES &&
           pArguments->_retryRCList.find(rc) != pArguments->_retryRCList.end() )
      {
         if ( pCoordCtxForData )
         {
            pRtncb->contextDelete ( pCoordCtxForData->contextID(), cb ) ;
            pCoordCtxForData.release() ;
         }
         sucGroupLst.clear() ;
         PD_LOG( PDWARNING, "Do phase 1 on data failed[rc: %d] for "
                 "command[%s, target: %s], retry",
                 rc, getName(), pArguments->_targetName.c_str() ) ;
         retryCount++ ;
         goto retryData ;
      }
      PD_RC_CHECK( rc, PDERROR, "Do phase 1 on data failed for command[%s, "
                   "target:%s, suc group size:%u], rc: %d",
                   getName(), pArguments->_targetName.c_str(),
                   sucGroupLst.size(), rc ) ;

      rc = _onDataP1Event( SDB_EVT_OCCUR_AFTER, pArguments, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call after data P1 events "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 1 on data done for command[%s, target:%s], "
              "succeed group size: %u", getName(),
              pArguments->_targetName.c_str(), sucGroupLst.size() ) ;

      /************************************************************************
       * Phase 2
       * 1. Execute P2 on Catalog
       * 2. Execute P2 on Data Groups
       ************************************************************************/
      // Execute P2 on Catalog
      rc = _doOnCataGroupP2( (MsgHeader*)pCataMsgBuf, cb, &pCoordCtxForCata,
                             pArguments, groupLst, cataP2Objs ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc &&
           retryCount < COORD_CMD_RETRY_TIMES )
      {
         if ( pCoordCtxForCata )
         {
            pRtncb->contextDelete( pCoordCtxForCata->contextID(), cb ) ;
            pCoordCtxForCata.release() ;
         }
         if ( pCoordCtxForData )
         {
            pRtncb->contextDelete( pCoordCtxForData->contextID(), cb ) ;
            pCoordCtxForData.release() ;
         }

         if ( isCataMsgRewritten )
         {
            msgReleaseBuffer( pCataMsgBuf, cb ) ;
            pCataMsgBuf = NULL ;
            cataMsgSize = 0 ;
            isCataMsgRewritten = FALSE ;
         }
         if ( isDataMsgRewritten )
         {
            msgReleaseBuffer( pDataMsgBuf, cb ) ;
            pDataMsgBuf = NULL ;
            dataMsgSize = 0 ;
            isDataMsgRewritten = FALSE ;
         }

         groupLst.clear() ;
         sucGroupLst.clear() ;

         PD_LOG( PDWARNING, "Do phase 2 on cata failed[rc: %d] for "
                 "command[%s, target: %s], retry",
                 rc, getName(), pArguments->_targetName.c_str() ) ;

         ++ retryCount ;
         goto retryCata ;
      }
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on catalog failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      rc = _parseCatP2Return( pArguments, cataP2Objs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse CATALOG P2 return objects "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 2 on catalog done for command[%s, target:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

      // Execute P2 on Data Groups
      rc = _onDataP2Event( SDB_EVT_OCCUR_BEFORE, pArguments, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call before data P2 events "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      rc = _doOnDataGroupP2( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                             pArguments, groupLst, cataObjs ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on data failed for command[%s, "
                   "target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      rc = _onDataP2Event( SDB_EVT_OCCUR_AFTER, pArguments, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call after data P2 events "
                   "for command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 2 on data done for command[%s, target:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

   commit :
      /************************************************************************
       * Phase Commit
       * 1. Commit on Catalog
       * 2. Update local catalog cache if needed
       ************************************************************************/
      // Commit on Catalog
      rc = _doCommit( pMsg, cb, &pCoordCtxForCata, pArguments );
      PD_RC_CHECK( rc, PDERROR, "Do commit phase on catalog failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      _onCommitEvent( pArguments, cb ) ;

      PD_LOG( PDINFO, "Do commit phase on catalog done for command[%s, "
              "target:%s]", getName(), pArguments->_targetName.c_str() ) ;

      // Update local catalog info caches if needed
      rc = _doComplete( pMsg, cb, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Do complete phase failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do complete phase done for command[%s, target:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

      rc = _doOutput( buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to output result, rc: %d", rc ) ;

   done :
      /************************************************************************
       * Phase Clean
       * 1. Audit the command Audit the command
       * 2. Delete contexts and free buffers
       ************************************************************************/
      // Audit command
      _doAudit( pArguments, rc ) ;

      // Clear allocated context and buffers
      if ( pCoordCtxForCata )
      {
         pRtncb->contextDelete ( pCoordCtxForCata->contextID(), cb ) ;
         pCoordCtxForCata.release() ;
      }

      if ( pCoordCtxForData )
      {
         pRtncb->contextDelete ( pCoordCtxForData->contextID(), cb ) ;
         pCoordCtxForData.release() ;
      }

      if ( isCataMsgRewritten )
      {
         msgReleaseBuffer( pCataMsgBuf, cb ) ;
         pCataMsgBuf = NULL ;
         cataMsgSize = 0 ;
         isCataMsgRewritten = FALSE ;
      }
      if ( isDataMsgRewritten )
      {
         msgReleaseBuffer( pDataMsgBuf, cb ) ;
         pDataMsgBuf = NULL ;
         dataMsgSize = 0 ;
         isDataMsgRewritten = FALSE ;
      }

      _unregEventHandlers() ;
      cb->setCurProcessName( lastProcessName.str() ) ;

      PD_TRACE_EXITRC ( COORD_CMD2PHASE_EXE, rc ) ;
      return rc ;

   error :
      /************************************************************************
       * Phase Rollback
       * 1. Generate rollback message to succeed Data Groups
       * 2. Execute rollback on succeed Data Groups
       * Note: updates to Catalog will be rollback by kill context
       ************************************************************************/
      {
         INT32 tmpRC = _doRollback( pMsg, cb, &pCoordCtxForCata, pArguments,
                                    sucGroupLst, rc ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Do rollback phase failed for "
                    "command[%s, target:%s], rc: %d", getName(),
                    pArguments->_targetName.c_str(), tmpRC ) ;
            if ( _flagCommitOnRollbackFailed() && pCoordCtxForCata )
            {
               goto commit ;
            }
         }
         PD_LOG( PDINFO, "Do rollback done for command[%s, target:%s]",
                 getName(), pArguments->_targetName.c_str() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_EXTMSG, "_coordCMD2Phase::_extractMsg" )
   INT32 _coordCMD2Phase::_extractMsg ( MsgHeader *pMsg,
                                        coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_EXTMSG ) ;

      const CHAR *pQuery = NULL ;

      try
      {
         _printDebug ( (const CHAR*)pMsg, getName() ) ;

         rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL,
                               NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse message for command[%s] failed, "
                      "rc: %d", getName(), rc ) ;

         pArgs->_boQuery.init( pQuery ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMD2PHASE_EXTMSG, rc ) ;
      return rc;
   error :
      goto done ;
   }

   INT32 _coordCMD2Phase::_doComplete ( MsgHeader *pMsg,
                                        pmdEDUCB * cb,
                                        coordCMDArguments *pArgs )
   {
      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_doOutput( rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_DOONCATAGROUP, "_coordCMD2Phase::_doOnCataGroup" )
   INT32 _coordCMD2Phase::_doOnCataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           rtnContextCoord::sharePtr *ppContext,
                                           coordCMDArguments *pArgs,
                                           CoordGroupList *pGroupLst,
                                           vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_DOONCATAGROUP ) ;

      rtnContextCoord::sharePtr pContext ;
      rtnContextBuf buffObj ;

      // Send request to catalog, and get the control of CoordContext
      rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, &pContext,
                               pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to execute on catalog in command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      /// get cached data
      while( pContext && pContext->getCachedRecordNum() > 0 )
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

            if ( pGroupLst )
            {
               rc = coordGetGroupsFromObj( obj, *pGroupLst ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get groups from catalog "
                            "reply [%s], rc: %d", obj.toString().c_str(), rc ) ;
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
         *ppContext = pContext ;
      }
      PD_TRACE_EXITRC ( COORD_CMD2PHASE_DOONCATAGROUP, rc ) ;
      return rc ;
   error :
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext.release() ;
      }
      goto done ;
   }

   INT32 _coordCMD2Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *ppContext,
                                             coordCMDArguments *pArgs,
                                             const CoordGroupList &pGroupLst,
                                             vector<BSONObj> &cataObjs )
   {

      /// Do nothing
      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *ppContext,
                                             coordCMDArguments *pArgs,
                                             const CoordGroupList &groupLst,
                                             const vector<BSONObj> &cataObjs )
   {
      /// Do nothing
      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst )
   {
      /// Do nothing
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_DOCOMMIT, "_coordCMD2Phase::_doCommit" )
   INT32 _coordCMD2Phase::_doCommit ( MsgHeader *pMsg,
                                      pmdEDUCB * cb,
                                      rtnContextCoord::sharePtr *ppContext,
                                      coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_DOCOMMIT ) ;

      rtnContextBuf buffObj ;

      rc = _processContext( cb, ppContext, -1, buffObj ) ;
      if ( SDB_RTN_CONTEXT_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING, "Do commit for command[%s] on [%s] failed, rc: %d",
                 getName(), pArgs->_targetName.c_str(), rc ) ;
      }

      PD_TRACE_EXITRC ( COORD_CMD2PHASE_DOCOMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_DOROLLBACK, "_coordCMD2Phase::_doRollback" )
   INT32 _coordCMD2Phase::_doRollback ( MsgHeader * pMsg,
                                        pmdEDUCB * cb,
                                        rtnContextCoord::sharePtr * ppCoordCtxForCata,
                                        coordCMDArguments * pArguments,
                                        CoordGroupList & sucGroupLst,
                                        INT32 failedRC )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_DOROLLBACK ) ;

      SDB_ASSERT( NULL != ppCoordCtxForCata, "ppCoordCtxForCata is invalid" ) ;

      SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

      CHAR * pRollbackMsgBuf = NULL ;
      INT32 rollbackMsgSize = 0 ;

      rtnContextCoord * pCoordCtxForCata = *ppCoordCtxForCata ;

      // The command could be rollbacked in two conditions:
      // 1. There are succeed Data Groups
      // 2. There are Catalog context to rollback catalog

      // Rollback Catalog first if needed
      if ( _flagRollbackCataBeforeData() && NULL != pCoordCtxForCata )
      {
         rtnCB->contextDelete ( pCoordCtxForCata->contextID(), cb ) ;
         pCoordCtxForCata = NULL ;
      }
      if ( !sucGroupLst.empty () )
      {
         // Generate rollback message to succeed Data Groups
         rc = _generateRollbackDataMsg( pMsg, cb, pArguments,
                                        &pRollbackMsgBuf,
                                        &rollbackMsgSize ) ;
         PD_RC_CHECK( rc, PDWARNING, "Generate rollback message to data failed "
                      "for command[%s, target:%s], rc: %d", getName(),
                      pArguments->_targetName.c_str(), rc ) ;
         SDB_ASSERT( NULL != pRollbackMsgBuf,
                     "rollback message buffer should be valid" ) ;
         PD_CHECK( NULL != pRollbackMsgBuf, SDB_SYS, error, PDERROR,
                   "Failed to generate rollback message for "
                   "command[%s, target:%s]", getName(),
                   pArguments->_targetName.c_str() ) ;
         cb->startTransRollback() ;
         rc = _rollbackOnDataGroup( (MsgHeader*)pRollbackMsgBuf, cb,
                                    pArguments, sucGroupLst ) ;
         cb->stopTransRollback() ;
         PD_RC_CHECK( rc, PDWARNING, "Do rollback phase on data failed for "
                      "command[%s, target:%s], rc: %d", getName(),
                      pArguments->_targetName.c_str(), rc ) ;
      }
      if ( NULL != pCoordCtxForCata )
      {
         rtnCB->contextDelete( pCoordCtxForCata->contextID(), cb ) ;
         pCoordCtxForCata = NULL ;
      }

      _onRollbackEvent( pArguments, cb ) ;

      PD_LOG( PDINFO, "Do rollback phase on data done for command[%s, "
              "target:%s]", getName(), pArguments->_targetName.c_str() ) ;


   done :
      if ( NULL == pCoordCtxForCata )
      {
         ppCoordCtxForCata->release() ;
      }
      if ( pRollbackMsgBuf )
      {
         _releaseRollbackDataMsg( pRollbackMsgBuf, rollbackMsgSize, cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD2PHASE_DOROLLBACK, rc ) ;
      return rc ;

   error :
      sucGroupLst.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_PROCESSCTX, "_coordCMD2Phase::_processContext" )
   INT32 _coordCMD2Phase::_processContext ( pmdEDUCB *cb,
                                            rtnContextCoord::sharePtr *ppContext,
                                            SINT32 maxNumSteps,
                                            rtnContextBuf & buffObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_PROCESSCTX ) ;

      pmdKRCB *pKrcb = pmdGetKRCB() ;
      _SDB_RTNCB *pRtncb = pKrcb->getRTNCB() ;

      if ( !ppContext || !(*ppContext) )
      {
         goto done ;
      }

      rc = (*ppContext)->getMore( maxNumSteps, buffObj, cb ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            PD_LOG( PDDEBUG, "Hit end of context" ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to process context with %d steps, "
                    "rc: %d", maxNumSteps, rc ) ;
         }
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMD2PHASE_PROCESSCTX, rc ) ;
      return rc ;

   error :
      if ( ppContext && (*ppContext) )
      {
         pRtncb->contextDelete ( (*ppContext)->contextID(), cb ) ;
         ppContext->release() ;
      }
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE__REGEVENTHANDLER, "_coordCMD2Phase::_regEventHandler" )
   INT32 _coordCMD2Phase::_regEventHandler( coordCMDEventHandler *handler )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__REGEVENTHANDLER ) ;

      SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *temp = *iter ;
         if ( handler == temp )
         {
            PD_LOG( PDDEBUG, "Handler [%s] already registered to command [%s]",
                    handler->getName(), getName() ) ;
            goto done ;
         }
      }

      try
      {
         _eventHandlers.push_back( handler ) ;
         PD_LOG( PDDEBUG, "Registered handler [%s] to command [%s]",
                 handler->getName(), getName() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register event handler, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__REGEVENTHANDLER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE__UNREGEVENTHANDLERS, "_coordCMD2Phase::_unregEventHandlers" )
   void _coordCMD2Phase::_unregEventHandlers()
   {
      PD_TRACE_ENTRY( COORD_CMD2PHASE__UNREGEVENTHANDLERS ) ;

      _eventHandlers.clear() ;

      PD_TRACE_EXIT( COORD_CMD2PHASE__UNREGEVENTHANDLERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__PARSECATRETURN, "_coordCMD2Phase::_parseCatReturn" )
   INT32 _coordCMD2Phase::_parseCatReturn( coordCMDArguments *pArgs,
                                           const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__PARSECATRETURN ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->parseCatReturn( pArgs, cataObjs ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call parse catalog return "
                      "for handler [%s] of command [%s], rc: %d",
                      handler->getName(), getName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__PARSECATP2RETURN, "_coordCMD2Phase::_parseCatP2Return" )
   INT32 _coordCMD2Phase::_parseCatP2Return( coordCMDArguments *pArgs,
                                             const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__PARSECATP2RETURN ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->parseCatP2Return( pArgs, cataObjs ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call parse catalog P2 return "
                      "for handler [%s] of command [%s], rc: %d",
                      handler->getName(), getName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__PARSECATP2RETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__NEEDREWRITEDATAMSG, "_coordCMD2Phase::_needRewriteDataMsg" )
   BOOLEAN _coordCMD2Phase::_needRewriteDataMsg()
   {
      BOOLEAN needRewrite = FALSE ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__NEEDREWRITEDATAMSG ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         if ( handler->needRewriteDataMsg() )
         {
            needRewrite = TRUE ;
            break ;
         }
      }

      PD_TRACE_EXIT( COORD_CMD2PHASE__NEEDREWRITEDATAMSG ) ;

      return needRewrite ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__REWRITEDATAMSG, "_coordCMD2Phase::_rewriteDataMsg" )
   INT32 _coordCMD2Phase::_rewriteDataMsg( MsgHeader *pMsg,
                                           coordCMDArguments *pArgs,
                                           pmdEDUCB *cb,
                                           CHAR **ppMsgBuf,
                                           INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__REWRITEDATAMSG ) ;

      INT32 flag = 0 ;
      const CHAR *pCommandName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pSelector = NULL ;
      const CHAR *pOrder = NULL ;
      const CHAR *pHint = NULL ;

      CHAR *newBuffer = NULL ;
      INT32 newBufferSize = 0 ;

      SDB_ASSERT( NULL != pMsg, "message is invalid" ) ;
      SDB_ASSERT( NULL != ppMsgBuf, "message buffer is invalid" ) ;
      SDB_ASSERT( NULL != pBufSize, "message buffer size is invalid" ) ;

      rc = msgExtractQuery( (const CHAR *)pMsg, &flag, &pCommandName,
                            &numToSkip, &numToReturn, &pQuery, &pSelector,
                            &pOrder, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse message for command[%s] failed, "
                   "rc: %d", getName(), rc ) ;

      try
      {
         BSONObjBuilder queryBuilder, hintBuilder ;

         BSONObj boQuery = BSONObj( pQuery ) ;
         BSONObj boSelector = BSONObj( pSelector ) ;
         BSONObj boOrder = BSONObj( pOrder ) ;
         BSONObj boHint = BSONObj( pHint ) ;
         BSONObj boNewQuery, boNewHint ;

         queryBuilder.appendElements( boQuery ) ;
         hintBuilder.appendElements( boHint ) ;

         for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
               iter != _eventHandlers.end() ;
               ++ iter )
         {
            coordCMDEventHandler *handler = *iter ;
            SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

            if ( handler->needRewriteDataMsg() )
            {
               rc = handler->rewriteDataMsg( queryBuilder, hintBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to call rewrite data message "
                            "for handler [%s] of command [%s], rc: %d",
                            handler->getName(), getName(), rc ) ;
            }
         }

         boNewQuery = queryBuilder.done() ;
         boNewHint = hintBuilder.done() ;

         rc = msgBuildQueryMsg( &newBuffer, &newBufferSize, pCommandName,
                                flag, 0, numToSkip, numToReturn, &boNewQuery,
                                &boSelector, &boOrder, &boNewHint, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build query message for "
                      "command[%s], rc: %d", getName(), rc ) ;

         *ppMsgBuf = newBuffer ;
         *pBufSize = newBufferSize ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rewrite data message for command[%s], "
                 "occur exception %s", getName(), e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__REWRITEDATAMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__ONBEGINEVENT, "_coordCMD2Phase::_onBeginEvent" )
   INT32 _coordCMD2Phase::_onBeginEvent( coordCMDArguments *pArgs,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__ONBEGINEVENT ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->onBeginEvent( _pResource, pArgs, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call begin event on "
                      "handler [%s] of command [%s], rc: %d",
                      handler->getName(), getName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__ONBEGINEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__ONDATAP1EVENT, "_coordCMD2Phase::_onDataP1Event" )
   INT32 _coordCMD2Phase::_onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                          coordCMDArguments *pArgs,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__ONDATAP1EVENT ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->onDataP1Event( type, _pResource, pArgs, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] execute event on "
                      "handler [%s] of command [%s], rc: %d",
                      SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                      handler->getName(), getName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__ONDATAP2EVENT, "_coordCMD2Phase::_onDataP2Event" )
   INT32 _coordCMD2Phase::_onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                          coordCMDArguments *pArgs,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_CMD2PHASE__ONDATAP2EVENT ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->onDataP2Event( type, _pResource, pArgs, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] execute event on "
                      "handler [%s] of command [%s], rc: %d",
                      SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                      handler->getName(), getName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CMD2PHASE__ONDATAP2EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__ONCOMMITEVENT, "_coordCMD2Phase::_onCommitEvent" )
   void _coordCMD2Phase::_onCommitEvent( coordCMDArguments *pArgs,
                                         pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( COORD_CMD2PHASE__ONCOMMITEVENT ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         // on commit phase, ignore error
         INT32 tmpRC = handler->onCommitEvent( _pResource, pArgs, cb ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to call commit event on "
                    "handler [%s] of command [%s], rc: %d",
                    handler->getName(), getName(), tmpRC ) ;
         }
      }

      PD_TRACE_EXIT( COORD_CMD2PHASE__ONCOMMITEVENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD2PHASE__ONROLLBACKEVENT, "_coordCMD2Phase::_onRollbackEvent" )
   void _coordCMD2Phase::_onRollbackEvent( coordCMDArguments *pArgs,
                                           pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( COORD_CMD2PHASE__ONROLLBACKEVENT ) ;

      for ( COORD_CMD_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         coordCMDEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         // on rollback phase, ignore error
         INT32 tmpRC = handler->onRollbackEvent( _pResource, pArgs, cb ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to call rollback event on "
                    "handler [%s] of command [%s], rc: %d",
                    handler->getName(), getName(), tmpRC ) ;
         }
      }

      PD_TRACE_EXIT( COORD_CMD2PHASE__ONROLLBACKEVENT ) ;
   }

}
