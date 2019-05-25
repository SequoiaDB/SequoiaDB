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

   Source File Name = catSplit.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "catSplit.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"

using namespace bson ;

namespace engine
{

   static BOOLEAN _isGroupInCataSet( UINT32 groupID, clsCatalogSet &cataSet )
   {
      BOOLEAN findGroup = FALSE ;

      VEC_GROUP_ID vecGroups ;
      cataSet.getAllGroupID( vecGroups ) ;
      VEC_GROUP_ID::iterator itVec = vecGroups.begin() ;
      while ( itVec != vecGroups.end() )
      {
         if ( *itVec == groupID )
         {
            findGroup = TRUE ;
            break ;
         }
         ++itVec ;
      }

      return findGroup ;
   }

   static INT32 _checkRangeSplitKey( const BSONObj &splitKey,
                                     clsCatalogSet &cataSet,
                                     BOOLEAN allowEmpty )
   {
      INT32 rc = SDB_OK ;

      if ( splitKey.nFields() != cataSet.getShardingKey().nFields() )
      {
         PD_LOG( PDWARNING, "Split key fields not match sharding key fields" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _checkHashSplitKey( const BSONObj &splitKey,
                                    clsCatalogSet &cataSet,
                                    BOOLEAN allowEmpty )
   {
      INT32 rc = SDB_OK ;

      if ( 1 != splitKey.nFields() ||
           NumberInt != splitKey.firstElement().type() )
      {
         PD_LOG( PDWARNING, "Split key field not 1 or field type not Int, "
                 "split key: %s", splitKey.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _checkSplitKey( const BSONObj &splitKey,
                                clsCatalogSet &cataSet,
                                UINT32 groupID,
                                BOOLEAN allowEmpty,
                                BOOLEAN allowOnBound )
   {
      INT32 rc = SDB_OK ;

      if ( splitKey.isEmpty() && allowEmpty )
      {
         goto done ;
      }

      if ( cataSet.isRangeSharding() )
      {
         rc = _checkRangeSplitKey( splitKey, cataSet, allowEmpty ) ;
      }
      else
      {
         rc = _checkHashSplitKey( splitKey, cataSet, allowEmpty ) ;
      }

      if ( rc )
      {
         goto error ;
      }

      if ( !cataSet.isKeyInGroup( splitKey, groupID ) )
      {
         rc = SDB_CLS_BAD_SPLIT_KEY ;

         if ( allowOnBound &&
              cataSet.isKeyOnBoundary( splitKey, &groupID ) )
         {
            rc = SDB_OK ;
         }

         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      PD_LOG( PDWARNING, "Split key: %s, catalog info: %s, group id: %d",
              splitKey.toString().c_str(),
              cataSet.toCataInfoBson().toString().c_str(),
              groupID ) ;
      goto done ;
   }

   static INT32 _checkDstGroupInCSDomain( const CHAR * groupName,
                                          const CHAR * clFullName,
                                          BOOLEAN & existed,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR szSpace [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]  = {0} ;
      CHAR szCollection [ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      existed = FALSE ;

      BSONObj csObj ;
      BOOLEAN csExist = FALSE ;

      const CHAR *domainName = NULL ;

      rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Resolve collection name[%s] failed, rc: %d",
                   clFullName, rc ) ;

      rc = catCheckSpaceExist( szSpace, csExist, csObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Check collection space[%s] exist failed, "
                   "rc: %d", szSpace, rc ) ;
      PD_CHECK( csExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space[%s] is not exist", szSpace ) ;

      rc = rtnGetStringElement( csObj, CAT_DOMAIN_NAME, &domainName ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         existed = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK == rc )
      {
         BSONObj domainObj ;
         map<string, UINT32> groups ;
         rc = catGetDomainObj( domainName, domainObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get domain[%s] failed, rc: %d",
                      domainName, rc ) ;

         rc = catGetDomainGroups( domainObj,  groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain info[%s], "
                      "rc: %d", domainObj.toString().c_str(), rc ) ;

         if ( groups.find( groupName ) != groups.end() )
         {
            existed = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _checkSplitTaskConflict( rtnObjBuff *pBuffObj,
                                         BOOLEAN &conflict,
                                         clsSplitTask *pTask )
   {
      INT32 rc = 0 ;
      BSONObj taskObj ;

      while ( TRUE )
      {
         rc = pBuffObj->nextObj( taskObj ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
            }
            break ;
         }
         clsSplitTask tmpTask( CLS_INVALID_TASKID ) ;
         rc = tmpTask.init( taskObj.objdata() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Init split task failed, rc: %d, obj: "
                      "%s", rc, taskObj.toString().c_str() ) ;
         if ( pTask->taskID() == tmpTask.taskID() ||
              pTask->muteXOn( &tmpTask ) || tmpTask.muteXOn( pTask ) )
         {
            conflict = TRUE ;
            PD_LOG( PDWARNING, "Exist task[%s] conflict with current "
                    "task[%s]", tmpTask.toBson().toString().c_str(),
                    pTask->toBson().toString().c_str() ) ;
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _catSplitCheckConflict( BSONObj & match,
                                        clsSplitTask & splitTask,
                                        BOOLEAN & conflict, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      BSONObj dummyObj ;
      INT64 contextID = -1 ;

      rtnContextBuf buffObj ;

      rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, match, dummyObj,
                     dummyObj, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc = %d", rc ) ;

      while ( SDB_OK == rc )
      {
         rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               contextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         rc = _checkSplitTaskConflict( &buffObj, conflict, &splitTask ) ;
         PD_RC_CHECK( rc, PDERROR, "Check split task conflict error, rc: %d",
                      rc ) ;

         if ( conflict )
         {
            break ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;

   error:
      goto done ;
   }

   static INT32 _catGetSplitTask ( UINT64 taskID,
                                  clsSplitTask &splitTask,
                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj boTask ;

      rc = catGetTask( taskID, boTask, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] failed, rc: %d",
                   taskID, rc ) ;

      rc = splitTask.init( boTask.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Init task failed, rc: %d, obj: %s",
                   rc, boTask.toString().c_str() ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCHECKSPLITGRP, "_catCheckSplitGroups" )
   INT32 _catCheckSplitGroups ( const BSONObj &splitInfo, clsCatalogSet &cataSet,
                                catCtxLockMgr &lockMgr, pmdEDUCB *cb,
                                UINT32 &srcGroupID, string &srcName,
                                UINT32 &dstGroupID, string &dstName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN dstInCSDomain = FALSE ;
      vector<UINT32> groupList ;

      PD_TRACE_ENTRY ( SDB__CATCHECKSPLITGRP ) ;

      rc = rtnGetSTDStringElement( splitInfo, CAT_SOURCE_NAME, srcName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field [%s] from query [%s]",
                   CAT_SOURCE_NAME, splitInfo.toString().c_str() ) ;

      rc = catGroupNameValidate( srcName.c_str(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Source group name [%s] is not valid, rc: %d",
                   srcName.c_str(), rc ) ;

      rc = rtnGetSTDStringElement( splitInfo, CAT_TARGET_NAME, dstName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field [%s] from query [%s]",
                   CAT_TARGET_NAME, splitInfo.toString().c_str() ) ;

      rc = catGroupNameValidate( dstName.c_str(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Target group name [%s] is not valid, rc: %d",
                   dstName.c_str(), rc ) ;

      PD_CHECK( 0 != dstName.compare( srcName ),
                SDB_INVALIDARG, error, PDERROR,
                "Target group name can not the same with source group name" ) ;

      rc = catGroupName2ID( srcName.c_str(), srcGroupID, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert group name [%s] to id, rc: %d",
                   srcName.c_str(), rc ) ;

      PD_CHECK( _isGroupInCataSet( srcGroupID, cataSet ),
                SDB_CL_NOT_EXIST_ON_GROUP, error, PDWARNING,
                "The collection [%s] does not exist on source group [%s]",
                cataSet.name(), srcName.c_str() ) ;

      rc = catGroupName2ID( dstName.c_str(), dstGroupID, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert group name [%s] to id, rc: %d",
                   dstName.c_str(), rc ) ;

      rc = _checkDstGroupInCSDomain( dstName.c_str(), cataSet.name(),
                                     dstInCSDomain, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Check destination group in space's domain failed, rc: %d",
                   rc ) ;
      PD_CHECK( dstInCSDomain,
                SDB_CAT_GROUP_NOT_IN_DOMAIN, error, PDWARNING,
                "Split target group [%s] is not in collection space domain",
                dstName.c_str() ) ;

      PD_LOG( PDDEBUG,
              "Got target group [%s], source group [%s]",
              dstName.c_str(), srcName.c_str() ) ;

      groupList.push_back( srcGroupID ) ;
      groupList.push_back( dstGroupID ) ;

      rc = catCheckGroupsByID( groupList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check available of groups, rc: %d",
                   rc ) ;

      rc = catLockGroups( groupList, cb, lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d",
                   rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB__CATCHECKSPLITGRP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCHKLCKSPLITPTASK, "_catCheckAndLockForSplitTask" )
   INT32 _catCheckAndLockForSplitTask ( INT32 opCode, const string &clName,
                                        clsCatalogSet &cataSet,
                                        const BSONObj &splitInfo,
                                        pmdEDUCB *cb,
                                        UINT32 &srcGroupID, string &srcName,
                                        UINT32 &dstGroupID, string &dstName,
                                        BOOLEAN needLock )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCHKLCKSPLITPTASK ) ;

      string mainCLName ;
      BSONObj boCollection ;

      catCtxLockMgr lockMgr ;

      rc = catGetAndLockCollection( clName, boCollection, cb,
                                    needLock ? &lockMgr : NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute splitCL [%d]: "
                   "Failed to get the collection [%s]",
                   opCode, clName.c_str() ) ;

      rc = cataSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute splitCL [%d] on [%s]: "
                   "Failed to update catalog set, cata info: %s, rc: %d",
                   opCode, clName.c_str(),
                   boCollection.toString().c_str(), rc ) ;

      PD_CHECK( cataSet.isSharding(),
                SDB_COLLECTION_NOTSHARD, error, PDERROR,
                "Failed to execute splitCL [%d] on [%s]: "
                "Could not split non-sharding collection",
                opCode, clName.c_str() ) ;

      PD_CHECK( !cataSet.isMainCL(),
                SDB_MAIN_CL_OP_ERR, error, PDERROR,
                "Failed to split step [%d] on [%s]: "
                "Could not split main-collection",
                opCode, clName.c_str() ) ;

      mainCLName = cataSet.getMainCLName() ;
      if ( !mainCLName.empty() )
      {
         BSONObj dummy ;
         rc = catGetAndLockCollection( mainCLName, dummy, cb,
                                       needLock ? &lockMgr : NULL, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to split step [%d] on [%s]: "
                      "Failed to get the main collection [%s]",
                      opCode, clName.c_str(),
                      mainCLName.c_str() ) ;
      }

      if ( MSG_CAT_SPLIT_PREPARE_REQ == opCode ||
           MSG_CAT_SPLIT_READY_REQ == opCode )
      {
         rc = _catCheckSplitGroups( splitInfo, cataSet, lockMgr, cb,
                                    srcGroupID, srcName,
                                    dstGroupID, dstName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to split step [%d] on [%s]: "
                      "Failed to check split groups",
                      opCode, clName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__CATCHKLCKSPLITPTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITPREPARE, "catSplitPrepare" )
   INT32 catSplitPrepare( const BSONObj &splitInfo, pmdEDUCB *cb,
                          UINT32 &returnGroupID, INT32 &returnVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITPREPARE ) ;

      try
      {
         string clName ;
         string srcName, dstName ;
         UINT32 srcGroupID = CAT_INVALID_GROUPID ;
         UINT32 dstGroupID = CAT_INVALID_GROUPID ;
         BSONObj splitQuery ;
         BOOLEAN existQuery = TRUE ;
         FLOAT64 percent = 0.0 ;

         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         clsCatalogSet cataSet( clName.c_str() ) ;

         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_PREPARE_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         rc = rtnGetObjElement( splitInfo, CAT_SPLITQUERY_NAME, splitQuery ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            existQuery = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc , PDERROR,
                      "Failed to get field [%s] from query [%s], rc: %d",
                      CAT_SPLITQUERY_NAME, splitInfo.toString().c_str(), rc ) ;

         percent = splitInfo.getField( CAT_SPLITPERCENT_NAME ).numberDouble() ;

         PD_CHECK( existQuery || ( percent > 0.0 && percent <= 100.0 ),
                   SDB_INVALIDARG, error, PDERROR,
                   "Split percent value [%f] error", percent ) ;

         returnGroupID = srcGroupID ;
         returnVersion = cataSet.getVersion() ;

         PD_LOG( PDDEBUG,
                 "Finished split prepare step on collection [%s] with [%s]",
                 clName.c_str(), splitInfo.toString().c_str() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITPREPARE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITREADY, "catSplitReady" )
   INT32 catSplitReady ( const BSONObj &splitInfo, UINT64 taskID,
                         pmdEDUCB *cb, INT16 w,
                         UINT32 &returnGroupID, INT32 &returnVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITREADY ) ;

      PD_CHECK( taskID != CLS_INVALID_TASKID,
                SDB_INVALIDARG, error, PDERROR,
                "Invalid task ID for split task" ) ;

      try
      {
         std::string clName ;
         std::string srcName, dstName ;
         UINT32 srcGroupID = CAT_INVALID_GROUPID ;
         UINT32 dstGroupID = CAT_INVALID_GROUPID ;
         BSONObj bKey, eKey ;
         BOOLEAN usePercent = FALSE ;
         FLOAT64 percent = 0.0 ;

         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         clsCatalogSet cataSet( clName.c_str() ) ;

         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_READY_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         rc = rtnGetObjElement( splitInfo, CAT_SPLITVALUE_NAME, bKey ) ;
         if ( SDB_FIELD_NOT_EXIST == rc ||
              ( SDB_OK == rc && bKey.isEmpty() ) )
         {
            usePercent = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field the [%s] from split info [%s], rc: %d",
                      CAT_SPLITVALUE_NAME, splitInfo.toString().c_str(), rc ) ;

         percent = splitInfo.getField( CAT_SPLITPERCENT_NAME ).numberDouble() ;

         if ( !usePercent )
         {
            rc = rtnGetObjElement( splitInfo, CAT_SPLITENDVALUE_NAME, eKey ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s] from split info [%s], rc: %d",
                         CAT_SPLITENDVALUE_NAME, splitInfo.toString().c_str(), rc ) ;
         }
         else
         {
            PD_CHECK( percent > 0.0 && percent <= 100.0,
                      SDB_INVALIDARG, error, PDERROR,
                      "Split percent value [%f] error", percent ) ;
            PD_CHECK( cataSet.isHashSharding(),
                      SDB_SYS, error, PDERROR,
                      "Split by percent must be hash sharding" ) ;
         }

         if ( !usePercent )
         {
            rc = _checkSplitKey( bKey, cataSet, srcGroupID, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Check split key failed, rc: %d", rc ) ;

            rc = _checkSplitKey( eKey, cataSet, srcGroupID, TRUE, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Check split end key failed, rc: %d", rc ) ;
         }

         BSONObj match ;
         BSONObj taskObj ;
         BOOLEAN conflict = FALSE ;
         clsSplitTask splitTask( taskID ) ;
         if ( usePercent )
         {
            rc = splitTask.calcHashPartition( cataSet, srcGroupID, percent,
                                              bKey, eKey ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to calculate hash percent partition of "
                         "split keys, rc: %d",
                         rc ) ;
         }
         rc = splitTask.init( clName.c_str(),
                              (INT32)srcGroupID, srcName.c_str(),
                              (INT32)dstGroupID, dstName.c_str(),
                              bKey, eKey, percent, cataSet ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Initialize split task failed, rc: %d",
                      rc ) ;

         match = splitTask.toBson( CLS_SPLIT_MASK_CLNAME ) ;
         rc = _catSplitCheckConflict( match, splitTask, conflict, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Check task conflict failed, rc: %d",
                      rc ) ;
         PD_CHECK( FALSE == conflict,
                   SDB_CLS_MUTEX_TASK_EXIST, error, PDERROR,
                   "Exist task not compatible with the new task" ) ;

         taskObj = splitTask.toBson( CLS_MASK_ALL ) ;
         rc = catAddTask( taskObj, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Add split task failed, rc: %d", rc ) ;

         returnGroupID = dstGroupID ;
         returnVersion = cataSet.getVersion() ;

         PD_LOG( PDDEBUG,
                 "Split ready step: added split task [%llu]: %s",
                 taskID, taskObj.toString().c_str() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITREADY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITSTART, "catSplitStart" )
   INT32 catSplitStart ( UINT64 taskID, pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITSTART ) ;

      INT32 status = CLS_TASK_STATUS_READY ;

      PD_CHECK( taskID != CLS_INVALID_TASKID,
                SDB_INVALIDARG, error, PDERROR,
                "Invalid task ID for split task" ) ;

      rc = catGetTaskStatus( taskID, status, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] status failed, rc: %d",
                   taskID, rc ) ;

      if ( CLS_TASK_STATUS_READY == status ||
           CLS_TASK_STATUS_PAUSE == status )
      {
         rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_RUN, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] status [%d] -> [%d], rc: %d",
                      taskID, status, CLS_TASK_STATUS_RUN, rc ) ;
      }
      else if ( CLS_TASK_STATUS_CANCELED == status )
      {
         rc = SDB_TASK_HAS_CANCELED ;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "Finished split start step on task [%llu]",
              taskID ) ;
   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITSTART, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITCHGMETA, "catSplitChgMeta" )
   INT32 catSplitChgMeta ( const BSONObj &splitInfo, UINT64 taskID,
                           pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITCHGMETA ) ;

      try
      {
         std::string clName, mainCLName ;
         std::string srcName, dstName ;
         UINT32 srcGroupID = CAT_INVALID_GROUPID ;
         UINT32 dstGroupID = CAT_INVALID_GROUPID ;

         BSONObj taskObj ;
         BSONObj cataInfo ;
         clsSplitTask splitTask( CLS_INVALID_TASKID ) ;


         rc = _catGetSplitTask( taskID, splitTask, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get task [%llu] failed, rc: %d",
                      taskID, rc ) ;

         if ( CLS_TASK_STATUS_META == splitTask.status() ||
              CLS_TASK_STATUS_FINISH == splitTask.status() )
         {
            goto done ;
         }
         else if ( CLS_TASK_STATUS_CANCELED == splitTask.status() )
         {
            rc = SDB_TASK_HAS_CANCELED ;
            goto error ;
         }

         PD_CHECK( CLS_TASK_STATUS_RUN == splitTask.status(),
                   SDB_SYS, error, PDERROR,
                   "Split task status error, task: %s",
                   taskObj.toString().c_str() ) ;

         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         PD_CHECK( 0 == clName.compare( splitTask.clFullName() ),
                   SDB_SYS, error, PDERROR,
                   "Task [%llu] split on different collection [%s] from "
                   "previous phase [%s]",
                   taskID, splitTask.clFullName(), clName.c_str() ) ;

         clsCatalogSet cataSet( clName.c_str() ) ;

         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_CHGMETA_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         if ( !cataSet.isKeyInGroup( splitTask.splitKeyObj(),
                                     splitTask.dstID() ) )
         {
            rc = _checkSplitKey( splitTask.splitKeyObj(), cataSet,
                                 splitTask.sourceID(), FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDSEVERE,
                         "Check split key failed, rc: %d, there's "
                         "possible data corruption, obj: %s",
                         rc, taskObj.toString().c_str() ) ;

            rc = _checkSplitKey( splitTask.splitEndKeyObj(), cataSet,
                                 splitTask.sourceID(), TRUE, TRUE ) ;
            PD_RC_CHECK( rc, PDSEVERE,
                         "Check split end key failed, rc: %d, "
                         "there's possible data corruption, obj: %s",
                         rc, taskObj.toString().c_str() ) ;

            rc = cataSet.split( splitTask.splitKeyObj(),
                                splitTask.splitEndKeyObj(),
                                splitTask.dstID(), splitTask.dstName() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Catalog split failed, rc: %d, catalog: %s, task obj: %s",
                         rc ,
                         cataSet.toCataInfoBson().toString().c_str(),
                         taskObj.toString().c_str() ) ;

            cataInfo = cataSet.toCataInfoBson() ;
            rc = catUpdateCatalog( clName.c_str(), cataInfo, cb, w ) ;
            PD_RC_CHECK( rc, PDSEVERE,
                         "Failed to update collection catalog, rc: %d",
                         rc ) ;

            PD_LOG( PDDEBUG,
                    "Split task [%llu] chgmeta step: updated collection [%s]",
                    taskID, clName.c_str() ) ;

            mainCLName = cataSet.getMainCLName();
            if ( !mainCLName.empty() )
            {
               BSONObj emptyObj;
               INT32 tmpRC = SDB_OK ;
               tmpRC = catUpdateCatalog( mainCLName.c_str(), emptyObj, cb, w ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to update version of main-collection[%s], rc: %d",
                          mainCLName.c_str(), rc ) ;
               }

               PD_LOG( PDDEBUG,
                       "Split task [%llu] chgmeta step: updated main-collection [%s]",
                       taskID, mainCLName.c_str() ) ;
            }
         }

         rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_META, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task status, rc: %d",
                      rc ) ;

         PD_LOG( PDDEBUG,
                 "Finished split chgmeta step on task [%llu]",
                 taskID ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITCHGMETA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITCLEANUP, "catSplitCleanup" )
   INT32 catSplitCleanup ( UINT64 taskID, pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITCLEANUP ) ;

      INT32 status = CLS_TASK_STATUS_READY ;

      PD_CHECK( taskID != CLS_INVALID_TASKID,
                SDB_INVALIDARG, error, PDERROR,
                "Invalid task ID for split task" ) ;

      rc = catGetTaskStatus( taskID, status, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] status failed, rc: %d",
                   taskID, rc ) ;

      if ( CLS_TASK_STATUS_META == status )
      {
         rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_FINISH, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] status [%d] -> [%d], rc: %d",
                      taskID, status, CLS_TASK_STATUS_FINISH, rc ) ;
      }
      else if ( CLS_TASK_STATUS_FINISH == status )
      {
      }
      else if ( CLS_TASK_STATUS_CANCELED == status )
      {
         rc = SDB_TASK_HAS_CANCELED ;
         goto error ;
      }
      else
      {
         PD_LOG( PDERROR,
                 "Task[%llu] status error in clean up step [%d]",
                 taskID, status ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_LOG( PDDEBUG,
              "Finished split cleanup step on task [%llu]",
              taskID ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITCLEANUP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITFINISH, "catSplitFinish" )
   INT32 catSplitFinish ( UINT64 taskID, pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSPLITFINISH ) ;

      PD_CHECK( taskID != CLS_INVALID_TASKID,
                SDB_INVALIDARG, error, PDERROR,
                "Invalid task ID for split task" ) ;

      rc = catRemoveTask( taskID, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Remove task[%llu] failed, rc: %d",
                   taskID, rc ) ;

      PD_LOG( PDDEBUG,
              "Finished split finish step on task [%llu]",
              taskID ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITFINISH, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSPLITCANCEL, "catSplitCancel" )
   INT32 catSplitCancel ( const BSONObj & splitInfo, pmdEDUCB * cb,
                          UINT64 &taskID, INT16 w, UINT32 &returnGroupID )
   {
      INT32 rc = SDB_OK ;
      INT32 tmpGrpID = CAT_INVALID_GROUPID ;

      PD_TRACE_ENTRY ( SDB_CATSPLITCANCEL ) ;

      BSONElement ele = splitInfo.getField( CAT_TASKID_NAME ) ;


      if ( !ele.eoo() )
      {
         INT32 status = CLS_TASK_STATUS_READY ;
         BSONObj taskObj ;

         PD_CHECK( ele.isNumber(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field [%s] from query, type: %d",
                   CAT_TASKID_NAME, ele.type() ) ;
         taskID = ( UINT64 )ele.numberLong() ;

         PD_LOG( PDDEBUG,
                 "Split cancel: got task ID [%llu]",
                 taskID ) ;


         rc = catGetTask( taskID, taskObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task [%llu], rc: %d",
                      taskID, rc ) ;

         rc = rtnGetIntElement( taskObj, CAT_STATUS_NAME, status ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get the field [%s] from task [%llu], rc: %d",
                      CAT_STATUS_NAME, taskID, rc ) ;

         rc = rtnGetIntElement( taskObj, CAT_TARGETID_NAME, tmpGrpID ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get the field [%s] from task [%llu], rc: %d",
                      CAT_TARGETID_NAME, taskID, rc ) ;
         returnGroupID = (UINT32)tmpGrpID ;

         PD_LOG( PDDEBUG,
                 "Split cancel: got target group ID [%u]",
                 returnGroupID ) ;

         if ( CLS_TASK_STATUS_META == status ||
              CLS_TASK_STATUS_FINISH == status )
         {
            rc = SDB_TASK_ALREADY_FINISHED ;
            goto error ;
         }
         else if ( CLS_TASK_STATUS_READY == status )
         {
            rc = catRemoveTask( taskID, cb ,w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to remove task [%llu] failed, rc: %d",
                         taskID, rc ) ;
         }
         else if ( CLS_TASK_STATUS_CANCELED != status )
         {
            rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_CANCELED,
                                      cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update data task [%llu] to canceled, rc: %d",
                         taskID, rc ) ;
         }

         PD_LOG( PDDEBUG,
                 "Finished split cancel step on task [%llu]",
                 taskID ) ;
      }
      else
      {
         BSONObjBuilder matchBuilder ;
         matchBuilder.append( CAT_TASKTYPE_NAME, CLS_TASK_SPLIT ) ;
         matchBuilder.append( splitInfo.getField( CAT_COLLECTION_NAME ) ) ;
         matchBuilder.append( splitInfo.getField( CAT_SOURCE_NAME ) ) ;
         matchBuilder.append( splitInfo.getField( CAT_TARGET_NAME ) ) ;

         BSONElement splitKeyEle = splitInfo.getField( CAT_SPLITVALUE_NAME ) ;
         if ( splitKeyEle.eoo() ||
              splitKeyEle.embeddedObject().isEmpty() )
         {
            matchBuilder.append( splitInfo.getField( CAT_SPLITPERCENT_NAME ) ) ;
         }
         else
         {
            matchBuilder.append( splitKeyEle ) ;
         }

         BSONObj match = matchBuilder.obj() ;


         rc = catRemoveTask( match, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove task [%s], rc: %d",
                      splitInfo.toString().c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITCANCEL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}
