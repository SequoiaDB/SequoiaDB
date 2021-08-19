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
   INT32 _omRestSession::_processMsg( restRequest &request,
                                      restResponse &response )
   {
      //PD_TRACE_ENTRY( SDB_OMRESTSN_PROMSG );
      INT32 rc = SDB_OK ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;
      string subCommand ;

      subCommand = request.getQuery( OM_REST_FIELD_COMMAND ) ;
      if ( FALSE == subCommand.empty() )
      {
         if ( ossStrcasecmp( subCommand.c_str(), OM_REGISTER_PLUGIN_REQ ) == 0 )
         {
            rc = _registerPlugin( request, response ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "register plugin failed:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
            }
            goto done ;
         }
         else if ( ossStrcasecmp( subCommand.c_str(), OM_LOGOUT_REQ ) == 0 )
         {
            if ( isAuthOK() )
            {
               doLogout() ;
               pAdaptor->sendRest( socket(), &response ) ;
               goto done ;
            }
            else
            {
               rc = SDB_PMD_SESSION_NOT_EXIST ;
               PD_LOG_MSG( PDERROR, "session does not exist:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
               goto done ;
            }
         }
         else if ( ossStrcasecmp( subCommand.c_str(), OM_LOGIN_REQ ) == 0 )
         {
            rc = _processBusinessMsg( pAdaptor, request, response ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "login failed:rc=%d", rc ) ;
               _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
            }

            goto done ;
         }
      }

      if ( COM_GETFILE == request.getCommand() )
      {
         //get file
         rc = _actionGetFile( request, response ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get file: rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         //close keep-alive
         response.setConnectionClose() ;

         rc = _actionCmd( request, response ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to exec common: rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      //PD_TRACE_EXITRC ( SDB_OMRESTSN_PROMSG, rc );
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
         PD_LOG_MSG( PDERROR, "cluster does not exist:cluster=%s", 
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
      //if do not have any record. just leave it empty
   done:
      return rc ;
   error:
      goto done ;
   } 

   INT32 _omRestSession::_getBusinessAccessNode( restRequest &request,
                                                 const CHAR *pClusterName,
                                                 const CHAR *pBusinessName,
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

      if ( request.isHeaderExist( OM_REST_HEAD_SDBUSER ) &&
           request.isHeaderExist( OM_REST_HEAD_SDBPASSWD ) )
      {
         user   = request.getHeader( OM_REST_HEAD_SDBUSER ) ;
         passwd = request.getHeader( OM_REST_HEAD_SDBPASSWD ) ;
      }
      else
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

   INT32 _omRestSession::_registerPlugin( restRequest &request,
                                          restResponse &response )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pOmCommand = NULL ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;
      const CHAR* hostName = pmdGetKRCB()->getHostName();
      string localAgentHost = hostName ;
      string localAgentPort = sdbGetOMManager()->getLocalAgentPort() ;

      pOmCommand = SDB_OSS_NEW omRegisterPluginsCommand( this, pAdaptor,
                                                         &request, &response,
                                                         localAgentHost,
                                                         localAgentPort,
                                                         _wwwRootPath ) ;
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

   INT32 _omRestSession::_setSpecifyNode( const string &sdbHostName,
                                          const string &sdbSvcName,
                                          list<omNodeInfo> &nodeList )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isFind = FALSE ;
      list<omNodeInfo>::iterator iter ;

      if( ( TRUE == sdbHostName.empty() && FALSE == sdbSvcName.empty() ) ||
          ( FALSE == sdbHostName.empty() && TRUE == sdbSvcName.empty() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "fail to get %s,rc=%d",
                 sdbHostName.empty() ?
                 OM_REST_HEAD_SDBHOSTNAME :OM_REST_HEAD_SDBSERVICENAME, rc ) ;
         goto error ;
      }

      if( FALSE == sdbHostName.empty() && FALSE == sdbSvcName.empty() )
      {
         for( iter = nodeList.begin(); iter != nodeList.end(); ++iter )
         {
            if( iter->hostName.compare( sdbHostName.c_str() ) == 0 &&
                iter->service.compare( sdbSvcName.c_str() ) == 0 )
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
                    OM_REST_HEAD_SDBHOSTNAME, sdbHostName.c_str(),
                    OM_REST_HEAD_SDBSERVICENAME, sdbSvcName.c_str(),
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omRestSession::_processSdbTransferMsg( restRequest &request,
                                                 restResponse &response,
                                                 const CHAR *pClusterName,
                                                 const CHAR *pBusinessName )
   {
      INT32 rc        = SDB_OK ;
      INT32 rtnCode   = SDB_OK ;
      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      BSONObjBuilder retBuilder( PMD_RETBUILDER_DFT_SIZE ) ;
      BOOLEAN needReplay = FALSE ;
      BOOLEAN needRollback = FALSE ;
      MsgHeader *msg = NULL ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;
      _omTransferProcessor *transProcessor = NULL ;
      list<omNodeInfo> nodeList ;
      string sdbHostName ;
      string sdbSvcName ;

      //check session
      if ( !isAuthOK() )
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         PD_LOG( PDERROR, "session does not exist:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, response, this, eduCB() ) ;
         goto error ;
      }

      rc = _getBusinessAccessNode( request, pClusterName, pBusinessName, 
                                   nodeList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get business info failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
         goto error ;
      }

      sdbHostName = request.getHeader( OM_REST_HEAD_SDBHOSTNAME ) ;
      sdbSvcName = request.getHeader( OM_REST_HEAD_SDBSERVICENAME ) ;
      rc = _setSpecifyNode( sdbHostName, sdbSvcName, nodeList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "set specify node failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
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
      rc = _translateMSG( pAdaptor, request, &msg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "translate message failed:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
         goto error ;
      }

      rtnCode = transProcessor->processMsg( msg, contextBuff, contextID, 
                                            needReplay, needRollback,
                                            retBuilder ) ;
      if ( rtnCode )
      {
         BSONObj tmp ;

         if ( 0 == contextBuff.size() )
         {
            utilBuildErrorBson( retBuilder, rtnCode,
                                _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
         }
         else
         {
            BSONObj errorInfo( contextBuff.data() ) ;
            if ( !errorInfo.hasField( OM_REST_RES_RETCODE ) )
            {
               retBuilder.append( OM_REST_RES_RETCODE, rtnCode ) ;
            }
            retBuilder.appendElements( errorInfo ) ;
         }

         tmp = retBuilder.obj() ;

         response.setOPResult( rtnCode, tmp ) ;

         rc = pAdaptor->sendRest( socket(), &response ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to send response: rc=%d", rc ) ;
            goto error ;
         }

         goto error ;
      }

      /// succeed and has result info
      {
         retBuilder.append( OM_REST_RES_RETCODE, rtnCode ) ;
         BSONObj tmp = retBuilder.obj() ;

         response.setOPResult( rtnCode, tmp ) ;
      }

      if ( contextBuff.recordNum() > 0 )
      {
         rc = pAdaptor->setResBody( socket(), &response,
                                    contextBuff.data(),
                                    contextBuff.size(),
                                    contextBuff.recordNum() ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to send response: rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( -1 != contextID )
      {
         rtnContext *pContext = _pRTNCB->contextFind( contextID ) ;

         while ( NULL != pContext )
         {
            rc = pContext->getMore( -1, contextBuff, _pEDUCB ) ;
            if ( rc )
            {
               _pRTNCB->contextDelete( contextID, _pEDUCB ) ;
               contextID = -1 ;
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG_MSG( PDERROR, "getmore failed:rc=%d", rc ) ;
                  goto error ;
               }

               rc = SDB_OK ;
               break ;
            }

            rc = pAdaptor->setResBody( socket(), &response,
                                       contextBuff.data(),
                                       contextBuff.size(),
                                       contextBuff.recordNum() ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "failed to send response: rc=%d", rc ) ;
               goto error ;
            }
         }
      }

      rc = pAdaptor->setResBodyEnd( socket(), &response ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to send response: rc=%d", rc ) ;
         goto error ;
      }

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

   INT32 _omRestSession::_processOMRestMsg( restRequest &request,
                                            restResponse &response )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pOmCommand = NULL ;
      pOmCommand = _createCommand( request, response ) ;
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

   INT32 _omRestSession::_actionGetFile( restRequest &request,
                                         restResponse &response )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pCommand = NULL ;
      restAdaptor *pAdptor = sdbGetPMDController()->getRestAdptor() ;
      const CHAR* hostName = pmdGetKRCB()->getHostName();
      string localAgentHost = hostName ;
      string localAgentPort = sdbGetOMManager()->getLocalAgentPort() ;

      PD_LOG( PDDEBUG, "OM: getfile command:file=%s",
              request.getRequestPath().c_str() ) ;

      pCommand = SDB_OSS_NEW omGetFileCommand( this, pAdptor,
                                               &request, &response,
                                               localAgentHost, localAgentPort,
                                               _wwwRootPath ) ;
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

   INT32 _omRestSession::_forwardPlugin( restRequest &request,
                                         restResponse &response )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *pCommand = NULL ;
      restAdaptor *pAdptor = sdbGetPMDController()->getRestAdptor() ;
      const CHAR* hostName = pmdGetKRCB()->getHostName();
      string localAgentHost = hostName ;
      string localAgentPort = sdbGetOMManager()->getLocalAgentPort() ;

      if ( !isAuthOK() )
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         PD_LOG( PDERROR, "session does not exist:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdptor, response, this, eduCB() ) ;

         PD_LOG( PDEVENT, "OM: redirect to:%s", OM_REST_LOGIN_HTML ) ;
         goto error ;
      }

      pCommand = SDB_OSS_NEW omForwardPluginCommand( this, pAdptor,
                                                     &request, &response,
                                                     localAgentHost,
                                                     localAgentPort,
                                                     _wwwRootPath ) ;
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

   INT32 _omRestSession::_actionCmd( restRequest &request,
                                     restResponse &response )
   {
      INT32 rc = SDB_OK ;
      restAdaptor *pAdaptor = sdbGetPMDController()->getRestAdptor() ;
      string clusterName ;
      string businessName ;

      clusterName  = request.getHeader( OM_REST_HEAD_CLUSTERNAME ) ;
      businessName = request.getHeader( OM_REST_HEAD_BUSINESSNAME ) ;
      if ( clusterName.empty() || businessName.empty() )
      {
         rc = _processOMRestMsg( request, response ) ;
      }
      else
      {
         string businessType ;
         string deployMode ;

         rc = _getBusinessInfo( clusterName.c_str(), businessName.c_str(),
                                businessType, deployMode ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "get business info failed:cluster=%s,"
                                 "business=%s,rc=%d",
                        clusterName.c_str(), businessName.c_str(), rc ) ;
            _sendOpError2Web( rc, pAdaptor, response, this, _pEDUCB ) ;
            goto error ;
         }

         if ( OM_BUSINESS_SEQUOIADB == businessType )
         {
            rc = _processSdbTransferMsg( request, response,
                                         clusterName.c_str(),
                                         businessName.c_str() ) ;
         }
         else
         {
            //forward to the plugin
            rc = _forwardPlugin( request, response ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omRestCommandBase *_omRestSession::_createCommand( restRequest &request,
                                                      restResponse &response )
   {
      INT32 rc = SDB_OK ;
      omRestCommandBase *commandIf = NULL ;
      restAdaptor *pAdptor = sdbGetPMDController()->getRestAdptor() ;
      const CHAR* hostName = pmdGetKRCB()->getHostName();
      string localAgentHost = hostName ;
      string localAgentPort = sdbGetOMManager()->getLocalAgentPort() ;
      string subCommand ;

      subCommand = request.getQuery( OM_REST_FIELD_COMMAND ) ;
      if ( subCommand.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "get command failed:filed=%s,rc=%d",
                     OM_REST_FIELD_COMMAND, rc ) ;
         _sendOpError2Web( rc, pAdptor, response, this, eduCB() ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "OM: command:command=%s", subCommand.c_str() ) ;

      if ( OM_LOGIN_REQ != subCommand && OM_CHECK_SESSION_REQ != subCommand &&
           !isAuthOK() )
      {
         // except login_rep and check_seesion_req, other commands can only 
         // execute in authrity status
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         PD_LOG( PDERROR, "session does not exist:rc=%d", rc ) ;
         _sendOpError2Web( rc, pAdptor, response, this, eduCB() ) ;

         PD_LOG( PDEVENT, "OM: redirect to:%s", OM_REST_LOGIN_HTML ) ;
         goto error ;
      }

      commandIf = getOmRestCmdBuilder()->create( subCommand.c_str(),
                                                 this, pAdptor,
                                                 &request, &response,
                                                 localAgentHost, localAgentPort,
                                                 _wwwRootPath ) ;
      if ( NULL == commandIf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "command is unrecognized:command=%s,rc=%d",
                     subCommand.c_str(), rc ) ;
         _sendOpError2Web( rc, pAdptor, response, this, eduCB() ) ;
         goto error ;
      }

   done:
      return commandIf ;
   error:
      goto done ;
   }
}

