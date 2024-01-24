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

   Source File Name = seAdptAgentSession.cpp

   Descriptive Name = Agent session on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptAgentSession.hpp"
#include "rtnContextBuff.hpp"
#include "utilCommon.hpp"
#include "seAdptCommand.hpp"

using engine::_pmdEDUCB ;

namespace seadapter
{
   BEGIN_OBJ_MSG_MAP( _seAdptAgentSession, _pmdAsyncSession)
      ON_MSG( MSG_BS_QUERY_REQ, _onOPMsg )
      ON_MSG( MSG_BS_GETMORE_REQ, _onOPMsg )
   END_OBJ_MSG_MAP()

   _seAdptAgentSession::_seAdptAgentSession( UINT64 sessionID )
   : _pmdAsyncSession( sessionID )
   {
      _contextIDHWM = 0 ;
   }

   _seAdptAgentSession::~_seAdptAgentSession()
   {
      for ( CTX_MAP_ITR itr = _ctxMap.begin(); itr != _ctxMap.end(); )
      {
         SDB_OSS_DEL itr->second ;
         _ctxMap.erase( itr++ ) ;
      }
   }

   SDB_SESSION_TYPE _seAdptAgentSession::sessionType() const
   {
      return SDB_SESSION_SE_AGENT ;
   }

   EDU_TYPES _seAdptAgentSession::eduType() const
   {
      return EDU_TYPE_SE_AGENT ;
   }

   void _seAdptAgentSession::onRecieve( const NET_HANDLE netHandle,
                                        MsgHeader *msg )
   {
   }

   BOOLEAN _seAdptAgentSession::timeout( UINT32 interval )
   {
      return FALSE ;
   }

   void _seAdptAgentSession::onTimer( UINT64 timerID, UINT32 interval )
   {
   }

   void _seAdptAgentSession::_onAttach()
   {
   }

   void _seAdptAgentSession::_onDetach()
   {
   }

   INT32 _seAdptAgentSession::_onOPMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      MsgOpReply reply ;
      const CHAR *msgBody = NULL ;
      INT32 bodySize = 0 ;
      utilCommObjBuff objBuff ;
      INT64 contextID = -1 ;

