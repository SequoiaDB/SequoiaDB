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

   Source File Name = rtnContextTS.cpp

   Descriptive Name = RunTime Text Search Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/30/2017  YSD Split from rtnContextData.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextTS.hpp"
#include "rtn.hpp"
#include "pmdController.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   RTN_CTX_AUTO_REGISTER( _rtnContextTS, RTN_CONTEXT_TS, "TS")

   _rtnContextTS::_rtnContextTS( INT64 contextID, UINT64 eduID )
   : rtnContextMain( contextID, eduID )
   {
      _eduCB = NULL ;
      _remoteSessionID = 0 ;
      _subContext = NULL ;
      _remoteCtxID = -1 ;
      _enableRIDFilter = FALSE ;
   }

   _rtnContextTS::~_rtnContextTS()
   {
      // When delete the context, need to kill the context on the adapter also.
      rtnRemoteMessenger *messenger =
         pmdGetKRCB()->getRTNCB()->getRemoteMessenger() ;
      if ( messenger && ( -1 != _remoteCtxID ) && ( 0 != _remoteSessionID ) )
      {
         CHAR *msg = NULL ;
         INT32 bufSize = 0 ;
         INT32 rc = msgBuildKillContextsMsg( &msg, &bufSize, 0, 1,
                                             &_remoteCtxID, _eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build kill context message failed[%d]", rc ) ;
         }
         else
         {
            rc = messenger->send( _remoteSessionID, (const MsgHeader *)msg,
                                  _eduCB ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send kill context message to remote "
                       "failed[%d]", rc ) ;
            }
         }
         if ( msg )
         {
            msgReleaseBuffer( msg, _eduCB ) ;
         }
      }

      if ( _subContext )
      {
         _releaseSubContext( _subContext ) ;
         _subContext = NULL ;
      }
      _orderedContexts.clear() ;
   }

   const CHAR* _rtnContextTS::name() const
   {
      return "TS" ;
   }

   RTN_CONTEXT_TYPE _rtnContextTS::getType() const
   {
      return RTN_CONTEXT_TS ;
   }

   _dmsStorageUnit* _rtnContextTS::getSU()
   {
      return NULL ;
   }

   void _rtnContextTS::enableRIDFilter()
   {
      _enableRIDFilter = TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS_OPEN, "_rtnContextTS::open" )
   INT32 _rtnContextTS::open( const rtnQueryOptions &options,
                              pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS_OPEN ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      rc = messenger->prepareSession( eduCB, _remoteSessionID ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare remote task failed[ %d ]", rc ) ;

      // 1. Store the query items.
      // 2. Send the query to search engine adapter, and get the replay. Then
      //    call query interface again, to get a sub context id.
      _options = options ;
      rc = _options.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Get owned of query options failed[ %d ]",
                   rc ) ;
      _numToReturn = _options.getLimit() ;
      _numToSkip = _options.getSkip() ;
      _eduCB = eduCB ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Options for search: %s", options.toString().c_str() ) ;
#endif /* _DEBUG */
      rc = _queryRemote( options, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Query from remote failed[ %d ]", rc ) ;

      rc = _prepareNextSubContext( eduCB, FALSE ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            rc = SDB_OK ;
            goto done ;
         }
         else
         {
            PD_LOG( PDERROR, "Prepare next sub context failed[ %d ]", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS_OPEN, rc ) ;
      return rc ;
   error:
      _isOpened = FALSE ;
      _hitEnd = TRUE ;
      goto done ;
   }

   BOOLEAN _rtnContextTS::_requireExplicitSorting () const
   {
      return FALSE ;
   }

   INT32 _rtnContextTS::_prepareAllSubCtxDataByOrder( _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX, "_rtnContextTS::_getNonEmptyNormalSubCtx" )
   INT32 _rtnContextTS::_getNonEmptyNormalSubCtx( _pmdEDUCB *cb,
                                                  rtnSubContext*& subCtx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX ) ;

      while ( TRUE )
      {
         if ( _subContext->recordNum() <= 0 )
         {
            rc = _prepareSubCtxData( cb, -1 ) ;
            if ( rc != SDB_OK )
            {
               _releaseSubContext( _subContext ) ;
               _subContext = NULL ;
               if ( SDB_DMS_EOC != rc )
               {
                  goto error ;
               }
               else
               {
                  rc = _prepareNextSubContext( cb ) ;
                  if ( rc )
                  {
                     if ( SDB_DMS_EOC != rc )
                     {
                        PD_LOG( PDERROR, "Prepare next sub context "
                                "failed[ %d ]", rc ) ;
                     }
                     goto error ;
                  }
                  if ( !_subContext )
                  {
                     rc = SDB_DMS_EOC ;
                     goto error ;
                  }
                  continue ;
               }
            }
         }
         subCtx = _subContext ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextTS::_saveEmptyOrderedSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }

   INT32 _rtnContextTS::_saveEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }

   INT32 _rtnContextTS::_saveNonEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__PRERELEASESUBCTX, "_rtnContextTS::_preReleaseSubContext" )
   void _rtnContextTS::_preReleaseSubContext( rtnSubContext *subCtx )
   {
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__PRERELEASESUBCTX ) ;

      if ( NULL != subCtx && -1 != subCtx->contextID() )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnCB->contextDelete( subCtx->contextID(), pmdGetThreadEDUCB() ) ;
      }

      PD_TRACE_EXIT( SDB__RTNCONTEXTTS__PRERELEASESUBCTX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT, "_rtnContextTS::_prepareNextSubContext" )
   INT32 _rtnContextTS::_prepareNextSubContext( pmdEDUCB *eduCB,
                                                BOOLEAN getMore )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT ) ;
      MsgOpReply *reply = NULL ;
      INT32 flag = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 subContextID = -1 ;
      MsgHeader *msg = NULL ;
      INT32 msgSize = 0 ;
      INT32 numToReturn = 0 ;
      UINT64 reqID = 0 ;

      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      if ( getMore )
      {
         if ( -1 == _remoteCtxID )
         {
            PD_LOG( PDDEBUG, "Hit the end of data on search engine" ) ;
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         rc = msgBuildGetMoreMsg( (CHAR **)&msg, &msgSize, numToReturn,
                                  _remoteCtxID, reqID, eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Build get more message failed[ %d ]", rc ) ;

         msg->opCode = MSG_BS_GETMORE_REQ ;

         rc = messenger->send( _remoteSessionID, msg, eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Send message by remote messenger failed[ %d ]",
                      rc ) ;
      }

      rc = messenger->receive( _remoteSessionID, eduCB, reply ) ;
      PD_RC_CHECK( rc, PDERROR, "Receive reply by remote messenger failed"
                   "[ %d ]", rc ) ;

      rc = msgExtractReply( (CHAR *)reply, &flag, &_remoteCtxID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query respond message failed[ %d ]",
                   rc ) ;

      try
      {
         rc = flag ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               if ( 0 != objList.size() )
               {
                  PD_LOG_MSG( PDERROR, "Error returned from remote: %s",
                              objList.front().toString( FALSE, TRUE ).c_str() ) ;
               }
            }
            goto error ;
         }

         // 4 objects are expected: matcher, selector, order by, hint.
         if ( objList.size() != 4 )
         {
            PD_LOG( PDERROR, "Respond message size is wrong, expect[ %d ], "
                    "actual[ %d ]", 4, objList.size() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

      _options.clearFlag( FLG_QUERY_PARALLED ) ;

      // Do a query, and get another subcontext. Sorting is not required here.
      // It will be done in outside sort context.
      rc = rtnQuery( _options.getCLFullName(), objList[1], objList[0],
                     BSONObj(), objList[3], _options.getFlag(), eduCB,
                     0, -1, dmsCB, rtnCB, subContextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query data on collection[ %s ] failed[ %d ]",
                   _options.getCLFullName(), rc ) ;
      if ( _subContext )
      {
         _releaseSubContext( _subContext ) ;
         _subContext = NULL ;
      }

      if ( _enableRIDFilter )
      {
         SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
         rtnContextPtr dataContext ;
         rc = rtnCB->contextFind( subContextID, RTN_CONTEXT_DATA, dataContext,
                                  eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get context [%llu], rc: %d",
                      subContextID, rc ) ;
         dataContext->setResultSetFilter( &_ridFilter ) ;
      }

      _subContext = SDB_OSS_NEW rtnSubCLContext( _options.getOrderBy(), _keyGen,
                                                 subContextID ) ;
      if ( !_subContext )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for rtnSubCLContext failed, "
                 "size[ %d ]", sizeof( rtnSubCLContext ) ) ;
         goto error ;
      }
      // take control by sub-context
      subContextID = -1 ;

      rc = _checkSubContext( _subContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub context, rc: %d", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT, rc ) ;
      return rc ;
   error:
      if ( _subContext )
      {
         _releaseSubContext( _subContext ) ;
         _subContext = NULL ;
      }
      if ( -1 != subContextID )
      {
         rtnCB->contextDelete( subContextID, eduCB ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA, "_rtnContextTS::_prepareSubCtxData" )
   INT32 _rtnContextTS::_prepareSubCtxData( pmdEDUCB *cb,
                                            INT32 maxNumToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA ) ;
      rtnContextBuf contextBuf ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;

      if ( _subContext->recordNum() > 0 )
      {
         goto done ;
      }

      rc = rtnGetMore( _subContext->contextID(), maxNumToReturn,
                       contextBuf, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get more for context[ %lld ] failed[ %d ]",
                    _subContext->contextID(), rc ) ;
         }
         goto error ;
      }

      {
         _subContext->setBuffer( contextBuf ) ;
         BOOLEAN skipBuffer = FALSE ;
         rc = _processSubContext( _subContext, skipBuffer ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to process sub-context [%lld], "
                    "rc: %d", _subContext->contextID(), rc ) ;
            goto error ;
         }
         if ( skipBuffer )
         {
            _subContext->popAll() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__QUERYREMOTE, "_rtnContextTS::_queryRemote" )
   INT32 _rtnContextTS::_queryRemote( const rtnQueryOptions &options,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__QUERYREMOTE ) ;
      MsgHeader *queryMsg = NULL ;
      INT32 msgSize = 0 ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      // Format the message, and send it to search engine adapter.
      rc = options.toQueryMsg( (CHAR **)&queryMsg, msgSize, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message from options failed[ %d ]",
                   rc ) ;

      queryMsg->opCode = MSG_BS_QUERY_REQ ;

      rc = messenger->send( _remoteSessionID, queryMsg, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message by remote messenger failed[ %d ]",
                   rc ) ;

   done:
      if ( queryMsg )
      {
         msgReleaseBuffer( (CHAR *)queryMsg, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__QUERYREMOTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
