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

   Source File Name = omagentSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentSession.hpp"
#include "omagentMgr.hpp"
#include "utilCommon.hpp"
#include "msgMessage.hpp"
#include "omagentHelper.hpp"
#include "omagentMsgDef.hpp"
#include "omagentUtil.hpp"
#include "msgAuth.hpp"
#include "../bson/bson.h"
#include "../bson/lib/md5.hpp"

using namespace bson ;

namespace engine
{
   /*
      Local define
   */
   #define OMAGENT_SESESSION_TIMEOUT         ( 120 )

   /*
      _omaSession implement
   */
   BEGIN_OBJ_MSG_MAP( _omaSession, _pmdAsyncSession )
      // msg map or event map
      ON_MSG( MSG_CM_REMOTE, _onNodeMgrReq )
      ON_MSG( MSG_AUTH_VERIFY_REQ, _onAuth )
      ON_MSG( MSG_AUTH_VERIFY1_REQ, _onAuth )
      ON_MSG( MSG_BS_QUERY_REQ, _onOMAgentReq )
   END_OBJ_MSG_MAP()

   _omaSession::_omaSession( UINT64 sessionID )
   :_pmdAsyncSession( sessionID )
   {
      ossMemset( (void*)&_replyHeader, 0, sizeof(_replyHeader) ) ;
      _pNodeMgr   = NULL ;
      ossMemset( _detailName, 0, sizeof( _detailName ) ) ;
      sdbGetOMAgentMgr()->incSession() ;
      _maxFileObjID = 0 ;
   }

   _omaSession::~_omaSession()
   {
      sdbGetOMAgentMgr()->decSession() ;
   }

   const CHAR* _omaSession::sessionName() const
   {
      if ( _detailName[0] )
      {
         return _detailName ;
      }
      return _pmdAsyncSession::sessionName() ;
   }

   SDB_SESSION_TYPE _omaSession::sessionType() const
   {
      return SDB_SESSION_OMAGENT ;
   }

   EDU_TYPES _omaSession::eduType() const
   {
      return EDU_TYPE_OMAAGENT ;
   }

   void _omaSession::onRecieve( const NET_HANDLE netHandle, MsgHeader * msg )
   {
      ossGetCurrentTime( _lastRecvTime ) ;
      sdbGetOMAgentMgr()->resetNoMsgTimeCounter() ;
   }

   BOOLEAN _omaSession::timeout ( UINT32 interval )
   {
      BOOLEAN ret = FALSE ;
      ossTimestamp curTime ;
      ossGetCurrentTime ( curTime ) ;

      if ( sdbGetOMAgentOptions()->isStandAlone() )
      {
         // will be release
         ret = TRUE ;
         goto done ;
      }
      else if ( curTime.time - _lastRecvTime.time > OMAGENT_SESESSION_TIMEOUT )
      {
         // will be release
         ret = TRUE ;
         goto done ;
      }

   done :
      return ret ;
   }

   void _omaSession::onTimer( UINT64 timerID, UINT32 interval )
   {
   }

   INT32 _omaSession::newFileObj( UINT32 &fID, sptUsrFileCommon** fileObj )
   {
      INT32 rc = SDB_OK ;
      sptUsrFileCommon *fileCommon =  NULL ;
      if( NULL == fileObj )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      fileCommon = SDB_OSS_NEW sptUsrFileCommon ;
      if( NULL == fileCommon )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      _maxFileObjID++ ;
      fID = _maxFileObjID ;
      _fileObjMap.insert( std::pair< UINT32,
                                     sptUsrFileCommon* >( fID, fileCommon ) ) ;
      *fileObj = fileCommon ;
   done:
      return rc ;
   error:
      goto done ;
   }

   void _omaSession::releaseFileObj( UINT32 fID )
   {
      std::map< UINT32, sptUsrFileCommon* >::iterator it ;
      it = _fileObjMap.find( fID ) ;
      if( it == _fileObjMap.end() )
      {
         PD_LOG( PDWARNING, "Failed to remove file obj, file obj not exist,"
                 " fID: %u", fID ) ;
      }
      else
      {
         if( it->second )
         {
            SDB_OSS_DEL it->second ;
            it->second = NULL ;
         }
         _fileObjMap.erase( it ) ;
      }
   }

   sptUsrFileCommon* _omaSession::getFileObjByID( UINT32 fID )
   {
      std::map< UINT32, sptUsrFileCommon* >::iterator it ;
      sptUsrFileCommon* pFileObj = NULL ;

      it = _fileObjMap.find( fID ) ;
      if( it == _fileObjMap.end() )
      {
         PD_LOG( PDWARNING, "Failed to find file obj, fID: %u", fID ) ;
      }
      else
      {
         pFileObj = it->second ;
      }
      return pFileObj ;
   }

