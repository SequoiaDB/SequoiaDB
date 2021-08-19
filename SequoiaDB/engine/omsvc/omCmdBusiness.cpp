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
#include "omConfigBuilder.hpp"
#include <set>
#include <stdlib.h>
#include <sstream>

using namespace bson;
using namespace boost::property_tree;


namespace engine
{
   // ***************** Get Config Template *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omGetConfigTemplateCommand ) ;

   INT32 omGetConfigTemplateCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj configTemplate ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "s",
                                OM_REST_FIELD_BUSINESS_TYPE, &_businessType ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      rc = _getTemplate( configTemplate ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get config template: rc=%d", rc ) ;
         goto error ;
      }

      restTool.appendResponeContent( configTemplate ) ;
      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omGetConfigTemplateCommand::_getTemplate( BSONObj &configTemplate )
   {
      INT32 rc = SDB_OK ;
      omConfigTool cfgTool( _rootPath, _languageFileSep ) ;

      //businessType
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

      //get config template
      rc = cfgTool.readBuzConfigTemplate( _businessType,
                                          OM_FIELD_OPERATION_DEPLOY, FALSE,
                                          configTemplate ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // ***************** Get Business Config *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omGetBusinessConfigCommand ) ;

   INT32 omGetBusinessConfigCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj templateInfo ;
      BSONObj deployModInfo ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

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
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
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

      //businessType
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

      //template
      {
         list<BSONObj> templateList ;
         list<BSONObj>::iterator iter ;

         rc =  cfgTool.readBuzDeployTemplate( _businessType, _operationType,
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

      //create instance
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

      //get deploy property
      rc = _getDeployProperty( templateInfo, deployModInfo, deployProperty ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get deploy property: rc=%d", rc ) ;
         goto error ;
      }

      //get business template
      rc = cfgTool.readBuzConfigTemplate( _businessType, _deployMod, FALSE,
                                          buzTemplate ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      //get host list of cluster
      rc = _getHostInfoOfCluster( hostListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get hosts info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      //get business list of cluster
      rc = _getBuzInfoOfCluster( buzListOfCluster ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get business info of cluster: rc=%d",
                 rc ) ;
         goto error ;
      }

      //get rest specified hosts list
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

   // ***************** omUnbindBusinessCommand *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omUnbindBusinessCommand ) ;

   omUnbindBusinessCommand::~omUnbindBusinessCommand()
   {
   }

   INT32 omUnbindBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;
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

      dbTool.removeAuth( _businessName ) ;

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
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

   // ***************** omRemoveBusinessCommand *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omRemoveBusinessCommand ) ;

   INT32 omRemoveBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      BSONObj buzInfo ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omArgOptions option( _request ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "s|b",
                                OM_REST_FIELD_BUSINESS_NAME, &_businessName,
                                OM_REST_FIELD_FORCE, &_force ) ;
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
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
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

      taskConfigBuilder.appendBool( OM_BSON_FORCE, _force ) ;
      taskConfigBuilder.append( OM_BSON_CLUSTER_NAME, _clusterName ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_TYPE, _businessType ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_NAME, _businessName ) ;
      taskConfigBuilder.append( OM_BSON_DEPLOY_MOD, _deployMod ) ;

      if ( OM_BUSINESS_ZOOKEEPER == _businessType ||
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
      else
      {
         BSONObj authInfo ;
         string authUser ;
         string authPasswd ;
         string defaultDBName ;

         rc = dbTool.getAuth( _businessName, authInfo ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business auth: "
                                      "name=%s, rc=%d",
                                _businessName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         authUser = authInfo.getStringField( OM_AUTH_FIELD_USER ) ;
         authPasswd = authInfo.getStringField( OM_AUTH_FIELD_PASSWD ) ;
         defaultDBName = authInfo.getStringField( OM_BSON_DB_NAME ) ;

         taskConfigBuilder.append( OM_TASKINFO_FIELD_AUTH_USER, authUser ) ;
         taskConfigBuilder.append( OM_TASKINFO_FIELD_AUTH_PASSWD, authPasswd ) ;

         if( !defaultDBName.empty() )
         {
            taskConfigBuilder.append( OM_TASKINFO_FIELD_DBNAME,
                                      defaultDBName ) ;
         }
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

         //get install path
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
      else if( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == _businessType )
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

   // ***************** restart Business *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omRestartBusinessCommand ) ;

   INT32 omRestartBusinessCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      BSONObj options ;
      BSONObj taskConfig ;
      BSONArray resultInfo ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;
      omArgOptions option( _request ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "ss|j",
                                OM_REST_FIELD_CLUSTER_NAME, &_clusterName,
                                OM_REST_FIELD_BUSINESS_NAME, &_businessName,
                                OM_REST_FIELD_OPTIONS, &options ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _check() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _generateRequest( options, taskConfig, resultInfo ) ;
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
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omRestartBusinessCommand::_check()
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj businessInfo ;
      vector<simpleAddressInfo> addressList ;
      omDatabaseTool dbTool( _cb ) ;

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

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestartBusinessCommand::_generateRequest( const BSONObj &options,
                                                     BSONObj &taskConfig,
                                                     BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> configList ;
      omDatabaseTool dbTool( _cb ) ;

      rc = _getBusinessConfig( options, configList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get business address: rc=%d", rc ) ;
         goto error ;
      }

      _generateTaskConfig( configList, taskConfig ) ;

      _generateResultInfo( configList, resultInfo ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void omRestartBusinessCommand::_generateTaskConfig(
                                                   list<BSONObj> &configList,
                                                   BSONObj &taskConfig )
   {
      BSONObjBuilder taskConfigBuilder ;
      BSONArrayBuilder configBuilder ;
      list<BSONObj>::iterator iter ;

      taskConfigBuilder.append( OM_BSON_CLUSTER_NAME, _clusterName ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_TYPE, _businessType ) ;
      taskConfigBuilder.append( OM_BSON_BUSINESS_NAME, _businessName ) ;

      for ( iter = configList.begin(); iter != configList.end(); ++iter )
      {
         configBuilder.append( *iter ) ;
      }

      taskConfigBuilder.append( OM_TASKINFO_FIELD_CONFIG,
                                configBuilder.arr() ) ;

      taskConfig = taskConfigBuilder.obj() ;
   }

   void omRestartBusinessCommand::_generateResultInfo(
                                             list<BSONObj> &configList,
                                             BSONArray &resultInfo )
   {
      BSONArrayBuilder resultInfoBuilder ;
      list<BSONObj>::iterator iter ;

      for ( iter = configList.begin(); iter != configList.end(); ++iter )
      {
         BSONObjBuilder resultEleBuilder ;

         resultEleBuilder.appendElements( *iter ) ;
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

      resultInfo = resultInfoBuilder.arr() ;
   }

   BOOLEAN omRestartBusinessCommand::_isRequestNode(
                                          list<simpleAddressInfo> &addrList,
                                          const string &hostName,
                                          const string &port )
   {
      BOOLEAN result = FALSE ;
      list<simpleAddressInfo>::iterator iter ;

      if ( addrList.empty() )
      {
         result = TRUE ;
         goto done ;
      }

      for ( iter = addrList.begin(); iter != addrList.end(); ++iter )
      {
         if ( iter->hostName == hostName && iter->port == port )
         {
            result = TRUE ;
            break ;
         }
      }

   done:
      return result ;
   }

   INT32 omRestartBusinessCommand::_getBusinessConfig(
                                                   const BSONObj &options,
                                                   list<BSONObj> &configList )
   {
      INT32 rc = SDB_OK ;
      list<simpleAddressInfo> tmpAddrList ;
      list<BSONObj> tmpConfigList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;

      rc = dbTool.getConfigByBusiness( _businessName, tmpConfigList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to config: business=%s, rc=%d",
                             _businessName.c_str(), rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         BSONObj nodes = options.getObjectField( OM_BSON_NODES ) ;
         BSONObjIterator iterBson( nodes ) ;

         while ( iterBson.more() )
         {
            simpleAddressInfo address ;
            BSONElement ele = iterBson.next() ;

            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "Invalid element, %s is not Object "
                                         "type: type=%d, rc=%d",
                                   OM_BSON_NODES, ele.type(), rc ) ;
               PD_LOG_MSG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            {
               BSONObj oneNode = ele.embeddedObject() ;

               address.hostName = oneNode.getStringField( OM_BSON_HOSTNAME ) ;
               address.port = oneNode.getStringField( OM_BSON_SVCNAME ) ;
               tmpAddrList.push_back( address ) ;
            }
         }
      }

      for ( iter = tmpConfigList.begin(); iter != tmpConfigList.end(); ++iter )
      {
         string hostName = iter->getStringField(
                                             OM_CONFIGURE_FIELD_HOSTNAME ) ;
         BSONObj configInfo = iter->getObjectField(
                                             OM_CONFIGURE_FIELD_CONFIG ) ;
         BSONObjIterator configIter( configInfo ) ;

         while ( configIter.more() )
         {
            BSONObjBuilder configInfoBuilder ;
            BSONElement ele = configIter.next() ;
            BSONObj nodeInfo = ele.embeddedObject() ;

            if ( OM_BUSINESS_SEQUOIADB == _businessType )
            {
               string port = nodeInfo.getStringField( OM_CONF_DETAIL_SVCNAME ) ;

               if ( _isRequestNode( tmpAddrList, hostName, port ) )
               {
                  string role = nodeInfo.getStringField( OM_CONF_DETAIL_ROLE ) ;
                  string groupName = nodeInfo.getStringField(
                                                OM_CONF_DETAIL_DATAGROUPNAME ) ;

                  configInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
                  configInfoBuilder.append( OM_BSON_SVCNAME, port ) ;
                  configInfoBuilder.append( OM_BSON_ROLE, role ) ;
                  configInfoBuilder.append( OM_BSON_DATAGROUPNAME, groupName ) ;

                  configList.push_back( configInfoBuilder.obj() ) ;
               }
            }
            else if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == _businessType ||
                      OM_BUSINESS_SEQUOIASQL_MYSQL == _businessType )
            {
               string port = nodeInfo.getStringField(
                                             OM_CONFIGURE_FIELD_PORT2 ) ;
               string installPath ;

               if( FALSE == dbTool.getHostPackagePath( hostName, _businessType,
                                                       installPath ) )
               {
                  rc = SDB_SYS ;
                  _errorMsg.setError( TRUE, "Install path not found: "
                                            "name=%s, host=%s, type=%s",
                                      _businessName.c_str(), hostName.c_str(),
                                      _businessType.c_str() ) ;
                  PD_LOG_MSG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               configInfoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
               configInfoBuilder.append( OM_BSON_PORT, port ) ;
               configInfoBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;

               configList.push_back( configInfoBuilder.obj() ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestartBusinessCommand::_createTask( const BSONObj &taskConfig,
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

      rc = taskTool.createTask( OM_TASK_TYPE_RESTART_BUSINESS, taskID,
                                getTaskTypeStr( OM_TASK_TYPE_RESTART_BUSINESS ),
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

   // ***************** modify business config *****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omUpdateBusinessConfigCommand ) ;
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omDeleteBusinessConfigCommand ) ;

   INT32 omModifyBusinessConfigCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj configInfo ;
      BSONObj result ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;
      omArgOptions option( _request ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "j", OM_REST_FIELD_CONFIGINFO, &configInfo ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = _check( configInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check: rc=%d", rc ) ;
         goto error ;
      }

      rc = _modifyConfig( configInfo, result ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to modify business config: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendResponse( result ) ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   INT32 omModifyBusinessConfigCommand::_check( BSONObj &configInfo )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj businessInfo ;
      omDatabaseTool dbTool( _cb ) ;

      _clusterName = configInfo.getStringField( OM_BSON_CLUSTER_NAME ) ;
      if ( 0 == _clusterName.length() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "%s is empty", OM_BSON_CLUSTER_NAME ) ;
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

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omModifyBusinessConfigCommand::_generateRequest( BSONObj &configInfo,
                                                          BSONObj &request )
   {
      INT32 rc = SDB_OK ;
      string authUser ;
      string authPwd ;
      BSONObjBuilder builder ;
      BSONArrayBuilder arrayBuilder ;
      omDatabaseTool dbTool( _cb ) ;

      builder.append( OM_BSON_COMMAND, name() ) ;
      builder.append( OM_BSON_CLUSTER_NAME, _clusterName ) ;
      builder.append( OM_BSON_BUSINESS_NAME, _businessName ) ;
      builder.append( OM_BSON_BUSINESS_TYPE, _businessType ) ;

      _getBusinessAuth( _businessName, authUser, authPwd ) ;

      builder.append( OM_BSON_USER, authUser ) ;
      builder.append( OM_BSON_PASSWD, authPwd ) ;

      if ( OM_BUSINESS_SEQUOIADB == _businessType )
      {
         vector<simpleAddressInfo> addressList ;
         vector<simpleAddressInfo>::iterator iter ;

         rc = dbTool.getBusinessAddressWithConfig( _businessName,
                                                   addressList ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to get business address: "
                                      "businessName=%s, rc=%d",
                                _businessName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         for ( iter = addressList.begin(); iter != addressList.end(); ++iter )
         {
            BSONObjBuilder addressBuilder ;

            addressBuilder.append( OM_CONFIGURE_FIELD_HOSTNAME,
                                   iter->hostName ) ;
            addressBuilder.append( OM_CONFIGURE_FIELD_SVCNAME, iter->port ) ;
            arrayBuilder.append( addressBuilder.obj() ) ;
         }

         builder.append( OM_BSON_ADDRESS, arrayBuilder.arr() ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == _businessType ||
                OM_BUSINESS_SEQUOIASQL_MYSQL == _businessType )
      {
         BSONObj buzInfo ;
         BSONObj buzConfig ;
         list<BSONObj> configList ;
         string hostName ;
         string dbpath ;
         string installPath ;

         rc = dbTool.getConfigByBusiness( _businessName, configList ) ;
         if ( rc || configList.empty() )
         {
            if ( configList.empty() )
            {
               rc = SDB_DMS_RECORD_NOTEXIST ;
            }

            _errorMsg.setError( TRUE, "failed to get business info: "
                                      "businessName=%s, rc=%d",
                                _businessName.c_str(), rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         buzInfo = configList.back() ;
         hostName = buzInfo.getStringField( OM_CONFIGURE_FIELD_HOSTNAME ) ;

         buzConfig = buzInfo.getObjectField( OM_CONFIGURE_FIELD_CONFIG ) ;

         if( FALSE == dbTool.getHostPackagePath( hostName, _businessType,
                                                 installPath ) )
         {
            rc = SDB_SYS ;
            _errorMsg.setError( TRUE, "Install path not found: "
                                      "name=%s, host=%s, type=%s",
                                _businessName.c_str(), hostName.c_str(),
                                _businessType.c_str() ) ;
            PD_LOG_MSG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }

         {
            BSONObjIterator iter( buzConfig ) ;

            if ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               BSONObj oneNodeConfig = ele.embeddedObject() ;

               dbpath = oneNodeConfig.getStringField(
                                             OM_CONFIGURE_FIELD_DBPATH ) ;
            }
         }

         {
            BSONObjBuilder infoBuilder ;

            infoBuilder.append( OM_BSON_HOSTNAME, hostName ) ;
            infoBuilder.append( OM_BSON_DBPATH, dbpath ) ;
            infoBuilder.append( OM_BSON_INSTALL_PATH, installPath ) ;

            builder.append( OM_BSON_INFO, infoBuilder.obj() ) ;
         }
      }

      builder.append( OM_BSON_CONFIG,
                      configInfo.getObjectField( OM_BSON_CONFIG ) ) ;

      request = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omModifyBusinessConfigCommand::_modifyConfig( BSONObj &configInfo,
                                                       BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      BSONObj request ;
      string errDetail ;
      omTaskTool taskTool( _cb, _localAgentHost, _localAgentService ) ;

      rc = _generateRequest( configInfo, request ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to generate request, rc=%d", rc ) ;
         goto error ;
      }

      rc = taskTool.notifyAgentMsg(
                                 CMD_ADMIN_PREFIX OM_MODIFY_BUSINESS_CONFIG_REQ,
                                 request, errDetail, result ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to notify agent: detail=%s, rc=%d",
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