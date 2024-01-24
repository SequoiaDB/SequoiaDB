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

   Source File Name = catContext.cpp

   Descriptive Name = Runtime Context of Catalog

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context of Catalog
   helper functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catContext.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"

namespace engine
{
   /*
    *  _catContextBase implement
    */
   _catContextBase::_catContextBase ( INT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _pDmsCB       = krcb->getDMSCB() ;
      _pDpsCB       = krcb->getDPSCB() ;
      _pCatCB       = krcb->getCATLOGUECB() ;
      _status       = CAT_CONTEXT_NEW ;

      _executeOnP1 = FALSE ;
      _needPreExecute = FALSE ;
      _needRollbackAlways = FALSE ;
      _needRollback = FALSE ;

      _needUpdate = FALSE ;
      _hasUpdated = FALSE ;
      _needClearAfterDone = FALSE ;
      _version = -1 ;
      _hitEnd = FALSE ;
   }

   _catContextBase::~_catContextBase ()
   {
      SDB_ASSERT ( _status == CAT_CONTEXT_END,
                   "Wrong catalog status after deleting" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_OPEN, "_catContextBase::open" )
   INT32 _catContextBase::open ( const NET_HANDLE &handle,
                                 MsgHeader *pMsg,
                                 const CHAR *pQuery,
                                 const CHAR *pHint,
                                 rtnContextBuf &buffObj,
                                 _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( _status == CAT_CONTEXT_NEW,
                   "Wrong catalog status before opening" ) ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE_OPEN ) ;

      rc = _initQuery( handle, pMsg, pQuery, pHint, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to initialize query, rc: %d",
                   contextID(), rc) ;

      rc = _open( buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXBASE_OPEN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__OPEN_OBJ, "_catContextBase::_open" )
   INT32 _catContextBase::_open( const BSONObj &queryObject,
                                 MSG_TYPE cmdType,
                                 rtnContextBuf &buffObj,
                                 _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( _status == CAT_CONTEXT_NEW,
                   "Wrong catalog status before opening" ) ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__OPEN_OBJ ) ;

      try
      {
         _boQuery = queryObject.getOwned() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get query object, rc: %d", rc ) ;
      }

      _cmdType = cmdType ;

      rc = _open( buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXBASE__OPEN_OBJ, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__OPEN, "_catContextBase::_open" )
   INT32 _catContextBase::_open ( rtnContextBuf &buffObj,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__OPEN ) ;

      rc = _regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register event handlers, "
                   "rc: %d", rc ) ;

      rc = _parseQuery( cb ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to parse query, rc: %d",
                   contextID(), rc ) ;

      cb->setCurProcessName( _targetName.c_str() ) ;

      rc = _parseQueryForHandlers( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse query for handlers, "
                   "rc: %d", rc ) ;

      _setStatus( CAT_CONTEXT_LOCKING ) ;

      rc = _checkContext( cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to check and lock catalog objects, rc: %d",
                   contextID(), rc ) ;