      rc = objBuff.init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Initialize result buffer failed[ %d ]", rc ) ;
      }
      else
      {
         switch ( msg->opCode )
         {
            case MSG_BS_QUERY_REQ:
               rc = _onQueryReq( msg, objBuff, contextID ) ;
               break ;
            case MSG_BS_GETMORE_REQ:
               rc = _onGetmoreReq( msg, objBuff, contextID ) ;
               break ;
            case MSG_BS_KILL_CONTEXT_REQ:
               rc = _onKillCtxReq( msg ) ;
               break ;
            default:
               rc = SDB_UNKNOWN_MESSAGE ;
               break ;
         }
      }

      reply.header.opCode = MAKE_REPLY_TYPE( msg->opCode ) ;
      reply.header.messageLength = sizeof( MsgOpReply ) ;
      reply.header.requestID = msg->requestID ;
      reply.header.TID = msg->TID ;
      reply.header.routeID.value = 0 ;
      reply.header.globalID = msg->globalID ;

      if ( objBuff.valid() )
      {
         if ( SDB_OK != rc )
         {
            if ( 0 == objBuff.getObjNum() )
            {
               _errorInfo =
                  utilGetErrorBson( rc, _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
               objBuff.appendObj( _errorInfo ) ;
            }
         }

         // Maybe data, maybe the error message.
         if ( objBuff.getObjNum() > 0 )
         {
            msgBody = objBuff.data() ;
            bodySize = objBuff.dataSize() ;
            reply.numReturned = objBuff.getObjNum() ;
         }
      }
      else
      {
         reply.numReturned = 0 ;
      }
      reply.contextID = contextID ;
      reply.flags = rc ;
      reply.header.messageLength += bodySize ;
      rc = _reply( &reply, handle, msgBody, bodySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Reply the message failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // Handle query message from data node.
   // It will analyze the original query, fetch the neccessary data, and rewrite
   // the items, then return the new message.
   INT32 _seAdptAgentSession::_onQueryReq( MsgHeader *msg,
                                           utilCommObjBuff &objBuff,
                                           INT64 &contextID,
                                           pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      const CHAR *pCollectionName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      seAdptContextQuery *context = NULL ;
      seAdptCommand *command = NULL ;

      rc = msgExtractQuery( (const CHAR *)msg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[ %s ] extract query message "
                   "failed[ %d ]", sessionName(), rc ) ;

      if ( !seAdptIsCommand( pCollectionName ) )
      {
         try
         {
            BSONObj matcher( pQuery ) ;
            BSONObj selector ( pFieldSelector ) ;
            BSONObj orderBy ( pOrderBy ) ;
            BSONObj hint ( pHint ) ;
            BSONObj newHint ;
            UINT16 indexID = SEADPT_INVALID_IMID ;

            rc = _selectIndex( pCollectionName, hint, newHint, indexID ) ;
            PD_RC_CHECK( rc, PDERROR, "Select index for collection[%s] failed[%d]",
                        pCollectionName, rc ) ;

            context = SDB_OSS_NEW seAdptContextQuery() ;
            if ( !context )
            {
               rc = SDB_OOM ;
               PD_LOG_MSG( PDERROR, "Allocate memory for query context failed, "
                           "size[ %d ]", sizeof( seAdptContextQuery ) ) ;
               goto error ;
            }

            rc = context->open( pCollectionName, indexID, matcher, selector,
                                orderBy, newHint, objBuff, eduCB ) ;
            if ( rc || context->eof() )
            {
               contextID = -1 ;
               SDB_OSS_DEL context ;
               context = NULL ;
               if ( SDB_OK == rc )
               {
                  goto done ;
               }
               else if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG_MSG( PDERROR, "Open context for rewrite query "
                              "failed[%d]", rc ) ;
               }
               goto error ;
            }
            contextID = _contextIDHWM++ ;
            _ctxMap[ contextID ] = context ;
         }
         catch ( std::exception &e )
         {
            if ( context )
            {
               SDB_OSS_DEL context ;
               context = NULL ;
            }
            PD_LOG_MSG( PDERROR, "Session[ %s ] create BSON objects for query "
                        "items failed: %s", sessionName(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         rc = seAdptGetCommand( pCollectionName, command ) ;
         PD_RC_CHECK( rc, PDERROR, "Get command instance for[%s] failed[%d]",
                      pCollectionName, rc ) ;
         rc = command->init( flag, numToSkip, numToReturn, pQuery,
                             pFieldSelector, pOrderBy, pHint ) ;
         PD_RC_CHECK( rc, PDERROR, "Init command[%s] failed[%d]",
                      pCollectionName, rc ) ;

         rc = command->doit( eduCB, objBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute command[%s] failed[%d]",
                      pCollectionName, rc ) ;
      }

   done:
      if ( command )
      {
         seAdptReleaseCommand( command ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // Generate another new message.
   INT32 _seAdptAgentSession::_onGetmoreReq( MsgHeader *msg,
                                             utilCommObjBuff &objBuff,
                                             INT64 &contextID,
                                             pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      seAdptContextBase *context = NULL ;

      contextID = ((MsgOpGetMore *)msg)->contextID ;

      CTX_MAP_ITR itr = _ctxMap.find( contextID ) ;
      if ( _ctxMap.end() == itr )
      {
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         PD_LOG( PDERROR, "Context %lld does not exist", contextID ) ;
         goto error ;
      }

      context = itr->second ;
      rc = context->getMore( 1, objBuff ) ;
      if ( rc || context->eof() )
      {
         contextID = -1 ;
         SDB_OSS_DEL context ;
         _ctxMap.erase( itr ) ;
         context = NULL ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG_MSG( PDERROR, "Get more rewrite query failed[%d]", rc ) ;
            }
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptAgentSession::_onKillCtxReq( MsgHeader *msg, pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      INT32 contextNum = 0 ;
      const INT64 *contextIDs = NULL ;

      rc = msgExtractKillContexts( (const CHAR *)msg, &contextNum,
                                   &contextIDs )  ;
      PD_RC_CHECK( rc, PDERROR, "Parse kill context message failed[%d]", rc ) ;

      for ( INT32 i = 0; i < contextNum; ++i )
      {
         CTX_MAP_ITR itr = _ctxMap.find( contextIDs[i] ) ;
         if ( itr != _ctxMap.end() )
         {
            SDB_OSS_DEL itr->second ;
            _ctxMap.erase( itr ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // Send reply message. It contains a header, and an optinal body.
   INT32 _seAdptAgentSession::_reply( MsgOpReply *header, NET_HANDLE handle,
                                      const CHAR *buff, UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( size > 0 )
      {
         rc = routeAgent()->syncSend( handle, (MsgHeader *)header,
                                      (void *)buff, size ) ;
      }
      else
      {
         rc = routeAgent()->syncSend( handle, (MsgHeader *)header ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Session[ %s ] send reply message failed[ %d ]",
                   sessionName(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptAgentSession::_defaultMsgFunc ( NET_HANDLE handle,
                                                MsgHeader * msg )
   {
      return _onOPMsg( handle, msg ) ;
   }

   INT32 _seAdptAgentSession::_selectIndex( const CHAR *clName,
                                            const BSONObj &hint,
                                            BSONObj &newHint,
                                            UINT16 &indexID )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      UINT32 textIdxNum = 0 ;
      map<string, UINT16> idxNameMap ;
      UINT16 imID = SEADPT_INVALID_IMID ;

      indexID = SEADPT_INVALID_IMID ;
      sdbGetSeAdapterCB()->getIdxMetaMgr()->getIdxNamesByCL( clName,
                                                             idxNameMap ) ;
      if ( 0 == idxNameMap.size() )
      {
         rc = SDB_RTN_INDEX_NOTEXIST ;
         PD_LOG_MSG( PDERROR, "No index information found for collection[%s]",
                     clName ) ;
         goto error ;
      }

      try
      {
         // Traverse the hint to find text indices. They should be removed from
         // hint.
         BSONObjIterator itr( hint ) ;
         while ( itr.more() )
         {
            string esIdxName ;
            BSONElement ele = itr.next() ;
            string idxName = ele.str() ;
            if ( idxName.empty() )
            {
               continue ;
            }

            map<string, UINT16>::iterator itr =
                  idxNameMap.find( idxName.c_str() )  ;
            if ( itr != idxNameMap.end() )
            {
               // If multiple text indices specified in the hint, use the first
               // one.
               if ( 0 == textIdxNum )
               {
                  imID = itr->second ;
               }
               textIdxNum++ ;
            }
            else
            {
               builder.append( ele ) ;
            }
         }

         // If no text index in the hint, and there is only one text index on
         // the collection, use it for search. Otherwise, report error.
         if ( 0 == textIdxNum )
         {
            if ( 1 == idxNameMap.size() )
            {
               imID = idxNameMap.begin()->second ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Text index name should be specified in "
                                    "hint when there are multiple text indices") ;
               goto error ;
            }
         }

         newHint = builder.obj() ;
         indexID = imID ;

         PD_LOG( PDDEBUG, "Original hint[ %s ], new hint[ %s ]",
                 hint.toString( FALSE, TRUE ).c_str(),
                 newHint.toString( FALSE, TRUE ).c_str() ) ;
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
}

