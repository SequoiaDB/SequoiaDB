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

   Source File Name = pmdRestSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdRestSession.hpp"
#include "pmdController.hpp"
#include "omManager.hpp"
#include "pmdEDUMgr.hpp"
#include "msgDef.h"
#include "fmpDef.h"
#include "utilCommon.hpp"
#include "ossMem.hpp"
#include "rtnCommand.hpp"
#include "../omsvc/omCommand.hpp"
#include "rtn.hpp"
#include "msgAuth.hpp"
#include "pmdTrace.hpp"
#include "../bson/bson.h"
#include "authCB.hpp"
#include "utilStr.hpp"
#include "msg.h"
#include "omDef.hpp"

using namespace bson ;

namespace engine
{
   void _sendOpError2Web ( INT32 rc, restAdaptor *pAdptor,
                           pmdRestSession *pRestSession, pmdEDUCB* pEduCB )
   {
      SDB_ASSERT( ( NULL != pAdptor ) && ( NULL != pRestSession )
                  && ( NULL != pEduCB ), "pAdptor or pRestSession or pEduCB"
                  " can't be null" ) ;

      BSONObj _errorInfo = utilGetErrorBson( rc, pEduCB->getInfo(
                                             EDU_INFO_ERROR ) ) ;
      pAdptor->setOPResult( pRestSession, rc, _errorInfo ) ;
      pAdptor->sendResponse( pRestSession, HTTP_OK ) ;
   }

   #define PMD_REST_SESSION_SNIFF_TIMEOUT    ( 10 * OSS_ONE_SEC )

   /*
      util
   */
   // PD_TRACE_DECLARE_FUNCTION( SDB__UTILMSGFLAG, "utilMsgFlag" )
   static INT32 utilMsgFlag( const string strFlag, INT32 &flag )
   {
      PD_TRACE_ENTRY( SDB__UTILMSGFLAG ) ;
      INT32 rc = SDB_OK ;
      flag = 0 ;
      INT32 subFlag = 0 ;

      vector<string> subFlags ;
      subFlags = utilStrSplit( strFlag, REST_FLAG_SEP ) ;

      vector<string>::iterator it = subFlags.begin() ;
      for ( ; it != subFlags.end(); it++ )
      {
         utilStrTrim( *it ) ;
         const char *strSubFlag = ( *it ).c_str() ;

         if ( 0 == ossStrcmp( strSubFlag,
                              REST_VALUE_FLAG_UPDATE_KEEP_SK ) )
         {
            flag |= FLG_UPDATE_KEEP_SHARDINGKEY ;
            continue ;
         }
         if ( 0 == ossStrcmp( strSubFlag,
                              REST_VALUE_FLAG_QUERY_KEEP_SK_IN_UPDATE ) )
         {
            flag |= FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE ;
            continue ;
         }
         if ( 0 == ossStrcmp( strSubFlag,
                              REST_VALUE_FLAG_QUERY_FORCE_HINT ) )
         {
            flag |= FLG_QUERY_FORCE_HINT ;
            continue ;
         }
         if ( 0 == ossStrcmp( strSubFlag,
                              REST_VALUE_FLAG_QUERY_PARALLED ) )
         {
            flag |= FLG_QUERY_PARALLED ;
            continue ;
         }
         if ( 0 == ossStrcmp( strSubFlag,
                              REST_VALUE_FLAG_QUERY_WITH_RETURNDATA ) )
         {
            flag |= FLG_QUERY_WITH_RETURNDATA ;
            continue ;
         }

         rc = utilStr2Num( strSubFlag, subFlag );
         if ( rc )
         {
            PD_LOG( PDERROR, "Unrecognized flag: %s, rc: %d",
                    strSubFlag, rc ) ;
            goto error ;
         }
         flag |= subFlag ;

      }

   done :
      PD_TRACE_EXITRC( SDB__UTILMSGFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _restSessionInfo implement
   */
   void _restSessionInfo::releaseMem()
   {
      pmdEDUCB::CATCH_MAP_IT it = _catchMap.begin() ;
      while ( it != _catchMap.end() )
      {
         SDB_OSS_FREE( it->second ) ;
         ++it ;
      }
      _catchMap.clear() ;
   }

   void _restSessionInfo::pushMemToMap( _pmdEDUCB::CATCH_MAP &catchMap )
   {
      _pmdEDUCB::CATCH_MAP_IT it = _catchMap.begin() ;
      while ( it != _catchMap.end() )
      {
         catchMap.insert( std::make_pair( it->first, it->second ) ) ;
         ++it ;
      }
      _catchMap.clear() ;
   }

   void _restSessionInfo::makeMemFromMap( _pmdEDUCB::CATCH_MAP &catchMap )
   {
      _pmdEDUCB::CATCH_MAP_IT it = catchMap.begin() ;
      while ( it != catchMap.end() )
      {
         _catchMap.insert( std::make_pair( it->first, it->second ) ) ;
         ++it ;
      }
      catchMap.clear() ;
   }

   /*
      _pmdRestSession implement
   */
   _pmdRestSession::_pmdRestSession( SOCKET fd )
   :_pmdSession( fd )
   {
      _pFixBuff         = NULL ;
      _pSessionInfo     = NULL ;
      _pRTNCB           = NULL ;

      _wwwRootPath      = pmdGetOptionCB()->getWWWPath() ;
      _pRestTransfer    = SDB_OSS_NEW RestToMSGTransfer( this ) ;
      _pRestTransfer->init() ;
   }

   _pmdRestSession::~_pmdRestSession()
   {
      if ( _pFixBuff )
      {
         sdbGetPMDController()->releaseFixBuf( _pFixBuff ) ;
         _pFixBuff = NULL ;
      }

      if ( NULL != _pRestTransfer )
      {
         SDB_OSS_DEL _pRestTransfer ;
         _pRestTransfer = NULL ;
      }
   }

   INT32 _pmdRestSession::getServiceType() const
   {
      return CMD_SPACE_SERVICE_LOCAL ;
   }

   SDB_SESSION_TYPE _pmdRestSession::sessionType() const
   {
      return SDB_SESSION_REST ;
   }

   INT32 _pmdRestSession::run()
   {
      INT32 rc                         = SDB_OK ;
      restAdaptor *pAdptor             = sdbGetPMDController()->getRestAdptor() ;
      pmdEDUMgr *pEDUMgr               = NULL ;
      const CHAR *pSessionID           = NULL ;
      HTTP_PARSE_COMMON httpCommon     = COM_GETFILE ;
      CHAR *pFilePath                  = NULL ;
      INT32 bodySize                   = 0 ;
      monDBCB *mondbcb                 = pmdGetKRCB()->getMonDBCB () ;

      if ( !_pEDUCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      pEDUMgr = _pEDUCB->getEDUMgr() ;

      while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
      {
         _pEDUCB->resetInterrupt() ;
         _pEDUCB->resetInfo( EDU_INFO_ERROR ) ;
         _pEDUCB->resetLsn() ;

         rc = sniffData( _pSessionInfo ? OSS_ONE_SEC :
                         PMD_REST_SESSION_SNIFF_TIMEOUT ) ;
         if ( SDB_TIMEOUT == rc )
         {
            if ( _pSessionInfo )
            {
               saveSession() ;
               sdbGetPMDController()->detachSessionInfo( _pSessionInfo ) ;
               _pSessionInfo = NULL ;
               continue ;
            }
            else
            {
               rc = SDB_OK ;
               break ;
            }
         }
         else if ( rc < 0 )
         {
            break ;
         }

         rc = pAdptor->recvRequestHeader( this ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s] failed to recv rest header, "
                    "rc: %d", sessionName(), rc ) ;
            if ( SDB_REST_EHS == rc )
            {
               pAdptor->sendResponse( this, HTTP_BADREQ ) ;
            }
            else if ( SDB_APP_FORCED != rc )
            {
               _sendOpError2Web( rc, pAdptor, this, _pEDUCB ) ;
            }
            break ;
         }
         if ( !_pSessionInfo )
         {
            pAdptor->getHttpHeader( this, OM_REST_HEAD_SESSIONID,
                                    &pSessionID ) ;
            if ( pSessionID )
            {
               PD_LOG( PDINFO, "Rest session: %s", pSessionID ) ;
               _pSessionInfo = sdbGetPMDController()->attachSessionInfo(
                                  pSessionID ) ;
            }

            if ( _pSessionInfo )
            {
               _client.setAuthed( TRUE ) ;
               restoreSession() ;
            }
         }
         rc = pAdptor->recvRequestBody( this, httpCommon, &pFilePath,
                                        bodySize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s] failed to recv rest body, "
                    "rc: %d", sessionName(), rc ) ;
            if ( SDB_REST_EHS == rc )
            {
               pAdptor->sendResponse( this, HTTP_BADREQ ) ;
            }
            else if ( SDB_APP_FORCED != rc )
            {
               _sendOpError2Web( rc, pAdptor, this, _pEDUCB ) ;
            }
            break ;
         }

         if ( _pSessionInfo )
         {
            _pSessionInfo->active() ;
         }

         _pEDUCB->incEventCount() ;
         mondbcb->addReceiveNum() ;

         if ( SDB_OK != ( rc = pEDUMgr->activateEDU( _pEDUCB ) ) )
         {
            PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                    sessionName(), rc ) ;
            break ;
         }

