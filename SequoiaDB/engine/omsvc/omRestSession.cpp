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

   Source File Name = omRestSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/29/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omRestSession.hpp"
#include "pmdController.hpp"
#include "omManager.hpp"
#include "pmdEDUMgr.hpp"
#include "msgDef.h"
#include "utilCommon.hpp"
#include "ossMem.hpp"
#include "rtnCommand.hpp"
#include "omCommand.hpp"
#include "omTransferProcessor.hpp"
#include "rtn.hpp"
#include "msgAuth.hpp"
#include "pmdTrace.hpp"
#include "../bson/bson.h"
#include "../bson/lib/md5.hpp"

using namespace bson ;

namespace engine
{

   _omRestSession::_omRestSession( SOCKET fd )
                  :_pmdRestSession( fd )
   {
   }

   _omRestSession::~_omRestSession()
   {
   }

   SDB_SESSION_TYPE _omRestSession::sessionType() const
   {
      return SDB_SESSION_REST ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_OMRESTSN_PROMSG, "_omRestSession::_processMsg" )
   INT32 _omRestSession::_processMsg( HTTP_PARSE_COMMON command, 
                                      const CHAR *pFilePath )
   {
      INT32 rc = SDB_OK ;
      restAdaptor *pAdaptor   = sdbGetPMDController()->getRestAdptor() ;
      const CHAR *pSubCommand = NULL ;

      pAdaptor->getQuery( this, OM_REST_FIELD_COMMAND, &pSubCommand ) ;
      if ( NULL != pSubCommand )
      {
         if ( ossStrcasecmp( pSubCommand, OM_REGISTER_PLUGIN_REQ ) == 0 )
         {
            rc = _registerPlugin( pAdaptor ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "register plugin failed:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
            }
            goto done ;
         }
         else if ( ossStrcasecmp( pSubCommand, OM_LOGOUT_REQ ) == 0 )
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
         else if ( ossStrcasecmp( pSubCommand, OM_LOGIN_REQ ) == 0 )
         {
            rc = _processBusinessMsg( pAdaptor ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "login failed:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
            }

            goto done ;
         }
      }

      if ( COM_GETFILE == command )
      {
         rc = _actionGetFile( pFilePath ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get file: rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _actionCmd( pFilePath ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to exec common: rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_queryTable( const string &tableName, 
                                      const BSONObj &selector, 
                                      const BSONObj &matcher,
                                      const BSONObj &order, 
                                      const BSONObj &hint, SINT32 flag,
                                      SINT64 numSkip, SINT64 numReturn, 
                                      list<BSONObj> &records )
   {
      INT32 rc         = SDB_OK ;
      SINT64 contextID = -1 ;
      pmdKRCB *krcb    = pmdGetKRCB() ;
      SDB_DMSCB *dmscb = krcb->getDMSCB() ;
      SDB_RTNCB *rtncb = krcb->getRTNCB() ;
      rc = rtnQuery( tableName.c_str(), selector, matcher, order, hint, flag, 
                     eduCB(), numSkip, numReturn, dmscb, rtncb, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:name=%s,rc=%d", 
                     tableName.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, eduCB(), rtncb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               _pRTNCB->contextDelete( contextID, eduCB() ) ;
               contextID = -1 ;
               PD_LOG_MSG( PDERROR, "failed to get record from table:name=%s,"
                           "rc=%d", tableName.c_str(), rc ) ;
               goto error ;   
            }

            rc = SDB_OK ;
            goto done ;
         }

         BSONObj result( buffObj.data() ) ;
         records.push_back( result.copy() ) ;
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, eduCB() ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _omRestSession::_isClusterExist( const CHAR *pClusterName )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      list<BSONObj> records ;

      matcher = BSON( OM_CLUSTER_FIELD_NAME << pClusterName ) ;
      rc = _queryTable( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order, 
                        hint, 0, 0, -1, records ) ;
      if ( SDB_OK == rc && records.size() == 1 )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _omRestSession::_getBusinessInfo( const CHAR *pClusterName,
                                           const CHAR *pBusinessName,
                                           string &businessType, 
                                           string &deployMode )
   {
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      list<BSONObj> records ;
      BSONObj result ;
      INT32 rc = SDB_OK ;

      if ( !_isClusterExist( pClusterName ) )
      {
         rc = SDB_OM_CLUSTER_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "cluster is not exist:cluster=%s", 
                     pClusterName ) ;
         goto error ;
      }

      matcher = BSON( OM_BUSINESS_FIELD_CLUSTERNAME << pClusterName 
                      << OM_BUSINESS_FIELD_NAME << pBusinessName ) ;
      rc = _queryTable( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order, 
                        hint, 0, 0, -1, records ) ;
      if ( rc )
      {
         rc = SDB_OM_BUSINESS_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         goto error ;
      }

      if ( records.size() != 1 )
      {
         rc = SDB_OM_BUSINESS_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "business[%s] do not exist in cluster[%s]", 
                     pBusinessName, pClusterName ) ;
         goto error ;
      }

      result = *( records.begin() ) ;
      businessType = result.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      deployMode   = result.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_getBusinessAuth( const CHAR *pClusterName,
                                           const CHAR *pBusinessName,
                                           string &user, string &passwd )
   {
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      list<BSONObj> records ;
      BSONObj result ;
      INT32 rc = SDB_OK ;

      matcher = BSON( OM_AUTH_FIELD_BUSINESS_NAME << pBusinessName ) ;
      rc = _queryTable( OM_CS_DEPLOY_CL_BUSINESS_AUTH, selector, matcher, order, 
                        hint, 0, 0, -1, records ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
         goto error ;
      }

      if ( records.size() > 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get business authority info failed:cluster=%s,"
                     "business=%s,expect_num=1,actual_num=%u", pClusterName, 
                     pBusinessName, records.size() ) ;
         goto error ;
      }
      else if ( records.size() == 1 )
      {
         result = *( records.begin() ) ;
         user   = result.getStringField( OM_AUTH_FIELD_USER ) ;
         passwd = result.getStringField( OM_AUTH_FIELD_PASSWD ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   } 

   INT32 _omRestSession::_getBusinessAccessNode( const CHAR *pClusterName,
                                                 const CHAR *pBusinessName,
                                                 const CHAR *pSdbUser,
                                                 const CHAR *pSdbPasswd,
                                                 list<omNodeInfo> &nodeList )
   {
      INT32 rc = SDB_OK ;
      string businessType ;
      string deployMode ;
      string user ;
      string passwd ;
      rc = _getBusinessInfo( pClusterName, pBusinessName, businessType, 
                             deployMode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get business info failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( businessType != OM_BUSINESS_SEQUOIADB )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "businessType is not %s:clusterName=%s,"
                     "businessName=%s", OM_BUSINESS_SEQUOIADB, pClusterName,
                     pBusinessName ) ;
         goto error ;
      }

      if ( NULL == pSdbUser || NULL == pSdbPasswd )
      {
         md5::md5digest digest ;
         string tmpPasswd ;
         rc = _getBusinessAuth( pClusterName, pBusinessName, user, tmpPasswd ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "get business auth failed:rc=%d", rc ) ;
            goto error ;
         }
      
         md5::md5( ( const void * )tmpPasswd.c_str(), 
                   tmpPasswd.length(), digest) ;
         passwd = md5::digestToString( digest ) ;
      }
      else
      {
         user   = pSdbUser ;
         passwd = pSdbPasswd ;
      }

      {
         BSONObj selector ;
         BSONObj matcher ;
         BSONObj order ;
         BSONObj hint ;
         list<BSONObj>::iterator iter ;
         list<BSONObj> records ;
         BSONObj result ;

         matcher = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << pBusinessName ) ;
         rc = _queryTable( OM_CS_DEPLOY_CL_CONFIGURE, selector, matcher, order, 
                           hint, 0, 0, -1, records ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }

         if ( records.size() == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "get business configure info failed:"
                        "cluster=%s,business=%s", pClusterName, 
                        pBusinessName ) ;
            goto error ;
         }

         iter = records.begin() ;
         while ( iter != records.end() )
         {
            BSONElement confEle = iter->getField( OM_CONFIGURE_FIELD_CONFIG ) ;
            if ( confEle.type() != Array )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "ele is not array type:type=%d", 
                       confEle.type() ) ;
               goto error ;
            }
            BSONObjIterator bsonIter( confEle.embeddedObject() ) ;
            while ( bsonIter.more() )
            {
               BSONElement ele = bsonIter.next() ;
               if ( ele.type() != Object )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "ele is not Object type:type=%d", 
                          ele.type() ) ;
                  goto error ;
               }
               BSONObj oneConfig = ele.embeddedObject() ;
               string role = oneConfig.getStringField( OM_CONF_DETAIL_ROLE ) ;
               if ( role == SDB_ROLE_STANDALONE_STR 
                    || role == SDB_ROLE_COORD_STR )
               {
                  _omNodeInfo node ;
                  node.hostName = 
                           iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
                  node.service  = 
                           oneConfig.getStringField( OM_CONF_DETAIL_SVCNAME ) ;
                  node.user     = user ;
                  node.passwd   = passwd ;
                  if ( role == SDB_ROLE_STANDALONE_STR )
                  {
                     node.preferedInstance = 1 ;
                  }
                  else
                  {
                     node.preferedInstance = 8 ;
                  }
                  nodeList.push_back( node ) ;
               }
            }
            
            iter++ ;
         }
      }

      if ( 0 == nodeList.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "business no node: name=%s", pBusinessName ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_registerPlugin( restAdaptor *pAdaptor )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pOmCommand = NULL ;

      pOmCommand = SDB_OSS_NEW omRegisterPluginsCommand( pAdaptor, this ) ;
      if ( NULL == pOmCommand )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      pOmCommand->init( _pEDUCB ) ;

      pOmCommand->doCommand() ;

   done:
      if ( pOmCommand )
      {
         SDB_OSS_DEL pOmCommand ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _omRestSession::_setSpecifyNode( const CHAR *pSdbHostName,
                                          const CHAR *pSdbSvcName,
                                          list<omNodeInfo> &nodeList )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isFind = FALSE ;
      list<omNodeInfo>::iterator iter ;

      if( ( pSdbHostName == NULL && pSdbSvcName != NULL ) ||
          ( pSdbHostName != NULL && pSdbSvcName == NULL ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "fail to get %s,rc=%d",
                 pSdbHostName == NULL ?
                 OM_REST_HEAD_SDBHOSTNAME :OM_REST_HEAD_SDBSERVICENAME, rc ) ;
         goto error ;
      }

      if( pSdbHostName != NULL && pSdbSvcName != NULL )
      {
         for( iter = nodeList.begin(); iter != nodeList.end(); ++iter )
         {
            if( iter->hostName.compare( pSdbHostName ) == 0 &&
                iter->service.compare( pSdbSvcName ) == 0 )
            {
               omNodeInfo tmpNodeInfo = *iter ;
               nodeList.erase( iter ) ;
               nodeList.push_front( tmpNodeInfo ) ;
               isFind = TRUE ;
               break ;
            }
         }
         if( isFind == FALSE )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "specified node does not exist,"
                    " %s: %s, %s: %s, rc=%d",
                    OM_REST_HEAD_SDBHOSTNAME, pSdbHostName,
                    OM_REST_HEAD_SDBSERVICENAME, pSdbSvcName, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_processSdbTransferMsg( restAdaptor *pAdaptor,
                                                 const CHAR *pClusterName,
                                                 const CHAR *pBusinessName )
   {
      INT32 rc        = SDB_OK ;
      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      BOOLEAN needReplay = FALSE ;
      MsgHeader *msg = NULL ;
      _omTransferProcessor *transProcessor = NULL ;
      list<omNodeInfo> nodeList ;
      const CHAR *pSdbUser      = NULL ;
      const CHAR *pSdbPasswd    = NULL ;
      const CHAR *pSdbHostName  = NULL ;
      const CHAR *pSdbSvcName   = NULL ;
      
      if ( !isAuthOK() )
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         PD_LOG( PDERROR, "session is not exist:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, eduCB() ) ;
         goto error ;
      }
      
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBUSER, &pSdbUser ) ;
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBPASSWD, &pSdbPasswd ) ;
      rc = _getBusinessAccessNode( pClusterName, pBusinessName, 
                                   pSdbUser, pSdbPasswd, nodeList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get business info failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
         goto error ;
      }

      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBHOSTNAME,
                               &pSdbHostName ) ;
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_SDBSERVICENAME,
                               &pSdbSvcName ) ;
      rc = _setSpecifyNode( pSdbHostName, pSdbSvcName, nodeList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "set specify node failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
         goto error ;
      }

      transProcessor = SDB_OSS_NEW _omTransferProcessor( nodeList ) ;
      if ( NULL == transProcessor )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "new _omTransferProcessor failed" ) ;
         goto error ;
      }
      transProcessor->attach( this ) ;
      rc = _translateMSG( pAdaptor, &msg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "translate message failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
         goto error ;
      }

      rc = transProcessor->processMsg( msg, contextBuff, contextID, 
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
         rtnContextBuf fetchOneBuff ;
         if ( -1 != contextID )
         {
            rc = _fetchOneContext( contextID, fetchOneBuff ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "fetch context failed:rc=%d", rc ) ;
               disconnect() ;
               goto error ;
            }
            if ( -1 != contextID )
            {
               pAdaptor->setChunkModal( this ) ;
            }
         }

         BSONObj tmp = BSON( OM_REST_RES_RETCODE << rc ) ;
         pAdaptor->setOPResult( this, rc, tmp ) ;
         if ( 0 != contextBuff.recordNum() )
         {
            pAdaptor->appendHttpBody( this, contextBuff.data(), 
                                      contextBuff.size(), 
                                      contextBuff.recordNum() ) ;
         }

         if ( 0 != fetchOneBuff.recordNum() )
         {
            pAdaptor->appendHttpBody( this, fetchOneBuff.data(), 
                                      fetchOneBuff.size(), 
                                      fetchOneBuff.recordNum() ) ;
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
                     disconnect() ;
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
                     disconnect() ;
                     goto error ;
                  }

                  rc = SDB_OK ;
                  break ;
               }
            }
         }
      }

      pAdaptor->sendResponse( this, HTTP_OK ) ;
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete( contextID, _pEDUCB ) ;
         contextID = -1 ;
      }
      if ( NULL != transProcessor )
      {
         transProcessor->detach() ;
         SDB_OSS_DEL transProcessor ;
         transProcessor = NULL ;
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

   INT32 _omRestSession::_processOMRestMsg( const CHAR *pFilePath )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pOmCommand = NULL ;
      pOmCommand = _createCommand( pFilePath ) ;
      if ( NULL == pOmCommand )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pOmCommand->init( _pEDUCB ) ;
      pOmCommand->doCommand() ;

   done:
      if ( NULL != pOmCommand )
      {
         SDB_OSS_DEL pOmCommand ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _omRestSession::_actionGetFile( const CHAR *pFilePath )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pCommand = NULL ;
      restAdaptor *pAdptor = sdbGetPMDController()->getRestAdptor() ;

      PD_LOG( PDDEBUG, "OM: getfile command:file=%s", pFilePath ) ;

      pCommand = SDB_OSS_NEW omGetFileCommand( pAdptor, this,
                                               _wwwRootPath.c_str(),
                                               pFilePath ) ;
      if ( NULL == pCommand )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      pCommand->init( _pEDUCB ) ;

      pCommand->doCommand() ;

   done:
      if ( NULL != pCommand )
      {
         SDB_OSS_DEL pCommand ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_forwardPlugin( restAdaptor *pAdptor,
                                         const string &businessType )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pCommand = NULL ;

      pCommand = SDB_OSS_NEW omForwardPluginCommand( pAdptor, this,
                                                     businessType ) ;
      if ( NULL == pCommand )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      pCommand->init( _pEDUCB ) ;

      pCommand->doCommand() ;

   done:
      if ( NULL != pCommand )
      {
         SDB_OSS_DEL pCommand ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_actionCmd( const CHAR *pFilePath )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusinessName = NULL ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;

      pAdaptor->getHttpHeader( this, OM_REST_HEAD_CLUSTERNAME, &pClusterName ) ;
      pAdaptor->getHttpHeader( this, OM_REST_HEAD_BUSINESSNAME, 
                               &pBusinessName ) ;
      if ( NULL == pClusterName || NULL == pBusinessName )
      {
         rc = _processOMRestMsg( pFilePath ) ;
      }
      else
      {
         string businessType ;
         string deployMode ;

         rc = _getBusinessInfo( pClusterName, pBusinessName, businessType, 
                                deployMode ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "get business info failed:cluster=%s,"
            "business=%s,rc=%d", pClusterName, pBusinessName, rc ) ;
            _sendOpError2Web( rc, pAdaptor, this, _pEDUCB ) ;
            goto error ;
         }

         if ( OM_BUSINESS_SEQUOIADB == businessType )
         {
            rc = _processSdbTransferMsg( pAdaptor, pClusterName, 
                                         pBusinessName ) ;
         }
         else
         {
            rc = _forwardPlugin( pAdaptor, businessType ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omRestCommandBase *_omRestSession::_createCommand( const CHAR *pFilePath )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *commandIf = NULL ;
      restAdaptor *pAdptor         = NULL ;
      const CHAR* hostName = pmdGetKRCB()->getHostName();
      pAdptor = sdbGetPMDController()->getRestAdptor() ;
      string localAgentHost = hostName ;
      string localAgentPort = sdbGetOMManager()->getLocalAgentPort() ;

      const CHAR *pSubCommand = NULL ;
      pAdptor->getQuery( this, OM_REST_FIELD_COMMAND, &pSubCommand ) ;
      if ( NULL == pSubCommand )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get command failed:filed=%s,rc=%d",
                     OM_REST_FIELD_COMMAND, rc ) ;
         _sendOpError2Web( rc, pAdptor, this, eduCB() ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "OM: command:command=%s", pSubCommand ) ;
      if ( ossStrcmp( pSubCommand, OM_LOGIN_REQ ) != 0
           && ossStrcmp( pSubCommand, OM_CHECK_SESSION_REQ ) != 0
           && !isAuthOK() )
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         PD_LOG( PDERROR, "session is not exist:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdptor, this, eduCB() ) ;

         PD_LOG( PDEVENT, "OM: redirect to:%s", OM_REST_LOGIN_HTML ) ;
         goto error ;
      }

      if ( ossStrcasecmp( pSubCommand, OM_CHANGE_PASSWD_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omChangePasswdCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_CHECK_SESSION_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omCheckSessionCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_CREATE_CLUSTER_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omCreateClusterCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_QUERY_CLUSTER_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryClusterCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_SCAN_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omScanHostCommand( pAdptor, this, 
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_CHECK_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omCheckHostCommand( pAdptor, this, 
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_ADD_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omAddHostCommand( pAdptor, this, 
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListHostCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_QUERY_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryHostCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_BUSINESS_TYPE_REQ ) 
                                                                       == 0 )
      {
         commandIf = SDB_OSS_NEW omListBusinessTypeCommand( pAdptor, this, 
                                    _wwwRootPath.c_str(), pFilePath ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_GET_BUSINESS_TEMPLATE_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGetBusinessTemplateCommand( pAdptor, 
                                    this, _wwwRootPath.c_str(), pFilePath ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_CONFIG_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGetBusinessConfigCommand( pAdptor, this, 
                                                             _wwwRootPath ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_INSTALL_BUSINESS_REQ) == 0 )
      {
         string filePath( pFilePath ) ;

         commandIf = SDB_OSS_NEW omAddBusinessCommand( pAdptor, this, 
                                    _wwwRootPath, filePath, 
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_NODE_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListNodeCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_GET_NODE_CONF_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGetNodeConfCommand( pAdptor, this ) ;
      }
      else if( ossStrcasecmp( pSubCommand, OM_QUERY_NODE_CONF_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryNodeConfCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListBusinessCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_QUERY_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryBusinessCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_REMOVE_CLUSTER_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omRemoveClusterCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_REMOVE_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omRemoveHostCommand( pAdptor, this,
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_REMOVE_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omRemoveBusinessCommand( pAdptor, this,
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_QUERY_HOST_STATUS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryHostStatusCommand( pAdptor, this,
                                    localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_PREDICT_CAPACITY_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omPredictCapacity( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_TASK_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListTaskCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_QUERY_TASK_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryTaskCommand( pAdptor, this, 
                                           localAgentHost, localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_GET_LOG_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGetLogCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_SET_BUSINESS_AUTH_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omSetBusinessAuthCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_REMOVE_BUSINESS_AUTH_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omRemoveBusinessAuthCommand( pAdptor, 
                                                              this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_QUERY_BUSINESS_AUTH_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omQueryBusinessAuthCommand( pAdptor, 
                                                             this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_DISCOVER_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omDiscoverBusinessCommand( pAdptor, this,
                                                            localAgentHost,
                                                            localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_UNDISCOVER_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omUnDiscoverBusinessCommand( pAdptor, 
                                                              this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_SSQL_EXEC_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omSsqlExecCommand( pAdptor, this, 
                                                    localAgentHost,
                                                    localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_INTERRUPT_TASK_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omInterruptTaskCommand( pAdptor, this, 
                                                         localAgentHost,
                                                         localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_LIST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyList( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_ADD_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyInsert( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_UPDATE_NICE_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyUpdateNice( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_ADD_IPS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyAddIps( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_DEL_IPS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyDelIps( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_DEL_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyDel( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_STRATEGY_UPDATE_STAT_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyUpdateStatus( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand,
                               OM_TASK_STRATEGY_UPDATE_SORT_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyUpdateSortID( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_ADD_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyTaskInsert( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_LIST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyTaskList( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_UPDATE_STATUS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyUpdateTaskStatus( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, 
                               OM_TASK_DEL_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyTaskDel( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand,
                               OM_TASK_STRATEGY_FLUSH ) == 0 )
      {
         commandIf = SDB_OSS_NEW omStrategyFlush( pAdptor, this ) ;
      }

      else if ( ossStrcasecmp( pSubCommand, 
                               OM_LIST_HOST_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListHostBusinessCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_UPDATE_HOST_INFO_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omUpdateHostInfoCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_GET_SYSTEM_INFO_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGetSystemInfoCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_EXTEND_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omExtendBusinessCommand(
                                                      pAdptor, this,
                                                      _wwwRootPath.c_str(),
                                                      localAgentHost,
                                                      localAgentPort ) ;
      }
      else if( ossStrcasecmp( pSubCommand, OM_SHRINK_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omShrinkBusinessCommand(
                                                      pAdptor, this,
                                                      _wwwRootPath.c_str(),
                                                      localAgentHost,
                                                      localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand,
                               OM_SYNC_BUSINESS_CONF_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omSyncBusinessConfigureCommand(
                                                           pAdptor,
                                                           this,
                                                           localAgentHost,
                                                           localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_GRANT_SYSCONF_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omGrantSysConfigureCommand( pAdptor,
                                                             this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_UNBIND_BUSINESS_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omUnbindBusinessCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_UNBIND_HOST_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omUnbindHostCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_DEPLOY_PACKAGE_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omDeployPackageCommand( pAdptor, this,
                                                         localAgentHost,
                                                         localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand,
                               OM_CREATE_RELATIONSHIP_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omCreateRelationshipCommand(
                                                         pAdptor, this,
                                                         localAgentHost,
                                                         localAgentPort ) ;
      }
      else if ( ossStrcasecmp( pSubCommand,
                               OM_REMOVE_RELATIONSHIP_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omRemoveRelationshipCommand(
                                                         pAdptor, this,
                                                         localAgentHost,
                                                         localAgentPort ) ;

      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_RELATIONSHIP_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListRelationshipCommand( pAdptor, this ) ;
      }
      else if ( ossStrcasecmp( pSubCommand, OM_LIST_PLUGIN_REQ ) == 0 )
      {
         commandIf = SDB_OSS_NEW omListPluginsCommand( pAdptor, this ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "command is unrecognized:command=%s,rc=%d",
                     pSubCommand, rc ) ;
         _sendOpError2Web( rc, pAdptor, this, eduCB() ) ;
         goto error ;
      }

   done:
      return commandIf ;
   error:
      goto done ;
   }
}

