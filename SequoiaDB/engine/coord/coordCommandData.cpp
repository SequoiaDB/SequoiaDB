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

   Source File Name = coordCommandData.cpp

   Descriptive Name = Coord Commands for Data Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCommandData.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "dms.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "rtnContextDump.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson;

namespace engine
{
   /*
      _coordDataCMD2Phase implement
   */
   _coordDataCMD2Phase::_coordDataCMD2Phase()
   {
   }

   _coordDataCMD2Phase::~_coordDataCMD2Phase()
   {
   }

   INT32 _coordDataCMD2Phase::_generateDataMsg ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      pMsg->opCode = MSG_BS_QUERY_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordDataCMD2Phase::_releaseDataMsg( CHAR *pMsgBuf,
                                              INT32 bufSize,
                                              pmdEDUCB *cb )
   {
   }

   INT32 _coordDataCMD2Phase::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         coordCMDArguments *pArgs,
                                                         CHAR **ppMsgBuf,
                                                         INT32 *pBufSize )
   {
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordDataCMD2Phase::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                      INT32 bufSize,
                                                      pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOONCATA, "_coordDataCMD2Phase::_doOnCataGroup" )
   INT32 _coordDataCMD2Phase::_doOnCataGroup( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord **ppContext,
                                              coordCMDArguments *pArgs,
                                              CoordGroupList *pGroupLst,
                                              vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOONCATA ) ;

      rtnContextCoord *pContext = NULL ;
      coordCataSel cataSel ;

      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         rc = cataSel.bind( _pResource, pArgs->_targetName.c_str(),
                            cb, TRUE, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                    "failed on command[%s], rc: %d",
                    pArgs->_targetName.c_str(), getName(), rc ) ;
            goto error ;
         }
      }

   retry:
      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         MsgOpQuery *pOpMsg = (MsgOpQuery *)pMsg ;
         pOpMsg->version = cataSel.getCataPtr()->getVersion() ;
      }

      rc = _coordCMD2Phase::_doOnCataGroup( pMsg, cb, &pContext, pArgs,
                                            pGroupLst, pReplyObjs ) ;
      if ( rc && _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         if ( checkRetryForCLOpr( rc, NULL, cataSel, pMsg,
                                  cb, rc, NULL, TRUE ) )
         {
            _groupSession.getGroupCtrl()->incRetry() ;
            goto retry ;
         }
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Do on catalog failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

   done :
      if ( pContext )
      {
         (*ppContext) = pContext ;
      }
      PD_TRACE_EXITRC ( COORD_DATA2PHASE_DOONCATA, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOONDATA, "_coordDataCMD2Phase::_doOnDataGroup" )
   INT32 _coordDataCMD2Phase::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOONDATA ) ;

      if ( _flagDoOnCollection() )
      {
         rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                           _flagUpdateBeforeData(),
                           _flagUseGrpLstInCoord() ? NULL : &groupLst,
                           &(pArgs->_ignoreRCList), &sucGroupLst,
                           ppContext, pArgs->_pBuf ) ;
      }
      else
      {
         rc = executeOnDataGroup( pMsg, cb, groupLst, TRUE,
                                  &(pArgs->_ignoreRCList), NULL,
                                  ppContext, pArgs->_pBuf ) ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Do on data group failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DATA2PHASE_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOAUDIT "_coordDataCMD2Phase::_doAudit" )
   INT32 _coordDataCMD2Phase::_doAudit ( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOAUDIT ) ;

      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(),
                           _flagDoOnCollection() ? AUDIT_OBJ_CL : AUDIT_OBJ_CS,
                           pArgs->_targetName.c_str(), rc, "Option: %s",
                           pArgs->_boQuery.toString().c_str() ) ;
      }

      PD_TRACE_EXIT ( COORD_DATA2PHASE_DOAUDIT ) ;
      return SDB_OK ;
   }

   /*
      _coordDataCMD3Phase implement
   */
   _coordDataCMD3Phase::_coordDataCMD3Phase()
   {
   }

   _coordDataCMD3Phase::~_coordDataCMD3Phase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA3PHASE_DOONCATA2, "_coordDataCMD3Phase::_doOnCataGroupP2" )
   INT32 _coordDataCMD3Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &pGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA3PHASE_DOONCATA2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( COORD_DATA3PHASE_DOONCATA2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA3PHASE_DOONDATA2, "_coordDataCMD3Phase::_doOnDataGroupP2" )
   INT32 _coordDataCMD3Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA3PHASE_DOONDATA2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( COORD_DATA3PHASE_DOONDATA2, rc ) ;
      return rc ;
   }

   /*
      _coordCMDTestCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTestCollectionSpace,
                                      CMD_NAME_TEST_COLLECTIONSPACE,
                                      TRUE ) ;
   _coordCMDTestCollectionSpace::_coordCMDTestCollectionSpace()
   {
   }

   _coordCMDTestCollectionSpace::~_coordCMDTestCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_TESTCS_EXE, "_coordCMDTestCollectionSpace::execute" )
   INT32 _coordCMDTestCollectionSpace::execute( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                INT64 &contextID,
                                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMD_TESTCS_EXE ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;
      rtnContextBuf buffObj ;

      contextID = -1 ;

      rc = pFactory->create( CMD_NAME_LIST_COLLECTIONSPACES, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_LIST_COLLECTIONSPACES, rc ) ;
         goto error ;
      }
      rc = pOperator->init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator failed[%s], rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Execute operator[%s] failed, rc: %d",
                  pOperator->getName(), rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( rc )
      {
         contextID = -1 ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         else
         {
            PD_LOG ( PDERROR, "getmore failed, rc: %d", rc ) ;
         }
      }

   done:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_TESTCS_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDTestCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTestCollection,
                                      CMD_NAME_TEST_COLLECTION,
                                      TRUE ) ;
   _coordCMDTestCollection::_coordCMDTestCollection()
   {
   }

   _coordCMDTestCollection::~_coordCMDTestCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_TESTCL_EXE, "_coordCMDTestCollection::execute" )
   INT32 _coordCMDTestCollection::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMD_TESTCL_EXE ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;
      rtnContextBuf buffObj ;

      contextID                        = -1 ;

      rc = pFactory->create( CMD_NAME_LIST_COLLECTIONS, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_LIST_COLLECTIONS, rc ) ;
         goto error ;
      }
      rc = pOperator->init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator failed[%s], rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Execute operator[%s] failed, rc: %d",
                  pOperator->getName(), rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( rc )
      {
         contextID = -1 ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         else
         {
            PD_LOG ( PDERROR, "Getmore failed, rc: %d", rc ) ;
         }
      }

   done:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_TESTCL_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdWaitTask implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdWaitTask,
                                      CMD_NAME_WAITTASK,
                                      TRUE ) ;
   _coordCmdWaitTask::_coordCmdWaitTask()
   {
   }

   _coordCmdWaitTask::~_coordCmdWaitTask()
   {
   }

   INT32 _coordCmdWaitTask::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SET_RC ignoreRC ;
      rtnContextCoord *pContext        = NULL ;
      rtnContextBuf buffObj ;
      pmdKRCB *pKRCB                   = pmdGetKRCB() ;
      contextID                        = -1 ;
      pMsg->opCode                     = MSG_CAT_QUERY_TASK_REQ ;
      pMsg->TID                        = cb->getTID() ;

      ignoreRC.insert( SDB_DMS_EOC ) ;
      ignoreRC.insert( SDB_CAT_TASK_NOTFOUND ) ;

      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = executeOnCataGroup( pMsg, cb, TRUE, &ignoreRC, &pContext, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query task on catalog failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = pContext->getMore( -1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get more failed, rc: %d", rc ) ;
            goto error ;
         }

         pKRCB->getRTNCB()->contextDelete( pContext->contextID(), cb ) ;
         pContext = NULL ;
         ossSleep( OSS_ONE_SEC ) ;
      }

   done:
      if ( pContext )
      {
         pKRCB->getRTNCB()->contextDelete( pContext->contextID(),  cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdCancelTask implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdCancelTask,
                                      CMD_NAME_CANCEL_TASK,
                                      TRUE ) ;
   _coordCmdCancelTask::_coordCmdCancelTask()
   {
   }

   _coordCmdCancelTask::~_coordCmdCancelTask()
   {
   }

   INT32 _coordCmdCancelTask::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCommandFactory *pFactory    = NULL ;
      coordOperator *pOperator         = NULL ;
      BOOLEAN async                    = FALSE ;

      contextID                        = -1 ;

      CoordGroupList groupLst ;
      INT32 rcTmp = SDB_OK ;

      CHAR *pQueryBuf = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL, &pQueryBuf,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      try
      {
         BSONObj matcher( pQueryBuf ) ;
         rc = rtnGetBooleanElement( matcher, FIELD_NAME_ASYNC, async ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_ASYNC, rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pMsg->opCode                     = MSG_CAT_SPLIT_CANCEL_REQ ;

      rc = executeOnCataGroup( pMsg, cb, &groupLst, NULL, TRUE,
                               NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Excute on catalog failed, rc: %d", rc ) ;

      pMsg->opCode                     = MSG_BS_QUERY_REQ ;
      rcTmp = executeOnDataGroup( pMsg, cb, groupLst,
                                  TRUE, NULL, NULL, NULL,
                                  buf ) ;
      if ( rcTmp )
      {
         PD_LOG( PDWARNING, "Failed to notify to data node, rc: %d", rcTmp ) ;
      }

      if ( !async )
      {
         pFactory = coordGetFactory() ;
         rc = pFactory->create( CMD_NAME_WAITTASK, pOperator ) ;
         PD_RC_CHECK( rc, PDERROR, "Create operator by name[%s] failed, rc: %d",
                      CMD_NAME_WAITTASK, rc ) ;
         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                      pOperator->getName(), rc ) ;
         rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDTruncate implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTruncate,
                                      CMD_NAME_TRUNCATE,
                                      FALSE ) ;
   _coordCMDTruncate::_coordCMDTruncate()
   {
   }

   _coordCMDTruncate::~_coordCMDTruncate()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_TRUNCATE_EXE, "_coordCMDTruncate::execute" )
   INT32 _coordCMDTruncate::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_TRUNCATE_EXE ) ;
      CHAR *option = NULL;
      BSONObj boQuery ;
      const CHAR *fullName = NULL ;
      rc = msgExtractQuery( ( CHAR * )pMsg, NULL, NULL,
                            NULL, NULL, &option, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         boQuery = BSONObj( option );
         BSONElement e = boQuery.getField( FIELD_NAME_COLLECTION );
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid truncate msg:%s",
                    boQuery.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fullName = e.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

      rc = executeOnCL( pMsg, cb, fullName, FALSE, NULL, NULL,
                        NULL, NULL, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate cl:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }
   done:
      if ( fullName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_TRUNCATE, AUDIT_OBJ_CL,
                           fullName, rc, "" ) ;
      }
      PD_TRACE_EXITRC( COORD_TRUNCATE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDCreateCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateCollectionSpace,
                                      CMD_NAME_CREATE_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCMDCreateCollectionSpace::_coordCMDCreateCollectionSpace()
   {
   }

   _coordCMDCreateCollectionSpace::~_coordCMDCreateCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATECS_EXE, "_coordCMDCreateCollectionSpace::execute" )
   INT32 _coordCMDCreateCollectionSpace::execute( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECS_EXE ) ;

      CHAR *pQuery = NULL ;

      contextID = -1 ;
      MsgOpQuery *pCreateReq = (MsgOpQuery *)pMsg;

      try
      {
         BSONObj boQuery ;
         BSONElement ele ;
         const CHAR *pCSName = NULL ;

         _printDebug ( (const CHAR*)pMsg, getName() ) ;

         rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to extract message, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;
         ele = boQuery.getField( FIELD_NAME_NAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Get field[%s] failed from obj[%s] in "
                    "command[%s]", FIELD_NAME_NAME, boQuery.toString().c_str(),
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pCSName = ele.valuestr() ;

         pCreateReq->header.opCode = MSG_CAT_CREATE_COLLECTION_SPACE_REQ ;
         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_CS,
                           pCSName, rc, "Option:%s",
                           boQuery.toString().c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create collection space[%s], rc: %d",
                     pCSName, rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CREATECS_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDDropCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropCollectionSpace,
                                      CMD_NAME_DROP_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCMDDropCollectionSpace::_coordCMDDropCollectionSpace()
   {
   }

   _coordCMDDropCollectionSpace::~_coordCMDDropCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS_PARSEMSG, "_coordCMDDropCollectionSpace::_parseMsg" )
   INT32 _coordCMDDropCollectionSpace::_parseMsg ( MsgHeader *pMsg,
                                                   coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCS_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      CAT_COLLECTION_SPACE_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_COLLECTION_SPACE_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collectionspace name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pArgs->_ignoreRCList.insert( SDB_DMS_CS_NOTEXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPCS_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDDropCollectionSpace::_generateCataMsg ( MsgHeader *pMsg,
                                                          pmdEDUCB *cb,
                                                          coordCMDArguments *pArgs,
                                                          CHAR **ppMsgBuf,
                                                          INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DROP_SPACE_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDDropCollectionSpace::_releaseCataMsg( CHAR *pMsgBuf,
                                                       INT32 bufSize,
                                                       pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS_DOCOMPLETE, "_coordCMDDropCollectionSpace::_doComplete" )
   INT32 _coordCMDDropCollectionSpace::_doComplete ( MsgHeader *pMsg,
                                                     pmdEDUCB * cb,
                                                     coordCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( COORD_DROPCS_DOCOMPLETE ) ;

      vector< string > subCLSet ;
      _pResource->removeCataInfoByCS( pArgs->_targetName.c_str(), &subCLSet ) ;

      vector< string >::iterator it = subCLSet.begin() ;
      while( it != subCLSet.end() )
      {
         _pResource->removeCataInfo( (*it).c_str() ) ;
         ++it ;
      }

      PD_TRACE_EXIT ( COORD_DROPCS_DOCOMPLETE ) ;
      return SDB_OK ;
   }

   /*
      _coordCMDCreateCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateCollection,
                                      CMD_NAME_CREATE_COLLECTION,
                                      FALSE ) ;
   _coordCMDCreateCollection::_coordCMDCreateCollection()
   {
   }

   _coordCMDCreateCollection::~_coordCMDCreateCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_PARSEMSG, "_coordCMDCreateCollection::_parseMsg" )
   INT32 _coordCMDCreateCollection::_parseMsg ( MsgHeader *pMsg,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CREATECL_PARSEMSG ) ;

      try
      {
         BOOLEAN isMainCL = FALSE ;
         BOOLEAN isCapped = FALSE ;
         BOOLEAN isCompressed = FALSE ;
         BOOLEAN autoSplit = FALSE ;
         BOOLEAN autoIndexId = FALSE ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_CAPPED,
                                    isCapped ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isCapped = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_CAPPED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ISMAINCL,
                                    isMainCL ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isMainCL = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_ISMAINCL, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_COMPRESSED,
                                    isCompressed ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isCompressed = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_COMPRESSED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery,
                                    FIELD_NAME_DOMAIN_AUTO_SPLIT, autoSplit ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            autoSplit = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_DOMAIN_AUTO_SPLIT, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery,
                                    FIELD_NAME_AUTO_INDEX_ID, autoIndexId ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            autoIndexId = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_AUTO_INDEX_ID, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( isCapped &&
              ( isMainCL || isCompressed || autoSplit || autoIndexId ) )
         {
            PD_LOG( PDERROR, "Sharding/Compress/Index is not support on capped "
                    "collection" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( isMainCL )
         {
            BSONObj boShardingKey ;
            string shardingType ;

            if ( isCapped )
            {
               PD_LOG( PDERROR,
                       "Sharding is not allowed on capped collection" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = rtnGetObjElement( pArgs->_boQuery, FIELD_NAME_SHARDINGKEY,
                                   boShardingKey ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                       FIELD_NAME_SHARDINGKEY, getName(), rc ) ;
               rc = SDB_FIELD_NOT_EXIST == rc ?
                    SDB_NO_SHARDINGKEY : SDB_INVALIDARG ;
               goto error ;
            }

            rc = rtnGetSTDStringElement( pArgs->_boQuery, FIELD_NAME_SHARDTYPE,
                                         shardingType ) ;
            if ( SDB_OK == rc )
            {
               if ( 0 != shardingType.compare( FIELD_NAME_SHARDTYPE_RANGE ) )
               {
                  PD_LOG( PDERROR, "Sharding type must be range in "
                          "main colllection" ) ;
                  rc = SDB_INVALID_MAIN_CL_TYPE ;
                  goto error ;
               }
            }
            else if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                       FIELD_NAME_SHARDTYPE_HASH, getName(), rc ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         pArgs->_ignoreRCList.insert( SDB_DMS_EXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATECL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDCreateCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       coordCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_CREATE_COLLECTION_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDCreateCollection::_releaseCataMsg( CHAR *pMsgBuf,
                                                    INT32 bufSize,
                                                    pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_GENROLLBACKMSG, "_coordCMDCreateCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDCreateCollection::_generateRollbackDataMsg( MsgHeader *pMsg,
                                                              pmdEDUCB *cb,
                                                              coordCMDArguments *pArgs,
                                                              CHAR **ppMsgBuf,
                                                              INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECL_GENROLLBACKMSG ) ;

      rc = msgBuildDropCLMsg( ppMsgBuf, pBufSize,
                              pArgs->_targetName.c_str(),
                              0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message failed on command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATECL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDCreateCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                            INT32 bufSize,
                                                            pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_ROLLBACKONDATA, "_coordCMDCreateCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDCreateCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                           pmdEDUCB *cb,
                                                           coordCMDArguments *pArgs,
                                                           const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECL_ROLLBACKONDATA ) ;

      SET_RC ignoreRC ;
      rtnContextCoord *pCtxForData = NULL ;

      ignoreRC.insert( SDB_DMS_NOTEXIST ) ;
      ignoreRC.insert( SDB_DMS_CS_NOTEXIST ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &ignoreRC, NULL,
                        &pCtxForData, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback-phase1 command[%s, target:%s] on "
                 "data group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

      if ( pCtxForData )
      {
         rc = _processContext( cb, &pCtxForData, -1 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rollback-phase2 command[%s, targe:%s] on "
                    "data group failed, rc: %d", getName(),
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }
      }

      _pResource->removeCataInfo( pArgs->_targetName.c_str() ) ;

   done :
      if ( pCtxForData )
      {
         pmdKRCB *pKrcb = pmdGetKRCB();
         _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
         pRtncb->contextDelete ( pCtxForData->contextID(), cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_CREATECL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropCollection,
                                      CMD_NAME_DROP_COLLECTION,
                                      FALSE ) ;
   _coordCMDDropCollection::_coordCMDDropCollection()
   {
   }

   _coordCMDDropCollection::~_coordCMDDropCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_PARSEMSG, "_coordCMDDropCollection::_parseMsg" )
   INT32 _coordCMDDropCollection::_parseMsg ( MsgHeader *pMsg,
                                              coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCL_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( dmsCheckFullCLName( pArgs->_targetName.c_str() ) )
         {
            PD_LOG( PDERROR, "Collection name is invalid[%s]",
                    pArgs->_targetName.c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pArgs->_ignoreRCList.insert( SDB_DMS_NOTEXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDDropCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DROP_COLLECTION_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDDropCollection::_releaseCataMsg( CHAR *pMsgBuf,
                                                  INT32 bufSize,
                                                  pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_GENDATAMSG, "_coordCMDDropCollection::_generateDataMsg" )
   INT32 _coordCMDDropCollection::_generateDataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     const vector<BSONObj> &cataObjs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCL_GENDATAMSG ) ;

      rc = _coordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                  cataObjs, ppMsgBuf,
                                                  pBufSize ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( cataObjs.size() > 0 )
      {
         try
         {
            CoordCataInfoPtr cataPtr ;
            BSONObj objCata ;
            BSONElement beCollection = cataObjs[0].getField( CAT_COLLECTION ) ;
            if ( Object == beCollection.type() )
            {
               objCata = beCollection.embeddedObject() ;
               PD_LOG( PDDEBUG, "Updating catalog info of collection [%s]",
                       pArgs->_targetName.c_str() ) ;
               rc = coordInitCataPtrFromObj( objCata, cataPtr ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Init catalog info from obj[%s] failed, "
                          "collection:%s, rc: %d", pArgs->_targetName.c_str(),
                          objCata.toString().c_str(), rc ) ;
                  goto error ;
               }
               _pResource->addCataInfo( cataPtr ) ;
               ((MsgOpQuery*)(*ppMsgBuf))->version = cataPtr->getVersion() ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Occur exception when parse catalog "
                     "object info: %s", e.what() ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( COORD_DROPCL_GENDATAMSG, rc ) ;
      return rc ;
   error :
      if ( *ppMsgBuf )
      {
         _coordDataCMD3Phase::_releaseDataMsg( *ppMsgBuf, *pBufSize, cb ) ;
         *ppMsgBuf = NULL ;
         *pBufSize = 0 ;
      }
      goto done ;
   }

   void _coordCMDDropCollection::_releaseDataMsg( CHAR *pMsgBuf,
                                                  INT32 bufSize,
                                                  pmdEDUCB *cb )
   {
      _coordDataCMD3Phase::_releaseDataMsg( pMsgBuf, bufSize, cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_DOCOMPLETE, "_coordCMDDropCollection::_doComplete" )
   INT32 _coordCMDDropCollection::_doComplete ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                coordCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( COORD_DROPCL_DOCOMPLETE) ;

      _pResource->removeCataInfoWithMain( pArgs->_targetName.c_str() ) ;

      PD_TRACE_EXIT ( COORD_DROPCL_DOCOMPLETE ) ;
      return SDB_OK ;
   }

   /*
      _coordCMDAlterCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterCollection,
                                      CMD_NAME_ALTER_COLLECTION,
                                      FALSE ) ;
   _coordCMDAlterCollection::_coordCMDAlterCollection()
   {
   }

   _coordCMDAlterCollection::~_coordCMDAlterCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL_PARSEMSG, "_coordCMDAlterCollection::_parseMsg" )
   INT32 _coordCMDAlterCollection::_parseMsg ( MsgHeader *pMsg,
                                               coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERCL_PARSEMSG ) ;

      try
      {
         BOOLEAN isOldAlterCMD = FALSE ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get failed[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_boQuery.getField( FIELD_NAME_VERSION ).eoo() )
         {
            isOldAlterCMD = TRUE ;
         }

         PD_LOG ( PDDEBUG, "Alter collection command is %s",
                  isOldAlterCMD ? "old" : "new" ) ;

         if ( isOldAlterCMD )
         {
            pArgs->_ignoreRCList.insert( SDB_MAIN_CL_OP_ERR ) ;
            pArgs->_ignoreRCList.insert( SDB_CLS_COORD_NODE_CAT_VER_OLD ) ;
         }
         else
         {
            pArgs->_ignoreRCList.insert( SDB_IXM_REDEF ) ;
            pArgs->_ignoreRCList.insert( SDB_IXM_NOTEXIST ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_ALTERCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDAlterCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                      pmdEDUCB *cb,
                                                      coordCMDArguments *pArgs,
                                                      CHAR **ppMsgBuf,
                                                      INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_ALTER_COLLECTION_REQ ;
      *ppMsgBuf = ( CHAR* )pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDAlterCollection::_releaseCataMsg( CHAR *pMsgBuf,
                                                   INT32 bufSize,
                                                   pmdEDUCB *cb )
   {
   }

   /*
      _coordCMDLinkCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDLinkCollection,
                                      CMD_NAME_LINK_CL,
                                      FALSE ) ;
   _coordCMDLinkCollection::_coordCMDLinkCollection()
   {
   }

   _coordCMDLinkCollection::~_coordCMDLinkCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_PARSEMSG, "_coordCMDLinkCollection::_parseMsg" )
   INT32 _coordCMDLinkCollection::_parseMsg ( MsgHeader *pMsg,
                                              coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_PARSEMSG ) ;

      try
      {
         BSONObj lowBound, upBound ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_SUBCL_NAME,
                                      _subCLName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_SUBCL_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( _subCLName.empty() )
         {
            PD_LOG( PDERROR, "Sub collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetObjElement( pArgs->_boQuery, CAT_LOWBOUND_NAME,
                                lowBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_LOWBOUND_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetObjElement( pArgs->_boQuery, CAT_UPBOUND_NAME,
                                upBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_UPBOUND_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDLinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_LINK_CL_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDLinkCollection::_releaseCataMsg( CHAR *pMsgBuf,
                                                  INT32 bufSize,
                                                  pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_GENROLLBACKMSG, "_coordCMDLinkCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDLinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                             pmdEDUCB *cb,
                                                             coordCMDArguments *pArgs,
                                                             CHAR **ppMsgBuf,
                                                             INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_GENROLLBACKMSG ) ;

      rc = msgBuildUnlinkCLMsg( ppMsgBuf, pBufSize,
                                pArgs->_targetName.c_str(),
                                _subCLName.c_str(), 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message failed on command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDLinkCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                          INT32 bufSize,
                                                          pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_ROLLBACKONDATA, "_coordCMDLinkCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDLinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         coordCMDArguments *pArgs,
                                                         const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_ROLLBACKONDATA ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback command[%s, target:%s, sub:%s] on data "
                 "group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(), _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDUnlinkCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDUnlinkCollection,
                                      CMD_NAME_UNLINK_CL,
                                      FALSE ) ;
   _coordCMDUnlinkCollection::_coordCMDUnlinkCollection()
   {
   }

   _coordCMDUnlinkCollection::~_coordCMDUnlinkCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_PARSEMSG, "_coordCMDUnlinkCollection::_parseMsg" )
   INT32 _coordCMDUnlinkCollection::_parseMsg ( MsgHeader *pMsg,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_SUBCL_NAME,
                                      _subCLName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_SUBCL_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( _subCLName.empty() )
         {
            PD_LOG( PDERROR, "Sub collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDUnlinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       coordCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_UNLINK_CL_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDUnlinkCollection::_releaseCataMsg( CHAR *pMsgBuf,
                                                    INT32 bufSize,
                                                    pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_GENROLLBACKMSG, "_coordCMDUnlinkCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDUnlinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                               pmdEDUCB *cb,
                                                               coordCMDArguments *pArgs,
                                                               CHAR **ppMsgBuf,
                                                               INT32 *pBufSize  )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_GENROLLBACKMSG ) ;

      rc = msgBuildLinkCLMsg( ppMsgBuf, pBufSize,
                              pArgs->_targetName.c_str(),
                              _subCLName.c_str(),
                              NULL, NULL, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message on "
                 "command[%s, target:%s, sub:%s] failed, rc: %d",
                 getName(), pArgs->_targetName.c_str(),
                 _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDUnlinkCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                            INT32 bufSize,
                                                            pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_ROLLBACKONDATA, "_coordCMDUnlinkCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDUnlinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                           pmdEDUCB *cb,
                                                           coordCMDArguments *pArgs,
                                                           const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_ROLLBACKONDATA ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback command[%s, target:%s, sub:%s] on data "
                 "group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(),
                 _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDSplit implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSplit,
                                      CMD_NAME_SPLIT,
                                      FALSE ) ;
   _coordCMDSplit::_coordCMDSplit()
   {
      _async = FALSE ;
      _percent = 0.0 ;
   }

   _coordCMDSplit::~_coordCMDSplit()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETCOUNT, "_coordCMDSplit::_getCLCount" )
   INT32 _coordCMDSplit::_getCLCount( const CHAR *clFullName,
                                      const CoordGroupList &groupList,
                                      pmdEDUCB *cb,
                                      UINT64 &count,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_SPLIT_GETCOUNT ) ;

      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      rtnContextCoord *pContext = NULL ;
      BSONObj collectionObj ;
      BSONObj dummy ;
      rtnContextBuf buffObj ;

      CHAR *pMsg = NULL ;
      INT32 bufSize = 0 ;

      count = 0 ;

      collectionObj = BSON( FIELD_NAME_COLLECTION << clFullName ) ;
      rtnQueryOptions queryOption( dummy, dummy, dummy, collectionObj,
                                   CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                                   0, 1, FLG_QUERY_WITH_RETURNDATA ) ;

      rc = queryOption.toQueryMsg( &pMsg, bufSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build get count message failed in command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      rc = queryOnCL( (MsgHeader*)pMsg, cb, clFullName, &pContext,
                      FALSE, &groupList, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get count from source node failed, collection:%s, "
                 "rc: %d", clFullName, rc ) ;
         goto error ;
      }

      rc = pContext->getMore( -1, buffObj, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get more from context[%lld] failed, rc: %d",
                 pContext->contextID(), rc ) ;
         goto error ;
      }
      else
      {
         BSONObj countObj ( buffObj.data() ) ;
         BSONElement beTotal = countObj.getField( FIELD_NAME_TOTAL ) ;
         if ( !beTotal.isNumber() )
         {
            PD_LOG( PDERROR, "Parse get count result[%s] failed, "
                    "the field[%s] is not number",
                    countObj.toString().c_str(), FIELD_NAME_TOTAL ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         count = beTotal.numberLong() ;
      }

   done :
      if ( pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pRtncb->contextDelete ( contextID, cb ) ;
      }
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_SPLIT_GETCOUNT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitPrepare( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CoordGroupList srcGrpLst ;

      MsgOpQuery *pSplitReq            = (MsgOpQuery *)pMsg ;
      pSplitReq->header.opCode         = MSG_CAT_SPLIT_PREPARE_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, &srcGrpLst, NULL, TRUE,
                                NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Split prepare on catalog failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = msgExtractQuery ( (CHAR*)pMsg, NULL, NULL,
                             NULL, NULL, &pQuery,
                             NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract split message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj obj( pQuery ) ;
         rc = _splitParamCheck( obj, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _makeSplitRange( cb, srcGrpLst, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitParamCheck( const BSONObj &obj,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      BSONElement ele ;

      ele = obj.getField ( CAT_COLLECTION_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Collection name is invalid in object[%s]",
                 obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _clName = ele.valuestr() ;

      _eleSplitQuery = obj.getField ( CAT_SPLITQUERY_NAME ) ;

      ele = obj.getField ( CAT_SOURCE_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                 CAT_SOURCE_NAME, obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = catGroupNameValidate ( ele.valuestr() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Source group name[%s] is invalid, rc: %d",
                 ele.valuestr(), rc ) ;
         goto error ;
      }
      _srcGroup = ele.valuestr() ;

      ele = obj.getField ( CAT_TARGET_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                 CAT_TARGET_NAME, obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = catGroupNameValidate ( ele.valuestr() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Target group name[%s] is invalid, rc: %d",
                 ele.valuestr(), rc ) ;
         goto error ;
      }
      _dstGroup = ele.valuestr() ;

      ele = obj.getField ( FIELD_NAME_ASYNC ) ;
      if ( Bool == ele.type() )
      {
         _async = ele.Bool() ? TRUE : FALSE ;
      }
      else if ( !ele.eoo() )
      {
         PD_LOG( PDERROR, "Field[%s] type[%d] error", FIELD_NAME_ASYNC,
                 ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ele = obj.getField( CAT_SPLITPERCENT_NAME ) ;
      _percent = ele.numberDouble() ;

      if ( !_eleSplitQuery.eoo() )
      {
         if ( Object != _eleSplitQuery.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                    CAT_SPLITQUERY_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _eleSplitEndQuery = obj.getField ( CAT_SPLITENDQUERY_NAME ) ;
         if ( !_eleSplitEndQuery.eoo() )
         {
            if ( Object != _eleSplitQuery.type() )
            {
               PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                       CAT_SPLITENDQUERY_NAME, obj.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      else
      {
         if ( _percent <= 0.0 || _percent > 100.0 )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                    CAT_SPLITPERCENT_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      rc = _cataSel.bind( _pResource, _clName.c_str(), cb, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", _clName.c_str(), rc ) ;
         goto error ;
      }

      _cataSel.getCataPtr()->getShardingKey( _shardkingKey ) ;
      if ( !_cataSel.getCataPtr()->isSharded() ||
           _shardkingKey.isEmpty() )
      {
         PD_LOG( PDERROR, "Collection[%s] is not shared", _clName.c_str() ) ;
         rc = SDB_COLLECTION_NOTSHARD ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_makeSplitRange( pmdEDUCB *cb,
                                          const CoordGroupList &srcGrpLst,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      clsCatalogSet *pSet = _cataSel.getCataPtr()->getCatalogSet() ;

      if ( pSet->isHashSharding() )
      {
         if ( !_eleSplitQuery.eoo() )
         {
            BSONObj tmpStart = _eleSplitQuery.embeddedObject() ;
            BSONObjBuilder tmpStartBuilder ;
            tmpStartBuilder.appendElementsWithoutName( tmpStart ) ;
            _lowBound = tmpStartBuilder.obj() ;

            if ( !_eleSplitEndQuery.eoo() )
            {
               BSONObj tmpEnd = _eleSplitEndQuery.embeddedObject() ;
               BSONObjBuilder tmpEndBuilder ;
               tmpEndBuilder.appendElementsWithoutName( tmpEnd ) ;
               _upBound = tmpEndBuilder.obj() ;
            }
         }
      }
      else
      {
         if ( _eleSplitQuery.eoo() )
         {
            rc = _getBoundByPercent( _clName.c_str(), _percent,
                                     _cataSel.getCataPtr(), srcGrpLst,
                                     cb, _lowBound, _upBound,
                                     buf ) ;
         }
         else
         {
            rc = _getBoundByCondition( _clName.c_str(),
                                       _eleSplitQuery.embeddedObject(),
                                       _eleSplitEndQuery.eoo() ?
                                       BSONObj() :
                                       _eleSplitEndQuery.embeddedObject(),
                                       srcGrpLst, cb,
                                       _lowBound, _upBound,
                                       buf ) ;
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Make bound failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitReady( pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf,
                                      UINT64 &taskID,
                                      BSONObj &taskInfoObj )
   {
      INT32 rc = SDB_OK ;
      CHAR *pReadyMsg = NULL ;
      INT32 readyMsgSize = 0 ;
      MsgOpQuery *pSplitReadyMsg = NULL ;
      vector< BSONObj > vecObjs ;
      CoordGroupList dstGrpLst ;

      try
      {
         BSONObj boSend ;
         boSend = BSON ( CAT_COLLECTION_NAME << _clName<<
                         CAT_SOURCE_NAME << _srcGroup <<
                         CAT_TARGET_NAME << _dstGroup <<
                         CAT_SPLITPERCENT_NAME << _percent <<
                         CAT_SPLITVALUE_NAME << _lowBound <<
                         CAT_SPLITENDVALUE_NAME << _upBound ) ;
         taskInfoObj = boSend ;

         rc = msgBuildQueryMsg( &pReadyMsg, &readyMsgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                0, 0, 0, -1,
                                &boSend, NULL, NULL, NULL,
                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build split ready message failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pSplitReadyMsg = (MsgOpQuery *)pReadyMsg ;
      pSplitReadyMsg->header.opCode = MSG_CAT_SPLIT_READY_REQ ;
      pSplitReadyMsg->version = _cataSel.getCataPtr()->getVersion() ;

      rc = executeOnCataGroup ( (MsgHeader*)pReadyMsg, cb, &dstGrpLst,
                                &vecObjs, TRUE, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute split ready on catalog failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( vecObjs.empty() )
      {
         PD_LOG( PDERROR, "Failed to get task id from result msg" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         BSONObj resultObj = vecObjs[0] ;
         BSONElement ele = resultObj.getField( CAT_TASKID_NAME ) ;
         if ( !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Get taskid from split result object[%s] "
                    "failed", resultObj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         taskID = ele.numberLong() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc =  SDB_SYS ;
         goto error ;
      }

      pSplitReadyMsg->header.opCode = MSG_BS_QUERY_REQ ;
      pSplitReadyMsg->version = _cataSel.getCataPtr()->getVersion() ;

      rc = executeOnCL( (MsgHeader *)pSplitReadyMsg, cb, _clName.c_str(),
                        FALSE, &dstGrpLst, NULL, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute split on data node failed, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( pReadyMsg )
      {
         msgReleaseBuffer( pReadyMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitRollback( UINT64 taskID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      MsgOpQuery *pRollbackMsg = NULL ;
      rtnContextBuf buff ;

      try
      {
         BSONObj boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
         rc = msgBuildQueryMsg( &pMsg, &msgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                0, 0, 0, -1,
                                &boSend, NULL, NULL, NULL,
                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build split notify message failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pRollbackMsg = ( MsgOpQuery* )pMsg ;
      pRollbackMsg->header.opCode = MSG_CAT_SPLIT_CANCEL_REQ ;

      rc = executeOnCataGroup ( (MsgHeader*)pRollbackMsg, cb, TRUE,
                                NULL, NULL, &buff ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Execute split cancel on catalog failed, "
                 "rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitPost( UINT64 taskID,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      coordCommandFactory *pFactory = NULL ;
      coordOperator *pOperator = NULL ;
      rtnContextDump *pContext = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      if ( !_async )
      {
         try
         {
            BSONObj boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
            rc = msgBuildQueryMsg( &pMsg, &msgSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                   0, 0, 0, -1,
                                   &boSend, NULL, NULL, NULL,
                                   cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Build split notify message failed, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pFactory = coordGetFactory() ;
         rc = pFactory->create( CMD_NAME_WAITTASK, pOperator ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Create operator[%s] failed, rc: %d",
                    CMD_NAME_WAITTASK, rc ) ;
            rc = SDB_OK ;
            goto done ;
         }
         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOperator->getName(), rc ) ;
            goto error ;
         }
         rc = pOperator->execute( (MsgHeader *)pMsg, cb,
                                  contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Wait task[%lld] failed, rc: %d",
                    taskID, rc ) ;
            rc = SDB_OK ;
            goto done ;
         }
      }
      else // return taskid to client
      {
         rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                                 (rtnContext**)&pContext,
                                 contextID, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = pContext->open( BSONObj(), BSONObj(), 1, 0 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
            goto error ;
         }
         pContext->append( BSON( CAT_TASKID_NAME << (long long)taskID ) ) ;
      }

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_EXE, "_coordCMDSplit::execute" )
   INT32 _coordCMDSplit::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_SPLIT_EXE ) ;
      coordSessionPropSite *pSiteProp  = NULL ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      BSONObj taskInfoObj ;
      rtnInstanceOption instanceOption ;
      BOOLEAN replacedInstanceOption = FALSE ;

      contextID = -1 ;

      if ( cb->getRemoteSite() )
      {
         pmdRemoteSessionSite *pSite = NULL ;
         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pSiteProp = (coordSessionPropSite*)pSite->getUserData() ;
      }

      if ( NULL != pSiteProp && !pSiteProp->isMasterPreferred() )
      {
         instanceOption = pSiteProp->getInstanceOption() ;
         pSiteProp->setMasterPreferred() ;
         replacedInstanceOption = TRUE ;
      }

      rc = _splitPrepare( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _splitReady( cb, contextID, buf, taskID, taskInfoObj ) ;
      if ( rc )
      {
         if ( CLS_INVALID_TASKID != taskID )
         {
            _splitRollback( taskID, cb ) ;
         }
         goto error ;
      }

      rc = _splitPost( taskID, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( NULL != pSiteProp && replacedInstanceOption )
      {
         pSiteProp->setInstanceOption( instanceOption ) ;
      }
      if ( !_clName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_CL,
                           _clName.c_str(), rc, "Option:%s, TaskID:%llu",
                           taskInfoObj.toString().c_str(), taskID ) ;
      }
      PD_TRACE_EXITRC ( COORD_SPLIT_EXE, rc ) ;
      return rc ;
   error :
      if ( SDB_RTN_INVALID_HINT == rc )
      {
         rc = SDB_COORD_SPLIT_NO_SHDIDX ;
      }
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDRECORDONDATA, "_coordCMDSplit::_getBoundRecordOnData" )
   INT32 _coordCMDSplit::_getBoundRecordOnData( const CHAR *cl,
                                                const BSONObj &condition,
                                                const BSONObj &hint,
                                                const BSONObj &sort,
                                                INT32 flag,
                                                INT64 skip,
                                                const CoordGroupList &groupList,
                                                pmdEDUCB *cb,
                                                const BSONObj &shardingKey,
                                                BSONObj &record,
                                                rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDRECORDONDATA ) ;
      INT32 rc = SDB_OK ;
      rtnContextCoord *pContext = NULL ;
      BSONObj obj ;

      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;

      if ( !condition.okForStorage() )
      {
         PD_LOG( PDERROR, "Condition[%s] has invalid field name",
                 condition.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( condition.isEmpty() )
      {
         BSONObj empty ;
         rtnContextBuf buffObj ;
         rtnQueryOptions queryOption( condition, empty, sort, hint, cl,
                                      skip, 1, flag ) ;
         rc = queryOption.toQueryMsg( &pMsg, msgSize, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build query message failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = queryOnCL( (MsgHeader*)pMsg, cb, NULL, &pContext,
                         FALSE, &groupList, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query on data group failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = pContext->getMore( -1, buffObj, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            obj = BSONObj( buffObj.data() ) ;
         }
      }
      else
      {
         obj = condition ;
      }

      {
         PD_LOG( PDINFO, "Split found record: %s", obj.toString().c_str() ) ;

         ixmIndexKeyGen keyGen ( shardingKey ) ;
         BSONObjSet keys ;
         BSONObjSet::iterator keyIter ;
         rc = keyGen.getKeys ( obj, keys, NULL, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Gen key failed, rc: %d"OSS_NEWLINE
                    "record: %s"OSS_NEWLINE"keyDef: %s", rc,
                    obj.toString().c_str(),
                    shardingKey.toString().c_str() ) ;
            goto error ;
         }
         if ( 1 != keys.size() )
         {
            PD_LOG( PDERROR, "There must be a single key generate for "
                    "sharding"OSS_NEWLINE"record: %s"OSS_NEWLINE"keyDef: %s",
                    obj.toString().c_str(),
                    shardingKey.toString().c_str() ) ;
            rc = SDB_INVALID_SHARDINGKEY ;
            goto error ;
         }

         keyIter = keys.begin() ;
         record = (*keyIter).getOwned() ;

         /*
         {
            BSONObjIterator iter ( record ) ;
            while ( iter.more () )
            {
               BSONElement e = iter.next () ;
               PD_CHECK ( e.type() != Undefined, SDB_CLS_BAD_SPLIT_KEY,
                          error, PDERROR, "The split record does not contains "
                          "a valid key\nRecord: %s\nShardingKey: %s\n"
                          "SplitKey: %s", obj.toString().c_str(),
                          shardingKey.toString().c_str(),
                          record.toString().c_str() ) ;
            }
         }
         */

        PD_LOG ( PDINFO, "Split found key: %s", record.toString().c_str() ) ;
     }

   done:
      if ( NULL != pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
      }
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDRECORDONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDBYCOND, "_coordCMDSplit::_getBoundByCondition" )
   INT32 _coordCMDSplit::_getBoundByCondition( const CHAR *cl,
                                               const BSONObj &begin,
                                               const BSONObj &end,
                                               const CoordGroupList &groupList,
                                               pmdEDUCB *cb,
                                               BSONObj &lowBound,
                                               BSONObj &upBound,
                                               rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDBYCOND ) ;
      INT32 rc = SDB_OK ;

      rc = _getBoundRecordOnData( cl, begin, BSONObj(), BSONObj(),
                                  FLG_QUERY_WITH_RETURNDATA, 0,
                                  groupList, cb, _shardkingKey,
                                  lowBound, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get lowbound failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( !end.isEmpty() )
      {
         rc = _getBoundRecordOnData( cl, end, BSONObj(),BSONObj(),
                                     FLG_QUERY_WITH_RETURNDATA, 0,
                                     groupList, cb, _shardkingKey,
                                     upBound, buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Get upbound failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDBYCOND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDBYPERCENT, "_coordCMDSplit::_getBoundByPercent" )
   INT32 _coordCMDSplit::_getBoundByPercent( const CHAR *cl,
                                             FLOAT64 percent,
                                             const CoordCataInfoPtr &cataInfo,
                                             const CoordGroupList &groupList,
                                             pmdEDUCB *cb,
                                             BSONObj &lowBound,
                                             BSONObj &upBound,
                                             rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDBYPERCENT ) ;
      INT32 rc = SDB_OK ;

      if ( 100.0 - percent < OSS_EPSILON )
      {
         rc = cataInfo->getGroupLowBound( groupList.begin()->second,
                                          lowBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get group[%d]'s low bound failed, rc: %d",
                    groupList.begin()->second, rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT64 totalCount    = 0 ;
         INT64 skipCount      = 0 ;
         INT32 flag = FLG_QUERY_FORCE_IDX_BY_SORT | FLG_QUERY_WITH_RETURNDATA ;
         BSONObj hint = BSON( "" << "" ) ;

         while ( TRUE )
         {
            rc = _getCLCount( cl, groupList, cb, totalCount, buf ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get count from collection[%s] failed, rc: %d",
                       cl, rc ) ;
               goto error ;
            }

            if ( 0 == totalCount )
            {
               rc = SDB_DMS_EMPTY_COLLECTION ;
               PD_LOG( PDDEBUG, "Collection[%s] is empty", cl ) ;
               break ;
            }

            skipCount = (INT64)(totalCount * ( ( 100 - percent ) / 100 ) ) ;

            rc = _getBoundRecordOnData( cl, BSONObj(), hint, _shardkingKey,
                                        flag, skipCount, groupList,
                                        cb, _shardkingKey, lowBound, buf ) ;
            if ( SDB_DMS_EOC == rc )
            {
               continue ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get bound from data, rc: %d",
                       rc ) ;
               goto error ;
            }
            else
            {
               break ;
            }
         }
      }

      upBound = BSONObj() ;
   done:
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDBYPERCENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDCreateIndex implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateIndex,
                                      CMD_NAME_CREATE_INDEX,
                                      FALSE ) ;
   _coordCMDCreateIndex::_coordCMDCreateIndex()
   {
   }

   _coordCMDCreateIndex::~_coordCMDCreateIndex()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CRTIDX_PARSEMSG, "_coordCMDCreateIndex::_parseMsg" )
   INT32 _coordCMDCreateIndex::_parseMsg ( MsgHeader *pMsg,
                                           coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CRTIDX_PARSEMSG ) ;

      try
      {
         BSONObj boIndex ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetObjElement( pArgs->_boQuery, FIELD_NAME_INDEX,
                                boIndex ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_INDEX, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( boIndex, IXM_FIELD_NAME_NAME,
                                      _indexName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    IXM_FIELD_NAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( _indexName.empty() )
         {
            PD_LOG( PDERROR, "Index name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CRTIDX_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDCreateIndex::_generateCataMsg( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_CREATE_IDX_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;
      return SDB_OK ;
   }

   void _coordCMDCreateIndex::_releaseCataMsg( CHAR *pMsgBuf,
                                               INT32 bufSize,
                                               pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CRTIDX_GENROLLBACKMSG, "_coordCMDCreateIndex::_generateRollbackDataMsg" )
   INT32 _coordCMDCreateIndex::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                          pmdEDUCB *cb,
                                                          coordCMDArguments *pArgs,
                                                          CHAR **ppMsgBuf,
                                                          INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CRTIDX_GENROLLBACKMSG ) ;

      rc = msgBuildDropIndexMsg( ppMsgBuf, pBufSize,
                                 pArgs->_targetName.c_str(),
                                 _indexName.c_str(), 0,
                                 cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message on "
                 "command[%s, target:%s, IndexName:%s] failed, rc: %d",
                 getName(), pArgs->_targetName.c_str(),
                 _indexName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CRTIDX_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDCreateIndex::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                       INT32 bufSize,
                                                       pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CRTIDX_ROLLBACKONDATA, "_coordCMDCreateIndex::_rollbackOnDataGroup" )
   INT32 _coordCMDCreateIndex::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                      pmdEDUCB *cb,
                                                      coordCMDArguments *pArgs,
                                                      const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CRTIDX_ROLLBACKONDATA ) ;

      CoordCataInfoPtr cataPtr ;
      SET_RC ignoreRC ;

      rc = _pResource->getOrUpdateCataInfo( pArgs->_targetName.c_str(),
                                            cataPtr, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed on command[%s, IndexName:%s], rc: %d",
                 pArgs->_targetName.c_str(), getName(),
                 _indexName.c_str(), rc ) ;
         goto error ;
      }

      if ( cataPtr->isMainCL() )
      {
         PD_LOG( PDWARNING, "Main collection[%s] create index[%s] failed "
                 "but not rollback", pArgs->_targetName.c_str(),
                 _indexName.c_str() ) ;
         goto done ;
      }

      ignoreRC.insert( SDB_IXM_NOTEXIST ) ;
      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        FALSE, &groupLst, &ignoreRC, NULL,
                        NULL, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback command[%s, target:%s, Index:%s] failed, "
                 "rc: %d", getName(), pArgs->_targetName.c_str(),
                 _indexName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CRTIDX_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropIndex define
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropIndex,
                                      CMD_NAME_DROP_INDEX,
                                      FALSE ) ;
   _coordCMDDropIndex::_coordCMDDropIndex()
   {
   }

   _coordCMDDropIndex::~_coordCMDDropIndex()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPIDX_PARSEMSG, "_coordCMDDropIndex::_parseMsg" )
   INT32 _coordCMDDropIndex::_parseMsg ( MsgHeader *pMsg,
                                         coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPIDX_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPIDX_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDDropIndex::_generateCataMsg( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               coordCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DROP_IDX_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordCMDDropIndex::_releaseCataMsg( CHAR *pMsgBuf,
                                             INT32 bufSize,
                                             pmdEDUCB *cb )
   {
   }

   /*
      _coordCMDPop implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDPop,
                                      CMD_NAME_POP,
                                      FALSE ) ;
   _coordCMDPop::_coordCMDPop()
   {
   }

   _coordCMDPop::~_coordCMDPop()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_POP_EXE, "_coordCMDPop::execute" )
   INT32 _coordCMDPop::execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_POP_EXE ) ;
      CHAR *option = NULL;
      BSONObj boQuery ;
      const CHAR *fullName = NULL ;

      rc = msgExtractQuery( ( CHAR * )pMsg, NULL, NULL,
                            NULL, NULL, &option, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         boQuery = BSONObj( option );
         BSONElement e = boQuery.getField( FIELD_NAME_COLLECTION );
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid pop msg:%s",
                    boQuery.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fullName = e.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

      rc = executeOnCL( pMsg, cb, fullName, FALSE, NULL, NULL,
                        NULL, NULL, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate cl:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( COORD_POP_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