         rc = _processMsg( httpCommon, pFilePath ) ;
         if ( rc )
         {
            break ;
         }

         if ( FALSE == pAdptor->isKeepAlive( this ) )
         {
            break ;
         }

         if ( SDB_OK != ( rc = pEDUMgr->waitEDU( _pEDUCB ) ) )
         {
            PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
                    sessionName(), rc ) ;
            break ;
         }

         if ( pFilePath )
         {
            releaseBuff( pFilePath ) ;
            pFilePath = NULL ;
         }
         rc = SDB_OK ;
      } // end while

   done:
      if ( pFilePath )
      {
         releaseBuff( pFilePath ) ;
      }
      disconnect() ;
      return rc ;
   error:
      goto done ;
   }


   INT32 _pmdRestSession::_translateMSG( restAdaptor *pAdaptor,
                                         MsgHeader **msg )
   {
      SDB_ASSERT( NULL != msg, "msg can't be null" ) ;

      INT32 rc = SDB_OK ;
      rc = _pRestTransfer->trans( pAdaptor, msg ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _pmdRestSession::_fetchOneContext( SINT64 &contextID,
                                            rtnContextBuf &contextBuff )
   {
      INT32 rc = SDB_OK ;
      rtnContext *pContext = _pRTNCB->contextFind( contextID ) ;
      if ( NULL == pContext )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "can't fine context,contextID=%u", rc, contextID ) ;
         contextID = -1 ;
      }
      else
      {
         rc = pContext->getMore( -1, contextBuff, _pEDUCB ) ;
         if ( rc || pContext->eof() )
         {
            _pRTNCB->contextDelete( contextID, _pEDUCB ) ;
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "getmore failed:rc=%d,contextID=%u", rc,
                       contextID ) ;
               goto error ;
            }
         }
      }

      rc = SDB_OK ;
   done:
      return rc ;
   error:
      contextID = -1 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDRESTSN_PROMSG, "_pmdRestSession::_processMsg" )
   INT32 _pmdRestSession::_processMsg( HTTP_PARSE_COMMON command,
                                       const CHAR *pFilePath )
   {
      PD_TRACE_ENTRY( SDB_PMDRESTSN_PROMSG );
      INT32 rc = SDB_OK ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;
      const CHAR *pSubCommand = NULL ;
      pAdaptor->getQuery( this, OM_REST_FIELD_COMMAND, &pSubCommand ) ;
      if ( NULL != pSubCommand )
      {
         if ( ossStrcasecmp( pSubCommand, OM_LOGOUT_REQ ) == 0 )
         {
            if ( isAuthOK() )
            {
               doLogout() ;
               pAdaptor->sendResponse( this, HTTP_OK ) ;
               goto done ;
            }
            else
            {
               rc = SDB_PMD_SESSION_NOT_EXIST ;
               PD_LOG_MSG( PDERROR, "session is not exist:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
               goto done ;
            }
         }
      }

      rc = _processBusinessMsg( pAdaptor ) ;
      if ( SDB_UNKNOWN_MESSAGE == rc )
      {
         PD_LOG_MSG( PDERROR, "translate message failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_PMDRESTSN_PROMSG, rc );
      return rc ;
   }

   INT32 _pmdRestSession::_checkAuth( restAdaptor *pAdaptor )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pUserName = NULL ;
      const CHAR *pPasswd   = NULL ;
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBUSER, &pUserName ) ;
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBPASSWD, &pPasswd ) ;
      if ( NULL != pUserName && NULL != pPasswd )
      {
         BSONObj bsonAuth = BSON( SDB_AUTH_USER << pUserName
                                  << SDB_AUTH_PASSWD << pPasswd ) ;
         if ( !getClient()->isAuthed() )
         {
            rc = getClient()->authenticate( pUserName, pPasswd ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "authenticate failed:rc=%d", rc ) ;
               goto error ;
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRestSession::_processBusinessMsg( restAdaptor *pAdaptor )
   {
      INT32 rc        = SDB_OK ;
      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      BOOLEAN needReplay = FALSE ;
      MsgHeader *msg = NULL ;
      rc = _translateMSG( pAdaptor, &msg ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_UNKNOWN_MESSAGE != rc )
         {
            PD_LOG( PDERROR, "translate message failed:rc=%d", rc ) ;
            _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
         }

         goto error ;
      }

      rc = _checkAuth( pAdaptor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "check auth failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, eduCB() ) ;
         goto error ;
      }

      rc = getProcessor()->processMsg( msg, contextBuff, contextID,
                                       needReplay ) ;
      if ( SDB_OK != rc )
      {
         BSONObjBuilder builder ;
         if ( contextBuff.recordNum() != 0 )
         {
            BSONObj errorInfo( contextBuff.data() ) ;
            if ( !errorInfo.hasField( OM_REST_RES_RETCODE ) )
            {
               builder.append( OM_REST_RES_RETCODE, rc ) ;
            }
            builder.appendElements( errorInfo ) ;
         }
         else
         {
            BSONObj errorInfo = utilGetErrorBson( rc,
                                          _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
            builder.appendElements( errorInfo ) ;
         }

         BSONObj tmp = builder.obj() ;
         pAdaptor->setOPResult( this, rc, tmp ) ;
      }
      else
      {
         if ( -1 != contextID && contextBuff.recordNum() > 0 )
         {
            pAdaptor->setChunkModal( this ) ;
         }

         BSONObj tmp = BSON( OM_REST_RES_RETCODE << rc ) ;
         pAdaptor->setOPResult( this, rc, tmp ) ;
         if ( contextBuff.recordNum() > 0 )
         {
            pAdaptor->appendHttpBody( this, contextBuff.data(),
                                      contextBuff.size(),
                                      contextBuff.recordNum() ) ;
         }

         if ( -1 != contextID )
         {
            rtnContext *pContext = _pRTNCB->contextFind( contextID ) ;
            while ( NULL != pContext )
            {
               rtnContextBuf tmpContextBuff ;
               rc = pContext->getMore( -1, tmpContextBuff, _pEDUCB ) ;
               if ( SDB_OK == rc )
               {
                  rc = pAdaptor->appendHttpBody( this, tmpContextBuff.data(),
                                                 tmpContextBuff.size(),
                                                 tmpContextBuff.recordNum() ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG_MSG( PDERROR, "append http body failed:rc=%d",
                                 rc ) ;
                     _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
                     goto error ;
                  }
               }
               else
               {
                  _pRTNCB->contextDelete( contextID, _pEDUCB ) ;
                  contextID = -1 ;
                  if ( SDB_DMS_EOC != rc )
                  {
                     PD_LOG_MSG( PDERROR, "getmore failed:rc=%d", rc ) ;
                     _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
                     goto error ;
                  }

                  rc = SDB_OK ;
                  break ;
               }
            }
         }
      }

      _dealWithLoginReq( rc ) ;
      pAdaptor->sendResponse( this, HTTP_OK ) ;

      rc = SDB_OK ;

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete( contextID, _pEDUCB ) ;
         contextID = -1 ;
      }
      if ( NULL != msg )
      {
         SDB_OSS_FREE( msg ) ;
         msg = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRestSession::_dealWithLoginReq( INT32 result )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCommand = NULL ;
      restAdaptor *pAdaptor   = sdbGetPMDController()->getRestAdptor() ;

      if ( SDB_OK != result )
      {
         goto done ;
      }

      pAdaptor->getQuery( this, OM_REST_FIELD_COMMAND, &pSubCommand ) ;
      if ( NULL != pSubCommand )
      {
         if ( ossStrcasecmp( pSubCommand, OM_LOGIN_REQ ) == 0 )
         {
            const CHAR* pUser = NULL ;
            pAdaptor->getQuery( this, OM_REST_FIELD_LOGIN_NAME , &pUser ) ;
            SDB_ASSERT( ( NULL != pUser ), "" ) ;
            rc = doLogin( pUser, socket()->getLocalIP() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "login failed:rc=%d", rc ) ;
            }
            pAdaptor->appendHttpHeader( this, OM_REST_HEAD_SESSIONID,
                                        getSessionID() ) ;

         }
      }

   done:
      return rc ;
   }

   void _pmdRestSession::_onAttach()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _pRTNCB = krcb->getRTNCB() ;

      if ( NULL != sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->registerEDU( eduCB() ) ;
      }
   }

   void _pmdRestSession::_onDetach()
   {
      if ( _pSessionInfo )
      {
         saveSession() ;
         sdbGetPMDController()->detachSessionInfo( _pSessionInfo ) ;
         _pSessionInfo = NULL ;
      }

      if ( NULL != sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->unregEUD( eduCB() ) ;
      }
   }

   INT32 _pmdRestSession::getFixBuffSize() const
   {
      return sdbGetPMDController()->getFixBufSize() ;
   }

   CHAR* _pmdRestSession::getFixBuff ()
   {
      if ( !_pFixBuff )
      {
         _pFixBuff = sdbGetPMDController()->allocFixBuf() ;
      }
      return _pFixBuff ;
   }

   void _pmdRestSession::restoreSession()
   {
      pmdEDUCB::CATCH_MAP catchMap ;
      _pSessionInfo->pushMemToMap( catchMap ) ;
      eduCB()->restoreBuffs( catchMap ) ;
   }

   void _pmdRestSession::saveSession()
   {
      pmdEDUCB::CATCH_MAP catchMap ;
      eduCB()->saveBuffs( catchMap ) ;
      _pSessionInfo->makeMemFromMap( catchMap ) ;
   }

   BOOLEAN _pmdRestSession::isAuthOK()
   {
      if ( NULL != _pSessionInfo )
      {
         if ( _pSessionInfo->_authOK )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   string _pmdRestSession::getLoginUserName()
   {
      if ( isAuthOK() )
      {
         return _pSessionInfo->_attr._userName ;
      }

      return "" ;
   }

   const CHAR* _pmdRestSession::getSessionID()
   {
      if ( NULL != _pSessionInfo )
      {
         if ( _pSessionInfo->_authOK )
         {
            return _pSessionInfo->_id.c_str();
         }
      }

      return "" ;
   }

   INT32 _pmdRestSession::doLogin( const string & username,
                                   UINT32 localIP )
   {
      INT32 rc = SDB_OK ;

      doLogout() ;

      _pSessionInfo = sdbGetPMDController()->newSessionInfo( username,
                                                             localIP ) ;
      if ( !_pSessionInfo )
      {
         PD_LOG ( PDERROR, "Failed to allocate session" ) ;
         rc = SDB_OOM ;
      }
      else
      {
         _pSessionInfo->_authOK = TRUE ;
      }
      return rc ;
   }

   void _pmdRestSession::doLogout()
   {
      if ( _pSessionInfo )
      {
         sdbGetPMDController()->releaseSessionInfo( _pSessionInfo->_id ) ;
         _pSessionInfo = NULL ;
      }
   }

   typedef struct restCommand2Func_s
   {
      string commandName ;
      restTransFunc func ;
   } restCommand2Func ;

   RestToMSGTransfer::RestToMSGTransfer( pmdRestSession *session )
                     :_restSession( session )
   {
   }

   RestToMSGTransfer::~RestToMSGTransfer()
   {
   }

   INT32 RestToMSGTransfer::init()
   {
      INT32 rc  = SDB_OK ;
      INT32 i   = 0 ;
      INT32 len = 0 ;
      static restCommand2Func s_commandArray[] = {
         { REST_CMD_NAME_QUERY,  &RestToMSGTransfer::_convertQuery },
         { REST_CMD_NAME_INSERT, &RestToMSGTransfer::_convertInsert },
         { REST_CMD_NAME_DELETE, &RestToMSGTransfer::_convertDelete },
         { REST_CMD_NAME_UPDATE, &RestToMSGTransfer::_convertUpdate },
         { REST_CMD_NAME_UPSERT, &RestToMSGTransfer::_convertUpsert },
         { REST_CMD_NAME_QUERY_UPDATE,
                                 &RestToMSGTransfer::_convertQueryUpdate },
         { REST_CMD_NAME_QUERY_REMOVE,
                                 &RestToMSGTransfer::_convertQueryRemove },
         { CMD_NAME_CREATE_COLLECTIONSPACE,
                                 &RestToMSGTransfer::_convertCreateCS },
         { CMD_NAME_CREATE_COLLECTION,
                                 &RestToMSGTransfer::_convertCreateCL },
         { CMD_NAME_DROP_COLLECTIONSPACE,
                                 &RestToMSGTransfer::_convertDropCS },
         { CMD_NAME_DROP_COLLECTION,
                                 &RestToMSGTransfer::_convertDropCL },
         { CMD_NAME_CREATE_INDEX, &RestToMSGTransfer::_convertCreateIndex },
         { CMD_NAME_DROP_INDEX,  &RestToMSGTransfer::_convertDropIndex },
         { CMD_NAME_SPLIT,       &RestToMSGTransfer::_convertSplit },

         { REST_CMD_NAME_TRUNCATE_COLLECTION,
                              &RestToMSGTransfer::_convertTruncateCollection },
         { REST_CMD_NAME_ATTACH_COLLECTION,
                                 &RestToMSGTransfer::_coverAttachCollection },
         { REST_CMD_NAME_DETACH_COLLECTION,
                                 &RestToMSGTransfer::_coverDetachCollection },
         { CMD_NAME_ALTER_COLLECTION,
                                 &RestToMSGTransfer::_convertAlterCollection },

         { CMD_NAME_GET_COUNT,   &RestToMSGTransfer::_convertGetCount },

         { CMD_NAME_LIST_GROUPS, &RestToMSGTransfer::_convertListGroups },
         { REST_CMD_NAME_START_GROUP,
                                 &RestToMSGTransfer::_convertStartGroup },
         { REST_CMD_NAME_STOP_GROUP,
                                 &RestToMSGTransfer::_convertStopGroup },

         { REST_CMD_NAME_START_NODE,
                                 &RestToMSGTransfer::_convertStartNode },
         { REST_CMD_NAME_STOP_NODE,
                                 &RestToMSGTransfer::_convertStopNode },

         { CMD_NAME_CRT_PROCEDURE,
                                 &RestToMSGTransfer::_convertCreateProcedure },
         { CMD_NAME_RM_PROCEDURE,
                                 &RestToMSGTransfer::_convertRemoveProcedure },
         { CMD_NAME_LIST_PROCEDURES,
                                 &RestToMSGTransfer::_convertListProcedures },
         { CMD_NAME_LIST_CONTEXTS,
                                 &RestToMSGTransfer::_convertListContexts },
         { CMD_NAME_LIST_CONTEXTS_CURRENT,
                                 &RestToMSGTransfer::_convertListContextsCurrent },
         { CMD_NAME_LIST_SESSIONS,
                                 &RestToMSGTransfer::_convertListSessions },
         { CMD_NAME_LIST_SESSIONS_CURRENT,
                                 &RestToMSGTransfer::_convertListSessionsCurrent },
         { CMD_NAME_LIST_COLLECTIONS,
                                 &RestToMSGTransfer::_convertListCollections },
         { CMD_NAME_LIST_COLLECTIONSPACES,
                                 &RestToMSGTransfer::_convertListCollectionSpaces },
         { CMD_NAME_LIST_STORAGEUNITS,
                                 &RestToMSGTransfer::_convertListStorageUnits },
         { CMD_NAME_CREATE_DOMAIN,
                                 &RestToMSGTransfer::_convertCreateDomain },
         { CMD_NAME_DROP_DOMAIN, &RestToMSGTransfer::_convertDropDomain },
         { CMD_NAME_ALTER_DOMAIN,
                                 &RestToMSGTransfer::_convertAlterDomain },
         { CMD_NAME_LIST_DOMAINS,
                                 &RestToMSGTransfer::_convertListDomains },
         { CMD_NAME_LIST_TASKS,  &RestToMSGTransfer::_convertListTasks },
         { REST_CMD_NAME_LISTINDEXES,
                                 &RestToMSGTransfer::_convertListIndexes },
         { CMD_NAME_SNAPSHOT_CONTEXTS,
                                 &RestToMSGTransfer::_convertSnapshotContext },
         { CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT,
                                 &RestToMSGTransfer::_convertSnapshotContextCurrent },
         { CMD_NAME_SNAPSHOT_SESSIONS,
                                 &RestToMSGTransfer::_convertSnapshotSessions },
         { CMD_NAME_SNAPSHOT_SESSIONS_CURRENT,
                                 &RestToMSGTransfer::_convertSnapshotSessionsCurrent },
         { CMD_NAME_SNAPSHOT_COLLECTIONS,
                                 &RestToMSGTransfer::_convertSnapshotCollections },
         { CMD_NAME_SNAPSHOT_COLLECTIONSPACES,
                                 &RestToMSGTransfer::_convertSnapshotCollectionSpaces },
         { CMD_NAME_SNAPSHOT_DATABASE,
                                 &RestToMSGTransfer::_convertSnapshotDatabase },
         { CMD_NAME_SNAPSHOT_SYSTEM,
                                 &RestToMSGTransfer::_convertSnapshotSystem },
         { CMD_NAME_SNAPSHOT_CATA,
                                 &RestToMSGTransfer::_convertSnapshotCata },
         { CMD_NAME_SNAPSHOT_ACCESSPLANS,
                                 &RestToMSGTransfer::_convertSnapshotAccessPlans },
         { CMD_NAME_SNAPSHOT_HEALTH,
                                 &RestToMSGTransfer::_convertSnapshotHealth },
         { CMD_NAME_LIST_LOBS,   &RestToMSGTransfer::_convertListLobs },
         { OM_LOGIN_REQ,         &RestToMSGTransfer::_convertLogin },
         { REST_CMD_NAME_EXEC,   &RestToMSGTransfer::_convertExec },
         { CMD_NAME_FORCE_SESSION,
                                 &RestToMSGTransfer::_convertForceSession },
         { CMD_NAME_ANALYZE,     &RestToMSGTransfer::_convertAnalyze }
      } ;

      len = sizeof( s_commandArray ) / sizeof( restCommand2Func ) ;
      for ( i = 0 ; i < len ; i++ )
      {
         _mapTransFunc.insert( _value_type( s_commandArray[i].commandName,
                                            s_commandArray[i].func ) ) ;
      }

      return rc ;
   }

   INT32 RestToMSGTransfer::trans( restAdaptor *pAdaptor, MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSubCommand = NULL ;
      _iterator iter ;

      pAdaptor->getQuery( _restSession, OM_REST_FIELD_COMMAND, &pSubCommand ) ;
      if ( NULL == pSubCommand )
      {
         rc = SDB_UNKNOWN_MESSAGE ;
         if ( !pmdGetKRCB()->isCBValue( SDB_CB_OMSVC ) )
         {
            PD_LOG_MSG( PDERROR, "can't resolve field:field=%s",
                        OM_REST_FIELD_COMMAND ) ;
         }

         goto error ;
      }

      iter = _mapTransFunc.find( pSubCommand ) ;
      if ( iter != _mapTransFunc.end() )
      {
         restTransFunc func = iter->second ;
         rc = (this->*func)( pAdaptor, msg ) ;
      }
      else
      {
         if ( !pmdGetKRCB()->isCBValue( SDB_CB_OMSVC ) )
         {
            PD_LOG_MSG( PDERROR, "unsupported command:command=%s",
                        pSubCommand ) ;
         }
         rc = SDB_UNKNOWN_MESSAGE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertCreateCS( restAdaptor *pAdaptor,
                                              MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
      const CHAR *pOption   = NULL ;
      const CHAR *pCollectionSpace = NULL ;
      BSONObj option ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollectionSpace ) ;
      if ( NULL == pCollectionSpace )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTIONSPACE,
                             &pCollectionSpace ) ;
         if ( NULL == pCollectionSpace )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collectionspace's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTIONSPACE ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOption ) ;
      if ( NULL != pOption )
      {
         rc = fromjson( pOption, option, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_OPTIONS, pOption ) ;
            goto error ;
         }
      }

      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_NAME, pCollectionSpace ) ;
         {
            BSONObjIterator it ( option ) ;
            while ( it.more() )
            {
               builder.append( it.next() ) ;
            }
         }

         query = builder.obj() ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertCreateCL( restAdaptor *pAdaptor,
                                              MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
      const CHAR *pOption   = NULL ;
      const CHAR *pCollection = NULL ;
      BSONObj option ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOption ) ;
      if ( NULL != pOption )
      {
         rc = fromjson( pOption, option, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_OPTIONS, pOption ) ;
            goto error ;
         }
      }

      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_NAME, pCollection ) ;
         {
            BSONObjIterator it ( option ) ;
            while ( it.more() )
            {
               builder.append( it.next() ) ;
            }
         }

         query = builder.obj() ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertDropCS( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ;
      const CHAR *pCollectionSpace = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollectionSpace ) ;
      if ( NULL == pCollectionSpace )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTIONSPACE,
                             &pCollectionSpace ) ;
         if ( NULL == pCollectionSpace )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collectionspace's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTIONSPACE ) ;
            goto error ;
         }
      }

      query = BSON( FIELD_NAME_NAME << pCollectionSpace ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertDropCL( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ;
      const CHAR *pCollection = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      query = BSON( FIELD_NAME_NAME  << pCollection ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertQueryBasic( restAdaptor* pAdaptor,
                                                const CHAR** collectionName,
                                                BSONObj& match,
                                                BSONObj& selector,
                                                BSONObj& order,
                                                BSONObj& hint,
                                                INT32* flag,
                                                SINT64* skip,
                                                SINT64* returnRow )
   {
      INT32 rc              = SDB_OK ;
      const CHAR *pTable    = NULL ;
      const CHAR *pOrder    = NULL ;
      const CHAR *pHint     = NULL ;
      const CHAR *pMatch    = NULL ;
      const CHAR *pSelector = NULL ;
      const CHAR *pFlag     = NULL ;
      const CHAR *pSkip     = NULL ;
      const CHAR *pReturnRow = NULL ;

      SDB_ASSERT( pAdaptor, "pAdaptor can't be null") ;
      SDB_ASSERT( collectionName, "collectionName can't be null") ;
      SDB_ASSERT( flag, "flag can't be null") ;
      SDB_ASSERT( skip, "skip can't be null") ;
      SDB_ASSERT( returnRow, "returnRow can't be null") ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pTable ) ;
      if ( NULL == pTable )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION, &pTable ) ;
         if ( NULL == pTable )
         {
            PD_LOG_MSG( PDERROR, "get field failed:field=%s[or %s]",
                     FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      *collectionName = pTable ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_FILTER, &pMatch ) ;
      if ( NULL == pMatch )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pMatch ) ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_SELECTOR, &pSelector ) ;
      pAdaptor->getQuery( _restSession, FIELD_NAME_SORT, &pOrder ) ;
      if ( NULL == pOrder )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_ORDERBY, &pOrder ) ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_HINT, &pHint ) ;
      pAdaptor->getQuery( _restSession, REST_KEY_NAME_FLAG, &pFlag ) ;
      pAdaptor->getQuery( _restSession, FIELD_NAME_SKIP, &pSkip ) ;
      pAdaptor->getQuery( _restSession, FIELD_NAME_RETURN_NUM, &pReturnRow ) ;
      if ( NULL == pReturnRow )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_LIMIT, &pReturnRow ) ;
      }

      if ( NULL != pMatch )
      {
         rc = fromjson( pMatch, match, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s], "
                        "value=%s", FIELD_NAME_FILTER,
                        REST_KEY_NAME_MATCHER, pMatch ) ;
            goto error ;
         }
      }

      if ( NULL != pSelector )
      {
         rc = fromjson( pSelector, selector, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_SELECTOR, pSelector ) ;
            goto error ;
         }
      }

      if ( NULL != pOrder )
      {
         rc = fromjson( pOrder, order, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s], "
                        "value=%s", FIELD_NAME_SORT, REST_KEY_NAME_ORDERBY,
                        pOrder ) ;
            goto error ;
         }
      }

      if ( NULL != pHint )
      {
         rc = fromjson( pHint, hint, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_HINT, pHint ) ;
            goto error ;
         }
      }

      if ( NULL != pFlag )
      {
         rc = utilMsgFlag( pFlag, *flag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s,"
                        "value=%s", REST_KEY_NAME_FLAG, pFlag ) ;
            goto error ;
         }
         *flag = *flag | FLG_QUERY_WITH_RETURNDATA ;
      }

      if ( NULL != pSkip )
      {
         *skip = ossAtoll( pSkip ) ;
      }

      if ( NULL != pReturnRow )
      {
         *returnRow = ossAtoll( pReturnRow ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertQuery( restAdaptor *pAdaptor,
                                           MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pTable    = NULL ;

      {
         BSONObj order ;
         BSONObj hint ;
         BSONObj match ;
         BSONObj selector ;
         INT32 flag  = 0 ;
         SINT64 skip = 0 ;
         SINT64 returnRow = -1 ;

         rc = _convertQueryBasic( pAdaptor, &pTable, match, selector, order, hint,
                                  &flag, &skip, &returnRow ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "convert basic queryMsg failed:rc=%d", rc ) ;
            goto error ;
         }

         rc = msgBuildQueryMsg( &pBuff, &buffSize, pTable, flag, 0, skip,
                                returnRow, &match, &selector, &order, &hint ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "build queryMSG failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertQueryUpdate( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      return _convertQueryModify( pAdaptor, msg, TRUE ) ;
   }

   INT32 RestToMSGTransfer::_convertQueryRemove( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      return _convertQueryModify( pAdaptor, msg, FALSE ) ;
   }

   INT32 RestToMSGTransfer::_convertQueryModify( restAdaptor *pAdaptor,
                                                 MsgHeader **msg,
                                                 BOOLEAN isUpdate )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pTable    = NULL ;
      INT32 flag            = 0 ;
      SINT64 skip           = 0 ;
      SINT64 returnRow      = -1 ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj match ;
      BSONObj selector ;
      BSONObj newHint ;

      rc = _convertQueryBasic( pAdaptor, &pTable, match, selector, order, hint,
                               &flag, &skip, &returnRow ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "convert basic queryMsg failed:rc=%d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder newHintBuilder ;
         BSONObjBuilder modifyBuilder ;
         BSONObj modify ;

         if ( isUpdate )
         {
            const CHAR* pUpdate = NULL ;
            const CHAR* pReturnNew = NULL ;
            BSONObj update ;
            BOOLEAN returnNew = FALSE ;

            pAdaptor->getQuery( _restSession, REST_KEY_NAME_UPDATOR, &pUpdate ) ;
            if ( NULL == pUpdate )
            {
               PD_LOG_MSG( PDERROR, "get field failed:field=%s",
                           REST_KEY_NAME_UPDATOR ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = fromjson( pUpdate, update, 0 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                           FIELD_NAME_OP_UPDATE, pUpdate ) ;
               goto error ;
            }

            pAdaptor->getQuery( _restSession, FIELD_NAME_RETURNNEW, &pReturnNew ) ;
            if ( NULL != pReturnNew )
            {
               if ( 0 == ossStrcasecmp(pReturnNew, "true") )
               {
                  returnNew = TRUE ;
               }
               else if ( 0 == ossStrcasecmp(pReturnNew, "false") )
               {
                  returnNew = FALSE ;
               }
               else
               {
                  PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                           FIELD_NAME_RETURNNEW, pReturnNew ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            modifyBuilder.append( FIELD_NAME_OP, FIELD_OP_VALUE_UPDATE ) ;
            modifyBuilder.appendObject( FIELD_NAME_OP_UPDATE, update.objdata() ) ;
            modifyBuilder.appendBool( FIELD_NAME_RETURNNEW, returnNew ) ;
         }
         else
         {
            modifyBuilder.append( FIELD_NAME_OP, FIELD_OP_VALUE_REMOVE ) ;
            modifyBuilder.appendBool( FIELD_NAME_OP_REMOVE, TRUE ) ;
         }
         modify = modifyBuilder.obj() ;

         if ( !hint.isEmpty() )
         {
            newHintBuilder.appendElements( hint ) ;
         }
         newHintBuilder.appendObject( FIELD_NAME_MODIFY, modify.objdata() ) ;
         newHint = newHintBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to create $Modify, %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      flag |= FLG_QUERY_MODIFY ;

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pTable, flag, 0, skip,
                             returnRow, &match, &selector, &order, &newHint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build queryMSG failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertInsert( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pFlag     = NULL ;
      SINT32 flag           = 0 ;
      const CHAR *pCollection = NULL ;
      const CHAR *pInsertor   = NULL ;
      BSONObj insertor ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_FLAG, &pFlag ) ;
      if ( NULL != pFlag )
      {
         flag = ossAtoi( pFlag ) ;
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_INSERTOR, &pInsertor ) ;
      if ( NULL == pInsertor )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get collection's %s failed",
                     REST_KEY_NAME_INSERTOR ) ;
         goto error ;
      }

      rc = fromjson( pInsertor, insertor, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                     REST_KEY_NAME_INSERTOR, pInsertor ) ;
         goto error ;
      }

      rc = msgBuildInsertMsg( &pBuff, &buffSize, pCollection, flag, 0,
                              &insertor );
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build insertMsg failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertUpsert( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      return _convertUpdateBase( pAdaptor, msg, TRUE ) ;
   }

   INT32 RestToMSGTransfer::_convertUpdate( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      return _convertUpdateBase( pAdaptor, msg, FALSE ) ;
   }

   INT32 RestToMSGTransfer::_convertUpdateBase( restAdaptor *pAdaptor,
                                                MsgHeader **msg,
                                                BOOLEAN isUpsert )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pFlag     = NULL ;
      SINT32 flag           = 0 ;
      const CHAR *pCollection = NULL ;
      const CHAR *pUpdator    = NULL ;
      const CHAR *pMatcher    = NULL ;
      const CHAR *pHint       = NULL ;
      BSONObj updator ;
      BSONObj matcher ;
      BSONObj hint ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_FLAG, &pFlag ) ;
      if ( NULL != pFlag )
      {
         rc = utilMsgFlag( pFlag, flag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s,"
                        "value=%s", REST_KEY_NAME_FLAG, pFlag ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_FILTER, &pMatcher ) ;
      if ( NULL == pMatcher )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pMatcher ) ;
      }

      if ( NULL != pMatcher )
      {
         rc = fromjson( pMatcher, matcher, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s],"
                        "value=%s", FIELD_NAME_FILTER,
                        REST_KEY_NAME_MATCHER, pMatcher ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_UPDATOR, &pUpdator ) ;
      if ( NULL == pUpdator )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get collection's %s failed",
                     REST_KEY_NAME_UPDATOR ) ;
         goto error ;
      }

      rc = fromjson( pUpdator, updator, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                     REST_KEY_NAME_UPDATOR, pUpdator ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_HINT, &pHint ) ;
      if ( NULL != pHint )
      {
         rc = fromjson( pHint, hint, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                        FIELD_NAME_HINT, pHint ) ;
            goto error ;
         }
      }

      if ( isUpsert )
      {
         const CHAR* pSetOnInsert = NULL ;
         BSONObj setOnInsert ;

         pAdaptor->getQuery( _restSession, REST_KEY_NAME_SET_ON_INSERT, &pSetOnInsert ) ;
         if ( NULL != pSetOnInsert )
         {
            rc = fromjson( pSetOnInsert, setOnInsert, 0 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                           REST_KEY_NAME_SET_ON_INSERT, pSetOnInsert ) ;
               goto error ;
            }

            try
            {
               BSONObjBuilder newHintBuilder ;
               if ( !hint.isEmpty() )
               {
                  newHintBuilder.appendElements( hint ) ;
               }
               newHintBuilder.append( FIELD_NAME_SET_ON_INSERT, setOnInsert ) ;
               hint = newHintBuilder.obj() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG_MSG ( PDERROR, "Failed to create BSON object: %s",
                        e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         flag |= FLG_UPDATE_UPSERT ;
      }

      rc = msgBuildUpdateMsg( &pBuff, &buffSize, pCollection, flag, 0,
                              &matcher, &updator, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build updateMsg failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertDelete( restAdaptor *pAdaptor,
                                            MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pFlag     = NULL ;
      SINT32 flag           = 0 ;
      const CHAR *pCollection = NULL ;
      const CHAR *pDeletor    = NULL ;
      const CHAR *pHint       = NULL ;
      BSONObj deletor ;
      BSONObj hint ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_FLAG, &pFlag ) ;
      if ( NULL != pFlag )
      {
         flag = ossAtoi( pFlag ) ;
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_DELETOR, &pDeletor ) ;
      if ( NULL == pDeletor )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pDeletor ) ;
      }

      if ( NULL != pDeletor )
      {
         rc = fromjson( pDeletor, deletor, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s],"
                        "value=%s", REST_KEY_NAME_DELETOR,
                        REST_KEY_NAME_MATCHER, pDeletor ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_HINT, &pHint ) ;
      if ( NULL != pHint )
      {
         rc = fromjson( pHint, hint, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                        FIELD_NAME_HINT, pHint ) ;
            goto error ;
         }
      }

      rc = msgBuildDeleteMsg( &pBuff, &buffSize, pCollection, flag, 0,
                              &deletor, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build deleteMsg failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertAlterCollection( restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION ;
      const CHAR *pOption   = NULL ;
      const CHAR *pCollection = NULL ;
      BSONObj option ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOption ) ;
      if ( NULL == pOption )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get alter collection's %s failed",
                     FIELD_NAME_OPTIONS ) ;
         goto error ;
      }

      rc = fromjson( pOption, option, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                     FIELD_NAME_OPTIONS, pOption ) ;
         goto error ;
      }

      query = BSON( FIELD_NAME_NAME << pCollection
                    << FIELD_NAME_OPTIONS << option ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertCreateIndex( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      INT32 rc                = SDB_OK ;
      CHAR *pBuff             = NULL ;
      INT32 buffSize          = 0 ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX ;
      const CHAR *pIndexName  = NULL ;
      const CHAR *pIndexDef   = NULL ;
      const CHAR *pUnique     = NULL ;
      const CHAR *pEnforced   = NULL ;
      const CHAR *pSortBufferSize = NULL ;
      const CHAR *pCollection = NULL ;

      BSONObj indexDef ;

      bool isUnique   = false ;
      bool isEnforced = false ;
      INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                          &pCollection ) ;
      if ( NULL == pCollection )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     REST_KEY_NAME_COLLECTION ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_INDEXNAME, &pIndexName ) ;
      if ( NULL == pIndexName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_INDEXNAME ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, IXM_FIELD_NAME_INDEX_DEF, &pIndexDef ) ;
      if ( NULL == pIndexDef )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     IXM_FIELD_NAME_INDEX_DEF ) ;
         goto error ;
      }

      rc = fromjson( pIndexDef, indexDef, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                     IXM_FIELD_NAME_INDEX_DEF, pIndexDef ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, IXM_FIELD_NAME_UNIQUE, &pUnique ) ;
      if ( NULL != pUnique )
      {
         if ( ossStrcasecmp( pUnique, "true" ) == 0 )
         {
            isUnique = true ;
         }
         else if ( ossStrcasecmp( pUnique, "false" ) == 0 )
         {
            isUnique = false ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        IXM_FIELD_NAME_UNIQUE, pUnique ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, IXM_FIELD_NAME_ENFORCED, &pEnforced ) ;
      if ( NULL != pEnforced )
      {
         if ( ossStrcasecmp( pEnforced, "true" ) == 0 )
         {
            isEnforced = true ;
         }
         else if ( ossStrcasecmp( pEnforced, "false" ) == 0 )
         {
            isEnforced = false ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        IXM_FIELD_NAME_ENFORCED, pEnforced ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, IXM_FIELD_NAME_SORT_BUFFER_SIZE, &pSortBufferSize ) ;
      if ( NULL != pSortBufferSize )
      {
         sortBufferSize = ossAtoi ( pSortBufferSize ) ;
         if ( sortBufferSize < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "field's value error:field=%s, value=%s",
                        IXM_FIELD_NAME_SORT_BUFFER_SIZE, pSortBufferSize ) ;
            goto error ;
         }
      }

      {
         BSONObj index ;
         BSONObj query ;
         BSONObj hint ;
         index = BSON( IXM_FIELD_NAME_KEY << indexDef
                       << IXM_FIELD_NAME_NAME << pIndexName
                       << IXM_FIELD_NAME_UNIQUE << isUnique
                       << IXM_FIELD_NAME_ENFORCED << isEnforced ) ;

         query = BSON( FIELD_NAME_COLLECTION << pCollection
                       << FIELD_NAME_INDEX << index ) ;
         hint = BSON( IXM_FIELD_NAME_SORT_BUFFER_SIZE << sortBufferSize ) ;

         rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                                &query, NULL, NULL, &hint ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                        pCommand, rc ) ;
            goto error ;
         }
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertDropIndex( restAdaptor *pAdaptor,
                                               MsgHeader **msg )
   {
      INT32 rc                = SDB_OK ;
      CHAR *pBuff             = NULL ;
      INT32 buffSize          = 0 ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ;
      const CHAR *pIndexName  = NULL ;
      const CHAR *pCollection = NULL ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                          &pCollection ) ;
      if ( NULL == pCollection )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     REST_KEY_NAME_COLLECTION ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_INDEXNAME, &pIndexName ) ;
      if ( NULL == pIndexName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_INDEXNAME ) ;
         goto error ;
      }

      {
         BSONObj index ;
         BSONObj query ;
         index = BSON( "" << pIndexName ) ;
         query = BSON( FIELD_NAME_COLLECTION << pCollection
                       << FIELD_NAME_INDEX << index ) ;

         rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                                &query, NULL, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                        pCommand, rc ) ;
            goto error ;
         }
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSplit( restAdaptor *pAdaptor,
                                           MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SPLIT ;
      const CHAR *pSource   = NULL ;
      const CHAR *pTarget   = NULL ;
      const CHAR *pAsync    = NULL ;
      const CHAR *pCollection    = NULL ;
      const CHAR *pSplitQuery    = NULL ;
      const CHAR *pSplitEndQuery = NULL ;
      const CHAR *pPercent = NULL ;
      BOOLEAN isUsePercent = FALSE ;
      INT32 percent        = 0 ;
      bool bAsync   = false ;
      BSONObj splitQuery ;
      BSONObj splitEndQuery ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_SOURCE, &pSource ) ;
      if ( NULL == pSource )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get split's %s failed", FIELD_NAME_SOURCE ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_TARGET, &pTarget ) ;
      if ( NULL == pTarget )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get split's %s failed", FIELD_NAME_TARGET ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_SPLITPERCENT, &pPercent ) ;
      if ( NULL == pPercent )
      {
         pAdaptor->getQuery( _restSession, FIELD_NAME_SPLITQUERY,
                             &pSplitQuery ) ;
         if ( NULL == pSplitQuery )
         {
            pAdaptor->getQuery( _restSession, REST_KEY_NAME_LOWBOUND,
                                &pSplitQuery ) ;
            if ( NULL == pSplitQuery )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "get split's %s[or %s] failed",
                           FIELD_NAME_SPLITQUERY, REST_KEY_NAME_LOWBOUND ) ;
               goto error ;
            }
         }

         rc = fromjson( pSplitQuery, splitQuery, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s],"
                        "value=%s", FIELD_NAME_SPLITQUERY,
                        REST_KEY_NAME_LOWBOUND, pSplitQuery ) ;
            goto error ;
         }

         pAdaptor->getQuery( _restSession, FIELD_NAME_SPLITENDQUERY,
                             &pSplitEndQuery ) ;
         if ( NULL == pSplitEndQuery )
         {
            pAdaptor->getQuery( _restSession, REST_KEY_NAME_UPBOUND,
                                &pSplitEndQuery ) ;
         }

         if ( NULL != pSplitEndQuery )
         {
            rc = fromjson( pSplitEndQuery, splitEndQuery, 0 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                           FIELD_NAME_SPLITENDQUERY, pSplitEndQuery ) ;
               goto error ;
            }
         }
      }
      else
      {
         isUsePercent = TRUE ;
         percent      = ossAtoi( pPercent ) ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_ASYNC, &pAsync ) ;
      if ( NULL != pAsync )
      {
         if ( ossStrcasecmp( pAsync, "TRUE" ) == 0 )
         {
            bAsync = true ;
         }
      }

      if ( isUsePercent )
      {
         query = BSON( FIELD_NAME_NAME << pCollection << FIELD_NAME_SOURCE
                       << pSource << FIELD_NAME_TARGET << pTarget
                       << FIELD_NAME_SPLITPERCENT << ( FLOAT64 )percent
                       << FIELD_NAME_ASYNC << bAsync ) ;
      }
      else
      {
         query = BSON( FIELD_NAME_NAME << pCollection << FIELD_NAME_SOURCE
                       << pSource << FIELD_NAME_TARGET << pTarget
                       << FIELD_NAME_SPLITQUERY << splitQuery
                       << FIELD_NAME_SPLITENDQUERY << splitEndQuery
                       << FIELD_NAME_ASYNC << bAsync ) ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertTruncateCollection( restAdaptor *pAdaptor,
                                                        MsgHeader **msg )
   {
      INT32 rc                = SDB_OK ;
      INT32 buffSize          = 0 ;
      CHAR *pBuff             = NULL ;
      const CHAR *pCollection = NULL ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get collection's %s failed",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_COLLECTION, pCollection ) ;
         query = ob.obj () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_coverAttachCollection( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc                = SDB_OK ;
      CHAR *pBuff             = NULL ;
      INT32 buffSize          = 0 ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_LINK_CL ;
      const CHAR *pLowbound   = NULL ;
      const CHAR *pUpbound    = NULL ;
      const CHAR *pCollection = NULL ;
      const CHAR *pSubCLName  = NULL ;
      BSONObj lowbound ;
      BSONObj upbound ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_SUBCLNAME, &pSubCLName ) ;
      if ( NULL == pSubCLName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field[%s] failed",
                     REST_KEY_NAME_SUBCLNAME ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_LOWBOUND, &pLowbound ) ;
      if ( NULL == pLowbound )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field[%s] failed",
                     REST_KEY_NAME_LOWBOUND ) ;
         goto error ;
      }

      rc = fromjson( pLowbound, lowbound, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                     REST_KEY_NAME_LOWBOUND, pLowbound ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_UPBOUND, &pUpbound ) ;
      if ( NULL == pUpbound )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field[%s] failed",
                     REST_KEY_NAME_UPBOUND ) ;
         goto error ;
      }

      rc = fromjson( pUpbound, upbound, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                     REST_KEY_NAME_UPBOUND, pUpbound ) ;
         goto error ;
      }

      query = BSON( FIELD_NAME_NAME << pCollection
                    << FIELD_NAME_SUBCLNAME << pSubCLName
                    << FIELD_NAME_LOWBOUND << lowbound
                    << FIELD_NAME_UPBOUND << upbound ) ;

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &query,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_coverDetachCollection( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc                = SDB_OK ;
      INT32 buffSize          = 0 ;
      CHAR *pBuff             = NULL ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL ;
      const CHAR *pCollection = NULL ;
      const CHAR *pSubCLName  = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                          &pCollection ) ;
      if ( NULL == pCollection )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get collection's %s failed",
                     REST_KEY_NAME_COLLECTION ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_SUBCLNAME, &pSubCLName ) ;
      if ( NULL == pSubCLName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field[%s] failed",
                     REST_KEY_NAME_SUBCLNAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_NAME, pCollection ) ;
         ob.append ( FIELD_NAME_SUBCLNAME, pSubCLName ) ;
         query = ob.obj () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListGroups( restAdaptor *pAdaptor,
                                                MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPS ;

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, NULL,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertStartGroup( restAdaptor *pAdaptor,
                                                MsgHeader **msg )
   {
      INT32 rc             = SDB_OK ;
      INT32 buffSize       = 0 ;
      CHAR *pBuff          = NULL ;
      const CHAR *pName    = NULL ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pName ) ;
      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_GROUPNAME, pName ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertStopGroup( restAdaptor *pAdaptor,
                                               MsgHeader **msg )
   {
      INT32 rc             = SDB_OK ;
      INT32 buffSize       = 0 ;
      CHAR *pBuff          = NULL ;
      const CHAR *pName    = NULL ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_GROUP ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pName ) ;
      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_GROUPNAME, pName ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertStartNode( restAdaptor *pAdaptor,
                                               MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      INT32 buffSize        = 0 ;
      CHAR *pBuff           = NULL ;
      const CHAR *pHostName = NULL ;
      const CHAR *pSvcname  = NULL ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_STARTUP_NODE ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_HOST, &pHostName ) ;
      if ( NULL == pHostName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_HOST ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, OM_CONF_DETAIL_SVCNAME, &pSvcname ) ;
      if ( NULL == pSvcname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_CONF_DETAIL_SVCNAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_HOST, pHostName ) ;
         ob.append ( OM_CONF_DETAIL_SVCNAME, pSvcname ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertStopNode( restAdaptor *pAdaptor,
                                              MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      INT32 buffSize        = 0 ;
      CHAR *pBuff           = NULL ;
      const CHAR *pHostName = NULL ;
      const CHAR *pSvcname  = NULL ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_NODE ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_HOST, &pHostName ) ;
      if ( NULL == pHostName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_HOST ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, OM_CONF_DETAIL_SVCNAME, &pSvcname ) ;
      if ( NULL == pSvcname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_CONF_DETAIL_SVCNAME ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_HOST, pHostName ) ;
         ob.append ( OM_CONF_DETAIL_SVCNAME, pSvcname ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertGetCount( restAdaptor *pAdaptor,
                                              MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT ;
      const CHAR *pCollection = NULL ;
      const CHAR *pMatcher    = NULL ;
      const CHAR *pHint       = NULL ;
      BSONObj matcher ;
      BSONObj hint ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pCollection ) ;
      if ( NULL == pCollection )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION,
                             &pCollection ) ;
         if ( NULL == pCollection )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s[or %s] failed",
                        FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_FILTER, &pMatcher ) ;
      if ( NULL == pMatcher )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pMatcher ) ;
      }

      if ( NULL != pMatcher )
      {
         rc = fromjson( pMatcher, matcher, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s],"
                        "value=%s", FIELD_NAME_FILTER, REST_KEY_NAME_MATCHER,
                        pMatcher ) ;
            goto error ;
         }
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_HINT, &pHint ) ;
      if ( NULL != pHint )
      {
         rc = fromjson( pHint, hint, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s,value=%s",
                        FIELD_NAME_HINT, pHint ) ;
            goto error ;
         }
      }

      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_COLLECTION, pCollection ) ;
         if ( !hint.isEmpty() )
         {
            builder.append( FIELD_NAME_HINT, hint ) ;
         }

         hint = builder.obj() ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &matcher,
                             NULL, NULL, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListBase( restAdaptor *pAdaptor,
                                              BSONObj &match, BSONObj &selector,
                                              BSONObj &order )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pOrder    = NULL ;
      const CHAR *pMatch    = NULL ;
      const CHAR *pSelector = NULL ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_FILTER, &pMatch ) ;
      if ( NULL == pMatch )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pMatch ) ;
      }
      pAdaptor->getQuery( _restSession, FIELD_NAME_SELECTOR, &pSelector ) ;
      pAdaptor->getQuery( _restSession, FIELD_NAME_SORT, &pOrder ) ;
      if ( NULL == pOrder )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_ORDERBY, &pOrder ) ;
      }

      if ( NULL != pMatch )
      {
         rc = fromjson( pMatch, match, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s], "
                        "value=%s", FIELD_NAME_FILTER, REST_KEY_NAME_MATCHER,
                        pMatch ) ;
            goto error ;
         }
      }

      if ( NULL != pSelector )
      {
         rc = fromjson( pSelector, selector, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_SELECTOR, pSelector ) ;
            goto error ;
         }
      }

      if ( NULL != pOrder )
      {
         rc = fromjson( pOrder, order, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s[or %s], "
                        "value=%s", FIELD_NAME_SORT, REST_KEY_NAME_ORDERBY,
                        pOrder ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListContexts( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXTS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListContextsCurrent( restAdaptor *pAdaptor,
                                                         MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXTS_CURRENT ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListSessions( restAdaptor *pAdaptor,
                                                  MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSIONS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListSessionsCurrent( restAdaptor *pAdaptor,
                                                         MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSIONS_CURRENT ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListCollections( restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListCollectionSpaces( restAdaptor *pAdaptor,
                                                          MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListStorageUnits( restAdaptor *pAdaptor,
                                                      MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_STORAGEUNITS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertCreateProcedure( restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc             = SDB_OK ;
      INT32 buffSize       = 0 ;
      CHAR *pBuff          = NULL ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_CRT_PROCEDURE ;
      const CHAR *pJsCode  = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_CODE,
                          &pJsCode ) ;
      if ( NULL == pJsCode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     REST_KEY_NAME_CODE ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.appendCode ( FIELD_NAME_FUNC, pJsCode ) ;
         ob.append ( FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertRemoveProcedure( restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      INT32 buffSize        = 0 ;
      CHAR *pBuff           = NULL ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_RM_PROCEDURE ;
      const CHAR *pFuncName = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_FUNCTION,
                          &pFuncName ) ;
      if ( NULL == pFuncName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     REST_KEY_NAME_FUNCTION ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_FUNC, pFuncName ) ;
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListProcedures( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      INT32 buffSize = 0 ;
      SINT64 skip = 0 ;
      SINT64 returnRow = -1 ;
      CHAR *pBuff = NULL ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_LIST_PROCEDURES ;
      const CHAR *pSkip = NULL ;
      const CHAR *pReturnRow = NULL ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }
      pAdaptor->getQuery( _restSession, FIELD_NAME_SKIP, &pSkip ) ;
      pAdaptor->getQuery( _restSession, FIELD_NAME_RETURN_NUM, &pReturnRow ) ;
      if ( NULL == pReturnRow )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_LIMIT, &pReturnRow ) ;
      }
      if ( NULL != pSkip )
      {
         skip = ossAtoll( pSkip ) ;
      }

      if ( NULL != pReturnRow )
      {
         returnRow = ossAtoll( pReturnRow ) ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, skip,
                             returnRow, &match, &selector, &order,
                             NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertAlterDomain( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      INT32 buffSize = 0 ;
      CHAR *pBuff = NULL ;
      const CHAR *pName = NULL ;
      const CHAR *pOptions = NULL ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN ;
      BSONObj query ;
      BSONObj options ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pName ) ;
      if( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOptions ) ;
      if ( NULL == pOptions )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_OPTIONS ) ;
         goto error ;
      }

      rc = fromjson( pOptions, options, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                     FIELD_NAME_OPTIONS, pOptions ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_NAME, pName ) ;
         ob.append ( FIELD_NAME_OPTIONS, options ) ;
         query = ob.obj () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListDomains( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_DOMAINS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertCreateDomain( restAdaptor *pAdaptor,
                                                  MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      INT32 buffSize        = 0 ;
      CHAR *pBuff           = NULL ;
      const CHAR *pName     = NULL ;
      const CHAR *pOptions  = NULL ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_CREATE_DOMAIN ;
      BSONObj query ;
      BSONObj options ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pName ) ;
      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOptions ) ;
      if( NULL != pOptions )
      {
         rc = fromjson( pOptions, options, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, "
                        "value=%s", FIELD_NAME_OPTIONS, pOptions ) ;
            goto error ;
         }
      }

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_NAME, pName ) ;
         if( pOptions != NULL )
         {
            ob.append ( FIELD_NAME_OPTIONS, options ) ;
         }
         query = ob.obj () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertDropDomain( restAdaptor *pAdaptor,
                                                MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_DROP_DOMAIN ;
      const CHAR *pName     = NULL ;
      BSONObj query ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME,
                          &pName ) ;
      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_NAME ) ;
         goto error ;
      }

      query = BSON( FIELD_NAME_NAME << pName ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListTasks( restAdaptor *pAdaptor,
                                               MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_TASKS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListIndexes( restAdaptor *pAdaptor,
                                                 MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;
      const CHAR *pTable    = NULL ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &pTable ) ;
      if ( NULL == pTable )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION, &pTable ) ;
         if ( NULL == pTable )
         {
            PD_LOG_MSG( PDERROR, "get field failed:field=%s[or %s]",
                     FIELD_NAME_NAME, REST_KEY_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      hint = BSON( FIELD_NAME_COLLECTION << pTable ) ;

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, NULL,
                             NULL, NULL, &hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListCSInDomain( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_CS_IN_DOMAIN ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListCLInDomain( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_LIST_CL_IN_DOMAIN ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert list failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertListLobs( restAdaptor *pAdaptor,
                                              MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj match ;
      CHAR *pBuff          = NULL ;
      INT32 buffSize       = 0 ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS ;
      const CHAR *clName   = NULL ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_NAME, &clName ) ;
      if ( NULL == clName )
      {
         pAdaptor->getQuery( _restSession, REST_KEY_NAME_COLLECTION, &clName ) ;
         if ( NULL == clName )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get collection's %s failed",
                        FIELD_NAME_NAME ) ;
            goto error ;
         }
      }

      match = BSON( FIELD_NAME_COLLECTION << clName ) ;
      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }






   INT32 RestToMSGTransfer::_convertSnapshotContext( restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotContextCurrent(
                                                     restAdaptor *pAdaptor,
                                                     MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX
                              CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotSessions( restAdaptor *pAdaptor,
                                                      MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotSessionsCurrent(
                                                        restAdaptor *pAdaptor,
                                                        MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX
                              CMD_NAME_SNAPSHOT_SESSIONS_CURRENT ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotCollections( restAdaptor *pAdaptor,
                                                         MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTIONS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotCollectionSpaces(
                                                         restAdaptor *pAdaptor,
                                                         MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX
                              CMD_NAME_SNAPSHOT_COLLECTIONSPACES ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotDatabase( restAdaptor *pAdaptor,
                                                      MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotSystem( restAdaptor *pAdaptor,
                                                    MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotCata( restAdaptor *pAdaptor,
                                                  MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CATA ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotAccessPlans ( restAdaptor * pAdaptor,
                                                          MsgHeader ** msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_ACCESSPLANS ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertSnapshotHealth ( restAdaptor * pAdaptor,
                                                     MsgHeader ** msg )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj match ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pCommand  = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_HEALTH ;

      rc = _convertListBase( pAdaptor, match, selector, order ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "convert snapshot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1, &match,
                             &selector, &order, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_buildExecMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                           const CHAR *pSql, UINT64 reqID )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == pSql )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      {
         INT32 sqlLen = ossStrlen( pSql ) + 1 ;
         MsgOpSql *sqlMsg = NULL ;
         INT32 len = sizeof( MsgOpSql ) +
                     ossRoundUpToMultipleX( sqlLen, 4 ) ;
         if ( len < 0 )
         {
            ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = msgCheckBuffer( ppBuffer, bufferSize, len ) ;
         if ( rc )
         {
            ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }

         sqlMsg                       = ( MsgOpSql *)( *ppBuffer ) ;
         sqlMsg->header.requestID     = reqID ;
         sqlMsg->header.opCode        = MSG_BS_SQL_REQ ;
         sqlMsg->header.messageLength = sizeof( MsgOpSql ) + sqlLen ;
         sqlMsg->header.routeID.value = 0 ;
         sqlMsg->header.TID           = ossGetCurrentThreadID() ;
         ossMemcpy( *ppBuffer + sizeof( MsgOpSql ),
                    pSql, sqlLen ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertExec( restAdaptor *pAdaptor,
                                          MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pSql     = NULL ;
      CHAR *pBuff          = NULL ;
      INT32 buffSize       = 0 ;

      pAdaptor->getQuery( _restSession, REST_KEY_NAME_SQL, &pSql ) ;
      if ( NULL == pSql )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get exec's field failed:field=%s",
                     REST_KEY_NAME_SQL ) ;
         goto error ;
      }

      rc = _buildExecMsg( &pBuff, &buffSize, pSql, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build exec command failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertForceSession( restAdaptor *pAdaptor,
                                                  MsgHeader **msg )
   {
      INT32 rc = SDB_OK ;
      INT32 buffSize = 0 ;
      SINT64 sessionId = 0 ;
      CHAR *pBuff = NULL ;
      const CHAR *pOption    = NULL ;
      const CHAR *pSessionId = NULL ;
      const CHAR *pCommand   = CMD_ADMIN_PREFIX CMD_NAME_FORCE_SESSION ;
      BSONObj query ;
      BSONObj options ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_SESSIONID, &pSessionId ) ;
      if( NULL == pSessionId )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get exec's field failed:field=%s",
                     FIELD_NAME_SESSIONID ) ;
         goto error ;
      }
      sessionId = ossAtoll( pSessionId ) ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOption ) ;
      if( NULL != pOption )
      {
         rc = fromjson( pOption, options, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_OPTIONS, pOption ) ;
            goto error ;
         }
      }

      try
      {
         BSONObjBuilder ob ;
         BSONObjIterator it( options ) ;
         ob.appendNumber( FIELD_NAME_SESSIONID, sessionId ) ;
         while( it.more() )
         {
            BSONElement ele = it.next() ;
            if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_SESSIONID ) )
            {
               ob.append( ele ) ;
            }
         }
         query = ob.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }






   INT32 RestToMSGTransfer::_convertLogin( restAdaptor *pAdaptor,
                                           MsgHeader **msg )
   {
      INT32 rc              = SDB_OK ;
      CHAR *pBuff           = NULL ;
      INT32 buffSize        = 0 ;
      const CHAR *pUser     = NULL ;
      const CHAR *pPasswd   = NULL ;

      pAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_NAME,
                          &pUser ) ;
      if ( NULL == pUser )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get login name failed:field=%s",
                     OM_REST_FIELD_LOGIN_NAME ) ;
         goto error ;
      }

      pAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_PASSWD,
                          &pPasswd ) ;
      if ( NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get passwd failed:field=%s",
                     OM_REST_FIELD_LOGIN_PASSWD ) ;
         goto error ;
      }

      rc = msgBuildAuthMsg( &pBuff, &buffSize, pUser, pPasswd, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "buildAuthMsg failed:rc=%d", rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RestToMSGTransfer::_convertAnalyze ( restAdaptor * pAdaptor,
                                              MsgHeader ** msg )
   {
      INT32 rc = SDB_OK ;

      INT32 buffSize = 0 ;
      CHAR *pBuff = NULL ;
      const CHAR *pOption    = NULL ;
      const CHAR *pCommand   = CMD_ADMIN_PREFIX CMD_NAME_ANALYZE ;
      BSONObj query ;
      BSONObj options ;

      pAdaptor->getQuery( _restSession, FIELD_NAME_OPTIONS, &pOption ) ;
      if( NULL != pOption )
      {
         rc = fromjson( pOption, options, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "field's format error:field=%s, value=%s",
                        FIELD_NAME_OPTIONS, pOption ) ;
            goto error ;
         }
      }

      try
      {
         BSONObjBuilder builder ;
         builder.appendElements( options ) ;
         query = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build command failed:command=%s, rc=%d",
                     pCommand, rc ) ;
         goto error ;
      }

      *msg = ( MsgHeader * )pBuff ;

   done:
      return rc ;

   error:
      goto done ;
   }

}

