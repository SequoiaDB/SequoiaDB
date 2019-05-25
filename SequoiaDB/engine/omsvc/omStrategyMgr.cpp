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
#include "../util/fromjson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   static const BSONObj s_emptyObj ;

   /*
      _omStrategyMgr implement
   */
   _omStrategyMgr::_omStrategyMgr()
   : m_curTaskID( 1 ), m_curRuleID( 1 )
   {
      m_pKrCB = NULL ;
      m_pDmsCB = NULL ;
      m_pRtnCB = NULL ;
   }

   _omStrategyMgr::~_omStrategyMgr()
   {
   }

   _omStrategyMgr::_omStrategyMgr( const _omStrategyMgr &others )
   {
      m_curTaskID = others.m_curTaskID ;
      m_curRuleID = others.m_curRuleID ;
      m_pKrCB = others.m_pKrCB ;
      m_pDmsCB = others.m_pDmsCB ;
      m_pRtnCB = others.m_pRtnCB ;
   }

   INT32 _omStrategyMgr::init( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      INT64 valTmp ;

      m_pKrCB  = pmdGetKRCB() ;
      m_pDmsCB = m_pKrCB->getDMSCB() ;
      m_pRtnCB = m_pKrCB->getRTNCB() ;

      rc = rtnTestAndCreateCL( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                               cb, m_pDmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create collection[%s], rc: %d",
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, rc ) ;
         goto error ;
      }

      rc = fromjson ( OM_STRATEGY_BUSINESSTASKPROIDX1, indexDef ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse index define[%s] failed, rc: %d",
                 OM_STRATEGY_BUSINESSTASKPROIDX1, rc ) ;
         goto error ;
      }

      rc = rtnTestAndCreateIndex( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                                  indexDef, cb, m_pDmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create index[%s] on collection[%s] failed, rc: %d",
                 indexDef.toString().c_str(),
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 rc ) ;
         goto error ;
      }

      rc = getFieldMaxValue( OM_REST_FIELD_TASK_ID, valTmp, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get max task ID failed, rc: %d", rc ) ;
         goto error ;
      }
      m_curTaskID = valTmp + 1 ;

      rc = getFieldMaxValue( OM_REST_FIELD_RULE_ID, valTmp, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get max rule ID failed, rc: %d", rc ) ;
         goto error ;
      }
      m_curRuleID = valTmp + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                                pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;

      if ( strategyInfo.getTaskName().empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Task name can't be empty" ) ;
         goto error ;
      }

      rc = getTaskID( strategyInfo.getTaskName(), taskID, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get task ID by name[%s] failed, rc: %d",
                 strategyInfo.getTaskName().c_str(), rc ) ;
         goto error ;
      }
      strategyInfo.setTaskID( taskID ) ;

      if ( strategyInfo.getID() >= m_curRuleID ||
           strategyInfo.getID() <= 0 )
      {
         strategyInfo.setID( incRuleID() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::incRecordRuleID( INT64 beginID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;

      SDB_ASSERT( beginID < m_curRuleID, "RuleID error!" ) ;

      if ( m_curRuleID - 1 == beginID )
      {
         goto done ;
      }
      else
      {
         BSONObj updator = BSON( "$inc" <<
                                 BSON( OM_REST_FIELD_RULE_ID << 1 ) ) ;
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID <<
                                 BSON( "$gte" << beginID ) ) ;

         rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                    rc ) ;
            goto error ;
         }
         PD_LOG( PDINFO, "Updated %lld records on collection[%s] by "
                 "condition[%s]", updatedNum,
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 matcher.toString().c_str() ) ;

         incRuleID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::decRecordRuleID( INT64 beginID, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;

      SDB_ASSERT( beginID < m_curRuleID, "RuleID error!" ) ;

      if ( m_curRuleID - 1 == beginID )
      {
         goto done ;
      }
      else
      {
         BSONObj updator = BSON( "$inc" <<
                                 BSON( OM_REST_FIELD_RULE_ID << -1 ) ) ;
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID <<
                                 BSON( "$gt" << beginID ) ) ;

         rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                    rc ) ;
            goto error ;
         }
         PD_LOG( PDINFO, "Updated %lld records on collection[%s] by "
                 "condition[%s]", updatedNum,
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 matcher.toString().c_str() ) ;

         decRuleID() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::getFieldMaxValue( const CHAR *pFieldName,
                                           INT64 &value,
                                           INT64 defaultVal,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj recordObj ;
      BSONObj orderBy = BSON( pFieldName << -1 ) ;
      BSONElement fieldTmp ;

      rc = getATaskStrategyRecord( recordObj, s_emptyObj,
                                   s_emptyObj, orderBy, cb ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            value = defaultVal ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Get task strategy record failed, rc: %d", rc ) ;
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

   INT32 _omStrategyMgr::getTaskID( const string &taskName,
                                    INT64 &taskID,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj recordObj ;
      BSONObj matcher = BSON( OM_REST_FIELD_TASK_NAME << taskName ) ;
      BSONElement fieldTmp ;

      rc = getATaskStrategyRecord( recordObj, s_emptyObj,
                                   matcher, s_emptyObj, cb ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            taskID = m_curTaskID++ ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Get task strategy record failed, rc: %d", rc ) ;
         goto error ;
      }

      fieldTmp = recordObj.getField( OM_REST_FIELD_TASK_ID ) ;
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

   INT32 _omStrategyMgr::getATaskStrategyRecord( BSONObj &recordObj,
                                                 const BSONObj &selector,
                                                 const BSONObj &matcher,
                                                 const BSONObj &orderBy,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      rc = rtnQuery( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                     selector, matcher, orderBy, s_emptyObj,
                     FLG_QUERY_WITH_RETURNDATA, cb, 0, 1,
                     m_pDmsCB, m_pRtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, 1, buffObj, cb, m_pRtnCB ) ;
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
         m_pRtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::queryTasks( SINT64 &contextID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      static BSONObj orderBy = BSON( OM_REST_FIELD_RULE_ID << 1 ) ;
      rc = rtnQuery( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                     s_emptyObj, s_emptyObj, orderBy, s_emptyObj,
                     FLG_QUERY_WITH_RETURNDATA, cb, 0, -1,
                     m_pDmsCB, m_pRtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 _omStrategyMgr::incRuleID()
   {
      return m_curRuleID++ ;
   }

   INT64 _omStrategyMgr::decRuleID()
   {
      return m_curRuleID-- ;
   }

   INT32 _omStrategyMgr::insertTask( omTaskStrategyInfo &strategyInfo,
                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BOOLEAN hasIncRuleID = FALSE ;

      ossScopedLock lock( &m_mutex ) ;

      rc = checkTaskStrategyInfo( strategyInfo, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Check strategy info failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = incRecordRuleID( strategyInfo.getID(), cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Increase rule ID failed, rc: %d", rc ) ;
         goto error ;
      }
      hasIncRuleID = TRUE ;

      obj = strategyInfo.toBSON() ;
      rc = rtnInsert( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      obj, 1, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Insert strategy info to collection[%s] failed, "
                 "rc: %d", obj.toString().c_str(),
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( hasIncRuleID )
      {
         decRecordRuleID( strategyInfo.getID(), cb ) ;
      }
      goto done ;
   }

   INT32 _omStrategyMgr::updateTaskNiceById( INT32 newNice,
                                             INT64 id,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;

      if ( newNice < OM_TASK_STRATEGY_NICE_MIN )
      {
         newNice = OM_TASK_STRATEGY_NICE_MIN ;
      }
      else if ( newNice > OM_TASK_STRATEGY_NICE_MAX )
      {
         newNice = OM_TASK_STRATEGY_NICE_MAX ;
      }

      BSONObj updator = BSON( "$set" <<
                              BSON( OM_REST_FIELD_NICE << newNice ) ) ;
      BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << id ) ;

      ossScopedLock lock( &m_mutex ) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, updator, s_emptyObj, 0, cb,
                      &updatedNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update[%s] on collection[%s] failed, rc: %d",
                 updator.toString().c_str(),
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 rc ) ;
         goto error ;
      }
      PD_LOG( PDINFO, "Update %lld records on collection[%s] by "
              "condition[%s]", updatedNum,
              OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
              matcher.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::addTaskIpsById( const set< string > &ips,
                                         INT64 id,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

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
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << id ) ;

         ossScopedLock lock( &m_mutex ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                    rc ) ;
            goto error ;
         }
         PD_LOG( PDINFO, "Update %lld records on collection[%s] by "
                 "condition[%s]", updatedNum,
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 matcher.toString().c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delTaskIpsById( const set< string > &ips,
                                         INT64 id,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

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
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << id ) ;

         ossScopedLock lock( &m_mutex ) ;
         rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                         matcher, updator, s_emptyObj, 0, cb,
                         &updatedNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                    updator.toString().c_str(),
                    OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                    rc ) ;
            goto error ;
         }
         PD_LOG( PDINFO, "Update %lld records on collection[%s] by "
                 "condition[%s]", updatedNum,
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 matcher.toString().c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delAllTaskIpsById( INT64 id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 updatedNum = 0 ;

      BSONObjBuilder builder ;
      BSONArrayBuilder arr( builder.subarrayStart( OM_REST_FIELD_IPS ) ) ;
      arr.done() ;

      BSONObj updator = BSON( "$set" << builder.obj() ) ;
      BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << id ) ;

      ossScopedLock lock( &m_mutex ) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, updator, s_emptyObj, 0, cb,
                      &updatedNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update[%s] on collection failed, rc: %d",
                 updator.toString().c_str(),
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 rc ) ;
         goto error ;
      }
      PD_LOG( PDINFO, "Update %lld records on collection[%s] by "
              "condition[%s]", updatedNum,
              OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
              matcher.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omStrategyMgr::delTaskById( INT64 id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 delNum = 0 ;

      ossScopedLock lock( &m_mutex ) ;

      if ( id >= m_curRuleID || id <= 0 )
      {
         goto done ;
      }
      else
      {
         BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << id ) ;

         rc = rtnDelete( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, matcher,
                         s_emptyObj, 0, cb, &delNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Delete[%s] on collection[%s] failed, rc: %d",
                    matcher.toString().c_str(),
                    OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO ) ;
            goto error ;
         }
         PD_LOG( PDINFO, "Delete %lld records on collection[%s] by "
                 "condition[%s]", delNum,
                 OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                 matcher.toString().c_str() ) ;

         rc = decRecordRuleID( id, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Decrease rule ID failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
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

