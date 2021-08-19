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

   Source File Name = omCommandTool.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/17/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "omCommandTool.hpp"
#include "omDef.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "../bson/lib/md5.hpp"
#include "ossPath.hpp"

using namespace bson ;

namespace engine
{
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
      filePath = tmpPath ;
      goto done ;
   }

   BOOLEAN pathCompare( const string &p1, const string &p2 )
   {
      INT32 len1 = p1.length() ;
      INT32 len2 = p2.length() ;

      if ( len1 == len2 && p1 == p2 )
      {
         return TRUE ;
      }
      else if ( len1 - len2 == 1 )
      {
         return ( p1 == p2 + OSS_FILE_SEP_CHAR ) ? TRUE : FALSE ;
      }
      else if ( len2 - len1 == 1 )
      {
         return ( p1 + OSS_FILE_SEP_CHAR == p2 ) ? TRUE : FALSE ;
      }

      return FALSE ;
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

   INT32 omXmlTool::readXml2Bson( const string &fileName, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         ptree pt ;
         read_xml( fileName.c_str(), pt ) ;
         _xml2Bson( pt, obj ) ;
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

   BOOLEAN omXmlTool::_isStringValue( ptree &pt )
   {
      BOOLEAN isStringV = FALSE ;
      if( _isArray( pt ) )
      {
         isStringV = FALSE ;
         goto done ;
      }

      if( pt.size() == 0 )
      {
         isStringV = TRUE ;
         goto done ;
      }

      if( pt.size() > 1 )
      {
         isStringV = FALSE ;
         goto done ;
      }

      // in this case pt.size() == 1
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

   BOOLEAN omXmlTool::_isArray( ptree &pt )
   {
      BOOLEAN isArr = FALSE ;
      string type ;
      try
      {
         type = pt.get<string>( OM_XMLATTR_TYPE ) ;
      }
      catch( std::exception & )
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

   void omXmlTool::_recurseParseObj( ptree &pt, BSONObj &out )
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
            // obj
            BSONObj obj ;
            _recurseParseObj( child, obj ) ;
            builder.append(key, obj ) ;
         }
      }
      out = builder.obj() ;
   }


   void omXmlTool::_parseArray( ptree &pt, BSONArrayBuilder &arrayBuilder )
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

   void omXmlTool::_xml2Bson( ptree &pt, BSONObj &out )
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
            // obj
            BSONObj obj ;
            _recurseParseObj( child, obj ) ;
            builder.append(key, obj ) ;
         }
      }
      out = builder.obj() ;
   }

   string omConfigTool::getBuzDeployTemplatePath( const string &businessType,
                                                  const string &operationType )
   {
      string templateFile ;

      templateFile = _rootPath + OSS_FILE_SEP + OM_BUSINESS_CONFIG_SUBDIR
                           + OSS_FILE_SEP + businessType ;

      if( operationType == OM_FIELD_OPERATION_EXTEND )
      {
         templateFile = templateFile + "_"OM_FIELD_OPERATION_EXTEND ;
      }

      templateFile = templateFile + OM_TEMPLATE_FILE_NAME + _languageFileSep
                           + OM_CONFIG_FILE_TYPE ;

      return templateFile ;
   }

   string omConfigTool::getBuzConfigTemplatePath( const string &businessType,
                                                  const string &deployMod,
                                                  BOOLEAN isSeparateConfig )
   {
      stringstream path ;

      path << _rootPath ;
      if( OSS_FILE_SEP != _rootPath.substr( _rootPath.length() - 1, 1 ) )
      {
         path << OSS_FILE_SEP ;
      }
      path << OM_BUSINESS_CONFIG_SUBDIR
           << OSS_FILE_SEP
           << businessType ;
      if( isSeparateConfig )
      {
         path << "_" << deployMod ;
      }
      path << OM_CONFIG_ITEM_FILE_NAME
           << _languageFileSep
           << OM_CONFIG_FILE_TYPE ;

      return path.str() ;
   }

   string omConfigTool::getBuzConfigTemplatePath(
                                                const string &businessType,
                                                const string &deployMod,
                                                const string &isSeparateConfig )
   {
      BOOLEAN sepCfg = FALSE ;
      ossStrToBoolean( isSeparateConfig.c_str(), &sepCfg ) ;
      return getBuzConfigTemplatePath( businessType, deployMod, sepCfg ) ;
   }

   INT32 omConfigTool::readBuzTypeList( list<BSONObj> &businessList )
   {
      INT32 rc = SDB_OK ;
      string businessFile ;
      BSONObj fileContent ;

      businessFile = _rootPath + OSS_FILE_SEP + OM_BUSINESS_CONFIG_SUBDIR
                     + OSS_FILE_SEP + OM_BUSINESS_FILE_NAME
                     + _languageFileSep + OM_CONFIG_FILE_TYPE ;

      rc = readXml2Bson( businessFile, fileContent ) ;
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
            BSONElement ele = iter.next() ;
            if( Object != ele.type() )
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

   INT32 omConfigTool::readBuzDeployTemplate( const string &businessType,
                                              const string &operationType,
                                              list<BSONObj> &objList )
   {
      INT32 rc = SDB_OK ;
      string templateFile ;
      BSONObj templateArray ;
      BSONObj deployMods ;

      templateFile = getBuzDeployTemplatePath( businessType, operationType ) ;

      rc = readXml2Bson( templateFile, templateArray ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "read file failed:file=%s", templateFile.c_str() ) ;
         goto error ;
      }

      deployMods = templateArray.getObjectField( OM_BSON_DEPLOY_MOD_LIST ) ;
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
            objList.push_back( oneDeployMod.copy()) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omConfigTool::readBuzConfigTemplate( const string &businessType,
                                              const string &deployMod,
                                              BOOLEAN isSeparateConfig,
                                              BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      string templateFile ;

      templateFile = getBuzConfigTemplatePath( businessType,
                                               deployMod, isSeparateConfig ) ;

      rc = readXml2Bson( templateFile, obj ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "read file failed:file=%s", templateFile.c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omConfigTool::readBuzConfigTemplate( const string &businessType,
                                              const string &deployMod,
                                              const string &isSeparateConfig,
                                              BSONObj &obj )
   {
      BOOLEAN sepCfg = FALSE ;
      ossStrToBoolean( isSeparateConfig.c_str(), &sepCfg ) ;
      return readBuzConfigTemplate( businessType, deployMod, sepCfg, obj ) ;
   }

   INT32 omDatabaseTool::_getOneTasktInfo( const BSONObj &matcher,
                                           const BSONObj &selector,
                                           BSONObj &taskInfo )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj order ;
      BSONObj hint ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order, hint,
                     0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query task:rc=%d,condition=%s", rc,
                 matcher.toString().c_str() ) ;
         goto error ;
      }

      while( TRUE )
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

            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            taskInfo = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get task id of thr running business.
    *
    * @param(in)  businessName
    *
    * @return     taskID, -1: not exist   >=0: exist
    *
   */
   INT64 omDatabaseTool::getTaskIdOfRunningBuz( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      SINT64 contextID = -1 ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      string businessKey ;
      rtnContextBuf buffObj ;

      //Status not in( OM_TASK_STATUS_FINISH, OM_TASK_STATUS_CANCEL )
      BSONArrayBuilder arrBuilder ;
      arrBuilder.append( OM_TASK_STATUS_FINISH ) ;
      arrBuilder.append( OM_TASK_STATUS_CANCEL ) ;
      BSONObj status = BSON( "$nin" << arrBuilder.arr() ) ;

      businessKey = OM_TASKINFO_FIELD_INFO"."OM_BSON_BUSINESS_NAME ;
      matcher = BSON( businessKey << businessName
                      << OM_TASKINFO_FIELD_STATUS << status ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query business:rc=%d,matcher=%s", rc,
                 matcher.toString().c_str() ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         // notice: if rc != SDB_OK, contextID is deleted in rtnGetMore
         goto done ;
      }

      {
         BSONObj result( buffObj.data() ) ;

         taskID = result.getField( OM_TASKINFO_FIELD_TASKID ).numberLong() ;
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return taskID ;
   error:
      goto done ;
   }


   INT64 omDatabaseTool::getTaskIdOfRunningHost( const string &hostName )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = -1 ;
      BSONObj taskInfo ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj elemMatch = BSON( "$elemMatch" <<
                                      BSON( OM_HOST_FIELD_NAME << hostName ) ) ;
      BSONObj status = BSON( "$nin" << BSON_ARRAY( OM_TASK_STATUS_FINISH <<
                                                   OM_TASK_STATUS_CANCEL ) ) ;

      matcher = BSON( OM_TASKINFO_FIELD_RESULTINFO << elemMatch <<
                      OM_TASKINFO_FIELD_STATUS << status ) ;

      rc = _getOneTasktInfo( matcher, selector, taskInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get task info: rc=%d", rc ) ;
         goto error ;
      }

      taskID = taskInfo.getField( OM_TASKINFO_FIELD_TASKID ).numberLong() ;

   done:
      return taskID ;
   error:
      goto done ;
   }

   /**
    * whether the task is running
    *
    * @return     taskID, -1: not exist   >=0: exist
    *
   */
   BOOLEAN omDatabaseTool::hasTaskRunning()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasTaskRun = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      rtnContextBuf buffObj ;

      //Status not in( OM_TASK_STATUS_FINISH, OM_TASK_STATUS_CANCEL )
      //BSONArrayBuilder arrBuilder ;
      //arrBuilder.append( OM_TASK_STATUS_FINISH ) ;
      //arrBuilder.append( OM_TASK_STATUS_CANCEL ) ;
      BSONObj status = BSON( "$nin" << BSON_ARRAY( OM_TASK_STATUS_FINISH <<
                                                   OM_TASK_STATUS_CANCEL ) ) ;

      matcher = BSON( OM_TASKINFO_FIELD_STATUS << status ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query business:rc=%d,matcher=%s", rc,
                 matcher.toString().c_str() ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         // notice: if rc != SDB_OK, contextID is deleted in rtnGetMore
         goto done ;
      }

      hasTaskRun = TRUE ;

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return hasTaskRun ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getMaxTaskID( INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      SINT64 contextID   = -1 ;

      taskID = 0 ;

      selector = BSON( OM_TASKINFO_FIELD_TASKID << 1 ) ;
      orderBy  = BSON( OM_TASKINFO_FIELD_TASKID << -1 ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, orderBy,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get taskid failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONElement ele ;
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
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

         {
            BSONObj result( buffObj.data() ) ;

            ele    = result.getField( OM_TASKINFO_FIELD_TASKID ) ;
            taskID = ele.numberLong() ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeTask( INT64 taskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj deletor = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_TASKINFO, deletor, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "rtnDelete task failed:taskID="OSS_LL_PRINT_FORMAT
                          ",rc=%d",
                 taskID, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::addBusinessInfo( const INT32 addType,
                                          const string &clusterName,
                                          const string &businessName,
                                          const string &businessType,
                                          const string &deployMod,
                                          const BSONObj &businessInfo )
   {
      INT32 rc = SDB_OK ;
      time_t now = time( NULL ) ;
      BSONObj buzRecord ;
      BSONObjBuilder builder ;

      builder.append( OM_BUSINESS_FIELD_ADDTYPE, addType ) ;
      builder.append( OM_BUSINESS_FIELD_CLUSTERNAME, clusterName ) ;
      builder.append( OM_BUSINESS_FIELD_NAME, businessName ) ;
      builder.append( OM_BUSINESS_FIELD_TYPE, businessType ) ;
      builder.append( OM_BUSINESS_FIELD_DEPLOYMOD, deployMod ) ;
      builder.appendTimestamp( OM_BUSINESS_FIELD_TIME, (UINT64)now * 1000, 0 ) ;

      builder.appendElements( businessInfo ) ;
      buzRecord = builder.obj() ;

      if ( FALSE == isClusterExist( clusterName ) )
      {
         rc = SDB_OM_CLUSTER_NOT_EXIST ;
         PD_LOG( PDERROR, "cluster does not exist,name:%s",
                 clusterName.c_str() ) ;
         goto error ;
      }

      if ( TRUE == isBusinessExist( businessName ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "business already exist,name:%s",
                 businessName.c_str() ) ;
         goto error ;
      }

      rc = rtnInsert( OM_CS_DEPLOY_CL_BUSINESS, buzRecord, 1, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "insert record failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get business info.
    *
    * @param(in)  businessName
    *
    * @param(out) businessType
    *
    * @param(out) deployMod
    *
    * @param(out) clusterName
    *
   */
   INT32 omDatabaseTool::getOneBusinessInfo( const string &businessName,
                                             BSONObj &businessInfo )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist  = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName )  ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            businessInfo = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDWARNING, "business does not exist: business=%s",
                 businessName.c_str() ) ;
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

   /**
    * Get all the business info of the cluster
    *
    * @param(in)  clusterName
    *
    * @param(out) buzInfoList
    *
   */
   INT32 omDatabaseTool::getBusinessInfoOfCluster(
                                                const string &clusterName,
                                                list<BSONObj> &buzInfoList )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;

      matcher = BSON( OM_BUSINESS_FIELD_CLUSTERNAME << clusterName ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order, hint,
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;

            buzInfoList.push_back( result.copy() ) ;
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

   BOOLEAN omDatabaseTool::isBusinessExistOfCluster(
                                                   const string &clusterName,
                                                   const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj buzInfo ;

      rc = getOneBusinessInfo( businessName, buzInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      if ( clusterName != buzInfo.getStringField(
                                             OM_BUSINESS_FIELD_CLUSTERNAME ) )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isBusinessExist( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj buzInfo ;

      rc = getOneBusinessInfo( businessName, buzInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   INT32 omDatabaseTool::upsertBusinessInfo( const string &businessName,
                                             const BSONObj &newBusinessInfo,
                                             INT64 &updateNum )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;
      BSONObj updator = BSON( "$replace" << newBusinessInfo ) ;
      BSONObj hint ;
      utilUpdateResult upResult ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_BUSINESS, condition, updator, hint,
                      FLG_UPDATE_UPSERT,
                      _cb, &upResult ) ;
      updateNum = upResult.updateNum() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "falied to update business info,"
                          "condition=%s,updator=%s,rc=%d",
                 condition.toString().c_str(),
                 updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeBusiness( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete configure from table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::addCluster( const BSONObj &clusterInfo )
   {
      INT32 rc = SDB_OK ;
      string installPath ;
      BSONObjBuilder recordBuilder ;
      BSONElement grantConfEle ;
      BSONObj record ;
      BSONObj filter = BSON( OM_BSON_CLUSTER_NAME        << "" <<
                             OM_BSON_FIELD_CLUSTER_DESC  << "" <<
                             OM_BSON_FIELD_SDB_USER      << "" <<
                             OM_BSON_FIELD_SDB_PASSWD    << "" <<
                             OM_BSON_FIELD_SDB_USERGROUP << "" ) ;
      BSONObj newClusterInfo = clusterInfo.filterFieldsUndotted( filter,
                                                                 TRUE ) ;

      // ClusterName Desc SdbUser SdbPasswd SdbUserGroup
      recordBuilder.appendElements( newClusterInfo ) ;

      // InstallPath
      installPath = clusterInfo.getStringField( OM_BSON_FIELD_INSTALL_PATH ) ;
      if ( installPath.empty() )
      {
         installPath = OM_DEFAULT_INSTALL_PATH ;
      }
      recordBuilder.append( OM_BSON_FIELD_INSTALL_PATH,
                            OM_DEFAULT_INSTALL_PATH ) ;

      // GrantConf
      grantConfEle = clusterInfo.getField( OM_BSON_FIELD_GRANTCONF ) ;
      if ( bson::Array == grantConfEle.type() )
      {
         BSONArrayBuilder grantConfBuilder ;
         BSONObjIterator iter( grantConfEle.embeddedObject() ) ;

         while ( iter.more() )
         {
            BSONObjBuilder privilegeBuilder ;
            BSONElement ele = iter.next() ;
            BSONObj grantInfo = ele.embeddedObject() ;
            string tmpName = grantInfo.getStringField(
                                                OM_CLUSTER_FIELD_GRANTNAME ) ;
            BOOLEAN tmpPrivilege = grantInfo.getBoolField(
                                                OM_CLUSTER_FIELD_PRIVILEGE ) ;

            privilegeBuilder.append( OM_CLUSTER_FIELD_GRANTNAME,
                                     tmpName ) ;
            privilegeBuilder.appendBool( OM_CLUSTER_FIELD_PRIVILEGE,
                                         tmpPrivilege ) ;
            grantConfBuilder.append( privilegeBuilder.obj() ) ;
         }

         recordBuilder.append( OM_BSON_FIELD_GRANTCONF,
                               grantConfBuilder.arr() ) ;
      }
      else
      {
         BSONArrayBuilder grantConfBuilder ;
         BSONObjBuilder hostFileBuilder ;
         BSONObjBuilder rootUserBuilder ;

         hostFileBuilder.append( OM_CLUSTER_FIELD_GRANTNAME,
                                 OM_CLUSTER_FIELD_HOSTFILE ) ;
         hostFileBuilder.appendBool( OM_CLUSTER_FIELD_PRIVILEGE, TRUE ) ;

         rootUserBuilder.append( OM_CLUSTER_FIELD_GRANTNAME,
                                 OM_CLUSTER_FIELD_ROOTUSER ) ;
         rootUserBuilder.appendBool( OM_CLUSTER_FIELD_PRIVILEGE, TRUE ) ;

         grantConfBuilder.append( hostFileBuilder.obj() ) ;
         grantConfBuilder.append( rootUserBuilder.obj() ) ;

         recordBuilder.append( OM_BSON_FIELD_GRANTCONF,
                               grantConfBuilder.arr() ) ;
      }
      record = recordBuilder.obj() ;

      rc = rtnInsert( OM_CS_DEPLOY_CL_CLUSTER, record, 1, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "insert record failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get cluster info
    *
    * @param(in)  clusterName
    *
    * @param(out) clusterInfo
    *
   */
   INT32 omDatabaseTool::getClusterInfo( const string &clusterName,
                                         BSONObj &clusterInfo )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasFind   = FALSE ;
      SINT64 contextID  = -1 ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj condition = BSON( OM_CLUSTER_FIELD_NAME << clusterName ) ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, condition, order,
                     hint, 0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      while( TRUE )
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         {
            BSONObj tmpConf( buffObj.data() ) ;
            clusterInfo = tmpConf.copy() ;
            hasFind = TRUE ;
         }
      }

      if( FALSE == hasFind )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
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

   INT32 omDatabaseTool::updateClusterInfo( const string &clusterName,
                                            const BSONObj &clusterInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_CLUSTER_FIELD_NAME << clusterName ) ;
      BSONObj updator = BSON( "$set" << clusterInfo ) ;
      BSONObj hint ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_CLUSTER, condition, updator, hint,
                      0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "falied to update cluster info,"
                          "condition=%s,updator=%s,rc=%d",
                 condition.toString().c_str(),
                 updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omDatabaseTool::isClusterExist( const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj clusterInfo ;

      rc = getClusterInfo( clusterName, clusterInfo ) ;
      if ( rc )
      {
         isExist= FALSE ;
      }

      return isExist ;
   }

   INT32 omDatabaseTool::updateClusterGrantConf( const string &clusterName,
                                                 const string &grantName,
                                                 const BOOLEAN privilege )
   {
      INT32 rc = SDB_OK ;
      BSONObj clusterInfo ;
      BSONObj newClusterInfo ;
      BSONObj grantConf ;
      map<string,BOOLEAN> grantList ;

      rc = getClusterInfo( clusterName, clusterInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get cluster info: cluster=%s, rc=%d",
                 clusterName.c_str(), rc ) ;
         goto error ;
      }

      grantConf = clusterInfo.getObjectField( OM_CLUSTER_FIELD_GRANTCONF ) ;
      {
         BSONObjIterator iter( grantConf ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj grantInfo = ele.embeddedObject() ;
            string tmpName = grantInfo.getStringField(
                                                OM_CLUSTER_FIELD_GRANTNAME ) ;
            BOOLEAN tmpPrivilege = grantInfo.getBoolField(
                                                OM_CLUSTER_FIELD_PRIVILEGE ) ;

            grantList[tmpName] = tmpPrivilege ;
         }
      }

      grantList[grantName] = privilege ;

      {
         BSONObjBuilder grantConfBuilder ;
         BSONArrayBuilder grantArray ;
         map<string,BOOLEAN>::iterator iter ;

         for ( iter = grantList.begin(); iter != grantList.end(); ++iter )
         {
            BSONObjBuilder grantInfoBuilder ;

            grantInfoBuilder.append( OM_CLUSTER_FIELD_GRANTNAME, iter->first ) ;
            grantInfoBuilder.appendBool( OM_CLUSTER_FIELD_PRIVILEGE,
                                         iter->second ) ;

            grantArray.append( grantInfoBuilder.obj() ) ;
         }

         grantConfBuilder.append( OM_CLUSTER_FIELD_GRANTCONF,
                                  grantArray.arr() ) ;
         newClusterInfo = grantConfBuilder.obj() ;
      }

      rc = updateClusterInfo( clusterName, newClusterInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get cluster info: cluster=%s, rc=%d",
                 clusterName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getOneNodeConfig( const string &businessName,
                                           const string &hostName,
                                           const string &svcname,
                                           BSONObj &config )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName
                                << OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }

         {
            BSONObj configInfo( buffObj.data() ) ;
            BSONObj configs = configInfo.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;
            BSONObjIterator iter( configs ) ;

            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               BSONObj tmpConfig = ele.embeddedObject() ;
               string tmpSvcname = tmpConfig.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ;

               if ( tmpSvcname == svcname )
               {
                  config = tmpConfig.copy() ;
                  break ;
               }
            }
         }
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getConfigByBusiness( const string &businessName,
                                              list<BSONObj> &configList )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;

      condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }

         {
            BSONObj configure( buffObj.data() ) ;

            configList.push_back( configure.copy() ) ;
         }
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get one or more host config.
    *
    * @param(in)  hostName
    *
    * @param(out) config
    * {
    *    "Config": [
    *        {
    *           "BusinessName":"b1",
    *           "BusinessType": "sequoiadb",
    *           "DeployMod": "distribution",
    *           "dbpath":"",
    *           "svcname":"11810",
    *            ...
    *        },
    *        ...
    *     ]
    * }
   **/
   INT32 omDatabaseTool::getConfigByHostName( const string &hostName,
                                              BSONObj &config )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObjBuilder confBuilder ;
      BSONArrayBuilder arrayBuilder ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         string businessName ;
         string businessType ;
         string deployMode ;
         BSONObj nodeConfig ;
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }

         {
            /*
               hostConfig
               {
                  "HostName": "h1", "BusinessName":"b1",
                  "BusinessType": "sequoiadb", "DeployMod": "distribution",
                  "Config": [
                     { "dbpath":"xxxx", "svcname":"11810", ... },
                     ...
                  ]
               }
            */
            BSONObj configure( buffObj.data() ) ;

            businessType = configure.getStringField(
                                            OM_CONFIGURE_FIELD_BUSINESSTYPE ) ;
            deployMode   = configure.getStringField(
                                            OM_CONFIGURE_FIELD_DEPLOYMODE ) ;
            businessName = configure.getStringField(
                                            OM_CONFIGURE_FIELD_BUSINESSNAME ) ;
            nodeConfig   = configure.getObjectField(
                                                  OM_CONFIGURE_FIELD_CONFIG ) ;
            BSONObjIterator iter( nodeConfig ) ;

            while ( iter.more() )
            {
               BSONObjBuilder innerBuilder ;
               BSONElement ele = iter.next() ;

               innerBuilder.appendElements( ele.embeddedObject() ) ;
               innerBuilder.append( OM_BSON_BUSINESS_TYPE, businessType ) ;
               innerBuilder.append( OM_BUSINESS_FIELD_DEPLOYMOD, deployMode ) ;
               innerBuilder.append( OM_BSON_BUSINESS_NAME, businessName ) ;
               arrayBuilder.append( innerBuilder.obj() ) ;
            }
         }
      }

      /*
      arrayBuilder
         [
            {
               "BusinessName":"b1",
               "BusinessType": "sequoiadb",
               "DeployMod": "distribution",
               "dbpath":"",
               "svcname":"11810",
                ...
            },
            ...
         ]
      */
      confBuilder.append( OM_BSON_FIELD_CONFIG, arrayBuilder.arr() ) ;
      config = confBuilder.obj() ;

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get all the address of the business( seqipoadb: standalone, coord )
    *
    * @param(in)  businessName
    *
    * @param(out) addressList
    *
   */
   INT32 omDatabaseTool::getBusinessAddressWithConfig(
                                       const string &businessName,
                                       vector<simpleAddressInfo> &addressList )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID  = -1 ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName )  ;
      string businessType = "" ;
      string deployMod = "" ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            string hostName = result.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;

            if ( 0 == businessType.length() )
            {
               businessType = result.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
            }

            if ( 0 == deployMod.length() )
            {
               deployMod = result.getStringField(
                                                OM_BUSINESS_FIELD_DEPLOYMOD ) ;
            }

            if ( OM_BUSINESS_SEQUOIADB == businessType )
            {
               BSONObj nodes = result.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;
               BSONObjIterator iterBson( nodes ) ;

               while ( iterBson.more() )
               {
                  simpleAddressInfo address ;
                  BSONElement ele = iterBson.next() ;
                  BSONObj oneNode = ele.embeddedObject() ;
                  string role = oneNode.getStringField(
                                                   OM_CONFIGURE_FIELD_ROLE ) ;

                  address.hostName = hostName ;
                  address.port = oneNode.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ;
                  if ( OM_DEPLOY_MOD_DISTRIBUTION == deployMod &&
                       OM_NODE_ROLE_COORD == role )
                  {
                     addressList.push_back( address ) ;
                  }
                  else if ( OM_DEPLOY_MOD_STANDALONE == deployMod &&
                            OM_NODE_ROLE_STANDALONE == role )
                  {
                     addressList.push_back( address ) ;
                  }
               }
            }
            else if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == businessType ||
                      OM_BUSINESS_SEQUOIASQL_MYSQL == businessType )
            {
               BSONObj nodes = result.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;
               BSONObjIterator iterBson( nodes ) ;

               while ( iterBson.more() )
               {
                  simpleAddressInfo address ;
                  BSONElement ele = iterBson.next() ;
                  BSONObj oneNode = ele.embeddedObject() ;

                  address.hostName = hostName ;
                  address.port = oneNode.getStringField(
                                                   OM_CONFIGURE_FIELD_PORT2 ) ;

                  addressList.push_back( address ) ;
               }
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

   /**
    * Get all the catalog address of the business( seqipoadb )
    *
    * @param(in)  businessName
    *
    * @param(out) addressList
    *
   */
   INT32 omDatabaseTool::getCatalogAddressWithConfig(
                                       const string &businessName,
                                       vector<simpleAddressInfo> &addressList )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID  = -1 ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName )  ;
      string businessType = "" ;
      string deployMod = "" ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            string hostName = result.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;

            if ( 0 == businessType.length() )
            {
               businessType = result.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
            }

            if ( 0 == deployMod.length() )
            {
               deployMod = result.getStringField(
                                                OM_BUSINESS_FIELD_DEPLOYMOD ) ;
            }

            if ( OM_BUSINESS_SEQUOIADB == businessType &&
                 OM_DEPLOY_MOD_DISTRIBUTION == deployMod )
            {
               BSONObj nodes = result.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;
               BSONObjIterator iterBson( nodes ) ;

               while ( iterBson.more() )
               {
                  simpleAddressInfo address ;
                  BSONElement ele = iterBson.next() ;
                  BSONObj oneNode = ele.embeddedObject() ;
                  string role = oneNode.getStringField( OM_CONF_DETAIL_ROLE ) ;

                  address.hostName = hostName ;
                  address.port = oneNode.getStringField(
                                                      OM_CONF_DETAIL_SVCNAME ) ;
                  if ( OM_NODE_ROLE_CATALOG == role )
                  {
                     addressList.push_back( address ) ;
                  }
               }
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


   INT32 omDatabaseTool::getHostUsedPort( const string &hostName,
                                          vector<string> &portList )
   {
      INT32 rc = SDB_OK ;
      BSONObj hostConfig ;
      BSONObj config ;

      rc = getConfigByHostName( hostName, hostConfig ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get host config, host=%s, rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

      /*
      hostConfig
      {
         "Config": [
             {
                "BusinessName":"b1",
                "BusinessType": "sequoiadb",
                "DeployMod": "distribution",
                "dbpath":"",
                "svcname":"11810",
                 ...
             },
             ...
          ]
      }
      */
      config = hostConfig.getObjectField( OM_BSON_FIELD_CONFIG ) ;

      {
         BSONObjIterator iter( config ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj nodeConfig = ele.embeddedObject() ;
            string buzType = nodeConfig.getStringField(
                                             OM_CONFIGURE_FIELD_BUSINESSTYPE ) ;

            if ( OM_BUSINESS_SEQUOIADB == buzType )
            {
               portList.push_back( nodeConfig.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omDatabaseTool::isConfigExist( const string &businessName,
                                          const string &hostName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName
                             << OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;
      BSONObj selector ;
      BSONObj configure ;

      rc = _getOneConfigure( condition, selector, configure ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isConfigExistOfCluster( const string &hostName,
                                                   const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_CLUSTERNAME << clusterName <<
                                OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;
      BSONObj selector ;
      BSONObj configure ;

      rc = _getOneConfigure( condition, selector, configure ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get configure info:rc=%d", rc ) ;
         goto error ;
      }

      isExist = TRUE ;

   done:
      return isExist ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::insertConfigure( const string &businessName,
                                          const string &hostName,
                                          const string &businessType,
                                          const string &clusterName,
                                          const string &deployMode,
                                          const BSONObj &oneNodeConfig )
   {
      INT32 rc = SDB_OK ;
      BSONObj filter = BSON( OM_BSON_FIELD_HOST_NAME << "" <<
                             OM_BSON_FIELD_HOST_USER << "" <<
                             OM_BSON_FIELD_HOST_PASSWD << "" <<
                             OM_BSON_FIELD_HOST_SSHPORT << "" ) ;
      BSONObj oneConf = oneNodeConfig.filterFieldsUndotted( filter, false ) ;
      BSONObj newConfigure ;
      BSONArrayBuilder arrayBuilder ;

      arrayBuilder.append( oneConf ) ;

      newConfigure = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName <<
                           OM_CONFIGURE_FIELD_HOSTNAME     << hostName <<
                           OM_CONFIGURE_FIELD_BUSINESSTYPE << businessType <<
                           OM_CONFIGURE_FIELD_CLUSTERNAME  << clusterName <<
                           OM_CONFIGURE_FIELD_DEPLOYMODE   << deployMode <<
                           OM_CONFIGURE_FIELD_CONFIG << arrayBuilder.arr() ) ;

      rc = rtnInsert( OM_CS_DEPLOY_CL_CONFIGURE, newConfigure, 1, 0, _cb );
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to store config into table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::upsertConfigure( const string &businessName,
                                          const string &hostName,
                                          const BSONObj &newConfig,
                                          INT64 &updateNum )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME <<
            businessName << OM_CONFIGURE_FIELD_HOSTNAME <<  hostName ) ;
      BSONObj updator = BSON( "$replace" << newConfig ) ;
      BSONObj hint ;
      utilUpdateResult upResult ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_CONFIGURE, condition, updator, hint,
                      FLG_UPDATE_UPSERT, _cb, &upResult ) ;
      updateNum = upResult.updateNum() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "falied to update host config,"
                          "condition=%s,updator=%s,rc=%d",
                 condition.toString().c_str(),
                 updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::appendConfigure( const string &businessName,
                                          const string &hostName,
                                          const BSONObj &oneNodeConfig )
   {
      INT32 rc = SDB_OK ;
      BSONObj filter  = BSON( OM_BSON_FIELD_HOST_NAME << "" <<
                              OM_BSON_FIELD_HOST_USER << "" <<
                              OM_BSON_FIELD_HOST_PASSWD << "" <<
                              OM_BSON_FIELD_HOST_SSHPORT << "" ) ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName
                               << OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;
      BSONObj updator ;
      BSONObj hint ;
      BSONObj config ;
      BSONObj oneConf = oneNodeConfig.filterFieldsUndotted( filter, FALSE ) ;
      BSONArrayBuilder arrayBuilder ;

      arrayBuilder.append( oneConf ) ;

      config = BSON( OM_CONFIGURE_FIELD_CONFIG  << arrayBuilder.arr() ) ;
      updator = BSON( "$addtoset" << config ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_CONFIGURE, condition, updator, hint,
                      0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to update config for %s in %s:rc=%d",
                 hostName.c_str(), OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::_removeConfigure( const BSONObj &condition )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_CONFIGURE, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete configure from table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeConfigure( const string &businessName,
                                          const string &hostName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME <<
            businessName << OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;

      rc = _removeConfigure( condition ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete configure from table:%s,"
                          "%s=%s,%s=%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE,
                 OM_CONFIGURE_FIELD_BUSINESSNAME, businessName.c_str(),
                 OM_CONFIGURE_FIELD_HOSTNAME, hostName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeConfigure( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME
                                       << businessName ) ;

      rc = _removeConfigure( condition ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete configure from table:%s,"
                          "%s=%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE,
                 OM_CONFIGURE_FIELD_BUSINESSNAME, businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getAuth( const string &businessName,
                                  string &authUser, string &authPasswd )
   {
      INT32 rc = SDB_OK ;
      BSONObj authInfo ;

      rc = getAuth( businessName, authInfo ) ;
      if( SDB_OK == rc )
      {
         authUser = authInfo.getStringField( OM_AUTH_FIELD_USER ) ;
         authPasswd = authInfo.getStringField( OM_AUTH_FIELD_PASSWD ) ;
      }

      return rc ;
   }

   INT32 omDatabaseTool::getAuth( const string &businessName,
                                  BSONObj &authInfo )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj condition = BSON( OM_BUSINESS_FIELD_NAME << businessName )  ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS_AUTH, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            BSONObj filter = BSON( "_id" << "" ) ;
            BSONObj newResult = result.filterFieldsUndotted( filter, FALSE ) ;

            authInfo = newResult.copy() ;
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

   INT32 omDatabaseTool::upsertAuth( const string &businessName,
                                     const string &authUser,
                                     const string &authPasswd )
   {
      BSONObj options ;

      return upsertAuth( businessName, authUser, authPasswd, options ) ;
   }

   INT32 omDatabaseTool::upsertAuth( const string &businessName,
                                     const string &authUser,
                                     const string &authPasswd,
                                     BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_AUTH_FIELD_BUSINESS_NAME << businessName ) ;
      BSONObj updator ;
      BSONObj hint ;
      BSONObjBuilder updatorBuilder ;

      if ( !authUser.empty() )
      {
         updatorBuilder.append( OM_AUTH_FIELD_USER, authUser ) ;
         updatorBuilder.append( OM_AUTH_FIELD_PASSWD, authPasswd ) ;
      }

      if ( !options.isEmpty() )
      {
         updatorBuilder.appendElements( options ) ;
      }

      updator = BSON( "$set" << updatorBuilder.obj() ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_BUSINESS_AUTH, condition, updator, hint,
                      FLG_UPDATE_UPSERT, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "falied to update business auth,"
                          "condition=%s,updator=%s,rc=%d",
                 condition.toString().c_str(),
                 updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeAuth( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_AUTH_FIELD_BUSINESS_NAME << businessName ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS_AUTH, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete configure from table:%s,"
                          "%s=%s,%s=%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH,
                 OM_AUTH_FIELD_BUSINESS_NAME, businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::_getOneHostInfo( const BSONObj &matcher,
                                          const BSONObj &selector,
                                          BSONObj &hostInfo )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj order ;
      BSONObj hint ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 0,
                     _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query host:rc=%d,condition=%s", rc,
                 matcher.toString().c_str() ) ;
         goto error ;
      }

      while( TRUE )
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

            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            hostInfo = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDERROR, "Failed to query host info:rc=%d", rc ) ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::_getOneConfigure( const BSONObj &condition,
                                           const BSONObj &selector,
                                           BSONObj &configure )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj order ;
      BSONObj hint ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            configure = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDWARNING, "Failed to query info:rc=%d", rc ) ;
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

   INT32 omDatabaseTool::upsertPackage( const string &hostName,
                                        const string &packageName,
                                        const string &installPath,
                                        const string &version )
   {
      INT32 rc = SDB_OK ;
      BSONObj packageList ;
      BSONObj packageInfo ;
      BSONObj hostInfo ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj updator ;
      BSONObj hint ;

      condition = BSON( OM_HOST_FIELD_NAME << hostName  ) ;
      selector  = BSON( OM_HOST_FIELD_PACKAGES << "" ) ;

      rc = _getOneHostInfo( condition, selector, hostInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get host info: host=%s, rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

      packageList = hostInfo.getObjectField( OM_HOST_FIELD_PACKAGES ) ;

      {
         BSONArrayBuilder packageListBuilder ;
         BSONObjIterator iter( packageList ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj package = ele.embeddedObject() ;
            string tmpPackageName = package.getStringField(
                                                OM_HOST_FIELD_PACKAGENAME ) ;

            if ( packageName != tmpPackageName )
            {
               packageListBuilder.append( package ) ;
            }
         }

         packageListBuilder.append(
                     BSON( OM_HOST_FIELD_PACKAGENAME << packageName <<
                           OM_HOST_FIELD_INSTALLPATH << installPath <<
                           OM_HOST_FIELD_VERSION     << version ) ) ;

         packageInfo = BSON( OM_HOST_FIELD_PACKAGES <<
                             packageListBuilder.arr() ) ;
      }

      updator = BSON( "$set" << packageInfo ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_HOST, condition, updator, hint,
                      0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table failed:table=%s,updator=%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getHostNameByAddress( const string &address,
                                               string &hostName )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher = BSON( "$or" << BSON_ARRAY(
                                    BSON( OM_HOST_FIELD_NAME << address ) <<
                                    BSON( OM_HOST_FIELD_IP   << address ) ) ) ;
      BSONObj selector ;
      BSONObj hostInfo ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get host info:rc=%d", rc ) ;
         goto error ;
      }

      hostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getHostInfoByAddress( const string &address,
                                               BSONObj &hostInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher = BSON( "$or" << BSON_ARRAY(
                                    BSON( OM_HOST_FIELD_NAME << address ) <<
                                    BSON( OM_HOST_FIELD_IP   << address ) ) ) ;
      BSONObj selector ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get host info:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omDatabaseTool::isHostExistOfClusterByAddr(
                                                   const string &address,
                                                   const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj hostInfo ;

      matcher = BSON( "$or" << BSON_ARRAY(
                              BSON( OM_HOST_FIELD_NAME << address ) <<
                              BSON( OM_HOST_FIELD_IP   << address ) ) <<
                      OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isHostExistOfCluster( const string &hostName,
                                                 const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj hostInfo ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName <<
                      OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isHostExistOfClusterByIp( const string &IP,
                                                     const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj hostInfo ;

      matcher = BSON( OM_HOST_FIELD_IP << IP <<
                      OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isHostHasPackage( const string &hostName,
                                             const string &packageName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj hostInfo ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName <<
                      OM_HOST_FIELD_PACKAGES"."OM_HOST_FIELD_PACKAGENAME <<
                            packageName ) ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::getHostPackagePath( const string &hostName,
                                               const string &packageName,
                                               string &installPath )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj hostInfo ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName <<
                      OM_HOST_FIELD_PACKAGES"."OM_HOST_FIELD_PACKAGENAME <<
                            packageName ) ;

      rc = _getOneHostInfo( matcher, selector, hostInfo ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      if ( TRUE == isExist )
      {
         BSONObj packages = hostInfo.getObjectField( OM_HOST_FIELD_PACKAGES ) ;
         BSONObjIterator iter( packages ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj onePkg = ele.embeddedObject() ;
            string name = onePkg.getStringField( OM_HOST_FIELD_PACKAGENAME ) ;
            string path = onePkg.getStringField( OM_HOST_FIELD_INSTALLPATH ) ;

            if( name == packageName )
            {
               installPath = path ;
               break ;
            }
         }
      }

      return isExist ;
   }

   INT32 omDatabaseTool::removeHost( const string &address,
                                     const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      BSONObj hint ;

      condition = BSON( "$or" << BSON_ARRAY(
                              BSON( OM_HOST_FIELD_NAME << address ) <<
                              BSON( OM_HOST_FIELD_IP   << address ) ) <<
                      OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_HOST, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to delete host from table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /**
    * Get all the host config of the cluster.
   */
   INT32 omDatabaseTool::getHostInfoByCluster( const string &clusterName,
                                               list<BSONObj> &hostList )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BSONObj condition = BSON( OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;

      //query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, condition, order, hint, 0,
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_HOST, rc ) ;
            goto error ;
         }

         {
            BSONObj hostInfo( buffObj.data() ) ;

            hostList.push_back( hostInfo.copy() ) ;
         }
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::_getOneRelationship( const BSONObj &condition,
                                              const BSONObj &selector,
                                              BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj order ;
      BSONObj hint ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_RELATIONSHIP, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;

            info = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDWARNING, "Failed to query info:rc=%d", rc ) ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::_getOnePluginInfo( const BSONObj &condition,
                                            const BSONObj &selector,
                                            BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      SINT64 contextID = -1 ;
      BSONObj order ;
      BSONObj hint ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_PLUGINS, selector, condition, order,
                     hint, 0, _cb, 0, 1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;

            info = result.copy() ;
            isExist = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDWARNING, "Failed to query info:rc=%d", rc ) ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::createRelationship( const string &name,
                                             const string &fromBuzName,
                                             const string &toBuzName,
                                             const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      time_t now = time( NULL ) ;
      BSONObj record ;
      BSONObjBuilder builder ;

      builder.append( OM_RELATIONSHIP_FIELD_NAME, name ) ;
      builder.append( OM_RELATIONSHIP_FIELD_FROM, fromBuzName ) ;
      builder.append( OM_RELATIONSHIP_FIELD_TO, toBuzName ) ;
      builder.append( OM_RELATIONSHIP_FIELD_OPTIONS, options ) ;
      builder.appendTimestamp( OM_RELATIONSHIP_FIELD_CREATETIME,
                               (unsigned long long)now * 1000, 0 ) ;

      record = builder.obj() ;
      rc = rtnInsert( OM_CS_DEPLOY_CL_RELATIONSHIP, record, 1, 0, _cb ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "insert task failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omDatabaseTool::isRelationshipExist( const string &name )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj condition = BSON( OM_RELATIONSHIP_FIELD_NAME << name ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOneRelationship( condition, selector, info ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isRelationshipExistByBusiness(
                                                   const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj condition = BSON( "$or" << BSON_ARRAY(
                        BSON( OM_RELATIONSHIP_FIELD_FROM << businessName ) <<
                        BSON( OM_RELATIONSHIP_FIELD_TO   << businessName ) ) ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOneRelationship( condition, selector, info ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   INT32 omDatabaseTool::getRelationshipInfo( const string &name,
                                              string &fromBuzName,
                                              string &toBuzName )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_RELATIONSHIP_FIELD_NAME << name ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOneRelationship( condition, selector, info ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get info: name=%s, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      fromBuzName = info.getStringField( OM_RELATIONSHIP_FIELD_FROM ) ;
      toBuzName = info.getStringField( OM_RELATIONSHIP_FIELD_TO ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getRelationshipOptions( const string &name,
                                                 BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_RELATIONSHIP_FIELD_NAME << name ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOneRelationship( condition, selector, info ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get info: name=%s, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      options = info.getObjectField( OM_RELATIONSHIP_FIELD_OPTIONS ).copy() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getRelationshipList( list<BSONObj> &relationshipList )
   {
      INT32 rc = SDB_OK ;
      SINT64 numToSkip   = 0 ;
      SINT64 numToReturn = -1 ;
      SINT64 contextID   = -1 ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj filter = BSON( "_id" << "" ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_RELATIONSHIP, selector, condition, order,
                     hint, 0, _cb, numToSkip, numToReturn,
                     _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            BSONObj newResult = result.filterFieldsUndotted( filter, FALSE ) ;

            relationshipList.push_back( newResult.copy() ) ;
         }
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removeRelationship( const string &name )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_RELATIONSHIP_FIELD_NAME << name ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_RELATIONSHIP, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to remove relationship: rc=%d",
                 OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omDatabaseTool::isPluginExist( const string &name )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj condition = BSON( OM_PLUGINS_FIELD_NAME << name ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOnePluginInfo( condition, selector, info ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   BOOLEAN omDatabaseTool::isPluginBusinessTypeExist(
                                                   const string& businessType )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = TRUE ;
      BSONObj condition = BSON( OM_PLUGINS_FIELD_BUSINESSTYPE << businessType ) ;
      BSONObj selector ;
      BSONObj info ;

      rc = _getOnePluginInfo( condition, selector, info ) ;
      if ( rc )
      {
         isExist = FALSE ;
      }

      return isExist ;
   }

   INT32 omDatabaseTool::getPluginInfoByBusinessType( const string &businessType,
                                                      BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON( OM_PLUGINS_FIELD_BUSINESSTYPE << businessType ) ;
      BSONObj selector ;

      rc = _getOnePluginInfo( condition, selector, info ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get plugin info: business type=%s, rc=%d",
                 businessType.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::getPluginList( list<BSONObj> &pluginList )
   {
      INT32 rc = SDB_OK ;
      SINT64 numToSkip   = 0 ;
      SINT64 numToReturn = -1 ;
      SINT64 contextID   = -1 ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj filter = BSON( "_id" << "" ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_PLUGINS, selector, condition, order,
                     hint, 0, _cb, numToSkip, numToReturn,
                     _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
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
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_RELATIONSHIP, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            BSONObj newResult = result.filterFieldsUndotted( filter, FALSE ) ;

            pluginList.push_back( newResult.copy() ) ;
         }
      }

   done:
      if( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::upsertPlugin( const string &name,
                                       const string&businessType,
                                       const string &serviceName )
   {
      INT32 rc = SDB_OK ;
      time_t now = time( NULL ) ;
      BSONObj condition ;
      BSONObj updator ;
      BSONObj hint ;
      BSONObjBuilder builder ;

      if ( name.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "plugin name cannot be empty" ) ;
         goto error ;
      }

      if ( businessType.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "plugin business type cannot be empty" ) ;
         goto error ;
      }

      if ( serviceName.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "plugin service name cannot be empty" ) ;
         goto error ;
      }

      builder.append( OM_PLUGINS_FIELD_NAME, name ) ;
      builder.append( OM_PLUGINS_FIELD_BUSINESSTYPE, businessType ) ;
      builder.append( OM_PLUGINS_FIELD_SERVICENAME, serviceName ) ;
      builder.appendTimestamp( OM_PLUGINS_FIELD_UPDATETIME,
                               (unsigned long long)now * 1000, 0 ) ;

      condition = BSON( OM_PLUGINS_FIELD_NAME << name <<
                        OM_PLUGINS_FIELD_BUSINESSTYPE << businessType ) ;
      updator = BSON( "$replace" << builder.obj() ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_PLUGINS, condition, updator, hint,
                      FLG_UPDATE_UPSERT, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "falied to update business auth,"
                          "condition=%s,updator=%s,rc=%d",
                 condition.toString().c_str(),
                 updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDatabaseTool::removePlugin( const string &name,
                                       const string &businessType )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition = BSON(OM_PLUGINS_FIELD_NAME << name <<
                               OM_PLUGINS_FIELD_BUSINESSTYPE << businessType ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_PLUGINS, condition, hint, 0, _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to remove plugin: rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /* ======================== Trans ======================== */
   /*
      Trans order
      1. SYSCONFIGURE
      2. SYSBUSINESS
   */

   INT32 omDatabaseTool::addPackageOfHosts( set<string> &hostList,
                                            const string &packageName,
                                            const string &installPath,
                                            const string &version )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTransBegin( _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to begin trans: name=%s, rc=%d",
                 packageName.c_str(), rc ) ;
         goto error ;
      }

      {
         set<string>::iterator iter ;

         for( iter = hostList.begin(); iter != hostList.end(); ++iter )
         {
            string hostName = *iter ;

            upsertPackage( hostName, packageName, installPath, version ) ;
         }
      }

      rc = rtnTransCommit( _cb, _pDpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to commit trans: name=%s, rc=%d",
                 packageName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      rtnTransRollback( _cb, _pDpsCB ) ;
      goto done ;
   }

   INT32 omDatabaseTool::addNodeConfigOfBusiness( const string &clusterName,
                                                  const string &businessName,
                                                  const string &businessType,
                                                  const BSONObj &newConfig )
   {
      INT32 rc = SDB_OK ;
      INT64 updateNum = 0 ;
      string deployMod ;
      BSONObj hostInfoList ;
      BSONObjBuilder builder ;
      BSONArrayBuilder hostLocation ;
      BSONObj hostInfoFilter = BSON( OM_CONFIGURE_FIELD_ERRNO  << "" <<
                                     OM_CONFIGURE_FIELD_DETAIL << "" ) ;

      hostInfoList = newConfig.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;

      rc = rtnTransBegin( _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to begin trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      {
         BSONObjIterator iter( hostInfoList ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj tmpConfig = ele.embeddedObject() ;
            BSONObj filterConfg = tmpConfig.filterFieldsUndotted( hostInfoFilter,
                                                                  FALSE ) ;
            string hostName = filterConfg.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;

            if ( 0 == deployMod.length() )
            {
               deployMod = filterConfg.getStringField(
                                             OM_CONFIGURE_FIELD_DEPLOYMODE ) ;
            }

            rc = upsertConfigure( businessName, hostName, filterConfg,
                                  updateNum ) ;
            if( rc )
            {
               PD_LOG( PDERROR, "failed to update configure,hostname=%s,rc=%d",
                       hostName.c_str(), rc ) ;
               goto error ;
            }

            //used to construct business info
            hostLocation.append( BSON( OM_CONFIGURE_FIELD_HOSTNAME <<
                                       hostName ) ) ;
         }
      }

      //add business
      builder.append( OM_BUSINESS_FIELD_LOCATION, hostLocation.arr() ) ;
      rc = addBusinessInfo( OM_BUSINESS_ADDTYPE_INSTALL, clusterName,
                            businessName, businessType,
                            deployMod, builder.obj() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add business,name=%s,rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = rtnTransCommit( _cb, _pDpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to commit trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      rtnTransRollback( _cb, _pDpsCB ) ;
      goto done ;
   }

   /**
    * update node config of the business
    *
    * @param(in) businessName
    *
    * @param(in) newConfig
    * {
    *    "HostInfo": [
    *       {
    *          "BusinessName": xxx,
    *          "BusinessType": xxx,
    *          "ClusterName": xxx,
    *          "DeployMod": xxx,
    *          "HostName": xxx,
    *          "Config": [
    *             { "role": xxx, "svcname": xxx, "dbpath": xxx, ... },
    *             ...
    *          ]
    *       },
    *       ...
    *    ]
    * }
   */
   INT32 omDatabaseTool::updateNodeConfigOfBusiness( const string &businessName,
                                                     const BSONObj &newConfig )
   {
      INT32 rc = SDB_OK ;
      INT64 updateNum = 0 ;
      BSONObj buzInfo ;
      BSONObjBuilder builder ;
      BSONArrayBuilder hostLocation ;
      BSONObj hostInfoFilter = BSON( OM_CONFIGURE_FIELD_ERRNO  << "" <<
                                     OM_CONFIGURE_FIELD_DETAIL << "" ) ;
      set<string> newHostList ;
      string deployMod = "" ;

      rc = getOneBusinessInfo( businessName, buzInfo ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "business does not exist: %s",
                 businessName.c_str() ) ;
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "failed to get business info,name=%s,rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = rtnTransBegin( _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to begin trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      //host and node info by omagent
      {
         BSONObj hostInfoList = newConfig.getObjectField(
                                                   OM_BSON_FIELD_HOST_INFO ) ;
         BSONObjIterator iter( hostInfoList ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj tmpConfig = ele.embeddedObject() ;
            BSONObj filterConfg = tmpConfig.filterFieldsUndotted( hostInfoFilter,
                                                                  FALSE ) ;
            string hostName = filterConfg.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;

            if ( 0 == deployMod.length() )
            {
               deployMod = filterConfg.getStringField(
                                             OM_CONFIGURE_FIELD_DEPLOYMODE ) ;
            }

            newHostList.insert( hostName ) ;

            rc = upsertConfigure( businessName, hostName, filterConfg,
                                  updateNum ) ;
            if( rc )
            {
               PD_LOG( PDERROR, "failed to update configure: hostname=%s,rc=%d",
                       hostName.c_str(), rc ) ;
               goto error ;
            }

            //used to construct business info
            hostLocation.append( BSON( OM_CONFIGURE_FIELD_HOSTNAME <<
                                       hostName ) ) ;
         }
      }

      //delete the host config that does not exist
      {
         BSONObj deleteCondition ;
         BSONObjBuilder deleteBuilder ;
         BSONObjBuilder conditionBuilder ;
         BSONArrayBuilder notDeleteArray ;

         for ( set<string>::iterator iter = newHostList.begin();
               iter != newHostList.end(); ++iter )
         {
            string hostName = *iter ;
            notDeleteArray.append( hostName ) ;
         }
         conditionBuilder.append( "$nin", notDeleteArray.arr() ) ;

         deleteBuilder.append( OM_CONFIGURE_FIELD_BUSINESSNAME, businessName ) ;
         deleteBuilder.append( OM_CONFIGURE_FIELD_HOSTNAME,
                               conditionBuilder.obj() ) ;
         deleteCondition = deleteBuilder.obj() ;

         rc = _removeConfigure( deleteCondition ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to remove configure,condition=%s,rc=%d",
                    deleteCondition.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      //upsert business
      {
         BSONObj condition = BSON( OM_BUSINESS_FIELD_LOCATION << "" <<
                                   OM_BUSINESS_FIELD_DEPLOYMOD << "" <<
                                   OM_BUSINESS_FIELD_ID << "" ) ;
         BSONObj businessInfo = buzInfo.filterFieldsUndotted( condition,
                                                              FALSE ) ;

         builder.appendElements( businessInfo ) ;
      }

      builder.append( OM_BUSINESS_FIELD_DEPLOYMOD, deployMod ) ;
      builder.append( OM_BUSINESS_FIELD_LOCATION, hostLocation.arr() ) ;
      rc = upsertBusinessInfo( businessName, builder.obj(), updateNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to update business info: name=%s,rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = rtnTransCommit( _cb, _pDpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to commit trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      rtnTransRollback( _cb, _pDpsCB ) ;
      goto done ;
   }

   INT32 omDatabaseTool::unbindBusiness( const string &businessName )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTransBegin( _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to begin trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = removeConfigure( businessName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to remove configure: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = removeBusiness( businessName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to remove business: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = rtnTransCommit( _cb, _pDpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to commit trans: name=%s, rc=%d",
                 businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      rtnTransRollback( _cb, _pDpsCB ) ;
      goto done ;
   }

   INT32 omDatabaseTool::unbindHost( const string &clusterName,
                                     list<string> &hostList )
   {
      INT32 rc = SDB_OK ;
      list<string>::iterator iter ;

      rc = rtnTransBegin( _cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to begin trans: name=%s, rc=%d",
                 clusterName.c_str(), rc ) ;
         goto error ;
      }

      for ( iter = hostList.begin(); iter != hostList.end(); ++iter )
      {
         string hostName = *iter ;

         rc = removeHost( hostName, clusterName ) ;
         if ( rc )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "failed to remove host: host=%s, rc=%d",
                    hostName.c_str(), rc ) ;
            goto error ;
         }
      }

      rc = rtnTransCommit( _cb, _pDpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to commit trans: name=%s, rc=%d",
                 clusterName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      rtnTransRollback( _cb, _pDpsCB ) ;
      goto done ;
   }

   INT32 omDatabaseTool::createCollection( const CHAR *pCollection )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTestAndCreateCL( pCollection, _cb, _pDMSCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create collection: name=%s, rc=%d",
                 pCollection, rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omDatabaseTool::createCollectionIndex( const CHAR *pCollection,
                                                const CHAR *pIndex )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;

      rc = fromjson ( pIndex, indexDef ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to build index object: rc=%d", rc ) ;
         goto error ;
      }

      rc = rtnTestAndCreateIndex( pCollection, indexDef, _cb, _pDMSCB,
                                  NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create index: name=%s, rc=%d",
                 pCollection, rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omDatabaseTool::removeCollectionIndex( const CHAR *pCollection,
                                                const CHAR *pIndex )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      BSONElement ele ;

      rc = fromjson ( pIndex, indexDef ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to build index object, rc = %d", rc ) ;
         goto error ;
      }

      ele = indexDef.getField( IXM_NAME_FIELD ) ;

      rc = rtnDropIndexCommand( pCollection, ele, _cb, _pDMSCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to drop index: name=%s, rc=%d",
                 pCollection, rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omAuthTool::createOmsvcDefaultUsr( const CHAR *pPluginPasswd,
                                            INT32 pluginPasswdLen )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN need = TRUE ;

      rc = _pAuthCB->needAuthenticate( _cb, need ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check if need to authenticate:%d", rc ) ;
         goto error ;
      }

      if ( FALSE == need && _pAuthCB->authEnabled() )
      {
         rc = createAdminUsr() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create default user: rc=%d", rc ) ;
            goto error ;
         }

      }

      rc = createPluginUsr( pPluginPasswd, pluginPasswdLen ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create plugin user: rc=%d", rc ) ;
         goto error ;
      }

      rc = _pAuthCB->needAuthenticate( _cb, need ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to check if need to authenticate:%d", rc ) ;
         goto error ;
      }
      if ( !need && _pAuthCB->authEnabled() )
      {
         PD_LOG( PDERROR, "can not start auth after adding user" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAuthTool::createAdminUsr()
   {
      INT32 rc = SDB_OK ;
      md5::md5digest digest ;
      BSONObj obj ;
      BSONObjBuilder objBuilder ;

      md5::md5( ( const void * )OM_DEFAULT_LOGIN_PASSWD,
                ossStrlen( OM_DEFAULT_LOGIN_PASSWD ), digest ) ;

      objBuilder.append( SDB_AUTH_USER, OM_DEFAULT_LOGIN_USER ) ;
      objBuilder.append( SDB_AUTH_PASSWD, md5::digestToString( digest ) ) ;
      obj = objBuilder.obj() ;

      rc = _pAuthCB->createUsr( obj, _cb ) ;
      if ( SDB_IXM_DUP_KEY == rc || SDB_AUTH_USER_ALREADY_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create default user: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAuthTool::createPluginUsr( const CHAR *pPasswd, INT32 length )
   {
      INT32 rc = SDB_OK ;
      md5::md5digest digest ;
      BSONObj obj ;
      BSONObjBuilder objBuilder ;

      md5::md5( (const void *)pPasswd, length, digest ) ;

      objBuilder.append( SDB_AUTH_USER, OM_DEFAULT_PLUGIN_USER ) ;
      objBuilder.append( SDB_AUTH_PASSWD, md5::digestToString( digest ) ) ;
      obj = objBuilder.obj() ;

      rc = _pAuthCB->createUsr( obj, _cb ) ;
      if ( SDB_IXM_DUP_KEY == rc || SDB_AUTH_USER_ALREADY_EXIST == rc )
      {
         BSONObj userInfo ;
         string oldPasswd ;
         string newPasswd ;

         rc = getUsrInfo( OM_DEFAULT_PLUGIN_USER, userInfo ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to get plugin user: rc=%d", rc ) ;
            goto error ;
         }

         oldPasswd = userInfo.getStringField( SDB_AUTH_PASSWD ) ;
         newPasswd = obj.getStringField( SDB_AUTH_PASSWD ) ;

         rc = _pAuthCB->updatePasswd( OM_DEFAULT_PLUGIN_USER, oldPasswd,
                                      newPasswd, _cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to create plugin user: rc=%d", rc ) ;
            goto error ;
         }
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create plugin user: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAuthTool::getUsrInfo( const string &user, BSONObj &info )
   {
      INT32 rc = SDB_OK ;

      rc = _pAuthCB->getUsrInfo( user.c_str(), _cb, info ) ;
      if ( rc )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDWARNING, "Failed to get user info: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omAuthTool::generateRandomVisualString( CHAR* pPasswd, INT32 length )
   {
      for ( INT32 i = 0; i < length; )
      {
         CHAR character = rand() % 255 ;

         if ( ( character >= '0' && character <= '9' ) ||
              ( character >= 'a' && character <= 'z' ) )
         {
            pPasswd[i] = character ;
            ++i ;
         }
      }
   }

   omRestTool::omRestTool( ossSocket *socket, restAdaptor *pRestAdaptor,
                           restResponse *response )
   {
      _socket = socket ;
      _pRestAdaptor = pRestAdaptor ;
      _response = response ;
   }

   void omRestTool::sendRecord2Web( list<BSONObj> &records,
                                    const BSONObj *pFilter,
                                    BOOLEAN inFilter )
   {
      list<BSONObj>::iterator iter ;
      for( iter = records.begin(); iter != records.end(); ++iter )
      {
         if( pFilter )
         {
            BSONObj tmp = iter->filterFieldsUndotted( *pFilter, inFilter ) ;
            _response->appendBody( tmp.objdata(), tmp.objsize(), 1 ) ;
         }
         else
         {
            _response->appendBody( iter->objdata(), iter->objsize(), 1 ) ;
         }
      }

      sendResponse( SDB_OK, "" ) ;
   }

   void omRestTool::sendOkRespone()
   {
      sendResponse( SDB_OK, "" ) ;
   }

   void omRestTool::sendResponse( INT32 rc, const string &detail )
   {
      sendResponse( rc, detail.c_str() ) ;
   }

   void omRestTool::sendResponse( INT32 rc, const char *pDetail )
   {
      list<BSONObj>::iterator iter ;
      BSONObj res ;
      BSONObj defaultRes ;
      BSONObjBuilder resBuilder ;

      defaultRes = BSON( OM_REST_RES_RETCODE << rc <<
                         OM_REST_RES_DESP << getErrDesp( rc ) <<
                         OM_REST_RES_DETAIL << pDetail ) ;

      resBuilder.appendElements( defaultRes ) ;
      for ( iter = _msgList.begin(); iter != _msgList.end(); ++iter )
      {
         resBuilder.appendElements( *iter ) ;
      }

      res = resBuilder.obj() ;
      _response->setOPResult( rc, res ) ;
      _pRestAdaptor->sendRest( _socket, _response ) ;
   }

   void omRestTool::sendResponse( const BSONObj &msg )
   {
      _response->setOPResult( SDB_OK, msg ) ;
      _pRestAdaptor->sendRest( _socket, _response ) ;
   }

   INT32 omRestTool::appendResponeContent( const BSONObj &content )
   {
      return _response->appendBody( content.objdata(), content.objsize(), 1 ) ;
   }

   void omRestTool::appendResponeMsg( const BSONObj &msg )
   {
      _msgList.push_back( msg.copy() ) ;
   }

   INT32 omTaskTool::createTask( INT32 taskType, INT64 taskID,
                                 const string &taskName,
                                 const BSONObj &taskInfo,
                                 const BSONArray &resultInfo )
   {
      INT32 rc = SDB_OK ;
      time_t now = time( NULL ) ;
      BSONObj record ;
      BSONObjBuilder builder ;

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
      builder.append( OM_TASKINFO_FIELD_AGENTHOST, _localAgentHost ) ;
      builder.append( OM_TASKINFO_FIELD_AGENTPORT, _localAgentService ) ;
      builder.append( OM_TASKINFO_FIELD_INFO, taskInfo ) ;
      builder.append( OM_TASKINFO_FIELD_ERRNO, SDB_OK ) ;
      builder.append( OM_TASKINFO_FIELD_DETAIL, "" ) ;
      builder.append( OM_TASKINFO_FIELD_PROGRESS, 0 ) ;
      builder.append( OM_TASKINFO_FIELD_RESULTINFO, resultInfo ) ;

      record = builder.obj() ;
      rc = rtnInsert( OM_CS_DEPLOY_CL_TASKINFO, record, 1, 0, _cb ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "insert task failed:rc=%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskTool::notifyAgentMsg( const CHAR *pCmd,
                                     const BSONObj &request,
                                     string &errDetail,
                                     BSONObj &result )
   {
      INT32 rc          = SDB_OK ;
      SINT32 flag       = SDB_OK ;
      INT32 contentSize = 0 ;
      CHAR *pContent    = NULL ;
      MsgHeader *pMsg   = NULL ;
      omManager *om     = NULL ;
      pmdRemoteSession *remoteSession = NULL ;
      BSONObj bsonRequest ;
      SDB_ASSERT( pCmd != NULL , "" ) ;

      /*
         pContent has allocated memory. After the remote session ends,
         it is auto free by the destructor of the session
      */
      rc = msgBuildQueryMsg( &pContent, &contentSize, pCmd,
                             0, 0, 0, -1, &request, NULL, NULL, NULL ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "build msg failed:cmd=%s,rc=%d", OM_NOTIFY_TASK,
                 rc ) ;
         goto error ;
      }

      // create remote session
      om = sdbGetOMManager() ;
      remoteSession = om->getRSManager()->addSession( _cb,
                                                      OM_WAIT_SCAN_RES_INTERVAL,
                                                      NULL ) ;
      if( NULL == remoteSession )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "create remote session failed:rc=%d", rc ) ;
         goto error ;
      }

      // send message to agent
      pMsg = (MsgHeader *)pContent ;
      rc = _sendMsgToLocalAgent( om, remoteSession, pMsg ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "send message to agent failed:rc=%d", rc ) ;
         remoteSession->clearSubSession() ;
         goto error ;
      }

      // receiving for agent's response
      rc = _receiveFromAgent( remoteSession, flag, result ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "receive from agent failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( flag )
      {
         rc = flag ;
         errDetail = result.getStringField( OM_REST_RES_DETAIL ) ;
         PD_LOG( PDERROR, "agent process failed:detail=(%s),rc=%d",
                 errDetail.c_str(), rc ) ;
         goto error ;
      }

   done:
      if( remoteSession )
      {
         _clearSession( om, remoteSession ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskTool::notifyAgentTask( INT64 taskID, string &errDetail )
   {
      INT32 rc = SDB_OK ;
      BSONObj request ;
      BSONObj result ;

      request = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;

      rc = notifyAgentMsg( CMD_ADMIN_PREFIX OM_NOTIFY_TASK,
                           request, errDetail, result ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to notify agent,rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskTool::_sendMsgToLocalAgent( omManager *om,
                                           pmdRemoteSession *pRemoteSession,
                                           MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      MsgRouteID localAgentID ;

      localAgentID = om->updateAgentInfo( _localAgentHost,
                                          _localAgentService ) ;
      if( NULL == pRemoteSession->addSubSession( localAgentID.value ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "addSubSession failed:id=%ld", localAgentID.value ) ;
         goto error ;
      }

      rc = pRemoteSession->sendMsg( pMsg, PMD_EDU_MEM_ALLOC ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "send msg to localhost's agent failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskTool::_receiveFromAgent( pmdRemoteSession *pRemoteSession,
                                        SINT32 &flag,
                                        BSONObj &result )
   {
      INT32 rc           = SDB_OK ;
      SINT32 startFrom   = 0 ;
      SINT32 numReturned = 0 ;
      SINT64 contextID   = -1 ;
      MsgHeader *pRspMsg = NULL ;
      vector<BSONObj> objVec ;
      VEC_SUB_SESSIONPTR subSessionVec ;

      rc = pRemoteSession->waitReply( TRUE, &subSessionVec ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "wait reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if( 1 != subSessionVec.size() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG( PDERROR, "unexpected session size:size=%d",
                 subSessionVec.size() ) ;
         goto error ;
      }

      if( subSessionVec[0]->isDisconnect() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG(PDERROR, "session disconnected:id=%s,rc=%d",
                routeID2String(subSessionVec[0]->getNodeID()).c_str(), rc ) ;
         goto error ;
      }

      pRspMsg = subSessionVec[0]->getRspMsg() ;
      if( NULL == pRspMsg )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG( PDERROR, "receive null response:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgExtractReply( (CHAR *)pRspMsg, &flag, &contextID, &startFrom,
                            &numReturned, objVec ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "extract reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if( objVec.size() > 0 )
      {
         result = objVec[0] ;
         result = result.getOwned() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omTaskTool::_clearSession( omManager *om,
                                   pmdRemoteSession *pRemoteSession )
   {
      if( NULL != pRemoteSession )
      {
         pRemoteSession->clearSubSession() ;
         om->getRSManager()->removeSession( pRemoteSession ) ;
      }
   }

   void omErrorTool::setError( BOOLEAN isCover, const CHAR *pFormat, ... )
   {
      if( _isSet == FALSE || isCover == TRUE )
      {
         va_list ap;

         va_start( ap, pFormat ) ;
         vsnprintf( _errorDetail, PD_LOG_STRINGMAX, pFormat, ap ) ;
         va_end( ap ) ;

         _isSet = TRUE ;
      }
   }

   const CHAR* omErrorTool::getError()
   {
      if( _isSet == FALSE )
      {
         _errorDetail[0] = 0 ;
      }
      return _errorDetail ;
   }

   omArgOptions::omArgOptions( restRequest *pRequest )
   {
      _request = pRequest ;
   }

   INT32 omArgOptions::parseRestArg( const CHAR *pFormat, ... )
   {
      INT32 rc = SDB_OK ;
      va_list vaList ;

      va_start( vaList, pFormat ) ;
      rc = _parserArg( pFormat, vaList ) ;
      va_end( vaList ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *omArgOptions::getErrorMsg()
   {
      return _errorMsg.getError() ;
   }

   INT32 omArgOptions::_parserArg( const CHAR *pFormat, va_list &vaList )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isOptional   = FALSE ;
      const CHAR *specWalk = pFormat ;

      while ( *specWalk )
      {
         CHAR character = *specWalk ;

         if ( '|' == character )
         {
            isOptional = TRUE ;
         }
         else
         {
            const CHAR *pField = va_arg( vaList, const CHAR * ) ;
            string value ;

            SDB_ASSERT( ( NULL != pField ), "pField can not null" ) ;

            if ( FALSE == isOptional &&
                 FALSE == _request->isQueryArgExist( pField ) )
            {
               rc = SDB_INVALIDARG ;
               _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                   pField ) ;
               PD_LOG( PDERROR, _errorMsg.getError() ) ;
               goto error ;
            }

            value = _request->getQuery( pField ) ;

            if ( 's' == character )
            {
               //string
               string *pArg = va_arg( vaList, string * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( value.length() > 0 )
               {
                  *pArg = value ;
               }
            }
            else if ( 'S' == character )
            {
               //string
               string *pArg = va_arg( vaList, string * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( FALSE == isOptional && value.empty() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                      pField ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( value.length() > 0 )
               {
                  *pArg = value ;
               }
            }
            else if ( 'l' == character )
            {
               //int32
               INT32 *pArg = va_arg( vaList, INT32 * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( FALSE == isOptional && value.empty() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                      pField ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( value.length() > 0 )
               {
                  if ( FALSE == ossIsInteger( value.c_str() ) )
                  {
                     rc = SDB_INVALIDARG ;
                     _errorMsg.setError( TRUE, "invalid value: %s = %s",
                                         pField, value.c_str() ) ;
                     PD_LOG( PDERROR, _errorMsg.getError() ) ;
                     goto error ;
                  }

                  *pArg = ossAtoi( value.c_str() ) ;
               }
            }
            else if ( 'L' == character )
            {
               //int64
               INT64 *pArg = va_arg( vaList, INT64 * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( FALSE == isOptional && value.empty() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                      pField ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( value.length() > 0 )
               {
                  if ( FALSE == ossIsInteger( value.c_str() ) )
                  {
                     rc = SDB_INVALIDARG ;
                     _errorMsg.setError( TRUE, "invalid value: %s = %s",
                                         pField, value.c_str() ) ;
                     PD_LOG( PDERROR, _errorMsg.getError() ) ;
                     goto error ;
                  }

                  *pArg = ossAtoll( value.c_str() ) ;
               }
            }
            else if ( 'b' == character )
            {
               //bool
               BOOLEAN *pArg = va_arg( vaList, BOOLEAN * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( FALSE == isOptional && value.empty() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                      pField ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( value.length() > 0 )
               {
                  if ( "true" == value || "TRUE" == value || "True" == value )
                  {
                     *pArg = TRUE ;
                  }
                  else if ( "false" == value || "FALSE" == value ||
                            "False" == value )
                  {
                     *pArg = FALSE ;
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     _errorMsg.setError( TRUE, "get rest info failed: %s invalid",
                                         pField ) ;
                     PD_LOG( PDERROR, _errorMsg.getError() ) ;
                     goto error ;
                  }
               }
            }
            else if ( 'j' == character )
            {
               //json
               BSONObj *pArg = va_arg( vaList, BSONObj * ) ;

               SDB_ASSERT( ( NULL != pArg ), "pArg can not null" ) ;

               if ( FALSE == isOptional && value.empty() )
               {
                  rc = SDB_INVALIDARG ;
                  _errorMsg.setError( TRUE, "get rest info failed: %s is NULL",
                                      pField ) ;
                  PD_LOG( PDERROR, _errorMsg.getError() ) ;
                  goto error ;
               }

               if ( value.length() > 0 )
               {
                  rc = fromjson( value.c_str(), *pArg ) ;
                  if ( rc )
                  {
                     _errorMsg.setError( TRUE, "get rest info failed: %s invalid",
                                         pField ) ;
                     PD_LOG( PDERROR, _errorMsg.getError() ) ;
                     goto error ;
                  }
               }
            }
            else
            {
               SDB_ASSERT( FALSE, "unknow type" ) ;
            }

         }

         ++specWalk ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}

