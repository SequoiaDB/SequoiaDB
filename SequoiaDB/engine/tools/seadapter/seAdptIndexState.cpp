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

   Source File Name = seAdptIndexSession.cpp

   Descriptive Name = Index session state.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/24/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptIndexState.hpp"
#include "seAdptIndexSession.hpp"
#include "utilESUtil.hpp"
#include "seAdptMgr.hpp"
#include "ixmIndexKey.hpp"

namespace seadapter
{
   // Basic operation time limit.
   const UINT16 SEADPT_BASIC_OP_TIMEOUT = 5000 ;

   // Max retry times for indexing operations.
   const UINT16 SEADPT_INDEX_MAX_RETRY = 60 ;

   const CHAR* seAdptGetIndexerStateDesp( SEADPT_INDEXER_STATE state )
   {
      switch ( state )
      {
      case CONSULT:
         return "Consult" ;
      case FULL_INDEX:
         return "FullIndex" ;
      case INCREMENT_INDEX:
         return "IncrementalIndex" ;
      default:
         break ;
      }
      return "Unknow" ;
   }

   BEGIN_OBJ_MSG_MAP( _seAdptIndexerState, _pmdObjBase )
      ON_MSG( MSG_BS_QUERY_RES, handleQueryRes )
      ON_MSG( MSG_BS_GETMORE_RES, handleGetmoreRes )
      ON_MSG( MSG_CAT_QUERY_CATALOG_RSP, handleCatalogRes )
      ON_MSG( MSG_BS_KILL_CONTEXT_RES, handleKillCtxRes )
   END_OBJ_MSG_MAP()
   _seAdptIndexerState::_seAdptIndexerState( _seAdptIndexSession *session )
   : _session( session ),
     _begin( TRUE ),
     _timeout( 0 ),
     _retryTimes( 0 ),
     _rebuild( FALSE )
   {
   }

   INT32 _seAdptIndexerState::onTimer( UINT32 interval )
   {
      INT32 rc = SDB_OK ;

      if ( _idxDef.isEmpty() )
      {
         // Let's check if the index exists or not.
         seIdxMetaContext *imContext = _session->idxMetaContext() ;
         INT32 rcTmp = imContext->metaLock( SHARED ) ;
         if ( SDB_RTN_INDEX_NOTEXIST == rcTmp )
         {
            rc = rcTmp ;
            goto error ;
         }
         else if ( SDB_OK == rcTmp )
         {
            _idxDef = imContext->meta()->getIdxDef().copy() ;
            imContext->metaUnlock() ;
            if ( _idxDef.isEmpty() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "No index definition found in the meta" ) ;
               goto error ;
            }
         }
      }

      // No wait for the first round of all states.
      if ( !_begin )
      {
         _timeout += interval ;
         if ( _timeout < SEADPT_BASIC_OP_TIMEOUT )
         {
            goto done ;
         }

         ++_retryTimes ;
         if ( _retryTimes > SEADPT_INDEX_MAX_RETRY )
         {
            // Incase of timeout, let's check if the index exists or not.
            seIdxMetaContext *imContext = _session->idxMetaContext() ;
            INT32 rcTmp = imContext->metaLock( SHARED ) ;
            if ( SDB_RTN_INDEX_NOTEXIST == rcTmp )
            {
               rc = rcTmp ;
               goto error ;
            }
            else if ( SDB_OK == rcTmp )
            {
               imContext->metaUnlock() ;
            }
            rc = SDB_TIMEOUT ;
            PD_LOG( PDERROR, "In stage[%s:%s] operation timeout",
                    seAdptGetIndexerStateDesp( type() ), _getStepDesp() ) ;
            goto error ;
         }

         _timeout = 0 ;
      }

      rc = _processTimeout() ;
      PD_RC_CHECK( rc, PDERROR, "Process timeout failed[%d]", rc ) ;
      if ( _begin )
      {
         _begin = FALSE ;
      }

