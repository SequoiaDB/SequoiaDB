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

   Source File Name = omCmdHost.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/07/2018  HJW Initial Draft

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
   // ***************** omDeployPackageCommand *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omDeployPackageCommand ) ;

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
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

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
      try
      {
         _recordHistory( rc, OM_DEPLOY_PACKAGE_REQ,
                         BSON( OM_BSON_CLUSTER_NAME << _clusterName <<
                               OM_REST_FIELD_PACKAGENAME << _packageName <<
                               OM_REST_FIELD_ENFORCED << _enforced ) ) ;
      }
      catch( std::exception &e )
      {
      }
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
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

      //check cluster
      rc = dbTool.getClusterInfo( _clusterName, clusterInfo ) ;
      if ( rc )
      {
         if ( SDB_DMS_RECORD_NOTEXIST == rc )
         {
            _errorMsg.setError( TRUE, "cluster does not exist: name=%s",
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

      //check package
      if ( OM_PACKAGE_SEQUOIADB == _packageName )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "the host already has package: package=%s",
                             _packageName.c_str() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = getPacketFile( _packageName, packetPath ) ;
      if ( rc )
      {
         const CHAR *hostName = pmdGetKRCB()->getHostName() ;

         _errorMsg.setError( TRUE,"the %s package does not exist, please put it"
                             " in %s of the %s host",
                             _packageName.c_str(),
                             packetPath.c_str(), hostName ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //check host
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
               _errorMsg.setError( TRUE, "host does not exist: name=%s",
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
                                   "in task[" OSS_LL_PRINT_FORMAT "]",
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

               //check install path
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
                          FALSE == pathCompare( _installPath, installPath ) )
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
}

