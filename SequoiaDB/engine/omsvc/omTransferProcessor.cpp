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

   Source File Name = omTransferProcessor.cpp

   Descriptive Name = om transfer message Processor source file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/09/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdStartup.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "omManager.hpp"
#include "msgMessage.hpp"
#include "omDef.hpp"
#include "omContextTransfer.hpp"
#include "omTransferProcessor.hpp"

namespace engine
{

   _omTransferProcessor::_omTransferProcessor( list< _omNodeInfo > &nodeList )
   {
      _nodeList = nodeList ;
   }

   _omTransferProcessor::~_omTransferProcessor()
   {
   }

   void _omTransferProcessor::_clearRemoteSession( 
                                             pmdRemoteSessionMgr *rsManager,
                                             pmdRemoteSession *remoteSession )
   {
      if ( NULL != remoteSession && NULL != rsManager )
      {
         remoteSession->clearSubSession() ;
         rsManager->removeSession( remoteSession ) ;
      }
   }

   INT32 _omTransferProcessor::_sendMsg2Target( const omNodeInfo &nodeInfo,
                                                MsgHeader *msg, 
                                                omSdbConnector **connector,
                                                MsgHeader **result )
   {
      SDB_ASSERT( NULL != connector && NULL != result, "connector or result "
      "can't be null" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *reply = NULL ;
      omSdbConnector *conn = SDB_OSS_NEW omSdbConnector() ;
      if ( NULL == conn )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "out of memory!" ) ;
         goto error ;
      }

      rc = conn->init( nodeInfo.hostName, ossAtoi( nodeInfo.service.c_str() ), 
                       nodeInfo.user, nodeInfo.passwd, 
                       nodeInfo.preferedInstance ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "initial the connection failed:host=%s,port=%s,rc=%d", 
                 nodeInfo.hostName.c_str(), nodeInfo.service.c_str(), rc ) ;
         goto error ;
      }

      rc = conn->sendMessage( msg ) ;
      if ( SDB_OK != rc )
      {  
         PD_LOG( PDERROR, "send msg to target failed:host=%s,port=%s,rc=%d", 
                 nodeInfo.hostName.c_str(), nodeInfo.service.c_str(), rc ) ;
         goto error ;
      }

      rc = conn->recvMessage( &reply ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "receive reply failed:rc=%d", rc ) ;
         goto error ;
      }

      *connector = conn ;
      *result    = reply ;

   done:
      return rc ;
   error:
      if ( NULL != conn )
      {
         conn->close() ;
         SDB_OSS_DEL conn ;
         conn = NULL ;
      }
      if ( NULL != reply )
      {
         SDB_OSS_FREE( reply ) ;
         reply = NULL ;
      }

      goto done ;
   }

   INT32 _omTransferProcessor::processMsg( MsgHeader *msg,
                                           rtnContextBuf &contextBuff,
                                           INT64 &contextID,
                                           BOOLEAN &needReply )
   {
      pmdKRCB *pKrcb       = pmdGetKRCB();
      SDB_RTNCB *pRtncb    = pKrcb->getRTNCB();
      INT32 rc             = SDB_OK ;
      omSdbConnector *conn = NULL ;
      MsgHeader *result    = NULL ;
      rtnContext *pContext = NULL ;
      omManager *om = sdbGetOMManager() ;
      MsgRouteID routeID ;
      _omContextTransfer *pTmpContext = NULL ;

      contextID = -1 ;
      list< omNodeInfo >::iterator iter = _nodeList.begin() ;

      if ( _nodeList.size() <= 0 )
      {
         SDB_ASSERT( _nodeList.size() > 0, "nodelist can't be 0" ) ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "nodeList is 0" ) ;
         goto error ;
      }

      while ( iter != _nodeList.end() )
      {
         routeID = om->updateAgentInfo( iter->hostName, iter->service ) ;
         rc = _sendMsg2Target( *iter, msg, &conn, &result ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }

         PD_LOG( PDERROR, "send message to target failed:host=%s,port=%s",
                 iter->hostName.c_str(), iter->service.c_str() ) ;
         iter++ ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "all nodes is failure." ) ;
         goto error ;
      }

      rc = pRtncb->contextNew( RTN_CONTEXT_OM_TRANSFER,
                               &pContext, contextID, eduCB() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                   rc ) ;

      pTmpContext = ( _omContextTransfer *)pContext ;
      rc = pTmpContext->open( conn, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "open context failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != result )
      {
         SDB_OSS_FREE( result ) ;
         result = NULL ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, eduCB() ) ;
         contextID = -1 ;
      }
      else
      {
         if ( NULL != conn )
         {
            conn->close() ;
            SDB_OSS_DEL conn ;
            conn = NULL ;
         }
      }
      goto done ;
   }

   SDB_PROCESSOR_TYPE _omTransferProcessor::processorType() const
   {
      return SDB_PROCESSOR_OM ;
   }

   const CHAR* _omTransferProcessor::processorName() const
   {
      return "transferProcessor" ;
   }

   void _omTransferProcessor::attach( pmdSession *session )
   {
      attachSession( session ) ;
   }

   void _omTransferProcessor::detach()
   {
      detachSession() ;
   }

   void _omTransferProcessor::_onAttach()
   {
   }

   void _omTransferProcessor::_onDetach()
   {
   }
}