   done:
      return rc ;
   error:
      if ( SDB_RTN_INDEX_NOTEXIST == rc )
      {
         PD_LOG( PDEVENT, "Index meta not found. "
                          "Clean index on search engine" ) ;
         INT32 rcTmp = _cleanSearchEngine() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                    _session->getESIdxName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32 _seAdptIndexerState::handleQueryRes( NET_HANDLE handle,
                                              MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = SDB_OK ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> resultSet ;

      // If it's obsolete message, just ignore.
      if ( msg->requestID != _session->currentRequestID() )
      {
         rc = _cleanObsoleteContext( handle, msg ) ;
         PD_RC_CHECK( rc, PDERROR, "Clean obsolete context failed[%d]", rc ) ;
         PD_LOG( PDWARNING, "Request id in message[%llu] dose not match the "
                            "current id[%llu]. Ignore the message",
                 msg->requestID, _session->currentRequestID() ) ;
         goto done ;
      }

      _retryTimes = 0 ;
      rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                            &numReturned, resultSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply message failed[%d]", rc ) ;

      rc = flag ;

      // If original CL/CS or capped CS/CL is dropped, remove the index on
      // search engine.
      if ( SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc
           || SDB_DMS_CS_DELETING == rc )
      {
         PD_LOG( PDEVENT, "Index on data node is dropped. Clean index[%s] on "
                          "search engine", _session->getESIdxName() ) ;
         {
            INT32 rcTmp = _cleanSearchEngine() ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                       _session->getESIdxName(), rcTmp ) ;
            }
         }
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "In stage[%s:%s] query request failed[%d]",
                 seAdptGetIndexerStateDesp( type() ), _getStepDesp(),  rc ) ;
         goto error ;
      }

      rc = _processQueryRes( contextID, startFrom, numReturned, resultSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Process query reply failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      if ( SDB_RTN_INDEX_NOTEXIST == rc )
      {
         PD_LOG( PDEVENT, "Index meta not found. "
                          "Clean index on search engine" ) ;
         INT32 rcTmp = _cleanSearchEngine() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                    _session->getESIdxName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32
   _seAdptIndexerState::handleGetmoreRes( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0;
      INT64 contextID = 0;
      INT32 startFrom = 0;
      INT32 numReturned = 0;
      vector<BSONObj> resultSet;

      // If it's obsolete message, just ignore.
      if ( msg->requestID != _session->currentRequestID() )
      {
         rc = _cleanObsoleteContext( handle, msg ) ;
         PD_RC_CHECK( rc, PDERROR, "Clean obsolete context failed[%d]", rc ) ;
         PD_LOG( PDWARNING, "Request id in message[%llu] dose not match the "
                            "current id[%llu]. Ignore the message",
                 msg->requestID, _session->currentRequestID() ) ;
         goto done ;
      }

      _retryTimes = 0 ;

      rc = msgExtractReply( ( CHAR * )msg, &flag, &contextID, &startFrom,
                            &numReturned, resultSet );
      PD_RC_CHECK( rc, PDERROR, "Extract getmore reply message failed[%d]",
                   rc );

      rc = flag ;

      // If original CL/CS or capped CS/CL is dropped, remove the index on
      // search engine.
      if ( SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc
           || SDB_DMS_CS_DELETING == rc )
      {
         PD_LOG( PDEVENT, "Index on data node is dropped. Clean index[%s] on "
                          "search engine", _session->getESIdxName() ) ;
         {
            INT32 rcTmp = _cleanSearchEngine() ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                       _session->getESIdxName(), rcTmp ) ;
            }
         }
         goto error ;
      }

      rc = _processGetmoreRes( flag, contextID, startFrom,
                               numReturned, resultSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Process getmore reply failed[%d]", rc ) ;

      if ( _rebuild )
      {
         // Ready to recreate the index, kill the current context.
         rc = _cleanObsoleteContext( handle, msg ) ;
         PD_RC_CHECK( rc, PDERROR, "Clean the current contex failed[%d]", rc ) ;
         rc = _cleanSearchEngine() ;
         PD_RC_CHECK( rc, PDERROR, "Clean index on search engine failed[%d]",
                      rc ) ;
         _session->triggerStateTransition( FULL_INDEX ) ;
      }

   done:
      return rc ;
   error:
      if ( SDB_RTN_INDEX_NOTEXIST == rc )
      {
         PD_LOG( PDEVENT, "Index meta not found. "
                          "Clean index on search engine" ) ;
         INT32 rcTmp = _cleanSearchEngine() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                    _session->getESIdxName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32 _seAdptIndexerState::handleCatalogRes( NET_HANDLE handle,
                                                MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = SDB_OK ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> resultSet ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;

      // If it's obsolete message, just ignore.
      if ( msg->requestID != _session->currentRequestID() )
      {
         rc = _cleanObsoleteContext( handle, msg ) ;
         PD_RC_CHECK( rc, PDERROR, "Clean obsolete context failed[%d]", rc ) ;
         PD_LOG( PDWARNING, "Request id in message[%llu] dose not match the "
                            "current id[%llu]. Ignore the message",
                 msg->requestID, _session->currentRequestID() ) ;
         goto done ;
      }

      _retryTimes = 0 ;

      if ( !dbAssist->cataNetHandleValid() )
      {
         dbAssist->setCataNetHandle( handle ) ;
      }

      rc = msgExtractReply( (CHAR *)msg, &flag, &contextID, &startFrom,
                            &numReturned, resultSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract catalogue reply message failed[%d]",
                   rc ) ;

      rc = flag ;

      // If original CL/CS or capped CS/CL is dropped, remove the index on
      // search engine.
      if ( SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc
           || SDB_DMS_CS_DELETING == rc )
      {
         PD_LOG( PDEVENT, "Original collection is dropped. Clean index[%s] on "
                          "search engine", _session->getESIdxName() ) ;
         {
            INT32 rcTmp = _cleanSearchEngine() ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                       _session->getESIdxName(), rcTmp ) ;
            }
         }
         goto error ;
      }
      else if ( SDB_CLS_NOT_PRIMARY == rc || SDB_NET_CANNOT_CONNECT == rc )
      {
         // startFrom will contain the primary node id, if any.
         if ( INVALID_NODE_ID == startFrom && ( SDB_NET_CANNOT_CONNECT != rc ) )
         {
            PD_LOG( PDERROR, "No primary in catalogue group now. Wait for 1 "
                             "second and retry..." ) ;
            ossSleep( OSS_ONE_SEC ) ;
         }
         else
         {
            rc = dbAssist->changeCataPrimary( startFrom ) ;
            PD_RC_CHECK( rc, PDERROR, "Set primary of catalog group failed[%d]",
                         rc ) ;
         }
         // Stay in state of QUERY_CL_VERSION. Wait for next query of the
         // collection version.
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "In stage[%s:%s] query request failed[%d]",
                 seAdptGetIndexerStateDesp( type() ), _getStepDesp(),  rc ) ;
         goto error ;
      }

      rc = _processCatalogRes( contextID, startFrom, numReturned, resultSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Process catalog reply failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      if ( SDB_RTN_INDEX_NOTEXIST == rc )
      {
         PD_LOG( PDEVENT, "Index meta not found. "
                          "Clean index on search engine" ) ;
         INT32 rcTmp = _cleanSearchEngine() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Clean index[%s] on search engine failed[%d]",
                    _session->getESIdxName(), rcTmp ) ;
         }
      }
      goto done ;
   }

   INT32 _seAdptIndexerState::handleKillCtxRes( NET_HANDLE handle,
                                                MsgHeader *msg )
   {
      _retryTimes = 0 ;
      return SDB_OK ;
   }

   INT32 _seAdptIndexerState::_processQueryRes( INT64 contextID,
                                                INT32 startFrom,
                                                INT32 numReturned,
                                                vector<BSONObj> &resultSet )
   {
      return SDB_OK ;
   }

   INT32 _seAdptIndexerState::_processGetmoreRes( INT32 flag, INT64 contextID,
                                                  INT32 startFrom,
                                                  INT32 numReturned,
                                                  vector<BSONObj> &resultSet )
   {
      return SDB_OK ;
   }

   INT32 _seAdptIndexerState::_processCatalogRes( INT64 contextID,
                                                  INT32 startFrom,
                                                  INT32 numReturned,
                                                  vector<BSONObj> &resultSet )
   {
      return SDB_OK ;
   }

   INT32 _seAdptIndexerState::_updateProgress( UINT32 hashVal )
   {
      INT32 rc = SDB_OK ;
      const seIdxMetaContext *imContext = _session->idxMetaContext() ;
      seAdptSEAssist *seAssist = _session->seAssist() ;

      try
      {
         BSONObj progressObj =
               BSON( SEADPT_FIELD_NAME_CLUID << (INT64)imContext->getCLUID() <<
                     SEADPT_FIELD_NAME_CLLID << imContext->getCLLID() <<
                     SEADPT_FIELD_NAME_IDXLID << imContext->getIdxLID() <<
                     SEADPT_FIELD_NAME_LID <<
                        ( ( 0 == hashVal ) ? -1 : _session->getExpectLID() ) <<
                     SEADPT_FIELD_NAME_HASH << hashVal ) ;

         rc = seAssist->indexDocument(
               _session->getESIdxName(),
               _session->getESTypeName(),
               SEADPT_COMMIT_ID,
               progressObj.toString(false, true).c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Update progress in search engine "
                                   "failed[%d]. Progress information: %s",
                      rc, progressObj.toString(false, true).c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIndexerState::_getProgressInfo( BSONObj &progressInfo )
   {
      seAdptSEAssist *seAssist = _session->seAssist() ;

      return seAssist->getDocument( _session->getESIdxName(),
                                    _session->getESTypeName(),
                                    SEADPT_COMMIT_ID, progressInfo ) ;
   }

   INT32 _seAdptIndexerState::_validateProgress( const BSONObj &progressInfo,
                                                 BOOLEAN &valid )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN validate = FALSE ;
      const seIdxMetaContext *imContext = _session->idxMetaContext() ;
      try
      {
         validate =
               ( ( (UINT64)
                   progressInfo.getField( SEADPT_FIELD_NAME_CLUID ).numberLong()
                   == imContext->getCLUID() ) &&
                 ( (UINT32)progressInfo.getIntField( SEADPT_FIELD_NAME_CLLID )
                   == imContext->getCLLID() ) &&
                 ( (UINT32)progressInfo.getIntField( SEADPT_FIELD_NAME_IDXLID )
                   == imContext->getIdxLID() ) ) ;

         valid = validate ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIndexerState::_cleanObsoleteContext( NET_HANDLE handle,
                                                     MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      CHAR *killCtxMsg = NULL ;
      INT32 buffSize = 0 ;
      INT64 contextID = ((MsgOpReply *)msg)->contextID ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;

      rc = msgBuildKillContextsMsg( &killCtxMsg, &buffSize,
                                    _session->nextRequestID(),
                                    1, &contextID, _session->eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Build kill context[%lld] message failed[%d]",
                   rc ) ;
      ((MsgHeader *)killCtxMsg)->TID = _session->tid() ;
      rc = dbAssist->sendMsg( (const MsgHeader *)killCtxMsg, handle ) ;
      PD_RC_CHECK( rc, PDERROR, "Send kill context with net handle[%u] "
                                "failed[%d]",
                   handle, rc ) ;

   done:
      if ( killCtxMsg )
      {
         msgReleaseBuffer( killCtxMsg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIndexerState::_cleanSearchEngine()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN idxExist = FALSE ;
      seAdptSEAssist *es= _session->seAssist() ;
      const CHAR *idxName = _session->getESIdxName() ;

      rc = es->indexExist( idxName, idxExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Check index[%s] existence failed[%d]",
                   idxName, rc ) ;
      if ( idxExist )
      {
         rc = es->dropIndex( idxName ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop index[%s] on search engine failed[%d]",
                      idxName, rc) ;
         PD_LOG( PDEVENT, "Drop index[%s] on search engine successfully",
                 idxName ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   _seAdptConsultState::_seAdptConsultState( _seAdptIndexSession *session )
   : _seAdptIndexerState( session )
   {
   }

   _seAdptConsultState::~_seAdptConsultState()
   {
   }

   SEADPT_INDEXER_STATE _seAdptConsultState::type() const
   {
      return CONSULT ;
   }

   INT32 _seAdptConsultState::_processTimeout()
   {
      INT32 rc = SDB_OK ;
      rc = _consult() ;
      PD_RC_CHECK( rc, PDERROR, "Indexing consult failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptConsultState::_processQueryRes( INT64 contextID,
                                                INT32 startFrom,
                                                INT32 numReturned,
                                                vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == numReturned || !_progressMatch( resultSet[0] ) )
      {
         PD_LOG( PDERROR, "Record of expected progress[%s] is not found "
                          "on data node. Drop and recreate index again",
                 _progressInfo.toString(false, true).c_str() ) ;
         rc = _cleanSearchEngine() ;
         PD_RC_CHECK( rc, PDERROR, "Clean index[%s] on search engine "
                                   "failed[%d]",
                      _session->getESIdxName(), rc ) ;
         _session->triggerStateTransition( FULL_INDEX ) ;
      }
      else
      {
         _session->setExpectLID(
               _progressInfo.getField(SEADPT_FIELD_NAME_LID).numberLong() ) ;
         _session->triggerStateTransition( INCREMENT_INDEX ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_seAdptConsultState::_getStepDesp() const
   {
      return "Query min logical id" ;
   }

   INT32 _seAdptConsultState::_consult()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      const CHAR *esIdxName = _session->getESIdxName() ;

      // 1. Check if the index exists on search engine.
      rc = _session->seAssist()->indexExist( esIdxName,  exist ) ;
      PD_RC_CHECK( rc, PDERROR, "Check index[%s] existance on search engine "
                                "failed[%d]", esIdxName, rc ) ;

      if ( !exist )
      {
         PD_LOG( PDEVENT, "Index[%s] not found on search engine. "
                          "Start full indexing", esIdxName ) ;
         _session->triggerStateTransition( FULL_INDEX ) ;
         goto done ;
      }
      else
      {
         try
         {
            // 2. If index exists, get and validate the meta data.
            BOOLEAN valid = FALSE ;
            INT64 expectLID = SEADPT_INVALID_LID ;
            rc = _getProgressInfo( _progressInfo ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDEVENT, "Commit mark for index[%s] dose not exist. "
                                "The index on search engine will be re-created",
                       esIdxName ) ;
               rc = _cleanSearchEngine() ;
               PD_RC_CHECK( rc, PDERROR, "Clean index[%s] on search engine "
                                         "failed[%d]", esIdxName, rc ) ;
               _session->triggerStateTransition( FULL_INDEX ) ;
               goto done ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Check commit mark for index[%s] on search "
                                "engine failed[%d]", esIdxName, rc ) ;
               goto error ;
            }

            rc = _validateProgress( _progressInfo, valid ) ;
            PD_RC_CHECK( rc, PDERROR, "Validate progress information of "
                                      "index[%s] failed[%d]", esIdxName, rc ) ;
            if ( !valid )
            {
               PD_LOG( PDEVENT, "Commit mark for index[%s] is not as expected. "
                                "The index on search engine will be re-created",
                       esIdxName ) ;
               rc = _cleanSearchEngine() ;
               PD_RC_CHECK( rc, PDERROR, "Clean index[%s] on search engine "
                                         "failed[%d]", esIdxName, rc ) ;
               _session->triggerStateTransition( FULL_INDEX ) ;
               goto done ;
            }

            // 3. Query expected record from data node.
            expectLID =
                  _progressInfo.getField(SEADPT_FIELD_NAME_LID).numberLong() ;
            if ( -1 == expectLID )
            {
               // In the last run, full indexing had just finished.
               _session->triggerStateTransition( INCREMENT_INDEX ) ;
               goto done ;
            }

            rc = _queryExpectRecord( expectLID ) ;
            PD_RC_CHECK( rc, PDERROR, "Query record on data node failed[%d]",
                         rc ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptConsultState::_queryExpectRecord( INT64 expectLID )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;
      BSONObj condition ;
      seAdptDBAssist* dbAssist = _session->dbAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      try
      {
         condition = BSON( SEADPT_FIELD_NAME_ID << expectLID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }
      rc = msgBuildQueryMsg( &msg, &bufSize,
                             meta->getCappedCLName(),
                             FLG_QUERY_WITH_RETURNDATA,
                             _session->nextRequestID(), 0, 1, &condition,
                             NULL, NULL, NULL, _session->eduCB() ) ;
      imContext->metaUnlock() ;
      PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;
      ((MsgHeader *)msg)->TID = _session->tid() ;
      rc = dbAssist->sendToDataNode( (const MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send query message to data node failed[%d]",
                   rc ) ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      if ( msg )
      {
         msgReleaseBuffer( msg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _seAdptConsultState::_progressMatch( const BSONObj &record )
   {
      BOOLEAN match = FALSE ;
      try
      {
         UINT32 expectHash =
               (UINT32)(_progressInfo.getIntField( SEADPT_FIELD_NAME_HASH ) ) ;
         match =
               ( ossHash( record.objdata(), record.objsize() ) == expectHash ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
      }

      return match ;
   }

   _seAdptFullIndexState::_seAdptFullIndexState( _seAdptIndexSession *session )
   : _seAdptIndexerState( session ),
     _step( CLEAN_ES ),
     _clVersion( -1 )
   {
   }

   _seAdptFullIndexState::~_seAdptFullIndexState()
   {
   }

   SEADPT_INDEXER_STATE _seAdptFullIndexState::type() const
   {
      return FULL_INDEX ;
   }

   INT32 _seAdptFullIndexState::_processTimeout()
   {
      INT32 rc = SDB_OK ;

      switch ( _step )
      {
      case CLEAN_ES:
         rc = _cleanSearchEngine() ;
         PD_RC_CHECK( rc, PDERROR, "Clean data on ES failed[%d]", rc ) ;
         _step = CRT_ES_IDX ;
         // Intentially fall through.
      case CRT_ES_IDX:
         rc = _crtESIdx() ;
         PD_RC_CHECK( rc, PDERROR, "Create index on ES failed[%d]", rc ) ;
         // Intentially fall through.
      case CLEAN_DB_P1:
         rc = _prepareCleanDB() ;
         PD_RC_CHECK( rc, PDERROR, "Clean data on data node failed[%d]", rc ) ;
         break ;
      case QUERY_CL_VERSION:
         rc = _queryCLVersion() ;
         PD_RC_CHECK( rc, PDERROR, "Query version of collection failed[%d]",
                      rc ) ;
         break ;
      case QUERY_DATA:
         rc = _queryData() ;
         PD_RC_CHECK( rc, PDERROR, "Query data failed[%d]", rc ) ;
         break ;
      default:
         break ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_processCatalogRes( INT64 contextID,
                                                    INT32 startFrom,
                                                    INT32 numReturned,
                                                    vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;

      if ( 1 != numReturned )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Expect result number is 1. Actual: %d",
                 numReturned ) ;
         goto error ;
      }

      try
      {
         BSONObj record ;
         record = resultSet.front() ;
         _clVersion = record.getIntField( FIELD_NAME_VERSION ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
      PD_LOG( PDDEBUG, "Update local version of original collection to %d",
              _clVersion ) ;

      _step = QUERY_DATA ;
      rc = _queryData() ;
      PD_RC_CHECK( rc, PDERROR, "Query data failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_processQueryRes( INT64 contextID,
                                                  INT32 startFrom,
                                                  INT32 numReturned,
                                                  vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;
      switch ( _step )
      {
      case CLEAN_DB_P1:
      {
         SDB_ASSERT( numReturned <=1, "Return number is not as expected") ;
         if ( 0 == numReturned )
         {
            // No Records in capped collection. No cleanup required.
            _step = QUERY_CL_VERSION ;
            rc = _queryCLVersion() ;
            PD_RC_CHECK( rc, PDERROR, "Query version of collection failed[%d]",
                         rc ) ;
            goto done ;
         }

         rc = _doCleanDB( resultSet.front() ) ;
         PD_RC_CHECK( rc, PDERROR, "Clean data on data node failed[%d]", rc ) ;
         _step = CLEAN_DB_P2 ;
         break ;
      }
      case CLEAN_DB_P2:
      {
         _step = QUERY_CL_VERSION ;
         rc = _queryCLVersion() ;
         PD_RC_CHECK( rc, PDERROR, "Query version of collection failed[%d]",
                      rc ) ;
         break ;
      }
      case QUERY_DATA:
      {
         rc = _getMore( contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Send getmore request to data node "
                                   "failed[%d]", rc ) ;
         break ;
      }
      default:
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_processGetmoreRes( INT32 flag, INT64 contextID,
                                                    INT32 startFrom,
                                                    INT32 numReturned,
                                                    vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;
      seAdptSEAssist *seAssist = _session->seAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      try
      {
         if ( SDB_DMS_EOC == flag )
         {
            rc = _updateProgress( 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Update indexing progress failed[%d]",
                         rc ) ;
            _session->triggerStateTransition( INCREMENT_INDEX ) ;
            goto done ;
         }
         else if ( SDB_OK != flag )
         {
            rc = flag ;
            PD_LOG( PDERROR, "Get more failed[%d]", rc );
            goto error;
         }

         rc = seAssist->bulkPrepare( _session->getESIdxName(),
                                     _session->getESTypeName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare of bulk operation failed[%d]",
                      rc ) ;

         for ( vector<BSONObj>::const_iterator itr = resultSet.begin();
               itr != resultSet.end(); ++itr )
         {
            BSONObj finalRec ;
            string finalID ;
            rc = _parseRecord( *itr, finalID, finalRec ) ;
            if ( SDB_INVALIDARG == rc || finalRec.isEmpty() )
            {
               continue ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Format record[ %s ] failed[%d]",
                       itr->toString( false, true ).c_str(), rc ) ;
               goto error ;
            }

            if ( finalID.size() > SEADPT_MAX_ID_SZ )
            {
               PD_LOG( PDDEBUG, "Ignore document as actual id length[%d] "
                                "exceeds limit[%d]. id value: %s",
                       finalID.size(), SEADPT_MAX_ID_SZ, finalID.c_str() ) ;
               continue ;
            }

            {
               utilESActionIndex item( _session->getESIdxName(),
                                       _session->getESTypeName() ) ;
               rc = item.setID( finalID.c_str() ) ;
               PD_RC_CHECK( rc, PDERROR, "Set _id for action failed[%d]",
                            rc ) ;
               item.setSourceData( finalRec ) ;

               rc = seAssist->bulkProcess( item ) ;
               PD_RC_CHECK( rc, PDERROR, "Bulk processing item failed[%d]",
                            rc ) ;
               seAssist->oprMonitor()->monInsertCountInc() ;
            }
         }

         rc = seAssist->bulkFinish() ;
         PD_RC_CHECK( rc, PDERROR, "Finish operation of bulk failed[%d]",
                      rc ) ;
         rc = _getMore( contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Send getmore request to data node "
                                   "failed[%d]", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_seAdptFullIndexState::_getStepDesp() const
   {
      switch ( _step )
      {
      case CLEAN_ES:
         return "Clean search engine" ;
      case CRT_ES_IDX:
         return "Create index on search engine" ;
      case CLEAN_DB_P1:
         return "Clean DB phase 1" ;
      case CLEAN_DB_P2:
         return "Clean DB phase 2" ;
      case QUERY_CL_VERSION:
         return "Query collection version";
      default:
         return "Query data";
      }
   }

   INT32 _seAdptFullIndexState::_crtESIdx()
   {
      INT32 rc = SDB_OK ;
      seAdptSEAssist *se = _session->seAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *indexMeta = imContext->meta() ;
      const CHAR *idxName = _session->getESIdxName() ;
      const CHAR *typeName = _session->getESTypeName() ;
      UINT16 strMapType = sdbGetSeAdptOptions()->getStrMapType() ;
      ES_DATA_TYPE type = ES_TEXT ;

      if ( 2 == strMapType )
      {
         type = ES_KEYWORD ;
      }
      else if ( 3 == strMapType )
      {
         type = ES_MULTI_FIELDS ;
      }

      try
      {
         BSONObj mappingObj ;
         utilESMapping mapping( idxName, typeName ) ;

         rc = imContext->metaLock( SHARED ) ;
         if ( rc )
         {
            if ( SDB_RTN_INDEX_NOTEXIST == rc )
            {
               PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                                "unique ID[%llu], logical ID[%u], index logical "
                                "ID[%u]", imContext->getCLUID(),
                                imContext->getCLLID(), imContext->getIdxLID() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                       rc ) ;
            }
            goto error ;
         }

         // Generate the Elasticsearch index mapping. Only string fields.
         BSONObjIterator itr( indexMeta->getIdxDef() ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            mapping.addProperty( ele.fieldName(), type ) ;
         }

         imContext->metaUnlock() ;

         rc = mapping.toObj( mappingObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Build index mapping object failed[%d]",
                      rc ) ;

         rc = se->createIndex( idxName,
                               mappingObj.toString( FALSE, TRUE).c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Create index[%s] with mapping[%s] on "
                                   "search engine failed[%d]",
                      idxName, mappingObj.toString( FALSE, TRUE).c_str(), rc ) ;
         PD_LOG( PDEVENT, "Create index[%s] with mapping[%s] on "
                          "search engine successfully",
                 idxName, mappingObj.toString( FALSE, TRUE).c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _step = CLEAN_DB_P1 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_prepareCleanDB()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      // Get the _id of the first record in capped collection.
      try
      {
         selector = BSON( "_id" << "" );
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;

      }
      rc = dbAssist->queryOnDataNode( meta->getCappedCLName(),
                                      _session->nextRequestID(),
                                      _session->tid(), FLG_QUERY_WITH_RETURNDATA,
                                      0, 1, NULL, &selector,
                                      NULL, NULL, _session->eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Query _id of the first record in "
                                "collection[%s] failed[%d]",
                   meta->getCappedCLName(), rc ) ;

      PD_LOG( PDDEBUG, "Ready to clean capped collection[%s]. Query for the "
                       "first record...", meta->getCappedCLName() ) ;
      _timeout = 0 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_doCleanDB( const BSONObj &targetObj )
   {
      INT32 rc = SDB_OK ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      try
      {
         BSONObjBuilder builder ;
         BSONObj option ;
         BSONElement lidEle= targetObj.getField( "_id" ) ;

         rc = imContext->metaLock( SHARED ) ;
         if ( rc )
         {
            if ( SDB_RTN_INDEX_NOTEXIST == rc )
            {
               PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                                "unique ID[%llu], logical ID[%u], index logical "
                                "ID[%u]", imContext->getCLUID(),
                                imContext->getCLLID(), imContext->getIdxLID() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                       rc ) ;
            }
            goto error ;
         }
         builder.append( FIELD_NAME_COLLECTION, meta->getCappedCLName() ) ;
         builder.appendAs( lidEle, FIELD_NAME_LOGICAL_ID ) ;
         builder.appendIntOrLL( FIELD_NAME_DIRECTION, -1 ) ;
         option = builder.done() ;

         rc = dbAssist->doCmdOnDataNode( CMD_ADMIN_PREFIX CMD_NAME_POP,
                                         option, _session->nextRequestID(),
                                         _session->tid(), _session->eduCB() ) ;
         PD_RC_CHECK( rc, PDERROR, "Send pop command to data node failed[%d]",
                      rc ) ;
         PD_LOG( PDEVENT, "Begin to clean capped collection[%s] by pop "
                          "command. Pop options: %s",
                 meta->getCappedCLName(),
                 option.toString(false, true).c_str() ) ;
         _timeout = 0 ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_queryCLVersion()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      try
      {
         selector = BSON( FIELD_NAME_VERSION << "" ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred when creating query: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }
      rc = dbAssist->queryCLCataInfo( meta->getOrigCLName(),
                                      _session->nextRequestID(),
                                      _session->tid(), &selector,
                                      _session->eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Query catalogue information of collection[%s] "
                                " failed[%d]", meta->getOrigCLName(), rc ) ;
      _timeout = 0 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      if ( SDB_NET_CANNOT_CONNECT == rc )
      {
         dbAssist->changeCataPrimary( INVALID_NODEID ) ;
      }
      goto done ;
   }

   INT32 _seAdptFullIndexState::_queryData()
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *msg = NULL ;
      INT32 bufSize = 0 ;
      BSONObj query ;
      BSONObj selector ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      rc = _genQueryOptions( query, selector ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate query and selector for the original "
                                "colleciton failed[%d]", rc ) ;
      PD_LOG( PDDEBUG, "Full indexing query condition: %s",
              query.toString(false, true).c_str() ) ;
      rc = msgBuildQueryMsg( (CHAR **)&msg, &bufSize,
                             meta->getOrigCLName(),
                             0, _session->nextRequestID(), 0, -1, &query,
                             &selector, NULL, NULL, _session->eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;
      imContext->metaUnlock() ;

      msg->version = _clVersion ;
      msg->header.TID = _session->tid() ;
      rc = _session->dbAssist()->sendToDataNode( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send query message to data node failed[%d]",
                   rc ) ;
      _timeout = 0 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_genQueryOptions( BSONObj &query,
                                                  BSONObj &selector )
   {
      INT32 rc = SDB_OK ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      SDB_ASSERT( meta, "index meta is NULL" ) ;

      try
      {
         BSONObjBuilder queryBuilder ;
         BSONObjBuilder selectorBuilder ;
         BSONObjIterator idxItr( meta->getIdxDef() ) ;
         BSONArrayBuilder
            queryObj( queryBuilder.subarrayStart( SEADPT_OPERATOR_STR_OR ) ) ;

         BSONObj existTmp = BSON( SEADPT_OPERATOR_STR_EXIST << 1 ) ;
         BSONObj includeObj = BSON( SEADPT_OPERATOR_STR_INCLUDE << 1 ) ;

         selectorBuilder.appendObject( SEADPT_FIELD_NAME_ID,
                                       includeObj.objdata(),
                                       includeObj.objsize() ) ;

         while ( idxItr.more() )
         {
            BSONElement ele = idxItr.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            SDB_ASSERT( 0 != ossStrcmp( fieldName, SEADPT_FIELD_NAME_ID ),
                        "Text index should not include _id" ) ;
            selectorBuilder.appendObject( fieldName, includeObj.objdata(),
                                          includeObj.objsize() ) ;
            BSONObjBuilder existObj( queryObj.subobjStart() ) ;
            existObj.appendObject( fieldName, existTmp.objdata(),
                                   existTmp.objsize() ) ;
            existObj.done() ;
         }
         queryObj.done() ;
         query = queryBuilder.obj() ;
         selector = selectorBuilder.obj() ;

         PD_LOG( PDDEBUG, "Original collection query, condition: %s, "
                          "selector: %s",
                 query.toString().c_str(),
                 selector.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_getMore( INT64 contextID )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;

      rc = msgBuildGetMoreMsg( &msg, &bufSize, -1, contextID,
                               _session->nextRequestID(), _session->eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build get more request failed[%d]", rc ) ;
         goto error ;
      }
      ((MsgHeader *)msg)->TID = _session->tid() ;
      rc = _session->dbAssist()->sendToDataNode( (const MsgHeader *)msg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send get more request to data node failed[%d]",
                 rc ) ;
         goto error ;
      }
      _timeout = 0 ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_parseRecord( const BSONObj &origRecord,
                                              string &finalID,
                                              BSONObj &finalRecord )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement idEle ;
         BSONElement arrayEle ;
         BSONObjSet keySet ;
         SDB_ASSERT( !_idxDef.isEmpty(), "Index definition is empty" ) ;
         ixmIndexKeyGen keygen( _idxDef, GEN_OBJ_KEEP_FIELD_NAME ) ;
         rc = keygen.getKeys( origRecord, keySet, &arrayEle, TRUE, TRUE ) ;
         if ( SDB_IXM_MULTIPLE_ARRAY == rc )
         {
            // Ignore the record if there are more than one array in it's index
            // fields.
            PD_LOG( PDWARNING, "Record ignored as there are multiple arrays in "
                    "it's index fields. Record: %s",
                    origRecord.toString().c_str() ) ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get index keys from record[%s] failed[%d]",
                      origRecord.toString().c_str(), rc ) ;
         if ( 0 == keySet.size() )
         {
            // No index key in the record.
            goto done ;
         }

         idEle = origRecord.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() )
         {
            PD_LOG( PDWARNING, "Record with no _id field will be ignored. "
                    "Record: %s", origRecord.toString().c_str() ) ;
            goto done ;
         }

         encodeID( idEle, finalID ) ;
         if ( !finalID.empty() )
         {
            rc = _genRecordByKeySet( keySet, finalRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Generate record by keyset failed[%d]",
            rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred[%s]", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptFullIndexState::_genRecordByKeySet( BSONObjSet keySet,
                                                    BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( !keySet.empty(), "Key set is empty") ;

      BSONObjBuilder builder ;
      BOOLEAN found = FALSE ;    // Whether any string field(or string array)
                                 // is found.

      try
      {
         if ( 1 == keySet.size() )
         {
            BSONObjIterator itr( *( keySet.begin() ) ) ;
            // Loop and check if the record contains only strings.
            while ( itr.more() )
            {
               BSONElement ele = itr.next() ;
               if ( String == ele.type() )
               {
                  builder.append( ele ) ;
                  if ( !found )
                  {
                     found = TRUE ;
                  }
               }
            }
         }
         else
         {
            // One of the index field is of type array. Resume the array from
            // the keys. Compare the elements of the first and second key. If
            // they are not the same, that's the array field.
            BOOLEAN arrayFieldHit = FALSE ;
            BSONObjIterator itrFirst( *(keySet.begin()) ) ;
            BSONObjIterator itrSecond( *(++keySet.begin()) ) ;

            while ( itrFirst.more() )
            {
               BSONElement lNextEle = itrFirst.next() ;
               BSONElement rNextEle = itrSecond.next() ;
               if ( arrayFieldHit || 0 == lNextEle.woCompare( rNextEle, true) )
               {
                  // Same, it's not the array field. Only keep string fields.
                  if ( String != lNextEle.type() )
                  {
                     continue ;
                  }
                  builder.append( lNextEle ) ;
                  if ( !found )
                  {
                     found = TRUE ;
                  }
               }
               else
               {
                  // The array field is found. If any element of the array field
                  // is of non-string type, ignore the array.
                  arrayFieldHit = TRUE ;
                  if ( String == lNextEle.type() && String == rNextEle.type() )
                  {
                     BOOLEAN pureStrArray = TRUE ;
                     const CHAR *arrField = lNextEle.fieldName() ;
                     BSONArrayBuilder arrBuilder ;
                     arrBuilder.append( lNextEle ) ;
                     arrBuilder.append( rNextEle ) ;
                     // Get the array field from all key record.
                     BSONObjSet::iterator itr = keySet.begin() ;
                     // Skip the first and second key.
                     std::advance( itr, 2 ) ;
                     while ( itr != keySet.end() )
                     {
                        BSONElement ele = itr->getField( arrField ) ;
                        if ( String != ele.type() )
                        {
                           pureStrArray = FALSE ;
                           break ;
                        }
                        arrBuilder.append( itr->getStringField( arrField ) ) ;
                        ++itr ;
                     }
                     if ( pureStrArray )
                     {
                        arrBuilder.doneFast() ;
                        builder.append( arrField, arrBuilder.arr() ) ;
                        if ( !found )
                        {
                           found = TRUE ;
                        }
                     }
                  }
               }
            }
         }
         if ( found )
         {
            record = builder.obj() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _seAdptIncIndexState::_seAdptIncIndexState( _seAdptIndexSession *session )
   : _seAdptIndexerState( session ),
     _step( QUERY_DATA ),
     _hasNewData( FALSE ),
     _firstBatchData( TRUE ),
     _expectRecHash( 0 ),
     _queryCtxID( -1 )
   {

   }

   _seAdptIncIndexState::~_seAdptIncIndexState()
   {

   }

   INT32 _seAdptIncIndexState::_processTimeout()
   {
      INT32 rc = SDB_OK ;

      if ( QUERY_DATA == _step )
      {
         rc = _queryData() ;
         PD_RC_CHECK( rc, PDERROR, "Query data of capped collection failed[%d]",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   SEADPT_INDEXER_STATE _seAdptIncIndexState::type() const
   {
      return INCREMENT_INDEX ;
   }

   INT32 _seAdptIncIndexState::_processQueryRes( INT64 contextID,
                                                 INT32 startFrom,
                                                 INT32 numReturned,
                                                 vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;

      switch ( _step )
      {
      case QUERY_DATA:
      {
         rc = _getMore( contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Getmore request failed[%d]", rc ) ;
         _queryCtxID = contextID ;
         _firstBatchData = TRUE ;
         break ;
      }
      case CLEAN_SRC:
         // After clean the last batch of data, continue to fetch next batch of
         // data with the original query context.
         rc = _getMore( _queryCtxID ) ;
         PD_RC_CHECK( rc, PDERROR, "Getmore request failed[%d]", rc ) ;
      default:
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_processGetmoreRes( INT32 flag, INT64 contextID,
                                                   INT32 startFrom,
                                                   INT32 numReturned,
                                                   vector<BSONObj> &resultSet )
   {
      INT32 rc = SDB_OK ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      if ( SDB_DMS_EOC == flag )
      {
         // If the expected LID is -1 and the capped collection is empty, there
         // are two possibilities:
         // (1) All records in the original collection have been processed, and
         //     no more record change after the index creation
         // (2) The capped collection has been re-created. That may happen when
         //     the collection is re-created, or the original table is truncated.
         // So we check the meta data. If it's the second condition, let's just
         // quit. The new worker will drop and create the ES index during
         // consult.
         if ( _hasNewData )
         {
            _hasNewData = FALSE ;
            PD_LOG( PDDEBUG, "All records in capped collection[%s] have been "
                             "processed. Ready to start a new query on it",
                    imContext->meta()->getCappedCLName() ) ;
         }

         // Wait for next round to be triggered by the timer.
         _timeout = 0 ;
         _step = QUERY_DATA ;
         goto done ;
      }
      else if ( flag )
      {
         rc = flag ;
         PD_LOG( PDERROR, "Get data failed[%d]", rc ) ;
         goto error ;
      }

      try
      {
         rc = _processDocuments( resultSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Index documents on search engine "
                                   "failed[%d]", rc ) ;
         if ( _rebuild )
         {
            goto done ;
         }

         if ( _hasNewData &&
              _session->getLastExpectLID() != SEADPT_INVALID_LID &&
              ( _session->getLastExpectLID() < _session->getExpectLID() ) )
         {
            // pop data
            rc = _cleanData() ;
            PD_RC_CHECK( rc, PDERROR, "Send clean data request failed[%d]",
                         rc ) ;
            goto done ;
         }
         else
         {
            rc = _getMore( contextID ) ;
            PD_RC_CHECK( rc, PDERROR, "Send getmore request to data node "
                                      "failed[%d]", rc ) ;
         }
         if ( _firstBatchData )
         {
            _firstBatchData = FALSE ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_seAdptIncIndexState::_getStepDesp() const
   {
      switch ( _step )
      {
      case QUERY_DATA:
         return "Query data" ;
      case GETMORE_DATA:
         return "Get more data" ;
      default:
         return "Clean capped collection" ;
      }
   }

   INT32 _seAdptIncIndexState::_genQueryOptions( BSONObj &query,
                                                 BSONObj &selector )
   {
      INT32 rc = SDB_OK ;

      try
      {
         query = BSON( SEADPT_FIELD_NAME_ID <<
                       BSON( "$gte" << _session->getExpectLID() ) ) ;
         PD_LOG( PDDEBUG, "Inremental condition: %s",
                 query.toString(false, true).c_str() ) ;
         selector = BSONObj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_queryData()
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;
      BSONObj query ;
      BSONObj selector ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;

      rc = _genQueryOptions( query, selector ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate query and selector for the capped"
                                "colleciton failed[%d]", rc ) ;
      PD_LOG( PDDEBUG, "Incremental indexing query condition: %s",
              query.toString(false, true).c_str() ) ;

      // Need to get the capped collection name from the metadata item. So need
      // to latch it.
      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      rc = msgBuildQueryMsg( &msg, &bufSize,
                             imContext->meta()->getCappedCLName(),
                             0, _session->nextRequestID(), 0, -1, &query,
                             &selector, NULL, NULL, _session->eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;

      imContext->metaUnlock() ;

      ((MsgHeader *)msg)->TID = _session->tid() ;
      rc = _session->dbAssist()->sendToDataNode( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send query message to data node failed[%d]",
                   rc ) ;
      _timeout = 0 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }

      if ( msg )
      {
         msgReleaseBuffer( msg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_getMore( INT64 contextID )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;

      _step = GETMORE_DATA ;
      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      rc = msgBuildGetMoreMsg( &msg, &bufSize, -1, contextID,
                               _session->nextRequestID(), _session->eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build getmore request failed[%d]", rc ) ;
         goto error ;
      }
      ((MsgHeader *)msg)->TID = _session->tid() ;
      rc = _session->dbAssist()->sendToDataNode( (const MsgHeader *)msg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send getmore request to data node failed[%d]", rc ) ;
         goto error ;
      }
      _timeout = 0 ;

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      if ( msg )
      {
         msgReleaseBuffer( msg, _session->eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_processDocuments( const vector <BSONObj> &docs )
   {
      INT32 rc = SDB_OK ;
      UINT32 lastObjHash = 0 ;
      INT64 logicalID = SEADPT_INVALID_LID ;
      seAdptSEAssist* seAssist = _session->seAssist() ;

      try
      {
         if ( docs.size() > 0 )
         {
            if ( _firstBatchData && 0 != _expectRecHash )
            {
               rc = _consistencyCheck( docs.front() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "The first record in capped collection"
                                   " is not as we expected. Clean index[%s] on "
                                   "search engine",
                          _session->getESIdxName() ) ;
                  {
                     INT32 rcTmp = _cleanSearchEngine() ;
                     if ( rcTmp )
                     {
                        PD_LOG( PDERROR, "Cleanup on search engine when "
                                         "consistency check error failed[%d]",
                                rcTmp ) ;
                     }
                  }
                  goto error ;
               }
            }

            if ( !_firstBatchData && 1 == docs.size() )
            {
               _hasNewData = FALSE ;
               goto done ;
            }
            {
               // Calculate expect hash value of next round.
               BSONObj lastObj = docs.back() ;
               lastObjHash = ossHash( lastObj.objdata(), lastObj.objsize() ) ;
               if ( !_hasNewData )
               {
                  _hasNewData = TRUE ;
               }
            }
         }

         vector<BSONObj>::const_iterator itr = docs.begin() ;

         // We always get one more record, if the _expectLID is not -1. So the
         // first one should be filtered out.
         if ( SEADPT_INVALID_LID != _session->getExpectLID() && _firstBatchData )
         {
            itr++ ;
            if ( itr == docs.end() )
            {
               _hasNewData = FALSE ;
               goto done ;
            }
         }

         rc = seAssist->bulkPrepare( _session->getESIdxName(),
                                     _session->getESTypeName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare of bulk operation failed[%d]",
                      rc ) ;
         for ( ; itr != docs.end(); ++itr )
         {
            BOOLEAN isRebuildRecord = FALSE ;
            rc = _processDocument( *itr, logicalID, isRebuildRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Process document failed[%d]. "
                                      "Document: %s",
                         rc, itr->toString(false, true).c_str() ) ;
            if ( isRebuildRecord )
            {
               _rebuild = TRUE ;
               goto done ;
            }
         }

         rc = seAssist->bulkFinish() ;
         PD_RC_CHECK( rc, PDERROR, "Finish operation of bulk "
                                   "failed[%d]", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      if ( SEADPT_INVALID_LID != logicalID )
      {
         _session->setExpectLID( logicalID ) ;
         rc = _updateProgress( lastObjHash ) ;
         PD_RC_CHECK( rc, PDERROR, "Update progress failed[%d]", rc ) ;
      }

      _expectRecHash = lastObjHash ;
      _firstBatchData = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_processDocument( const BSONObj &document,
                                                 INT64 &logicalID,
                                                 BOOLEAN &isRebuildRecord )
   {
      INT32 rc = SDB_OK ;

      BSONObj newNameObj ;
      string idStr ;
      BSONObj sourceObj ;
      string finalID ;
      string finalIdNew ;
      _rtnExtOprType oprType = RTN_EXT_INVALID ;
      seAdptSEAssist *seAssist = _session->seAssist() ;

      rc = _parseRecord( document, oprType, finalID, logicalID,
                         sourceObj, &finalIdNew ) ;
      if ( SDB_INVALIDARG == rc )
      {
         // Unsupport type, just ignore.
         seAssist->oprMonitor()->monIgnoreCountInc() ;
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Get id string and source object failed[%d]", rc ) ;
         goto error ;
      }

      if ( RTN_EXT_DUMMY == oprType )
      {
         isRebuildRecord = TRUE ;
         goto done ;
      }

      if ( finalID.size() > SEADPT_MAX_ID_SZ )
      {
         PD_LOG( PDDEBUG, "Ignore document as actual id length[%d] "
                          "exceeds limit[%d]. id value: %s",
                 finalID.size(), SEADPT_MAX_ID_SZ, finalID.c_str() ) ;
         seAssist->oprMonitor()->monIgnoreCountInc() ;
         goto done ;
      }

      if ( sourceObj.isEmpty() && ( RTN_EXT_INSERT == oprType ) )
      {
         // Nothing should be inserted.
         seAssist->oprMonitor()->monIgnoreCountInc() ;
         goto done ;
      }

      seAssist->oprMonitor()->monOprCountInc( oprType ) ;

      if ( sourceObj.isEmpty() && RTN_EXT_UPDATE == oprType )
      {
         oprType = RTN_EXT_DELETE ;
      }

      switch ( oprType )
      {
         // In case of update, we are going to update the whole document,
         // not part of it. So indexing should be done instead of updating.
      case RTN_EXT_INSERT:
      case RTN_EXT_UPDATE:
      {
         rc = seAssist->bulkAppendIndex( finalID.length() > 0 ?
                                         finalID.c_str() : NULL, sourceObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Index document failed[%d]", rc ) ;
         break ;
      }
      case RTN_EXT_DELETE:
      {
         rc = seAssist->bulkAppendDel( finalID.length() > 0 ?
                                       finalID.c_str() : NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Delete document failed[%d]", rc ) ;
         break ;
      }
      case RTN_EXT_UPDATE_WITH_ID:
      {
         rc = seAssist->bulkAppendReplace( finalID.length() > 0 ?
                                           finalID.c_str() : NULL,
                                           finalIdNew.length() > 0 ?
                                           finalIdNew.c_str() : NULL,
                                           sourceObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Update record failed[%d]", rc ) ;
         break ;
      }
      default:
         PD_LOG( PDERROR, "Invalid operation type[%d] in source data",
                 oprType ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_consistencyCheck( const BSONObj &doc )
   {
      INT32 rc = SDB_OK ;

      // Check if the first record is the one we expected. If not, start
      // over again. Check only once in one query round.
      if ( SEADPT_INVALID_LID != _session->getExpectLID() )
      {
         UINT32 objHash = ossHash( doc.objdata(), doc.objsize() ) ;
         if ( objHash != _expectRecHash )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The first record in capped collection is not as "
                             "we expected" ) ;
            goto error ;
         }
         _hasNewData = FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_parseRecord( const BSONObj &origObj,
                                             _rtnExtOprType &oprType,
                                             string &finalID,
                                             INT64 &logicalID,
                                             BSONObj &sourceObj,
                                             string *newFinalID )
   {
      INT32 rc = SDB_OK ;
      INT32 type = 0 ;

      try
      {
         BSONElement lidField = origObj.getField( SEADPT_FIELD_NAME_ID ) ;
         BSONElement ridEle = origObj.getField( SEADPT_FIELD_NAME_RID ) ;
         type = origObj.getIntField( FIELD_NAME_TYPE ) ;
         if ( !_typeSupport( type ) )
         {
            PD_LOG( PDERROR, "Operation type[%d] is not supported in "
                             "source record: %s",
                    type, origObj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         oprType = ( _rtnExtOprType )type ;
         if ( RTN_EXT_DUMMY == oprType )
         {
            goto done ;
         }

         if ( lidField.eoo() || ridEle.eoo() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "_id or _rid not found in record: %s",
                    origObj.toString().c_str() ) ;
            goto error ;
         }

         logicalID = lidField.Long() ;
         encodeID( ridEle, finalID ) ;

         // When updating the _id field, it's a little complicated, as some data
         // types are not supported now, $minKey, for example. It may be updated
         // from unsupported type to one that we  support, or reverse scenario.
         // In these cases, the encode of the old or new _id will fail.
         // Need careful checking to handle all these cases.
         if ( RTN_EXT_UPDATE_WITH_ID == oprType )
         {
            BSONElement newIdEle =
                  origObj.getField( SEADPT_FIELD_NAME_RID_NEW ) ;
            if ( newIdEle.eoo() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get new _id from object failed[%d]", rc ) ;
               goto error ;
            }
            encodeID( newIdEle, *newFinalID ) ;
            // Change from unsupported type to another unsupported type, nothing
            // should be done.
            if ( finalID.empty() && newFinalID->empty() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            // For any other cases(_id not changed), if the encoding fails,
            // return error.
            if ( finalID.empty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDWARNING, "Encode _id[ %s ] failed[%d]",
                       ridEle.toString( false, true ).c_str(), rc ) ;
               goto error ;
            }
         }

         {
            BSONObjBuilder builder ;
            BSONObj source = origObj.getObjectField( "_source" ) ;
            if ( !source.isEmpty() && !source.isValid() )
            {
               PD_LOG( PDERROR, "_source field is invalid. Object: %s",
                       origObj.toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // Only keep the string fields. Fields of other type(include oid)
            // will be ignored.
            for ( BSONObj::iterator eleItr = source.begin(); eleItr.more(); )
            {
               BSONElement ele = eleItr.next() ;
               if ( String == ele.type() )
               {
                  builder.append( ele ) ;
               }
               else if ( Array == ele.type() )
               {
                  // As ES dose not support mixed type of array, so check if the
                  // array only contains strings. If yes, index it on ES.
                  // Otherwise, ignore the field.
                  BOOLEAN onlyString = TRUE ;
                  BSONObjIterator itr( ele.embeddedObject() ) ;
                  while ( itr.more() )
                  {
                     if ( String != itr.next().type() )
                     {
                        onlyString = FALSE ;
                        break ;
                     }
                  }

                  if ( onlyString )
                  {
                     builder.append( ele ) ;
                  }
               }
            }

            sourceObj = builder.obj() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptIncIndexState::_cleanData()
   {
      INT32 rc = SDB_OK ;
      seAdptDBAssist *dbAssist = _session->dbAssist() ;
      seIdxMetaContext *imContext = _session->idxMetaContext() ;
      const seIndexMeta *meta = imContext->meta() ;

      if ( SEADPT_INVALID_LID == _session->getLastExpectLID() ||
           0 == _expectRecHash )
      {
         _step = QUERY_DATA ;
         goto done ;
      }

      rc = imContext->metaLock( SHARED ) ;
      if ( rc )
      {
         if ( SDB_RTN_INDEX_NOTEXIST == rc )
         {
            PD_LOG( PDEVENT, "Index metadata not found any more. Collection "
                             "unique ID[%llu], logical ID[%u], index logical "
                             "ID[%u]", imContext->getCLUID(),
                             imContext->getCLLID(), imContext->getIdxLID() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Lock index metadata in SHARED mode failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         BSONObj option ;

         PD_RC_CHECK( rc, PDERROR, "Lock index metadata failed[%d]", rc ) ;
         builder.append( FIELD_NAME_COLLECTION, meta->getCappedCLName() ) ;
         builder.append( FIELD_NAME_LOGICAL_ID, _session->getLastExpectLID() ) ;
         builder.appendIntOrLL( FIELD_NAME_DIRECTION, 1 ) ;
         option = builder.done() ;

         rc = dbAssist->doCmdOnDataNode( CMD_ADMIN_PREFIX CMD_NAME_POP,
                                         option, _session->nextRequestID(),
                                         _session->tid(), _session->eduCB() ) ;
         PD_RC_CHECK( rc, PDERROR, "Send command to catalogue node failed[%d]",
                      rc ) ;
         PD_LOG( PDINFO, "Clean capped collection[%s] by pop "
                         "command. Pop options: %s",
                 meta->getCappedCLName(),
                 option.toString(false, true).c_str() ) ;
         _timeout = 0 ;
         _step = CLEAN_SRC ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( imContext->isMetaLocked() )
      {
         imContext->metaUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }
}

