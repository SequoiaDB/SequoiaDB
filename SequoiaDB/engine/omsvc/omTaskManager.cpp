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

   Source File Name = omTaskManager.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/12/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omTaskManager.hpp"
#include "omCommandTool.hpp"
#include "omDef.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "ossVer.hpp"

using namespace bson ;

namespace engine
{
   static BOOLEAN isSubSet( const BSONObj &source, const BSONObj &find ) ;
   static BOOLEAN isInElement( const BSONObj &source, const string eleKey,
                               const BSONObj &find ) ;
   static INT32 queryOneTask( const BSONObj &selector, const BSONObj &matcher,
                              const BSONObj &orderBy, const BSONObj &hint,
                              BSONObj &oneTask ) ;
   static INT32 queryTasks( const BSONObj &selecor, const BSONObj &matcher,
                            const BSONObj &orderBy, const BSONObj &hint,
                            vector< BSONObj > &tasks ) ;

   BOOLEAN isSubSet( const BSONObj &source, const BSONObj &find )
   {
      BSONObjIterator iter( find ) ;
      while ( iter.more() )
      {
         BSONElement ele       = iter.next() ;
         BSONElement sourceEle = source.getField( ele.fieldName() ) ;
         if ( ele.woCompare( sourceEle, false ) != 0 )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   BOOLEAN isInElement( const BSONObj &source, const string eleKey,
                        const BSONObj &find )
   {
      BSONElement eleSource = source.getField( eleKey ) ;
      if ( eleSource.type() != Array )
      {
         return FALSE ;
      }

      BSONObj obj = eleSource.embeddedObject() ;
      BSONObjIterator iter( obj ) ;
      while ( iter.more() )
      {
         BSONObj oneObj ;
         BSONElement ele = iter.next() ;
         if ( ele.type() != Object )
         {
            continue ;
         }

         oneObj = ele.embeddedObject() ;
         if ( isSubSet( oneObj, find ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   INT32 queryOneTask( const BSONObj &selector, const BSONObj &matcher,
                       const BSONObj &orderBy, const BSONObj &hint ,
                       BSONObj &oneTask )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID = -1 ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, orderBy,
                     hint, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "query table failed:table=%s,rc=%d",
                     OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
         goto error ;
      }

      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         oneTask = result.copy() ;
         goto done ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 queryTasks( const BSONObj &selector, const BSONObj &matcher,
                     const BSONObj &orderBy, const BSONObj &hint,
                     vector< BSONObj > &tasks )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID = -1 ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, orderBy,
                     hint, 0, cb, 0, -1, pdmsCB, pRtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "query table failed:table=%s,rc=%d",
                     OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
         goto error ;
      }

      while (TRUE )
      {
         rtnContextBuf contextBuff ;
         rc = rtnGetMore ( contextID, -1, contextBuff, cb, pRtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_TASKINFO, rc ) ;
            goto error ;
         }
         else
         {
            BSONObj record ;
            _rtnObjBuff rtnObj( contextBuff.data(), contextBuff.size(),
                                contextBuff.recordNum() ) ;
            while( TRUE )
            {
               rc = rtnObj.nextObj( record ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG ( PDERROR, "Failed to get nextObj:rc=%d", rc ) ;
                  goto error ;
               }

               tasks.push_back( record.copy() ) ;
            }
         }
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   omTaskBase::omTaskBase()
   {
   }

   omTaskBase::~omTaskBase()
   {
   }

   INT32 omTaskBase::_getTaskInfo( INT64 taskID, BSONObj &taskInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;
      BSONObj matcher  = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj tmpTaskInfo ;

      rc = queryOneTask( selector, matcher, orderBy, hint, tmpTaskInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="
                          OSS_LL_PRINT_FORMAT",rc=%d",
                 taskID, rc ) ;
         goto error ;
      }

      taskInfo = tmpTaskInfo.getObjectField( OM_TASKINFO_FIELD_INFO ).copy() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskBase::getTaskInfo( BSONObj &taskInfo )
   {
      return SDB_OK ;
   }

   INT32 omTaskBase::checkUpdateInfo(const BSONObj & updateInfo)
   {
      INT32 rc = SDB_OK ;
      if ( !updateInfo.hasField( OM_TASKINFO_FIELD_RESULTINFO )
           || !updateInfo.hasField( OM_TASKINFO_FIELD_PROGRESS )
           || !updateInfo.hasField( OM_TASKINFO_FIELD_STATUS ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "updateinfo miss field:fields=[%s,%s,%s],updateInfo"
                 "=%s", OM_TASKINFO_FIELD_RESULTINFO,
                 OM_TASKINFO_FIELD_PROGRESS, OM_TASKINFO_FIELD_STATUS,
                 updateInfo.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omAddHostTask::omAddHostTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_ADD_HOST ;
   }

   omAddHostTask::~omAddHostTask()
   {

   }

   INT32 omAddHostTask::_getSuccessHost( BSONObj &resultInfo,
                                         set<string> &successHostSet )
   {
      INT32 rc = SDB_OK ;
      BSONObj hosts ;
      hosts = resultInfo.getObjectField( OM_TASKINFO_FIELD_RESULTINFO ) ;
      BSONObjIterator iter( hosts ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj tmpHost = ele.embeddedObject() ;
         string hostName = tmpHost.getStringField( OM_HOST_FIELD_NAME ) ;
         INT32 retCode   = tmpHost.getIntField( OM_REST_RES_RETCODE ) ;
         if ( SDB_OK == retCode )
         {
            successHostSet.insert( hostName ) ;
         }
      }

      return rc ;
   }

   void omAddHostTask::_getOMVersion( string &version )
   {
      INT32 major        = 0 ;
      INT32 minor        = 0 ;
      stringstream stream ;

      ossGetVersion ( &major, &minor, NULL, NULL, NULL, NULL ) ;
      stream << major << "." << minor ;
      version = stream.str() ;
   }

   void omAddHostTask::_getPackageVersion( const BSONObj resultInfo,
                                           const string &hostName,
                                           string &version )
   {
      BSONObjIterator iter( resultInfo ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj oneHost = ele.embeddedObject() ;
         string tmpHostName = oneHost.getStringField( OM_HOST_FIELD_NAME ) ;

         if ( tmpHostName == hostName )
         {
            version = oneHost.getStringField( OM_HOST_FIELD_VERSION ) ;
            break ;
         }
      }
   }

   INT32 omAddHostTask::finish( BSONObj &resultInfo )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      string clusterName ;
      BSONObj taskInfoValue ;
      BSONObj hosts ;
      set<string> successHostSet ;

      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj taskInfo ;
      BSONObj taskResultInfo ;
      string omVersion ;

      _getOMVersion( omVersion ) ;

      rc = _getSuccessHost( resultInfo, successHostSet ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get success host failed:rc=%d", rc ) ;
         goto error ;
      }

      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;
      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      rc = queryOneTask( selector, matcher, orderBy, hint, taskInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      taskInfoValue  = taskInfo.getObjectField( OM_TASKINFO_FIELD_INFO ) ;
      taskResultInfo = resultInfo.getObjectField( OM_TASKINFO_FIELD_RESULTINFO ) ;
      hosts = taskInfoValue.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;
      SDB_ASSERT( !hosts.isEmpty(), "" ) ;
      {
         BSONObjIterator iter( hosts ) ;
         while ( iter.more() )
         {
            BSONObjBuilder oneHostBuilder ;
            BSONElement ele = iter.next() ;
            BSONObj oneHost = ele.embeddedObject() ;
            string hostName = oneHost.getStringField( OM_HOST_FIELD_NAME ) ;

            if ( successHostSet.find( hostName ) == successHostSet.end() )
            {
               // ignore the failure host
               continue ;
            }

            {
               BSONArrayBuilder packageBuilder ;

               {
                  BSONObj packageInfo ;
                  BSONObj oma = oneHost.getObjectField( OM_HOST_FIELD_OMA ) ;
                  string version = oma.getStringField(
                                                   OM_HOST_FIELD_OM_VERSION ) ;
                  string hostName = oneHost.getStringField(
                                                   OM_HOST_FIELD_NAME ) ;
                  string installPath = oneHost.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;

                  if ( version.empty() )
                  {
                     _getPackageVersion( taskResultInfo, hostName, version ) ;
                     if ( version.empty() )
                     {
                        version = omVersion ;
                     }
                  }

                  packageInfo = BSON(
                           OM_HOST_FIELD_PACKAGENAME << OM_BUSINESS_SEQUOIADB <<
                           OM_HOST_FIELD_INSTALLPATH << installPath <<
                           OM_HOST_FIELD_VERSION << version ) ;

                  packageBuilder.append( packageInfo ) ;
               }

               {
                  BSONObj packageInfo ;
                  BSONObj postgres = oneHost.getObjectField(
                                                   OM_HOST_FIELD_POSTGRESQL ) ;
                  string version = postgres.getStringField(
                                                   OM_HOST_FIELD_OM_VERSION ) ;
                  string installPath = postgres.getStringField(
                                                   OM_HOST_FIELD_OM_PATH ) ;

                  if ( !version.empty() )
                  {
                     packageInfo = BSON(
                           OM_HOST_FIELD_PACKAGENAME <<
                                          OM_BUSINESS_SEQUOIASQL_POSTGRESQL <<
                           OM_HOST_FIELD_INSTALLPATH << installPath <<
                           OM_HOST_FIELD_VERSION << version ) ;

                     packageBuilder.append( packageInfo ) ;
                  }
               }

               {
                  BSONObj packageInfo ;
                  BSONObj mysql = oneHost.getObjectField(
                                                   OM_HOST_FIELD_MYSQL ) ;
                  string version = mysql.getStringField(
                                                   OM_HOST_FIELD_OM_VERSION ) ;
                  string installPath = mysql.getStringField(
                                                   OM_HOST_FIELD_OM_PATH ) ;

                  if ( !version.empty() )
                  {
                     packageInfo = BSON(
                           OM_HOST_FIELD_PACKAGENAME <<
                                             OM_BUSINESS_SEQUOIASQL_MYSQL <<
                           OM_HOST_FIELD_INSTALLPATH << installPath <<
                           OM_HOST_FIELD_VERSION << version ) ;

                     packageBuilder.append( packageInfo ) ;
                  }
               }

               oneHostBuilder.appendElements( oneHost ) ;
               oneHostBuilder.append( OM_HOST_FIELD_PACKAGES,
                                      packageBuilder.arr() ) ;
               oneHost = oneHostBuilder.obj() ;
            }

            rc = rtnInsert( OM_CS_DEPLOY_CL_HOST, oneHost, 1, 0, cb ) ;
            if ( rc )
            {
               if ( SDB_IXM_DUP_KEY != rc )
               {
                  PD_LOG( PDERROR, "insert into table failed:%s,rc=%d",
                          OM_CS_DEPLOY_CL_HOST, rc ) ;
                  goto error ;
               }
            }
            clusterName = oneHost.getStringField( OM_BSON_CLUSTER_NAME ) ;
            sdbGetOMManager()->updateClusterVersion( clusterName ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddHostTask::getType()
   {
      return _taskType ;
   }

   INT64 omAddHostTask::getTaskID()
   {
      return _taskID ;
   }

   INT32 omAddHostTask::checkUpdateInfo( const BSONObj &updateInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement resultInfoEle ;
      BSONObj agentResultInfo ;

      BSONObj localTask ;
      BSONObj localResultInfo ;

      rc = omTaskBase::checkUpdateInfo( updateInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "omTaskBase::checkUpdateInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         // get local result info
         BSONObj filterResult = BSON( OM_TASKINFO_FIELD_RESULTINFO << "" ) ;
         BSONObj selector ;
         BSONObj matcher = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
         BSONObj orderBy ;
         BSONObj hint ;
         rc = queryOneTask( selector, matcher, orderBy, hint, localTask ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "query task failed:taskID="OSS_LL_PRINT_FORMAT
                    ",rc=%d", _taskID, rc ) ;
            goto error ;
         }

         localResultInfo = localTask.filterFieldsUndotted( filterResult,
                                                           true ) ;
      }

      resultInfoEle = updateInfo.getField( OM_TASKINFO_FIELD_RESULTINFO ) ;
      if ( Array != resultInfoEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s is not Array type",
                 OM_TASKINFO_FIELD_RESULTINFO ) ;
         goto error ;
      }
      agentResultInfo = resultInfoEle.embeddedObject() ;
      {
         BSONObj filter = BSON( OM_HOST_FIELD_NAME << "" ) ;
         BSONObjIterator iterResult( agentResultInfo ) ;
         while ( iterResult.more() )
         {
            BSONObj find ;
            BSONObj oneResult ;
            BSONElement ele = iterResult.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "%s's element is not Object type",
                       OM_TASKINFO_FIELD_RESULTINFO ) ;
               goto error ;
            }

            oneResult = ele.embeddedObject() ;
            find      = oneResult.filterFieldsUndotted( filter, true ) ;
            if ( !isInElement( localResultInfo, OM_TASKINFO_FIELD_RESULTINFO,
                               find ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "agent's result is not in localTask:"
                       "agentResult=%s", find.toString().c_str() ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omRemoveHostTask::omRemoveHostTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_REMOVE_HOST ;
   }

   omRemoveHostTask::~omRemoveHostTask()
   {

   }

   INT32 omRemoveHostTask::_getSuccessHost( BSONObj &resultInfo,
                                         set<string> &successHostSet )
   {
      INT32 rc = SDB_OK ;
      BSONObj hosts ;
      hosts = resultInfo.getObjectField( OM_TASKINFO_FIELD_RESULTINFO ) ;
      BSONObjIterator iter( hosts ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj tmpHost = ele.embeddedObject() ;
         string hostName = tmpHost.getStringField( OM_HOST_FIELD_NAME ) ;
         INT32 retCode   = tmpHost.getIntField( OM_REST_RES_RETCODE ) ;
         if ( SDB_OK == retCode )
         {
            successHostSet.insert( hostName ) ;
         }
      }

      return rc ;
   }

   INT32 omRemoveHostTask::finish( BSONObj &resultInfo )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      string clusterName ;
      BSONObj taskInfoValue ;
      BSONObj hosts ;
      set<string> successHostSet ;

      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj taskInfo ;

      rc = _getSuccessHost( resultInfo, successHostSet ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get success host failed:rc=%d", rc ) ;
         goto error ;
      }

      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;
      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      rc = queryOneTask( selector, matcher, orderBy, hint, taskInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      taskInfoValue = taskInfo.getObjectField( OM_TASKINFO_FIELD_INFO ) ;
      hosts = taskInfoValue.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;
      SDB_ASSERT( !hosts.isEmpty(), "" ) ;
      {
         string clusterName ;
         BSONObjIterator iter( hosts ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj oneHost = ele.embeddedObject() ;
            string hostName = oneHost.getStringField( OM_HOST_FIELD_NAME ) ;
            if ( successHostSet.find( hostName ) == successHostSet.end() )
            {
               // ignore the failure host
               continue ;
            }

            BSONObj condition = BSON( OM_HOST_FIELD_NAME << hostName ) ;
            rc = rtnDelete( OM_CS_DEPLOY_CL_HOST, condition, hint, 0, cb );
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to delete record from table:%s,"
                       "%s=%s,rc=%d", OM_CS_DEPLOY_CL_HOST,
                       OM_HOST_FIELD_NAME, hostName.c_str(), rc ) ;
               goto error ;
            }

            clusterName = oneHost.getStringField( OM_BSON_CLUSTER_NAME ) ;
            sdbGetOMManager()->updateClusterVersion( clusterName ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveHostTask::getType()
   {
      return _taskType ;
   }

   INT64 omRemoveHostTask::getTaskID()
   {
      return _taskID ;
   }

   INT32 omRemoveHostTask::checkUpdateInfo( const BSONObj &updateInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement resultInfoEle ;
      BSONObj agentResultInfo ;

      BSONObj localTask ;
      BSONObj localResultInfo ;

      rc = omTaskBase::checkUpdateInfo( updateInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "omTaskBase::checkUpdateInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         // get local result info
         BSONObj filterResult = BSON( OM_TASKINFO_FIELD_RESULTINFO << "" ) ;
         BSONObj selector ;
         BSONObj matcher = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
         BSONObj orderBy ;
         BSONObj hint ;
         rc = queryOneTask( selector, matcher, orderBy, hint, localTask ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "query task failed:taskID="OSS_LL_PRINT_FORMAT
                    ",rc=%d", _taskID, rc ) ;
            goto error ;
         }

         localResultInfo = localTask.filterFieldsUndotted( filterResult,
                                                           true ) ;
      }

      resultInfoEle = updateInfo.getField( OM_TASKINFO_FIELD_RESULTINFO ) ;
      if ( Array != resultInfoEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "%s is not Array type",
                 OM_TASKINFO_FIELD_RESULTINFO ) ;
         goto error ;
      }
      agentResultInfo = resultInfoEle.embeddedObject() ;
      {
         BSONObj filter = BSON( OM_HOST_FIELD_NAME << "" ) ;
         BSONObjIterator iterResult( agentResultInfo ) ;
         while ( iterResult.more() )
         {
            BSONObj find ;
            BSONObj oneResult ;
            BSONElement ele = iterResult.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "%s's element is not Object type",
                       OM_TASKINFO_FIELD_RESULTINFO ) ;
               goto error ;
            }

            oneResult = ele.embeddedObject() ;
            find      = oneResult.filterFieldsUndotted( filter, true ) ;
            if ( !isInElement( localResultInfo, OM_TASKINFO_FIELD_RESULTINFO,
                               find ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "agent's result is not in localTask:"
                       "agentResult=%s", find.toString().c_str() ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omAddBusinessTask::omAddBusinessTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_ADD_BUSINESS ;
   }

   omAddBusinessTask::~omAddBusinessTask()
   {

   }

   INT32 omAddBusinessTask::_storeBusinessInfo( BSONObj &taskInfoValue )
   {
      INT32 rc = SDB_OK ;
      string businessName ;
      string deployMod ;
      string businessType ;
      string clusterName ;
      BSONObj obj ;
      BSONObj configs ;
      BSONObjBuilder builder ;
      time_t now = time( NULL ) ;
      pmdEDUCB *cb  = pmdGetThreadEDUCB() ;

      businessName  = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME );
      deployMod     = taskInfoValue.getStringField( OM_BSON_DEPLOY_MOD ) ;
      businessType  = taskInfoValue.getStringField( OM_BSON_BUSINESS_TYPE );
      clusterName   = taskInfoValue.getStringField( OM_BSON_CLUSTER_NAME ) ;

      builder.append( OM_BUSINESS_FIELD_NAME, businessName ) ;
      builder.append( OM_BUSINESS_FIELD_TYPE, businessType ) ;
      builder.append( OM_BUSINESS_FIELD_DEPLOYMOD, deployMod ) ;
      builder.append( OM_BUSINESS_FIELD_CLUSTERNAME, clusterName ) ;
      builder.appendTimestamp( OM_BUSINESS_FIELD_TIME, now * 1000, 0 ) ;
      builder.append( OM_BUSINESS_FIELD_ADDTYPE, OM_BUSINESS_ADDTYPE_INSTALL ) ;

      obj = builder.obj() ;
      rc = rtnInsert( OM_CS_DEPLOY_CL_BUSINESS, obj, 1, 0, cb );
      if ( rc )
      {
         if ( SDB_IXM_DUP_KEY != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to store business into table:%s,rc=%d",
                        OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         rc = SDB_OK ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessTask::_storeConfigInfo( BSONObj &taskInfoValue )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( pmdGetThreadEDUCB() ) ;
      string businessName ;
      string businessType ;
      string clusterName ;
      string deployMode ;
      BSONObj configs ;

      businessName  = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME ) ;
      businessType  = taskInfoValue.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      clusterName   = taskInfoValue.getStringField( OM_BSON_CLUSTER_NAME ) ;
      deployMode    = taskInfoValue.getStringField( OM_BSON_DEPLOY_MOD ) ;
      configs       = taskInfoValue.getObjectField( OM_BSON_FIELD_CONFIG ) ;

      {
         BSONObjIterator iter( configs ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj oneNode = ele.embeddedObject() ;
            string hostName ;

            hostName = oneNode.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
            if ( dbTool.isConfigExist( businessName, hostName ) )
            {
               rc = dbTool.appendConfigure( businessName, hostName, oneNode ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "append configure failed:host=%s,"
                          "business=%s, node=%s, rc=%d",
                          hostName.c_str(), businessName.c_str(),
                          oneNode.toString().c_str(), rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = dbTool.insertConfigure( businessName, hostName,
                                            businessType, clusterName,
                                            deployMode, oneNode ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "insert configure failed:host=%s,"
                          "business=%s, node=%s, rc=%d",
                          hostName.c_str(), businessName.c_str(),
                          oneNode.toString().c_str(), rc ) ;
                  goto error ;
               }
            }

         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   #define OM_TASK_MANAGER_GRANT_TYPE_CREATE_USER "create new user"
   INT32 omAddBusinessTask::_storeBusinessAuth()
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( pmdGetThreadEDUCB() ) ;
      string businessName ;
      string businessType ;
      BSONObj configs ;
      BSONObj taskInfo ;

      rc = _getTaskInfo( _taskID, taskInfo ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "failed to get task info, rc=%d", rc ) ;
         goto error ;
      }

      configs = taskInfo.getObjectField( OM_BSON_FIELD_CONFIG ) ;
      businessName = taskInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      businessType = taskInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      if ( OM_BUSINESS_SEQUOIASQL_MYSQL == businessType )
      {
         BSONObjIterator iter( configs ) ;

         if ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj oneNode = ele.embeddedObject() ;
            string user = oneNode.getStringField(
                                                OM_TASKINFO_FIELD_AUTH_USER ) ;
            string passwd = oneNode.getStringField(
                                                OM_TASKINFO_FIELD_AUTH_PASSWD );

            if( !user.empty() )
            {
               rc = dbTool.upsertAuth( businessName, user, passwd ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "failed to add business auth:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessTask::_updateBizHostInfo( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      list <string> hostsList ;
      rc = sdbGetOMManager()->getBizHostInfo( businessName, hostsList ) ;
      PD_RC_CHECK( rc, PDERROR, "get business host info failed:rc=%d", rc ) ;

      rc = sdbGetOMManager()->appendBizHostInfo( businessName, hostsList ) ;
      PD_RC_CHECK( rc, PDERROR, "get business host info failed:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessTask::finish( BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string businessName ;
      BSONObj taskInfo ;

      rc = getTaskInfo( taskInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get task info, rc=%d", rc ) ;
         goto error ;
      }

      rc = _storeBusinessInfo( taskInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "store business info failed:rc=%d", rc ) ;

      rc = _storeConfigInfo( taskInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "store configure info failed:rc=%d", rc ) ;

      rc = _storeBusinessAuth() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "store business auth failed:rc=%d", rc ) ;
         goto error ;
      }

      businessName = taskInfo.getStringField( OM_BSON_BUSINESS_NAME );
      rc = _updateBizHostInfo( businessName ) ;
      PD_RC_CHECK( rc, PDERROR, "update business host info failed:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessTask::getType()
   {
      return _taskType ;
   }

   INT64 omAddBusinessTask::getTaskID()
   {
      return _taskID ;
   }

   INT32 omAddBusinessTask::checkUpdateInfo( const BSONObj &updateInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement resultInfoEle ;
      BSONObj agentResultInfo ;

      BSONObj localTask ;
      BSONObj localResultInfo ;

      rc = omTaskBase::checkUpdateInfo( updateInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "omTaskBase::checkUpdateInfo failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omAddBusinessTask::getTaskInfo( BSONObj &taskInfo )
   {
      INT32 rc = SDB_OK ;
      string businessType ;
      BSONObj tmpTaskInfo ;

      rc = _getTaskInfo( _taskID, tmpTaskInfo ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "failed to get task info, rc=%d", rc ) ;
         goto error ;
      }

      businessType = tmpTaskInfo.getStringField(
                                          OM_TASKINFO_FIELD_BUSINESS_TYPE ) ;

      if ( OM_BUSINESS_SEQUOIASQL_MYSQL == businessType )
      {
         BSONObj taskInfoFilter = BSON( OM_TASKINFO_FIELD_CONFIG << 1 ) ;
         BSONObj nodeFilter = BSON( OM_PUBLIC_FIELD_GRANT_TYPE    << 1 <<
                                    OM_TASKINFO_FIELD_AUTH_USER   << 1 <<
                                    OM_TASKINFO_FIELD_AUTH_PASSWD << 1 ) ;
         BSONObj taskInfoData ;
         BSONObj tmpTaskInfoConfig ;
         BSONObj taskInfoConfig ;
         BSONArrayBuilder nodesBuilder ;
         BSONObjBuilder taskInfoBuilder ;

         taskInfoData = tmpTaskInfo.filterFieldsUndotted( taskInfoFilter,
                                                          FALSE ) ;

         tmpTaskInfoConfig = tmpTaskInfo.filterFieldsUndotted( taskInfoFilter,
                                                               TRUE ) ;

         taskInfoConfig = tmpTaskInfoConfig.getObjectField(
                                                OM_TASKINFO_FIELD_CONFIG ) ;

         {
            BSONObjIterator iter( taskInfoConfig ) ;

            while ( iter.more() )
            {
               BSONObj nodeInfo ;
               BSONObj newNodeInfo ;
               BSONElement ele = iter.next() ;

               if ( ele.type() != Object )
               {
                  continue ;
               }

               nodeInfo = ele.embeddedObject() ;
               newNodeInfo = nodeInfo.filterFieldsUndotted( nodeFilter, FALSE );
               nodesBuilder.append( newNodeInfo ) ;
            }
         }

         taskInfoBuilder.appendElements( taskInfoData ) ;
         taskInfoBuilder.append( OM_TASKINFO_FIELD_CONFIG,
                                 nodesBuilder.arr() ) ;

         tmpTaskInfo = taskInfoBuilder.obj() ;
         taskInfo = tmpTaskInfo.copy() ;
      }
      else
      {
         taskInfo = tmpTaskInfo.copy() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omExtendBusinessTask::omExtendBusinessTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_EXTEND_BUSINESS;
   }

   omExtendBusinessTask::~omExtendBusinessTask()
   {
   }

   INT32 omExtendBusinessTask::finish( BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string businessName ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj taskInfo ;
      BSONObj taskInfoValue ;

      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;

      rc = queryOneTask( selector, matcher, orderBy, hint, taskInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="
                          OSS_LL_PRINT_FORMAT",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      taskInfoValue = taskInfo.getObjectField( OM_TASKINFO_FIELD_INFO ) ;

      rc = _storeConfigInfo( taskInfoValue ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "store configure info failed:rc=%d", rc ) ;
         goto error ;
      }

      businessName = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME );

      rc = _updateBizHostInfo( businessName ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "update business host info failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessTask::_updateBizHostInfo( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      list <string> hostsList ;

      rc = sdbGetOMManager()->getBizHostInfo( businessName, hostsList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get business host info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = sdbGetOMManager()->appendBizHostInfo( businessName, hostsList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get business host info failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 omExtendBusinessTask::_storeConfigInfo( const BSONObj &taskInfoValue )
   {
      INT32 rc = SDB_OK ;
      omDatabaseTool dbTool( pmdGetThreadEDUCB() ) ;
      string businessName ;
      string businessType ;
      string clusterName ;
      string deployMode ;
      BSONObj configs ;

      businessName = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME ) ;
      businessType = taskInfoValue.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      deployMode  = taskInfoValue.getStringField( OM_BSON_DEPLOY_MOD ) ;
      clusterName = taskInfoValue.getStringField( OM_BSON_CLUSTER_NAME ) ;
      configs     = taskInfoValue.getObjectField( OM_BSON_FIELD_CONFIG ) ;

      if ( OM_BUSINESS_SEQUOIADB == businessType )
      {
         deployMode = OM_DEPLOY_MOD_DISTRIBUTION ;
      }

      {
         BSONObjIterator iter( configs ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj oneNode = ele.embeddedObject() ;
            string hostName = oneNode.getStringField( OM_BSON_FIELD_HOST_NAME ) ;

            if ( dbTool.isConfigExist( businessName, hostName ) )
            {
               rc = dbTool.appendConfigure( businessName, hostName, oneNode ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "append configure failed:host=%s,"
                          "business=%s, node=%s, rc=%d",
                          hostName.c_str(), businessName.c_str(),
                          oneNode.toString().c_str(), rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = dbTool.insertConfigure( businessName, hostName,
                                            businessType, clusterName,
                                            deployMode, oneNode ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "insert configure failed:host=%s,"
                          "business=%s, node=%s, rc=%d",
                          hostName.c_str(), businessName.c_str(),
                          oneNode.toString().c_str(), rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omExtendBusinessTask::getType()
   {
      return _taskType ;
   }

   INT64 omExtendBusinessTask::getTaskID()
   {
      return _taskID ;
   }

   omShrinkBusinessTask::omShrinkBusinessTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_SHRINK_BUSINESS;
   }

   omShrinkBusinessTask::~omShrinkBusinessTask()
   {
   }

   INT32 omShrinkBusinessTask::_removeNodeConfig( const string &businessName,
                                                  const string &hostName,
                                                  const string &svcname )
   {
      INT32 rc = SDB_OK ;
      INT32 configNum  = 0 ;
      BOOLEAN isExist  = FALSE ;
      SINT64 contextID = -1 ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj sort ;
      BSONObj hint ;
      BSONObj configs ;
      BSONObj updator ;

      condition = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName <<
                        OM_CONFIGURE_FIELD_HOSTNAME << hostName ) ;

      // query table
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, selector, condition, sort,
                     hint, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore ( contextID, 1, buffObj, cb, pRtnCB ) ;
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
            BSONObj tmpConfig = result.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;

            configs  = tmpConfig.copy() ;
            isExist  = TRUE ;
         }
      }

      if ( FALSE == isExist )
      {
         goto done ;
      }

      {
         BSONObjBuilder updatorBuilder ;
         BSONArrayBuilder configsBuilder ;
         BSONObjIterator iter( configs ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj config = ele.embeddedObject() ;
            string tmpSvcname = config.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ;

            if ( tmpSvcname != svcname )
            {
               configsBuilder.append( config ) ;
               ++configNum ;
            }
         }

         updatorBuilder.append( OM_CONFIGURE_FIELD_CONFIG,
                                configsBuilder.arr() ) ;
         updator = BSON( "$set" << updatorBuilder.obj() ) ;
      }

      if ( 0 < configNum )
      {
         rc = rtnUpdate( OM_CS_DEPLOY_CL_CONFIGURE, condition, updator, hint,
                         FLG_UPDATE_UPSERT, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "falied to update host config,"
                             "condition=%s,updator=%s,rc=%d",
                    condition.toString().c_str(),
                    updator.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = rtnDelete( OM_CS_DEPLOY_CL_CONFIGURE, condition, hint, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to delete configure from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessTask::_removeConfig( const BSONObj &taskInfo,
                                              const BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string businessName = taskInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      string businessType = taskInfo.getStringField( OM_BSON_BUSINESS_TYPE ) ;

      if ( OM_BUSINESS_SEQUOIADB == businessType )
      {
         BSONObjIterator iter( resultInfo ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            BSONObj config  = ele.embeddedObject() ;
            INT32 errorNum  = config.getIntField( OM_TASKINFO_FIELD_ERRNO ) ;
            string hostName = config.getStringField(
                                                OM_CONFIGURE_FIELD_HOSTNAME ) ;
            string svcname  = config.getStringField(
                                                OM_CONFIGURE_FIELD_SVCNAME ) ;

            if ( 0 == errorNum )
            {
               rc = _removeNodeConfig( businessName, hostName, svcname ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "remove node config failed, rc=%d", rc ) ;
                  goto error ;
               }
            }
         }

      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessTask::_updateBizHostInfo( const string &businessName )
   {
      INT32 rc = SDB_OK ;
      list <string> hostsList ;

      rc = sdbGetOMManager()->getBizHostInfo( businessName, hostsList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get business host info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = sdbGetOMManager()->appendBizHostInfo( businessName, hostsList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get business host info failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessTask::finish( BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj task ;
      BSONObj taskInfo ;
      BSONObj taskResultInfo ;
      string businessName ;

      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;

      rc = queryOneTask( selector, matcher, orderBy, hint, task ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="
                          OSS_LL_PRINT_FORMAT",rc=%d",
                 _taskID, rc ) ;
         goto error ;
      }

      taskInfo = task.getObjectField( OM_TASKINFO_FIELD_INFO ) ;
      taskResultInfo = resultInfo.getObjectField(
                                                OM_TASKINFO_FIELD_RESULTINFO ) ;
      rc = _removeConfig( taskInfo, taskResultInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "remove node config failed:rc=%d", rc ) ;
         goto error ;
      }

      businessName = taskInfo.getStringField( OM_BSON_BUSINESS_NAME ) ;
      rc = _updateBizHostInfo( businessName ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "update business host info failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omShrinkBusinessTask::getType()
   {
      return _taskType ;
   }

   INT64 omShrinkBusinessTask::getTaskID()
   {
      return _taskID ;
   }

   omDeployPackageTask::omDeployPackageTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_DEPLOY_PACKAGE ;
   }

   omDeployPackageTask::~omDeployPackageTask()
   {
   }

   INT32 omDeployPackageTask::_addPackage( const BSONObj &taskInfo,
                                           const BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      string packageName ;
      string installPath ;
      string version ;
      set<string> hostList ;
      BSONObjIterator iter( resultInfo ) ;
      omDatabaseTool dbTool( pmdGetThreadEDUCB() ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONObj hostInfo  = ele.embeddedObject() ;
         INT32 errorNum  = hostInfo.getIntField( OM_TASKINFO_FIELD_ERRNO ) ;
         string hostName = hostInfo.getStringField(
                                                OM_TASKINFO_FIELD_HOSTNAME ) ;

         if ( SDB_OK == errorNum )
         {
            hostList.insert( hostName ) ;
            if ( 0 == version.length() )
            {
               version = hostInfo.getStringField( OM_TASKINFO_FIELD_VERSION ) ;
            }
         }
      }

      packageName = taskInfo.getStringField( OM_TASKINFO_FIELD_PACKAGENAME ) ;
      installPath = taskInfo.getStringField( OM_TASKINFO_FIELD_INSTALLPATH ) ;

      rc = dbTool.addPackageOfHosts( hostList, packageName, installPath,
                                     version ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add package of hosts: package=%s, rc=%d",
                 packageName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDeployPackageTask::finish( BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj task ;
      BSONObj taskInfo ;
      BSONObj taskResultInfo ;

      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;

      rc = queryOneTask( selector, matcher, orderBy, hint, task ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="
                          OSS_LL_PRINT_FORMAT",rc=%d",
                 _taskID, rc ) ;
         goto error ;
      }

      taskInfo = task.getObjectField( OM_TASKINFO_FIELD_INFO ) ;
      taskResultInfo = resultInfo.getObjectField(
                                                OM_TASKINFO_FIELD_RESULTINFO ) ;

      rc = _addPackage( taskInfo, taskResultInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add package: taskID="
                          OSS_LL_PRINT_FORMAT",rc=%d",
                 _taskID, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omDeployPackageTask::getType()
   {
      return _taskType ;
   }

   INT64 omDeployPackageTask::getTaskID()
   {
      return _taskID ;
   }

   /* restart business */
   omRestartBusinessTask::omRestartBusinessTask( INT64 taskID )
   {
      _taskID   = taskID ;
   }

   omRestartBusinessTask::~omRestartBusinessTask()
   {
   }

   INT32 omRestartBusinessTask::finish( BSONObj &resultInfo )
   {
      return SDB_OK ;
   }

   /* remove business */
   omRemoveBusinessTask::omRemoveBusinessTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_REMOVE_BUSINESS;
   }

   omRemoveBusinessTask::~omRemoveBusinessTask()
   {

   }

   INT32 omRemoveBusinessTask::_removeBusinessInfo( BSONObj &taskInfoValue )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      string businessName ;
      BSONObj condition ;
      BSONObj hint ;
      businessName = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME ) ;

      cb           = pmdGetThreadEDUCB() ;
      condition    = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS, condition, hint, 0, cb );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to delete business from table:%s,"
                     "business=%s,rc=%d", OM_CS_DEPLOY_CL_BUSINESS,
                     businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessTask::_removeAuthInfo( BSONObj &taskInfoValue )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      string businessName ;
      BSONObj condition ;
      BSONObj hint ;

      businessName = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME ) ;

      cb        = pmdGetThreadEDUCB() ;
      condition = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_BUSINESS_AUTH, condition, hint, 0, cb );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to delete business auth from table:%s,"
                     "business=%s,rc=%d", OM_CS_DEPLOY_CL_BUSINESS_AUTH,
                     businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessTask::_removeConfigInfo( BSONObj &taskInfoValue )
   {
      INT32 rc     = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      string businessName ;
      BSONObj condition ;
      BSONObj hint ;
      businessName = taskInfoValue.getStringField( OM_BSON_BUSINESS_NAME ) ;
      cb           = pmdGetThreadEDUCB() ;
      condition    = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_CONFIGURE, condition, hint, 0, cb );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to delete configure from table:%s,"
                     "business=%s,rc=%d", OM_CS_DEPLOY_CL_CONFIGURE,
                     businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessTask::finish( BSONObj &resultInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj taskInfo ;
      BSONObj taskInfoValue ;

      selector = BSON( OM_TASKINFO_FIELD_INFO << 1 ) ;
      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << _taskID ) ;
      rc = queryOneTask( selector, matcher, orderBy, hint, taskInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task info failed:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      taskInfoValue = taskInfo.getObjectField( OM_TASKINFO_FIELD_INFO ) ;

      rc = _removeBusinessInfo( taskInfoValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "delete business info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _removeConfigInfo( taskInfoValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "delete configure info failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _removeAuthInfo( taskInfoValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "delete auth info failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRemoveBusinessTask::getType()
   {
      return _taskType ;
   }

   INT64 omRemoveBusinessTask::getTaskID()
   {
      return _taskID ;
   }

   INT32 omRemoveBusinessTask::checkUpdateInfo( const BSONObj &updateInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement resultInfoEle ;
      BSONObj agentResultInfo ;

      BSONObj localTask ;
      BSONObj localResultInfo ;

      rc = omTaskBase::checkUpdateInfo( updateInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "omTaskBase::checkUpdateInfo failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   omSsqlExecTask::omSsqlExecTask( INT64 taskID )
   {
      _taskID   = taskID ;
      _taskType = OM_TASK_TYPE_SSQL_EXEC ;
   }

   omSsqlExecTask::~omSsqlExecTask()
   {

   }

   INT32 omSsqlExecTask::finish( BSONObj &resultInfo )
   {
      return SDB_OK ;
   }

   INT32 omSsqlExecTask::getType()
   {
      return _taskType ;
   }

   INT64 omSsqlExecTask::getTaskID()
   {
      return _taskID ;
   }

   INT32 omSsqlExecTask::checkUpdateInfo( const BSONObj &updateInfo )
   {
      INT32 rc = SDB_OK ;

      if ( !updateInfo.hasField( OM_TASKINFO_FIELD_STATUS ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "updateinfo miss field:fields=[%s],updateInfo"
                 "=%s", OM_TASKINFO_FIELD_STATUS,
                 updateInfo.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   omTaskManager::omTaskManager()
   {
   }

   omTaskManager::~omTaskManager()
   {
   }

   INT32 omTaskManager::_updateTask( omTaskBase *pTask, INT64 taskID,
                                     const BSONObj &taskUpdateInfo,
                                     BOOLEAN isFinish )
   {
      INT32 rc = SDB_OK ;
      utilUpdateResult upResult ;
      time_t now = time( NULL ) ;
      BSONObj selector = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      BSONObj updator ;
      BSONObj hint ;
      BSONObj taskInfo ;
      BSONObjBuilder builder ;

      builder.appendElements( taskUpdateInfo ) ;

      if ( isFinish )
      {
         pTask->getTaskInfo( taskInfo ) ;
         if ( !taskInfo.isEmpty() )
         {
            builder.append( OM_TASKINFO_FIELD_INFO, taskInfo ) ;
         }
      }

      builder.appendTimestamp( OM_TASKINFO_FIELD_END_TIME, now * 1000, 0 ) ;

      updator  = BSON( "$set" << builder.obj() ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_TASKINFO, selector, updator, hint, 0,
                      pmdGetThreadEDUCB(), &upResult ) ;
      if ( rc || 0 == upResult.updateNum() )
      {
         PD_LOG( PDERROR, "update task failed:table=%s,updateNum=%d,taskID="
                 OSS_LL_PRINT_FORMAT",updator=%s,selector=%s,rc=%d",
                 OM_CS_DEPLOY_CL_TASKINFO, upResult.updateNum(), taskID,
                 updator.toString().c_str(), selector.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskManager::_getTaskType( INT64 taskID, INT32 &taskType )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj task ;

      selector = BSON( OM_TASKINFO_FIELD_TYPE << 1 ) ;
      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = queryOneTask( selector, matcher, orderBy, hint, task ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task info faild:rc=%d", rc ) ;
         goto error ;
      }

      taskType = task.getIntField( OM_TASKINFO_FIELD_TYPE ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskManager::queryTasks( const BSONObj &selector,
                                    const BSONObj &matcher,
                                    const BSONObj &orderBy, const BSONObj &hint,
                                    vector< BSONObj >&tasks )
   {
      return engine::queryTasks( selector, matcher, orderBy, hint, tasks ) ;
   }

   INT32 omTaskManager::queryOneTask( const BSONObj &selector,
                                      const BSONObj &matcher,
                                      const BSONObj &orderBy,
                                      const BSONObj &hint, BSONObj &oneTask )
   {
      return engine::queryOneTask( selector, matcher, orderBy, hint, oneTask ) ;
   }

   INT32 omTaskManager::_getTaskFlag( INT64 taskID, BOOLEAN &existFlag,
                                      BOOLEAN &isFinished, INT32 &taskType )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      BSONObj task ;
      INT32 status ;
      existFlag  = FALSE ;
      isFinished = FALSE ;
      taskType   = OM_TASK_TYPE_END ;

      matcher  = BSON( OM_TASKINFO_FIELD_TASKID << taskID ) ;
      rc = queryOneTask( selector, matcher, orderBy, hint, task ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            existFlag = FALSE ;
            goto done ;
         }

         PD_LOG( PDERROR, "get task info faild:rc=%d", rc ) ;
         goto error ;
      }

      existFlag = TRUE ;
      status = task.getIntField( OM_TASKINFO_FIELD_STATUS ) ;
      if ( OM_TASK_STATUS_FINISH == status )
      {
         isFinished = TRUE ;
      }

      taskType = task.getIntField( OM_TASKINFO_FIELD_TYPE ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omTaskManager::updateTask( INT64 taskID,
                                    const BSONObj &taskUpdateInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 status ;
      INT32 errFlag ;
      BOOLEAN isExist   = FALSE ;
      BOOLEAN isFinish  = FALSE ;
      INT32 taskType    = OM_TASK_TYPE_END ;
      omTaskBase *pTask = NULL ;

      rc = _getTaskFlag( taskID, isExist, isFinish, taskType ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get task flag failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( !isExist || isFinish )
      {
         rc = SDB_OM_TASK_NOT_EXIST ;
         PD_LOG( PDERROR, "task status error:taskID="OSS_LL_PRINT_FORMAT
                 ",isExist[%d],isFinish[%d]",  taskID, isExist, isFinish ) ;
         goto error ;
      }

      switch ( taskType )
      {
      case OM_TASK_TYPE_ADD_HOST :
         pTask = SDB_OSS_NEW omAddHostTask( taskID ) ;
         break ;

      case OM_TASK_TYPE_REMOVE_HOST :
         pTask = SDB_OSS_NEW omRemoveHostTask( taskID ) ;
         break ;

      case OM_TASK_TYPE_ADD_BUSINESS :
         pTask = SDB_OSS_NEW omAddBusinessTask( taskID ) ;
         break ;

      case OM_TASK_TYPE_REMOVE_BUSINESS :
         pTask = SDB_OSS_NEW omRemoveBusinessTask( taskID ) ;
         break ;

      case OM_TASK_TYPE_SSQL_EXEC :
         pTask = SDB_OSS_NEW omSsqlExecTask( taskID ) ;
         break ;

      case OM_TASK_TYPE_EXTEND_BUSINESS:
         pTask = SDB_OSS_NEW omExtendBusinessTask( taskID ) ;
         break ;
      case OM_TASK_TYPE_SHRINK_BUSINESS:
         pTask = SDB_OSS_NEW omShrinkBusinessTask( taskID ) ;
         break ;
      case OM_TASK_TYPE_DEPLOY_PACKAGE:
         pTask = SDB_OSS_NEW omDeployPackageTask( taskID ) ;
         break ;
      case OM_TASK_TYPE_RESTART_BUSINESS:
         pTask = SDB_OSS_NEW omRestartBusinessTask( taskID ) ;
         break ;
      default :
         PD_LOG( PDERROR, "unknown task type:taskID="OSS_LL_PRINT_FORMAT
                 ",taskType=%d", taskID, taskType ) ;
         SDB_ASSERT( FALSE, "unknown task type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pTask->checkUpdateInfo( taskUpdateInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "check update info failed:updateInfo=%s,rc=%d",
                 taskUpdateInfo.toString().c_str(), rc ) ;
         goto error ;
      }

      status = taskUpdateInfo.getIntField( OM_TASKINFO_FIELD_STATUS ) ;
      if ( OM_TASK_STATUS_FINISH == status )
      {
         isFinish = TRUE ;
      }
      else
      {
         isFinish = FALSE ;
      }

      errFlag = taskUpdateInfo.getIntField( OM_TASKINFO_FIELD_ERRNO ) ;
      if ( SDB_OK == errFlag )
      {
         if ( isFinish )
         {
            BSONObj resultInfo ;
            BSONObj tmpFilter = BSON( OM_TASKINFO_FIELD_RESULTINFO << 1 ) ;
            resultInfo = taskUpdateInfo.filterFieldsUndotted( tmpFilter,
                                                              true ) ;
            rc = pTask->finish( resultInfo ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "finish task failed:taskID="OSS_LL_PRINT_FORMAT
                       ",rc=%d", pTask->getTaskID(), rc ) ;
               goto error ;
            }
         }
      }

      rc = _updateTask( pTask, taskID, taskUpdateInfo, isFinish ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "update task failed:taskID="OSS_LL_PRINT_FORMAT
                 ",rc=%d", taskID, rc ) ;
         goto error ;
      }

   done:
      if ( NULL != pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }
}

