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

   Source File Name = omCommand.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2014  LYB Initial Draft
          04/01/2017  HJW Increase extend business and shrink business

   Last Changed =

*******************************************************************************/

#include "omCommand.hpp"
#include "omDef.hpp"
#include "rtn.hpp"
#include "rtnContextDump.hpp"
#include "omManager.hpp"
#include "omConfigBuilder.hpp"
#include "omTaskManager.hpp"
#include "omStrategyMgr.hpp"
#include "../bson/lib/md5.hpp"
#include "../bson/lib/base64.h"
#include "../util/des.h"
#include "ossPath.hpp"
#include "ossProc.hpp"
#include "ossVer.hpp"
#include <set>
#include <stdlib.h>
#include <sstream>

using namespace bson;
using namespace boost::property_tree;


namespace engine
{
   static INT32 getPacketFile( const string &businessType, string &filePath ) ;
   static INT32 getMaxTaskID( INT64 &taskID ) ;
   static INT32 createTask( INT32 taskType, INT64 taskID,
                            const string &taskName, const string &agentHost,
                            const string &agentService, const BSONObj &taskInfo,
                            const BSONArray &resultInfo ) ;

   static INT32 removeTask( INT64 taskID ) ;

   INT32 getPacketFile( const string &businessType, string &filePath )
   {
      INT32 rc = SDB_OK ;
      CHAR tmpPath[ OSS_MAX_PATHSIZE + 1 ] = "" ;
      string filter = businessType + "*" ;
      multimap< string, string> mapFiles ;

      ossGetEWD( tmpPath, OSS_MAX_PATHSIZE ) ;
      utilCatPath( tmpPath, OSS_MAX_PATHSIZE, ".." ) ;
      utilCatPath( tmpPath, OSS_MAX_PATHSIZE, OM_PACKET_SUBPATH ) ;

      if ( NULL == ossGetRealPath( tmpPath, tmpPath, OSS_MAX_PATHSIZE ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "path is invalid:path=%s,rc=%d", tmpPath, rc ) ;
         goto error ;
      }

      rc = ossEnumFiles( tmpPath, mapFiles, filter.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "path is invalid:path=%s,filter=%s,rc=%d",
                     tmpPath, filter.c_str(), rc ) ;
         goto error ;
      }

      if ( mapFiles.size() != 1 )
      {
         rc = SDB_FNE ;
         PD_LOG_MSG( PDERROR, "path is invalid:path=%s,filter=%s,count=%d",
                     tmpPath, filter.c_str(), mapFiles.size() ) ;
         goto error ;
      }

      filePath = mapFiles.begin()->second ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 getMaxTaskID( INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      SINT64 contextID   = -1 ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;

      taskID = 0 ;

      selector    = BSON( OM_TASKINFO_FIELD_TASKID << 1 ) ;
      orderBy     = BSON( OM_TASKINFO_FIELD_TASKID << -1 ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, orderBy,
                     hint, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get taskid failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONElement ele ;
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               goto done ;
            }

            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         ele    = result.getField( OM_TASKINFO_FIELD_TASKID ) ;
         taskID = ele.numberLong() ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 createTask( INT32 taskType, INT64 taskID, const string &taskName,
                     const string &agentHost, const string &agentService,
                     const BSONObj &taskInfo, const BSONArray &resultInfo )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      BSONObj record ;

      BSONObjBuilder builder ;
      time_t now = time( NULL ) ;
      builder.append( OM_TASKINFO_FIELD_TASKID, taskID ) ;
      builder.append( OM_TASKINFO_FIELD_TYPE, taskType ) ;
      builder.append( OM_TASKINFO_FIELD_TYPE_DESC,
                      getTaskTypeStr( taskType ) ) ;
      builder.append( OM_TASKINFO_FIELD_NAME, taskName ) ;
      builder.appendTimestamp( OM_TASKINFO_FIELD_CREATE_TIME,
                               (unsigned long long)now * 1000, 0 ) ;
      builder.appendTimestamp( OM_TASKINFO_FIELD_END_TIME,
                               (unsigned long long)now * 1000, 0 ) ;
      builder.append( OM_TASKINFO_FIELD_STATUS, OM_TASK_STATUS_INIT ) ;
      builder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                      getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
      builder.append( OM_TASKINFO_FIELD_AGENTHOST, agentHost ) ;
      builder.append( OM_TASKINFO_FIELD_AGENTPORT, agentService ) ;
      builder.append( OM_TASKINFO_FIELD_INFO, taskInfo ) ;
      builder.append( OM_TASKINFO_FIELD_ERRNO, SDB_OK ) ;
      builder.append( OM_TASKINFO_FIELD_DETAIL, "" ) ;
      builder.append( OM_TASKINFO_FIELD_PROGRESS, 0 ) ;
      builder.append( OM_TASKINFO_FIELD_RESULTINFO, resultInfo ) ;

      record = builder.obj() ;
      rc = rtnInsert( OM_CS_DEPLOY_CL_TASKINFO, record, 1, 0, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "insert task failed:rc=%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 removeTask( INT64 taskID )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      BSONObj deletor ;
      BSONObj hint ;
      deletor = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = rtnDelete( OM_CS_DEPLOY_CL_TASKINFO, deletor, hint, 0, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "rtnDelete task failed:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", taskID, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *getTaskStatusStr( INT32 status )
   {
      static CHAR *s_taskStatusStr[ OM_TASK_STATUS_END ] =
                                                 {
                                                    OM_TASK_STATUS_INIT_STR,
                                                    OM_TASK_STATUS_RUNNING_STR,
                                                    OM_TASK_STATUS_ROLLBACK_STR,
                                                    OM_TASK_STATUS_CANCEL_STR,
                                                    OM_TASK_STATUS_FINISH_STR
                                                 } ;
      if ( status < OM_TASK_STATUS_END )
      {
         return s_taskStatusStr[status] ;
      }
      else
      {
         return "unknown" ;
      }
   }

   const CHAR *getTaskTypeStr( INT32 type )
   {
      static CHAR *s_taskTypeStr[ OM_TASK_TYPE_END ] = {
         OM_TASK_TYPE_ADD_HOST_STR,
         OM_TASK_TYPE_REMOVE_HOST_STR,
         OM_TASK_TYPE_ADD_BUSINESS_STR,
         OM_TASK_TYPE_REMOVE_BUSINESS_STR,
         OM_TASK_TYPE_SSQL_EXEC_STR,
         OM_TASK_TYPE_EXTEND_BUSINESS_STR,
         OM_TASK_TYPE_SHRINK_BUSINESS_STR,
         OM_TASK_TYPE_DEPLOY_PACKAGE_STR,
      } ;

      if ( type < OM_TASK_TYPE_END )
      {
         return s_taskTypeStr[type] ;
      }
      else
      {
         return "unknown" ;
      }
   }

   omContextAssist::omContextAssist( SINT64 contextID, pmdEDUCB *cb,
                                     SDB_RTNCB *rtncb )
   {
      SDB_ASSERT( NULL != cb, "cb can't be null" ) ;
      SDB_ASSERT( NULL != rtncb, "rtncb can't be null" ) ;
      SDB_ASSERT( -1 != contextID, "contextID can't be -1" ) ;
      _cb           = cb ;
      _rtncb        = rtncb ;
      _orgContextID = contextID ;
      _contextID    = -1 ;
   }

   omContextAssist::~omContextAssist()
   {
      if ( NULL != _rtncb && NULL != _cb )
      {
         if ( -1 != _orgContextID )
         {
            _rtncb->contextDelete( _orgContextID, _cb ) ;
            _orgContextID = -1 ;
         }

         if ( -1 != _contextID )
         {
            _rtncb->contextDelete( _contextID, _cb ) ;
            _contextID = -1 ;
         }
      }

      _rtncb = NULL ;
      _cb    = NULL ;
   }

   INT32 omContextAssist::init( const BSONObj &selector, const BSONObj &matcher,
                                INT64 numToSkip, INT64 numToReturn )
   {
      INT32 rc                 = SDB_OK ;
      rtnContextDump *pContext = NULL ;
      if ( -1 == _orgContextID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "org context is invalid" ) ;
         goto error ;
      }

      rc = _rtncb->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&pContext,
                                _contextID, _cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to create new dump context" ) ;
         goto error ;
      }

      rc = pContext->open( selector, matcher, numToReturn, numToSkip ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open dump context" ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( _orgContextID, 1, buffObj, _cb, _rtncb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            _orgContextID = -1 ;
            PD_LOG( PDERROR, "failed to get more,rc=%d", rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         rc = pContext->monAppend( record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "append record failed:rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != _orgContextID )
      {
         _rtncb->contextDelete( _orgContextID, _cb ) ;
         _orgContextID = -1 ;
      }

      return rc ;
   error:
      if ( -1 != _contextID )
      {
         _rtncb->contextDelete( _contextID, _cb ) ;
         _contextID = -1 ;
      }
      goto done ;
   }

   INT32 omContextAssist::getNext( BSONObj &data )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;
      rc = rtnGetMore ( _contextID, 1, buffObj, _cb, _rtncb ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }

         _contextID = -1 ;
         PD_LOG( PDERROR, "failed to get more,rc=%d", rc ) ;
         goto error ;
      }

      rc = buffObj.nextObj( data ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get next obj,rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( -1 != _contextID )
      {
         _rtncb->contextDelete( _contextID, _cb ) ;
         _contextID = -1 ;
      }

      goto done ;
   }

   omAuthCommand::omAuthCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession )
   : omRestCommandBase( pRestAdaptor, pRestSession )
   {
   }

   omAuthCommand::~omAuthCommand()
   {
   }

   string omAuthCommand::_getLanguage()
   {
      const CHAR *pLanguage = NULL ;
      _restAdaptor->getHttpHeader( _restSession, OM_REST_HEAD_LANGUAGE,
                                   &pLanguage ) ;
      if ( NULL != pLanguage
           && ossStrcasecmp( pLanguage, OM_REST_LANGUAGE_EN ) == 0 )
      {
         return OM_REST_LANGUAGE_EN ;
      }
      else
      {
         return OM_REST_LANGUAGE_ZH_CN ;
      }
   }

   void omAuthCommand::_setFileLanguageSep()
   {
      string language = _getLanguage() ;
      _languageFileSep = "_" + language ;
   }

   INT32 omAuthCommand::_getQueryPara( BSONObj &selector,  BSONObj &matcher,
                                       BSONObj &order, BSONObj &hint,
                                       SINT64 &numSkip, SINT64 &numReturn )
   {
      const CHAR *pSelector = NULL ;
      const CHAR *pMatcher  = NULL ;
      const CHAR *pOrder    = NULL ;
      const CHAR *pHint     = NULL ;
      const CHAR *pSkip     = NULL ;
      const CHAR *pReturn   = NULL ;
      INT32 rc = SDB_OK ;
      _restAdaptor->getQuery( _restSession, FIELD_NAME_SELECTOR, &pSelector ) ;
      if ( NULL != pSelector )
      {
         rc = fromjson( pSelector, selector ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                        "rc=%d", pSelector, rc ) ;
            goto error ;
         }
      }

      _restAdaptor->getQuery(_restSession, FIELD_NAME_FILTER, &pMatcher ) ;
      if( NULL == pMatcher )
      {
         _restAdaptor->getQuery( _restSession, REST_KEY_NAME_MATCHER, &pMatcher ) ;
      }
      if ( NULL != pMatcher )
      {
         rc = fromjson( pMatcher, matcher ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                        "rc=%d", pMatcher, rc ) ;
            goto error ;
         }
      }

      _restAdaptor->getQuery(_restSession, FIELD_NAME_SORT, &pOrder ) ;
      if( NULL == pOrder )
      {
         _restAdaptor->getQuery( _restSession,
                                 REST_KEY_NAME_ORDERBY,
                                 &pOrder ) ;
      }
      if ( NULL != pOrder )
      {
         rc = fromjson( pOrder, order ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                        "rc=%d", pOrder, rc ) ;
            goto error ;
         }
      }