      if ( _executeOnP1 )
      {
         if ( _needPreExecute )
         {
            rc = _preExecute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to execute catalog command, rc: %d",
                         contextID(), rc ) ;
         }
         else
         {
            rc = _execute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to execute catalog command, rc: %d",
                         contextID(), rc ) ;
         }
      }

      rc = _makeReply( CAT_CONTEXT_PHASE_1, buffObj ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed in catContext [%lld]: "
                    "failed to make reply message, rc: %d",
                    contextID(), rc ) ;

      PD_LOG( PDDEBUG,
              "catContext [%lld]: open finished",
              contextID() ) ;

      _isOpened = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXBASE__OPEN, rc ) ;
      return rc ;

   error:
      _changeStatusOnError() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_PREPAREDATA, "_catContextBase::_prepareData" )
   INT32 _catContextBase::_prepareData ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE_PREPAREDATA ) ;

      CAT_CONTEXT_PHASE phase = CAT_CONTEXT_PHASE_2 ;

      switch ( _status )
      {
      case CAT_CONTEXT_READY :
      {
         if ( _needPreExecute )
         {
            rc = _preExecute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to pre-execute catalog command, rc: %d",
                         contextID(), rc ) ;
         }
         else
         {
            rc = _execute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to execute catalog command, rc: %d",
                         contextID(), rc ) ;
         }
         break ;
      }
      case CAT_CONTEXT_PREEXECUTED :
      {
         rc = _execute( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to execute catalog command, rc: %d",
                      contextID(), rc ) ;
         break ;
      }
      case CAT_CONTEXT_CAT_DONE :
      {
         rc = _commit( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to commit catalog changes, rc: %d",
                      contextID(), rc ) ;
         _clear( cb ) ;
         phase = CAT_CONTEXT_PHASE_COMMIT ;
         break ;
      }
      default :
         PD_LOG( PDDEBUG,
                 "Failed in catContext [%lld]: "
                 "wrong status %d in get-more",
                 contextID(), _status ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _makeReply( phase, buffObj ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed in catContext [%lld]: "
                    "failed to make reply message, rc: %d",
                    contextID(), rc ) ;

      if ( _status == CAT_CONTEXT_DATA_DONE )
      {
         // End of execution
         rc = SDB_DMS_EOC ;
      }
      else
      {
         rc = appendObjs( buffObj.data(), buffObj.size(),
                          buffObj.recordNum() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Append data(%d) failed, rc: %d",
                    buffObj.size(), rc ) ;
            goto error ;
         }
      }

      PD_LOG( PDDEBUG, "Finished context[%lld] getmore", contextID() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_PREPAREDATA, rc ) ;
      return rc ;

   error :
      _changeStatusOnError() ;
      goto done ;
   }

   void _catContextBase::_setStatus ( CAT_CONTEXT_STATUS status )
   {
      PD_LOG( PDDEBUG,
              "catContext [%lld] status change: %d -> %d",
              contextID(), _status, status ) ;
      _status = status ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_CHECKCTX, "_catContextBase::_checkContext" )
   INT32 _catContextBase::_checkContext ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_CHECKCTX ) ;

      INT16 w = _pCatCB->majoritySize() ;

      try
      {
         rc = _onCheckEvent( SDB_EVT_OCCUR_BEFORE, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call before check event "
                      "on context [%s], rc: %d", name(), rc ) ;

         rc = _checkInternal( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to check context internal, rc: %d",
                      contextID(), rc ) ;

         rc = _onCheckEvent( SDB_EVT_OCCUR_AFTER, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call after check event "
                      "on context [%s], rc: %d", name(), rc ) ;

         _needUpdate = TRUE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished check context",
              contextID() ) ;

      _setStatus( CAT_CONTEXT_READY ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_CHECKCTX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_PREEXECUTE, "_catContextBase::_preExecute" )
   INT32 _catContextBase::_preExecute ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_PREEXECUTE ) ;

      INT16 w = _pCatCB->majoritySize() ;

      if ( !_needUpdate || !_needPreExecute )
      {
         _needPreExecute = FALSE ;
         goto done ;
      }

      try
      {
         rc = _preExecuteInternal( cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to pre-execute catalog changes, rc: %d",
                      contextID(), rc ) ;

         // Only need one pre-execute
         _needPreExecute = FALSE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished pre-execute",
              contextID() ) ;

   done :
      if ( SDB_OK == rc )
      {
         _setStatus ( CAT_CONTEXT_PREEXECUTED ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_PREEXECUTE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_EXECUTE, "_catContextBase::_execute" )
   INT32 _catContextBase::_execute ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_EXECUTE ) ;

      INT16 w = _pCatCB->majoritySize() ;

      if ( !_needUpdate )
      {
         goto done ;
      }

      try
      {
         rc = _onExecuteEvent( SDB_EVT_OCCUR_BEFORE, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call before execute event "
                      "on context [%s], rc: %d", name(), rc ) ;

         if ( _needRollbackAlways )
         {
            _hasUpdated = TRUE ;
         }

         rc = _executeInternal( cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to execute context internal, rc: %d",
                      contextID(), rc ) ;

         _hasUpdated = TRUE ;

         rc = _onExecuteEvent( SDB_EVT_OCCUR_AFTER, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call after execute event "
                      "on context [%s], rc: %d", name(), rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished execute",
              contextID() ) ;

   done :
      if ( SDB_OK == rc )
      {
         // Update status
         _setStatus( CAT_CONTEXT_CAT_DONE ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_EXECUTE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_COMMIT, "_catContextBase::_commit" )
   INT32 _catContextBase::_commit ( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXBASE_COMMIT ) ;

      INT16 w = _pCatCB->majoritySize() ;

      _onCommitEvent( SDB_EVT_OCCUR_BEFORE, cb, w ) ;

      _setStatus( CAT_CONTEXT_DATA_DONE ) ;

      _onCommitEvent( SDB_EVT_OCCUR_AFTER, cb, w ) ;

      PD_TRACE_EXIT ( SDB_CATCTXBASE_COMMIT ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_ROLLBACK, "_catContextBase::_rollback" )
   INT32 _catContextBase::_rollback ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_ROLLBACK ) ;

      if ( cb->isForced() )
      {
         // The database is closing, do not rollback
         goto done ;
      }

      if ( !_hasUpdated )
      {
         goto done ;
      }

      try
      {
         INT16 w = _pCatCB->majoritySize() ;

         _onRollbackEvent( SDB_EVT_OCCUR_BEFORE, cb, w ) ;

         rc = _rollbackInternal( cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to rollback context, rc: %d",
                      contextID(), rc ) ;

         _onRollbackEvent( SDB_EVT_OCCUR_AFTER, cb, w ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished rollback",
              contextID() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_ROLLBACK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_INITQUERY, "_catContextBase::_initQuery" )
   INT32 _catContextBase::_initQuery ( const NET_HANDLE &handle,
                                       MsgHeader *pMsg,
                                       const CHAR *pQuery,
                                       const CHAR *pHint,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_INITQUERY ) ;

      BOOLEAN ignoreLock = FALSE ;

      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      _cmdType = (MSG_TYPE)pMsg->opCode ;
      _version = pQueryReq->version ;

      try
      {
         BSONObj dump( pQuery ) ;
         _boQuery = dump.getOwned() ;

         BSONObj dumpHint( pHint ) ;

         // check ignore lock
         rc = rtnGetBooleanElement( dumpHint,
                                    FIELD_NAME_IGNORE_LOCK,
                                    ignoreLock ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            // default is false
            ignoreLock = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_IGNORE_LOCK, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      if ( ignoreLock )
      {
         _lockMgr.setIgnoreLock( ignoreLock ) ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished initialization of query",
              contextID() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_INITQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_CLEAR, "_catContextBase::_clear" )
   INT32 _catContextBase::_clear ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_CLEAR ) ;

      INT16 w = _pCatCB->majoritySize() ;

      try
      {
         if ( _needClearAfterDone )
         {
            rc = _clearInternal( cb, w ) ;
         }
       }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished clear",
              contextID() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_CLEAR, rc ) ;
      return rc ;
   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_ONCTXDEL, "_catContextBase::_onCtxDelete" )
   INT32 _catContextBase::_onCtxDelete ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_ONCTXDEL ) ;

      switch ( _status )
      {
      case CAT_CONTEXT_CAT_DONE :
      case CAT_CONTEXT_CAT_ERROR :
         if ( _needRollback )
         {
            pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
            pmdEDUCB *cb = eduMgr->getEDUByID( eduID() ) ;
            rc = _rollback( cb ) ;
            PD_RC_CHECK ( rc, PDWARNING,
                          "Failed in catContext [%lld]: "
                          "failed to rollback when deleting context, rc: %d",
                          contextID(), rc ) ;
         }
         break ;
      default :
         break ;
      }

      _onDeleteEvent() ;
      _unregEventHandlers() ;

      PD_LOG( PDDEBUG,
              "catContext [%lld]: finished context delete",
              contextID() ) ;

   done :
      _setStatus( CAT_CONTEXT_END ) ;
      PD_TRACE_EXITRC ( SDB_CATCTXBASE_ONCTXDEL, rc ) ;
      return rc ;

   error :
      // No more rollback
      goto done ;
   }

   void _catContextBase::_changeStatusOnError ()
   {
      switch ( _status )
      {
      case CAT_CONTEXT_NEW :
         _setStatus( CAT_CONTEXT_END ) ;
         break ;
      case CAT_CONTEXT_LOCKING :
      case CAT_CONTEXT_CAT_DONE :
      case CAT_CONTEXT_CAT_ERROR :
      case CAT_CONTEXT_DATA_DONE :
      case CAT_CONTEXT_DATA_ERROR :
         _setStatus( CAT_CONTEXT_CLEANING ) ;
         break ;
      case CAT_CONTEXT_READY :
      case CAT_CONTEXT_PREEXECUTED :
         _setStatus( CAT_CONTEXT_CAT_ERROR ) ;
         break ;
      default :
         _setStatus( CAT_CONTEXT_END ) ;
         break ;
      }

      _isOpened = FALSE ;
   }

   void _catContextBase::_toString ( stringstream &ss )
   {
      ss << ",Status:" << _status ;
      if ( !_boQuery.isEmpty() )
      {
         ss << ",Query:" << _boQuery.toString() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__MAKEREPLY, "_catContextBase::_makeReply" )
   INT32 _catContextBase::_makeReply( CAT_CONTEXT_PHASE phase,
                                      rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__MAKEREPLY ) ;

      BSONObjBuilder builder ;

      if ( CAT_CONTEXT_PHASE_1 == phase )
      {
         rc = _buildP1Reply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 1 reply, "
                      "rc: %d", rc ) ;

         rc = _buildP1HandlerReply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 1 reply "
                      "for handlers, rc: %d", rc ) ;
      }
      else if ( CAT_CONTEXT_PHASE_2 == phase )
      {
         rc = _buildP2Reply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 2 reply, "
                      "rc: %d", rc ) ;

         rc = _buildP2HandlerReply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 2 reply "
                      "for handlers, rc: %d", rc ) ;
      }
      else if ( CAT_CONTEXT_PHASE_COMMIT == phase )
      {
         rc = _buildPCReply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase commit reply, "
                      "rc: %d", rc ) ;

         rc = _buildPCHandlerReply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase commit reply "
                      "for handlers, rc: %d", rc ) ;
      }

      buffObj = rtnContextBuf( builder.obj() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__MAKEREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__REGEVENTHANDLER, "_catContextBase::_regEventHandler" )
   INT32 _catContextBase::_regEventHandler( catCtxEventHandler *handler )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__REGEVENTHANDLER ) ;

      SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *temp = *iter ;
         if ( handler == temp )
         {
            PD_LOG( PDDEBUG, "Handler [%s] already registered to context [%s]",
                    handler->getName(), name() ) ;
            goto done ;
         }
      }

      try
      {
         _eventHandlers.push_back( handler ) ;
         PD_LOG( PDDEBUG, "Registered handler [%s] to context [%s]",
                 handler->getName(), name() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register event handler, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__REGEVENTHANDLER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__UNREGEVENTHANDLERS, "_catContextBase::_unregEventHandlers" )
   void _catContextBase::_unregEventHandlers()
   {
      PD_TRACE_ENTRY( SDB_CATCTXBASE__UNREGEVENTHANDLERS ) ;

      _eventHandlers.clear() ;

      PD_TRACE_EXIT( SDB_CATCTXBASE__UNREGEVENTHANDLERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__PARSEQUERYHANDLERS, "_catContextBase::_parseQueryForHandlers" )
   INT32 _catContextBase::_parseQueryForHandlers( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__PARSEQUERYHANDLERS ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->parseQuery( _boQuery, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse query on "
                      "handler [%s] of context [%s], rc: %d",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__PARSEQUERYHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__ONCHECKEVENT, "_catContextBase::_onCheckEvent" )
   INT32 _catContextBase::_onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                         _pmdEDUCB *cb,
                                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__ONCHECKEVENT ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->onCheckEvent( type, _targetName.c_str(), _boTarget, cb,
                                     w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] check event on "
                      "handler [%s] of context [%s], rc: %d",
                      SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__ONCHECKEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__ONEXECUTEEVENT, "_catContextBase::_onExecuteEvent" )
   INT32 _catContextBase::_onExecuteEvent( SDB_EVENT_OCCUR_TYPE type,
                                           _pmdEDUCB *cb,
                                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__ONEXECUTEEVENT ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->onExecuteEvent( type, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call [%s] execute event on "
                      "handler [%s] of context [%s], rc: %d",
                      SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__ONEXECUTEEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__ONCOMMITEVENT, "_catContextBase::_onCommitEvent" )
   INT32 _catContextBase::_onCommitEvent( SDB_EVENT_OCCUR_TYPE type,
                                          _pmdEDUCB *cb,
                                          INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__ONCOMMITEVENT ) ;

      // commit event must be handled for all handlers
      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         INT32 tmpRC = handler->onCommitEvent( type, cb, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to call [%s] commit event on "
                    "handler [%s] of context [%s], rc: %d",
                    SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                    handler->getName(), name(), tmpRC ) ;
            if ( SDB_OK == rc )
            {
               rc = tmpRC ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB_CATCTXBASE__ONCOMMITEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__ONROLLBACKEVENT, "_catContextBase::_onRollbackEvent" )
   INT32 _catContextBase::_onRollbackEvent( SDB_EVENT_OCCUR_TYPE type,
                                            _pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__ONROLLBACKEVENT ) ;

      // rollback event must be handled for all handlers
      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         INT32 tmpRC = handler->onRollbackEvent( type, cb, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to call [%s] rollback event on "
                    "handler [%s] of context [%s], rc: %d",
                    SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                    handler->getName(), name(), tmpRC ) ;
            if ( SDB_OK == rc )
            {
               rc = tmpRC ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB_CATCTXBASE__ONROLLBACKEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__ONDELETEEVENT, "_catContextBase::_onDeleteEvent" )
   void _catContextBase::_onDeleteEvent()
   {
      PD_TRACE_ENTRY( SDB_CATCTXBASE__ONDELETEEVENT ) ;

      // rollback event must be handled for all handlers
      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         handler->onDeleteEvent() ;
      }

      PD_TRACE_EXIT( SDB_CATCTXBASE__ONDELETEEVENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__BLDP1HANDLERREPLY, "_catContextBase::_buildP1HandlerReply" )
   INT32 _catContextBase::_buildP1HandlerReply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__BLDP1HANDLERREPLY ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->buildP1Reply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 1 reply for "
                      "handler [%s] of context [%s], rc: %d",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__BLDP1HANDLERREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__BLDP2HANDLERREPLY, "_catContextBase::_buildP2HandlerReply" )
   INT32 _catContextBase::_buildP2HandlerReply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__BLDP2HANDLERREPLY ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->buildP2Reply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase 2 reply for "
                      "handler [%s] of context [%s], rc: %d",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__BLDP2HANDLERREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE__BLDPCHANDLERREPLY, "_catContextBase::_buildPCHandlerReply" )
   INT32 _catContextBase::_buildPCHandlerReply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE__BLDPCHANDLERREPLY ) ;

      for ( CAT_CTX_EVENT_HANDLER_LIST_IT iter = _eventHandlers.begin() ;
            iter != _eventHandlers.end() ;
            ++ iter )
      {
         catCtxEventHandler *handler = *iter ;
         SDB_ASSERT( NULL != handler, "handler is invalid" ) ;

         rc = handler->buildPCReply( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build phase commit reply for "
                      "handler [%s] of context [%s], rc: %d",
                      handler->getName(), name(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXBASE__BLDPCHANDLERREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
