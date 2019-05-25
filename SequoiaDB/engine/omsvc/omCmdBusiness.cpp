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

   Source File Name = omBusinessCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/06//2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "omCommand.hpp"
#include "omDef.hpp"
#include <set>
#include <stdlib.h>
#include <sstream>

using namespace bson;
using namespace boost::property_tree;


namespace engine
{
   omUnbindBusinessCommand::omUnbindBusinessCommand(
                                                restAdaptor *pRestAdaptor,
                                                pmdRestSession *pRestSession )
         : omAuthCommand( pRestAdaptor, pRestSession )
   {
   }

   omUnbindBusinessCommand::~omUnbindBusinessCommand()
   {
   }

   INT32 omUnbindBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;
      omDatabaseTool dbTool( _cb ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "ss",
                                OM_REST_FIELD_CLUSTER_NAME, &_clusterName,
                                OM_REST_FIELD_BUSINESS_NAME, &_businessName ) ;
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

      rc = dbTool.unbindBusiness( _businessName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to unbind business: name=%s, rc=%d",
                             _businessName.c_str(), rc ) ;
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

   INT32 omUnbindBusinessCommand::_check()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;

      if ( FALSE == dbTool.isClusterExist( _clusterName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
                             _clusterName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( FALSE == dbTool.isBusinessExist( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: name=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == dbTool.isRelationshipExistByBusiness( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business has relationship: name=%s",
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

   done:
      return rc ;
   error:
      goto done ;
   }

   omRemoveBusinessCommand::omRemoveBusinessCommand( restAdaptor *pRestAdaptor,
                                                   pmdRestSession *pRestSession,
                                                   string localAgentHost,
                                                   string localAgentService )
                           : omAuthCommand( pRestAdaptor, pRestSession ),
                           _localAgentHost( localAgentHost ),
                           _localAgentService( localAgentService )
   {
   }

   omRemoveBusinessCommand::~omRemoveBusinessCommand()
   {
   }

   INT32 omRemoveBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      BSONObj buzInfo ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omArgOptions option( _restAdaptor, _restSession ) ;
      omRestTool restTool( _restAdaptor, _restSession ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "s",
                                OM_REST_FIELD_BUSINESS_NAME, &_businessName ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _check( buzInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateRequest( buzInfo, taskConfig, resultInfo ) ;
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

   BOOLEAN omRemoveBusinessCommand::_isDiscoveredBusiness( BSONObj &buzInfo )
   {
      BSONElement eleAddType = buzInfo.getField( OM_BUSINESS_FIELD_ADDTYPE ) ;

      if ( eleAddType.isNumber() &&
           OM_BUSINESS_ADDTYPE_DISCOVERY == eleAddType.Int())
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 omRemoveBusinessCommand::_check( BSONObj &buzInfo )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      omDatabaseTool dbTool( _cb ) ;

      if ( FALSE == dbTool.isBusinessExist( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business does not exist: name=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == dbTool.isRelationshipExistByBusiness( _businessName ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "business has relationship: name=%s",
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

      rc = dbTool.getOneBusinessInfo( _businessName, buzInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get business info: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( TRUE == _isDiscoveredBusiness( buzInfo ) )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "discovered business could not be removed: "
                                   "business=%s",
                             _businessName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      _clusterName  = buzInfo.getStringField( OM_BUSINESS_FIELD_CLUSTERNAME ) ;
      _businessType = buzInfo.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      _deployMod    = buzInfo.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;

      if ( OM_BUSINESS_SEQUOIADB != _businessType &&
           OM_BUSINESS_ZOOKEEPER != _businessType &&
           OM_BUSINESS_SEQUOIASQL_OLAP != _businessType &&
           OM_BUSINESS_SEQUOIASQL_OLTP != _businessType )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "Unsupported business type: type=%s",
                             _businessType.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessCommand::_generateTaskConfig(
                                                      list<BSONObj> &configList,
                                                      BSONObj &taskConfig )
   {
      INT32 rc = SDB_OK ;
      BSONObj filter ;
      BSONObjBuilder taskConfigBuilder ;
      BSONArrayBuilder configBuilder ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      filter = BSON( OM_HOST_FIELD_NAME << "" <<
                     OM_HOST_FIELD_IP   << "" <<
                     OM_HOST_FIELD_CLUSTERNAME << "" <<
                     OM_HOST_FIELD_USER << "" <<
                     OM_HOST_FIELD_PASSWD << "" <<
                     OM_HOST_FIELD_SSHPORT << "" ) ;

      taskConfigBuilder.append( OM_BSON_CLUSTER_NAME, _clusterName ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_TYPE, _businessType ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_NAME, _businessName ) ;
      taskConfigBuilder.append( OM_BSON_DEPLOY_MOD, _deployMod ) ;

      if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         string authUser ;
         string authPasswd ;

         rc = dbTool.getAuth( _businessName, authUser, authPasswd ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: "
                                      "name=%s, rc=%d",
                                _businessName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         taskConfigBuilder.append( OM_TASKINFO_FIELD_AUTH_USER, authUser ) ;
         taskConfigBuilder.append( OM_TASKINFO_FIELD_AUTH_PASSWD, authPasswd ) ;
      }
      else if ( OM_BUSINESS_ZOOKEEPER == _businessType ||
                OM_BUSINESS_SEQUOIASQL_OLAP == _businessType )
      {
         string sdbUser ;
         string sdbPasswd ;
         string sdbUserGroup ;
         BSONObj clusterInfo ;

         rc = dbTool.getClusterInfo( _clusterName, clusterInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get cluster info: "
                                      "name=%s, rc=%d",
                                _clusterName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         sdbUser      = clusterInfo.getStringField( OM_CLUSTER_FIELD_SDBUSER ) ;
         sdbPasswd    = clusterInfo.getStringField(
                                            OM_CLUSTER_FIELD_SDBPASSWD ) ;
         sdbUserGroup = clusterInfo.getStringField(
                                            OM_CLUSTER_FIELD_SDBUSERGROUP ) ;

         taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBUSER, sdbUser ) ;
         taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBPASSWD, sdbPasswd ) ;
         taskConfigBuilder.append( OM_TASKINFO_FIELD_SDBUSERGROUP,
                                   sdbUserGroup ) ;
      }
      else if( OM_BUSINESS_SEQUOIASQL_OLTP == _businessType )
      {
      }

      for ( iter = configList.begin(); iter != configList.end(); ++iter )
      {
         string hostName ;
         string installPath ;
         BSONObj hostInfo ;
         BSONObj tmpHostInfo ;
         BSONObj configInfo ;
         BSONObj packages ;

         hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
         configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

         rc = dbTool.getHostInfoByAddress( hostName, tmpHostInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get host info: name=%s, rc=%d",
                                hostName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         hostInfo = tmpHostInfo.filterFieldsUndotted( filter, TRUE ) ;

         packages = tmpHostInfo.getObjectField( OM_HOST_FIELD_PACKAGES ) ;
         {
            BSONObjIterator pkgIter( packages ) ;

            while ( pkgIter.more() )
            {
               BSONElement ele = pkgIter.next() ;
               BSONObj pkgInfo = ele.embeddedObject() ;
               string pkgName = pkgInfo.getStringField(
                                                   OM_HOST_FIELD_PACKAGENAME ) ;

               if ( pkgName == _businessType )
               {
                  installPath = pkgInfo.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;
                  break ;
               }     
            }
         }

         {
            BSONObjIterator configIter( configInfo ) ;

            while ( configIter.more() )
            {
               BSONObjBuilder configInfoBuilder ;
               BSONElement ele = configIter.next() ;
               BSONObj nodeInfo = ele.embeddedObject() ;

               if ( OM_BUSINESS_SEQUOIADB == _businessType &&
                    0 == ossStrlen( nodeInfo.getStringField(
                                                   OM_CONF_DETAIL_CATANAME ) ) )
               {
                  CHAR catName[ OM_INT32_LENGTH + 1 ] = { 0 } ;
                  string svcName = nodeInfo.getStringField(
                                                      OM_CONF_DETAIL_SVCNAME ) ;
                  INT32 iSvcName = ossAtoi( svcName.c_str() ) ;
                  INT32 iCatName = iSvcName + MSG_ROUTE_CAT_SERVICE ;

                  ossItoa( iCatName, catName, OM_INT32_LENGTH ) ;
                  configInfoBuilder.append( OM_CONF_DETAIL_CATANAME, catName ) ;
               }

               configInfoBuilder.appendElements( nodeInfo ) ;
               configInfoBuilder.appendElements( hostInfo ) ;
               configInfoBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;

               configBuilder.append( configInfoBuilder.obj() ) ;
            }
         }
      }

      taskConfigBuilder.append( OM_TASKINFO_FIELD_CONFIG,
                                configBuilder.arr() ) ;

      taskConfig = taskConfigBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void omRemoveBusinessCommand::_generateResultInfo( list<BSONObj> &configList,
                                                      BSONArray &resultInfo )
   {
      BSONObj filter ;
      BSONArrayBuilder resultInfoBuilder ;
      list<BSONObj>::iterator iter ;

      if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         filter = BSON( OM_BSON_SVCNAME  << "" <<
                        OM_BSON_ROLE     << "" <<
                        OM_BSON_DATAGROUPNAME << "" ) ;
      }
      else if ( OM_BUSINESS_ZOOKEEPER == _businessType )
      {
         filter = BSON( OM_ZOO_CONF_DETAIL_ZOOID << "" ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_OLAP == _businessType )
      {
         filter = BSON( OM_SSQL_OLAP_CONF_ROLE << "" ) ;
      }
      else if( OM_BUSINESS_SEQUOIASQL_OLTP == _businessType )
      {
         filter = BSON( OM_BSON_PORT << "" ) ;
      }

      for ( iter = configList.begin(); iter != configList.end(); ++iter )
      {
         string hostName ;
         BSONObj configInfo ;

         hostName = iter->getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;
         configInfo = iter->getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

         {
            BSONObjIterator configIter( configInfo ) ;

            while ( configIter.more() )
            {
               BSONObjBuilder resultEleBuilder ;
               BSONElement ele = configIter.next() ;
               BSONObj tmpNodeInfo = ele.embeddedObject() ;
               BSONObj nodeInfo = tmpNodeInfo.filterFieldsUndotted( filter,
                                                                    TRUE ) ;

               resultEleBuilder.append( OM_TASKINFO_FIELD_HOSTNAME, hostName ) ;
               resultEleBuilder.appendElements( nodeInfo ) ;

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
         }
      }

      resultInfo = resultInfoBuilder.arr() ;
   }

   INT32 omRemoveBusinessCommand::_generateRequest( const BSONObj &buzInfo,
                                                    BSONObj &taskConfig,
                                                    BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      omDatabaseTool dbTool( _cb ) ;
      list<BSONObj> configList ;

      rc = dbTool.getConfigByBusiness( _businessName, configList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to config: business=%s, rc=%d",
                             _businessName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _generateTaskConfig( configList, taskConfig ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate task config: rc=%d", rc ) ;
         goto error ;
      }

      _generateResultInfo( configList, resultInfo ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessCommand::_createTask( const BSONObj &taskConfig,
                                               const BSONArray &resultInfo,
                                               INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      string errDetail ;
      omDatabaseTool dbTool( _cb ) ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = dbTool.getMaxTaskID( taskID ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to get max task id:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      ++taskID ;

      rc = taskTool.createTask( OM_TASK_TYPE_REMOVE_BUSINESS, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_REMOVE_BUSINESS ),
                                taskConfig, resultInfo ) ;
      if( rc )
      {
         _errorMsg.setError( TRUE, "failed to create task:rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentTask( taskID, errDetail ) ;
      if( rc )
      {
         dbTool.removeTask( taskID ) ;
         _errorMsg.setError( TRUE, "failed to notify agent:detail:%s,rc=%d",
                             errDetail.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