   void _omaSession::_onDetach()
   {
      /// clear self scopes
      sdbGetOMAgentMgr()->clearScopeBySession() ;
      /// clear open file
      _clearFileObjMap() ;
   }

   void _omaSession::_onAttach()
   {
      ossSnprintf( _detailName, SESSION_NAME_LEN, "%s,R-IP:%s,R-Port:%u",
                   _pmdAsyncSession::sessionName(), _client.getPeerIPAddr(),
                   _client.getPeerPort() ) ;
      eduCB()->setName( _detailName ) ;
      /// register edu exit hook func
      pmdSetEDUHook( (PMD_ON_EDU_EXIT_FUNC)sdbHookFuncOnThreadExit ) ;
      _pNodeMgr = sdbGetOMAgentMgr()->getNodeMgr() ;
   }

   INT32 _omaSession::_defaultMsgFunc( NET_HANDLE handle, MsgHeader * msg )
   {
      PD_LOG( PDWARNING, "Session[%s] Recieve unknow msg[type:[%d]%u, len:%u]",
              sessionName(), IS_REPLY_TYPE( msg->opCode ) ? 1 : 0,
              GET_REQUEST_TYPE( msg->opCode ), msg->messageLength ) ;

      return _reply( SDB_CLS_UNKNOW_MSG, msg ) ;
   }

