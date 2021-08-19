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

      _executeAfterLock = FALSE ;
      _commitAfterExecute = FALSE ;
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
                                 rtnContextBuf &buffObj,
                                 _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( _status == CAT_CONTEXT_NEW,
                   "Wrong catalog status before opening" ) ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE_OPEN ) ;

      _isOpened = TRUE ;

      rc = _initQuery( handle, pMsg, pQuery, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to initialize query, rc: %d",
                   contextID(), rc) ;

      rc = _parseQuery( cb ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to parse query, rc: %d",
                   contextID(), rc ) ;

      _setStatus( CAT_CONTEXT_LOCKING ) ;

      rc = _checkContext( cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in catContext [%lld]: "
                   "failed to check and lock catalog objects, rc: %d",
                   contextID(), rc ) ;

      if ( _executeAfterLock )
      {
         if ( _needPreExecute )
         {
            rc = _preExecute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to execute catalog command], rc: %d",
                         contextID(), rc ) ;
         }
         else
         {
            rc = _execute( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed in catContext [%lld]: "
                         "failed to execute catalog command], rc: %d",
                         contextID(), rc ) ;
         }
      }

      rc = _makeReply( buffObj ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed in catContext [%lld]: "
                    "failed to make reply message, rc: %d",
                    contextID(), rc ) ;

      PD_LOG( PDDEBUG,
              "catContext [%lld]: open finished",
              contextID() ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXBASE_OPEN, rc ) ;
      return rc ;

   error :
      _changeStatusOnError() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXBASE_PREPAREDATA, "_catContextBase::_prepareData" )
   INT32 _catContextBase::_prepareData ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;

      PD_TRACE_ENTRY( SDB_CATCTXBASE_PREPAREDATA ) ;

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

      rc = _makeReply( buffObj ) ;
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

      try
      {
         rc = _checkInternal( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to check context internal, rc: %d",
                      contextID(), rc ) ;

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

         // Continue to commit if needed
         if ( _commitAfterExecute )
         {
            rc = _commit( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR,
                       "Failed in catContext [%lld]: "
                       "failed to commit catalog changes, rc: %d",
                       contextID(), rc ) ;
            }
         }
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

      _setStatus( CAT_CONTEXT_DATA_DONE ) ;

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

         rc = _rollbackInternal( cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed in catContext [%lld]: "
                      "failed to rollback context, rc: %d",
                      contextID(), rc ) ;
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
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXBASE_INITQUERY ) ;

      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      _cmdType = (MSG_TYPE)pMsg->opCode ;
      _version = pQueryReq->version ;

      try
      {
         BSONObj dump( pQuery ) ;
         _boQuery = dump.getOwned() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
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
}
