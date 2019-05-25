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
      return &_args ;
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
      rtnContextCoord *pCoordCtxForCata = NULL ; // rename
      rtnContextCoord *pCoordCtxForData = NULL ;

      CHAR *pCataMsgBuf = NULL ;
      INT32 cataMsgSize = 0 ;

      CHAR *pDataMsgBuf = NULL ;
      INT32 dataMsgSize = 0 ;

      CHAR *pRollbackMsgBuf = NULL ;
      INT32 rollbackMsgSize = 0 ;

      coordCMDArguments *pArguments = NULL ;

      contextID = -1 ;

      /************************************************************************
       * Prepare phase
       * 1. Parse message
       * 2. Sanity check for arguments
       ************************************************************************/
      pArguments = _getArguments() ;
      PD_CHECK( pArguments, SDB_SYS, error, PDERROR, "Get arguments failed" ) ;

      pArguments->_pBuf = buf ;
      rc = _extractMsg ( pMsg, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message for command[%s], "
                   "rc: %d", getName(), rc ) ;

      rc = _parseMsg ( pMsg, pArguments ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to parse arguments for command[%s], "
                   "rc: %d", getName(), rc ) ;

      /************************************************************************
       * Phase 1
       * 1. Generate P1 message to Catalog
       * 2. Execute P1 on Catalog
       * 3. Generate P1 message to Data Groups
       * 4. Execute P1 on Data Groups
       ************************************************************************/
      rc = _generateCataMsg( pMsg, cb, pArguments,
                             &pCataMsgBuf, &cataMsgSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate message to catalog failed for "
                   "command[%s, targe:%s], rc: %d",
                   getName(), pArguments->_targetName.c_str(), rc ) ;

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

      PD_LOG( PDINFO, "Do phase 1 on catalog done for command[%s, targe:%s], "
              "get %u target groups back", getName(),
              pArguments->_targetName.c_str(), groupLst.size() ) ;

      rc = _generateDataMsg( pMsg, cb, pArguments, cataObjs,
                             &pDataMsgBuf, &dataMsgSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate message to data failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      rc = _doOnDataGroup( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                           pArguments, groupLst, cataObjs, sucGroupLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 1 on data failed for command[%s, "
                   "targe:%s, suc group size:%u], rc: %d",
                   getName(), pArguments->_targetName.c_str(),
                   sucGroupLst.size(), rc ) ;

      PD_LOG( PDINFO, "Do phase 1 on data done for command[%s, targe:%s], "
              "succeed group size: %u", getName(),
              pArguments->_targetName.c_str(), sucGroupLst.size() ) ;

      /************************************************************************
       * Phase 2
       * 1. Execute P2 on Catalog
       * 2. Execute P2 on Data Groups
       ************************************************************************/
      rc = _doOnCataGroupP2( (MsgHeader*)pCataMsgBuf, cb, &pCoordCtxForCata,
                             pArguments, groupLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on catalog failed for "
                   "command[%s, target:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 2 on catalog done for command[%s, targe:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

      rc = _doOnDataGroupP2( (MsgHeader*)pDataMsgBuf, cb, &pCoordCtxForData,
                             pArguments, groupLst, cataObjs ) ;
      PD_RC_CHECK( rc, PDERROR, "Do phase 2 on data failed for command[%s, "
                   "targe:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do phase 2 on data done for command[%s, targe:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

      /************************************************************************
       * Phase Commit
       * 1. Commit on Catalog
       * 2. Update local catalog cache if needed
       ************************************************************************/
   commit :
      rc = _doCommit( pMsg, cb, &pCoordCtxForCata, pArguments );
      PD_RC_CHECK( rc, PDERROR, "Do commit phase on catalog failed for "
                   "command[%s, targe:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do commit phase on catalog done for command[%s, "
              "targe:%s]", getName(), pArguments->_targetName.c_str() ) ;

      rc = _doComplete( pMsg, cb, pArguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Do complete phase failed for "
                   "command[%s, targe:%s], rc: %d", getName(),
                   pArguments->_targetName.c_str(), rc ) ;

      PD_LOG( PDINFO, "Do complete phase done for command[%s, targe:%s]",
              getName(), pArguments->_targetName.c_str() ) ;

   done :
      /************************************************************************
       * Phase Clean
       * 1. Audit the command Audit the command
       * 2. Delete contexts and free buffers
       ************************************************************************/
      _doAudit( pArguments, rc ) ;

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
      if ( pRollbackMsgBuf )
      {
         _releaseRollbackDataMsg( pRollbackMsgBuf, rollbackMsgSize, cb ) ;
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
      if ( !sucGroupLst.empty () && pCoordCtxForCata )
      {
         INT32 tmprc = SDB_OK ;

         if ( _flagRollbackCataBeforeData() && pCoordCtxForCata )
         {
            pRtncb->contextDelete ( pCoordCtxForCata->contextID(), cb ) ;
            pCoordCtxForCata = NULL ;
         }

         tmprc = _generateRollbackDataMsg( pMsg, cb, pArguments,
                                           &pRollbackMsgBuf,
                                           &rollbackMsgSize ) ;
         if ( SDB_OK != tmprc )
         {
            PD_LOG( PDWARNING, "Generate rollback message to data failed for "
                    "command[%s, targe:%s], rc: %d", getName(),
                    pArguments->_targetName.c_str(), tmprc ) ;

            sucGroupLst.clear() ;

            if ( _flagCommitOnRollbackFailed() && pCoordCtxForCata )
            {
               goto commit ;
            }
            goto done ;
         }
         tmprc = _rollbackOnDataGroup( (MsgHeader*)pRollbackMsgBuf, cb,
                                       pArguments, sucGroupLst ) ;
         if ( SDB_OK != tmprc )
         {
            PD_LOG( PDWARNING, "Do rollback phase on data failed for "
                    "command[%s, targe:%s], rc: %d", getName(),
                    pArguments->_targetName.c_str(), tmprc ) ;

            sucGroupLst.clear() ;

            if ( _flagCommitOnRollbackFailed() && pCoordCtxForCata )
            {
               goto commit ;
            }
            goto done ;
         }

         PD_LOG( PDINFO, "Do rollback phase on data done for command[%s, "
                 "targe:%s]", getName(), pArguments->_targetName.c_str() ) ;

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

      rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, &pContext,
                               pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to execute on catalog in command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

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

      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **ppContext,
                                             coordCMDArguments *pArgs,
                                             const CoordGroupList &groupLst,
                                             const vector<BSONObj> &cataObjs )
   {
      return SDB_OK ;
   }

   INT32 _coordCMD2Phase::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst )
   {
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

      rc = _processContext( cb, ppContext, -1 ) ;
      if ( SDB_RTN_CONTEXT_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING, "Do commit for command[%s] on [%s] failed, rc: %d",
                 getName(), pArgs->_targetName.c_str(), rc ) ;
      }

      PD_TRACE_EXITRC ( COORD_CMD2PHASE_DOCOMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD2PHASE_PROCESSCTX, "_coordCMD2Phase::_processContext" )
   INT32 _coordCMD2Phase::_processContext ( pmdEDUCB *cb,
                                            rtnContextCoord **ppContext,
                                            SINT32 maxNumSteps )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMD2PHASE_PROCESSCTX ) ;

      pmdKRCB *pKrcb = pmdGetKRCB() ;
      _SDB_RTNCB *pRtncb = pKrcb->getRTNCB() ;
      rtnContextBuf buffObj ;

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

