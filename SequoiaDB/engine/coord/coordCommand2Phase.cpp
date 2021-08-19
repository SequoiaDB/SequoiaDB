/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_CMD_RETRY_TIMES ( 10 )
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
      vector<BSONObj> cataObjs ;
      rtnContextCoord *pCoordCtxForCata = NULL ;
      rtnContextCoord *pCoordCtxForData = NULL ;

      CHAR *pCataMsgBuf = NULL ;
      INT32 cataMsgSize = 0 ;

      CHAR *pDataMsgBuf = NULL ;
      INT32 dataMsgSize = 0 ;

      coordCMDArguments arguments ;
      coordCMDArguments *pArguments = NULL ;

      contextID = -1 ;
      INT32 retryCount = 0 ;

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

      if ( !_allowInTransaction() && cb->isTransaction() )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "Operation(%s) in transaction is not supported",
                     getName() ) ;
         goto error ;
      }

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

      PD_LOG( PDINFO, "Do phase 1 on catalog done for command[%s, target:%s], "
              "get %u target groups back", getName(),
              pArguments->_targetName.c_str(), groupLst.size() ) ;

      // Generate P1 message to Data Groups
      rc = _generateDataMsg( pMsg, cb, pArguments, cataObjs,
                             &pDataMsgBuf, &dataMsgSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate message to data failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

   retry :
      // Execute P1 on Data Groups
      rc = _doOnDataGroup( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                           pArguments, groupLst, cataObjs, sucGroupLst ) ;
      if ( retryCount < COORD_CMD_RETRY_TIMES &&
           pArguments->_retryRCList.find(rc) != pArguments->_retryRCList.end() )
      {
         if ( pCoordCtxForData )
         {
            pRtncb->contextDelete ( pCoordCtxForData->contextID(), cb ) ;
            pCoordCtxForData = NULL ;
         }
         sucGroupLst.clear() ;
         PD_LOG( PDWARNING, "Do phase 1 on data failed[rc: %d] for "
                 "command[%s, target: %s], retry",
                 rc, getName(), pArguments->_targetName.c_str() ) ;
         retryCount++ ;
         goto retry ;
      }
      PD_RC_CHECK( rc, PDERROR, "Do phase 1 on data failed for command[%s, "
                   "target:%s, suc group size:%u], rc: %d",
                   getName(), pArguments->_targetName.c_str(),
                   sucGroupLst.size(), rc ) ;

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
                             pArguments, groupLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on catalog failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 2 on catalog done for command[%s, target:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

      // Execute P2 on Data Groups
      rc = _doOnDataGroupP2( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                             pArguments, groupLst, cataObjs ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on data failed for command[%s, "
                   "target:%s], rc: %d", getName(),
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

      PD_LOG( PDINFO, "Do commit phase on catalog done for command[%s, "
              "target:%s]", getName(), pArguments->_targetName.c_str() ) ;

      // Update local catalog info caches if needed
      rc = _doComplete( pMsg, cb, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Do complete phase failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do complete phase done for command[%s, target:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

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
         pCoordCtxForCata = NULL ;
      }

      if ( pCoordCtxForData )
      {
         pRtncb->contextDelete ( pCoordCtxForData->contextID(), cb ) ;
         pCoordCtxForData = NULL ;
      }

      if ( pCataMsgBuf )
      {
         _releaseCataMsg( pCataMsgBuf, cataMsgSize, cb ) ;
      }
      if ( pDataMsgBuf )
      {
         _releaseDataMsg( pDataMsgBuf, dataMsgSize, cb ) ;
      }

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
            if ( _flagCommitOnRollbackFailed() && NULL != pCoordCtxForCata )
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

      CHAR *pQuery = NULL ;

      try
      {
         _printDebug ( (const CHAR*)pMsg, getName() ) ;

         rc = msgExtractQuery( (CHAR *)pMsg, NULL, NULL,
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

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_DOONCATAGROUP, "_coordCMD2Phase::_doOnCataGroup" )
   INT32 _coordCMD2Phase::_doOnCataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           rtnContextCoord **ppContext,
                                           coordCMDArguments *pArgs,
                                           CoordGroupList *pGroupLst,
                                           vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_DOONCATAGROUP ) ;

      rtnContextCoord *pContext = NULL ;
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
         (*ppContext) = pContext ;
      }
      PD_TRACE_EXITRC ( COORD_CMD2PHASE_DOONCATAGROUP, rc ) ;
      return rc ;
   error :
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext = NULL ;
      }
      goto done ;
   }

   INT32 _coordCMD2Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **ppContext,
                                             coordCMDArguments *pArgs,
                                             const CoordGroupList &pGroupLst )
   {

      /// Do nothing
      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **ppContext,
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
                                      rtnContextCoord **ppContext,
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
                                        rtnContextCoord ** ppCoordCtxForCata,
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

      PD_LOG( PDINFO, "Do rollback phase on data done for command[%s, "
              "target:%s]", getName(), pArguments->_targetName.c_str() ) ;


   done :
      if ( NULL == pCoordCtxForCata )
      {
         *ppCoordCtxForCata = NULL ;
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
                                            rtnContextCoord **ppContext,
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
         (*ppContext) = NULL ;
      }
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      goto done ;
   }

}

