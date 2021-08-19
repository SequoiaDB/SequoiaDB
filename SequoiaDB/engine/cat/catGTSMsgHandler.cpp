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

   Source File Name = catGTSMsgHandler.cpp

   Descriptive Name = GTS message handler

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catGTSMsgHandler.hpp"
#include "catGTSMsgJob.hpp"
#include "catGTSDef.hpp"
#include "catalogueCB.hpp"
#include "catCommon.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "utilCommon.hpp"
#include "../bson/bson.h"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   // according to perf test, 2-threads almost has the best performance
   #define GTS_MSG_MAX_JOB                  ( 2 )
   #define GTS_MSG_JOB_TIMEOUT              ( 300 * OSS_ONE_SEC )

   struct _catGTSMsg
   {
      NET_HANDLE  netHandle ;
      MsgHeader   msg ;
   } ;

   _catGTSMsgHandler::_catGTSMsgHandler()
   {
      _gtsMgr = NULL ;
      _catCB = NULL ;
      _activeJobNum = 0 ;
      _maxJobNum = GTS_MSG_MAX_JOB ;
      _isControllerStarted = FALSE ;
   }

   _catGTSMsgHandler::~_catGTSMsgHandler()
   {
      while ( !_msgQueue.empty() )
      {
         _catGTSMsg* msg = NULL ;
         if ( _msgQueue.try_pop( msg ) && NULL != msg )
         {
            SDB_THREAD_FREE( msg ) ;
         }
      }
   }

   INT32 _catGTSMsgHandler::init()
   {
      INT32 rc = SDB_OK ;

      pmdKRCB* krcb = pmdGetKRCB() ;
      _catCB = krcb->getCATLOGUECB() ;
      _gtsMgr = _catCB->getCatGTSMgr() ;

      return rc ;
   }

   INT32 _catGTSMsgHandler::fini()
   {
      SDB_ASSERT( 0 == _activeJobNum, "active job num must be 0" ) ;
      return SDB_OK ;
   }

   INT32 _catGTSMsgHandler::active()
   {
      INT32 rc = SDB_OK ;

      rc = _ensureMsgJobController() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to ensure GTS msg job controller, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catGTSMsgHandler::deactive()
   {
      // clean queue
      while ( !_msgQueue.empty() )
      {
         _catGTSMsg* msg = NULL ;
         if ( _msgQueue.try_pop( msg ) && NULL != msg )
         {
            SDB_THREAD_FREE( msg ) ;
         }
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_CHECK_LOAD, "_catGTSMsgHandler::checkLoad" )
   void _catGTSMsgHandler::checkLoad()
   {
      UINT32 msgNum = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_MSG_HANDLER_CHECK_LOAD ) ;

      msgNum = _msgQueue.size() ;
      if ( msgNum > 0 )
      {
         while( _activeJobNum < _maxJobNum &&
                msgNum / 1000 > (UINT32)_activeJobNum )
         {
            ossScopedLock lock( &_jobLatch ) ;

            if ( SDB_OK == catStartGTSMsgJob( this, FALSE,
                                              GTS_MSG_JOB_TIMEOUT ) )
            {
               _activeJobNum++ ;
            }
            else
            {
               break ;
            }
         }
      }

      PD_LOG( PDDEBUG, "GTS active msg job: %d, msg num: %d", _activeJobNum, msgNum ) ;

      PD_TRACE_EXIT( SDB_GTS_MSG_HANDLER_CHECK_LOAD ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_JOB_EXIT, "_catGTSMsgHandler::jobExit" )
   void _catGTSMsgHandler::jobExit( BOOLEAN isController )
   {
      PD_TRACE_ENTRY ( SDB_GTS_MSG_HANDLER_JOB_EXIT ) ;

      ossScopedLock lock( &_jobLatch ) ;

      _activeJobNum-- ;
      if ( isController )
      {
         _isControllerStarted = FALSE ;
      }

      PD_TRACE_EXIT( SDB_GTS_MSG_HANDLER_JOB_EXIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_HANDLE_MSG, "_catGTSMsgHandler::postMsg" )
   INT32 _catGTSMsgHandler::postMsg( const NET_HANDLE& handle,
                                     const MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;
      _catGTSMsg* gtsMsg = NULL ;
      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER_HANDLE_MSG ) ;

      rc = _ensureMsgJobController() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to ensure GTS msg job controller, rc=%d", rc ) ;
         goto error ;
      }

      gtsMsg = (_catGTSMsg*)SDB_THREAD_ALLOC( sizeof( NET_HANDLE ) +
                                              msg->messageLength ) ;
      if ( NULL == gtsMsg )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc GTS msg(size = %d), rc=%d",
                 sizeof( NET_HANDLE ) + msg->messageLength, rc ) ;
         goto error ;
      }
      gtsMsg->netHandle = handle ;
      ossMemcpy( &(gtsMsg->msg), msg, msg->messageLength ) ;

      try
      {
         _msgQueue.push( gtsMsg ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to push GTS msg to queue, exception=%s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER_HANDLE_MSG, rc ) ;
      return rc ;
   error :
      SDB_THREAD_FREE( gtsMsg ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_POP_MSG, "_catGTSMsgHandler::popMsg" )
   BOOLEAN _catGTSMsgHandler::popMsg( INT64 timeout, _catGTSMsg*& gtsMsg )
   {
      BOOLEAN rc = FALSE ;
      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER_POP_MSG ) ;
      rc = _msgQueue.timed_wait_and_pop( gtsMsg, timeout ) ;
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER_POP_MSG, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_PROCESS_MSG, "_catGTSMsgHandler::processMsg" )
   INT32 _catGTSMsgHandler::processMsg( const _catGTSMsg* gtsMsg )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buf ;
      MsgOpReply reply ;
      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER_PROCESS_MSG ) ;
      pmdEDUCB* eduCB = pmdGetThreadEDUCB() ;
      MsgHeader* msg = (MsgHeader*)&( gtsMsg->msg ) ;

      PD_LOG( PDINFO, "Receive GTS msg: %d, length:%d",
              msg->opCode, msg->messageLength ) ;

      rc = primaryCheck() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "service deactive but received GTS request, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( _catCB->isDCReadonly() )
      {
         rc = SDB_CAT_CLUSTER_IS_READONLY ;
         PD_LOG( PDERROR, "Cluster is readonly, rc: %d", rc ) ;
         goto error ;
      }

      switch( msg->opCode )
      {
      case MSG_GTS_SEQUENCE_ACQUIRE_REQ:
         rc = _processSequenceAcquireMsg( msg, buf, eduCB ) ;
         break ;
      case MSG_GTS_SEQUENCE_CREATE_REQ:
         rc = _processSequenceCreateMsg( msg, eduCB ) ;
         break ;
      case MSG_GTS_SEQUENCE_DROP_REQ:
         rc = _processSequenceDropMsg( msg, eduCB ) ;
         break ;
      case MSG_GTS_SEQUENCE_ALTER_REQ:
         rc = _processSequenceAlterMsg( msg, eduCB ) ;
         break ;
      default:
         rc = SDB_UNKNOWN_MESSAGE ;
         PD_LOG( PDERROR, "Receive unknown msg[opCode:(%d)%d, len: %d, "
                 "tid: %d, reqID: %lld, nodeID: %u.%u.%u]",
                 IS_REPLY_TYPE(msg->opCode), GET_REQUEST_TYPE(msg->opCode),
                 msg->messageLength, msg->TID, msg->requestID,
                 msg->routeID.columns.groupID, msg->routeID.columns.nodeID,
                 msg->routeID.columns.serviceID ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      reply.header.messageLength = sizeof( MsgOpReply ) + buf.size() ;
      reply.header.opCode = MAKE_REPLY_TYPE( msg->opCode ) ;
      reply.header.requestID = msg->requestID ;
      reply.header.routeID.value = msg->routeID.value ;
      reply.header.TID = msg->TID ;
      reply.flags = rc ;
      reply.contextID = -1 ;
      reply.numReturned = buf.recordNum() ;
      reply.startFrom = buf.getStartFrom() ;

      if ( buf.size() > 0 )
      {
         rc = sendReply( gtsMsg->netHandle, &reply,
                         (void*) buf.data(), buf.size() ) ;
      }
      else
      {
         rc = sendReply( gtsMsg->netHandle, &reply ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to send GTS reply message [%d], rc: %d",
                 reply.header.opCode, rc ) ;
         // no way to handle sending failure, do not jump to error
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER_PROCESS_MSG, rc ) ;
      return rc ;
   error:
      {
         reply.header.messageLength = sizeof( MsgOpReply ) ;
         reply.header.opCode = MAKE_REPLY_TYPE( msg->opCode ) ;
         reply.header.requestID = msg->requestID ;
         reply.header.routeID.value = msg->routeID.value ;
         reply.header.TID = msg->TID ;
         reply.flags = rc ;
         reply.contextID = -1 ;
         reply.numReturned = 0 ;
         reply.startFrom = 0 ;
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            reply.startFrom = _catCB->getPrimaryNode() ;
         }

         INT32 tempRc = sendReply( gtsMsg->netHandle, &reply ) ;
         if ( SDB_OK != tempRc )
         {
            PD_LOG( PDERROR,
                    "Failed to send GTS reply message [%d], rc: %d",
                    reply.header.opCode, tempRc ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_PRIMARY_CHECK, "_catGTSMsgHandler::primaryCheck" )
   INT32 _catGTSMsgHandler::primaryCheck()
   {
      BOOLEAN isDelay = FALSE ;
      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER_PRIMARY_CHECK ) ;
      INT32 rc = _catCB->primaryCheck( pmdGetThreadEDUCB(), FALSE, isDelay ) ;
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER_PRIMARY_CHECK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER_SEND_REPLY, "_catGTSMsgHandler::sendReply" )
   INT32 _catGTSMsgHandler::sendReply( const NET_HANDLE& handle,
                                       MsgOpReply* reply,
                                       void* replyData,
                                       UINT32 replyDataLen )
   {
      INT32 rc = SDB_OK ;
      BSONObj errInfo ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER_SEND_REPLY ) ;
      SDB_ASSERT( NULL != reply, "reply should be not null" ) ;

      PD_LOG( PDDEBUG,
              "Sending reply message [%d] with rc [%d]",
              reply->header.opCode, reply->flags ) ;

      /// when error, but has no data, fill the error obj
      if ( reply->flags &&
           reply->header.messageLength == sizeof( MsgOpReply ) )
      {
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         errInfo = utilGetErrorBson( reply->flags,
                                     cb->getInfo( EDU_INFO_ERROR ) ) ;
         replyData = ( void* )errInfo.objdata() ;
         replyDataLen = (INT32)errInfo.objsize() ;
         reply->numReturned = 1 ;
         reply->header.messageLength += replyDataLen ;
      }

      if ( replyData )
      {
         rc = _catCB->netWork()->syncSend( handle, &(reply->header),
                                           replyData, replyDataLen ) ;
      }
      else
      {
         rc = _catCB->netWork()->syncSend( handle, &(reply->header) ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to send reply message [%d], rc: %d",
                 reply->header.opCode, rc ) ;
      }

      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER_SEND_REPLY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER__ENSURE_MSG_JOB_CTRL, "_catGTSMsgHandler::_ensureMsgJobController" )
   INT32 _catGTSMsgHandler::_ensureMsgJobController()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER__ENSURE_MSG_JOB_CTRL ) ;

      ossScopedLock lock( &_jobLatch ) ;

      // start controller job
      if ( !_isControllerStarted )
      {
         rc = catStartGTSMsgJob( this, TRUE, -1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to start GTS msg job controller, rc=%d", rc ) ;
            goto error ;
         }
         _activeJobNum++ ;
         _isControllerStarted = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER__ENSURE_MSG_JOB_CTRL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ACQUIRE, "_catGTSMsgHandler::_processSequenceAcquireMsg" )
   INT32 _catGTSMsgHandler::_processSequenceAcquireMsg( MsgHeader* msg, rtnContextBuf& buf, _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      const CHAR *name = NULL ;
      utilSequenceID ID = UTIL_SEQUENCEID_NULL ;
      BOOLEAN hasExpectValue = FALSE ;
      INT64 expectValue = 0 ;
      _catSequenceAcquirer acquirer ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ACQUIRE ) ;
      SDB_ASSERT( NULL != msg, "msg must be not null" ) ;
      SDB_ASSERT( NULL != eduCB, "eduCB must be not null" ) ;

      _catSequenceManager* seqMgr = _gtsMgr->getSequenceMgr() ;

      rc = msgExtractSequenceRequestMsg( (CHAR*) msg, options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      {
         BSONObjIterator iter( options );
         while ( iter.more() )
         {
            BSONElement ele = iter.next();
            if ( 0 == ossStrcmp( ele.fieldName(), CAT_SEQUENCE_NAME ) )
            {
               if ( String != ele.type() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                          ele.type(), CAT_SEQUENCE_NAME );
                  goto error;
               }
               name = ele.valuestr();
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), CAT_SEQUENCE_ID ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                          ele.type(), CAT_SEQUENCE_ID );
                  goto error;
               }
               ID = ele.Long();
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      CAT_SEQUENCE_EXPECT_VALUE ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                          ele.type(), CAT_SEQUENCE_EXPECT_VALUE );
                  goto error;
               }
               hasExpectValue = TRUE;
               expectValue = ele.Long();
            }
         }
      }

      if ( NULL == name )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "No sequence name" ) ;
         goto error ;
      }

      if ( hasExpectValue )
      {
         rc = seqMgr->adjustSequence( name, ID, expectValue, eduCB, 1 ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      rc = seqMgr->acquireSequence( name, ID, acquirer, eduCB,
                                    _catCB->majoritySize( TRUE ) ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      try
      {
         BSONObj result = BSON(
            CAT_SEQUENCE_NAME << name <<
            CAT_SEQUENCE_ID << (INT64)acquirer.ID <<
            CAT_SEQUENCE_NEXT_VALUE << acquirer.nextValue <<
            CAT_SEQUENCE_ACQUIRE_SIZE << acquirer.acquireSize <<
            CAT_SEQUENCE_INCREMENT << acquirer.increment
         ) ;
         buf = _rtnContextBuf( result ) ;
      }
      catch( std::exception & )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ACQUIRE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_CREATE, "_catGTSMsgHandler::_processSequenceCreateMsg" )
   INT32 _catGTSMsgHandler::_processSequenceCreateMsg( MsgHeader* msg, _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj options ;
      string name ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_CREATE ) ;
      SDB_ASSERT( NULL != msg, "msg must be not null" ) ;
      SDB_ASSERT( NULL != eduCB, "eduCB must be not null" ) ;

      _catSequenceManager* seqMgr = _gtsMgr->getSequenceMgr() ;

      rc = msgExtractSequenceRequestMsg( (CHAR*) msg, options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      ele = options.getField( CAT_SEQUENCE_NAME ) ;
      if ( String != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                 ele.type(), CAT_SEQUENCE_NAME ) ;
         goto error ;
      }

      name = ele.String() ;

      rc = seqMgr->createSequence( name, options, eduCB, _catCB->majoritySize( TRUE ) ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_CREATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_DROP, "_catGTSMsgHandler::_processSequenceDropMsg" )
   INT32 _catGTSMsgHandler::_processSequenceDropMsg( MsgHeader* msg, _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj options ;
      string name ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_DROP ) ;
      SDB_ASSERT( NULL != msg, "msg must be not null" ) ;
      SDB_ASSERT( NULL != eduCB, "eduCB must be not null" ) ;

      _catSequenceManager* seqMgr = _gtsMgr->getSequenceMgr() ;
      INT16 w = _catCB->majoritySize( TRUE ) ;

      rc = msgExtractSequenceRequestMsg( (CHAR*) msg, options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      ele = options.getField( CAT_SEQUENCE_NAME ) ;
      if ( String != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                 ele.type(), CAT_SEQUENCE_NAME ) ;
         goto error ;
      }
      name = ele.String() ;

      // clear sequence tasks
      catRemoveSequenceTasks( name.c_str(), eduCB, w ) ;

      rc = seqMgr->dropSequence( name, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_DROP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ALTER, "_catGTSMsgHandler::_processSequenceAlterMsg" )
   INT32 _catGTSMsgHandler::_processSequenceAlterMsg( MsgHeader* msg, _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj options ;
      string name ;

      PD_TRACE_ENTRY( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ALTER ) ;
      SDB_ASSERT( NULL != msg, "msg must be not null" ) ;
      SDB_ASSERT( NULL != eduCB, "eduCB must be not null" ) ;

      _catSequenceManager* seqMgr = _gtsMgr->getSequenceMgr() ;

      rc = msgExtractSequenceRequestMsg( (CHAR*) msg, options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      ele = options.getField( CAT_SEQUENCE_NAME ) ;
      if ( String != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                 ele.type(), CAT_SEQUENCE_NAME ) ;
         goto error ;
      }

      name = ele.String() ;

      rc = seqMgr->alterSequence( name, options, eduCB,
                                  _catCB->majoritySize( TRUE ), NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MSG_HANDLER__PROCESS_SEQ_ALTER, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