      _restAdaptor->getQuery(_restSession, FIELD_NAME_HINT, &pHint ) ;
      if ( NULL != pHint )
      {
         rc = fromjson( pHint, hint ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                        "rc=%d", pHint, rc ) ;
            goto error ;
         }
      }

      numSkip = 0 ;
      _restAdaptor->getQuery(_restSession, FIELD_NAME_SKIP, &pSkip ) ;
      if ( NULL != pSkip )
      {
         numSkip = ossAtoll( pSkip ) ;
      }

      numReturn = -1 ;
      _restAdaptor->getQuery(_restSession, REST_KEY_NAME_LIMIT, &pReturn ) ;
      if ( NULL != pReturn )
      {
         numReturn = ossAtoll( pReturn ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omAuthCommand::_decryptPasswd( const string &encryptPasswd,
                                       const string &time,
                                       string &decryptPasswd )
   {
      decryptPasswd = encryptPasswd ;
   }

   INT32 omAuthCommand::_getSdbUsrInfo( const string &clusterName, string &sdbUser,
                                        string &sdbPasswd,
                                        string &sdbUserGroup )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder resultBuilder ;
      BSONObj result ;

      BSONObjBuilder builder ;
      builder.append( OM_CLUSTER_FIELD_NAME, clusterName ) ;
      BSONObj matcher ;
      matcher = builder.obj() ;

      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID             = -1 ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order, hint, 0,
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               PD_LOG_MSG( PDERROR, "cluster is not exist:cluster=%s,rc=%d",
                           clusterName.c_str(), rc ) ;
               goto error ;
            }

            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get recrod from table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         sdbUser      = record.getStringField( OM_CLUSTER_FIELD_SDBUSER ) ;
         sdbPasswd    = record.getStringField( OM_CLUSTER_FIELD_SDBPASSWD ) ;
         sdbUserGroup = record.getStringField( OM_CLUSTER_FIELD_SDBUSERGROUP ) ;
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omAuthCommand::doCommand()
   {
      const CHAR *pUserName        = NULL ;
      const CHAR *pPasswd          = NULL ;
      const CHAR *pTime            = NULL ;
      INT32 rc                     = SDB_OK ;
      ossSocket *socket            = NULL ;
      BSONObjBuilder authBuilder ;
      BSONObjBuilder resBuilder ;
      BSONObj bsonRes ;
      BSONObj bsonAuth ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_NAME,
                              &pUserName ) ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_PASSWD,
                              &pPasswd ) ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TIMESTAMP, &pTime ) ;
      if ( ( NULL == pUserName ) || ( NULL == pPasswd ) || ( NULL == pTime ) )
      {
         rc = SDB_INVALIDARG ;
         _errorDetail = string( OM_REST_FIELD_LOGIN_NAME ) + " or "
                        + OM_REST_FIELD_LOGIN_PASSWD + " or "
                        + OM_REST_FIELD_TIMESTAMP + " is null" ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      authBuilder.append( SDB_AUTH_USER, pUserName ) ;
      authBuilder.append( SDB_AUTH_PASSWD, pPasswd ) ;
      bsonAuth = authBuilder.obj() ;
      rc = sdbGetOMManager()->authenticate( bsonAuth, _cb ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
         {
            _sendErrorRes2Web( rc, "username or passwd is wrong" ) ;
         }
         else
         {
            _sendErrorRes2Web( rc, "system error" ) ;
         }

         goto error ;
      }

      socket = _restSession->socket() ;
      if ( NULL == socket )
      {
         PD_LOG( PDERROR, "socket is null, impossible" ) ;
         _sendErrorRes2Web( SDB_SYS, "system error" ) ;
         goto error ;
      }

      rc = _restSession->doLogin( pUserName, socket->getLocalIP() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "do login failed:user=%s, ip=%u", pUserName,
                 socket->getLocalIP() ) ;
         _sendErrorRes2Web( SDB_SYS, "system error" ) ;
         goto error ;
      }

      _restAdaptor->appendHttpHeader( _restSession, OM_REST_HEAD_SESSIONID,
                                      _restSession->getSessionID() ) ;
      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omExtendBusinessCommand::omExtendBusinessCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession,
                                                const CHAR *pRootPath,
                                                string& localAgentHost,
                                                string& localAgentService )
                           :omAuthCommand( pRestAdaptor, pRestSession ),
                            _rootPath( pRootPath ),
                            _localAgentHost( localAgentHost ),
                            _localAgentService( localAgentService )
   {
   }

   omExtendBusinessCommand::~omExtendBusinessCommand()
   {
   }

   INT32 omExtendBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj extendConfig ;
      BSONObj confProperties ;
      BSONObj buzInfoForCluster ;
      BSONObj hostsInfoForCluster ;
      string extendConfigMod ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo( extendConfig, extendConfigMod ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkBusiness() ;
      if( rc )
      {
         PD_LOG( PDERROR, "_checkBusiness failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _readConfigProperties( extendConfigMod, confProperties ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_combineConfDetail failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _getClusterInfo( hostsInfoForCluster, buzInfoForCluster ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "get cluster info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkExtendConfig( confProperties, hostsInfoForCluster,
                               buzInfoForCluster, extendConfig ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "check extend business config failed: rc=%d", rc ) ;
         goto error ;
      }

      rc = _createExtendTask( extendConfig, hostsInfoForCluster, taskID ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "failed to create extend task: rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << (long long)taskID ) ;
         _restAdaptor->appendHttpBody( _restSession, result.objdata(),
                                       result.objsize(), 1 ) ;
         _sendOKRes2Web() ;
      }

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omExtendBusinessCommand::_getRestInfo( BSONObj &extendConfig,
                                                string& extendConfigMod )
   {
      INT32 rc          = SDB_OK ;
      const CHAR *pInfo = NULL ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CONFIGINFO, &pInfo ) ;
      if( NULL == pInfo )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "rest field is null:field=%s",
                             OM_REST_FIELD_CONFIGINFO ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = fromjson( pInfo, extendConfig ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "invalid json: %s=%s",
                             OM_REST_FIELD_CONFIGINFO, pInfo ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _clusterName = extendConfig.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if( _clusterName == "" )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_CLUSTER_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessName = extendConfig.getStringField( OM_BSON_BUSINESS_NAME ) ;
      if( _businessName == "" )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      extendConfigMod = extendConfig.getStringField( OM_BSON_DEPLOY_MOD ) ;
      if( extendConfigMod == "" )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_DEPLOY_MOD ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessCommand::_checkBusiness()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj buzInfo ;
      string clusterName ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getOneBusinessInfo( _businessName, buzInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: %s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get exist business:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = buzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      _deployMod    = buzInfo.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;
      clusterName   = buzInfo.getStringField( OM_BUSINESS_FIELD_CLUSTERNAME ) ;

      if( _businessType != OM_BUSINESS_SEQUOIADB )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported business type:type=%s",
                             _businessType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if( _deployMod != OM_DEPLOY_MOD_DISTRIBUTION )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported deployment mode:type=%s",
                             _deployMod.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if( _clusterName != clusterName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "the cluster %s has no business %s",
                             _clusterName.c_str(), _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _businessName ) ;
      if( taskID >= 0 )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task["OSS_LL_PRINT_FORMAT"]",
                             _businessName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessCommand::_readConfigProperties(
                                                const string& extendConfigMod,
                                                BSONObj& buzDetail )
   {
      INT32 rc = SDB_OK ;
      string separateConfig ;
      list <BSONObj> deployModList ;
      list <BSONObj>::iterator iterList ;
      string operationType = OM_FIELD_OPERATION_EXTEND ;
      omConfigTool confTool( _rootPath, _languageFileSep ) ;

      rc = confTool.readBuzTemplate( _businessType, operationType,
                                     deployModList ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "read template file failed:file=%s,rc=%d",
                             confTool.getBuzTemplatePath( _businessType,
                                                          operationType )
                                                                .c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      iterList = deployModList.begin() ;
      while ( iterList != deployModList.end() )
      {
         string deployMod = iterList->getStringField( OM_BSON_DEPLOY_MOD ) ;
         if ( extendConfigMod == deployMod )
         {
            BSONObj bsonDeployMod = *iterList ;
            separateConfig = bsonDeployMod.getStringField(
                                                     OM_BSON_SEPARATE_CONFIG ) ;
            break ;
         }
         iterList++ ;
      }

      if ( iterList == deployModList.end() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is not exsit: %s", OM_BSON_DEPLOY_MOD,
                             _deployMod.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = confTool.readBuzConfig( _businessType, _deployMod, separateConfig,
                                   buzDetail ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to read config file:file=%s,rc=%d",
                             confTool.getBuzConfigPath( _businessType,
                                                        _deployMod,
                                                        separateConfig )
                                                                .c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessCommand::_getClusterInfo( BSONObj& hostsInfoForCluster,
                                                   BSONObj& buzInfoForCluster )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;

      {
         BSONArrayBuilder builder ;
         list<BSONObj> hostList ;
         list<BSONObj>::iterator iter ;

         rc = dbTool.getHostInfoByCluster( _clusterName, hostList ) ;
         if( rc )
         {
            _errorMsg.setError( TRUE, "failed to get host info:rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         if ( hostList.size() == 0 )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }

         for ( iter = hostList.begin(); iter != hostList.end(); ++iter )
         {
            string hostName ;
            BSONObjBuilder hostBuilder ;
            BSONObj hostConfig ;
            BSONObj hostInfo = *iter ;

            hostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
            rc = dbTool.getConfigByHostName( hostName, hostConfig ) ;
            if( rc )
            {
               PD_LOG( PDERROR, "Failed to get host config, rc=%d", rc ) ;
               goto error ;
            }
            hostBuilder.appendElements( hostInfo ) ;
            hostBuilder.appendElements( hostConfig ) ;

            builder.append( hostBuilder.obj() ) ;
         }

         hostsInfoForCluster = BSON( OM_BSON_HOST_INFO << builder.arr() ) ;
      }

      {
         BSONArrayBuilder builder ;
         list<BSONObj> businessList ;
         list<BSONObj>::iterator iter ;

         rc = dbTool.getBusinessInfoOfCluster( _clusterName, businessList ) ;
         if( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business info of cluster: "
                                      "name=%s",
                                _clusterName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         if ( businessList.size() == 0 )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }

         for ( iter = businessList.begin(); iter != businessList.end(); ++iter )
         {
            builder.append( *iter ) ;
         }

         buzInfoForCluster = BSON( OM_BSON_BUSINESS_INFO << builder.arr() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessCommand::_checkExtendConfig(
                                             const BSONObj& confProperties,
                                             const BSONObj& hostsInfoForCluster,
                                             const BSONObj& buzInfoForCluster,
                                             BSONObj& extendConfig )
   {
      INT32 rc = SDB_OK ;
      OmConfigBuilder* confBuilder = NULL ;
      string operationType = OM_FIELD_OPERATION_EXTEND ;
      OmBusinessInfo businessInfo ;
      omDatabaseTool dbTool( _cb ) ;

      businessInfo.clusterName  = _clusterName ;
      businessInfo.businessType = _businessType ;
      businessInfo.businessName = _businessName ;
      businessInfo.deployMode   = _deployMod ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = OmConfigBuilder::createInstance( businessInfo,
                                            operationType,
                                            confBuilder ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, "failed to create config builder: rc=%d", rc ) ;
         goto error ;
      }

      rc = confBuilder->checkConfig( confProperties, hostsInfoForCluster,
                                     buzInfoForCluster, extendConfig ) ;
      if ( SDB_OK != rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, "check config failed: rc=%d, detail: %s",
                 rc, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      if( confBuilder )
      {
         SDB_OSS_DEL( confBuilder ) ;
         confBuilder = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }


   INT32 omExtendBusinessCommand::_createExtendTask(
                                          const BSONObj& extendConfig,
                                          const BSONObj& hostsInfoForCluster,
                                          INT64& taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      BSONArray resultInfo ;
      BSONObj taskConfig ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = _generateTaskInfo( hostsInfoForCluster, extendConfig, taskConfig );
      if( rc )
      {
         PD_LOG( PDERROR, "failed to generate task info:rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateTaskResultInfo( taskConfig, resultInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "failed to generate task result info:rc=%d", rc ) ;
         goto error ;
      }

      getMaxTaskID( taskID ) ;
      taskID++ ;

      rc = taskTool.createTask( OM_TASK_TYPE_EXTEND_BUSINESS, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_EXTEND_BUSINESS ),
                                taskConfig, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "fail to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "fail to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

   }

   /*
   hostsInfoForCluster:
   {
     "HostInfo":[
          {
             "HostName":"host1", "ClusterName":"c1",
             "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
             "Config":[{"BusinessName":"b2","dbpath":"", svcname:"",
                        "role":"", ... }, ...]
          }
           , ...
       ]
   }
   extendConfig:
   {
      "BusinessType":"sequoiadb", "BusinessName":"b1", "deployMod":"xxx",
      "ClusterName":"c1",
      "Config": [
         {
            "HostName": "host1", "datagroupname": "",
            "dbpath": "/home/db2/standalone/11830", "svcname": "11830", ...
         },
         ...
      ]
   }

   */
   INT32 omExtendBusinessCommand::_generateTaskInfo(
                                             const BSONObj& hostsInfoForCluster,
                                             const BSONObj& extendConfig,
                                             BSONObj& taskConfig )
   {
      INT32 rc = SDB_OK ;
      string authUser = "" ;
      string authPwd  = "" ;
      BSONObjBuilder valueBuilder ;

      BSONObj condition = BSON( OM_BSON_FIELD_CONFIG << "" ) ;
      BSONObj configs = extendConfig.filterFieldsUndotted( condition, true ) ;
      BSONObj commons = extendConfig.filterFieldsUndotted( condition, false ) ;

      BSONObj nodes ;
      BSONArrayBuilder arrayBuilder ;

      nodes = configs.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      {
         BSONObjIterator iterBson( nodes ) ;
         while ( iterBson.more() )
         {
            BSONElement ele = iterBson.next() ;
            BSONObj oneNode = ele.embeddedObject() ;

            arrayBuilder.append( oneNode ) ;
         }
      }
      valueBuilder.append( OM_BSON_FIELD_CONFIG, arrayBuilder.arr() ) ;

      if( _businessType == OM_BUSINESS_SEQUOIADB )
      {
         BSONArrayBuilder coordBuilder ;
         BSONObj hostInfos = hostsInfoForCluster.getObjectField(
                                                   OM_BSON_FIELD_HOST_INFO ) ;
         BSONObjIterator hostIter( hostInfos ) ;

         while( hostIter.more() )
         {
            BSONElement hostEle  = hostIter.next() ;
            BSONObj oneHost  = hostEle.embeddedObject() ;
            BSONObj nodeInfos = oneHost.getObjectField( OM_BSON_FIELD_CONFIG ) ;
            BSONObjIterator nodeIter( nodeInfos ) ;
            string hostName = oneHost.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;
            while( nodeIter.more() )
            {
               string businessName ;
               string nodeRole ;
               string svcname ;
               BSONElement nodeEle  = nodeIter.next() ;
               BSONObj oneNode  = nodeEle.embeddedObject() ;

               businessName = oneNode.getStringField( OM_BUSINESS_FIELD_NAME ) ;
               nodeRole = oneNode.getStringField( OM_CONF_DETAIL_ROLE ) ;
               svcname = oneNode.getStringField( OM_CONF_DETAIL_SVCNAME ) ;

               if( businessName == _businessName &&
                   nodeRole == OM_NODE_ROLE_COORD )
               {
                  BSONObj coordInfo = BSON(
                                 OM_CONFIGURE_FIELD_HOSTNAME << hostName <<
                                 OM_CONF_DETAIL_SVCNAME << svcname ) ;

                  coordBuilder.append( coordInfo ) ;
               }
            }
         }
         valueBuilder.append( OM_NODE_COORDLIST, coordBuilder.arr() ) ;
      }

      _getBusinessAuth( _businessName, authUser, authPwd ) ;

      valueBuilder.append( OM_AUTH_FIELD_USER, authUser ) ;
      valueBuilder.append( OM_AUTH_FIELD_PASSWD, authPwd ) ;

      valueBuilder.appendElements( commons ) ;
      taskConfig = valueBuilder.obj() ;

      return rc ;
   }

   INT32 omExtendBusinessCommand::_generateTaskResultInfo(
                                                     const BSONObj &taskConfig,
                                                     BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string businessType ;
      string deployMode ;
      BSONArrayBuilder arrayBuilder ;
      BSONObj condition ;

      businessType = taskConfig.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      deployMode = taskConfig.getStringField( OM_BSON_DEPLOY_MOD ) ;

      if ( businessType == OM_BUSINESS_SEQUOIADB )
      {
         condition = BSON( OM_BSON_FIELD_HOST_NAME << ""
                           << OM_CONF_DETAIL_SVCNAME << ""
                           << OM_CONF_DETAIL_ROLE << ""
                           << OM_CONF_DETAIL_DATAGROUPNAME << "" ) ;
      }
      else if ( businessType == OM_BUSINESS_ZOOKEEPER )
      {
         condition = BSON( OM_BSON_FIELD_HOST_NAME << ""
                           << OM_ZOO_CONF_DETAIL_ZOOID << "" ) ;
      }
      else if ( businessType == OM_BUSINESS_SEQUOIASQL &&
                deployMode == OM_SEQUOIASQL_DEPLOY_OLAP )
      {
         condition = BSON( OM_BSON_FIELD_HOST_NAME << ""
                           << OM_SSQL_OLAP_CONF_ROLE << "" ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, businessType.c_str() ) ;
      }

      BSONObj nodes = taskConfig.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      BSONObjIterator iter( nodes ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj oneNode = ele.embeddedObject() ;

         BSONObj tmp = oneNode.filterFieldsUndotted( condition, true ) ;

         BSONObjBuilder builder ;
         builder.appendElements( tmp ) ;
         builder.append( OM_TASKINFO_FIELD_STATUS, OM_TASK_STATUS_INIT ) ;
         builder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                         getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
         builder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
         builder.append( OM_REST_RES_DETAIL, "" ) ;
         {
            BSONArrayBuilder tmpEmptyBuilder ;
            builder.append( OM_TASKINFO_FIELD_FLOW, tmpEmptyBuilder.arr() ) ;
         }

         BSONObj result = builder.obj() ;
         arrayBuilder.append( result ) ;
      }

      resultInfo = arrayBuilder.arr() ;

      return rc ;
   }


   omShrinkBusinessCommand::omShrinkBusinessCommand(
                                                   restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   const CHAR *pRootPath,
                                                   string& localAgentHost,
                                                   string& localAgentService )
                                 : omAuthCommand( pRestAdaptor, pRestSession ),
                                   _rootPath( pRootPath ),
                                   _localAgentHost( localAgentHost ),
                                   _localAgentService( localAgentService )
   {
   }

   omShrinkBusinessCommand::~omShrinkBusinessCommand()
   {
   }

   INT32 omShrinkBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      BSONObj shrinkConfig ;
      BSONObj hostInfoInBusiness ;
      vector<simpleAddressInfo> addressList ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo( shrinkConfig ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkBusiness() ;
      if( rc )
      {
         PD_LOG( PDERROR, "_checkBusiness failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkConfig( shrinkConfig ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_checkConfig failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = dbTool.getBusinessAddressWithConfig( _businessName, addressList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getBusinessInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _createTask( addressList, shrinkConfig, taskID ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_createTask failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << (long long)taskID ) ;
         _restAdaptor->appendHttpBody( _restSession, result.objdata(),
                                       result.objsize(), 1 ) ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_getRestInfo( BSONObj &shrinkConfig )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pInfo = NULL ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CONFIGINFO, &pInfo ) ;
      if( NULL == pInfo )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "rest field is null:field=%s",
                             OM_REST_FIELD_CONFIGINFO ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = fromjson( pInfo, shrinkConfig ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "invalid json: %s=%s",
                             OM_REST_FIELD_CONFIGINFO, pInfo ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _clusterName = shrinkConfig.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if( _clusterName == "" )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_CLUSTER_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessName = shrinkConfig.getStringField( OM_BSON_BUSINESS_NAME ) ;
      if( _businessName == "" )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_checkBusiness()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj buzInfo ;
      string clusterName ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getOneBusinessInfo( _businessName, buzInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: %s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get exist business:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = buzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      _deployMod    = buzInfo.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;
      clusterName   = buzInfo.getStringField( OM_BUSINESS_FIELD_CLUSTERNAME ) ;

      if( _clusterName != clusterName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "the cluster %s has no business %s",
                             _clusterName.c_str(), _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _businessName ) ;
      if( taskID >= 0 )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task["OSS_LL_PRINT_FORMAT"]",
                             _businessName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_checkSdbConfig( const BSONObj &shrinkConfig )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;
      BSONObj nodeConfig ;
      set<string> nodeList ;

      if( _deployMod != OM_DEPLOY_MOD_DISTRIBUTION )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported deployment mode:type=%s",
                             _deployMod.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      nodeConfig = shrinkConfig.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      {
         BSONObjIterator iter( nodeConfig ) ;
         while ( iter.more() )
         {
            BOOLEAN nodeIsExist = FALSE ;
            vector<string> portList ;
            vector<string>::iterator portListIter ;
            BSONElement ele = iter.next() ;
            BSONObj nodeInfo = ele.embeddedObject() ;
            string hostName = nodeInfo.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;
            string svcname = nodeInfo.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ;

            if ( hostName.empty() )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "the %s can not be empty,rc=%d",
                                   OM_CONFIGURE_FIELD_HOSTNAME, rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            if ( svcname.empty() )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "the %s can not be empty,rc=%d",
                                   OM_CONFIGURE_FIELD_SVCNAME, rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            if ( nodeList.find( hostName + ":" + svcname ) != nodeList.end() )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "node repeats, host=%s,port=%s,rc=%d",
                                   hostName.c_str(), svcname.c_str(), rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            nodeList.insert( hostName + ":" + svcname ) ;

            rc = dbTool.getHostUsedPort( hostName, portList ) ;
            if ( rc )
            {
               _errorMsg.setError( TRUE, "failed to get host used port,host=%s"
                                         ",rc=%d",
                                   hostName.c_str(), rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            for ( portListIter = portList.begin();
                        portListIter != portList.end(); ++portListIter )
            {
               string usedPort = *portListIter ;

               if ( usedPort == svcname  )
               {
                  nodeIsExist = TRUE ;
                  break ;
               }
            }

            if ( nodeIsExist == FALSE )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "the node does not exist,business=%s,"
                                         "host=%s,port=%s,rc=%d",
                                   _businessName.c_str(), hostName.c_str(),
                                   svcname.c_str(), rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;

            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_checkConfig( const BSONObj &shrinkConfig )
   {
      INT32 rc = SDB_OK ;

      if( _businessType == OM_BUSINESS_SEQUOIADB )
      {
         rc = _checkSdbConfig( shrinkConfig ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "_checkSdbConfig failed: rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported business type:type=%s",
                             _businessType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_generateTaskInfo(
                                       vector<simpleAddressInfo> &addressList,
                                       const BSONObj &shrinkConfig,
                                       BSONObj &taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 coordNum = 0 ;
      UINT32 removeCatalogNum = 0 ;
      omDatabaseTool dbTool( _cb ) ;
      string authUser = "" ;
      string authPwd  = "" ;
      BSONObjBuilder valueBuilder ;
      BSONObj condition = BSON( OM_BSON_FIELD_CONFIG << "" ) ;
      BSONObj configs = shrinkConfig.filterFieldsUndotted( condition, true ) ;
      BSONObj commons = shrinkConfig.filterFieldsUndotted( condition, false ) ;
      BSONObj nodes ;
      BSONArrayBuilder arrayBuilder ;
      BSONArrayBuilder coordBuilder ;
      set<string> removeCoordList ;
      vector<simpleAddressInfo> catalogList ;
      vector<simpleAddressInfo>::iterator addressListIter ;

      rc = dbTool.getCatalogAddressWithConfig( _businessName, catalogList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get catalog address:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      nodes = configs.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      {
         BSONObjIterator iterBson( nodes ) ;
         while ( iterBson.more() )
         {
            BSONObj newConfig ;
            BSONElement ele = iterBson.next() ;
            BSONObj oneNode = ele.embeddedObject() ;
            string hostName = oneNode.getStringField( OM_HOST_FIELD_NAME ) ;
            string svcname = oneNode.getStringField( OM_CONF_DETAIL_SVCNAME ) ;

            rc = dbTool.getOneNodeConfig( _businessName, hostName, svcname,
                                          newConfig ) ;
            if ( rc )
            {
               _errorMsg.setError( TRUE, "failed to get node config:rc=%d",
                                   rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               string role = newConfig.getStringField( OM_CONF_DETAIL_ROLE ) ;
               string group = newConfig.getStringField(
                                                OM_CONF_DETAIL_DATAGROUPNAME ) ;

               arrayBuilder.append( BSON( OM_HOST_FIELD_NAME << hostName <<
                                          OM_CONF_DETAIL_SVCNAME << svcname <<
                                          OM_CONF_DETAIL_ROLE << role <<
                                          OM_CONF_DETAIL_DATAGROUPNAME <<
                                          group ) ) ;

               if ( OM_NODE_ROLE_COORD == role )
               {
                  removeCoordList.insert( hostName + ":" + svcname ) ;
               }
               else if ( OM_NODE_ROLE_CATALOG == role )
               {
                  ++removeCatalogNum ;
               }
            }
         }
      }
      valueBuilder.append( OM_BSON_FIELD_CONFIG, arrayBuilder.arr() ) ;

      for ( addressListIter = addressList.begin();
                     addressListIter != addressList.end(); ++addressListIter )
      {
         simpleAddressInfo addressInfo = *addressListIter ;

         if ( removeCoordList.find(
                    addressInfo.hostName + ":" + addressInfo.port )
                          == removeCoordList.end() )
         {
            BSONObj coordInfo = BSON(
                        OM_CONFIGURE_FIELD_HOSTNAME << addressInfo.hostName <<
                        OM_CONF_DETAIL_SVCNAME << addressInfo.port ) ;

            coordBuilder.append( coordInfo ) ;
            ++coordNum ;
         }
      }

      if ( 0 == coordNum )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "do not remove all coord nodes: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( removeCatalogNum >= catalogList.size() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "do not remove all catalog nodes: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      valueBuilder.append( OM_NODE_COORDLIST, coordBuilder.arr() ) ;

      _getBusinessAuth( _businessName, authUser, authPwd ) ;

      valueBuilder.append( OM_AUTH_FIELD_USER, authUser ) ;
      valueBuilder.append( OM_AUTH_FIELD_PASSWD, authPwd ) ;

      valueBuilder.appendElements( commons ) ;
      valueBuilder.append( OM_CONFIGURE_FIELD_BUSINESSTYPE, _businessType ) ;
      valueBuilder.append( OM_CONFIGURE_FIELD_DEPLOYMODE, _deployMod ) ;
      taskInfo = valueBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessCommand::_generateTaskResultInfo(
                                                   const BSONObj &taskConfig,
                                                   BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrayBuilder ;
      BSONObj condition ;

      if ( _businessType == OM_BUSINESS_SEQUOIADB )
      {
         condition = BSON( OM_BSON_FIELD_HOST_NAME << "" <<
                           OM_CONF_DETAIL_SVCNAME << "" <<
                           OM_CONF_DETAIL_ROLE << ""<<
                           OM_CONF_DETAIL_DATAGROUPNAME << "" ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, _businessType.c_str() ) ;
      }

      BSONObj nodes = taskConfig.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      BSONObjIterator iter( nodes ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj oneNode = ele.embeddedObject() ;
         BSONObj tmp = oneNode.filterFieldsUndotted( condition, true ) ;
         BSONObjBuilder builder ;

         builder.appendElements( tmp ) ;
         builder.append( OM_TASKINFO_FIELD_STATUS, OM_TASK_STATUS_INIT ) ;
         builder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                         getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
         builder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
         builder.append( OM_REST_RES_DETAIL, "" ) ;

         {
            BSONArrayBuilder tmpEmptyBuilder ;

            builder.append( OM_TASKINFO_FIELD_FLOW, tmpEmptyBuilder.arr() ) ;
         }

         BSONObj result = builder.obj() ;
         arrayBuilder.append( result ) ;
      }

      resultInfo = arrayBuilder.arr() ;

      return rc ;
   }


   INT32 omShrinkBusinessCommand::_createTask(
                                       vector<simpleAddressInfo> &addressList,
                                       const BSONObj &shrinkConfig,
                                       INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      BSONObj taskInfo ;
      BSONArray resultInfo ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = _generateTaskInfo( addressList, shrinkConfig, taskInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_generateTaskInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateTaskResultInfo( taskInfo, resultInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "_generateTaskInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      getMaxTaskID( taskID ) ;
      ++taskID ;

      rc = taskTool.createTask( OM_TASK_TYPE_SHRINK_BUSINESS, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_SHRINK_BUSINESS ),
                                taskInfo, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "fail to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "fail to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omLogoutCommand::omLogoutCommand( restAdaptor *pRestAdaptor,
                                     pmdRestSession *pRestSession )
                   :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omLogoutCommand::~omLogoutCommand()
   {
   }

   INT32 omLogoutCommand::doCommand()
   {
      _restSession->doLogout() ;
      _sendOKRes2Web() ;
      return SDB_OK ;
   }

   omChangePasswdCommand::omChangePasswdCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                         :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omChangePasswdCommand::~omChangePasswdCommand()
   {
   }

   INT32 omChangePasswdCommand::_getRestDetail( string &user, string &oldPasswd,
                                                string &newPasswd, string &time )
   {
      INT32 rc               = SDB_OK ;
      const CHAR *pUser      = NULL ;
      const CHAR *pOldPasswd = NULL ;
      const CHAR *pNewPasswd = NULL ;
      const CHAR *pTime      = NULL ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_NAME, &pUser ) ;
      if ( NULL == pUser )
      {
         rc           = SDB_INVALIDARG ;
         _errorDetail = string( OM_REST_FIELD_LOGIN_NAME ) + " is null" ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_LOGIN_PASSWD,
                              &pOldPasswd ) ;
      if ( NULL == pOldPasswd )
      {
         rc           = SDB_INVALIDARG ;
         _errorDetail = string( OM_REST_FIELD_LOGIN_PASSWD ) + " is null" ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_NEW_PASSWD,
                              &pNewPasswd ) ;
      if ( NULL == pNewPasswd )
      {
         rc           = SDB_INVALIDARG ;
         _errorDetail = string( OM_REST_FIELD_NEW_PASSWD ) + " is null" ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TIMESTAMP, &pTime ) ;
      if ( NULL == pTime )
      {
         rc           = SDB_INVALIDARG ;
         _errorDetail = string( OM_REST_FIELD_TIMESTAMP ) + " is null" ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      user      = pUser ;
      newPasswd = pNewPasswd ;
      oldPasswd = pOldPasswd ;
      time      = pTime ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omChangePasswdCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string user ;
      string oldPasswd ;
      string newPasswd ;
      string time ;

      BSONObj bsonAuth ;

      rc = _getRestDetail( user, oldPasswd, newPasswd, time ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get rest info failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( user != _restSession->getLoginUserName() )
      {
         rc           = SDB_INVALIDARG ;
         _errorDetail = string( "can't not change other usr's passwd:" )
                        + "myusr=" + _restSession->getLoginUserName()
                        + ",usr=" + user ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      bsonAuth = BSON( SDB_AUTH_USER << user
                       << SDB_AUTH_PASSWD << oldPasswd ) ;
      rc = sdbGetOMManager()->authenticate( bsonAuth, _cb ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
         {
            PD_LOG( PDERROR, "username or passwd is wrong:rc=%d", rc ) ;
            _sendErrorRes2Web( rc, "username or passwd is wrong" ) ;
         }
         else
         {
            PD_LOG( PDERROR, "system error:rc=%d", rc ) ;
            _sendErrorRes2Web( rc, "system error" ) ;
         }

         goto error ;
      }

      rc = sdbGetOMManager()->authUpdatePasswd( user, oldPasswd, newPasswd,
                                                _cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "change passwd failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, "change passwd failed" ) ;
         goto error ;
      }

      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omCheckSessionCommand::omCheckSessionCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                         :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omCheckSessionCommand::~omCheckSessionCommand()
   {
   }

   INT32 omCheckSessionCommand::doCommand()
   {
      BSONObjBuilder bsonBuilder ;
      const CHAR* sessionID = NULL ;
      _restAdaptor->getHttpHeader( _restSession, OM_REST_HEAD_SESSIONID,
                                   &sessionID ) ;
      if ( NULL != sessionID && _restSession->isAuthOK() )
      {
         _sendOKRes2Web() ;
      }
      else
      {
         _sendErrorRes2Web( SDB_AUTH_AUTHORITY_FORBIDDEN, "" ) ;
      }

      return SDB_OK ;
   }

   omCreateClusterCommand::omCreateClusterCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                                 : omAuthCommand( pRestAdaptor,
                                                  pRestSession )
   {
   }

   omCreateClusterCommand::~omCreateClusterCommand()
   {
   }

   INT32 omCreateClusterCommand::_getRestInfo( string &clusterName,
                                               BSONObj &clusterInfo )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pClusterInfo = NULL ;
      string sdbUsr ;
      string sdbPasswd ;
      string sdbUsrGroup ;
      BSONElement grantConfEle ;

      _restAdaptor->getQuery(_restSession, OM_REST_CLUSTER_INFO,
                             &pClusterInfo ) ;
      if ( NULL == pClusterInfo )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is null,rc=%d",
                             OM_REST_CLUSTER_INFO, rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = fromjson( pClusterInfo, clusterInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "change rest field to BSONObj failed:"
                                   "src=%s,rc=%d",
                             pClusterInfo, rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      clusterName = clusterInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( clusterName.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is null",
                             OM_BSON_CLUSTER_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      sdbUsr = clusterInfo.getStringField( OM_BSON_FIELD_SDB_USER ) ;
      if ( sdbUsr.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is null",
                             OM_BSON_FIELD_SDB_USER ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      sdbPasswd = clusterInfo.getStringField( OM_BSON_FIELD_SDB_PASSWD ) ;
      if ( sdbPasswd.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is null",
                             OM_BSON_FIELD_SDB_PASSWD ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      sdbUsrGroup = clusterInfo.getStringField( OM_BSON_FIELD_SDB_USERGROUP ) ;
      if ( sdbUsrGroup.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is null",
                             OM_BSON_FIELD_SDB_USERGROUP ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      grantConfEle = clusterInfo.getField( OM_BSON_FIELD_GRANTCONF ) ;
      if ( FALSE == grantConfEle.eoo() )
      {
         if ( bson::Array != grantConfEle.type() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid argument:%s is not an array",
                                OM_BSON_FIELD_GRANTCONF ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         BSONObjIterator iter( grantConfEle.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj grantInfo = ele.embeddedObject() ;
            string tmpName = grantInfo.getStringField(
                                                OM_CLUSTER_FIELD_GRANTNAME ) ;
            BOOLEAN tmpPrivilege = grantInfo.getBoolField(
                                                OM_CLUSTER_FIELD_PRIVILEGE ) ;

            if ( OM_CLUSTER_FIELD_HOSTFILE == tmpName )
            {
            }
            else if ( OM_CLUSTER_FIELD_ROOTUSER == tmpName )
            {
               if ( FALSE == tmpPrivilege )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "%s can not be false",
                                      OM_CLUSTER_FIELD_ROOTUSER ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "invalid argument:%s=%s",
                                   OM_CLUSTER_FIELD_GRANTNAME,
                                   tmpName.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;

            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCreateClusterCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string clusterName ;
      BSONObj clusterInfo ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo( clusterName, clusterInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      /*
         duplicate check depends on the unique index of
         table (OM_CS_DEPLOY_CL_CLUSTERIDX1)
      */
      rc = dbTool.addCluster( clusterInfo ) ;
      if ( rc )
      {
         if ( SDB_IXM_DUP_KEY == rc )
         {
            _errorMsg.setError( TRUE, "%s is already exist:rc=%d",
                                clusterName.c_str(), rc ) ;
         }
         else
         {
            _errorMsg.setError( TRUE, "failed to insert cluster:name=%s,rc=%d",
                                clusterName.c_str(), rc ) ;
         }
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;

   }

   omQueryClusterCommand::omQueryClusterCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                         : omCheckSessionCommand( pRestAdaptor, pRestSession )
   {
   }

   omQueryClusterCommand::~omQueryClusterCommand()
   {
   }

   INT32 omQueryClusterCommand::doCommand()
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      INT32 rc         = SDB_OK ;
      list<BSONObj> records ;
      list<BSONObj>::iterator iter ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order,
                        hint, 0, skip, returnNum, records ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "query table failed:rc=%d", rc ) ;
      }

      iter = records.begin() ;
      while ( iter != records.end() )
      {
         rc = _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                            iter->objsize(), 1 ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "falied to append http body:rc=%d", rc ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _sendErrorRes2Web( rc, _errorDetail ) ;
            goto error ;
         }

         iter++ ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omUpdateHostInfoCommand::omUpdateHostInfoCommand( restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                          : omCheckSessionCommand( pRestAdaptor, pRestSession )
   {
   }

   omUpdateHostInfoCommand::~omUpdateHostInfoCommand()
   {
   }

   INT32 omUpdateHostInfoCommand::_getUpdateInfo(
                                               map< string, string > &mapHost )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pHostInfo = NULL ;
      BSONObj hostInfo ;
      BSONElement hostsEle ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_HOST_INFO,
                              &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field is null:field=%s",
                     OM_REST_FIELD_HOST_INFO ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, hostInfo ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pHostInfo, rc ) ;
         goto error ;
      }

      hostsEle = hostInfo.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if (  Array != hostsEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "%s is not array type:type=%d",
                     OM_BSON_FIELD_HOST_INFO, hostsEle.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( hostsEle.embeddedObject() ) ;
         string hostName ;
         string ip ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "host info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               goto error ;
            }

            BSONObj oneHost = ele.embeddedObject() ;
            hostName = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
            ip       = oneHost.getStringField( OM_BSON_FIELD_HOST_IP ) ;
            mapHost[ hostName ] = ip ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUpdateHostInfoCommand::_getClusterName( const string &hostName,
                                                   string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher = BSON( OM_HOST_FIELD_NAME << hostName ) ;
      BSONObj order ;
      BSONObj hint ;
      list< BSONObj > hosts ;

      rc = _queryTable( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                        0, 0, 1, hosts ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get clusterName failed:hostName=%s,rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

      if ( hosts.size() != 1 )
      {
         rc = SDB_DMS_EOC ;
         PD_LOG( PDERROR, "get clusterName failed:hostName=%s,rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

      clusterName = hosts.begin()->getStringField( OM_HOST_FIELD_CLUSTERNAME ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUpdateHostInfoCommand::_updateHostIP( map< string, string > &mapHost )
   {
      INT32 rc = SDB_OK ;
      map< string, string >::iterator iter ;

      iter = mapHost.begin() ;
      while ( iter != mapHost.end() )
      {
         string hostName  = iter->first ;
         string IP        = iter->second ;
         BSONObj selector = BSON( OM_HOST_FIELD_NAME << hostName ) ;
         BSONObj updator  = BSON( "$set" << BSON( OM_HOST_FIELD_IP << IP ) ) ;
         BSONObj hint ;
         INT64 updateNum  = 0 ;

         string clusterName ;
         rc = _getClusterName( hostName, clusterName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "get clusterName failed:hostName=%s,rc=%d",
                        hostName.c_str(), rc ) ;
            goto error ;
         }

         rc = rtnUpdate( OM_CS_DEPLOY_CL_HOST, selector, updator, hint, 0,
                         pmdGetThreadEDUCB(), &updateNum ) ;
         if ( SDB_OK != rc || 0 == updateNum )
         {
            PD_LOG_MSG( PDERROR, "update host failed:table=%s,updateNum=%d,"
                        "host=%s,updator=%s,selector=%s,rc=%d",
                        OM_CS_DEPLOY_CL_HOST, updateNum, hostName.c_str(),
                        updator.toString().c_str(), selector.toString().c_str(),
                        rc ) ;
            goto error ;
         }

         PD_LOG( PDEVENT, "update host(%s,%s) success", hostName.c_str(),
                 IP.c_str() ) ;
         sdbGetOMManager()->updateClusterVersion( clusterName ) ;

         iter++ ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUpdateHostInfoCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      map< string, string > mapHost ;
      rc = _getUpdateInfo( mapHost ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getUpdateInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _updateHostIP( mapHost ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_updateHostIP failed:rc=%d", rc ) ;
         goto error ;
      }

      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      _sendErrorRes2Web( rc, _errorDetail ) ;
      goto done ;
   }


   omScanHostCommand::omScanHostCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession,
                                         const string &localAgentHost,
                                         const string &localAgentService )
                     : omCheckSessionCommand( pRestAdaptor, pRestSession ),
                       _localAgentHost( localAgentHost ),
                       _localAgentService( localAgentService )
   {
   }

   omScanHostCommand::~omScanHostCommand()
   {
   }

   void omScanHostCommand::_generateHostList(
                                             list<omScanHostInfo> &hostInfoList,
                                             BSONObj &bsonRequest )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;

      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         omScanHostInfo *pInfo = &(*iter) ;
         BSONObjBuilder builder ;
         if ( pInfo->ip != "" )
         {
            builder.append( OM_BSON_FIELD_HOST_IP, pInfo->ip ) ;
         }

         if ( pInfo->hostName != "" )
         {
            builder.append( OM_BSON_FIELD_HOST_NAME, pInfo->hostName ) ;
         }

         builder.append( OM_BSON_FIELD_HOST_USER, pInfo->user ) ;
         builder.append( OM_BSON_FIELD_HOST_PASSWD, pInfo->passwd ) ;
         builder.append( OM_BSON_FIELD_HOST_SSHPORT, pInfo->sshPort ) ;
         BSONObj oneHost = builder.obj() ;
         arrayBuilder.append( oneHost.copy() ) ;
         iter++ ;
      }

      builder.appendArray( OM_REST_FIELD_HOST_INFO, arrayBuilder.arr() );
      bsonRequest = builder.obj() ;

      return ;
   }

   void omScanHostCommand::_generateArray( list<BSONObj> &hostInfoList,
                                           const string &arrayKeyName,
                                           BSONObj &result )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;

      list<BSONObj>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         arrayBuilder.append( *iter ) ;
         iter++ ;
      }

      builder.appendArray( arrayKeyName.c_str(), arrayBuilder.arr() );
      result = builder.obj() ;

      return ;
   }

   void omScanHostCommand::_sendResult2Web( list<BSONObj> &hostResult )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bsonBuilder ;

      list<BSONObj>::iterator iter = hostResult.begin() ;
      while ( iter != hostResult.end() )
      {
         rc = _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         if ( rc )
         {
            PD_LOG(PDERROR, "Failed to appendHttpBody:rc=%d", rc ) ;
            _sendErrorRes2Web( rc, "Failed to appendHttpBody" ) ;
            goto error ;
         }

         iter++ ;
      }

      _sendOKRes2Web() ;

   done:
      return ;
   error:
      goto done ;
   }

   void omScanHostCommand::_filterExistHost( list<omScanHostInfo> &hostInfoList,
                                             list<BSONObj> &hostResult )
   {
      set<string> hostNameSet ;
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( _isHostExist( *iter ) )
         {
            BSONObj tmp = BSON( OM_REST_RES_RETCODE << SDB_IXM_DUP_KEY
                                << OM_REST_RES_DETAIL << "host is exist"
                                << OM_BSON_FIELD_HOST_IP << iter->ip
                                << OM_BSON_FIELD_SCAN_STATUS
                                << OM_SCAN_HOST_STATUS_FINISH
                                << OM_BSON_FIELD_HOST_NAME << iter->hostName ) ;

            hostResult.push_back( tmp ) ;
            hostInfoList.erase( iter++ ) ;
            continue ;
         }
         else
         {
            if ( hostNameSet.find( iter->hostName ) != hostNameSet.end()
                 || hostNameSet.find( iter->ip ) != hostNameSet.end() )
            {
               PD_LOG( PDWARNING, "host is duplicate in request, ignored:"
                       "host=%s,ip=%s", iter->hostName.c_str(),
                       iter->ip.c_str() ) ;
               hostInfoList.erase( iter++ ) ;
               continue ;
            }
            else
            {
               if ( !iter->hostName.empty() )
               {
                  hostNameSet.insert( iter->hostName ) ;
               }
               else if ( !iter->ip.empty() )
               {
                  hostNameSet.insert( iter->ip ) ;
               }
            }
         }

         iter++ ;
      }
   }

   bool omScanHostCommand::_isHostIPExist( const string &ip )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID   = -1 ;
      INT32 rc           = SDB_OK ;
      rtnContextBuf buffObj ;

      matcher = BSON( OM_HOST_FIELD_IP << ip ) ; ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 0,
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query host:rc=%d,host=%s", rc,
                 matcher.toString().c_str() ) ;
         return false ;
      }

      rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
         }

         return false ;
      }

      _pRTNCB->contextDelete( contextID, _cb ) ;

      return true ;
   }

   bool omScanHostCommand::_isHostNameExist( const string &hostName )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID   = -1 ;
      INT32 rc           = SDB_OK ;
      rtnContextBuf buffObj ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName ) ; ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 0,
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query host:rc=%d,host=%s", rc,
                 matcher.toString().c_str() ) ;
         return false ;
      }

      rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
         }

         return false ;
      }

      _pRTNCB->contextDelete( contextID, _cb ) ;

      return true ;
   }

   bool omScanHostCommand::_isHostExist( const omScanHostInfo &host )
   {
      if ( host.hostName != "" )
      {
         return _isHostNameExist( host.hostName ) ;
      }

      if ( host.ip != "" )
      {
         return _isHostIPExist( host.ip );
      }

      return false ;
   }

   INT32 omScanHostCommand::_checkRestHostInfo( BSONObj &hostInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = hostInfo.getField( OM_BSON_FIELD_HOST_USER ) ;
      if ( ele.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostinfo field is not string:field=%s,type=%d",
                     OM_BSON_FIELD_HOST_USER, String ) ;
         goto error ;
      }

      ele = hostInfo.getField( OM_BSON_FIELD_HOST_PASSWD ) ;
      if ( ele.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostinfo field is not string:field=%s,type=%d",
                     OM_BSON_FIELD_HOST_PASSWD, String ) ;
         goto error ;
      }

      ele = hostInfo.getField( OM_BSON_FIELD_HOST_SSHPORT ) ;
      if ( ele.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostinfo field is not string:field=%s,type=%d",
                     OM_BSON_FIELD_HOST_SSHPORT, String ) ;
         goto error ;
      }

      ele = hostInfo.getField( OM_BSON_CLUSTER_NAME ) ;
      if ( ele.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostinfo field is not string:field=%s,type=%d",
                     OM_BSON_CLUSTER_NAME, String ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omScanHostCommand::_getScanHostList( string &clusterName,
                                              list<omScanHostInfo> &hostInfo )
   {
      INT32 rc                     = SDB_OK ;
      const CHAR* pGlobalUser      = NULL ;
      const CHAR* pGlobalPasswd    = NULL ;
      const CHAR* pGlobalSshPort   = NULL ;
      const CHAR* pGlobalAgentPort = NULL ;
      const CHAR* pHostInfo        = NULL ;
      BSONObj bsonHostInfo ;
      BSONElement element ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_HOST_INFO,
                              &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field is null:field=%s",
                     OM_REST_FIELD_HOST_INFO ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, bsonHostInfo ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pHostInfo, rc ) ;
         goto error ;
      }

      rc = _checkRestHostInfo( bsonHostInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "check rest hostinfo failed:rc=%d", rc ) ;
         goto error ;
      }

      pGlobalUser    = bsonHostInfo.getStringField( OM_BSON_FIELD_HOST_USER ) ;
      pGlobalPasswd  = bsonHostInfo.getStringField(
                                                OM_BSON_FIELD_HOST_PASSWD ) ;
      pGlobalSshPort = bsonHostInfo.getStringField(
                                                OM_BSON_FIELD_HOST_SSHPORT ) ;
      pGlobalAgentPort = _localAgentService.c_str() ;
      clusterName      = bsonHostInfo.getStringField(
                                                OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == ossStrlen( pGlobalUser ) || 0 == ossStrlen( pGlobalPasswd )
           || 0 == ossStrlen( pGlobalSshPort ) || 0 == clusterName.length()
           || 0 == ossStrlen( pGlobalAgentPort ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field have not been set:field="
                     "%s or %s or %s or %s or %s", OM_BSON_FIELD_HOST_USER,
                     OM_BSON_FIELD_HOST_PASSWD, OM_BSON_FIELD_HOST_SSHPORT,
                     OM_BSON_CLUSTER_NAME ) ;
         goto error ;
      }

      element = bsonHostInfo.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if ( element.isNull() || Array != element.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "%s is not array type:type=%d",
                     OM_BSON_FIELD_HOST_INFO, element.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( element.embeddedObject() ) ;
         while ( i.more() )
         {
            omScanHostInfo host ;
            BSONObjBuilder builder ;
            BSONObj tmp ;
            BSONElement ele = i.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "host info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               goto error ;
            }
            BSONObj oneHost = ele.embeddedObject() ;

            host.ip        = oneHost.getStringField( OM_BSON_FIELD_HOST_IP ) ;
            host.hostName  = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
            host.user      = pGlobalUser ;
            host.passwd    = pGlobalPasswd ;
            host.sshPort   = pGlobalSshPort ;
            host.agentPort = pGlobalAgentPort ;

            if ( oneHost.hasField( OM_BSON_FIELD_HOST_USER ) )
            {
               host.user = oneHost.getStringField( OM_BSON_FIELD_HOST_USER ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_HOST_PASSWD ) )
            {
               host.passwd = oneHost.getStringField(
                                                  OM_BSON_FIELD_HOST_PASSWD ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_HOST_SSHPORT ) )
            {
               host.sshPort = oneHost.getStringField(
                                                  OM_BSON_FIELD_HOST_SSHPORT ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_AGENT_PORT ) )
            {
               host.agentPort = oneHost.getStringField(
                                                  OM_BSON_FIELD_AGENT_PORT ) ;
            }

            if ( host.ip=="" && host.hostName=="" )
            {
               _errorDetail = string( "rest field miss:" )
                              + OM_BSON_FIELD_HOST_IP + " or "
                              + OM_BSON_FIELD_HOST_NAME ;
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "rest field miss:field=%s or %s",
                           OM_BSON_FIELD_HOST_IP, OM_BSON_FIELD_HOST_NAME ) ;
               goto error ;
            }

            hostInfo.push_back( host ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omScanHostCommand::_clearSession( omManager *om,
                                          pmdRemoteSession *remoteSession )
   {
      if ( NULL != remoteSession )
      {
         pmdSubSession *pSubSession = NULL ;
         pmdSubSessionItr itr       = remoteSession->getSubSessionItr() ;
         while ( itr.more() )
         {
            pSubSession = itr.next() ;
            MsgHeader *pMsg = pSubSession->getReqMsg() ;
            if ( NULL != pMsg )
            {
               SDB_OSS_FREE( pMsg ) ;
            }
         }

         remoteSession->clearSubSession() ;
         om->getRSManager()->removeSession( remoteSession ) ;
      }
   }

   INT32 omScanHostCommand::_notifyAgentTask( INT64 taskID )
   {
      INT32 rc          = SDB_OK ;
      SINT32 flag       = SDB_OK ;
      CHAR *pContent    = NULL ;
      INT32 contentSize = 0 ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      BSONObj result ;

      bsonRequest = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_NOTIFY_TASK,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build msg failed:cmd=%s,rc=%d", OM_NOTIFY_TASK,
                     rc ) ;
         goto error ;
      }

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_NOTIFY_TASK_TIMEOUT,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "create remote session failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      rc = _receiveFromAgent( remoteSession, flag, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         rc = flag ;
         string tmpError = result.getStringField( OM_REST_RES_DETAIL ) ;
         PD_LOG_MSG( PDERROR, "agent process failed:detail=(%s),rc=%d",
                     tmpError.c_str(), rc ) ;
         goto error ;
      }
   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omScanHostCommand::_sendMsgToLocalAgent( omManager *om,
                                             pmdRemoteSession *remoteSession,
                                             MsgHeader *pMsg,
                                             BOOLEAN isUseLocalHost )
   {
      MsgRouteID localAgentID ;
      INT32 rc = SDB_OK ;

      if ( isUseLocalHost )
      {
         localAgentID = om->updateAgentInfo( OM_DEFAULT_LOCAL_HOST,
                                             _localAgentService ) ;
      }
      else
      {
         localAgentID = om->updateAgentInfo( _localAgentHost,
                                             _localAgentService ) ;
      }

      if ( NULL == remoteSession->addSubSession( localAgentID.value ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "addSubSession failed:id=%ld", localAgentID.value ) ;
         goto error ;
      }

      rc = remoteSession->sendMsg( pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send msg to localhost's agent failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omScanHostCommand::_markHostExistence( BSONObj &oneHost )
   {
      INT32 retcode = oneHost.getIntField( OM_REST_RES_RETCODE ) ;
      if ( retcode == SDB_OK )
      {
         string hostName = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
         if ( _isHostNameExist( hostName ) )
         {
            BSONObjBuilder builder ;
            BSONObj filter = BSON( OM_REST_RES_RETCODE << ""
                                   << OM_REST_RES_DETAIL << "" ) ;
            BSONObj others = oneHost.filterFieldsUndotted( filter, false ) ;
            builder.append( OM_REST_RES_RETCODE, SDB_IXM_DUP_KEY ) ;
            builder.append( OM_REST_RES_DETAIL, "host is exist" ) ;
            builder.appendElements( others ) ;

            oneHost = builder.obj() ;
         }
      }
   }

   INT32 omScanHostCommand::_parseResonpse( VEC_SUB_SESSIONPTR &subSessionVec,
                                            BSONObj &response,
                                            list<BSONObj> &bsonResult )
   {
      INT32 rc           = SDB_OK ;
      BSONElement hostElement ;

      hostElement = response.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if ( hostElement.isNull() || Array != hostElement.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "agent's response is unrecognized, %s is not "
                     "array type:type=%d", OM_BSON_FIELD_HOST_INFO,
                     hostElement.type()) ;
         goto error ;
      }
      {
         BSONObjIterator i( hostElement.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONObjBuilder builder ;
            BSONObj tmp ;
            BSONElement ele = i.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "rest info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               goto error ;
            }

            BSONObj oneHost = ele.embeddedObject() ;
            _markHostExistence( oneHost ) ;
            bsonResult.push_back( oneHost ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omScanHostCommand::doCommand()
   {
      string clusterName ;
      list<omScanHostInfo> hostInfoList ;
      INT32 rc                        = SDB_OK ;
      CHAR* pContent                  = NULL ;
      INT32 contentSize               = 0 ;
      SINT32 flag                     = SDB_OK ;
      MsgHeader *pMsg                 = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      BSONObj bsonResponse ;
      list<BSONObj> bsonResult ;
      pmdEDUEvent eventData ;
      omManager *om                   = NULL ;
      VEC_SUB_SESSIONPTR subSessionVec ;

      rc = _getScanHostList( clusterName, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get host list failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _generateHostList( hostInfoList, bsonRequest ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_SCAN_HOST_REQ,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = string( "build message failed:cmd=" )
                        + OM_SCAN_HOST_REQ ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      om   = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_WAIT_SCAN_RES_INTERVAL,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         SDB_OSS_FREE( pContent ) ;

         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "create remove session failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg, TRUE ) ;
      if ( SDB_OK != rc )
      {
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;

         PD_LOG_MSG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _receiveFromAgent( remoteSession, flag, bsonResponse ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         rc = flag ;
         _errorDetail = bsonResponse.getStringField( OM_REST_RES_DETAIL ) ;
         PD_LOG( PDERROR, "agent process failed:detail=%s,rc=%d",
                 _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _parseResonpse( subSessionVec, bsonResponse, bsonResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_parseResonpse failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendResult2Web( bsonResult ) ;

   done:
      _clearSession( om, remoteSession ) ;
      return rc;
   error:
      goto done ;
   }

   omCheckHostCommand::omCheckHostCommand( restAdaptor *pRestAdaptor,
                                           pmdRestSession *pRestSession,
                                           const string &localAgentHost,
                                           const string &localAgentService )
                      : omScanHostCommand( pRestAdaptor, pRestSession,
                                           localAgentHost, localAgentService )
   {
   }

   omCheckHostCommand::~omCheckHostCommand()
   {
   }

   void omCheckHostCommand::_eraseFromListByIP(
                                            list<omScanHostInfo> &hostInfoList,
                                            const string &ip )
   {
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->ip == ip )
         {
            hostInfoList.erase( iter ) ;
            return ;
         }
         iter++ ;
      }
   }

   void omCheckHostCommand::_updateAgentService(
                                            list<omScanHostInfo> &hostInfoList,
                                            const string &ip,
                                            const string &port )
   {
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->ip == ip )
         {
            iter->agentPort = port ;
            return ;
         }
         iter++ ;
      }
   }

   void omCheckHostCommand::_eraseFromListByHost(
                                             list<omScanHostInfo> &hostInfoList,
                                             const string &hostName )
   {
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->hostName == hostName )
         {
            hostInfoList.erase( iter ) ;
            return ;
         }
         iter++ ;
      }
   }

   void omCheckHostCommand::_eraseFromList( list<omScanHostInfo> &hostInfoList,
                                            BSONObj &oneHost )
   {
      string hostName = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      _eraseFromListByHost( hostInfoList, hostName ) ;
   }

   void omCheckHostCommand::_updateUninstallFlag(
                                            list<omScanHostInfo> &hostInfoList,
                                            const string &ip,
                                            const string &agentPort,
                                            bool isNeedUninstall )
   {
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->ip == ip )
         {
            if ( agentPort != "" )
            {
               iter->agentPort       = agentPort ;
            }

            iter->isNeedUninstall = isNeedUninstall ;
            return ;
         }
         iter++ ;
      }
   }

   INT32 omCheckHostCommand::_installAgent( list<omScanHostInfo> &hostInfoList,
                                            list<BSONObj> &hostResult )
   {
      INT32 rc          = SDB_OK ;
      SINT32 flag       = SDB_OK ;
      CHAR *pContent    = NULL ;
      INT32 contentSize = 0 ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      BSONObj result ;
      BSONObj hostResults ;
      BSONElement rcElement ;

      _generateHostList( hostInfoList, bsonRequest ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_PRE_CHECK_HOST,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build query msg failed:cmd=%s,rc=%d",
                     OM_PRE_CHECK_HOST, rc ) ;
         goto error ;
      }

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_INSTALL_AGET_INTERVAL,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "addSession failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "send message to local agent failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      rc = _receiveFromAgent( remoteSession, flag, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         rc = flag ;
         PD_LOG_MSG( PDERROR, "agent process failed:detail=%s,rc=%d",
                     result.getStringField( OM_REST_RES_DETAIL ), rc ) ;
         goto error ;
      }

      hostResults = result.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;
      {
         BSONObjIterator iter( hostResults ) ;
         while ( iter.more() )
         {
            BSONElement ele   = iter.next() ;
            BSONObj oneResult = ele.embeddedObject() ;
            INT32 rc    = oneResult.getIntField( OM_REST_RES_RETCODE ) ;
            string ip   = oneResult.getStringField( OM_BSON_FIELD_HOST_IP ) ;
            string port = oneResult.getStringField( OM_BSON_FIELD_AGENT_PORT ) ;
            if ( SDB_OK != rc )
            {
               hostResult.push_back( oneResult.getOwned() ) ;
               _eraseFromListByIP( hostInfoList, ip ) ;
            }
            else
            {
               _updateAgentService( hostInfoList, ip, port ) ;
            }
         }
      }

   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::_addCheckHostReq( omManager *om,
                                           pmdRemoteSession *remoteSession,
                                           list<omScanHostInfo> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      _id2Host.clear() ;
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         MsgRouteID routeID ;
         BSONObj bsonRequest ;
         pmdSubSession *subSession = NULL ;
         CHAR *pContent            = NULL ;
         INT32 contentSize         = 0 ;

         routeID   = om->updateAgentInfo( iter->ip, iter->agentPort ) ;
         subSession = remoteSession->addSubSession( routeID.value ) ;
         if ( NULL == subSession )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "addSubSessin failed" ) ;
            goto error ;
         }

         bsonRequest = BSON( OM_BSON_FIELD_HOST_IP << iter->ip
                             << OM_BSON_FIELD_HOST_NAME << iter->hostName
                             << OM_BSON_FIELD_HOST_USER << iter->user
                             << OM_BSON_FIELD_HOST_PASSWD << iter->passwd ) ;
         rc = msgBuildQueryMsg( &pContent, &contentSize,
                                CMD_ADMIN_PREFIX OM_CHECK_HOST_REQ,
                                0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "msgBuildQueryMsg failed:rc=%d", rc ) ;
            goto error ;
         }

         _id2Host[routeID.value] = *iter ;

         subSession->setReqMsg( (MsgHeader *)pContent ) ;
         iter++;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omCheckHostCommand::_updateDiskInfo( BSONObj &onehost )
   {
      BSONObj filter = BSON( OM_BSON_FIELD_DISK << "" ) ;
      BSONObj others ;
      BSONObj disk ;
      others = onehost.filterFieldsUndotted( filter, false ) ;
      disk   = onehost.filterFieldsUndotted( filter, true ) ;

      BSONArrayBuilder arrayBuilder ;

      BSONObj disks = disk.getObjectField( OM_BSON_FIELD_DISK ) ;
      BSONObjIterator iter( disks ) ;
      while ( iter.more() )
      {
         BSONElement ele ;
         BSONObj oneDisk ;
         ele     = iter.next() ;
         oneDisk = ele.embeddedObject() ;

         BSONObjBuilder builder ;
         builder.appendElements( oneDisk ) ;

         BSONElement sizeEle ;
         sizeEle = oneDisk.getField( OM_BSON_FIELD_DISK_FREE_SIZE ) ;
         INT64 freeSize = sizeEle.numberLong();
         if ( freeSize < OM_MIN_DISK_FREE_SIZE )
         {
            builder.append( OM_BSON_FIELD_DISK_CANUSE, false ) ;
         }
         else
         {
            builder.append( OM_BSON_FIELD_DISK_CANUSE, true ) ;
         }

         BSONObj tmp = builder.obj() ;
         arrayBuilder.append( tmp ) ;
      }

      disk = BSON( OM_BSON_FIELD_DISK << arrayBuilder.arr() ) ;

      BSONObjBuilder builder ;
      builder.appendElements( others ) ;
      builder.appendElements( disk ) ;
      onehost = builder.obj() ;
   }

   INT32 omCheckHostCommand::_checkResFormat( BSONObj &oneHost )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;

      ele = oneHost.getField( OM_REST_RES_RETCODE ) ;
      if ( ele.isNumber() )
      {
         INT32 innerRC = ele.Int() ;
         if ( innerRC != SDB_OK )
         {
            goto done ;
         }
      }

      rc = _checkHostBasicContent( oneHost ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_checkHostBasicContent failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omCheckHostCommand::_errorCheckHostEnv(
                                          list<omScanHostInfo> &hostInfoList,
                                          list<BSONObj> &hostResult,
                                          MsgRouteID id, int flag,
                                          const string &error )
   {
      string hostName = "" ;
      string ip       = "" ;
      map<UINT64, omScanHostInfo>::iterator iter = _id2Host.find( id.value ) ;
      SDB_ASSERT( iter != _id2Host.end() , "" ) ;
      if ( iter != _id2Host.end() )
      {
         omScanHostInfo info = iter->second ;
         hostName = info.hostName ;
         ip       = info.ip ;
      }

      BSONObj result = BSON( OM_BSON_FIELD_HOST_IP << ip
                             << OM_BSON_FIELD_HOST_NAME << hostName
                             << OM_REST_RES_RETCODE << flag
                             << OM_REST_RES_DETAIL << error ) ;
      hostResult.push_back( result ) ;
      _eraseFromListByIP( hostInfoList, ip ) ;

      PD_LOG( PDERROR, "check host error:%s,agent=%s,rc=%d", error.c_str(),
              hostName.c_str(), flag ) ;
   }

   INT32 omCheckHostCommand::_checkHostEnv( list<omScanHostInfo> &hostInfoList,
                                            list<BSONObj> &hostResult )
   {
      INT32 rc          = SDB_OK ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      VEC_SUB_SESSIONPTR subSessionVec ;
      INT32 sucNum   = 0 ;
      INT32 totalNum = 0 ;

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_CHECK_HOST_INTERVAL,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "addSession failed:rc=%d", rc ) ;
         goto error ;
      }
      rc = _addCheckHostReq( om, remoteSession, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "generate check host request failed:rc=%d", rc ) ;
         goto done ;
      }

      remoteSession->sendMsg( &sucNum, &totalNum ) ;
      rc = _getAllReplay( remoteSession, &subSessionVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "wait agent's replay failed:rc=%d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < subSessionVec.size() ; i++ )
      {
         rc = SDB_OK ;
         vector<BSONObj> objVec ;
         SINT32 flag               = SDB_OK ;
         SINT64 contextID          = -1 ;
         SINT32 startFrom          = 0 ;
         SINT32 numReturned        = 0 ;
         MsgHeader* pRspMsg        = NULL ;
         pmdSubSession *subSession = subSessionVec[i] ;
         string errorMsg ;
         if ( subSession->isDisconnect() )
         {
            PD_LOG( PDERROR, "session disconnected:id=%s,rc=%d",
                    routeID2String(subSession->getNodeID()).c_str(), rc ) ;
            continue ;
         }

         pRspMsg = subSession->getRspMsg() ;
         if ( NULL == pRspMsg )
         {
            errorMsg = "receive null response from agent" ;
            PD_LOG( PDERROR, "%s", errorMsg.c_str() ) ;
            _errorCheckHostEnv( hostInfoList, hostResult,
                                subSession->getNodeID(), SDB_UNEXPECTED_RESULT,
                                errorMsg ) ;
            continue ;
         }

         rc = msgExtractReply( (CHAR *)pRspMsg, &flag, &contextID, &startFrom,
                               &numReturned, objVec ) ;
         if ( SDB_OK != rc )
         {
            errorMsg = "extract reply failed" ;
            PD_LOG( PDERROR, "%s:rc=%d", errorMsg.c_str(), rc ) ;
            _errorCheckHostEnv( hostInfoList, hostResult,
                                subSession->getNodeID(), rc, errorMsg ) ;
            continue ;
         }

         if ( SDB_OK != flag )
         {
            if ( objVec.size() > 0 )
            {
               errorMsg = objVec[0].getStringField( OM_REST_RES_DETAIL ) ;
            }
            _errorCheckHostEnv( hostInfoList, hostResult,
                                subSession->getNodeID(), flag, errorMsg ) ;
            continue ;
         }

         if ( 1 != objVec.size() )
         {
            errorMsg = "unexpected response size" ;
            _errorCheckHostEnv( hostInfoList, hostResult,
                                subSession->getNodeID(), SDB_UNEXPECTED_RESULT,
                                errorMsg ) ;
            continue ;
         }

         BSONObj result = objVec[0] ;
         rc = _checkResFormat( result ) ;
         if ( SDB_OK != rc )
         {
            errorMsg = "agent's response format error" ;
            _errorCheckHostEnv( hostInfoList, hostResult,
                                subSession->getNodeID(), rc, errorMsg ) ;
            continue ;
         }

         _updateDiskInfo( result ) ;
         hostResult.push_back( result ) ;
         _eraseFromList( hostInfoList, result ) ;
      }

      rc = SDB_OK ;

      {
         list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
         while ( iter != hostInfoList.end() )
         {
            BSONObj tmp = BSON( OM_BSON_FIELD_HOST_IP << iter->ip
                                << OM_BSON_FIELD_HOST_NAME << iter->hostName
                                << OM_REST_RES_RETCODE << SDB_NETWORK
                                << OM_REST_RES_DETAIL << "network error" ) ;
            hostResult.push_back( tmp ) ;
            hostInfoList.erase( iter++ ) ;
         }
      }

   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::_addAgentExitReq( omManager *om,
                                            pmdRemoteSession *remoteSession,
                                            list<omScanHostInfo> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->isNeedUninstall )
         {
            MsgRouteID routeID ;
            pmdSubSession *subSession = NULL ;
            CHAR *pContent            = NULL ;
            INT32 contentSize         = 0 ;
            routeID   = om->updateAgentInfo( iter->ip, iter->agentPort ) ;
            subSession = remoteSession->addSubSession( routeID.value ) ;
            if ( NULL == subSession )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "addSubSessin failed" ) ;
               goto error ;
            }

            rc = msgBuildQueryMsg( &pContent, &contentSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN,
                                   0, 0, 0, -1, NULL, NULL, NULL, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "msgBuildQueryMsg failed:rc=%d", rc ) ;
               goto error ;
            }

            subSession->setReqMsg( (MsgHeader *)pContent ) ;
         }

         iter++;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::_notifyAgentExit(
                                            list<omScanHostInfo> &hostInfoList )
   {
      INT32 rc                        = SDB_OK ;
      omManager *om                   = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      INT32 sucNum                    = 0 ;
      INT32 totalNum                  = 0 ;
      VEC_SUB_SESSIONPTR subSessionVec ;

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                OM_WAIT_AGENT_EXIT_RES_INTERVAL,
                                                NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "addSession failed" ) ;
         goto error ;
      }
      rc = _addAgentExitReq( om, remoteSession, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_addAgentExitReq failed:rc=%d", rc ) ;
         goto done ;
      }

      remoteSession->sendMsg( &sucNum, &totalNum ) ;
      _getAllReplay( remoteSession, &subSessionVec ) ;
   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   void omCheckHostCommand::_generateUninstallReq(
                                             list<omScanHostInfo> &hostInfoList,
                                             BSONObj &bsonRequest )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;

      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         omScanHostInfo *pInfo = &(*iter) ;
         if ( pInfo->isNeedUninstall )
         {
            BSONObjBuilder builder ;
            if ( pInfo->ip != "" )
            {
               builder.append( OM_BSON_FIELD_HOST_IP, pInfo->ip ) ;
            }

            if ( pInfo->hostName != "" )
            {
               builder.append( OM_BSON_FIELD_HOST_NAME, pInfo->hostName ) ;
            }

            builder.append( OM_BSON_FIELD_HOST_USER, pInfo->user ) ;
            builder.append( OM_BSON_FIELD_HOST_PASSWD, pInfo->passwd ) ;
            builder.append( OM_BSON_FIELD_HOST_SSHPORT, pInfo->sshPort ) ;
            BSONObj oneHost = builder.obj() ;
            arrayBuilder.append( oneHost.copy() ) ;
         }

         iter++ ;
      }

      builder.appendArray( OM_REST_FIELD_HOST_INFO, arrayBuilder.arr() );
      bsonRequest = builder.obj() ;

      return ;
   }

   bool omCheckHostCommand::_isNeedUnistall(
                                            list<omScanHostInfo> &hostInfoList )
   {
      bool isNeedUninsall = false ;
      list<omScanHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         if ( iter->isNeedUninstall )
         {
            isNeedUninsall = true ;
            break ;
         }
         iter++ ;
      }

      return isNeedUninsall ;
   }

   INT32 omCheckHostCommand::_uninstallAgent(
                                            list<omScanHostInfo> &hostInfoList )
   {
      INT32 rc          = SDB_OK ;
      CHAR *pContent    = NULL ;
      INT32 contentSize = 0 ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      VEC_SUB_SESSIONPTR subSessionVec ;

      if ( !_isNeedUnistall( hostInfoList ) )
      {
         goto done ;
      }

      _notifyAgentExit( hostInfoList ) ;

      _generateUninstallReq( hostInfoList, bsonRequest ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_POST_CHECK_HOST,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "build query msg failed:cmd=%s,rc=%d",
                OM_POST_CHECK_HOST, rc ) ;
         goto error ;
      }

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                OM_WAIT_AGENT_UNISTALL_INTERVAL,
                                                NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "addSession failed" ) ;
         SDB_OSS_FREE( pContent ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      remoteSession->waitReply( TRUE, &subSessionVec ) ;

   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::_doCheck( list<omScanHostInfo> &hostInfoList,
                                       list<BSONObj> &hostResult )
   {
      INT32 rc = SDB_OK ;
      list<omScanHostInfo> agentInstalledInfo ;
      PD_LOG( PDEVENT, "start to _installAgent" ) ;
      rc = _installAgent( hostInfoList, hostResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "install agent failed:rc=%d", rc ) ;
         goto error ;
      }

      agentInstalledInfo.assign( hostInfoList.begin(), hostInfoList.end() ) ;
      PD_LOG( PDEVENT, "start to _checkHostEnv" ) ;
      rc = _checkHostEnv( hostInfoList, hostResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "check host env failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_LOG( PDEVENT, "start to _uninstallAgent" ) ;
      _uninstallAgent( agentInstalledInfo ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::_getCheckHostList( string &clusterName,
                                                list<omScanHostInfo> &hostInfo )
   {
      INT32 rc                     = SDB_OK ;
      const CHAR* pGlobalUser      = NULL ;
      const CHAR* pGlobalPasswd    = NULL ;
      const CHAR* pGlobalSshPort   = NULL ;
      const CHAR* pGlobalAgentPort = NULL ;
      const CHAR* pHostInfo        = NULL ;
      BSONObj bsonHostInfo ;
      BSONElement element ;


      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_HOST_INFO,
                              &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "%s is null:rc=%d", OM_REST_FIELD_HOST_INFO,
                     rc ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, bsonHostInfo ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pHostInfo, rc ) ;
         goto error ;
      }

      pGlobalUser    = bsonHostInfo.getStringField( OM_BSON_FIELD_HOST_USER ) ;
      pGlobalPasswd  = bsonHostInfo.getStringField(
                                                OM_BSON_FIELD_HOST_PASSWD ) ;
      pGlobalSshPort = bsonHostInfo.getStringField(
                                                OM_BSON_FIELD_HOST_SSHPORT ) ;
      pGlobalAgentPort = _localAgentService.c_str() ;
      clusterName      = bsonHostInfo.getStringField(
                                                OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == ossStrlen( pGlobalUser ) || 0 == ossStrlen( pGlobalPasswd )
           || 0 == ossStrlen( pGlobalSshPort ) || 0 == clusterName.length() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "parameter is empty:%s=%s,%s=%d,%s=%s,%s=%s",
                     OM_BSON_FIELD_HOST_USER, pGlobalUser,
                     OM_BSON_FIELD_HOST_PASSWD, ossStrlen( pGlobalPasswd ),
                     OM_BSON_FIELD_HOST_SSHPORT, pGlobalSshPort,
                     OM_BSON_CLUSTER_NAME, clusterName.c_str() ) ;
         goto error ;
      }

      element = bsonHostInfo.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if ( Array != element.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "%s is not array type:type=%d",
                     OM_BSON_FIELD_HOST_INFO, element.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( element.embeddedObject() ) ;
         while ( i.more() )
         {
            omScanHostInfo host;
            BSONObjBuilder builder ;
            BSONObj tmp ;
            BSONElement ele = i.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "rest info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               goto error ;
            }

            BSONObj oneHost = ele.embeddedObject() ;

            host.ip        = oneHost.getStringField( OM_BSON_FIELD_HOST_IP ) ;
            host.hostName  = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
            host.user      = pGlobalUser ;
            host.passwd    = pGlobalPasswd ;
            host.sshPort   = pGlobalSshPort ;
            host.agentPort = pGlobalAgentPort ;
            host.isNeedUninstall = true ;

            if ( oneHost.hasField( OM_BSON_FIELD_HOST_USER ) )
            {
               host.user = oneHost.getStringField( OM_BSON_FIELD_HOST_USER ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_HOST_PASSWD ) )
            {
               host.passwd = oneHost.getStringField(
                                                  OM_BSON_FIELD_HOST_PASSWD ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_HOST_SSHPORT ) )
            {
               host.sshPort = oneHost.getStringField(
                                                  OM_BSON_FIELD_HOST_SSHPORT ) ;
            }
            if ( oneHost.hasField( OM_BSON_FIELD_AGENT_PORT ) )
            {
               host.agentPort = oneHost.getStringField(
                                                  OM_BSON_FIELD_AGENT_PORT ) ;
            }

            if ( host.ip=="" || host.hostName=="" )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "rest field miss:%s or %s",
                           OM_BSON_FIELD_HOST_IP, OM_BSON_FIELD_HOST_NAME ) ;
               goto error ;
            }

            hostInfo.push_back( host ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckHostCommand::doCommand()
   {
      list<omScanHostInfo> hostInfoList ;
      list<BSONObj> hostResult ;
      BSONObj bsonRequest ;
      string clusterName = "" ;
      INT32 rc           = SDB_OK ;

      rc = _getCheckHostList( clusterName, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "fail to get host list:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _filterExistHost( hostInfoList, hostResult ) ;
      if ( hostInfoList.size() == 0 )
      {
         _sendResult2Web( hostResult ) ;
         goto done ;
      }

      rc = _doCheck( hostInfoList, hostResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "do check failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendResult2Web( hostResult ) ;

   done:
      return rc;
   error:
      goto done ;
   }

   omAddHostCommand::omAddHostCommand( restAdaptor *pRestAdaptor,
                                       pmdRestSession *pRestSession,
                                       const string &localAgentHost,
                                       const string &localAgentService )
                    : omScanHostCommand( pRestAdaptor, pRestSession,
                                         localAgentHost, localAgentService )
   {
   }

   omAddHostCommand::~omAddHostCommand()
   {
   }

   INT32 omAddHostCommand::_getRestHostList( string &clusterName,
                                             list<BSONObj> &hostInfo )
   {
      INT32 rc                     = SDB_OK ;
      const CHAR* pGlobalUser      = NULL ;
      const CHAR* pGlobalPasswd    = NULL ;
      const CHAR* pGlobalSshPort   = NULL ;
      const CHAR* pGlobalAgentPort = NULL ;
      const CHAR* pHostInfo        = NULL ;
      string installPath ;
      BSONObj bsonHostInfo ;
      BSONElement element ;

      _restAdaptor->getQuery(_restSession, OM_REST_FIELD_HOST_INFO,
                             &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field miss:field=%s",
                     OM_REST_FIELD_HOST_INFO ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, bsonHostInfo ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pHostInfo, rc ) ;
         goto error ;
      }

      pGlobalUser      = bsonHostInfo.getStringField( OM_BSON_FIELD_HOST_USER ) ;
      pGlobalPasswd    = bsonHostInfo.getStringField( OM_BSON_FIELD_HOST_PASSWD ) ;
      pGlobalSshPort   = bsonHostInfo.getStringField(
                                       OM_BSON_FIELD_HOST_SSHPORT ) ;
      pGlobalAgentPort = bsonHostInfo.getStringField(
                                       OM_BSON_FIELD_AGENT_PORT ) ;
      clusterName    = bsonHostInfo.getStringField(
                                       OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == ossStrlen( pGlobalUser ) || 0 == ossStrlen( pGlobalPasswd )
           || 0 == ossStrlen( pGlobalSshPort ) || 0 == clusterName.length()
           || 0 == ossStrlen( pGlobalAgentPort ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field miss:field=[%s or %s or %s or %s "
                     "or %s]", OM_BSON_FIELD_HOST_USER,
                     OM_BSON_FIELD_HOST_PASSWD, OM_BSON_FIELD_HOST_SSHPORT,
                     OM_BSON_FIELD_AGENT_PORT, OM_BSON_CLUSTER_NAME ) ;
         goto error ;
      }

      rc = _getClusterInstallPath( clusterName, installPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get cluster's install path failed:rc=%d", rc ) ;
         goto error ;
      }

      element = bsonHostInfo.getField( OM_BSON_FIELD_HOST_INFO ) ;
      {
         BSONObjIterator i( element.embeddedObject() ) ;
         while ( i.more() )
         {
            string hostName ;
            BSONObjBuilder builder ;
            BSONObj tmp ;
            BSONElement ele = i.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "element of %s is not Object type"
                           ":type=%d", OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               goto error ;
            }
            BSONObj oneHost = ele.embeddedObject() ;
            rc = _checkHostBasicContent( oneHost ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_checkHostBasicContent failed:rc=%d", rc ) ;
               goto error ;
            }

            builder.appendElements( oneHost ) ;
            if ( !oneHost.hasField( OM_BSON_FIELD_HOST_USER ) )
            {
               builder.append( OM_BSON_FIELD_HOST_USER, pGlobalUser ) ;
            }

            if ( !oneHost.hasField( OM_BSON_FIELD_HOST_PASSWD ) )
            {
               builder.append( OM_BSON_FIELD_HOST_PASSWD, pGlobalPasswd) ;
            }

            if ( !oneHost.hasField( OM_BSON_FIELD_HOST_SSHPORT ) )
            {
               builder.append( OM_BSON_FIELD_HOST_SSHPORT, pGlobalSshPort) ;
            }

            if ( !oneHost.hasField( OM_BSON_FIELD_AGENT_PORT ) )
            {
               builder.append( OM_BSON_FIELD_AGENT_PORT, pGlobalAgentPort) ;
            }

            if ( !oneHost.hasField( OM_BSON_FIELD_INSTALL_PATH ) )
            {
               builder.append( OM_BSON_FIELD_INSTALL_PATH, installPath) ;
            }

            tmp = builder.obj() ;
            hostInfo.push_back( tmp ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostCommand::_getClusterInstallPath( const string &clusterName,
                                                   string &installPath )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder resultBuilder ;
      BSONObj result ;

      BSONObjBuilder builder ;
      builder.append( OM_CLUSTER_FIELD_NAME, clusterName ) ;
      BSONObj matcher ;
      matcher = builder.obj() ;

      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID             = -1 ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order, hint, 0,
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to query table:table=%s,rc=%d",
                     OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               PD_LOG_MSG( PDERROR, "cluster is not exist:cluster=%s,rc=%d",
                           clusterName.c_str(), rc ) ;
               goto error ;
            }

            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         installPath = record.getStringField( OM_CLUSTER_FIELD_INSTALLPATH ) ;
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void omAddHostCommand::_generateTableField( BSONObjBuilder &builder,
                                               const string &newFieldName,
                                               BSONObj &bsonOld,
                                               const string &oldFiledName )
   {
      BSONElement element = bsonOld.getField( oldFiledName ) ;
      if ( !element.eoo() )
      {
         builder.appendAs( element, newFieldName ) ;
      }
   }

   INT32 omAddHostCommand::_checkTaskExistence( list<BSONObj> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      list<BSONObj>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         string hostName = iter->getStringField( OM_BSON_FIELD_HOST_NAME ) ;
         if ( _isHostExistInTask( hostName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host is exist in task:host=%s",
                        hostName.c_str() ) ;
            goto error ;
         }
         iter++ ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostCommand::_checkHostExistence( list<BSONObj> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      set<string> hostNameSet ;
      list<BSONObj>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         string host = iter->getStringField( OM_BSON_FIELD_HOST_NAME ) ;
         if ( _isHostNameExist( host ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host is exist:host=%s", host.c_str() ) ;
            goto error ;
         }
         iter++ ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostCommand::_checkHostDisk( list<BSONObj> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      set<string> hostNameSet ;
      list<BSONObj>::iterator iter ;

      for ( iter = hostInfoList.begin(); iter != hostInfoList.end(); ++iter )
      {
         map<string,int> diskList ;
         BSONObj diskArray = iter->getObjectField( OM_BSON_FIELD_DISK ) ;
         BSONObjIterator iter( diskArray ) ;
         while ( iter.more() )
         {
            string diskName ;
            BSONElement ele = iter.next() ;
            if ( ele.type() != Object )
            {
               continue ;
            }
            BSONObj oneDisk = ele.embeddedObject() ;
            diskName = oneDisk.getStringField( OM_BSON_FIELD_DISK_NAME ) ;
            if( diskList.find( diskName ) != diskList.end() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "duplicate disk name:disk name=%s",
                           diskName.c_str() ) ;
               goto error ;
            }
            diskList[diskName] = 1 ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostCommand::_generateTaskInfo( const string &clusterName,
                                              list<BSONObj> &hostInfoList,
                                              BSONObj &taskInfo,
                                              BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder resultArrBuilder ;
      BSONArrayBuilder arrayBuilder ;
      list<BSONObj>::iterator iter ;
      string sdbUser ;
      string sdbPasswd ;
      string sdbUserGroup ;
      string packetPath ;
      rc = getPacketFile( OM_BUSINESS_SEQUOIADB, packetPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get packet path failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _getSdbUsrInfo( clusterName, sdbUser, sdbPasswd, sdbUserGroup ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getSdbUsrInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         BSONObjBuilder innerBuilder ;
         BSONObj tmp ;

         _generateTableField( innerBuilder, OM_HOST_FIELD_NAME, *iter,
                              OM_BSON_FIELD_HOST_NAME ) ;
         innerBuilder.append( OM_HOST_FIELD_CLUSTERNAME, clusterName ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_IP, *iter,
                              OM_BSON_FIELD_HOST_IP ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_USER, *iter,
                              OM_BSON_FIELD_HOST_USER ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_PASSWORD, *iter,
                              OM_BSON_FIELD_HOST_PASSWD ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_OS, *iter,
                              OM_BSON_FIELD_OS ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_OMA, *iter,
                              OM_BSON_FIELD_OMA ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_MEMORY, *iter,
                              OM_BSON_FIELD_MEMORY ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_DISK, *iter,
                              OM_BSON_FIELD_DISK ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_CPU, *iter,
                              OM_BSON_FIELD_CPU ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_NET, *iter,
                              OM_BSON_FIELD_NET ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_PORT, *iter,
                              OM_BSON_FIELD_PORT ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_SERVICE, *iter,
                              OM_BSON_FIELD_SERVICE ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_SAFETY, *iter,
                              OM_BSON_FIELD_SAFETY ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_INSTALLPATH, *iter,
                              OM_BSON_FIELD_INSTALL_PATH ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_AGENT_PORT, *iter,
                              OM_BSON_FIELD_AGENT_PORT ) ;
         _generateTableField( innerBuilder, OM_HOST_FIELD_SSHPORT, *iter,
                              OM_BSON_FIELD_HOST_SSHPORT ) ;

         tmp = innerBuilder.obj() ;
         arrayBuilder.append( tmp ) ;

         BSONObjBuilder itemBuilder ;
         itemBuilder.append( OM_HOST_FIELD_IP,
                             iter->getStringField( OM_HOST_FIELD_IP ) ) ;
         itemBuilder.append( OM_HOST_FIELD_NAME,
                             iter->getStringField( OM_HOST_FIELD_NAME ) ) ;
         itemBuilder.append( OM_TASKINFO_FIELD_STATUS, OM_TASK_STATUS_INIT ) ;
         itemBuilder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                             getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
         itemBuilder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
         itemBuilder.append( OM_REST_RES_DETAIL, "" ) ;
         {
            BSONArrayBuilder tmpEmptyBuilder ;
            itemBuilder.append( OM_TASKINFO_FIELD_FLOW,
                                tmpEmptyBuilder.arr() ) ;
         }


         BSONObj resultItem = itemBuilder.obj() ;
         resultArrBuilder.append( resultItem ) ;

         iter++ ;
      }

      taskInfo = BSON( OM_BSON_CLUSTER_NAME << clusterName
                       << OM_BSON_FIELD_SDB_USER << sdbUser
                       << OM_BSON_FIELD_SDB_PASSWD << sdbPasswd
                       << OM_BSON_FIELD_SDB_USERGROUP << sdbUserGroup
                       << OM_BSON_FIELD_PATCKET_PATH << packetPath
                       << OM_BSON_FIELD_HOST_INFO << arrayBuilder.arr() ) ;

      resultInfo = resultArrBuilder.arr() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 omAddHostCommand::_generateTaskID()
   {
      INT64 taskID = 0 ;
      getMaxTaskID( taskID ) ;

      return ++taskID ;
   }

   INT32 omAddHostCommand::_saveTask( INT64 taskID, const BSONObj &taskInfo,
                                      const BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;

      rc = createTask( OM_TASK_TYPE_ADD_HOST, taskID,
                       getTaskTypeStr( OM_TASK_TYPE_ADD_HOST ),
                       _localAgentHost, _localAgentService, taskInfo,
                       resultInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "create task failed:rc=%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostCommand::_removeTask( INT64 taskID )
   {
      return removeTask( taskID ) ;
   }

   INT32 omAddHostCommand::doCommand()
   {
      INT64 taskID ;
      list<BSONObj> hostInfoList ;
      string clusterName ;
      INT32 rc = SDB_OK ;
      BSONObjBuilder bsonBuilder ;

      rc = _getRestHostList( clusterName, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "fail to get host list:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _checkHostDisk( hostInfoList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to _checkHostDisk:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _checkHostExistence( hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "fail to _checkHostExistence:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _checkTaskExistence( hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "fail to _checkTaskExistence:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      {
         BSONObj taskInfo ;
         BSONArray resultInfo ;
         rc = _generateTaskInfo( clusterName, hostInfoList, taskInfo,
                                 resultInfo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "generate task info failed:rc=%d", rc ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _sendErrorRes2Web( rc, _errorDetail ) ;
            goto error ;
         }

         taskID = _generateTaskID() ;

         rc = _saveTask( taskID, taskInfo, resultInfo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "fail to save task:taskID="OSS_LL_PRINT_FORMAT
                    ",rc=%d", taskID, rc ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _sendErrorRes2Web( rc, _errorDetail ) ;
            goto error ;
         }


      }

      rc = _notifyAgentTask( taskID ) ;
      if ( SDB_OK != rc )
      {
         _removeTask( taskID ) ;
         PD_LOG( PDERROR, "fail to notify task:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", taskID, rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << (long long)taskID ) ;
         _restAdaptor->appendHttpBody( _restSession, result.objdata(),
                                       result.objsize(), 1 ) ;
         _sendOKRes2Web() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omListHostCommand::omListHostCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession )
                     : omCheckSessionCommand( pRestAdaptor, pRestSession )
   {
   }

   omListHostCommand::~omListHostCommand()
   {
   }

   void omListHostCommand::_sendHostInfo2Web( list<BSONObj> &hosts )
   {
      BSONObj filter = BSON( OM_HOST_FIELD_PASSWORD << "" ) ;
      list<BSONObj>::iterator iter = hosts.begin() ;
      while ( iter != hosts.end() )
      {
         BSONObj tmp = iter->filterFieldsUndotted( filter, false ) ;
         _restAdaptor->appendHttpBody( _restSession, tmp.objdata(),
                                       tmp.objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omListHostCommand::doCommand()
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusiness     = NULL ;
      list<BSONObj> records ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CLUSTER_NAME,
                              &pClusterName ) ;
      _restAdaptor->getQuery( _restSession, OM_REST_BUSINESS_NAME,
                              &pBusiness ) ;
      if( NULL != pClusterName )
      {
         BSONObj selector = BSON( OM_HOST_FIELD_NAME << 1
                                  << OM_HOST_FIELD_IP << 1) ;
         BSONObj matcher  = BSON( OM_HOST_FIELD_CLUSTERNAME << pClusterName ) ;
         BSONObj order ;
         BSONObj hint ;
         rc = _queryTable( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                           0, 0, -1, records ) ;
      }
      else if ( NULL != pBusiness )
      {
         list<BSONObj> tmpRecords ;
         BSONObj selector = BSON( OM_CONFIGURE_FIELD_HOSTNAME << 1 ) ;
         BSONObj matcher  = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME
                                  << pBusiness ) ;
         BSONObj order ;
         BSONObj hint ;
         rc = _queryTable( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order,
                           hint, 0, 0, -1, tmpRecords ) ;
         if ( SDB_OK == rc && tmpRecords.size() > 0 )
         {
            selector = BSON( OM_HOST_FIELD_NAME << 1 << OM_HOST_FIELD_IP << 1) ;
            BSONArrayBuilder arrBuilder ;
            list<BSONObj>::iterator iter = tmpRecords.begin() ;
            while ( iter != tmpRecords.end() )
            {
               arrBuilder.append( *iter ) ;
               iter++ ;
            }

            matcher  = BSON( "$or" << arrBuilder.arr() ) ;
            rc = _queryTable( OM_CS_DEPLOY_CL_HOST, selector, matcher,
                              order, hint, 0, 0, -1, records ) ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field miss:%s or %s",
                     OM_REST_FIELD_CLUSTER_NAME, OM_REST_BUSINESS_NAME ) ;
      }

      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendHostInfo2Web( records ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omQueryHostCommand::omQueryHostCommand( restAdaptor *pRestAdaptor,
                                           pmdRestSession *pRestSession )
                      :omListHostCommand( pRestAdaptor, pRestSession )
   {
   }

   omQueryHostCommand::~omQueryHostCommand()
   {
   }

   INT32 omQueryHostCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      list<BSONObj> hosts ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                        0, skip, returnNum, hosts ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendHostInfo2Web( hosts ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omListBusinessTypeCommand::omListBusinessTypeCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession,
                                                const CHAR *pRootPath,
                                                const CHAR *pSubPath )
                             : omCheckSessionCommand( pRestAdaptor,
                                                      pRestSession ),
                              _rootPath( pRootPath ), _subPath( pSubPath )
   {
   }

   omListBusinessTypeCommand::~omListBusinessTypeCommand()
   {
   }

   BOOLEAN omListBusinessTypeCommand::_isArray( ptree &pt )
   {
      BOOLEAN isArr = FALSE ;
      string type ;
      try
      {
         type = pt.get<string>( OM_XMLATTR_TYPE ) ;
      }
      catch( std::exception &e )
      {
         isArr = FALSE ;
         goto done ;
      }

      if ( ossStrcasecmp( type.c_str(), OM_XMLATTR_TYPE_ARRAY ) == 0 )
      {
         isArr = TRUE ;
         goto done ;
      }

   done:
      return isArr ;
   }

   BOOLEAN omListBusinessTypeCommand::_isStringValue( ptree &pt )
   {
      BOOLEAN isStringV = FALSE ;
      if ( _isArray( pt ) )
      {
         isStringV = FALSE ;
         goto done ;
      }

      if ( pt.size() == 0 )
      {
         isStringV = TRUE ;
         goto done ;
      }

      if ( pt.size() > 1 )
      {
         isStringV = FALSE ;
         goto done ;
      }

      {
         ptree::iterator ite = pt.begin() ;
         string key          = ite->first ;
         if ( ossStrcasecmp( key.c_str(), OM_XMLATTR_KEY ) == 0 )
         {
            isStringV = TRUE ;
            goto done ;
         }
      }

   done:
      return isStringV ;
   }

   void omListBusinessTypeCommand::_parseArray( ptree &pt,
                                            BSONArrayBuilder &arrayBuilder )
   {
      ptree::iterator ite = pt.begin() ;
      for( ; ite != pt.end() ; ite++ )
      {
         string key    = ite->first ;
         if ( ossStrcasecmp( key.c_str(), OM_XMLATTR_KEY ) == 0 )
         {
            continue ;
         }

         BSONObj obj ;
         ptree child = ite->second ;
         _recurseParseObj( child, obj ) ;
         arrayBuilder.append( obj ) ;
      }
   }

   void omListBusinessTypeCommand::_recurseParseObj( ptree &pt, BSONObj &out )
   {
      BSONObjBuilder builder ;
      ptree::iterator ite = pt.begin() ;
      for( ; ite != pt.end() ; ite++ )
      {
         string key = ite->first ;
         if ( ossStrcasecmp( key.c_str(), OM_XMLATTR_KEY ) == 0 )
         {
            continue ;
         }

         ptree child = ite->second ;
         if ( _isArray( child ) )
         {
            BSONArrayBuilder arrayBuilder ;
            _parseArray( child, arrayBuilder ) ;
            builder.append( key, arrayBuilder.arr() ) ;
         }
         else if ( _isStringValue( child ) )
         {
            string value = ite->second.data() ;
            builder.append( key, value ) ;
         }
         else
         {
            BSONObj obj ;
            _recurseParseObj( child, obj ) ;
            builder.append(key, obj ) ;
         }
      }

      out = builder.obj() ;
   }

   INT32 omListBusinessTypeCommand::_readConfigFile( const string &file,
                                                     BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         ptree pt ;
         read_xml( file.c_str(), pt ) ;
         _recurseParseObj( pt, obj ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDPATH ;
         PD_LOG_MSG( PDERROR, "%s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omListBusinessTypeCommand::_getBusinessList(
                                               list<BSONObj> &businessList )
   {
      INT32 rc = SDB_OK ;
      string businessFile ;
      BSONObj fileContent ;

      businessFile = _rootPath + OSS_FILE_SEP + OM_BUSINESS_CONFIG_SUBDIR
                     + OSS_FILE_SEP + OM_BUSINESS_FILE_NAME
                     + _languageFileSep + OM_CONFIG_FILE_TYPE ;

      rc = _readConfigFile( businessFile, fileContent ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "read business file failed:file=%s",
                 businessFile.c_str() ) ;
         goto error ;
      }

      {
         BSONObj businessArray = fileContent.getObjectField(
                                                       OM_BSON_BUSINESS_LIST ) ;
         BSONObjIterator iter( businessArray ) ;
         while( iter.more() )
         {
            BSONElement ele      = iter.next() ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "field is not Object:field=%s,type=%d",
                           OM_BSON_BUSINESS_LIST, ele.type() ) ;
               goto error ;
            }

            BSONObj oneBusiness = ele.embeddedObject() ;
            businessList.push_back( oneBusiness.copy() ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omListBusinessTypeCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> businessList ;

      _setFileLanguageSep() ;

      rc = _getBusinessList( businessList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getBusinessList failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      {
         list<BSONObj>::iterator iter = businessList.begin() ;
         while ( iter != businessList.end() )
         {

            _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                          iter->objsize(), 1 ) ;
            iter++ ;
         }
      }
      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omGetBusinessTemplateCommand::omGetBusinessTemplateCommand(
                                                   restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   const CHAR *pRootPath,
                                                   const CHAR *pSubPath )
                                :omListBusinessTypeCommand( pRestAdaptor,
                                                            pRestSession,
                                                            pRootPath,
                                                            pSubPath )
   {
   }

   omGetBusinessTemplateCommand::~omGetBusinessTemplateCommand()
   {
   }

   INT32 omGetBusinessTemplateCommand::_readConfTemplate(
                                                const string &businessType,
                                                const string &file,
                                                list<BSONObj> &deployModList )
   {
      INT32 rc = SDB_OK ;
      BSONObj deployModArray ;
      BSONObj deployMods ;
      rc = _readConfigFile( file, deployModArray ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "read file failed:file=%s", file.c_str() ) ;
         goto error ;
      }

      deployMods = deployModArray.getObjectField( OM_BSON_DEPLOY_MOD_LIST ) ;
      {
         BSONObjIterator iter( deployMods ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "field's element is not Object:field=%s"
                           ",type=%d", OM_BSON_DEPLOY_MOD_LIST, ele.type() ) ;
               goto error ;
            }
            BSONObj oneDeployMod = ele.embeddedObject() ;
            deployModList.push_back( oneDeployMod.copy()) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessTemplateCommand::_readConfDetail( const string &file,
                                                        BSONObj &bsonConfDetail )
   {
      INT32 rc = SDB_OK ;
      rc = _readConfigFile( file, bsonConfDetail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "read file failed:file=%s", file.c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessTemplateCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR* pBusinessType  = NULL ;
      const CHAR* pOperationType = NULL ;
      list<BSONObj> deployModList ;
      list<BSONObj>::iterator iter ;
      string businessType ;
      string operationType ;

      _setFileLanguageSep() ;

      omConfigTool configTool( _rootPath, _languageFileSep ) ;

      _restAdaptor->getQuery(_restSession, OM_REST_BUSINESS_TYPE,
                             &pBusinessType ) ;
      if ( NULL == pBusinessType )
      {
         _errorDetail = string( "rest field:" ) + OM_REST_BUSINESS_TYPE
                        + " is null" ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      businessType = pBusinessType ;

      _restAdaptor->getQuery( _restSession, OM_BSON_OPERATION_TYPE,
                              &pOperationType ) ;
      if ( pOperationType == NULL )
      {
         operationType = OM_FIELD_OPERATION_DEPLOY ;
      }
      else
      {
         operationType = pOperationType ;
         if( operationType != OM_FIELD_OPERATION_DEPLOY &&
             operationType != OM_FIELD_OPERATION_EXTEND )
         {
            _errorDetail = OM_BSON_OPERATION_TYPE" is invalid" ;
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "%s is invalid: %s=%s, rc=%d",
                    OM_BSON_OPERATION_TYPE,
                    OM_BSON_OPERATION_TYPE,
                    pOperationType,
                    rc ) ;
            _sendErrorRes2Web( rc, _errorDetail ) ;
            goto error ;
         }
      }

      if( businessType == OM_BUSINESS_SEQUOIASQL )
      {
         _errorDetail = "only supported in enterprise edition" ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }


      rc = configTool.readBuzTemplate( businessType, operationType,
                                       deployModList ) ;
      if( rc )
      {
         _errorDetail = "failed to read template file" ;
         PD_LOG( PDERROR, "failed to read template:file=%s:rc=%d",
                 configTool.getBuzTemplatePath( businessType,
                                                operationType ).c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      iter = deployModList.begin() ;
      while ( iter != deployModList.end() )
      {
         _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omGetBusinessConfigCommand::omGetBusinessConfigCommand(
                                                 restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession,
                                                 string &rootPath )
                           : omAuthCommand( pRestAdaptor, pRestSession ),
                           _rootPath( rootPath )

   {
   }

   omGetBusinessConfigCommand::~omGetBusinessConfigCommand()
   {
   }

   INT32 omGetBusinessConfigCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj templateInfo ;
      BSONObj deployModInfo ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      _operationType = OM_FIELD_OPERATION_DEPLOY ;

      rc = option.parseRestArg( "j|s",
                                OM_REST_TEMPLATE_INFO,        &templateInfo,
                                OM_REST_FIELD_OPERATION_TYPE, &_operationType ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( templateInfo, deployModInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateRequest( restTool, templateInfo, deployModInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate request: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_checkRestInfo(
                                                const BSONObj &templateInfo )
   {
      INT32 rc = SDB_OK ;

      _clusterName = templateInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == _clusterName.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_CLUSTER_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = templateInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      if ( 0 == _businessType.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_TYPE ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessName = templateInfo.getStringField(
                                                OM_BSON_BUSINESS_NAME ) ;
      if ( 0 == _businessName.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _deployMod = templateInfo.getStringField( OM_BSON_DEPLOY_MOD ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_checkTemplate( BSONObj &deployModInfo )
   {
      INT32 rc = SDB_OK ;
      omConfigTool cfgTool( _rootPath, _languageFileSep ) ;

      {
         list<BSONObj> businessTypeList ;
         list<BSONObj>::iterator iter ;

         rc = cfgTool.readBuzTypeList( businessTypeList ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         for ( iter = businessTypeList.begin();
                     iter != businessTypeList.end(); ++iter )
         {
            string businessType = iter->getStringField(
                                                OM_XML_FIELD_BUSINESS_TYPE ) ;

            if( _businessType == businessType )
            {
               break ;
            }
         }

         if ( iter == businessTypeList.end() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid %s: type=%s",
                                OM_XML_FIELD_BUSINESS_TYPE,
                                _businessType.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      {
         list<BSONObj> templateList ;
         list<BSONObj>::iterator iter ;

         rc =  cfgTool.readBuzTemplate( _businessType, _operationType,
                                        templateList ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         for ( iter = templateList.begin(); iter != templateList.end(); ++iter )
         {
            string deployMod = iter->getStringField( OM_XML_FIELD_DEPLOY_MOD ) ;

            if ( deployMod == _deployMod )
            {
               deployModInfo = iter->copy() ;
               break ;
            }
         }

         if ( iter == templateList.end() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid %s: mode=%s",
                                OM_XML_FIELD_DEPLOY_MOD, _deployMod.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_check( const BSONObj &templateInfo,
                                             BSONObj &deployModInfo )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isBusinessExist = FALSE ;
      omDatabaseTool dbTool( _cb ) ;

      if( OM_FIELD_OPERATION_DEPLOY != _operationType &&
          OM_FIELD_OPERATION_EXTEND != _operationType )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid %s: %s=%s",
                             OM_REST_FIELD_OPERATION_TYPE,
                             OM_REST_FIELD_OPERATION_TYPE,
                             _operationType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _checkRestInfo( templateInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check rest info: rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkTemplate( deployModInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check template info: rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      isBusinessExist = dbTool.isBusinessExist( _businessName ) ;

      if ( TRUE == isBusinessExist &&
           OM_FIELD_OPERATION_DEPLOY == _operationType )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business already exist: name=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( FALSE == isBusinessExist &&
           OM_FIELD_OPERATION_EXTEND == _operationType )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: name=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_getPropertyValue( const BSONObj &property,
                                                        const string &name,
                                                        string &value )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator iter( property ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj oneProperty = ele.embeddedObject() ;
         string propertyName = oneProperty.getStringField( OM_BSON_NAME ) ;

         if ( propertyName == name )
         {
            value = oneProperty.getStringField( OM_BSON_VALUE ) ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;
      _errorMsg.setError( TRUE, "Property %s does not exist: name=%s",
                          name.c_str() ) ;
      PD_LOG( PDERROR, _errorMsg.getError() ) ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_getDeployProperty(
                                                   const BSONObj &templateInfo,
                                                   const BSONObj &deployModInfo,
                                                   BSONObj &deployProperty )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj propertyTemplate = deployModInfo.getObjectField(
                                                            OM_BSON_PROPERTY ) ;
      BSONObj propertyValue = templateInfo.getObjectField( OM_BSON_PROPERTY ) ;

      builder.append( OM_BSON_CLUSTER_NAME,  _clusterName ) ;
      builder.append( OM_BSON_BUSINESS_TYPE, _businessType ) ;
      builder.append( OM_BSON_BUSINESS_NAME, _businessName ) ;
      builder.append( OM_BSON_DEPLOY_MOD,    _deployMod ) ;

      {
         BSONArrayBuilder propertyArray ;
         BSONObjIterator iter( propertyTemplate ) ;

         while ( iter.more() )
         {
            BSONObjBuilder onePropertyBuilder ;
            BSONElement ele = iter.next() ;
            BSONObj onePropertyTemplate = ele.embeddedObject() ;
            string name = onePropertyTemplate.getStringField(
                                                         OM_XML_FIELD_NAME ) ;
            string value ;

            rc = _getPropertyValue( propertyValue, name, value ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to get property value: name=%s, rc=%d",
                       name.c_str(), rc ) ;
               goto error ;
            }

            onePropertyBuilder.appendElements( onePropertyTemplate ) ;
            onePropertyBuilder.append( OM_BSON_VALUE, value ) ;

            propertyArray.append( onePropertyBuilder.obj() ) ;
         }

         builder.append( OM_BSON_PROPERTY, propertyArray.arr() ) ;
      }

      deployProperty = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_getBuzInfoOfCluster(
                                                   BSONObj &buzInfoOfCluster )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder builder ;
      list<BSONObj> buzInfoList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getBusinessInfoOfCluster( _clusterName, buzInfoList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info of cluster: "
                                   "cluster=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = buzInfoList.begin(); iter != buzInfoList.end(); ++iter )
      {
         builder.append( *iter ) ;
      }

      buzInfoOfCluster = BSON( OM_BSON_BUSINESS_INFO << builder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_getHostInfoOfCluster(
                                                   BSONObj &hostsInfoOfCluster )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder builder ;
      list<BSONObj> hostList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getHostInfoByCluster( _clusterName, hostList ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to get host info:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == hostList.size() )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         _errorMsg.setError( TRUE, "failed to get host info: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = hostList.begin(); iter != hostList.end(); ++iter )
      {
         string hostName ;
         BSONObjBuilder hostBuilder ;
         BSONObj hostConfig ;
         BSONObj hostInfo = *iter ;

         hostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
         rc = dbTool.getConfigByHostName( hostName, hostConfig ) ;
         if( rc )
         {
            _errorMsg.setError( TRUE, "Failed to get host config, rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
         hostBuilder.appendElements( hostInfo ) ;
         hostBuilder.appendElements( hostConfig ) ;

         builder.append( hostBuilder.obj() ) ;
      }

      hostsInfoOfCluster = BSON( OM_BSON_HOST_INFO << builder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetBusinessConfigCommand::_generateRequest(
                                                omRestTool &restTool,
                                                const BSONObj &templateInfo,
                                                const BSONObj &deployModInfo )
   {
      INT32 rc = SDB_OK ;
      OmConfigBuilder* confBuilder = NULL ;
      BSONObj deployProperty ;
      BSONObj buzTemplate ;
      BSONObj hostListOfCluster ;
      BSONObj buzListOfCluster ;
      BSONObj deployConfig ;
      set<string> hostNames ;
      omConfigTool cfgTool( _rootPath, _languageFileSep ) ;

      {
         OmBusinessInfo businessInfo ;

         businessInfo.clusterName  = _clusterName ;
         businessInfo.businessType = _businessType ;
         businessInfo.businessName = _businessName ;
         businessInfo.deployMode   = _deployMod ;

         rc = OmConfigBuilder::createInstance( businessInfo, _operationType,
                                               confBuilder ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, "failed to create builder: rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = _getDeployProperty( templateInfo, deployModInfo, deployProperty ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get deploy property: rc=%d", rc ) ;
         goto error ;
      }

      rc = cfgTool.readBuzConfig( _businessType, _deployMod, FALSE,
                                  buzTemplate ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _getHostInfoOfCluster( hostListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get hosts info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      rc = _getBuzInfoOfCluster( buzListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get business info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      rc = confBuilder->getHostNames( templateInfo, OM_BSON_HOST_INFO,
                                      hostNames ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, "failed to get host names: rc=%d", rc ) ;
         goto error ;
      }

      rc = confBuilder->generateConfig( deployProperty, buzTemplate,
                                        hostListOfCluster, buzListOfCluster,
                                        hostNames, deployConfig ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, confBuilder->getErrorDetail().c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      {
         BSONObjBuilder builder ;
         BSONArrayBuilder propertyArray ;
         BSONObj property = buzTemplate.getObjectField( OM_XML_FIELD_PROPERTY ) ;
         BSONObjIterator iter( property ) ;

         builder.appendElements( deployConfig ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj oneProperty = ele.embeddedObject() ;

            propertyArray.append( oneProperty ) ;
         }

         builder.append( OM_BSON_PROPERTY, propertyArray.arr() ) ;

         restTool.appendResponeContent( builder.obj() ) ;
      }

   done:
      if( confBuilder )
      {
         SDB_OSS_DEL( confBuilder ) ;
         confBuilder = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   omAddBusinessCommand::omAddBusinessCommand( restAdaptor *pRestAdaptor,
                                               pmdRestSession *pRestSession,
                                               string &rootPath,
                                               string &path,
                                               string &localAgentHost,
                                               string &localAgentService )
                     : omAuthCommand( pRestAdaptor, pRestSession ),
                     _rootPath( rootPath ),
                     _localAgentHost( localAgentHost ),
                     _localAgentService( localAgentService ),
                     _force( FALSE )
   {
   }

   omAddBusinessCommand::~omAddBusinessCommand()
   {
   }

   INT32 omAddBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      BSONObj configInfo ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "j|b", OM_REST_FIELD_CONFIGINFO, &configInfo,
                                       OM_REST_FIELD_FORCE, &_force ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( configInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateRequest( configInfo, taskConfig, resultInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate task info: rc=%d", rc ) ;
         goto error ;
      }

      rc = _createTask( taskConfig, resultInfo, taskID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to create task: rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << taskID ) ;

         rc = restTool.appendResponeContent( result ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to append respone content: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omAddBusinessCommand::_checkRestInfo( const BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;

      _clusterName = configInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == _clusterName.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_CLUSTER_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = configInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      if ( 0 == _businessType.length() )
      {
         rc = SDB_INVALIDARG ;
        _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_TYPE ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessName = configInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      if ( 0 == _businessName.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_BUSINESS_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _deployMod = configInfo.getStringField( OM_BSON_DEPLOY_MOD ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_checkTemplate()
   {
      INT32 rc = SDB_OK ;
      omConfigTool cfgTool( _rootPath, _languageFileSep ) ;

      {
         list<BSONObj> businessTypeList ;
         list<BSONObj>::iterator iter ;

         rc = cfgTool.readBuzTypeList( businessTypeList ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         for ( iter = businessTypeList.begin();
                     iter != businessTypeList.end(); ++iter )
         {
            string businessType = iter->getStringField(
                                                OM_XML_FIELD_BUSINESS_TYPE ) ;

            if( _businessType == businessType )
            {
               break ;
            }
         }

         if ( iter == businessTypeList.end() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid %s: type=%s",
                                OM_XML_FIELD_BUSINESS_TYPE,
                                _businessType.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      {
         list<BSONObj> templateList ;
         list<BSONObj>::iterator iter ;

         rc =  cfgTool.readBuzTemplate( _businessType, OM_FIELD_OPERATION_DEPLOY,
                                        templateList ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         for ( iter = templateList.begin(); iter != templateList.end(); ++iter )
         {
            string deployMod = iter->getStringField( OM_XML_FIELD_DEPLOY_MOD ) ;

            if ( deployMod == _deployMod )
            {
               break ;
            }
         }

         if ( iter == templateList.end() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid %s: mode=%s",
                                OM_XML_FIELD_DEPLOY_MOD, _deployMod.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_getBuzInfoOfCluster( BSONObj &buzInfoOfCluster )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder builder ;
      list<BSONObj> buzInfoList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getBusinessInfoOfCluster( _clusterName, buzInfoList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info of cluster: "
                                   "cluster=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = buzInfoList.begin(); iter != buzInfoList.end(); ++iter )
      {
         builder.append( *iter ) ;
      }

      buzInfoOfCluster = BSON( OM_BSON_BUSINESS_INFO << builder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_getHostInfoOfCluster(
                                                   BSONObj &hostsInfoOfCluster )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder builder ;
      list<BSONObj> hostList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getHostInfoByCluster( _clusterName, hostList ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to get host info:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == hostList.size() )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         _errorMsg.setError( TRUE, "failed to get host info: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = hostList.begin(); iter != hostList.end(); ++iter )
      {
         string hostName ;
         BSONObjBuilder hostBuilder ;
         BSONObj hostConfig ;
         BSONObj hostInfo = *iter ;

         hostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
         rc = dbTool.getConfigByHostName( hostName, hostConfig ) ;
         if( rc )
         {
            _errorMsg.setError( TRUE, "Failed to get host config, rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
         hostBuilder.appendElements( hostInfo ) ;
         hostBuilder.appendElements( hostConfig ) ;

         builder.append( hostBuilder.obj() ) ;
      }

      hostsInfoOfCluster = BSON( OM_BSON_HOST_INFO << builder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_checkConfig( BSONObj &deployConfig )
   {
      INT32 rc = SDB_OK ;
      OmConfigBuilder* confBuilder = NULL ;
      BSONObj buzTemplate ;
      BSONObj hostListOfCluster ;
      BSONObj buzListOfCluster ;
      omConfigTool cfgTool( _rootPath, _languageFileSep ) ;

      {
         OmBusinessInfo businessInfo ;
         string operationType = OM_FIELD_OPERATION_DEPLOY ;

         businessInfo.clusterName  = _clusterName ;
         businessInfo.businessType = _businessType ;
         businessInfo.businessName = _businessName ;
         businessInfo.deployMode   = _deployMod ;

         rc = OmConfigBuilder::createInstance( businessInfo, operationType,
                                               confBuilder ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
            PD_LOG( PDERROR, "failed to create builder: rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = cfgTool.readBuzConfig( _businessType, _deployMod, FALSE,
                                  buzTemplate ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _getHostInfoOfCluster( hostListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get hosts info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      rc = _getBuzInfoOfCluster( buzListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get business info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      rc = confBuilder->checkConfig( buzTemplate, hostListOfCluster,
                                     buzListOfCluster, deployConfig,
                                     _force ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, confBuilder->getErrorDetail().c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      if( confBuilder )
      {
         SDB_OSS_DEL( confBuilder ) ;
         confBuilder = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_check( BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;

      rc = _checkRestInfo( configInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check rest info: rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == dbTool.isBusinessExist( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business already exist,name:%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _businessName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task["OSS_LL_PRINT_FORMAT"]",
                             _businessName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _checkTemplate() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check template: rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkConfig( configInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check config: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_generateRequest( const BSONObj &configInfo,
                                                 BSONObj &taskConfig,
                                                 BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string sdbUser ;
      string sdbPasswd ;
      string sdbGroup ;
      BSONObjBuilder taskConfigBuilder ;
      BSONArrayBuilder resultInfoBuilder ;
      BSONObj condition ;
      BSONObj clusterInfo ;
      omDatabaseTool dbTool( _cb ) ;

      if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         condition = BSON( OM_BSON_HOSTNAME << "" <<
                           OM_BSON_SVCNAME  << "" <<
                           OM_BSON_ROLE     << "" <<
                           OM_BSON_DATAGROUPNAME << "" ) ;
      }
      else if ( OM_BUSINESS_ZOOKEEPER == _businessType )
      {
         condition = BSON( OM_BSON_HOSTNAME << "" <<
                           OM_BSON_ZOOID << "" ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_OLAP == _businessType )
      {
         condition = BSON( OM_BSON_HOSTNAME << "" <<
                           OM_BSON_ROLE << "" ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_OLTP == _businessType )
      {
         condition = BSON( OM_BSON_HOSTNAME << "" <<
                           OM_BSON_PORT << "" ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, _businessType.c_str() ) ;
      }

      rc = dbTool.getClusterInfo( _clusterName, clusterInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get cluster info: name=%s, rc=%d",
                             _clusterName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      sdbUser   = clusterInfo.getStringField( OM_CLUSTER_FIELD_SDBUSER ) ;
      sdbPasswd = clusterInfo.getStringField( OM_CLUSTER_FIELD_SDBPASSWD ) ;
      sdbGroup  = clusterInfo.getStringField( OM_CLUSTER_FIELD_SDBUSERGROUP ) ;

      {
         BSONArrayBuilder nodesConfigBuilder ;
         BSONObj nodesConfig = configInfo.getObjectField( OM_BSON_CONFIG ) ;
         BSONObjIterator iter( nodesConfig ) ;

         while ( iter.more() )
         {
            BOOLEAN hasPackage = FALSE ;
            BSONObjBuilder newNodeConfigBuilder ;
            BSONObjBuilder resultEleBuilder ;
            BSONElement ele = iter.next() ;
            BSONObj hostInfo ;
            BSONObj tmpResultEle ;
            BSONObj oneNodeConfig = ele.embeddedObject() ;
            string hostName = oneNodeConfig.getStringField( OM_BSON_HOSTNAME ) ;
            string installPath ;

            rc = dbTool.getHostInfoByAddress( hostName, hostInfo ) ;
            if ( rc )
            {
               _errorMsg.setError( TRUE, "failed to get host info: "
                                         "name=%s, rc=%d",
                                   hostName.c_str(), rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               BSONObj packages = hostInfo.getObjectField(
                                                      OM_HOST_FIELD_PACKAGES ) ;
               BSONObjIterator pkgIter( packages ) ;

               while ( pkgIter.more() )
               {
                  BSONElement ele = pkgIter.next() ;
                  BSONObj packageInfo = ele.embeddedObject() ;

                  if ( _businessType == packageInfo.getStringField(
                                                   OM_HOST_FIELD_PACKAGENAME ) )
                  {
                     installPath = packageInfo.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;
                     hasPackage = TRUE ;
                     break ;
                  }
               }
            }

            if ( FALSE == hasPackage )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "package does not exist: "
                                         "host=%s, package=%s, rc=%d",
                                   hostName.c_str(), _businessType.c_str(),
                                    rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            newNodeConfigBuilder.appendElements( oneNodeConfig ) ;
            if ( OM_BUSINESS_SEQUOIADB == _businessType )
            {
               string user    = hostInfo.getStringField( OM_HOST_FIELD_USER ) ;
               string passwd  = hostInfo.getStringField(
                                                      OM_HOST_FIELD_PASSWORD ) ;
               string sshPort = hostInfo.getStringField(
                                                      OM_HOST_FIELD_SSHPORT ) ;

               newNodeConfigBuilder.append( OM_TASKINFO_FIELD_USER, user ) ;
               newNodeConfigBuilder.append( OM_TASKINFO_FIELD_PASSWD, passwd ) ;
               newNodeConfigBuilder.append( OM_TASKINFO_FIELD_SSHPORT,
                                            sshPort ) ;
            }
            newNodeConfigBuilder.append( OM_TASKINFO_FIELD_INSTALLPATH,
                                         installPath ) ;

            nodesConfigBuilder.append( newNodeConfigBuilder.obj() ) ;

            tmpResultEle = oneNodeConfig.filterFieldsUndotted( condition,
                                                               TRUE ) ;

            resultEleBuilder.appendElements( tmpResultEle ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS,
                                     OM_TASK_STATUS_INIT ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                                     getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
            resultEleBuilder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
            resultEleBuilder.append( OM_REST_RES_DETAIL, "" ) ;
            {
               BSONArrayBuilder tmpEmptyBuilder ;

               resultEleBuilder.append( OM_TASKINFO_FIELD_FLOW,
                                        tmpEmptyBuilder.arr() ) ;
            }

            resultInfoBuilder.append( resultEleBuilder.obj() ) ;

         }

         taskConfig = BSON(
               OM_TASKINFO_FIELD_SDBUSER       << sdbUser       <<
               OM_TASKINFO_FIELD_SDBPASSWD     << sdbPasswd     <<
               OM_TASKINFO_FIELD_SDBUSERGROUP  << sdbGroup      <<
               OM_TASKINFO_FIELD_CLUSTERNAME   << _clusterName  <<
               OM_TASKINFO_FIELD_BUSINESS_TYPE << _businessType <<
               OM_TASKINFO_FIELD_BUSINESS_NAME << _businessName <<
               OM_TASKINFO_FIELD_DEPLOY_MOD    << _deployMod    <<
               OM_TASKINFO_FIELD_CONFIG        << nodesConfigBuilder.arr() ) ;

         resultInfo = resultInfoBuilder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessCommand::_createTask( const BSONObj &taskConfig,
                                            const BSONArray &resultInfo,
                                            INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      getMaxTaskID( taskID ) ;
      ++taskID ;

      rc = taskTool.createTask( OM_TASK_TYPE_ADD_BUSINESS, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_ADD_BUSINESS ),
                                taskConfig, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "fail to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "fail to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   omListTaskCommand::omListTaskCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession )
                     :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omListTaskCommand::~omListTaskCommand()
   {
   }

   INT32 omListTaskCommand::_getTaskList( list<BSONObj> &taskList )
   {
      INT32 rc = SDB_OK ;
      SINT64 numToSkip   = 0 ;
      SINT64 numToReturn = -1 ;
      SINT64 contextID   = -1 ;
      BSONObj filter ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj innerSelector ;

      rc = _getQueryPara( selector, filter, order, hint, numToSkip,
                          numToReturn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getQueryPara failed:rc=%d", rc ) ;
         goto error ;
      }

      innerSelector = BSON( OM_TASKINFO_FIELD_TASKID << 1
                            << OM_TASKINFO_FIELD_TYPE << 1
                            << OM_TASKINFO_FIELD_TYPE_DESC << 1
                            << OM_TASKINFO_FIELD_NAME << 1
                            << OM_TASKINFO_FIELD_STATUS << 1
                            << OM_TASKINFO_FIELD_STATUS_DESC << 1 ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, innerSelector, filter, order,
                     hint, 0, _cb, numToSkip, numToReturn,
                     _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to query table[%s],rc=%d",
                     OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
         goto error ;
      }

      {
         BSONObj empty ;
         omContextAssist contextAssistor( contextID, _cb, _pRTNCB ) ;
         rc = contextAssistor.init( selector, empty, 0, -1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "initial context failed:rc=%d", rc ) ;
            goto error ;
         }

         while ( TRUE )
         {
            BSONObj data ;
            rc = contextAssistor.getNext( data ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }

               PD_LOG_MSG( PDERROR, "failed to get record from table[%s],rc=%d",
                           OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
               goto error ;
            }

            taskList.push_back( data.copy() ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omListTaskCommand::_sendTaskList2Web( list<BSONObj> &taskList )
   {
      BSONObjBuilder opBuilder ;
      list<BSONObj>::iterator iter = taskList.begin() ;
      while ( iter != taskList.end() )
      {
         _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omListTaskCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> taskList ;
      rc = _getTaskList( taskList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task list failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendTaskList2Web( taskList ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omQueryTaskCommand::omQueryTaskCommand( restAdaptor *pRestAdaptor,
                                           pmdRestSession *pRestSession,
                                           const string &localAgentHost,
                                           const string &localAgentService )
                      :omScanHostCommand( pRestAdaptor, pRestSession,
                                          localAgentHost, localAgentService)
   {
   }

   omQueryTaskCommand::~omQueryTaskCommand()
   {
   }

   void omQueryTaskCommand::_modifyTaskInfo( BSONObj &task )
   {
      string clusterName ;
      string businessType ;
      string businessName ;
      string deployMod ;
      BSONObjBuilder resultBuilder ;
      BSONObjBuilder newTaskInfo ;
      BSONObj tmp ;
      BSONObj taskInfo ;
      BSONObj filter = BSON( OM_TASKINFO_FIELD_INFO << "" ) ;

      taskInfo = task.getObjectField( OM_TASKINFO_FIELD_INFO ) ;

      clusterName  = taskInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      businessType = taskInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      businessName = taskInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      deployMod    = taskInfo.getStringField( OM_BSON_DEPLOY_MOD ) ;

      newTaskInfo.append( OM_BSON_CLUSTER_NAME, clusterName ) ;

      if ( 0 < businessName.length() )
      {
         newTaskInfo.append( OM_BSON_BUSINESS_NAME, businessName ) ;
      }

      if ( 0 < businessType.length() )
      {
         newTaskInfo.append( OM_BSON_BUSINESS_TYPE, businessType ) ;
         newTaskInfo.append( OM_BSON_DEPLOY_MOD   , deployMod ) ;
      }

      tmp = task.filterFieldsUndotted( filter, false ) ;

      resultBuilder.appendElements( tmp ) ;

      if ( task.hasField( OM_TASKINFO_FIELD_INFO ) )
      {
         resultBuilder.append( OM_TASKINFO_FIELD_INFO, newTaskInfo.obj() ) ;
      }

      task = resultBuilder.obj() ;
   }

   void omQueryTaskCommand::_sendOneTaskInfo2Web( BSONObj &oneTask )
   {
      _modifyTaskInfo( oneTask ) ;
      _restAdaptor->appendHttpBody( _restSession, oneTask.objdata(),
                                    oneTask.objsize(), 1 ) ;
      _sendOKRes2Web() ;

      return ;
   }

   void omQueryTaskCommand::_sendTaskInfo2Web( list<BSONObj> &tasks )
   {
      list<BSONObj>::iterator iter = tasks.begin() ;
      while ( iter != tasks.end() )
      {
         BSONObj result = *iter ;
         _modifyTaskInfo( result ) ;
         _restAdaptor->appendHttpBody( _restSession, result.objdata(),
                                       result.objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omQueryTaskCommand::_ssqlGetMore( INT64 taskID, INT32 &flag,
                                           BSONObj &result )
   {
      INT32 rc          = SDB_OK ;
      CHAR *pContent    = NULL ;
      INT32 contentSize = 0 ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      bsonRequest = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_SSQL_GET_MORE_REQ,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build msg failed:cmd=%s,rc=%d", OM_NOTIFY_TASK,
                     rc ) ;
         goto error ;
      }

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_MSG_TIMEOUT_TWO_HOUR,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "create remote session failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      rc = _receiveFromAgent( remoteSession, flag, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omQueryTaskCommand::_updateSsqlTask( INT64 taskID,
                                              const BSONObj &taskInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      builder.appendElements( taskInfo ) ;
      time_t now = time( NULL ) ;
      builder.appendTimestamp( OM_TASKINFO_FIELD_END_TIME, now * 1000, 0 ) ;
      BSONObj selector = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      BSONObj updator  = BSON( "$set" << builder.obj() ) ;
      BSONObj hint ;
      INT64 updateNum = 0 ;
      rc = rtnUpdate( OM_CS_DEPLOY_CL_TASKINFO, selector, updator, hint, 0,
                      pmdGetThreadEDUCB(), &updateNum ) ;
      if ( SDB_OK != rc || 0 == updateNum )
      {
         PD_LOG( PDERROR, "update task failed:table=%s,updateNum=%d,taskID="
                 OSS_LL_PRINT_FORMAT",updator=%s,selector=%s,rc=%d",
                 OM_CS_DEPLOY_CL_TASKINFO, updateNum, taskID,
                 updator.toString().c_str(), selector.toString().c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omQueryTaskCommand::_getSsqlResult( BSONObj &oneTask )
   {
      INT32 rc   = SDB_OK ;
      INT32 flag = SDB_OK ;
      BSONObj result ;
      BSONElement taskEle ;
      INT64 taskID ;
      INT32 status ;

      status = oneTask.getIntField( OM_TASKINFO_FIELD_STATUS ) ;
      if ( OM_TASK_STATUS_FINISH == status || OM_TASK_STATUS_CANCEL == status ||
           OM_TASK_STATUS_INIT == status )
      {
         goto done ;
      }

      taskEle = oneTask.getField( OM_TASKINFO_FIELD_TASKID ) ;
      taskID  = taskEle.Long() ;
      rc = _ssqlGetMore( taskID, flag, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "ssql get more failed:taskID="OSS_LL_PRINT_FORMAT
                 "rc=%d", taskID, rc ) ;
         goto error ;
      }

      if ( flag != SDB_OK )
      {
         BSONObj taskInfo ;
         rc = flag ;
         string errorDetail = result.getStringField(
                                                 OM_TASKINFO_FIELD_DETAIL ) ;
         taskInfo = BSON( OM_TASKINFO_FIELD_STATUS << OM_TASK_STATUS_FINISH
                          << OM_TASKINFO_FIELD_STATUS_DESC
                          << OM_TASK_STATUS_FINISH_STR
                          << OM_TASKINFO_FIELD_ERRNO << rc
                          << OM_TASKINFO_FIELD_DETAIL << errorDetail
                          << OM_TASKINFO_FIELD_PROGRESS << 100 ) ;
         _updateSsqlTask( taskID, taskInfo ) ;
         PD_LOG_MSG( PDERROR, "ssql get more failed:taskID="OSS_LL_PRINT_FORMAT
                     ",rc=%d,detail=%s", taskID, flag, errorDetail.c_str() ) ;
         goto error ;
      }

      {
         INT32 progress = 0 ;
         BSONObj taskInfo ;
         BSONObjBuilder builder ;
         BSONElement resultElement ;
         resultElement = result.getField( OM_TASKINFO_FIELD_RESULTINFO ) ;
         status = result.getIntField( OM_TASKINFO_FIELD_STATUS ) ;
         if ( OM_TASK_STATUS_FINISH == status )
         {
            progress = 100 ;
         }

         builder.append( OM_TASKINFO_FIELD_STATUS, status ) ;
         builder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                         getTaskStatusStr( status ) ) ;
         builder.append( OM_TASKINFO_FIELD_PROGRESS, progress ) ;
         builder.append( resultElement ) ;
         BSONObj tmp = builder.obj() ;
         rc = _updateSsqlTask( taskID, tmp ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "update ssql's task info failed:task="
                        OSS_LL_PRINT_FORMAT, taskID ) ;
            goto error ;
         }
      }

      {
         BSONObj selector ;
         BSONObj matcher = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
         BSONObj order ;
         BSONObj hint ;
         list<BSONObj> tasks ;
         rc = _queryTable( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order,
                           hint, 0, 0, -1, tasks ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "query table failed:task="OSS_LL_PRINT_FORMAT
                    ",rc=%d", taskID, rc ) ;
            goto error ;
         }

         if ( tasks.size() != 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "query task failed:task="OSS_LL_PRINT_FORMAT
                    ",rc=%d", taskID, rc ) ;
            goto error ;
         }

         oneTask = ( *tasks.begin() ).getOwned() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omQueryTaskCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      list<BSONObj> tasks ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order,
                        hint, 0, skip, returnNum, tasks ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( tasks.size() == 1 )
      {
         INT32 taskType ;
         BSONObj oneTask = ( *tasks.begin() ).getOwned() ;

         taskType = oneTask.getIntField( OM_TASKINFO_FIELD_TYPE ) ;
         if ( OM_TASK_TYPE_SSQL_EXEC == taskType )
         {
            rc = _getSsqlResult( oneTask ) ;
            if ( SDB_OK != rc )
            {
               _errorDetail =  omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
               PD_LOG( PDERROR, "_getSsqlResult failed:rc=%d", rc ) ;
               _sendErrorRes2Web( rc, _errorDetail ) ;
               goto error ;
            }
            else
            {
               _sendOneTaskInfo2Web( oneTask ) ;
               goto done ;
            }
         }
      }

      _sendTaskInfo2Web( tasks ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omListNodeCommand::omListNodeCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession )
                     :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omListNodeCommand::~omListNodeCommand()
   {
   }

   INT32 omListNodeCommand::_getNodeList( string businessName,
                                          list<simpleNodeInfo> &nodeList )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID = -1 ;

      matcher  = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         _errorDetail = string( "fail to query table:" )
                        + OM_CS_DEPLOY_CL_CONFIGURE ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            _errorDetail = string( "failed to get record from table:" )
                           + OM_CS_DEPLOY_CL_CONFIGURE ;
            PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         BSONObj confs ;
         confs    = record.getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;
         {
            BSONObjIterator iterBson( confs ) ;
            while ( iterBson.more() )
            {
               BSONObjBuilder builder ;
               BSONElement ele = iterBson.next() ;
               BSONObj oneNode = ele.embeddedObject() ;
               simpleNodeInfo nodeInfo ;
               nodeInfo.hostName = record.getStringField(
                                                 OM_CONFIGURE_FIELD_HOSTNAME ) ;
               nodeInfo.role     = oneNode.getStringField(
                                                 OM_CONF_DETAIL_ROLE ) ;
               nodeInfo.svcName  = oneNode.getStringField(
                                                 OM_CONF_DETAIL_SVCNAME ) ;
               nodeList.push_back( nodeInfo ) ;
            }
         }
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void omListNodeCommand::_sendNodeList2Web( list<simpleNodeInfo> &nodeList )
   {
      BSONObjBuilder opBuilder ;
      list<simpleNodeInfo>::iterator iter = nodeList.begin() ;
      while ( iter != nodeList.end() )
      {
         BSONObj node ;
         node = BSON( OM_BSON_FIELD_HOST_NAME << (*iter).hostName
                      << OM_BSON_FIELD_ROLE << (*iter).role
                      << OM_BSON_FIELD_SVCNAME << (*iter).svcName ) ;
         _restAdaptor->appendHttpBody( _restSession, node.objdata(),
                                       node.objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omListNodeCommand::doCommand()
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *businessName  = NULL ;
      list<simpleNodeInfo> nodeList ;

      _restAdaptor->getQuery( _restSession, OM_REST_BUSINESS_NAME,
                              &businessName ) ;
      if ( NULL == businessName )
      {
         _errorDetail = "rest field miss:" + string( OM_REST_BUSINESS_NAME ) ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _getNodeList( businessName, nodeList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getNodeList failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendNodeList2Web( nodeList ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omGetNodeConfCommand::omGetNodeConfCommand( restAdaptor *pRestAdaptor,
                                               pmdRestSession *pRestSession )
                          :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omGetNodeConfCommand::~omGetNodeConfCommand()
   {
   }

   void omGetNodeConfCommand::_expandNodeInfo( BSONObj &oneConfig,
                                               const string &svcName,
                                               BSONObj &nodeinfo )
   {
      string hostName ;
      string business ;
      BSONObj confs ;
      hostName = oneConfig.getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
      business = oneConfig.getStringField( OM_CONFIGURE_FIELD_BUSINESSNAME ) ;
      confs    = oneConfig.getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;
      {
         BSONObjIterator iterBson( confs ) ;
         while ( iterBson.more() )
         {
            BSONObjBuilder builder ;
            BSONElement ele = iterBson.next() ;
            BSONObj oneNode = ele.embeddedObject() ;

            if ( oneNode.getStringField( OM_CONF_DETAIL_SVCNAME ) == svcName )
            {
               builder.append( OM_BSON_BUSINESS_NAME, business ) ;
               builder.append( OM_BSON_FIELD_HOST_NAME, hostName ) ;
               builder.appendElements( oneNode ) ;
               nodeinfo = builder.obj() ;
               break ;
            }
         }
      }
   }

   INT32 omGetNodeConfCommand::_getNodeInfo( const string &hostName,
                                             const string &svcName,
                                             BSONObj &nodeinfo )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      BSONObj tmpSvcName = BSON( OM_CONF_DETAIL_SVCNAME << svcName ) ;
      BSONObj elemMatch  = BSON( "$elemMatch" << tmpSvcName ) ;
      matcher = BSON( OM_CONFIGURE_FIELD_HOSTNAME << hostName
                      << OM_CONFIGURE_FIELD_CONFIG << elemMatch ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         _errorDetail = string( "fail to query table:" )
                        + OM_CS_DEPLOY_CL_CONFIGURE ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            _errorDetail = string( "failed to get record from table:" )
                           + OM_CS_DEPLOY_CL_CONFIGURE ;
            PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         _expandNodeInfo( result, svcName, nodeinfo ) ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void omGetNodeConfCommand::_sendNodeInfo2Web( BSONObj &nodeInfo )
   {
      if ( !nodeInfo.isEmpty() )
      {
          _restAdaptor->appendHttpBody( _restSession, nodeInfo.objdata(),
                                        nodeInfo.objsize(), 1 ) ;
      }
      _sendOKRes2Web() ;
      return ;
   }

   INT32 omGetNodeConfCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR* pHostName = NULL ;
      const CHAR* pSvcName  = NULL ;
      BSONObj nodeInfo ;

      _restAdaptor->getQuery( _restSession, OM_REST_HOST_NAME, &pHostName ) ;
      _restAdaptor->getQuery( _restSession, OM_REST_SVCNAME, &pSvcName ) ;
      if ( NULL == pHostName || NULL == pSvcName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field miss:field=%s or field=%s",
                     OM_REST_HOST_NAME, OM_REST_SVCNAME ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _getNodeInfo( pHostName, pSvcName, nodeInfo ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "get node info failed:rc=", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendNodeInfo2Web( nodeInfo ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omQueryNodeConfCommand::omQueryNodeConfCommand( restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                  :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omQueryNodeConfCommand::~omQueryNodeConfCommand()
   {
   }

   INT32 omQueryNodeConfCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      BSONObj matcher ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      list<BSONObj> nodes ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, "%s, rc=%d", _errorMsg.getError(), rc ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order,
                        hint, 0, skip, returnNum, nodes ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, "%s, rc=%d", _errorMsg.getError(), rc ) ;
         goto error ;
      }

      restTool.sendRecord2Web( nodes ) ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }


   omListBusinessCommand::omListBusinessCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                         :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omListBusinessCommand::~omListBusinessCommand()
   {
   }

   INT32 omListBusinessCommand::doCommand()
   {
      list<BSONObj> businessList ;
      INT32 rc = SDB_OK ;
      SINT64 numToSkip   = 0 ;
      SINT64 numToReturn = -1 ;
      SINT64 contextID   = -1 ;
      BSONObj filter ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj innerSelector ;

      rc = _getQueryPara( selector, filter, order, hint, numToSkip,
                          numToReturn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getQueryPara failed:rc=%d", rc ) ;
         goto error ;
      }

      innerSelector = BSON( OM_BUSINESS_FIELD_NAME << 1
                            << OM_BUSINESS_FIELD_TYPE << 1
                            << OM_BUSINESS_FIELD_CLUSTERNAME << 1
                            << OM_BUSINESS_FIELD_LOCATION << 1 ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, innerSelector, filter, order,
                     hint, 0, _cb, numToSkip, numToReturn,
                     _pDMSCB, _pRTNCB, contextID );
      PD_RC_CHECK( rc, PDERROR, "failed to query table[%s],rc=%d",
                   OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
      {
         BSONObj empty ;
         omContextAssist contextAssistor( contextID, _cb, _pRTNCB ) ;
         rc = contextAssistor.init( selector, empty, 0, -1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "initial context failed:rc=%d", rc ) ;
            goto error ;
         }

         while ( TRUE )
         {
            BSONObj data ;
            rc = contextAssistor.getNext( data ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }

               PD_LOG_MSG( PDERROR, "failed to get record from table[%s],rc=%d",
                           OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
               goto error ;
            }

            businessList.push_back( data.copy() ) ;
         }
      }

      _sendBusinessList2Web( businessList ) ;
   done:
      return rc ;
   error:
       PD_LOG( PDERROR, "list business failed:rc=%d", rc ) ;
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      _sendErrorRes2Web( rc, _errorDetail ) ;
      goto done ;
   }

   void omListBusinessCommand::_sendBusinessList2Web(
                                                   list<BSONObj> &businessList )
   {
      BSONObjBuilder opBuilder ;
      list<BSONObj>::iterator iter = businessList.begin() ;
      while ( iter != businessList.end() )
      {
         _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   omListHostBusinessCommand::omListHostBusinessCommand(
                                                 restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession )
                             :omAuthCommand( pRestAdaptor, pRestSession )
   {

   }

   omListHostBusinessCommand::~omListHostBusinessCommand()
   {
   }

   INT32 omListHostBusinessCommand::doCommand()
   {
      list<BSONObj> businessList ;
      INT32 rc = SDB_OK ;
      SINT64 numToSkip   = 0 ;
      SINT64 numToReturn = -1 ;
      SINT64 contextID   = -1 ;
      BSONObj filter ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj innerSelector ;

      rc = _getQueryPara( selector, filter, order, hint, numToSkip,
                          numToReturn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getQueryPara failed:rc=%d", rc ) ;
         goto error ;
      }

      innerSelector = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << 1
                            << OM_CONFIGURE_FIELD_BUSINESSTYPE << 1
                            << OM_CONFIGURE_FIELD_CLUSTERNAME << 1
                            << OM_CONFIGURE_FIELD_HOSTNAME << 1 ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, innerSelector, filter, order,
                     hint, 0, _cb, numToSkip, numToReturn,
                     _pDMSCB, _pRTNCB, contextID );
      PD_RC_CHECK( rc, PDERROR, "failed to query table[%s],rc=%d",
                   OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
      {
         BSONObj empty ;
         omContextAssist contextAssistor( contextID, _cb, _pRTNCB ) ;
         rc = contextAssistor.init( selector, empty, 0, -1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "initial context failed:rc=%d", rc ) ;
            goto error ;
         }

         while ( TRUE )
         {
            BSONObj data ;
            rc = contextAssistor.getNext( data ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }

               PD_LOG_MSG( PDERROR, "failed to get record from table[%s],rc=%d",
                           OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
               goto error ;
            }

            businessList.push_back( data.copy() ) ;
         }
      }

      _sendHostBusiness2Web( businessList ) ;
   done:
      return rc ;
   error:
       PD_LOG( PDERROR, "list business failed:rc=%d", rc ) ;
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      _sendErrorRes2Web( rc, _errorDetail ) ;
      goto done ;
   }

   void omListHostBusinessCommand::_sendHostBusiness2Web(
                                                  list<BSONObj> &businessList )
   {
      BSONObjBuilder opBuilder ;
      list<BSONObj>::iterator iter = businessList.begin() ;
      while ( iter != businessList.end() )
      {
         _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   omQueryBusinessCommand::omQueryBusinessCommand( restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                          :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omQueryBusinessCommand::~omQueryBusinessCommand()
   {
   }

   void omQueryBusinessCommand::_sendBusinessInfo2Web(
                                                  list<BSONObj> &businessInfo )
   {
      list<BSONObj>::iterator iter = businessInfo.begin() ;
      while ( iter != businessInfo.end() )
      {
         _restAdaptor->appendHttpBody( _restSession, iter->objdata(),
                                       iter->objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omQueryBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      list<BSONObj> businessInfo ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get query parameter failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order,
                        hint, 0, skip, returnNum, businessInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _sendBusinessInfo2Web( businessInfo ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   omStartBusinessCommand::omStartBusinessCommand( restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   string localAgentHost,
                                                   string localAgentService )
                          :omScanHostCommand( pRestAdaptor, pRestSession,
                                             localAgentHost, localAgentService )
   {
   }

   omStartBusinessCommand::~omStartBusinessCommand()
   {
   }

   INT32 omStartBusinessCommand::_getNodeInfo( const string &businessName,
                                               BSONObj &nodeInfos,
                                               BOOLEAN &isExistFlag )
   {
      BSONArrayBuilder arrayBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      isExistFlag      = FALSE ;

      matcher = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         _errorDetail = string( "fail to query table:" )
                        + OM_CS_DEPLOY_CL_CONFIGURE ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         string hostName ;
         BSONObjBuilder builder ;
         rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc          = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            _errorDetail = string( "failed to get record from table:" )
                           + OM_CS_DEPLOY_CL_CONFIGURE ;
            PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
            goto error ;
         }

         isExistFlag = TRUE ;
         BSONObj result( buffObj.data() ) ;
         rc = _expandNodeInfoToBuilder( result, arrayBuilder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_expandNodeInfoToBuilder failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      nodeInfos = BSON( OM_BSON_FIELD_CONFIG << arrayBuilder.arr() ) ;
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
     record:
     {
       BusinessName:"b1", Hostname:"h1"
       Config:
       [
         {
           dbpath:"", role:"", logfilesz:"", ...
         }, ...
       ]
     }

     this function transfer record to a BSON like below, and add to arrayBuilder

     {
       HostName:"", User:"", Passwd:"", dbpath:"", role:"", logfilesz:"", ...
     }
   */
   INT32 omStartBusinessCommand::_expandNodeInfoToBuilder(
                                                const BSONObj &record,
                                                BSONArrayBuilder &arrayBuilder )
   {
      INT32 rc = SDB_OK ;
      string businessName ;
      string businessType ;
      string deployMode ;
      string hostName ;
      simpleHostInfo hostInfo ;
      BOOLEAN isHostExist = FALSE ;
      BSONObj confs ;
      hostName = record.getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
      rc = _getHostInfo( hostName, hostInfo, isHostExist ) ;
      if ( SDB_OK != rc || !isHostExist )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "get host info failed:host=%s,rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

      businessName = record.getStringField( OM_CONFIGURE_FIELD_BUSINESSNAME ) ;
      rc = _getBusinessType( businessName, businessType, deployMode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get businessType failed:business=%s,rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      confs = record.getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;
      {
         CHAR catName[ OM_INT32_LENGTH + 1 ] = "" ;
         BSONObjIterator iter( confs ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "config info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_CONFIGURE_FIELD_CONFIG, ele.type() ) ;
               _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
               goto error ;
            }
            BSONObj tmp = ele.embeddedObject() ;
            BSONObjBuilder builder ;
            builder.appendElements( tmp ) ;
            if ( businessType == OM_BUSINESS_SEQUOIADB )
            {
               string svcName = tmp.getStringField( OM_CONF_DETAIL_SVCNAME ) ;
               INT32 iSvcName = ossAtoi( svcName.c_str() ) ;
               INT32 iCatName = iSvcName + MSG_ROUTE_CAT_SERVICE ;
               ossItoa( iCatName, catName, OM_INT32_LENGTH ) ;
               builder.append( OM_CONF_DETAIL_CATANAME, catName ) ;
            }
            builder.append( OM_BSON_FIELD_HOST_NAME, hostInfo.hostName ) ;
            builder.append( OM_BSON_FIELD_HOST_USER, hostInfo.user ) ;
            builder.append( OM_BSON_FIELD_HOST_PASSWD, hostInfo.passwd ) ;
            builder.append( OM_BSON_FIELD_HOST_SSHPORT, hostInfo.sshPort ) ;
            if ( businessType == OM_BUSINESS_ZOOKEEPER )
            {
               builder.append( OM_BSON_FIELD_HOST_SSHPORT, hostInfo.sshPort ) ;
            }

            BSONObj oneNode = builder.obj() ;
            arrayBuilder.append( oneNode ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStartBusinessCommand::_getHostInfo( const string &hostName,
                                               simpleHostInfo &hostInfo,
                                               BOOLEAN &isExistFlag )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc          = SDB_OK ;
               isExistFlag = FALSE ;
               break ;
            }

            contextID = -1 ;
            _errorDetail = string( "failed to get record from table:" )
                           + OM_CS_DEPLOY_CL_HOST ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_HOST, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         hostInfo.hostName    = result.getStringField( OM_HOST_FIELD_NAME ) ;
         hostInfo.clusterName = result.getStringField(
                                                   OM_HOST_FIELD_CLUSTERNAME ) ;
         hostInfo.ip          = result.getStringField( OM_HOST_FIELD_IP ) ;
         hostInfo.user        = result.getStringField( OM_HOST_FIELD_USER ) ;
         hostInfo.passwd      = result.getStringField(
                                                   OM_HOST_FIELD_PASSWORD ) ;
         hostInfo.installPath = result.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;
         hostInfo.agentPort   = result.getStringField(
                                                   OM_HOST_FIELD_AGENT_PORT ) ;
         hostInfo.sshPort     = result.getStringField(
                                                   OM_HOST_FIELD_SSHPORT ) ;
         isExistFlag = TRUE ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omStartBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      return rc ;
   }

   omStopBusinessCommand::omStopBusinessCommand( restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession,
                                                 string localAgentHost,
                                                 string localAgentService )
                         :omScanHostCommand( pRestAdaptor, pRestSession,
                                             localAgentHost, localAgentService )
   {
   }

   omStopBusinessCommand::~omStopBusinessCommand()
   {
   }

   INT32 omStopBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      return rc ;
   }

   omRemoveClusterCommand::omRemoveClusterCommand( restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                          :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omRemoveClusterCommand::~omRemoveClusterCommand()
   {
   }

   INT32 omRemoveClusterCommand::_getClusterExistFlag(
                                                      const string &clusterName,
                                                      BOOLEAN &flag )
   {
      INT32 rc = SDB_OK ;
      BSONObj clusterInfo ;
      rc = _getClusterInfo( clusterName, clusterInfo ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc   = SDB_OK ;
            flag = false ;
            pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;
            goto done ;
         }

         PD_LOG( PDERROR, "get clusterInfo failed:cluster=%s,rc=%d",
                 clusterName.c_str(), rc ) ;
         goto error ;
      }

      flag = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveClusterCommand::_getClusterExistHostFlag(
                                                      const string &clusterName,
                                                      BOOLEAN &flag )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      matcher = BSON( OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:table=%s,rc=%d",
                     OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               flag = FALSE ;
               break ;
            }

            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:table=%s,"
                        "rc=%d", OM_CS_DEPLOY_CL_HOST, rc ) ;
            goto error ;
         }

         flag = TRUE ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveClusterCommand::_removeCluster( const string &clusterName )
   {
      INT32 rc          = SDB_OK ;
      BSONObj condition = BSON( OM_CLUSTER_FIELD_NAME << clusterName ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_CLUSTER, condition, hint, 0, _cb );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to delete taskinfo from table:%s,"
                     "%s=%s,rc=%d", OM_CS_DEPLOY_CL_CLUSTER,
                     OM_CLUSTER_FIELD_NAME, clusterName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveClusterCommand::doCommand()
   {
      INT32 rc                   = SDB_OK ;
      const CHAR *clusterName    = NULL ;
      BOOLEAN isClusterExist     = FALSE ;
      BSONObj result ;
      BOOLEAN isClusterExistHost = FALSE ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CLUSTER_NAME,
                              &clusterName ) ;
      if ( NULL == clusterName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "rest field is miss:field=%s",
                     OM_REST_FIELD_CLUSTER_NAME ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _getClusterExistFlag( clusterName, isClusterExist ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "_getClusterExistFlag failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( !isClusterExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG_MSG( PDERROR, "%s is not exist:rc=%d", clusterName, rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto done ;
      }

      rc = _getClusterExistHostFlag( clusterName, isClusterExistHost ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "_getClusterExistHostFlag failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( isClusterExistHost )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host exist in cluster, host should be removed "
                     "first:cluster=%s", clusterName ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _removeCluster( clusterName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_removeCluster failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      sdbGetOMManager()->removeClusterVersion( clusterName ) ;

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omRemoveHostCommand::omRemoveHostCommand( restAdaptor *pRestAdaptor,
                                             pmdRestSession *pRestSession,
                                             string &localAgentHost,
                                             string &localAgentService )
               : omAuthCommand( pRestAdaptor, pRestSession ),
                 _localAgentHost( localAgentHost ),
                 _localAgentService( localAgentService )

   {
   }

   omRemoveHostCommand::~omRemoveHostCommand()
   {
   }

   INT32 omRemoveHostCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      string packetPath ;
      BSONObj restHostInfo ;
      BSONObj clusterInfo ;
      BSONObj hostsInfo ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "j", OM_REST_FIELD_HOST_INFO, &restHostInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( restHostInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateRequest( restHostInfo, taskConfig, resultInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate task request: rc=%d", rc ) ;
         goto error ;
      }

      rc = _createTask( taskConfig, resultInfo, taskID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to create task: rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << taskID ) ;

         rc = restTool.appendResponeContent( result ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to append respone content: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omRemoveHostCommand::_check( const BSONObj &hostList )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;
      BSONObj hostInfo = hostList.getObjectField( OM_BSON_HOST_INFO ) ;
      BSONObjIterator iter( hostInfo ) ;

      _clusterName = hostList.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: %s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      while ( iter.more() )
      {
         string hostName ;
         BSONObj hostInfo ;
         BSONElement ele = iter.next() ;

         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "element of %s is not Object type"
                        ":type=%d", OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
            goto error ;
         }

         hostInfo = ele.embeddedObject() ;
         hostName = hostInfo.getStringField( OM_BSON_HOSTNAME ) ;

         if( FALSE == dbTool.isHostExistOfCluster( hostName, _clusterName ) )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "the cluster does not have this host: "
                                      "cluster=%s, host=%s",
                                _clusterName.c_str(), hostName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         if ( TRUE == dbTool.isConfigExistOfCluster( hostName, _clusterName ) )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "failed to remove host, "
                                      "the host has business: host=%s",
                                hostName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         taskID = dbTool.getTaskIdOfRunningHost( hostName ) ;
         if ( 0 <= taskID )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "host[%s] is exist "
                                "in task["OSS_LL_PRINT_FORMAT"]",
                                hostName.c_str(), taskID ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveHostCommand::_generateRequest( const BSONObj &hostList,
                                               BSONObj &taskConfig,
                                               BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;
      BSONObjBuilder taskConfigBuilder ;
      BSONArrayBuilder resultInfoBuilder ;
      BSONObj hostInfo = hostList.getObjectField( OM_BSON_HOST_INFO ) ;
      BSONObj condition = BSON( OM_HOST_FIELD_NAME << "" <<
                                OM_HOST_FIELD_IP   << "" <<
                                OM_HOST_FIELD_CLUSTERNAME << "" <<
                                OM_HOST_FIELD_USER << "" <<
                                OM_HOST_FIELD_PASSWD << "" <<
                                OM_HOST_FIELD_SSHPORT << "" <<
                                OM_HOST_FIELD_PACKAGES << "" ) ;

      {
         BSONArrayBuilder hostInfoBuilder ;
         BSONObjIterator iter( hostInfo ) ;

         while ( iter.more() )
         {
            string hostName ;
            string ip ;
            BSONElement ele = iter.next() ;
            BSONObj tmpHostInfo = ele.embeddedObject() ;
            BSONObj hostInfo ;
            BSONObjBuilder resultEleBuilder ;

            hostName = tmpHostInfo.getStringField( OM_BSON_HOSTNAME ) ;

            rc = dbTool.getHostInfoByAddress( hostName, hostInfo ) ;
            if ( rc )
            {
               _errorMsg.setError( TRUE, "failed to get host info: host=%s",
                                   hostName.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            ip = hostInfo.getStringField( OM_HOST_FIELD_IP ) ;
            hostInfo = hostInfo.filterFieldsUndotted( condition, TRUE ) ;

            hostInfoBuilder.append( hostInfo ) ;

            resultEleBuilder.append( OM_TASKINFO_FIELD_HOSTNAME, hostName ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_IP, ip ) ;

            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS,
                                     OM_TASK_STATUS_INIT ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                                     getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
            resultEleBuilder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
            resultEleBuilder.append( OM_REST_RES_DETAIL, "" ) ;

            {
               BSONArrayBuilder tmpEmptyBuilder ;

               resultEleBuilder.append( OM_TASKINFO_FIELD_FLOW,
                                        tmpEmptyBuilder.arr() ) ;
            }

            resultInfoBuilder.append( resultEleBuilder.obj() ) ;
         }

         taskConfigBuilder.append( OM_TASKINFO_FIELD_HOSTINFO,
                                   hostInfoBuilder.arr() ) ;
      }

      taskConfig = taskConfigBuilder.obj() ;
      resultInfo = resultInfoBuilder.arr() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveHostCommand::_createTask( const BSONObj &taskConfig,
                                           const BSONArray &resultInfo,
                                           INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      getMaxTaskID( taskID ) ;
      taskID++ ;

      rc = taskTool.createTask( OM_TASK_TYPE_REMOVE_HOST, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_REMOVE_HOST ),
                                taskConfig, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "fail to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "fail to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omQueryHostStatusCommand::omQueryHostStatusCommand(
                                                   restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   string localAgentHost,
                                                   string localAgentService )
                            :omStartBusinessCommand( pRestAdaptor, pRestSession,
                                                    localAgentHost,
                                                    localAgentService )
   {
   }

   omQueryHostStatusCommand::~omQueryHostStatusCommand()
   {
   }

   INT32 omQueryHostStatusCommand::_getRestHostList(
                                                    list<string> &hostNameList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pHostInfo = NULL ;
      BSONObj hostInfo ;
      BSONObj hostInfoArray ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_HOST_INFO,
                              &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         _errorDetail = "rest field:" + string( OM_REST_FIELD_HOST_INFO )
                        + " is null" ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, hostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pHostInfo, rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

      hostInfoArray = hostInfo.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;
      {
         BSONObjIterator iter( hostInfoArray ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "host info is invalid, element of %s "
                           "is not Object type:type=%d",
                           OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
               goto error ;
            }
            BSONObj oneHost = ele.embeddedObject() ;
            hostNameList.push_back(
                           oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ) ;
         }
      }

      if ( hostNameList.size() == 0 )
      {
         _errorDetail = "rest field:" + string( OM_REST_FIELD_HOST_INFO )
                        + " is invalid" ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s:rest=%s", _errorDetail.c_str(), pHostInfo ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omQueryHostStatusCommand::_verifyHostInfo(
                                           list<string> &hostNameList,
                                           list<fullHostInfo> &hostInfoList )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      BSONArrayBuilder arrayBuilder ;
      list<string>::iterator iterList = hostNameList.begin() ;
      while ( iterList != hostNameList.end() )
      {
         BSONObj tmp = BSON( OM_HOST_FIELD_NAME << *iterList ) ;
         arrayBuilder.append( tmp ) ;
         iterList++ ;
      }
      matcher = BSON( "$or" << arrayBuilder.arr() ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         _errorDetail = string( "fail to query table:" )
                        + OM_CS_DEPLOY_CL_HOST ;
         PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            _errorDetail = string( "failed to get record from table:" )
                           + OM_CS_DEPLOY_CL_HOST ;
            PD_LOG( PDERROR, "%s,rc=%d", _errorDetail.c_str(), rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         BSONObj diskArray ;
         fullHostInfo hostInfo ;
         hostInfo.hostName  = result.getStringField( OM_HOST_FIELD_NAME ) ;
         hostInfo.user      = result.getStringField( OM_HOST_FIELD_USER ) ;
         hostInfo.passwd    = result.getStringField( OM_HOST_FIELD_PASSWORD ) ;
         hostInfo.agentPort = result.getStringField( OM_HOST_FIELD_AGENT_PORT ) ;
         diskArray = result.getObjectField( OM_HOST_FIELD_DISK ) ;
         {
            BSONObjIterator iter( diskArray ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( ele.type() != Object )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "disk info is invalid, element of %s "
                              "is not Object type:type=%d",
                              OM_HOST_FIELD_DISK, ele.type() ) ;
                  _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
                  goto error ;
               }
               BSONObj oneDisk = ele.embeddedObject() ;
               simpleDiskInfo diskInfo ;
               diskInfo.diskName  = oneDisk.getStringField(
                                                    OM_HOST_FIELD_DISK_NAME ) ;
               diskInfo.mountPath = oneDisk.getStringField(
                                                    OM_HOST_FIELD_DISK_MOUNT ) ;
               hostInfo.diskInfo.push_back( diskInfo ) ;
            }
         }

         BSONObj netArray ;
         netArray = result.getObjectField( OM_HOST_FIELD_NET ) ;
         {
            BSONObjIterator iter( netArray ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( ele.type() != Object )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "net info is invalid, element of %s "
                              "is not Object type:type=%d",
                              OM_HOST_FIELD_NET, ele.type() ) ;
                  _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
                  goto error ;
               }
               BSONObj oneNet = ele.embeddedObject() ;
               simpleNetInfo netInfo ;
               netInfo.netName = oneNet.getStringField(
                                                    OM_HOST_FIELD_NET_NAME ) ;
               netInfo.ip = oneNet.getStringField(
                                                    OM_HOST_FIELD_NET_IP ) ;
               hostInfo.netInfo.push_back( netInfo ) ;
            }
         }

         hostInfoList.push_back( hostInfo ) ;
      }

      if ( hostInfoList.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         _errorDetail = "host is not found" ;
         PD_LOG( PDERROR, "%s:rc=%d", _errorDetail.c_str(), rc ) ;
         goto error ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omQueryHostStatusCommand::_addQueryHostStatusReq( omManager *om,
                                            pmdRemoteSession *remoteSession,
                                            list<fullHostInfo> &hostInfoList )
   {
      INT32 rc = SDB_OK ;
      list<fullHostInfo>::iterator iter = hostInfoList.begin() ;
      while ( iter != hostInfoList.end() )
      {
         MsgRouteID routeID ;
         BSONArrayBuilder arrayBuilder ;
         BSONArrayBuilder arrayNetBuilder ;
         BSONObj bsonRequest ;
         pmdSubSession *subSession   = NULL ;
         CHAR *pContent              = NULL ;
         INT32 contentSize           = 0 ;
         fullHostInfo *pHostInfo     = &(*iter) ;
         routeID   = om->updateAgentInfo( pHostInfo->hostName,
                                          pHostInfo->agentPort ) ;
         subSession = remoteSession->addSubSession( routeID.value ) ;
         if ( NULL == subSession )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "addSubSessin failed" ) ;
            goto error ;
         }

         list<simpleDiskInfo>::iterator diskIter = pHostInfo->diskInfo.begin() ;
         while ( diskIter != pHostInfo->diskInfo.end() )
         {
            BSONObj tmp = BSON( OM_BSON_FIELD_DISK_NAME << diskIter->diskName
                                << OM_BSON_FIELD_DISK_MOUNT
                                << diskIter->mountPath ) ;
            arrayBuilder.append( tmp ) ;
            diskIter++ ;
         }

         list<simpleNetInfo>::iterator netIter = pHostInfo->netInfo.begin() ;
         while ( netIter != pHostInfo->netInfo.end() )
         {
            BSONObj tmp = BSON( OM_HOST_FIELD_NET_NAME << netIter->netName
                                << OM_HOST_FIELD_NET_IP
                                << netIter->ip ) ;
            arrayNetBuilder.append( tmp ) ;
            netIter++ ;
         }

         bsonRequest = BSON( OM_BSON_FIELD_HOST_NAME << pHostInfo->hostName
                             << OM_BSON_FIELD_HOST_USER << pHostInfo->user
                             << OM_BSON_FIELD_HOST_PASSWD
                             << pHostInfo->passwd
                             << OM_BSON_FIELD_DISK << arrayBuilder.arr()
                             << OM_BSON_FIELD_NET << arrayNetBuilder.arr() ) ;
         rc = msgBuildQueryMsg( &pContent, &contentSize,
                                CMD_ADMIN_PREFIX OM_QUERY_HOST_STATUS_REQ,
                                0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "msgBuildQueryMsg failed:rc=%d", rc ) ;
            goto error ;
         }

         subSession->setReqMsg( (MsgHeader *)pContent ) ;
         iter++;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omQueryHostStatusCommand::_appendErrorResult(
                                                BSONArrayBuilder &arrayBuilder,
                                                const string &host, INT32 err,
                                                const string &detail )
   {
      BSONObj obj = BSON( OM_BSON_FIELD_HOST_NAME << host
                          << OM_REST_RES_RETCODE << err
                          << OM_REST_RES_DETAIL << detail ) ;
      arrayBuilder.append( obj ) ;
   }

   INT32 omQueryHostStatusCommand::_getHostStatus(
                                             list<fullHostInfo> &hostInfoList,
                                             BSONObj &bsonStatus )
   {
      INT32 rc      = SDB_OK ;
      INT32 tmpRC   = SDB_OK ;
      omManager *om = NULL ;
      string tmpError ;
      BSONArrayBuilder arrayBuilder ;
      pmdRemoteSession *remoteSession = NULL ;
      VEC_SUB_SESSIONPTR subSessionVec ;
      INT32 sucNum   = 0 ;
      INT32 totalNum = 0 ;

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                  OM_QUERY_HOST_STATUS_TIMEOUT,
                                                  NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "addSession failed:rc=%d", rc ) ;
         goto error ;
      }
      rc = _addQueryHostStatusReq( om, remoteSession, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "_addQueryHostStatusReq failed:rc=%d", rc ) ;
         goto error ;
      }

      remoteSession->sendMsg( &sucNum, &totalNum ) ;
      rc = _getAllReplay( remoteSession, &subSessionVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "wait replay failed:rc=%d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < subSessionVec.size() ; i++ )
      {
         vector<BSONObj> objVec ;
         SINT32 flag               = SDB_OK ;
         SINT64 contextID          = -1 ;
         SINT32 startFrom          = 0 ;
         SINT32 numReturned        = 0 ;
         MsgHeader* pRspMsg        = NULL ;
         pmdSubSession *subSession = subSessionVec[i] ;

         string tmpHost = "" ;
         string tmpService ;
         pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;
         om->getHostInfoByID( subSession->getNodeID(), tmpHost, tmpService ) ;
         if ( subSession->isDisconnect() )
         {
            tmpRC = SDB_NETWORK ;
            PD_LOG_MSG( PDERROR, "session disconnected:id=%s,rc=%d",
                        routeID2String(subSession->getNodeID()).c_str(),
                        tmpRC ) ;
            tmpError = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _appendErrorResult( arrayBuilder, tmpHost, tmpRC, tmpError ) ;
            continue ;
         }

         pRspMsg = subSession->getRspMsg() ;
         if ( NULL == pRspMsg )
         {
            tmpRC = SDB_UNEXPECTED_RESULT ;
            PD_LOG_MSG( PDERROR, "unexpected result:rc=%d", tmpRC ) ;
            tmpError = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _appendErrorResult( arrayBuilder, tmpHost, tmpRC, tmpError ) ;
            continue ;
         }

         tmpRC = msgExtractReply( (CHAR *)pRspMsg, &flag, &contextID,
                                  &startFrom, &numReturned, objVec ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG_MSG( PDERROR, "extract reply failed:rc=%d", tmpRC ) ;
            tmpError = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _appendErrorResult( arrayBuilder, tmpHost, tmpRC, tmpError ) ;
            continue ;
         }

         if ( SDB_OK != flag )
         {
            if ( objVec.size() > 0 )
            {
               tmpError = objVec[0].getStringField( OM_REST_RES_DETAIL ) ;
            }
            _appendErrorResult( arrayBuilder, tmpHost, flag, tmpError ) ;
            PD_LOG( PDERROR, "agent process failed:detail=%s,rc=%d",
                    tmpError.c_str(), flag ) ;
            continue ;
         }

         if ( 1 != objVec.size() )
         {
            tmpRC = SDB_UNEXPECTED_RESULT ;
            PD_LOG_MSG( PDERROR, "unexpected response size:rc=%d", tmpRC ) ;
            tmpError = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            _appendErrorResult( arrayBuilder, tmpHost, tmpRC, tmpError ) ;
            continue ;
         }

         BSONObj result = objVec[0] ;
         _formatHostStatus( result ) ;
         arrayBuilder.append( result ) ;
      }

      rc = SDB_OK ;
      bsonStatus = BSON( OM_BSON_FIELD_HOST_INFO << arrayBuilder.arr() ) ;
   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      {
         "Name":"eth1","RXBytes":15686963,"RXPackets":192255,"RXErrors":0,
         "RXDrops":0,"TXBytes":4064080,"TXPackets":42780,"TXErrors":0,
         "TXDrops":0
      }
   */
   void omQueryHostStatusCommand::_formatHostStatusOneNet( BSONObj &oneNet )
   {
      BSONElement rxbyteEle  = oneNet.getField( OM_BSON_FIELD_NET_RXBYTES ) ;
      BSONElement rxpackEle  = oneNet.getField( OM_BSON_FIELD_NET_RXPACKETS ) ;
      BSONElement rxerrorEle = oneNet.getField( OM_BSON_FIELD_NET_RXERRORS ) ;
      BSONElement rxdropEle  = oneNet.getField( OM_BSON_FIELD_NET_RXDROPS ) ;
      if ( !rxbyteEle.isNumber() || !rxpackEle.isNumber()
           || !rxerrorEle.isNumber() || !rxdropEle.isNumber() )
      {
         return ;
      }

      BSONObj newRxByte ;
      _seperateMegaBitValue( newRxByte, rxbyteEle.numberLong() ) ;

      BSONObj newRxPack ;
      _seperateMegaBitValue( newRxPack, rxpackEle.numberLong() ) ;

      BSONObj newRxErr ;
      _seperateMegaBitValue( newRxErr, rxerrorEle.numberLong() ) ;

      BSONObj newRxDrop ;
      _seperateMegaBitValue( newRxDrop, rxdropEle.numberLong() ) ;

      BSONElement txbyteEle  = oneNet.getField( OM_BSON_FIELD_NET_TXBYTES ) ;
      BSONElement txpackEle  = oneNet.getField( OM_BSON_FIELD_NET_TXPACKETS ) ;
      BSONElement txerrorEle = oneNet.getField( OM_BSON_FIELD_NET_TXERRORS ) ;
      BSONElement txdropEle  = oneNet.getField( OM_BSON_FIELD_NET_TXDROPS ) ;
      if ( !txbyteEle.isNumber() || !txpackEle.isNumber()
           || !txerrorEle.isNumber() || !txdropEle.isNumber() )
      {
         return ;
      }

      BSONObj newTxByte ;
      _seperateMegaBitValue( newTxByte, txbyteEle.numberLong() ) ;

      BSONObj newTxPack ;
      _seperateMegaBitValue( newTxPack, txpackEle.numberLong() ) ;

      BSONObj newTxErr ;
      _seperateMegaBitValue( newTxErr, txerrorEle.numberLong() ) ;

      BSONObj newTxDrop ;
      _seperateMegaBitValue( newTxDrop, txdropEle.numberLong() ) ;

      BSONElement ip = oneNet.getField( OM_BSON_FIELD_NET_IP ) ;
      if( ip.type() != String && ip.type() != jstNULL )
      {
         return ;
      }
      oneNet = BSON( OM_BSON_FIELD_NET_NAME
                     << oneNet.getStringField( OM_BSON_FIELD_NET_NAME )
                     << OM_BSON_FIELD_NET_RXBYTES << newRxByte
                     << OM_BSON_FIELD_NET_RXPACKETS << newRxPack
                     << OM_BSON_FIELD_NET_RXERRORS << newRxErr
                     << OM_BSON_FIELD_NET_RXDROPS << newRxDrop
                     << OM_BSON_FIELD_NET_TXBYTES << newTxByte
                     << OM_BSON_FIELD_NET_TXPACKETS << newTxPack
                     << OM_BSON_FIELD_NET_TXERRORS << newTxErr
                     << OM_BSON_FIELD_NET_TXDROPS << newTxDrop
                     << OM_BSON_FIELD_NET_IP << ip ) ;
   }

   /*
      {
         "Net":[
            {
               "Name":"eth1","RXBytes":15686963,"RXPackets":192255,"RXErrors":0,
               "RXDrops":0,"TXBytes":4064080,"TXPackets":42780,"TXErrors":0,
               "TXDrops":0
            }
         ]
      }
   */
   void omQueryHostStatusCommand::_formatHostStatusNet( BSONObj &net )
   {
      BSONElement netEle = net.getField( OM_BSON_FIELD_NET ) ;
      if ( Array != netEle.type() )
      {
         return ;
      }

      BSONArrayBuilder arrayBuilder ;

      BSONObj nets = net.getObjectField( OM_BSON_FIELD_NET ) ;
      BSONObjIterator iter( nets ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         if ( ele.type() != Object )
         {
            return ;
         }

         BSONObj oneNet = ele.embeddedObject() ;
         _formatHostStatusOneNet( oneNet ) ;

         arrayBuilder.append( oneNet ) ;
      }

      net = BSON( OM_BSON_FIELD_NET << arrayBuilder.arr() ) ;
   }

   void omQueryHostStatusCommand::_seperateMegaBitValue( BSONObj &obj,
                                                         long value )
   {
      INT32 megabit = ( INT32 ) ( value/OM_SIZE_MEGABIT ) ;
      INT32 unit    = ( INT32 ) ( value%OM_SIZE_MEGABIT ) ;
      obj = BSON( OM_BSON_FIELD_CPU_MEGABIT << megabit
                  << OM_BSON_FIELD_CPU_UNIT  << unit ) ;
   }

   /*
      {
         CPU: { "Sys":349550,"Idle":404327420,"Other":149190,"User":402630 }
      }
   */
   void omQueryHostStatusCommand::_formatHostStatusCPU( BSONObj &cpu )
   {
      BSONElement cpuEle = cpu.getField( OM_BSON_FIELD_CPU ) ;
      if ( Object != cpuEle.type() )
      {
         return ;
      }

      BSONObj cpuValue     = cpuEle.embeddedObject() ;

      BSONElement sysEle   = cpuValue.getField( OM_BSON_FIELD_CPU_SYS ) ;
      BSONElement idleEle  = cpuValue.getField( OM_BSON_FIELD_CPU_IDLE ) ;
      BSONElement otherEle = cpuValue.getField( OM_BSON_FIELD_CPU_OTHER ) ;
      BSONElement userEle  = cpuValue.getField( OM_BSON_FIELD_CPU_USER ) ;
      if ( !sysEle.isNumber() || !idleEle.isNumber() || !otherEle.isNumber()
           || !userEle.isNumber() )
      {
         return ;
      }

      BSONObj newSys ;
      _seperateMegaBitValue( newSys, sysEle.numberLong() ) ;

      BSONObj newIdle ;
      _seperateMegaBitValue( newIdle, idleEle.numberLong() ) ;

      BSONObj newOther ;
      _seperateMegaBitValue( newOther, otherEle.numberLong() ) ;

      BSONObj newUser ;
      _seperateMegaBitValue( newUser, userEle.numberLong() ) ;

      BSONObj cpuNewValue = BSON( OM_BSON_FIELD_CPU_SYS << newSys
                                  << OM_BSON_FIELD_CPU_IDLE << newIdle
                                  << OM_BSON_FIELD_CPU_OTHER << newOther
                                  << OM_BSON_FIELD_CPU_USER << newUser ) ;

      cpu = BSON( OM_BSON_FIELD_CPU << cpuNewValue ) ;
   }

   void omQueryHostStatusCommand::_formatHostStatus( BSONObj &status )
   {
      BSONObj filterCPU = BSON( OM_BSON_FIELD_CPU << "" ) ;
      BSONObj cpu       = status.filterFieldsUndotted( filterCPU, true ) ;
      BSONObj others    = status.filterFieldsUndotted( filterCPU, false ) ;

      _formatHostStatusCPU( cpu ) ;

      BSONObj filterNet = BSON( OM_BSON_FIELD_NET << "" ) ;
      BSONObj net  = others.filterFieldsUndotted( filterNet, true ) ;
      BSONObj left = others.filterFieldsUndotted( filterNet, false ) ;

      /*
         {
           "Net": {
                     "CalendarTime": 1413277808,
                     "Net": [
                     {
                        "Name": "lo", "RXBytes": 5125811, "RXPackets": 95615,
                        "RXErrors": 0, "RXDrops": 0, "TXBytes": 5125811,
                        "TXPackets": 95615, "TXErrors": 0, "TXDrops": 0
                     }
                     ]
                  }
         }
      */
      BSONElement netEle = net.getField( OM_BSON_FIELD_NET ) ;
      if ( Object != netEle.type() )
      {
         return ;
      }

      BSONObj netValue = netEle.embeddedObject() ;

      BSONObj innerNetOther = netValue.filterFieldsUndotted( filterNet, false ) ;
      BSONObj innerNet = netValue.filterFieldsUndotted( filterNet, true ) ;
      _formatHostStatusNet( innerNet ) ;

      BSONObjBuilder innerBuilder ;
      innerBuilder.appendElements( innerNetOther ) ;
      innerBuilder.appendElements( innerNet ) ;
      BSONObj newNet = BSON( OM_BSON_FIELD_NET << innerBuilder.obj() ) ;

      BSONObjBuilder builder ;
      builder.appendElements( cpu ) ;
      builder.appendElements( newNet ) ;
      builder.appendElements( left ) ;

      status = builder.obj() ;
   }

   INT32 omQueryHostStatusCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<string> hostNameList ;
      list<fullHostInfo> hostInfoList ;
      BSONObj bsonStatus ;
      BSONObj result ;
      rc = _getRestHostList( hostNameList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getRestHostList failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _verifyHostInfo( hostNameList, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_verifyHostInfo failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _getHostStatus( hostInfoList, bsonStatus ) ;
      if ( SDB_OK != rc )
      {
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         PD_LOG( PDERROR, "_getHostStatus failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      _restAdaptor->appendHttpBody( _restSession, bsonStatus.objdata(),
                                    bsonStatus.objsize(), 1 ) ;
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omPredictCapacity::omPredictCapacity( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession )
                     : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omPredictCapacity::~omPredictCapacity()
   {
   }

   INT32 omPredictCapacity::_getHostList( BSONObj &hostInfos,
                                          list<string> &hostNameList )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator iter( hostInfos ) ;
      while ( iter.more() )
      {
         string hostName ;
         BSONElement oneEle  = iter.next() ;
         if ( oneEle.type() != Object )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host info is invalid, element of host "
                        "is not Object type:type=%d", oneEle.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }
         BSONObj oneHostName = oneEle.embeddedObject() ;
         BSONElement nameEle = oneHostName.getField(
                                                OM_BSON_FIELD_HOST_NAME ) ;
         if ( nameEle.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not String:field=%s,type=%d",
                        OM_BSON_FIELD_HOST_NAME, nameEle.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }
         hostNameList.push_back( nameEle.String() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omPredictCapacity::_getTemplateValue( BSONObj &properties,
                                               INT32 &replicaNum,
                                               INT32 &groupNum )
   {
      INT32 rc              = SDB_OK ;
      BOOLEAN isRepSet      = FALSE ;
      BSONObjIterator iter( properties ) ;
      while ( iter.more() )
      {
         string name ;
         string value ;
         BSONElement oneEle  = iter.next() ;
         if ( oneEle.type() != Object )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "property info is invalid, element of property "
                        "is not Object type:type=%d", oneEle.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }

         BSONObj oneProperty = oneEle.embeddedObject() ;
         BSONElement nameEle = oneProperty.getField(
                                                OM_BSON_PROPERTY_NAME ) ;
         if ( nameEle.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not String:field=%s,type=%d",
                        OM_BSON_PROPERTY_NAME, nameEle.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }
         name = nameEle.String() ;
         BSONElement valueEle = oneProperty.getField(
                                                OM_BSON_PROPERTY_VALUE ) ;
         if ( valueEle.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not String:field=%s,type=%d",
                        OM_BSON_PROPERTY_VALUE, valueEle.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }
         value = valueEle.String() ;

         if ( name == OM_TEMPLATE_REPLICA_NUM )
         {
            replicaNum = ossAtoi( value.c_str() ) ;
            isRepSet   = TRUE ;
         }
         else if ( name == OM_TEMPLATE_DATAGROUP_NUM )
         {
            groupNum      = ossAtoi( value.c_str() ) ;
         }
      }

      if ( !isRepSet )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "bson miss template field:field=%s",
                     OM_TEMPLATE_REPLICA_NUM ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

      if ( !isRepSet )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "bson miss template field:field=%s",
                     OM_TEMPLATE_DATAGROUP_NUM ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

      if ( replicaNum <= 0 || groupNum <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "replicaNum or groupNum is invalid:replicaNum=%d,"
                     "groupNum=%d", replicaNum, groupNum ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omPredictCapacity::_getRestInfo( list<string> &hostNameList,
                                          string &clusterName,
                                          INT32 &replicaNum, INT32 &groupNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTemplate = NULL ;
      BSONObj bsonTemplate ;
      _restAdaptor->getQuery( _restSession, OM_REST_TEMPLATE_INFO,
                              &pTemplate ) ;
      if ( NULL == pTemplate )
      {
         _errorDetail = "rest field:" + string( OM_REST_TEMPLATE_INFO )
                        + " is null" ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s", _errorDetail.c_str() ) ;
         goto error ;
      }

      rc = fromjson( pTemplate, bsonTemplate ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "change rest field to BSONObj failed:src=%s,"
                     "rc=%d", pTemplate, rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

      {
         BSONObj properties ;
         BSONElement ele = bsonTemplate.getField( OM_BSON_PROPERTY_ARRAY ) ;
         if ( ele.type() != Array )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not array:field=%s,type=%d",
                        OM_BSON_PROPERTY_ARRAY, ele.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }

         properties = ele.embeddedObject() ;
         rc = _getTemplateValue( properties, replicaNum, groupNum ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_getTemplateValue failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( bsonTemplate.hasField( OM_BSON_FIELD_HOST_INFO ) )
      {
         BSONObj hostInfos ;
         BSONElement ele = bsonTemplate.getField( OM_BSON_FIELD_HOST_INFO ) ;
         if ( ele.type() != Array )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not array:field=%s,type=%d",
                        OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }
         hostInfos = ele.embeddedObject() ;
         rc = _getHostList( hostInfos, hostNameList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_getHostList failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      {
         BSONElement ele ;
         ele = bsonTemplate.getField( OM_BSON_CLUSTER_NAME ) ;
         if ( ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "bson field is not String:field=%s,type=%d",
                        OM_BSON_CLUSTER_NAME, ele.type() ) ;
            _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
            goto error ;
         }

         clusterName = ele.String() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omPredictCapacity::_predictCapacity(
                                           list<simpleHostDisk> &hostInfoList,
                                           INT32 replicaNum, INT32 groupNum,
                                           UINT64 &totalSize, UINT64 &validSize,
                                           UINT32 &redundancyRate )
   {
      INT32 rc = SDB_OK ;
      redundancyRate = replicaNum - 1 ;

      UINT32 nodeCount         = replicaNum * groupNum ;
      UINT32 diskCount         = 0 ;
      UINT64 totalDiskFreeSize = 0 ;
      list<simpleHostDisk>::iterator iterHost = hostInfoList.begin() ;
      while ( iterHost != hostInfoList.end() )
      {
         simpleHostDisk *pHost = &( *iterHost ) ;
         list<simpleDiskInfo>::iterator iterDisk = pHost->diskInfo.begin() ;
         while ( iterDisk != pHost->diskInfo.end() )
         {
            simpleDiskInfo *pDisk = &( *iterDisk ) ;
            totalDiskFreeSize += pDisk->freeSize ;
            diskCount++ ;
            iterDisk++ ;
         }
         iterHost++ ;
      }

      if ( 0 == diskCount || 0 == totalDiskFreeSize )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "disk info error:diskCount=%u,diskFreeSize="
                     OSS_LL_PRINT_FORMAT, diskCount, totalDiskFreeSize ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         goto error ;
      }

      if ( nodeCount < diskCount )
      {
         totalSize = totalDiskFreeSize / diskCount * nodeCount ;
      }
      else
      {
         totalSize = totalDiskFreeSize ;
      }

      validSize = totalSize / replicaNum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omPredictCapacity::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<string> hostNameList ;
      string clusterName ;
      INT32 replicaNum ;
      INT32 groupNum ;
      list<simpleHostDisk> hostInfoList ;

      rc = _getRestInfo( hostNameList, clusterName, replicaNum, groupNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      rc = _fetchHostDiskInfo( clusterName, hostNameList, hostInfoList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getSpecificHostInfo failed:rc=%d", rc ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      if ( hostInfoList.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "there is no host in the cluster:clusterName=%s",
                     clusterName.c_str() ) ;
         _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
         _sendErrorRes2Web( rc, _errorDetail ) ;
         goto error ;
      }

      {
         BSONObj predict ;
         BSONObj result ;
         UINT64 totalSize      = 0 ;
         UINT64 validSize      = 0 ;
         UINT32 redundancyRate = 0 ;
         rc = _predictCapacity( hostInfoList, replicaNum, groupNum, totalSize,
                                validSize, redundancyRate ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_predictCapacity failed:rc=%d", rc ) ;
            _sendErrorRes2Web( rc, _errorDetail ) ;
            goto error ;
         }

         predict = BSON( OM_BSON_FIELD_TOTAL_SIZE << (long long)totalSize
                         << OM_BSON_FIELD_VALID_SIZE << (long long)validSize
                         << OM_BSON_FIELD_REDUNDANCY_RATE << redundancyRate ) ;
         _restAdaptor->appendHttpBody( _restSession, predict.objdata(),
                                       predict.objsize(), 1) ;
         _sendOKRes2Web() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omGetLogCommand::omGetLogCommand( restAdaptor *pRestAdaptor,
                                     pmdRestSession *pRestSession )
                   :omAuthCommand( pRestAdaptor, pRestSession )
   {
      _restAdaptor = pRestAdaptor ;
      _restSession = pRestSession ;
   }

   omGetLogCommand::~omGetLogCommand()
   {
   }

   INT32 omGetLogCommand::_getFileContent( string filePath,
                                           CHAR **pFileContent,
                                           INT32 &fileContentLen )
   {
      OSSFILE file ;
      INT32 rc               = SDB_OK ;
      INT64 fileSize         = 0 ;
      SINT64 realFileSize    = 0 ;
      bool isFileOpened      = false ;
      bool isAllocBuff       = false ;

      rc = ossOpen( filePath.c_str(), OSS_READONLY, OSS_RWXU, file ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to open file:file=%s, rc = %d",
                    filePath.c_str(), rc ) ;

      isFileOpened = true ;
      rc           = ossGetFileSize( &file, &fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get file size:file=%s, rc = %d",
                    filePath.c_str(), rc ) ;

      rc = _restSession->allocBuff( fileSize, pFileContent, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to alloc buff:buff_size=%I64d, rc = %d",
                    fileSize, rc ) ;

      isAllocBuff = true ;
      rc          = ossRead( &file, *pFileContent, fileSize, &realFileSize ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to read file content:file=%s, rc = %d",
                    filePath.c_str(), rc ) ;

      fileContentLen = realFileSize ;

   done:
      if ( isFileOpened )
      {
         ossClose( file ) ;
      }

      return rc ;
   error:
      if ( isAllocBuff )
      {
         _restSession->releaseBuff( *pFileContent ) ;
      }
      goto done ;
   }

   INT32 omGetLogCommand::doCommand()
   {
      INT32 rc             = SDB_OK ;
      CHAR *pContent       = NULL ;
      INT32 contentLength  = 0 ;
      const CHAR *pSubPath = NULL ;
      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR logPath[ OSS_MAX_PATHSIZE + 1 ] ;

      _restAdaptor->getQuery( _restSession, OM_REST_LOG_NAME,
                              &pSubPath ) ;
      if ( NULL == pSubPath )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_REST_LOG_NAME ) ;
         goto error ;
      }

      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "get current path failed:rc=%d", rc ) ;
         goto error ;
      }

      ossSnprintf( logPath, OSS_MAX_PATHSIZE, "%s%s..%s%s%s%s%s%s", currentPath,
                   OSS_FILE_SEP, OSS_FILE_SEP, OM_CONF_PATH_STR, OSS_FILE_SEP,
                   OM_LOG_PATH_STR, OSS_FILE_SEP, pSubPath ) ;
      rc = _getFileContent( logPath, &pContent, contentLength ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "OM: get file content failed:file=%s,rc=%d",
                     logPath, rc ) ;
         goto error ;
      }

      _restAdaptor->appendHttpBody( _restSession, pContent, contentLength, 1,
                                    FALSE ) ;
      _restAdaptor->sendResponse( _restSession, HTTP_OK ) ;

   done:
      if ( NULL != pContent )
      {
         _restSession->releaseBuff( pContent ) ;
      }
      return rc ;
   error:
      _restAdaptor->appendHttpBody( _restSession, "1", 0, 1, FALSE ) ;
      _restAdaptor->sendResponse( _restSession, HTTP_OK ) ;
      goto done ;
   }


   omSetBusinessAuthCommand::omSetBusinessAuthCommand(
                                                  restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                            :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omSetBusinessAuthCommand::~omSetBusinessAuthCommand()
   {
   }

   INT32 omSetBusinessAuthCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string businessName ;
      string userName ;
      string passwd ;
      string dbName ;
      BSONObj authOptions ;
      omDatabaseTool dbTool( _cb ) ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "sss|s",
                             OM_REST_FIELD_BUSINESS_NAME, &businessName,
                             OM_REST_FIELD_USER,          &userName,
                             OM_REST_FIELD_PASSWORD,      &passwd,
                             OM_REST_FIELD_DB_NAME,       &dbName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isBusinessExist( businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: name=%s",
                             businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      authOptions = BSON( OM_BSON_DB_NAME << dbName ) ;

      rc = dbTool.upsertAuth( businessName, userName, passwd, authOptions ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to set business auth,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }
   omRemoveBusinessAuthCommand::omRemoveBusinessAuthCommand(
                                                  restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                               :omAuthCommand( pRestAdaptor, pRestSession )
   {

   }

   omRemoveBusinessAuthCommand::~omRemoveBusinessAuthCommand()
   {
   }

   INT32 omRemoveBusinessAuthCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pBusinessName = NULL ;
      BSONObj deletor ;
      BSONObj hint ;

      _restAdaptor->getQuery( _restSession, OM_REST_BUSINESS_NAME,
                              &pBusinessName ) ;
      if ( NULL == pBusinessName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_REST_BUSINESS_NAME ) ;
         goto error ;
      }

      deletor = BSON( OM_AUTH_FIELD_BUSINESS_NAME << pBusinessName ) ;
      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS_AUTH, deletor, hint, 0, _cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "rtnDelete failed:deletor=%s,rc=%d",
                     deletor.toString().c_str(), rc ) ;
         goto error ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
      goto done ;
   }

   omQueryBusinessAuthCommand::omQueryBusinessAuthCommand(
                                                  restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession )
                              :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omQueryBusinessAuthCommand::~omQueryBusinessAuthCommand()
   {
   }

   void omQueryBusinessAuthCommand::_sendAuthInfo2Web( list<BSONObj> &authInfo )
   {
      BSONObj filter = BSON( OM_AUTH_FIELD_PASSWD << 1 ) ;
      list<BSONObj>::iterator iter = authInfo.begin() ;
      while ( iter != authInfo.end() )
      {
         BSONObj tmp = iter->filterFieldsUndotted( filter, false ) ;
         _restAdaptor->appendHttpBody( _restSession, tmp.objdata(),
                                       tmp.objsize(), 1 ) ;
         iter++ ;
      }

      _sendOKRes2Web() ;

      return ;
   }

   INT32 omQueryBusinessAuthCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 skip      = 0 ;
      SINT64 returnNum = -1 ;
      list<BSONObj> authInfo ;

      rc = _getQueryPara( selector, matcher, order, hint, skip, returnNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get query parameter failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _queryTable( OM_CS_DEPLOY_CL_BUSINESS_AUTH, selector, matcher, order,
                        hint, 0, skip, returnNum, authInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
         goto error ;
      }

      _sendAuthInfo2Web( authInfo ) ;
   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
      goto done ;
   }

   omDiscoverBusinessCommand::omDiscoverBusinessCommand(
                                                 restAdaptor *pRestAdaptor,
                                                 pmdRestSession *pRestSession,
                                                 string &localAgentHost,
                                                 string &localAgentService )
                             : omAuthCommand( pRestAdaptor, pRestSession ),
                               _localAgentHost( localAgentHost ),
                               _localAgentService( localAgentService )

   {
   }

   omDiscoverBusinessCommand::~omDiscoverBusinessCommand()
   {
   }

   INT32 omDiscoverBusinessCommand::_getRestInfo( BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pConfigInfo = NULL ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CONFIGINFO,
                              &pConfigInfo ) ;
      if ( NULL == pConfigInfo )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest field failed:field=%s",
                             OM_REST_FIELD_CONFIGINFO ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = fromjson( pConfigInfo, configInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "change rest field to BSONObj failed:"
                                   "src=%s,rc=%d",
                             pConfigInfo, rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_checkHostPort( const string &address,
                                                    const string &port )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      omDatabaseTool dbTool( _cb ) ;
      vector<string> portList ;
      vector<string>::iterator iter ;

      rc = dbTool.getHostNameByAddress( address, hostName ) ;
      if ( rc )
      {
         if ( SDB_DMS_RECORD_NOTEXIST == rc )
         {
            rc = SDB_OK ;
            hostName = address ;
         }
         else
         {
            _errorMsg.setError( TRUE, "failed to get host name,rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      rc = dbTool.getHostUsedPort( hostName, portList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get host used port,host=%s,rc=%d",
                             hostName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = portList.begin(); iter != portList.end(); ++iter )
      {
         string usedPort = *iter ;

         if ( usedPort == port  )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid argument: %s:%s has been used by"
                                      " other business.",
                                hostName.c_str(), port.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_checkWebLinkCFG( BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      string webPort ;

      hostName = buzInfo.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      webPort  = buzInfo.getStringField( OM_REST_WEB_SERVICE_PORT ) ;
      if ( hostName.empty() || webPort.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business info is invalid:%s=%s,%s=%s",
                             OM_BSON_FIELD_HOST_NAME, hostName.c_str(),
                             OM_REST_WEB_SERVICE_PORT, webPort.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      /*
      if ( !_isHostExistInCluster( hostName, clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host is not exist in cluster:host=%s,"
                     "cluster=%s", hostName.c_str(),
                      clusterName.c_str() ) ;
         goto error ;
      }
      */

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_checkSequoiasqlCFG( BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      string serviceName ;
      string dbName ;
      string user ;
      string passwd ;
      string installPath ;

      hostName    = buzInfo.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      serviceName = buzInfo.getStringField( OM_BSON_FIELD_SVCNAME ) ;
      installPath = buzInfo.getStringField( OM_BSON_FIELD_INSTALL_PATH ) ;
      dbName      = buzInfo.getStringField( OM_REST_DBNAME ) ;
      user        = buzInfo.getStringField( OM_BSON_FIELD_HOST_USER ) ;
      passwd      = buzInfo.getStringField( OM_BSON_FIELD_HOST_PASSWD ) ;
      if ( hostName.empty() || serviceName.empty() || dbName.empty() ||
           user.empty() || passwd.empty() || installPath.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Sequoiasql's info is invalid:%s=%s,"
                                   "%s=%s,%s=%s,%s=%s,%s=%s or %s is empty",
                             OM_BSON_FIELD_HOST_NAME, hostName.c_str(),
                             OM_BSON_FIELD_SVCNAME, serviceName.c_str(),
                             OM_REST_DBNAME, dbName.c_str(),
                             OM_BSON_FIELD_HOST_USER, user.c_str(),
                             OM_BSON_FIELD_INSTALL_PATH, installPath.c_str(),
                             OM_BSON_FIELD_HOST_PASSWD ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_checkSequoiaDBCFG( BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      string svcname ;

      hostName = buzInfo.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      if ( hostName.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is empty",
                             OM_BSON_FIELD_HOST_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      svcname = buzInfo.getStringField( OM_BSON_FIELD_SVCNAME ) ;
      if ( svcname.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s is empty",
                             OM_BSON_FIELD_SVCNAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _checkHostPort( hostName, svcname ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "failed to check host used port, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_checkBusinssCFG( BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;

      _clusterName = configInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( _clusterName.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to get cluster name:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_OM_CLUSTER_NOT_EXIST ;
         _errorMsg.setError( TRUE, "cluster is not exist:cluster=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessName = configInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      if ( _businessName.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to get business name:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = configInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      if ( _businessType.empty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to get business type:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == dbTool.isBusinessExist( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business already exist,name:%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if( dbTool.hasTaskRunning() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "the task is running" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      {
         BSONObj buzInfo ;
         BSONElement ele = configInfo.getField( OM_BSON_BUSINESS_INFO ) ;

         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid argument:field=%s,rc=%d",
                                OM_BSON_BUSINESS_INFO, rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         buzInfo  = ele.embeddedObject() ;

         if ( OM_BUSINESS_SPARK == _businessType ||
              OM_BUSINESS_HDFS  == _businessType ||
              OM_BUSINESS_YARN  == _businessType )
         {
            rc = _checkWebLinkCFG( buzInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "check %s BusinessInfo failed:rc=%d",
                       _businessType.c_str(), rc ) ;
               goto error ;
            }
         }
         else if ( OM_BUSINESS_SEQUOIASQL == _businessType )
         {
            rc = _checkSequoiasqlCFG( buzInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "check %s BusinessInfo failed:rc=%d",
                       _businessType.c_str(), rc ) ;
               goto error ;
            }
         }
         else if ( OM_BUSINESS_SEQUOIADB == _businessType )
         {
            rc = _checkSequoiaDBCFG( buzInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "check %s BusinessInfo failed:rc=%d",
                       _businessType.c_str(), rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid business type:type=%s,rc=%d",
                                _businessType.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_storeBusinessInfo( const INT32 addType,
                                                        const string &deployMod,
                                                        const BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.addBusinessInfo( OM_BUSINESS_ADDTYPE_DISCOVERY, _clusterName,
                                   _businessName, _businessType, deployMod,
                                   buzInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to add business:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

   }

   void omDiscoverBusinessCommand::_generateRequest( const string &hostName,
                                                     const string &svcname,
                                                     const string &authUser,
                                                     const string &authPwd,
                                                     const string &agentService,
                                                     BSONObj &request )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;
      BSONObjBuilder addressBuilder ;
      vector<simpleAddressInfo>::iterator iter ;

      builder.append( OM_BUSINESS_FIELD_CLUSTERNAME, _clusterName ) ;
      builder.append( OM_BUSINESS_FIELD_NAME, _businessName ) ;
      builder.append( OM_BUSINESS_FIELD_TYPE, _businessType ) ;

      builder.append( OM_AUTH_FIELD_USER, authUser ) ;
      builder.append( OM_AUTH_FIELD_PASSWD, authPwd ) ;

      addressBuilder.append( OM_CONFIGURE_FIELD_HOSTNAME, hostName ) ;
      addressBuilder.append( OM_CONF_DETAIL_SVCNAME, svcname ) ;
      if ( FALSE == agentService.empty() )
      {
         addressBuilder.append( OM_BSON_FIELD_AGENT_PORT, agentService ) ;
      }
      arrayBuilder.append( addressBuilder.obj() ) ;

      builder.append( OM_REST_FIELD_ADDRESS, arrayBuilder.arr() ) ;
      request = builder.obj() ;
   }

   void omDiscoverBusinessCommand::_parseHostMap( const BSONObj &hosts,
                                                map<string, string> &hostMap )
   {
      BSONObjIterator iter( hosts ) ;

      while ( iter.more() )
      {
         BSONElement ele  = iter.next() ;
         BSONObj hostInfo = ele.embeddedObject() ;
         string hostName  = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
         string ip        = hostInfo.getStringField( OM_HOST_FIELD_IP ) ;

         hostMap[hostName] = ip ;
      }
   }

   /*
      try convert hostName to ip
   */
   void omDiscoverBusinessCommand::_hostName2Address(
                                                   map<string, string> &hostMap,
                                                   string &hostName,
                                                   string &address )
   {
      map<string, string>::iterator iter ;

      iter = hostMap.find( hostName ) ;
      if ( iter == hostMap.end() )
      {
         address = hostName ;
      }
      else
      {
         address = hostMap[hostName] ;
      }
   }

   INT32 omDiscoverBusinessCommand::_checkSyncSdbResult(
                                                omRestTool &restTool,
                                                const BSONObj &hostInfo,
                                                map<string, string> &hostMap )
   {
      INT32 rc            = SDB_OK ;
      INT32 lastErrno     = SDB_OK ;
      INT32 coordNum      = 0 ;
      INT32 catalogNum    = 0 ;
      INT32 standaloneNum = 0 ;
      INT32 missHostNum   = 0 ;
      INT32 errorHostNum  = 0 ;
      string lastErrMsg ;
      omDatabaseTool dbTool( _cb ) ;
      BSONArrayBuilder hostsArray ;
      BSONObjIterator iter( hostInfo ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj tmpConfig = ele.embeddedObject() ;
         BSONObj nodeConfigs = tmpConfig.getObjectField(
                                                OM_CONFIGURE_FIELD_CONFIG ) ;
         INT32 hostErrno = tmpConfig.getIntField(
                                             OM_CONFIGURE_FIELD_ERRNO ) ;
         string tmpHostName = tmpConfig.getStringField(
                                             OM_CONFIGURE_FIELD_HOSTNAME ) ;
         BSONObjIterator configsIter( nodeConfigs ) ;

         while ( configsIter.more() )
         {
            BSONElement configEle = configsIter.next() ;
            BSONObj nodeConfig = configEle.embeddedObject() ;
            string role = nodeConfig.getStringField(
                                                OM_CONFIGURE_FIELD_ROLE ) ;

            if ( OM_NODE_ROLE_COORD == role )
            {
               ++coordNum ;
            }
            else if ( OM_NODE_ROLE_CATALOG == role )
            {
               ++catalogNum ;
            }
            else if ( OM_NODE_ROLE_STANDALONE == role )
            {
               ++standaloneNum ;
            }
         }

         if ( FALSE == dbTool.isHostExistOfCluster( tmpHostName,
                                                    _clusterName ) )
         {
            string tmpAddress ;

            if ( dbTool.isHostExistOfClusterByIp( tmpHostName,
                                                  _clusterName ) )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "IP mode is not supported" ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            ++missHostNum ;

            _hostName2Address( hostMap, tmpHostName, tmpAddress ) ;
            hostsArray.append( tmpAddress ) ;
         }

         if ( SDB_OK != hostErrno )
         {
            string hostErrMsg = tmpConfig.getStringField(
                                             OM_CONFIGURE_FIELD_DETAIL ) ;

            lastErrno  = hostErrno ;
            lastErrMsg = hostErrMsg ;
            ++errorHostNum ;
         }
      }

      if ( 0 < missHostNum )
      {
         BSONObjBuilder missHost ;

         missHost.append( OM_BUSINESS_RES_HOSTS, hostsArray.arr() ) ;
         restTool.appendResponeMsg( missHost.obj() ) ;

         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, missing host" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 < errorHostNum )
      {
         rc = lastErrno ;
         _errorMsg.setError( TRUE, "%s, rc=%d", lastErrMsg.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == coordNum && 0 == standaloneNum )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, "
                                   "missing coord node, do not add a "
                                   "temporary coord node" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == catalogNum && 0 == standaloneNum )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, "
                                   "missing catalog node" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_syncSequoiaDB( omRestTool &restTool,
                                                    const BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      string svcname ;
      string authUser ;
      string authPwd ;
      string agentService ;
      string errDetail ;
      BSONObj request ;
      BSONObj result ;
      map<string, string> hostMap ;
      omDatabaseTool dbTool( _cb ) ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      hostName = buzInfo.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      svcname  = buzInfo.getStringField( OM_BSON_FIELD_SVCNAME ) ;
      authUser = buzInfo.getStringField( OM_BSON_FIELD_HOST_USER ) ;
      authPwd  = buzInfo.getStringField( OM_BSON_FIELD_HOST_PASSWD ) ;
      agentService = buzInfo.getStringField( OM_BSON_FIELD_AGENT_PORT ) ;

      {
         string address = hostName ;

         rc = dbTool.getHostNameByAddress( address, hostName ) ;
         if ( rc )
         {
            if ( SDB_DMS_RECORD_NOTEXIST == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               _errorMsg.setError( TRUE, "failed to get host name,rc=%d", rc ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }
         }
      }

      _generateRequest( hostName, svcname, authUser, authPwd, agentService,
                        request ) ;

      rc = taskTool.notifyAgentMsg( CMD_ADMIN_PREFIX OM_SYNC_BUSINESS_CONF_REQ,
                                    request, errDetail, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to notify agent,detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _parseHostMap( result.getObjectField( OM_BSON_FIELD_HOSTS ), hostMap ) ;

      rc = _checkSyncSdbResult( restTool,
                                result.getObjectField( OM_BSON_FIELD_HOST_INFO ),
                                hostMap ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to call _checkSyncResult: rc=%d", rc ) ;
         goto error ;
      }

      if ( authUser.length() > 0 && authPwd.length() > 0 )
      {
         rc = dbTool.upsertAuth( _businessName, authUser, authPwd ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to add business auth,rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      rc = dbTool.addNodeConfigOfBusiness( _clusterName, _businessName,
                                           _businessType, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to add business config,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::_syncBusiness( omRestTool &restTool,
                                                   BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;

      if ( OM_BUSINESS_SPARK == _businessType ||
           OM_BUSINESS_HDFS  == _businessType ||
           OM_BUSINESS_YARN  == _businessType )
      {
         string deployMod = "" ;
         BSONObj filter = BSON( OM_BSON_BUSINESS_INFO << "" ) ;
         BSONObj buzInfo = configInfo.filterFieldsUndotted( filter, TRUE ) ;

         rc = _storeBusinessInfo( OM_BUSINESS_ADDTYPE_DISCOVERY, deployMod,
                                  buzInfo ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to store business info:rc=%d", rc ) ;
            goto error ;
         }
      }
      else if ( OM_BUSINESS_SEQUOIASQL == _businessType )
      {
         string deployMod = OM_SEQUOIASQL_DEPLOY_OLTP ;
         BSONObj filter = BSON( OM_BSON_BUSINESS_INFO << "" ) ;
         BSONObj buzInfo = configInfo.filterFieldsUndotted( filter, TRUE ) ;

         rc = _storeBusinessInfo( OM_BUSINESS_ADDTYPE_DISCOVERY, deployMod,
                                  buzInfo ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to store business info:rc=%d", rc ) ;
            goto error ;
         }
      }
      else if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         BSONObj buzInfo ;
         BSONElement ele = configInfo.getField( OM_BSON_BUSINESS_INFO ) ;

         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid argument:field=%s,rc=%d",
                                OM_BSON_BUSINESS_INFO, rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         buzInfo = ele.embeddedObject() ;

         rc = _syncSequoiaDB( restTool, buzInfo ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to sync business info:rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid business type:type=%s,rc=%d",
                             _businessType.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDiscoverBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj configInfo ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo( configInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get rest business info,rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkBusinssCFG( configInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check business config,rc=%d", rc ) ;
         goto error ;
      }

      rc = _syncBusiness( restTool, configInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "falied to sync business,rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;
   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   omUnDiscoverBusinessCommand::omUnDiscoverBusinessCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession )
                               :omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omUnDiscoverBusinessCommand::~omUnDiscoverBusinessCommand()
   {
   }

   INT32 omUnDiscoverBusinessCommand::_UnDiscoverBusiness(
                                                   const string &clusterName,
                                                   const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BSONObj businessInfo ;
      BSONObj deletor ;
      BSONObj hint ;
      BSONElement addTypeEle ;
      INT32 addType = 0 ;
      rc = _getBusinessInfo( businessName, businessInfo ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OM_BUSINESS_NOT_EXIST ;
            PD_LOG_MSG( PDERROR, "business is not exist:business=%s",
                        businessName.c_str() ) ;
            goto error ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "get business info failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      addTypeEle = businessInfo.getField( OM_BUSINESS_FIELD_ADDTYPE ) ;
      if ( addTypeEle.isNumber() )
      {
         addType = addTypeEle.Int() ;
      }

      if ( OM_BUSINESS_ADDTYPE_DISCOVERY != addType )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "can not undiscover business that is not"
                     " discovered:business=%s", businessName.c_str() ) ;
         goto error ;
      }

      deletor = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;
      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS, deletor, hint, 0, _cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "rtnDelete failed:deletor=%s,rc=%d",
                     deletor.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUnDiscoverBusinessCommand::_getRestBusinessInfo(
                                                       string &clusterName,
                                                       string &businessName )
   {
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusinessName = NULL ;
      INT32 rc = SDB_OK ;
      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_CLUSTER_NAME,
                              &pClusterName ) ;
      if ( NULL == pClusterName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_REST_FIELD_CLUSTER_NAME ) ;
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_BUSINESS_NAME,
                              &pBusinessName ) ;
      if ( NULL == pBusinessName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     OM_REST_BUSINESS_NAME) ;
         goto error ;
      }

      clusterName  = pClusterName ;
      businessName = pBusinessName ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUnDiscoverBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string clusterName ;
      string businessName ;

      rc = _getRestBusinessInfo( clusterName,  businessName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get business info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _UnDiscoverBusiness( clusterName, businessName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "remove business info failed:rc=%d", rc ) ;
         goto error ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
      goto done ;
   }

   omSsqlExecCommand::omSsqlExecCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession,
                                         const string &localAgentHost,
                                         const string &localAgentPort )
                     : omAuthCommand( pRestAdaptor, pRestSession ),
                       _localAgentHost( localAgentHost ),
                       _localAgentService( localAgentPort )
   {
   }

   omSsqlExecCommand::~omSsqlExecCommand()
   {
   }

   INT32 omSsqlExecCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string dbName ;
      string sql ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "ssss",
                                OM_REST_FIELD_CLUSTER_NAME, &_clusterName,
                                OM_REST_FIELD_BUSINESS_NAME, &_businessName,
                                OM_REST_FIELD_SQL, &sql,
                                OM_REST_FIELD_DB_NAME, &dbName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _execSsql( sql, dbName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to exec ssql: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omSsqlExecCommand::_check()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj businessInfo ;
      omDatabaseTool dbTool( _cb ) ;

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = dbTool.getOneBusinessInfo( _businessName, businessInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: name=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = businessInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      if( OM_BUSINESS_SEQUOIASQL_OLTP != _businessType )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported business type: name=%s, type=%s",
                             _businessName.c_str(), _businessType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      taskID = dbTool.getTaskIdOfRunningBuz( _businessName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task["OSS_LL_PRINT_FORMAT"]",
                             _businessName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omSsqlExecCommand::_execSsql( const string &sql, const string &dbName )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      BSONObj request ;
      BSONObj result ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = _generateRequest( sql, dbName, request ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate request: rc=%d", rc ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentMsg( CMD_ADMIN_PREFIX OM_SSQL_EXEC_REQ,
                                    request, errDetail, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "%s", errDetail.c_str() ) ;
         PD_LOG( PDERROR, "failed to notify agent: detail=%s, rc=%d",
                 errDetail.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omSsqlExecCommand::_generateRequest( const string &sql,
                                              const string &dbName,
                                              BSONObj &request )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder requestBuilder ;
      list<BSONObj> configList ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getConfigByBusiness( _businessName, configList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get configure: name=%s, rc=%d",
                             _businessName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( configList.empty() )
      {
         rc = SDB_SYS ;
         _errorMsg.setError( TRUE, "the business configure is empty:"
                                   " name=%s, rc=%d",
                             _businessName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      requestBuilder.append( OM_BSON_CLUSTER_NAME, _clusterName ) ;
      requestBuilder.append( OM_BSON_BUSINESS_NAME, _clusterName ) ;
      requestBuilder.append( OM_BSON_BUSINESS_TYPE, _clusterName ) ;

      {
         string hostName ;
         string serviceName ;
         string installPath ;
         list<BSONObj>::iterator iter = configList.begin() ;
         BSONObj configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

         hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;

         {
               BSONObjIterator configIter( configInfo ) ;

               while ( configIter.more() )
               {
                  BSONElement ele = configIter.next() ;
                  BSONObj tmpNodeInfo = ele.embeddedObject() ;

                  serviceName = tmpNodeInfo.getStringField(
                                                   OM_CONFIGURE_FIELD_PORT2 ) ;
                  installPath = tmpNodeInfo.getStringField(
                                             OM_CONFIGURE_FIELD_INSTALLPATH ) ;
                  break ;
               }
         }

         requestBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
         requestBuilder.append( OM_BSON_SERVICENAME, serviceName ) ;
         requestBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;
      }

      requestBuilder.append( OM_BSON_DB_NAME, dbName ) ;
      requestBuilder.append( OM_BSON_SQL, sql ) ;

      request = requestBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omInterruptTaskCommand::omInterruptTaskCommand( restAdaptor *pRestAdaptor,
                                                  pmdRestSession *pRestSession,
                                                  const string &localAgentHost,
                                                  const string &localAgentPort )
                          :omScanHostCommand( pRestAdaptor, pRestSession,
                                              localAgentHost, localAgentPort )
   {
   }

   omInterruptTaskCommand::~omInterruptTaskCommand()
   {
   }

   INT32 omInterruptTaskCommand::_parseInterruptTaskInfo()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTaskID = NULL ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      list<BSONObj> records ;
      _restAdaptor->getQuery( _restSession, FIELD_NAME_TASKID, &pTaskID ) ;
      if ( NULL == pTaskID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get rest field failed:field=%s",
                     FIELD_NAME_TASKID) ;
         goto error ;
      }

      _taskID = ossAtoll( pTaskID ) ;

      matcher = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      rc = _queryTable( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order,
                        hint, 0, 0, -1, records ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
         goto error ;
      }
      else if ( records.size() == 0 )
      {
         rc = SDB_OM_TASK_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "task is not exist:taskID="OSS_LL_PRINT_FORMAT,
                     _taskID ) ;
         goto error ;
      }
      else
      {
         INT32 status ;
         BSONObj oneTask = *records.begin() ;
         status = oneTask.getIntField( OM_TASKINFO_FIELD_STATUS ) ;
         if ( OM_TASK_STATUS_FINISH == status ||
              OM_TASK_STATUS_CANCEL == status )
         {
            PD_LOG( PDEVENT, "task is finish:taskID="OSS_LL_PRINT_FORMAT
                    ",status=%d", _taskID, status ) ;
            _isFinished = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omInterruptTaskCommand::notifyAgentInteruptTask( INT64 taskID )
   {
      INT32 rc          = SDB_OK ;
      SINT32 flag       = SDB_OK ;
      CHAR *pContent    = NULL ;
      INT32 contentSize = 0 ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      BSONObj result ;

      bsonRequest = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = msgBuildQueryMsg( &pContent, &contentSize,
                             CMD_ADMIN_PREFIX OM_INTERRUPT_TASK_REQ,
                             0, 0, 0, -1, &bsonRequest, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "build msg failed:cmd=%s,rc=%d",
                     OM_INTERRUPT_TASK_REQ, rc ) ;
         goto error ;
      }

      om            = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_NOTIFY_TASK_TIMEOUT,
                                                      NULL ) ;
      if ( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "create remote session failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         goto error ;
      }

      pMsg = (MsgHeader *)pContent ;
      rc   = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         SDB_OSS_FREE( pContent ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      rc = _receiveFromAgent( remoteSession, flag, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         rc = flag ;
         string tmpError = result.getStringField( OM_REST_RES_DETAIL ) ;
         PD_LOG_MSG( PDERROR, "agent process failed:detail=(%s),rc=%d",
                     tmpError.c_str(), rc ) ;
         goto error ;
      }
   done:
      _clearSession( om, remoteSession ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 omInterruptTaskCommand::updateTaskStatus( INT64 taskID, INT32 status,
                                                   INT32 errNo )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      time_t now = time( NULL ) ;
      builder.appendTimestamp( OM_TASKINFO_FIELD_END_TIME, now * 1000, 0 ) ;
      builder.append( OM_TASKINFO_FIELD_ERRNO, errNo ) ;
      builder.append( OM_TASKINFO_FIELD_STATUS, status ) ;
      builder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                      getTaskStatusStr( status ) ) ;
      BSONObj selector = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      BSONObj updator  = BSON( "$set" << builder.obj() ) ;
      BSONObj hint ;
      INT64 updateNum = 0 ;
      rc = rtnUpdate( OM_CS_DEPLOY_CL_TASKINFO, selector, updator, hint, 0,
                      pmdGetThreadEDUCB(), &updateNum ) ;
      if ( SDB_OK != rc || 0 == updateNum )
      {
         PD_LOG_MSG( PDERROR, "update task failed:table=%s,updateNum=%d,taskID="
                     OSS_LL_PRINT_FORMAT",updator=%s,selector=%s,rc=%d",
                 OM_CS_DEPLOY_CL_TASKINFO, updateNum, taskID,
                 updator.toString().c_str(), selector.toString().c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omInterruptTaskCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      rc = _parseInterruptTaskInfo() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse task info failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( !_isFinished )
      {
         rc = notifyAgentInteruptTask( _taskID ) ;
         if ( SDB_OK != rc && SDB_OM_TASK_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "notify agent to interrupt task failed:taskID="
                    OSS_LL_PRINT_FORMAT",rc=%d", _taskID, rc ) ;
            goto error ;
         }

         rc = updateTaskStatus( _taskID, OM_TASK_STATUS_CANCEL, 0 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "update task's status failed:taskID=%s"
                    OSS_LL_PRINT_FORMAT",rc=%d", _taskID, rc ) ;
            goto error ;
         }
      }

      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
      goto done ;
   }

   omForwardPluginCommand::omForwardPluginCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession,
                                                const string &businessType )
                                 : omAuthCommand( pRestAdaptor, pRestSession )
   {
      _businessType = businessType ;
   }

   omForwardPluginCommand::~omForwardPluginCommand()
   {
   }

   #define OM_FORWARD_TIMEOUT (10000)
   INT32 omForwardPluginCommand::doCommand()
   {
      UINT16 port = 0 ;
      INT32 rc = SDB_OK ;
      INT32 headerSize = 0 ;
      INT32 bodySize   = 0 ;
      const CHAR *pHttpHeader = NULL ;
      const CHAR *pHttpBody   = NULL ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;
      BSONObj pluginInfo ;
      string serviceName ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = dbTool.getPluginInfoByBusinessType( _businessType, pluginInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "Set socket keep alive failed[ %d ]", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      serviceName = pluginInfo.getStringField( OM_PLUGINS_FIELD_SERVICENAME ) ;

      ossGetPort( serviceName.c_str(), port ) ;

      {
         ossSocket client( "127.0.0.1", port ) ;

         rc = client.initSocket() ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to init network: rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         rc = client.setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                                   OSS_SOCKET_KEEP_INTERVAL,
                                   OSS_SOCKET_KEEP_CONTER ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "Set socket keep alive failed[ %d ]", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         rc = client.connect( OM_FORWARD_TIMEOUT ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to connect plugin: rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         headerSize  = _restAdaptor->getRequestHeaderSize( _restSession ) ;
         pHttpHeader = _restAdaptor->getRequestHeader( _restSession ) ;

         rc = pmdSend( pHttpHeader, headerSize, &client,
                       _cb, OM_FORWARD_TIMEOUT ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to forward http header: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         bodySize  = _restAdaptor->getRequestBodySize( _restSession ) ;
         pHttpBody = _restAdaptor->getRequestBody( _restSession ) ;

         rc = pmdSend( pHttpBody, bodySize, &client, _cb, OM_FORWARD_TIMEOUT ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to forward http body: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         while( !_cb->isInterrupted() )
         {
            INT32 recvSize = 0 ;
            CHAR tmpBuf[256] ;

            rc = client.recv( tmpBuf, 256, recvSize, OSS_SOCKET_DFT_TIMEOUT,
                              0, FALSE, FALSE ) ;
            if ( rc )
            {
               if ( SDB_TIMEOUT == rc )
               {
                  continue;
               }
               break ;
            }

            rc = _restSession->sendData( tmpBuf, recvSize, OM_FORWARD_TIMEOUT ) ;
            if ( rc )
            {
               break ;
            }
         }
      }

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   omGetFileCommand::omGetFileCommand( restAdaptor *pRestAdaptor,
                                       pmdRestSession *pRestSession,
                                       const CHAR *pRootPath,
                                       const CHAR *pSubPath )
                    :omGetLogCommand( pRestAdaptor, pRestSession )
   {
      _restAdaptor = pRestAdaptor ;
      _restSession = pRestSession ;
      _rootPath    = pRootPath ;
      _subPath     = pSubPath ;
   }

   omGetFileCommand::~omGetFileCommand()
   {
   }

   INT32 omGetFileCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT32 contentLength = 0 ;
      CHAR *pContent      = NULL ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusinessName = NULL ;
      restFileController* transfer = NULL ;
      string realSubPath = _subPath ;

      transfer = restFileController::getTransferInstance() ;
      transfer->getTransferedPath( _subPath.c_str(), realSubPath ) ;

      _restAdaptor->getHttpHeader( _restSession, OM_REST_HEAD_CLUSTERNAME,
                                   &pClusterName ) ;
      _restAdaptor->getHttpHeader( _restSession, OM_REST_HEAD_BUSINESSNAME,
                                   &pBusinessName ) ;
      if ( NULL == pClusterName || NULL == pBusinessName )
      {
         rc = _getFileContent( _rootPath + realSubPath, &pContent,
                               contentLength ) ;
      }
      else
      {
         omDatabaseTool dbTool( _cb ) ;
         string businessType ;
         BSONObj businessInfo ;

         rc = dbTool.getOneBusinessInfo( pBusinessName, businessInfo ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to get business info: cluster=%s,"
                             "business=%s, rc=%d",
                    pClusterName, pBusinessName, rc ) ;
            _restAdaptor->sendResponse( _restSession, HTTP_SERVICUNAVA ) ;
            goto error ;
         }

         businessType = businessInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;

         if ( OM_BUSINESS_SEQUOIADB == businessType )
         {
            rc = _getFileContent( _rootPath + realSubPath, &pContent,
                                  contentLength ) ;
         }
         else
         {
            CHAR pluginPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
            string realPluginPath = ".."OSS_FILE_SEP"plugins"OSS_FILE_SEP ;
            string pluginRootPath ;
            BSONObj pluginInfo ;

            rc = dbTool.getPluginInfoByBusinessType( businessType, pluginInfo ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to get plugin info: "
                                "business type=%s, rc=%d",
                       businessType.c_str(), rc ) ;
               _restAdaptor->sendResponse( _restSession, HTTP_SERVICUNAVA ) ;
               goto error ;
            }

            realPluginPath += pluginInfo.getStringField(
                                                      OM_PLUGINS_FIELD_NAME ) ;

            realPluginPath += OSS_FILE_SEP"www" ;

            rc = utilBuildFullPath( _rootPath.c_str(),
                                    realPluginPath.c_str(),
                                    OSS_MAX_PATHSIZE, pluginPath ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "plugin path is too long: rc=%d",
                       rc ) ;
               _restAdaptor->sendResponse( _restSession, HTTP_SERVICUNAVA ) ;
               goto error ;
            }

            pluginRootPath = pluginPath ;

            rc = _getFileContent( pluginRootPath + realSubPath, &pContent,
                                  contentLength ) ;
         }
      }

      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            PD_LOG( PDEVENT, "OM: file no found:%s, rc=%d",
                    realSubPath.c_str(), rc ) ;
            _restAdaptor->sendResponse( _restSession, HTTP_NOTFOUND ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "OM: open file failed:%s, rc=%d",
                    realSubPath.c_str(), rc ) ;
            _restAdaptor->sendResponse( _restSession, HTTP_SERVICUNAVA ) ;
         }

         goto error ;
      }

      _restAdaptor->appendHttpBody( _restSession, pContent, contentLength, 1,
                                    FALSE ) ;
      _restAdaptor->sendResponse( _restSession, HTTP_OK ) ;

   done:
      if ( NULL != pContent )
      {
         _restSession->releaseBuff( pContent ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetFileCommand::undoCommand()
   {
      return SDB_OK ;
   }

   restFileController::restFileController()
   {
      INT32 pairCount   = 0 ;
      INT32 i           = 0 ;
      INT32 publicCount = 0 ;

      static char* filePathTable[][2] = {
         { OSS_FILE_SEP,    OSS_FILE_SEP OM_REST_INDEX_HTML }
      } ;

      static char *fileAuthorityPublic[] = {
         OSS_FILE_SEP OM_REST_LOGIN_HTML ,
      } ;

      pairCount = sizeof( filePathTable ) / ( 2 * sizeof ( char * ) ) ;

      for ( i = 0 ; i < pairCount ; i++ )
      {
         _transfer.insert( mapValueType( filePathTable[i][0],
                                                    filePathTable[i][1])) ;
      }

      publicCount = sizeof( fileAuthorityPublic ) / ( sizeof ( char * ) ) ;
      for ( i = 0 ; i < publicCount ; i++ )
      {
         _publicAccessFiles.insert( mapValueType( fileAuthorityPublic[i],
                                                  fileAuthorityPublic[i])) ;
      }
   }

   restFileController* restFileController::getTransferInstance()
   {
      static restFileController instance ;
      return &instance ;
   }

   INT32 restFileController::getTransferedPath( const char *src,
                                                string &transfered )
   {
      INT32 rc = SDB_OK ;
      mapIteratorType iter = _transfer.find( src ) ;
      if ( iter == _transfer.end() )
      {
         goto error ;
      }

      transfered = iter->second ;

   done:
      return rc ;
   error:
      rc = -1 ;
      goto done ;
   }

   bool restFileController::isFileAuthorPublic( const char *file )
   {
      mapIteratorType iter = _publicAccessFiles.find( file ) ;
      if ( iter == _publicAccessFiles.end() )
      {
         return false ;
      }

      return true ;
   }

   omStrategyCmdBase::omStrategyCmdBase( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession )
   : omRestCommandBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyCmdBase::~omStrategyCmdBase()
   {
   }

   INT32 omStrategyCmdBase::_getAndCheckBusiness( string &clsName,
                                                  string &bizName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pValTmp = NULL ;

      _restAdaptor->getQuery( _restSession, OM_BSON_CLUSTER_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_BSON_CLUSTER_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      clsName = pValTmp ;

      _restAdaptor->getQuery( _restSession, OM_BSON_BUSINESS_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_BSON_BUSINESS_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      bizName = pValTmp ;

      if ( !_isBusinessExist( clsName, bizName ) )
      {
         _errorDetail = string( "cluster or business is not exist" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omStrategyTaskInsert::omStrategyTaskInsert( restAdaptor *pRestAdaptor,
                                               pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyTaskInsert::~omStrategyTaskInsert()
   {
   }

   INT32 omStrategyTaskInsert::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pValTmp = NULL ;
      omTaskInfo inputInfo ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }
      inputInfo.setClusterName( clsName ) ;
      inputInfo.setBusinessName( bizName ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TASK_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_TASK_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      inputInfo.setTaskName( pValTmp ) ;

      inputInfo.setCreator( _restSession->getLoginUserName() ) ;
      inputInfo.makeCreateTime() ;

      rc = omGetStrategyMgr()->insertTask( inputInfo, _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to add the task" );
         goto error ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyTaskList::omStrategyTaskList( restAdaptor *pRestAdaptor,
                                           pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyTaskList::~omStrategyTaskList()
   {
   }

   INT32 omStrategyTaskList::doCommand()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = - 1 ;
      const SINT32 maxNumToReturn = 1000 ;
      string clsName, bizName ;
      rtnContextBuf buffObj ;
      BSONObj objTmp ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = omGetStrategyMgr()->queryTasks( clsName, bizName,
                                           contextID, _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to list task" ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = rtnGetMore( contextID, maxNumToReturn, buffObj,
                          _cb, _pRTNCB ) ;
         if ( rc != SDB_OK )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               _errorDetail = string( "Getmore failed" ) ;
               goto error ;
            }
         }

         while( TRUE )
         {
            rc = buffObj.nextObj( objTmp ) ;
            if ( rc != SDB_OK )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               else
               {
                  _errorDetail = string( "Failed to parse record" ) ;
                  goto error ;
               }
            }
            rc = _restAdaptor->appendHttpBody( _restSession, objTmp.objdata(),
                                               objTmp.objsize(), 1 ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "failed to append http body:rc=%d", rc ) ;
               _errorDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               goto error ;
            }
         }
      }
      _sendOKRes2Web() ;

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, _cb, _pRTNCB ) ;
      }
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyUpdateTaskStatus::omStrategyUpdateTaskStatus( restAdaptor *pRestAdaptor,
                                                           pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyUpdateTaskStatus::~omStrategyUpdateTaskStatus()
   {
   }

   INT32 omStrategyUpdateTaskStatus::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT32 status = 0 ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName, taskName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TASK_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_TASK_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      taskName = pValTmp ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_STATUS,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_STATUS")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      status = ossAtoi( pValTmp ) ;

      rc = omGetStrategyMgr()->updateTaskStatus( clsName, bizName, taskName,
                                                 status, _cb ) ;
      if ( rc )
      {
         _errorDetail = string( "Failed to update task status" );
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyTaskDel::omStrategyTaskDel( restAdaptor * pRestAdaptor,
                                         pmdRestSession * pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyTaskDel::~omStrategyTaskDel()
   {
   }

   INT32 omStrategyTaskDel::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName, taskName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TASK_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_TASK_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      taskName = pValTmp ;

      rc = omGetStrategyMgr()->delTask( clsName, bizName,
                                        taskName, _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to delete task" );
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyInsert::omStrategyInsert( restAdaptor *pRestAdaptor, 
                                       pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyInsert::~omStrategyInsert()
   {
   }

   INT32 omStrategyInsert::doCommand()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pValTmp = NULL ;
      omTaskStrategyInfo inputInfo ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }
      inputInfo.setClusterName( clsName ) ;
      inputInfo.setBusinessName( bizName ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TASK_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_TASK_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      inputInfo.setTaskName( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_NICE,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_NICE")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      inputInfo.setNice( ossAtoi( pValTmp ) ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_SORT_ID,
                              &pValTmp ) ;
      if ( pValTmp != NULL )
      {
         inputInfo.setSortID( ossAtoll( pValTmp ) ) ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_USER_NAME,
                              &pValTmp ) ;
      if ( pValTmp != NULL )
      {
         inputInfo.setUserName( pValTmp ) ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_IPS,
                              &pValTmp ) ;
      if ( pValTmp != NULL )
      {
         set< string > ipSet ;
         rc = _parseIPsField( pValTmp, ipSet ) ;
         if ( rc != SDB_OK )
         {
            _errorDetail = string( "Failed to parse the field("
                                   OM_REST_FIELD_IPS")" ) ;
            goto error ;
         }
         inputInfo.setIPSet( ipSet ) ;
      }

      rc = omGetStrategyMgr()->insertStrategy( inputInfo, _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to add the strategy" );
         goto error ;
      }

      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyList::omStrategyList( restAdaptor *pRestAdaptor, 
                                   pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyList::~omStrategyList()
   {
   }

   INT32 omStrategyList::doCommand()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = - 1 ;
      const SINT32 maxNumToReturn = 1000 ;
      rtnContextBuf buffObj ;
      BSONObj objTmp ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName, taskName ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_TASK_NAME,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_TASK_NAME")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      taskName = pValTmp ;

      rc = omGetStrategyMgr()->queryStrategys( clsName, bizName,
                                               taskName, contextID,
                                               _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to list task-strategy" ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = rtnGetMore( contextID, maxNumToReturn, buffObj, _cb, _pRTNCB ) ;
         if ( rc != SDB_OK )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               _errorDetail = string( "Getmore failed" ) ;
               goto error ;
            }
         }
         while ( TRUE )
         {
            rc = buffObj.nextObj( objTmp ) ;
            if ( rc != SDB_OK )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               else
               {
                  _errorDetail = string( "Failed to parse record" ) ;
                  goto error ;
               }
            }
            rc = _restAdaptor->appendHttpBody( _restSession, objTmp.objdata(),
                                               objTmp.objsize(), 1 ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "failed to append http body:rc=%d", rc ) ;
               _errorDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               goto error ;
            }
         }
      }
      _sendOKRes2Web() ;

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, _cb, _pRTNCB ) ;
      }
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyUpdateNice::omStrategyUpdateNice( restAdaptor *pRestAdaptor,
                                               pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyUpdateNice::~omStrategyUpdateNice()
   {
   }

   INT32 omStrategyUpdateNice::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT32 nice = 0 ;
      INT64 ruleId = 0 ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_NICE,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_NICE")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      nice = ossAtoi( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      rc = omGetStrategyMgr()->updateStrategyNiceById( nice, ruleId,
                                                       clsName, bizName,
                                                       _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to update nice" ) ;
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyAddIps::omStrategyAddIps( restAdaptor *pRestAdaptor,
                                       pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyAddIps::~omStrategyAddIps()
   {
   }

   INT32 omStrategyAddIps::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 ruleId = 0 ;
      set< string > IPs ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_IPS,
                              &pValTmp ) ;
      if ( NULL == pValTmp )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_IPS")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      rc = _parseIPsField( pValTmp, IPs ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to parse the field("
                                 OM_REST_FIELD_IPS")" ) ;
         goto error ;
      }

      rc = omGetStrategyMgr()->addStrategyIpsById( IPs, ruleId,
                                                   clsName, bizName,
                                                   _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to add ip" ) ;
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyDelIps::omStrategyDelIps( restAdaptor *pRestAdaptor,
                                       pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyDelIps::~omStrategyDelIps()
   {
   }

   INT32 omStrategyDelIps::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 ruleId = 0 ;
      set< string > IPs ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_IPS,
                              &pValTmp ) ;
      if ( NULL == pValTmp )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_IPS")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      rc = _parseIPsField( pValTmp, IPs ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to parse the field("
                                OM_REST_FIELD_IPS")" ) ;
         goto error ;
      }

      rc = omGetStrategyMgr()->delStrategyIpsById( IPs, ruleId,
                                                   clsName, bizName,
                                                   _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to delete ip" );
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyDel::omStrategyDel( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyDel::~omStrategyDel()
   {
   }

   INT32 omStrategyDel::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 ruleId = 0 ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      rc = omGetStrategyMgr()->delStrategyById( ruleId,
                                                clsName, bizName,
                                                _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to delete task strategy record" ) ;
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyUpdateStatus::omStrategyUpdateStatus( restAdaptor *pRestAdaptor, 
                                                   pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyUpdateStatus::~omStrategyUpdateStatus()
   {
   }

   INT32 omStrategyUpdateStatus::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 ruleId = 0 ;
      INT32 status ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_STATUS,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_STATUS")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      status = ossAtoi( pValTmp ) ;

      rc = omGetStrategyMgr()->updateStrategyStatusById( status, ruleId,
                                                         clsName, bizName,
                                                         _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to update task strategy status" ) ;
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyUpdateSortID::omStrategyUpdateSortID( restAdaptor *pRestAdaptor, 
                                                   pmdRestSession *pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyUpdateSortID::~omStrategyUpdateSortID()
   {
   }

   INT32 omStrategyUpdateSortID::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 ruleId = 0 ;
      INT64 sortId = 0 ;
      const CHAR *pValTmp = NULL ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_RULE_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                 OM_REST_FIELD_RULE_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      ruleId = ossAtoll( pValTmp ) ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_SORT_ID,
                              &pValTmp ) ;
      if ( NULL == pValTmp || 0 == pValTmp[0] ||
           !ossIsInteger( pValTmp ) )
      {
          _errorDetail = string( "Failed to get the field("
                                  OM_REST_FIELD_SORT_ID")" ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
      }
      sortId = ossAtoll( pValTmp ) ;

      rc = omGetStrategyMgr()->updateStrategySortIDById( sortId, ruleId,
                                                         clsName, bizName,
                                                         _cb ) ;
      if ( rc != SDB_OK )
      {
         _errorDetail = string( "Failed to update task strategy sortID" ) ;
         goto error ;
      }
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omStrategyFlush::omStrategyFlush( restAdaptor * pRestAdaptor,
                                     pmdRestSession * pRestSession )
   : omStrategyCmdBase( pRestAdaptor, pRestSession )
   {
   }

   omStrategyFlush::~omStrategyFlush()
   {
   }

   INT32 omStrategyFlush::doCommand()
   {
      INT32 rc = SDB_OK ;
      string clsName, bizName ;

      SDB_ASSERT( _restAdaptor != NULL && _restSession != NULL,
                  "_restAdaptor and _restSession can't be null!" ) ;

      rc = _getAndCheckBusiness( clsName, bizName ) ;
      if ( rc )
      {
         goto error ;
      }

      omGetStrategyMgr()->flushStrategy( clsName, bizName, _cb ) ;
      _sendOKRes2Web() ;

   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      PD_LOG( PDERROR, "%s, rc: %d", _errorDetail.c_str(), rc ) ;
      goto done ;
   }

   omGetSystemInfoCommand::omGetSystemInfoCommand( restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession )
   : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omGetSystemInfoCommand::~omGetSystemInfoCommand()
   {
   }

   INT32 omGetSystemInfoCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT32 major        = 0 ;
      INT32 minor        = 0 ;
      INT32 fix          = 0 ;
      INT32 release      = 0 ;
      const CHAR *pBuild = NULL ;
      BSONObj ob ;
      BSONObjBuilder systemInfo ;
      BSONObjBuilder obVersion ;

      ossGetVersion ( &major, &minor, &fix, &release, &pBuild ) ;
      try
      {
         obVersion.append ( FIELD_NAME_MAJOR, major ) ;
         obVersion.append ( FIELD_NAME_MINOR, minor ) ;
         obVersion.append ( FIELD_NAME_FIX, fix ) ;
         obVersion.append ( FIELD_NAME_RELEASE, release ) ;
         obVersion.append ( FIELD_NAME_BUILD, pBuild ) ;
         systemInfo.append ( FIELD_NAME_VERSION, obVersion.obj () ) ;
      }
      catch ( std::exception &e )
      {
         _errorDetail = string( "Failed to append version information" ) ;
         PD_LOG ( PDWARNING, "Failed to append version information, %s",
                  e.what() ) ;
         goto error ;
      }
      ob = systemInfo.obj() ;
      _restAdaptor->appendHttpBody( _restSession, ob.objdata(),
                                    ob.objsize(), 1 ) ;
      _sendOKRes2Web() ;
   done:
      return rc ;
   error:
      _sendErrorRes2Web( rc, _errorDetail ) ;
      goto done ;
   }

   omSyncBusinessConfigureCommand::omSyncBusinessConfigureCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession,
                                                string &localAgentHost,
                                                string &localAgentService )
         : omAuthCommand( pRestAdaptor, pRestSession ),
           _localAgentHost( localAgentHost ),
           _localAgentService( localAgentService )
   {
   }

   omSyncBusinessConfigureCommand::~omSyncBusinessConfigureCommand()
   {
   }

   INT32 omSyncBusinessConfigureCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      string deployMod ;
      BSONObj businessInfo ;
      vector<simpleAddressInfo> addressList ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: %s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = dbTool.getOneBusinessInfo( _businessName, businessInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: %s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _businessType = businessInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;

      taskID = dbTool.getTaskIdOfRunningBuz( _businessName ) ;
      if( 0 <= taskID )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business[%s] is exist "
                             "in task["OSS_LL_PRINT_FORMAT"]",
                             _businessName.c_str(), taskID ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      deployMod = businessInfo.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;

      rc = dbTool.getBusinessAddressWithConfig( _businessName, addressList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getBusinessInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _syncBusinessConfig( restTool, addressList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_syncBusinessConfig failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         string newDeployMod ;

         rc = dbTool.getOneBusinessInfo( _businessName, businessInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business info,rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         newDeployMod = businessInfo.getStringField(
                                                OM_BUSINESS_FIELD_DEPLOYMOD ) ;

         if ( OM_BUSINESS_SEQUOIADB == _businessType &&
              OM_DEPLOY_MOD_DISTRIBUTION == deployMod &&
              OM_DEPLOY_MOD_STANDALONE == newDeployMod )
         {
            dbTool.removeAuth( _businessName ) ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omSyncBusinessConfigureCommand::_getRestInfo()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusinessName = NULL ;

      _restAdaptor->getQuery( _restSession, OM_CLUSTER_FIELD_NAME,
                              &pClusterName ) ;
      if ( NULL == pClusterName || 0 == ossStrlen( pClusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_CLUSTER_FIELD_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      _clusterName = pClusterName ;

      _restAdaptor->getQuery( _restSession, OM_BUSINESS_FIELD_NAME,
                              &pBusinessName ) ;
      if ( NULL == pBusinessName || 0 == ossStrlen( pBusinessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_BUSINESS_FIELD_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      _businessName = pBusinessName ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void omSyncBusinessConfigureCommand::_parseHostMap( const BSONObj &hosts,
                                                map<string, string> &hostMap )
   {
      BSONObjIterator iter( hosts ) ;

      while ( iter.more() )
      {
         BSONElement ele  = iter.next() ;
         BSONObj hostInfo = ele.embeddedObject() ;
         string hostName  = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
         string ip        = hostInfo.getStringField( OM_HOST_FIELD_IP ) ;

         hostMap[hostName] = ip ;
      }
   }

   /*
      try convert hostName to ip
   */
   void omSyncBusinessConfigureCommand::_hostName2Address(
                                                   map<string, string> &hostMap,
                                                   string &hostName,
                                                   string &address )
   {
      map<string, string>::iterator iter ;

      iter = hostMap.find( hostName ) ;
      if ( iter == hostMap.end() )
      {
         address = hostName ;
      }
      else
      {
         address = hostMap[hostName] ;
      }
   }

   INT32 omSyncBusinessConfigureCommand::_checkExecResult(
                                                omRestTool &restTool,
                                                const BSONObj &result,
                                                map<string, string> &hostMap )
   {
      INT32 rc = SDB_OK ;
      INT32 lastErrno     = SDB_OK ;
      INT32 coordNum      = 0 ;
      INT32 catalogNum    = 0 ;
      INT32 standaloneNum = 0 ;
      INT32 missHostNum   = 0 ;
      INT32 errorHostNum  = 0 ;
      string lastErrMsg ;
      BSONObj hostInfoList = result.getObjectField(
                                                OM_BSON_FIELD_HOST_INFO ) ;
      BSONArrayBuilder hostsArray ;
      BSONObjIterator iter( hostInfoList ) ;
      omDatabaseTool dbTool( _cb ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj tmpConfig = ele.embeddedObject() ;
         BSONObj nodeConfigs = tmpConfig.getObjectField(
                                                OM_CONFIGURE_FIELD_CONFIG ) ;
         INT32 hostErrno = tmpConfig.getIntField(
                                             OM_CONFIGURE_FIELD_ERRNO ) ;
         string tmpHostName = tmpConfig.getStringField(
                                             OM_CONFIGURE_FIELD_HOSTNAME ) ;
         BSONObjIterator configsIter( nodeConfigs ) ;

         while ( configsIter.more() )
         {
            BSONElement configEle = configsIter.next() ;
            BSONObj nodeConfig = configEle.embeddedObject() ;
            string role = nodeConfig.getStringField(
                                                OM_CONFIGURE_FIELD_ROLE ) ;

            if ( OM_NODE_ROLE_COORD == role )
            {
               ++coordNum ;
            }
            else if ( OM_NODE_ROLE_CATALOG == role )
            {
               ++catalogNum ;
            }
            else if ( OM_NODE_ROLE_STANDALONE == role )
            {
               ++standaloneNum ;
            }
         }

         if ( FALSE == dbTool.isHostExistOfCluster( tmpHostName,
                                                    _clusterName ) )
         {
            string tmpAddress ;

            if ( dbTool.isHostExistOfClusterByIp( tmpHostName,
                                                  _clusterName ) )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "IP mode is not supported" ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            ++missHostNum ;

            _hostName2Address( hostMap, tmpHostName, tmpAddress ) ;
            hostsArray.append( tmpAddress ) ;
         }


         if ( SDB_OK != hostErrno )
         {
            string hostErrMsg = tmpConfig.getStringField(
                                             OM_CONFIGURE_FIELD_DETAIL ) ;

            lastErrno  = hostErrno ;
            lastErrMsg = hostErrMsg ;
            ++errorHostNum ;
         }
      }

      if ( 0 < missHostNum )
      {
         BSONObjBuilder missHost ;

         missHost.append( OM_BUSINESS_RES_HOSTS, hostsArray.arr() ) ;
         restTool.appendResponeMsg( missHost.obj() ) ;

         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, missing host" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 < errorHostNum )
      {
         rc = lastErrno ;
         _errorMsg.setError( TRUE, "%s, rc=%d", lastErrMsg.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == coordNum && 0 == standaloneNum )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, "
                                   "missing coord node, do not add a "
                                   "temporary coord node" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 == catalogNum && 0 == standaloneNum )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "failed to add business, "
                                   "missing catalog node" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omSyncBusinessConfigureCommand::_syncBusinessConfig(
                                       omRestTool &restTool,
                                       vector<simpleAddressInfo> &addressList )
   {
      INT32 rc = SDB_OK ;
      BSONObj request ;
      BSONObj result ;
      string errDetail ;
      map<string, string> hostMap ;
      omDatabaseTool dbTool( _cb ) ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      _generateRequest( addressList, request ) ;

      rc = taskTool.notifyAgentMsg( CMD_ADMIN_PREFIX OM_SYNC_BUSINESS_CONF_REQ,
                                    request, errDetail, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to notify agent,detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _parseHostMap( result.getObjectField( OM_BSON_FIELD_HOSTS ), hostMap ) ;

      rc = _checkExecResult( restTool, result, hostMap ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "check result failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = dbTool.updateNodeConfigOfBusiness( _businessName, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to update node config,rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omSyncBusinessConfigureCommand::_generateRequest(
                                       vector<simpleAddressInfo> &addressList,
                                       BSONObj &request )
   {
      string authUser ;
      string authPwd ;
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;
      vector<simpleAddressInfo>::iterator iter ;

      _getBusinessAuth( _businessName, authUser, authPwd ) ;

      builder.append( OM_BUSINESS_FIELD_CLUSTERNAME, _clusterName ) ;
      builder.append( OM_BUSINESS_FIELD_NAME, _businessName ) ;
      builder.append( OM_BUSINESS_FIELD_TYPE, _businessType ) ;

      builder.append( OM_AUTH_FIELD_USER, authUser ) ;
      builder.append( OM_AUTH_FIELD_PASSWD, authPwd ) ;

      for ( iter = addressList.begin(); iter != addressList.end(); ++iter )
      {
         BSONObjBuilder addressBuilder ;

         addressBuilder.append( OM_CONFIGURE_FIELD_HOSTNAME, iter->hostName ) ;
         addressBuilder.append( OM_CONF_DETAIL_SVCNAME, iter->port ) ;
         arrayBuilder.append( addressBuilder.obj() ) ;
      }

      builder.append( OM_REST_FIELD_ADDRESS, arrayBuilder.arr() ) ;
      request = builder.obj() ;
   }

   omGrantSysConfigureCommand::omGrantSysConfigureCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession )
         : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omGrantSysConfigureCommand::~omGrantSysConfigureCommand()
   {
   }

   INT32 omGrantSysConfigureCommand::_getRestInfo()
   {
      INT32 rc = SDB_OK ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pName         = NULL ;
      const CHAR *pPrivilege    = NULL ;
      string privilege ;

      _restAdaptor->getQuery( _restSession, OM_CLUSTER_FIELD_NAME,
                              &pClusterName ) ;
      if ( NULL == pClusterName || 0 == ossStrlen( pClusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_CLUSTER_FIELD_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      _clusterName = pClusterName ;

      _restAdaptor->getQuery( _restSession, OM_CLUSTER_FIELD_GRANTNAME,
                              &pName ) ;
      if ( NULL == pName || 0 == ossStrlen( pName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_CLUSTER_FIELD_GRANTNAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      _grantName = pName ;

      if ( OM_CLUSTER_FIELD_HOSTFILE != _grantName &&
           OM_CLUSTER_FIELD_ROOTUSER != _grantName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s=%s",
                             OM_CLUSTER_FIELD_GRANTNAME, _grantName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _restAdaptor->getQuery( _restSession, OM_CLUSTER_FIELD_PRIVILEGE,
                              &pPrivilege ) ;
      if ( NULL == pPrivilege || 0 == ossStrlen( pPrivilege ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_CLUSTER_FIELD_PRIVILEGE ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      privilege = pPrivilege ;

      if ( OM_VALUE_BOOLEAN_TRUE1 == privilege ||
           OM_VALUE_BOOLEAN_TRUE2 == privilege )
      {
         _privilege = TRUE ;
      }
      else if ( OM_VALUE_BOOLEAN_FALSE1 == privilege ||
                OM_VALUE_BOOLEAN_FALSE2 == privilege )
      {
         _privilege = FALSE ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid argument:%s=%s",
                             OM_CLUSTER_FIELD_PRIVILEGE, privilege.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGrantSysConfigureCommand::_checkCluster()
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: %s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGrantSysConfigureCommand::_grantSysConf()
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.updateClusterGrantConf( _clusterName, _grantName,
                                          _privilege ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "grant cluster failed:%s=%s,%s=%s,%s=%s",
                             OM_CLUSTER_FIELD_NAME, _clusterName.c_str(),
                             OM_CLUSTER_FIELD_GRANTNAME, _grantName.c_str(),
                             OM_CLUSTER_FIELD_PRIVILEGE,
                             _privilege == TRUE ? OM_VALUE_BOOLEAN_TRUE1 :
                                                     OM_VALUE_BOOLEAN_FALSE1 ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omGrantSysConfigureCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( OM_CLUSTER_FIELD_ROOTUSER == _grantName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s can not be modified",
                             OM_CLUSTER_FIELD_ROOTUSER ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _checkCluster() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_checkCluster failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _grantSysConf() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_checkCluster failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( OM_CLUSTER_FIELD_HOSTFILE == _grantName )
      {
         sdbGetOMManager()->updateClusterHostFilePrivilege( _clusterName,
                                                            _privilege ) ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   omUnbindHostCommand::omUnbindHostCommand( restAdaptor *pRestAdaptor,
                                             pmdRestSession *pRestSession )
         : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omUnbindHostCommand::~omUnbindHostCommand()
   {
   }

   INT32 omUnbindHostCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;
      list<string> hostList ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = _getRestInfo( hostList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_getRestInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _checkHost( hostList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "_checkHost failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = dbTool.unbindHost( _clusterName, hostList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to unbind host: name=%s, rc=%d",
                             _clusterName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omUnbindHostCommand::_getRestInfo( list<string> &hostList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pClusterName = NULL ;
      const CHAR *pHostInfo    = NULL ;
      BSONObj hostInfo ;
      BSONElement element ;

      _restAdaptor->getQuery( _restSession, OM_CLUSTER_FIELD_NAME,
                              &pClusterName ) ;
      if ( NULL == pClusterName || 0 == ossStrlen( pClusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed:%s is NULL",
                             OM_CLUSTER_FIELD_NAME ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      _clusterName = pClusterName ;

      _restAdaptor->getQuery( _restSession, OM_REST_FIELD_HOST_INFO,
                              &pHostInfo ) ;
      if ( NULL == pHostInfo )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                             OM_REST_FIELD_HOST_INFO ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = fromjson( pHostInfo, hostInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to convert bson: json=%s,rc=%d",
                             pHostInfo, rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      element = hostInfo.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if ( Array != element.type() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is not array type: type=%d",
                             OM_BSON_FIELD_HOST_INFO, element.type() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( element.embeddedObject() ) ;

         while ( i.more() )
         {
            BSONElement ele = i.next() ;

            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "host info is invalid, element of %s "
                                         "is not Object type:type=%d",
                                   OM_BSON_FIELD_HOST_INFO, ele.type() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               BSONObj oneHost = ele.embeddedObject() ;
               string hostName ;

               hostName = oneHost.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
               if ( 0 == hostName.length() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "rest field miss:field=%s",
                                      OM_BSON_FIELD_HOST_NAME ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }
               hostList.push_back( hostName ) ;
            }
         }
      }

      if ( 0 == hostList.size() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s no hosts", OM_BSON_FIELD_HOST_INFO ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omUnbindHostCommand::_checkHost( list<string> &hostList )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;
      list<string>::iterator iter ;

      for ( iter = hostList.begin(); iter != hostList.end(); ++iter )
      {
         string hostName = *iter ;

         if ( FALSE == dbTool.isHostExistOfClusterByAddr( hostName,
                                                          _clusterName ) )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "host does not exist: host=%s",
                                hostName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         if ( TRUE == dbTool.isConfigExistOfCluster( hostName, _clusterName ) )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "failed to unbind host, "
                                      "the host has business: host=%s",
                                hostName.c_str() ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omDeployPackageCommand::omDeployPackageCommand( restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   string &localAgentHost,
                                                   string &localAgentService )
         : omAuthCommand( pRestAdaptor, pRestSession ),
           _enforced( FALSE ),
           _localAgentHost( localAgentHost ),
           _localAgentService( localAgentService )
   {
   }

   omDeployPackageCommand::~omDeployPackageCommand()
   {
   }

   INT32 omDeployPackageCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      string packetPath ;
      BSONObj restHostInfo ;
      BSONObj clusterInfo ;
      BSONObj hostsInfo ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "sssjss|b",
                             OM_REST_FIELD_CLUSTER_NAME,  &_clusterName,
                             OM_REST_FIELD_PACKAGENAME,   &_packageName,
                             OM_REST_FIELD_INSTALLPATH,   &_installPath,
                             OM_REST_FIELD_HOST_INFO,     &restHostInfo,
                             OM_REST_FIELD_USER,          &_user,
                             OM_REST_FIELD_PASSWORD,      &_passwd,
                             OM_REST_FIELD_ENFORCED,      &_enforced ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( restHostInfo, clusterInfo, hostsInfo, packetPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      _generateRequest( clusterInfo, hostsInfo, packetPath,
                        taskConfig, resultInfo ) ;

      rc = _createTask( taskConfig, resultInfo, taskID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to create task: rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONObj result = BSON( OM_BSON_TASKID << taskID ) ;

         rc = restTool.appendResponeContent( result ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to append respone content: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omDeployPackageCommand::_check( const BSONObj &restHostInfo,
                                         BSONObj &clusterInfo,
                                         BSONObj &hostsInfo,
                                         string &packetPath )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( _cb ) ;
      BSONElement hostInfoEle = restHostInfo.getField( OM_BSON_HOST_INFO ) ;

      if ( bson::Array != hostInfoEle.type() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is invalid, type:%d",
                             OM_BSON_HOST_INFO, hostInfoEle.type() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = dbTool.getClusterInfo( _clusterName, clusterInfo ) ;
      if ( rc )
      {
         if ( SDB_DMS_RECORD_NOTEXIST == rc )
         {
            _errorMsg.setError( TRUE, "cluster is not exist: name=%s",
                                _clusterName.c_str() ) ;
         }
         else
         {
            _errorMsg.setError( TRUE, "failed to get cluster info: "
                                      "name=%s, rc=%d",
                                _clusterName.c_str(), rc ) ;
         }
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( OM_BUSINESS_SEQUOIADB == _packageName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "the host already has package: package=%s",
                             _packageName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_OLTP == _packageName )
      {
      }
      else
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid package: name=%s",
                             _packageName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = getPacketFile( _packageName, packetPath ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "%s package is not exist",
                             _packageName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      {
         INT64 taskID = 0 ;
         BSONArrayBuilder hostsInfoBuilder ;
         BSONObjIterator iter( hostInfoEle.embeddedObject() ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj tmpHostInfo = ele.embeddedObject() ;
            string hostName = tmpHostInfo.getStringField(
                                                      OM_BSON_HOSTNAME ) ;
            string user = tmpHostInfo.getStringField( OM_REST_FIELD_USER ) ;
            string pwd = tmpHostInfo.getStringField( OM_REST_FIELD_PASSWORD ) ;

            if ( FALSE == dbTool.isHostExistOfCluster( hostName,
                                                       _clusterName ) )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "host is not exist: name=%s",
                                   hostName.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            if ( FALSE == _enforced &&
                 TRUE == dbTool.isHostHasPackage( hostName, _packageName ) )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "the host already has package: "
                                         "host=%s, package=%s",
                                   hostName.c_str(), _packageName.c_str() ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            taskID = dbTool.getTaskIdOfRunningHost( hostName ) ;
            if ( 0 < taskID )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "host[%s] is exist "
                                   "in task["OSS_LL_PRINT_FORMAT"]",
                                   hostName.c_str(), taskID ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            if ( user.empty() )
            {
               user = _user ;
            }

            if ( pwd.empty() )
            {
               pwd = _passwd ;
            }

            {
               BSONObjBuilder newHostInfoBuilder ;
               BSONObj hostInfo ;
               string tmpHostName ;
               string tmpIp ;
               string tmpAgentService ;
               string tmpSshPort ;

               rc = dbTool.getHostInfoByAddress( hostName, hostInfo ) ;
               if ( rc )
               {
                  _errorMsg.setError( TRUE, "failed to get host info: host=%s",
                                      hostName.c_str() ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( TRUE == _enforced )
               {
                  BSONObj packages = hostInfo.getObjectField(
                                                      OM_HOST_FIELD_PACKAGES ) ;
                  BSONObjIterator pkgIter( packages ) ;

                  while ( pkgIter.more() )
                  {
                     BSONElement ele = pkgIter.next() ;
                     BSONObj pkgInfo = ele.embeddedObject() ;
                     string installPath = pkgInfo.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;

                     if ( _packageName == pkgInfo.getStringField(
                                                OM_HOST_FIELD_PACKAGENAME ) &&
                          _installPath != installPath )
                     {
                        rc = SDB_INVALIDARG ;
                        _errorMsg.setError( TRUE, " the host already install "
                                                  "package, forced install, "
                                                  "path cannot be change: "
                                                  "host=%s, install path=%s, "
                                                  "new install path=%s",
                                            hostName.c_str(),
                                            installPath.c_str(),
                                            _installPath.c_str() ) ;
                        PD_LOG( PDERROR, _errorMsg.getError() ) ;
                        goto error ;
                     }
                  }
               }

               tmpHostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
               tmpIp = hostInfo.getStringField( OM_HOST_FIELD_IP ) ;
               tmpAgentService = hostInfo.getStringField(
                                                   OM_HOST_FIELD_AGENT_PORT ) ;
               tmpSshPort = hostInfo.getStringField( OM_HOST_FIELD_SSHPORT ) ;

               newHostInfoBuilder.append( OM_HOST_FIELD_NAME, tmpHostName ) ;
               newHostInfoBuilder.append( OM_HOST_FIELD_IP, tmpIp ) ;
               newHostInfoBuilder.append( OM_HOST_FIELD_SSHPORT, tmpSshPort ) ;
               newHostInfoBuilder.append( OM_HOST_FIELD_USER, user ) ;
               newHostInfoBuilder.append( OM_HOST_FIELD_PASSWORD, pwd ) ;

               hostsInfoBuilder.append( newHostInfoBuilder.obj() ) ;
            }
         }

         hostsInfo = BSON( OM_BSON_HOST_INFO << hostsInfoBuilder.arr() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omDeployPackageCommand::_generateRequest( const BSONObj &clusterInfo,
                                                  const BSONObj &hostInfo,
                                                  const string &packetPath,
                                                  BSONObj &taskConfig,
                                                  BSONArray &resultInfo )
   {
      BSONObjBuilder taskConfigBuilder ;
      BSONArrayBuilder resultInfoBuilder ;
      BSONElement hostInfoEle = hostInfo.getField( OM_BSON_HOST_INFO ) ;
      string sdbUser = clusterInfo.getStringField( OM_CLUSTER_FIELD_SDBUSER ) ;
      string sdbPasswd = clusterInfo.getStringField(
                                                OM_CLUSTER_FIELD_SDBPASSWD ) ;
      string sdbUserGroup = clusterInfo.getStringField(
                                             OM_CLUSTER_FIELD_SDBUSERGROUP ) ;
      string omVersion ;

      taskConfigBuilder.append( OM_TASKINFO_FIELD_CLUSTERNAME, _clusterName ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBUSER, sdbUser ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBPASSWD, sdbPasswd ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBUSERGROUP, sdbUserGroup ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_INSTALLPACKET, packetPath ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_PACKAGENAME, _packageName ) ;
      taskConfigBuilder.append( OM_TASKINFO_FIELD_INSTALLPATH, _installPath ) ;
      taskConfigBuilder.appendBool( OM_TASKINFO_FIELD_ENFORCED, _enforced ) ;

      {
         BSONArrayBuilder hostInfoBuilder ;
         BSONObjIterator iter( hostInfoEle.embeddedObject() ) ;

         while ( iter.more() )
         {
            BSONObjBuilder resultEleBuilder ;
            BSONElement ele = iter.next() ;
            BSONObj tmpHostInfo = ele.embeddedObject() ;
            string hostName = tmpHostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
            string hostIP = tmpHostInfo.getStringField( OM_HOST_FIELD_IP ) ;

            hostInfoBuilder.append( tmpHostInfo ) ;

            resultEleBuilder.append( OM_TASKINFO_FIELD_HOSTNAME, hostName ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_IP, hostIP ) ;

            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS,
                                     OM_TASK_STATUS_INIT ) ;
            resultEleBuilder.append( OM_TASKINFO_FIELD_STATUS_DESC,
                                     getTaskStatusStr( OM_TASK_STATUS_INIT ) ) ;
            resultEleBuilder.append( OM_REST_RES_RETCODE, SDB_OK ) ;
            resultEleBuilder.append( OM_REST_RES_DETAIL, "" ) ;

            {
               BSONArrayBuilder tmpEmptyBuilder ;

               resultEleBuilder.append( OM_TASKINFO_FIELD_FLOW,
                                        tmpEmptyBuilder.arr() ) ;
            }

            resultInfoBuilder.append( resultEleBuilder.obj() ) ;
         }

         taskConfigBuilder.append( OM_TASKINFO_FIELD_HOSTINFO,
                                   hostInfoBuilder.arr() ) ;
      }

      resultInfo = resultInfoBuilder.arr() ;
      taskConfig = taskConfigBuilder.obj() ;
   }

   INT32 omDeployPackageCommand::_createTask( const BSONObj &taskConfig,
                                              const BSONArray &resultInfo,
                                              INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      getMaxTaskID( taskID ) ;
      taskID++ ;

      rc = taskTool.createTask( OM_TASK_TYPE_DEPLOY_PACKAGE, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_DEPLOY_PACKAGE ),
                                taskConfig, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "fail to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "fail to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omRegisterPluginsCommand::omRegisterPluginsCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession )
                     : omRestCommandBase( pRestAdaptor, pRestSession )
   {
   }

   omRegisterPluginsCommand::~omRegisterPluginsCommand()
   {
   }

   INT32 omRegisterPluginsCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      string pluginName ;
      string hostName ;
      string serviceName ;
      string role ;
      string businessType ;
      string publicKey ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "ssssss",
                             OM_REST_FIELD_NAME,            &pluginName,
                             OM_REST_FIELD_HOST_NAME,       &hostName,
                             OM_REST_FIELD_SERVICE_NAME,    &serviceName,
                             OM_REST_FIELD_ROLE,            &role,
                             OM_REST_FIELD_TYPE,            &businessType,
                             OM_REST_FIELD_PUBLIC_KEY,      &publicKey ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( role, publicKey ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _updatePlugin( restTool, pluginName, businessType,
                          publicKey, serviceName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add plugin: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omRegisterPluginsCommand::_check( const string &role,
                                           const string &publicKey )
   {
      INT32 rc = SDB_OK ;
      INT32 length = 0 ;

      if ( OM_REST_VALUE_PLUGIN != role )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid parameter: name=%s",
                             OM_REST_FIELD_ROLE ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      length = publicKey.length() ;
      if ( 8 != length )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid parameter length: name=%s",
                             OM_REST_FIELD_PUBLIC_KEY ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for( INT32 i = 0; i < length; ++i )
      {
         if( ( publicKey[i] < '0' || publicKey[i] > '9' ) &&
             ( publicKey[i] < 'a' || publicKey[i] > 'z' ) &&
             ( publicKey[i] < 'A' || publicKey[i] > 'Z' ) )
         {
            rc = SDB_INVALIDARG ;
            _errorMsg.setError( TRUE, "invalid parameter: name=%s",
                                OM_REST_FIELD_PUBLIC_KEY ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRegisterPluginsCommand::_updatePlugin( omRestTool &restTool,
                                                  const string &pluginName,
                                                  const string &businessType,
                                                  const string &publicKey,
                                                  const string &serviceName )
   {
      INT32 rc = SDB_OK ;
      INT64 timeDiffer = 0 ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      omDatabaseTool dbTool( _cb ) ;
      omAuthTool authTool( _cb, pAuthCB ) ;
      string user ;
      string passwd ;
      string userEncrypt ;
      string passwdEncrypt ;
      string svcname ;

      rc = dbTool.upsertPlugin( pluginName, businessType, serviceName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to update plugin info: "
                                   "name=%s, type=%s, service=%s",
                             pluginName.c_str(), businessType.c_str(),
                             serviceName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      user = OM_DEFAULT_PLUGIN_USER ;
      sdbGetOMManager()->getPluginPasswd( passwd ) ;

      rc = _encrypt( publicKey, user, userEncrypt ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to encrypt: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _encrypt( publicKey, passwd, passwdEncrypt ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to encrypt: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      sdbGetOMManager()->getUpdatePluginPasswdTimeDiffer( timeDiffer ) ;

      {
         stringstream ss ;

         ss << pmdGetKRCB()->getOptionCB()->getServicePort() ;
         svcname = ss.str() ;
      }

      {
         BSONObj result = BSON( OM_BSON_USER2      << userEncrypt <<
                                OM_BSON_PASSWD2    << passwdEncrypt <<
                                OM_BSON_LEASETIME2 << timeDiffer <<
                                OM_BSON_SVCNAME    << svcname ) ;

         restTool.appendResponeMsg( result ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #define OM_ENCRYPT_PUBLIC_KEY_LEN 8
   #define OM_ENCRYPT_PRIVATE_KEY_SIZE 17
   INT32 omRegisterPluginsCommand::_encrypt( const string &publicKey,
                                             const string &src,
                                             string &dest )
   {
      INT32 rc = SDB_OK ;
      INT32 encryptSize = 0 ;
      BYTE *pCiphertext = NULL ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      const CHAR *pPublicKey = publicKey.c_str() ;
      omDatabaseTool dbTool( _cb ) ;
      omAuthTool authTool( _cb, pAuthCB ) ;
      CHAR privateKey[OM_ENCRYPT_PRIVATE_KEY_SIZE] ;
      md5::md5digest digest ;
      string md5String ;

      ossMemset( privateKey, 0, OM_ENCRYPT_PRIVATE_KEY_SIZE ) ;
      authTool.generateRandomVisualString( privateKey, OM_ENCRYPT_PUBLIC_KEY_LEN ) ;

      ossMemcpy( privateKey + OM_ENCRYPT_PUBLIC_KEY_LEN, pPublicKey,
                 OM_ENCRYPT_PUBLIC_KEY_LEN ) ;

      md5::md5( (const void*)privateKey, OM_ENCRYPT_PRIVATE_KEY_SIZE - 1,
                digest ) ;

      md5String = md5::digestToString( digest ) ;

      encryptSize = desEncryptSize( src.length() ) + OM_ENCRYPT_PUBLIC_KEY_LEN ;
      pCiphertext = (BYTE *)SDB_OSS_MALLOC( encryptSize + 1 ) ;
      if ( NULL == pCiphertext )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "out of memory" ) ;
         goto error ;
      }

      ossMemset( pCiphertext, 0, encryptSize + 1 ) ;

      rc = desEncrypt( (BYTE *)md5String.c_str(),
                       (BYTE *)src.c_str(), src.length(),
                       pCiphertext + OM_ENCRYPT_PUBLIC_KEY_LEN ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to call desEncrypt: rc=%d", rc ) ;
         goto error ;
      }

      ossMemcpy( pCiphertext, privateKey, OM_ENCRYPT_PUBLIC_KEY_LEN ) ;

      dest = base64::encode( (const CHAR *)pCiphertext, encryptSize ) ;

   done:
      if ( pCiphertext )
      {
         SDB_OSS_FREE( pCiphertext ) ;
         pCiphertext = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   omListPluginsCommand::omListPluginsCommand( restAdaptor *pRestAdaptor,
                                               pmdRestSession *pRestSession )
                     : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omListPluginsCommand::~omListPluginsCommand()
   {
   }

   INT32 omListPluginsCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> pluginList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = dbTool.getPluginList( pluginList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get plugin info: rc=%d",
                             rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = pluginList.begin(); iter != pluginList.end(); ++iter )
      {
         BSONObj pluginInfo = *iter ;

         rc = restTool.appendResponeContent( pluginInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to build respone: rc=%d",
                                rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendRespone( rc, _errorMsg.getError() ) ;
      goto done ;
   }
}

