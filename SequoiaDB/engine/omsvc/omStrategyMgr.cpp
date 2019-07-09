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

   Source File Name = omStrategyMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omStrategyMgr.hpp"
#include "omDef.hpp"
#include "rtn.hpp"
#include "pd.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   static const BSONObj s_emptyObj ;

   /*
      _omStrategyMgr implement
   */
   _omStrategyMgr::_omStrategyMgr()
   {
      _pSdbAdapter = NULL ;
   }

   _omStrategyMgr::~_omStrategyMgr()
   {
      fini() ;
   }

   INT32 _omStrategyMgr::init( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      BSONObj indexDef ;

      _pSdbAdapter = SDB_OSS_NEW omSdbAdaptor() ;
      if ( !_pSdbAdapter )
      {
         PD_LOG( PDERROR, "Allocate SDB adaptor failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnTestAndCreateCL( OM_CS_STRATEGY_CL_META_DATA,
                               cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_META_DATA, rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_META_DATA_IDX1 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_META_DATA,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_META_DATA,
                 rc ) ;
         goto error ;
      }

      rc = rtnTestAndCreateCL( OM_CS_STRATEGY_CL_TASK_PRO,
                               cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_TASK_PRO, rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_TASK_PRO_IDX1 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_TASK_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_TASK_PRO,
                 rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_TASK_PRO_IDX2 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_TASK_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_TASK_PRO,
                 rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_TASK_PRO_IDX3 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_TASK_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_TASK_PRO,
                 rc ) ;
         goto error ;
      }

      rc = rtnTestAndCreateCL( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                               cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_STRATEGY_PRO_IDX1 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_STRATEGY_PRO,
                 rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_STRATEGY_PRO_IDX2 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_STRATEGY_PRO,
                 rc ) ;
         goto error ;
      }
      indexDef = OM_CS_STRATEGY_CL_STRATEGY_PRO_IDX3 ;
      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                                  indexDef,
                                  cb, dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(), OM_CS_STRATEGY_CL_STRATEGY_PRO,
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omStrategyMgr::fini()
   {
      if ( _pSdbAdapter )
      {
         SDB_OSS_DEL _pSdbAdapter ;
         _pSdbAdapter = NULL ;
      }
   }

   omSdbAdaptor* _omStrategyMgr::getSdbAdaptor()
   {
      return _pSdbAdapter ;
   }

   INT32 _omStrategyMgr::checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                                pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      if ( !strategyInfo.isValid() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDWARNING, "Strategy info is invalid" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getFieldMaxValue( const CHAR *pCollection,
                                           const CHAR *pFieldName,
                                           INT64 &value,
                                           INT64 defaultVal,
                                           pmdEDUCB *cb,
                                           const BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;
      BSONObj recordObj ;
      BSONObj orderBy = BSON( pFieldName << -1 ) ;
      BSONElement fieldTmp ;

      rc = getARecord( pCollection, s_emptyObj, matcher, orderBy,
                       cb, recordObj ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            value = defaultVal ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Get record from collection[%s] failed, rc: %d",
                 pCollection, rc ) ;
         goto error ;
      }

      fieldTmp = recordObj.getField( pFieldName ) ;
      if ( fieldTmp.eoo() )
      {
         value = defaultVal ;
         goto done ;
      }
      else if ( !fieldTmp.isNumber() )
      {
         PD_LOG( PDERROR, "System error: Field[%s] must be number",
                 fieldTmp.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      value = fieldTmp.numberLong() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getTaskID( const string &clsName,
                                    const string &bizName,
                                    const string &taskName,
                                    INT64 &taskID,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObj matcher ;
      BSONElement fieldTmp ;

      if ( clsName.empty() || bizName.empty() || taskName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName, BusinessName and TaskName "
                 "can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_TASK_NAME << taskName ) ;

      rc = getARecord( OM_CS_STRATEGY_CL_TASK_PRO,
                       s_emptyObj, matcher, s_emptyObj,
                       cb, obj ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_STRTGY_TASK_NOT_EXISTED ;
            PD_LOG( PDERROR, "Task[%s] is not exist",
                    matcher.toString().c_str() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Get task record failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      fieldTmp = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      if ( !fieldTmp.isNumber() )
      {
         PD_LOG( PDERROR, "System error: Field[%s] must be number",
                 fieldTmp.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      taskID = fieldTmp.numberLong() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getSortID( const string &clsName,
                                    const string &bizName,
                                    INT64 ruleID,
                                    INT64 &taskID,
                                    INT64 &sortID,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObj matcher ;
      BSONElement e ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName and BusinessName  "
                 "can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_RULE_ID << ruleID ) ;

      rc = getARecord( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                       s_emptyObj, matcher, s_emptyObj,
                       cb, obj ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_RULE_ID_IS_NOT_EXIST ;
            PD_LOG( PDWARNING, "Strategy[%s] is not exist",
                    matcher.toString().c_str() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Get strategy failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      e = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      taskID = e.numberLong() ;

      e = obj.getField( OM_REST_FIELD_SORT_ID ) ;
      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      sortID = e.numberLong() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getARecord( const CHAR *pCollection,
                                     const BSONObj &selector,
                                     const BSONObj &matcher,
                                     const BSONObj &orderBy,
                                     pmdEDUCB *cb,
                                     BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      SINT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      rc = rtnQuery( pCollection,
                     selector, matcher, orderBy, s_emptyObj,
                     0, cb, 0, 1,
                     dmsCB, rtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 pCollection, rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get more failed, rc: %d", rc ) ;
         }
         contextID = -1 ;
         goto error ;
      }
      rc = buffObj.nextObj( recordObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get record from buffer failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( contextID != -1 )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::queryTasks( const string &clsName,
                                     const string &bizName,
                                     SINT64 &contextID,
                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;

      static BSONObj orderBy = BSON( OM_REST_FIELD_TASK_NAME << 1 ) ;
      BSONObj matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                              OM_BSON_BUSINESS_NAME << bizName ) ;

      rc = rtnQuery( OM_CS_STRATEGY_CL_TASK_PRO,
                     s_emptyObj, matcher, orderBy, s_emptyObj,
                     0, cb, 0, -1,
                     dmsCB, rtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_TASK_PRO, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::queryStrategys( const string &clsName,
                                         const string &bizName,
                                         const string &taskName,
                                         INT64 &contextID,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;
      BSONObj matcher ;
      static BSONObj orderBy = BSON( OM_REST_FIELD_RULE_ID << 1 ) ;

      rc = getTaskID( clsName, bizName, taskName, taskID, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_TASK_ID << taskID ) ;
      rc = rtnQuery( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                     s_emptyObj, matcher, orderBy, s_emptyObj,
                     0, cb, 0, -1,
                     dmsCB, rtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::insertTask( omTaskInfo &taskInfo,
                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObj matcher ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;

      if ( !taskInfo.isValid() )
      {
         PD_LOG( PDWARNING, "Task info[%s] is invalid",
                 taskInfo.toBSON().toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      matcher = taskInfo.toMatcher() ;

      {
         ossScopedLock lock( &m_mutex ) ;

         rc = getARecord( OM_CS_STRATEGY_CL_TASK_PRO,
                          s_emptyObj, matcher, s_emptyObj,
                          cb, obj ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDWARNING, "Task[%s] is already exist",
                    matcher.toString().c_str() ) ;
            rc = SDB_STRTGY_TASK_NAME_CONFLICT ;
            goto error ;
         }
         else if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Check task exist failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = allocateTaskID( taskInfo.getClusterName(),
                              taskInfo.getBusinessName(),
                              cb, taskID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Allocate task id failed, rc: %d", rc ) ;
            goto error ;
         }
         taskInfo.setTaskID( taskID ) ;

         obj = taskInfo.toBSON() ;
         rc = rtnInsert( OM_CS_STRATEGY_CL_TASK_PRO,
                         obj, 1, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Insert task info to collection[%s] failed, "
                    "rc: %d", obj.toString().c_str(),
                    OM_CS_STRATEGY_CL_TASK_PRO,
                    rc ) ;
            goto error ;
         }

         rc = _updateMeta( taskInfo.getClusterName(),
                           taskInfo.getBusinessName(),
                           cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::insertStrategy( omTaskStrategyInfo &strategyInfo,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;
      INT64 ruleID = OM_TASK_STRATEGY_INVALID_RULE_ID ;
      INT64 sortID = OM_TASK_STRATEGY_INVALID_SORTID ;
      BSONObj obj ;
      BSONObj matcher ;
      static BSONObj updator = BSON( "$inc" << BSON(
                               OM_REST_FIELD_SORT_ID << 1 ) ) ;

      rc = checkTaskStrategyInfo( strategyInfo, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      {
         ossScopedLock lock( &m_mutex ) ;

         rc = getTaskID( strategyInfo.getClusterName(),
                         strategyInfo.getBusinessName(),
                         strategyInfo.getTaskName(),
                         taskID, cb ) ;
         if ( rc )
         {
            goto error ;
         }
         strategyInfo.setTaskID( taskID ) ;

         rc = allocateRuleID( strategyInfo.getClusterName(),
                              strategyInfo.getBusinessName(),
                              cb, ruleID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Allocate rule id failed, rc: %d", rc ) ;
            goto error ;
         }
         strategyInfo.setRuleID( ruleID ) ;

         matcher = strategyInfo.toMatcher() ;
         rc = getFieldMaxValue( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                                OM_REST_FIELD_SORT_ID,
                                sortID, 0, cb, matcher ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get max sort id failed, rc: %d", rc ) ;
            goto error ;
         }

         if ( strategyInfo.getSortID() <= 0 ||
              strategyInfo.getSortID() > sortID )
         {
            strategyInfo.setSortID( sortID + 1 ) ;
         }

         matcher = BSON( OM_BSON_CLUSTER_NAME <<
                              strategyInfo.getClusterName() <<
                         OM_BSON_BUSINESS_NAME <<
                              strategyInfo.getBusinessName() <<
                         OM_REST_FIELD_TASK_ID <<
                              strategyInfo.getTaskID() <<
                         OM_REST_FIELD_SORT_ID <<
                              BSON( "$gte" << strategyInfo.getSortID() ) ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         matcher, updator, s_emptyObj, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update collection[%s] failed, rc: %d",
                    OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
            goto error ;
         }

         obj = strategyInfo.toBSON() ;
         rc = rtnInsert( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         obj, 1, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Insert obj[%s] to collection[%s] failed, rc: %d",
                    obj.toString().c_str(), OM_CS_STRATEGY_CL_STRATEGY_PRO,
                    rc ) ;
            goto error ;
         }

         rc = _updateMeta( strategyInfo.getClusterName(),
                           strategyInfo.getBusinessName(),
                           cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::updateTaskStatus( const string &clsName,
                                           const string &bizName,
                                           const string &taskName,
                                           INT32 status,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;
      BSONObj matcher ;
      BSONObj updator ;

      if ( clsName.empty() || bizName.empty() || taskName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName, BusinessName and TaskName "
                 "can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( OM_STRATEGY_STATUS_DISABLE != status )
      {
         status = OM_STRATEGY_STATUS_ENABLE ;
      }

      updator = BSON( "$set" << BSON( OM_REST_FIELD_STATUS << status ) ) ;
      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_TASK_NAME << taskName ) ;

      {
         ossScopedLock lock( &m_mutex ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_TASK_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_TASK_PRO,
                    rc ) ;
            goto error ;
         }
         else if ( 0 == updatedNum )
         {
            rc = SDB_STRTGY_TASK_NOT_EXISTED ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delTask( const string &clsName,
                                  const string &bizName,
                                  const string &taskName,
                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;
      BSONObj matcher ;

      {
         ossScopedLock lock( &m_mutex ) ;

         rc = getTaskID( clsName, bizName, taskName, taskID, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                         OM_BSON_BUSINESS_NAME << bizName <<
                         OM_REST_FIELD_TASK_ID << taskID ) ;
         rc = rtnDelete( OM_CS_STRATEGY_CL_STRATEGY_PRO, matcher,
                         s_emptyObj, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Delete record[%s] from collection[%s] "
                    "failed, rc: %d", matcher.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
            goto error ;
         }

         matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                         OM_BSON_BUSINESS_NAME << bizName <<
                         OM_REST_FIELD_TASK_NAME << taskName ) ;
         rc = rtnDelete( OM_CS_STRATEGY_CL_TASK_PRO, matcher,
                         s_emptyObj, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Delete record[%s] from collection[%s] "
                    "failed, rc: %d", matcher.toString().c_str(),
                    OM_CS_STRATEGY_CL_TASK_PRO, rc ) ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::_updateMeta( const string &clsName,
                                      const string &bizName,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj updator ;

      matcher = BSON( FIELD_NAME_NAME << OM_STRATEGY_BS_TASK_META_NAME <<
                      OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName ) ;
      updator = BSON( "$inc" << BSON( FIELD_NAME_VERSION << 1 ) ) ;

      rc = rtnUpdate( OM_CS_STRATEGY_CL_META_DATA, matcher, updator,
                      s_emptyObj, FLG_UPDATE_UPSERT, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_META_DATA, rc ) ;
         goto error ;
      }

      _add2ChangeMap( clsName, bizName, 60 * OSS_ONE_SEC ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::updateStrategyNiceById( INT32 newNice,
                                                 INT64 ruleID,
                                                 const string &clsName,
                                                 const string &bizName,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;
      BSONObj updator ;
      BSONObj matcher ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName and BusinessName can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( newNice < OM_TASK_STRATEGY_NICE_MIN )
      {
         newNice = OM_TASK_STRATEGY_NICE_MIN ;
      }
      else if ( newNice > OM_TASK_STRATEGY_NICE_MAX )
      {
         newNice = OM_TASK_STRATEGY_NICE_MAX ;
      }

      updator = BSON( "$set" << BSON( OM_REST_FIELD_NICE << newNice ) ) ;
      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_RULE_ID << ruleID ) ;

      {
         ossScopedLock lock( &m_mutex ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO,
                    rc ) ;
            goto error ;
         }
         else if ( 0 == updatedNum )
         {
            PD_LOG( PDWARNING, "Strategy[%s] is not exist",
                    matcher.toString().c_str() ) ;
            rc = SDB_RULE_ID_IS_NOT_EXIST ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::addStrategyIpsById( const set< string > &ips,
                                             INT64 ruleID,
                                             const string &clsName,
                                             const string &bizName,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName and BusinessName can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !ips.empty() )
      {
         INT64 updatedNum = 0 ;
         BSONObjBuilder builder( 512 ) ;
         BSONArrayBuilder arr( builder.subarrayStart( OM_REST_FIELD_IPS ) ) ;

         set< string >::iterator it = ips.begin() ;
         while( it != ips.end() )
         {
            if ( !it->empty() )
            {
               arr.append( *it ) ;
            }
            ++it ;
         }
         arr.done() ;

         BSONObj updator = BSON( "$addtoset" << builder.obj() ) ;
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << ruleID <<
                                 OM_BSON_CLUSTER_NAME << clsName <<
                                 OM_BSON_BUSINESS_NAME << bizName ) ;

         {
            ossScopedLock lock( &m_mutex ) ;
            rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                            matcher, updator, s_emptyObj, 0, cb,
                            &updatedNum ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                       updator.toString().c_str(),
                       OM_CS_STRATEGY_CL_STRATEGY_PRO,
                       rc ) ;
               goto error ;
            }
            else if ( 0 == updatedNum )
            {
               PD_LOG( PDERROR, "Strategy[%s] is not exist",
                       matcher.toString().c_str() ) ;
               rc = SDB_RULE_ID_IS_NOT_EXIST ;
               goto error ;
            }

            rc = _updateMeta( clsName, bizName, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delStrategyIpsById( const set< string > &ips,
                                             INT64 ruleID,
                                             const string &clsName,
                                             const string &bizName,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName and BusinessName can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !ips.empty() )
      {
         INT64 updatedNum = 0 ;
         BSONObjBuilder builder( 512 ) ;
         BSONArrayBuilder arr( builder.subarrayStart( OM_REST_FIELD_IPS ) ) ;

         set< string >::iterator it = ips.begin() ;
         while( it != ips.end() )
         {
            arr.append( *it ) ;
            ++it ;
         }
         arr.done() ;

         BSONObj updator = BSON( "$pull_all" << builder.obj() ) ;
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << ruleID <<
                                 OM_BSON_CLUSTER_NAME << clsName <<
                                 OM_BSON_BUSINESS_NAME << bizName ) ;

         {
            ossScopedLock lock( &m_mutex ) ;
            rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                            matcher, updator, s_emptyObj, 0, cb,
                            &updatedNum ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                       updator.toString().c_str(),
                       OM_CS_STRATEGY_CL_STRATEGY_PRO,
                       rc ) ;
               goto error ;
            }
            else if ( 0 == updatedNum )
            {
               PD_LOG( PDERROR, "Strategy[%s] is not exist",
                       matcher.toString().c_str() ) ;
               rc = SDB_RULE_ID_IS_NOT_EXIST ;
               goto error ;
            }

            rc = _updateMeta( clsName, bizName, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delAllStrategyIpsById( INT64 ruleID,
                                                const string &clsName,
                                                const string &bizName,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName and BusinessName can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         INT64 updatedNum = 0 ;
         BSONObjBuilder builder ;
         BSONArrayBuilder arr( builder.subarrayStart( OM_REST_FIELD_IPS ) ) ;
         arr.done() ;

         BSONObj updator = BSON( "$set" << builder.obj() ) ;
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << ruleID <<
                                 OM_BSON_CLUSTER_NAME << clsName <<
                                 OM_BSON_BUSINESS_NAME << bizName ) ;

         {
            ossScopedLock lock( &m_mutex ) ;
            rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                            matcher, updator, s_emptyObj, 0, cb,
                            &updatedNum ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                       updator.toString().c_str(),
                       OM_CS_STRATEGY_CL_STRATEGY_PRO,
                       rc ) ;
               goto error ;
            }
            else if ( 0 == updatedNum )
            {
               PD_LOG( PDERROR, "Strategy[%s] is not exist",
                       matcher.toString().c_str() ) ;
               rc = SDB_RULE_ID_IS_NOT_EXIST ;
               goto error ;
            }

            rc = _updateMeta( clsName, bizName, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delStrategyById( INT64 ruleID,
                                          const string &clsName,
                                          const string &bizName,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;
      INT64 sortID = OM_TASK_STRATEGY_INVALID_SORTID ;
      BSONObj matcher ;
      INT64 delNum = 0 ;
      static BSONObj updator = BSON( "$inc" << BSON(
                               OM_REST_FIELD_SORT_ID << -1 ) ) ;

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_RULE_ID << ruleID ) ;

      {
         ossScopedLock lock( &m_mutex ) ;

         rc = getSortID( clsName, bizName, ruleID, taskID, sortID, cb ) ;
         if ( SDB_RULE_ID_IS_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( rc )
         {
            goto error ;
         }

         rc = rtnDelete( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         matcher, s_emptyObj, 0, cb, &delNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Delete[%s] on collection failed, rc: %d",
                    matcher.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO,
                    rc ) ;
            goto error ;
         }
         else if ( 0 == delNum )
         {
            goto done ;
         }

         matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                         OM_BSON_BUSINESS_NAME << bizName <<
                         OM_REST_FIELD_TASK_ID << taskID <<
                         OM_REST_FIELD_SORT_ID <<
                              BSON( "$gt" << sortID ) ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         matcher, updator, s_emptyObj, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update collection[%s] failed, rc: %d",
                    OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::updateStrategyStatusById( INT32 status,
                                                   INT64 ruleID,
                                                   const string &clsName,
                                                   const string &bizName,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;
      BSONObj matcher ;
      BSONObj updator ;

      if ( clsName.empty() || bizName.empty() )
      {
         PD_LOG( PDWARNING, "ClusterName, BusinessName "
                 "can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( OM_STRATEGY_STATUS_DISABLE != status )
      {
         status = OM_STRATEGY_STATUS_ENABLE ;
      }

      updator = BSON( "$set" << BSON( OM_REST_FIELD_STATUS << status ) ) ;
      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_RULE_ID << ruleID ) ;

      {
         ossScopedLock lock( &m_mutex ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO,
                    rc ) ;
            goto error ;
         }
         else if ( 0 == updatedNum )
         {
            rc = SDB_RULE_ID_IS_NOT_EXIST ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::updateStrategySortIDById( INT64 sortID,
                                                   INT64 ruleID,
                                                   const string &clsName,
                                                   const string &bizName,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = OM_TASK_STRATEGY_INVALID_TASK_ID ;
      INT64 oldSortID = OM_TASK_STRATEGY_INVALID_SORTID ;
      INT64 maxSortID = 0 ;
      BSONObj matcher ;
      BSONObj maxSortMatcher ;
      BSONObj updateMatcher ;
      BSONObj updator ;

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName <<
                      OM_REST_FIELD_RULE_ID << ruleID ) ;

      {
         ossScopedLock lock( &m_mutex ) ;

         rc = getSortID( clsName, bizName, ruleID, taskID, oldSortID, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         maxSortMatcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                                OM_BSON_BUSINESS_NAME << bizName <<
                                OM_REST_FIELD_TASK_ID << taskID ) ;
         rc = getFieldMaxValue( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                                OM_REST_FIELD_SORT_ID,
                                maxSortID, 1, cb, maxSortMatcher ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( sortID < 1 )
         {
            sortID = 1 ;
         }
         else if ( sortID > maxSortID )
         {
            sortID = maxSortID ;
         }

         if ( sortID == oldSortID )
         {
            goto done ;
         }
         else if ( sortID > oldSortID )
         {
            updator = BSON( "$inc" << BSON( OM_REST_FIELD_SORT_ID << -1 ) ) ;
            updateMatcher = BSON( "$and" << BSON_ARRAY(
                                       BSON( OM_REST_FIELD_SORT_ID <<
                                             BSON( "$gt" << oldSortID ) ) <<
                                       BSON( OM_REST_FIELD_SORT_ID <<
                                             BSON( "$lte" << sortID ) ) <<
                                       BSON( OM_REST_FIELD_TASK_ID << taskID <<
                                             OM_BSON_CLUSTER_NAME << clsName <<
                                             OM_BSON_BUSINESS_NAME << bizName )
                                    )
                                  ) ;
         }
         else
         {
            updator = BSON( "$inc" << BSON( OM_REST_FIELD_SORT_ID << 1 ) ) ;
            updateMatcher = BSON( "$and" << BSON_ARRAY(
                                       BSON( OM_REST_FIELD_SORT_ID <<
                                             BSON( "$gte" << sortID ) ) <<
                                       BSON( OM_REST_FIELD_SORT_ID <<
                                             BSON( "$lt" << oldSortID ) ) <<
                                       BSON( OM_REST_FIELD_TASK_ID << taskID <<
                                             OM_BSON_CLUSTER_NAME << clsName <<
                                             OM_BSON_BUSINESS_NAME << bizName )
                                    )
                                  ) ;
         }

         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                         updateMatcher, updator, s_emptyObj,
                         0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
            goto error ;
         }

         updator = BSON( "$set" << BSON( OM_REST_FIELD_SORT_ID << sortID ) ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_STRATEGY_PRO, matcher,
                         updator, s_emptyObj, 0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_STRATEGY_PRO, rc ) ;
            goto error ;
         }

         rc = _updateMeta( clsName, bizName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update meta failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::allocateTaskID( const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb,
                                         INT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj obj ;
      INT64 maxTaskID = 0 ;
      INT64 i = 0 ;

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName ) ;

      rc = getFieldMaxValue( OM_CS_STRATEGY_CL_TASK_PRO,
                             OM_REST_FIELD_TASK_ID,
                             maxTaskID,
                             OM_TASK_STRATEGY_INVALID_TASK_ID,
                             cb, matcher ) ;
      if ( rc )
      {
         goto error ;
      }

      i = maxTaskID ;
      while( ++i != maxTaskID )
      {
         matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                         OM_BSON_BUSINESS_NAME << bizName <<
                         OM_REST_FIELD_TASK_ID << i ) ;

         rc = getARecord( OM_CS_STRATEGY_CL_TASK_PRO,
                          s_emptyObj, matcher, s_emptyObj,
                          cb, obj ) ;
         if ( SDB_OK == rc )
         {
            continue ;
         }
         rc = SDB_OK ;
         break ;
      }
      taskID = i ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::allocateRuleID( const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb,
                                         INT64 &ruleID )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj obj ;
      INT64 maxRuleID = 0 ;
      INT64 i = 0 ;

      matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName ) ;

      rc = getFieldMaxValue( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                             OM_REST_FIELD_RULE_ID,
                             maxRuleID,
                             OM_TASK_STRATEGY_INVALID_RULE_ID,
                             cb, matcher ) ;
      if ( rc )
      {
         goto error ;
      }

      i = maxRuleID ;
      while( ++i != maxRuleID )
      {
         matcher = BSON( OM_BSON_CLUSTER_NAME << clsName <<
                         OM_BSON_BUSINESS_NAME << bizName <<
                         OM_REST_FIELD_RULE_ID << i ) ;

         rc = getARecord( OM_CS_STRATEGY_CL_STRATEGY_PRO,
                          s_emptyObj, matcher, s_emptyObj,
                          cb, obj ) ;
         if ( SDB_OK == rc )
         {
            continue ;
         }
         rc = SDB_OK ;
         break ;
      }
      ruleID = i ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getMetaRecord( const string &clsName,
                                        const string &bizName,
                                        BSONObj &obj,
                                        pmdEDUCB *cb )
   {
      BSONObj matcher ;

      matcher = BSON( FIELD_NAME_NAME << OM_STRATEGY_BS_TASK_META_NAME <<
                      OM_BSON_CLUSTER_NAME << clsName <<
                      OM_BSON_BUSINESS_NAME << bizName ) ;

      return getARecord( OM_CS_STRATEGY_CL_META_DATA, s_emptyObj,
                         matcher, s_emptyObj, cb, obj ) ;
   }

   INT32 _omStrategyMgr::getVersion( const string &clsName,
                                     const string &bizName,
                                     INT32 &version,
                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement e ;

      rc = getMetaRecord( clsName, bizName, obj, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_VERSION ) ;
      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      version = e.numberInt() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omStrategyMgr::flushStrategy( const string &clsName,
                                       const string & bizName,
                                       pmdEDUCB *cb )
   {
      ossScopedLock lock( &m_mutex ) ;
      _add2ChangeMap( clsName, bizName, 0 ) ;
   }

   UINT32 _omStrategyMgr::popTimeoutBusiness( INT64 interval,
                                              set<omStrategyChangeKey> &setBiz )
   {
      MAP_BIZ_TIME::iterator it ;

      ossScopedLock lock( &m_mutex ) ;

      it = _mapBizTimeoutInfo.begin() ;
      while( it != _mapBizTimeoutInfo.end() )
      {
         const omStrategyChangeKey &key = it->first ;

         if ( it->second > interval )
         {
            it->second -= interval ;
            ++it ;
         }
         else
         {
            setBiz.insert( key ) ;
            _mapBizTimeoutInfo.erase( it++ ) ;
         }
      }

      return setBiz.size() ;
   }

   void _omStrategyMgr::_add2ChangeMap( const string &clsName,
                                        const string &bizName,
                                        INT64 timeout )
   {
      omStrategyChangeKey findKey( clsName, bizName ) ;

      MAP_BIZ_TIME::iterator it = _mapBizTimeoutInfo.find( findKey ) ;
      if ( it == _mapBizTimeoutInfo.end() )
      {
         _mapBizTimeoutInfo[ findKey ] = timeout ;
      }
      else
      {
         it->second = timeout ;
      }
   }

   /*
      Global Functions
   */
   omStrategyMgr* omGetStrategyMgr()
   {
      static omStrategyMgr s_strategyMgr ;
      return &s_strategyMgr ;
   }

}