   INT32 _omaSession::_reply( MsgOpReply *header, const CHAR *pBody,
                              INT32 bodyLen )
   {
      INT32 rc = SDB_OK ;

      if ( (UINT32)(header->header.messageLength) !=
           sizeof (MsgOpReply) + bodyLen )
      {
         PD_LOG ( PDERROR, "Session[%s] reply message length error[%u != %u]",
                  sessionName() ,header->header.messageLength,
                  sizeof ( MsgOpReply ) + bodyLen ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      //Send message
      if ( bodyLen > 0 )
      {
         rc = routeAgent()->syncSend ( _netHandle, (MsgHeader *)header,
                                       (void*)pBody, bodyLen ) ;
      }
      else
      {
         rc = routeAgent()->syncSend ( _netHandle, (MsgHeader *)header ) ;
      }

      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Session[%s] send reply message failed[rc:%d]",
                  sessionName(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaSession::_reply( INT32 flags, MsgHeader * pSrcReqMsg,
                              const CHAR *pBody, const INT32 *bodyLen )
   {
      const CHAR *body = pBody ;
      INT32 bLen = ( NULL == bodyLen ) ? 0 : *bodyLen ;

      //Build reply message
      _replyHeader.header.opCode = MAKE_REPLY_TYPE( pSrcReqMsg->opCode ) ;
      _replyHeader.header.messageLength = sizeof ( MsgOpReply ) + bLen ;
      _replyHeader.header.requestID = pSrcReqMsg->requestID ;
      _replyHeader.header.TID = pSrcReqMsg->TID ;
      _replyHeader.header.routeID.value = 0 ;
      _replyHeader.header.globalID = pSrcReqMsg->globalID ;
      _replyHeader.flags = flags ;
      _replyHeader.contextID = -1 ;

      /// when we have more than one record to return,
      /// rewrite here.
      _replyHeader.numReturned = ( ( SINT32 )sizeof( MsgOpReply )
                                          < _replyHeader.header.messageLength )
                                 ?  1 : 0 ;
      _replyHeader.startFrom = 0 ;

      if ( flags )
      {
         _errorInfo = utilGetErrorBson( flags, _pEDUCB->getInfo(
                                        EDU_INFO_ERROR ) ) ;
         bLen  = _errorInfo.objsize() ;
         body    = _errorInfo.objdata() ;
         _replyHeader.header.messageLength += bLen ;
         _replyHeader.numReturned = 1 ;
      }

      return _reply( &_replyHeader, body, bLen ) ;
   }

   INT32 _omaSession::_onAuth( const NET_HANDLE & handle, MsgHeader * pMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement user, pass ;

      rc = extractAuthMsg( pMsg, obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Extrace auth msg failed, rc: %d", rc ) ;

      user = obj.getField( SDB_AUTH_USER ) ;
      pass = obj.getField( SDB_AUTH_PASSWD ) ;

      // check usr and passwd
      if ( 0 != ossStrcmp( user.valuestrsafe(), SDB_OMA_USER ) )
      {
         PD_LOG( PDERROR, "User name[%s] is not support",
                 user.valuestrsafe() ) ;
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }

      if ( md5::md5simpledigest( string( SDB_OMA_USERPASSWD ) ) !=
           string( pass.valuestrsafe() ) )
      {
         PD_LOG( PDERROR, "User password[%s] is not error",
                 pass.valuestrsafe() ) ;
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }

      getClient()->authenticate( user.valuestrsafe(), pass.valuestrsafe() ) ;

   done:
      return _reply( rc, pMsg ) ;
   error:
      goto done ;
   }

   INT32 _omaSession::_onNodeMgrReq( const NET_HANDLE & handle,
                                     MsgHeader * pMsg )
   {
      INT32 rc = SDB_OK ;
      MsgCMRequest *pCMReq = ( MsgCMRequest* )pMsg ;
      INT32 remoteCode = 0 ;
      const CHAR *arg1 = NULL ;
      const CHAR *arg2 = NULL ;
      const CHAR *arg3 = NULL ;
      const CHAR *arg4 = NULL ;
      BSONObj obj ;
      const CHAR *body = NULL ;
      INT32 bodyLen = 0 ;

      if ( sdbGetOMAgentOptions()->isStandAlone() )
      {
         rc = SDB_PERM ;
         goto done ;
      }

      if ( pMsg->messageLength < (SINT32)sizeof (MsgCMRequest) )
      {
         PD_LOG( PDERROR, "Session[%s] recieve invalid msg[opCode: %d, "
                 "len: %d]", sessionName(), pMsg->opCode,
                 pMsg->messageLength ) ;
         goto done ;
      }

      rc = msgExtractCMRequest ( ( const CHAR*)pMsg, &remoteCode, &arg1, &arg2,
                                 &arg3, &arg4 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Session[%s]failed to extract cm request, rc: %d",
                  sessionName(), rc ) ;
         goto done ;
      }

      switch( pCMReq->remoCode )
      {
         case SDBSTART :
            rc = _pNodeMgr->startANode( arg1 ) ;
            break ;
         case SDBSTOP :
            rc = _pNodeMgr->stopANode( arg1 ) ;
            break ;
         case SDBADD :
            rc = _pNodeMgr->addANode( arg1, arg2 ) ;
            break ;
         case SDBMODIFY :
            rc = _pNodeMgr->mdyANode( arg1 ) ;
            break ;
         case SDBRM :
            rc = _pNodeMgr->rmANode( arg1, arg2 ) ;
            break ;
         case SDBSTARTALL :
            rc = _pNodeMgr->startAllNodes( NODE_START_CLIENT ) ;
            break ;
         case SDBSTOPALL :
            rc = _pNodeMgr->stopAllNodes() ;
            break ;
         case SDBGETCONF :
         {
            rc = _pNodeMgr->getOptions( arg1, obj ) ;
            if ( SDB_OK == rc && !obj.isEmpty() )
            {
               body = obj.objdata() ;
               bodyLen = obj.objsize() ;
            }
            break ;
         }
         case SDBCLEARDATA :
            rc = _pNodeMgr->clearData( arg1 ) ;
            break ;
         case SDBTEST :
            rc = SDB_OK ;
            break ;
         default :
            PD_LOG( PDERROR, "Unknow remote code[%d] in session[%s]",
                    pCMReq->remoCode, sessionName() ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s] process remote code[%d] failed, rc: %d",
                 sessionName(), pCMReq->remoCode, rc ) ;
      }

   done:
      return _reply( rc, pMsg, body, &bodyLen ) ;
   }

   INT32 _omaSession::_onOMAgentReq( const NET_HANDLE &handle,
                                     MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      INT32 flags               = 0 ;
      const CHAR *pCollectionName   = NULL ;
      const CHAR *pQuery         = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderByBuffer = NULL ;
      const CHAR *pHintBuffer    = NULL ;
      SINT64 numToSkip          = -1 ;
      SINT64 numToReturn        = -1 ;
      _omaCommand *pCommand     = NULL ;
      INT64 sec                 = 0 ;
      INT64 microSec            = 0 ;
      INT64 tkTime              = 0 ;
      ossTimestamp tmBegin ;
      ossTimestamp tmEnd ;
      BSONObj retObj ;
      BSONObjBuilder builder ;

      PD_LOG ( PDDEBUG, "Omagent receive requset from omsvc" ) ;
      // compute the time takes
      ossGetCurrentTime( tmBegin ) ;
      // build reply massage header
      _buildReplyHeader( pMsg ) ;
      // extract command
      rc = msgExtractQuery ( (const CHAR *)pMsg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQuery,
                             &pFieldSelector, &pOrderByBuffer,
                             &pHintBuffer ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Session[%s] extract omsvc's command msg failed, "
                  "rc: %d", sessionName(), rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // handle command
      if ( omaIsCommand ( pCollectionName ) )
      {
         PD_LOG( PDDEBUG, "Omagent receive command: %s, argument: %s",
                 pCollectionName, BSONObj(pQuery).toString().c_str() ) ;

         rc = omaParseCommand ( pCollectionName, &pCommand ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to parse omsvc's command[%s], rc:%d",
                    pCollectionName, rc ) ;
            goto error ;
         }
         rc = omaInitCommand( pCommand, flags, numToSkip, numToReturn,
                              pQuery, pFieldSelector, pOrderByBuffer,
                              pHintBuffer ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init omsvc's command[%s] for omagent, "
                    "rc: %d", pCollectionName, rc ) ;
            goto error ;
         }
         rc = omaRunCommand( pCommand, retObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to run omsvc's command[%s], rc: %d",
                    pCollectionName, rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Omsvc's request[%s] is not a command",
                  pCollectionName ) ;
         goto error ;
      }

      // consturct reply
      builder.appendElements( retObj ) ;

   done :
      // release command
      if ( pCommand )
      {
         omaReleaseCommand( &pCommand ) ;
      }
      // reply
      retObj = builder.obj() ;
      _replyHeader.header.messageLength += retObj.objsize() ;
      _replyHeader.numReturned = 1 ;
      _replyHeader.flags = rc ;

      ossGetCurrentTime ( tmEnd ) ;
      // time takes
      tkTime = ( tmEnd.time * 1000000 + tmEnd.microtm ) -
               ( tmBegin.time * 1000000 + tmBegin.microtm ) ;
      sec = tkTime/1000000 ;
      microSec = tkTime%1000000 ;
      PD_LOG ( PDDEBUG, "Excute command[%s] return: %s",
               pCollectionName, retObj.toString(FALSE, TRUE).c_str() ) ;
      PD_LOG ( PDDEBUG, "Excute command[%s] takes %lld.%llds.",
               pCollectionName, sec, microSec ) ;

      // reply message
      return _reply( &_replyHeader, retObj.objdata(), retObj.objsize() ) ;
   error :
      // check flags
      if ( rc < -SDB_MAX_ERROR || rc > SDB_MAX_WARNING )
      {
         PD_LOG ( PDERROR, "Error code is invalid[rc:%d]", rc ) ;
         rc = SDB_SYS ;
      }
      builder.append( OP_ERRNOFIELD, rc ) ;
      builder.append( OP_ERRDESP_FIELD, getErrDesp( rc ) ) ;

      if ( String == retObj.getField( OP_ERR_DETAIL ).type() )
      {
         builder.append( OP_ERR_DETAIL,
                         retObj.getStringField( OP_ERR_DETAIL ) ) ;
      }
      else if ( eduCB()->getInfo( EDU_INFO_ERROR ) &&
                0 != *( eduCB()->getInfo( EDU_INFO_ERROR ) ) )
      {
         builder.append( OP_ERR_DETAIL,
                         eduCB()->getInfo( EDU_INFO_ERROR ) ) ;
      }
      else
      {
         builder.append( OP_ERR_DETAIL, getErrDesp( rc ) ) ;
      }

      goto done ;
   }

   INT32 _omaSession::_buildReplyHeader( MsgHeader *pMsg )
   {
      _replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      _replyHeader.header.opCode        = MAKE_REPLY_TYPE(pMsg->opCode) ;
      _replyHeader.header.TID           = pMsg->TID ;
      _replyHeader.header.routeID.value = 0 ;
      _replyHeader.header.requestID     = pMsg->requestID ;
      _replyHeader.header.globalID      = pMsg->globalID ;
      _replyHeader.contextID            = -1 ;
      _replyHeader.flags                = 0 ;
      _replyHeader.numReturned          = 0 ;
      _replyHeader.startFrom            = 0 ;
      _replyHeader.numReturned          = 0 ;

      return SDB_OK ;
   }

   void _omaSession::_clearFileObjMap()
   {
      for( map< UINT32, sptUsrFileCommon* >::iterator it = _fileObjMap.begin();
           it != _fileObjMap.end();
           it++ )
      {
         // release obj
         if( it->second )
         {
            SDB_OSS_DEL it->second ;
         }
         it->second = NULL ;
      }
      _fileObjMap.clear() ;
      _maxFileObjID = 0 ;
   }
} // namespace engine

