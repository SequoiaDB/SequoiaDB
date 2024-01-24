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

   Source File Name = catTask.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "catTask.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"
#include "catCMDBase.hpp"

using namespace bson ;

namespace engine
{

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

      map< string, UINT32 > groups ;

      existed = FALSE ;

      rc = catGetSplitCandidateGroups( clFullName, groups, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get split candidate groups, "
                   "rc: %d", rc ) ;

      if ( groups.find( groupName ) != groups.end() )
      {
         existed = TRUE ;
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
      clsTask *pTmpTask = NULL ;

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

         rc = clsNewTask( taskObj, pTmpTask ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initialize task, rc: %d",
                      rc ) ;

         if ( pTask->taskID() == pTmpTask->taskID() ||
              pTask->muteXOn( pTmpTask ) || pTmpTask->muteXOn( pTask ) )
         {
            conflict = TRUE ;
            PD_LOG( PDWARNING, "Exist task[%s] conflict with current "
                    "task[%s]", pTmpTask->toBson().toString().c_str(),
                    pTask->toBson().toString().c_str() ) ;
            break ;
         }

         clsReleaseTask( pTmpTask ) ;
      }

   done:
      if ( pTmpTask )
      {
         clsReleaseTask( pTmpTask ) ;
      }
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

      // check dst group name is same with source name
      PD_CHECK( 0 != dstName.compare( srcName ),
                SDB_INVALIDARG, error, PDERROR,
                "Target group name can not the same with source group name" ) ;

      // get source id
      rc = catGroupName2ID( srcName.c_str(), srcGroupID, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert group name [%s] to id, rc: %d",
                   srcName.c_str(), rc ) ;

      // check the collection is in source id
      PD_CHECK( cataSet.isInGroup( srcGroupID ),
                SDB_CL_NOT_EXIST_ON_GROUP, error, PDWARNING,
                "The collection [%s] does not exist on source group [%s]",
                cataSet.name(), srcName.c_str() ) ;

      // get target id
      rc = catGroupName2ID( dstName.c_str(), dstGroupID, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert group name [%s] to id, rc: %d",
                   dstName.c_str(), rc ) ;

      // check dst is in cs domain
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

      // Lock objects to check available for updates only
      // Unlock immediately for long task, which make sure subsequent commands
      // e.g. DropCL, have higher priority to execute on the same collection
      catCtxLockMgr lockMgr ;

      // Get and lock collection catalog info
      // Lock for shared, so could be split in parallel
      rc = catGetAndLockCollection( clName, boCollection, cb,
                                    needLock ? &lockMgr : NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute splitCL [%d]: "
                   "Failed to get the collection [%s]",
                   opCode, clName.c_str() ) ;

      if ( needLock )
      {
         PD_CHECK( lockMgr.tryLockCollectionSharding( clName, SHARED ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock collection [%s] for sharding",
                   clName.c_str() ) ;
      }

      // Update catalog set
      rc = cataSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute splitCL [%d] on [%s]: "
                   "Failed to update catalog set, cata info: %s, rc: %d",
                   opCode, clName.c_str(),
                   boCollection.toString().c_str(), rc ) ;

      // Collection must be sharding
      PD_CHECK( cataSet.isSharding(),
                SDB_COLLECTION_NOTSHARD, error, PDERROR,
                "Failed to execute splitCL [%d] on [%s]: "
                "Could not split non-sharding collection",
                opCode, clName.c_str() ) ;

      // Main-collection can't be split
      PD_CHECK( !cataSet.isMainCL(),
                SDB_MAIN_CL_OP_ERR, error, PDERROR,
                "Failed to split step [%d] on [%s]: "
                "Could not split main-collection",
                opCode, clName.c_str() ) ;

      // Could not split when no $id index.
      {
         UINT32 attribute = cataSet.getAttribute() ;
         if ( OSS_BIT_TEST( attribute, DMS_MB_ATTR_NOIDINDEX ) )
         {
            rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
            PD_LOG( PDERROR, "Failed to split step [%d] on [%s]: "
                    "Could not split collection when AutoIndexId is false",
                    opCode, clName.c_str() ) ;
            goto error ;
         }
      }

      mainCLName = cataSet.getMainCLName() ;
      if ( !mainCLName.empty() )
      {
         BSONObj dummy ;
         // Lock for shared, so could be split in parallel
         // main-collection's version will be updated
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

         // Extract collection name from query
         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         // Initialize catalog set
         clsCatalogSet cataSet( clName.c_str() ) ;

         // Check and lock collections and groups for split task
         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_PREPARE_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         // Get split query
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

         // split query or percent validation
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
                         BOOLEAN needLock, pmdEDUCB *cb, INT16 w,
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

         // Extract collection name from query
         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         // Initialize catalog set
         clsCatalogSet cataSet( clName.c_str() ) ;

         // Check and lock collections and groups for split task
         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_READY_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, needLock ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         // Get split value
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

         // bKey and eKey validation
         if ( !usePercent )
         {
            rc = _checkSplitKey( bKey, cataSet, srcGroupID, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Check split key failed, rc: %d", rc ) ;

            rc = _checkSplitKey( eKey, cataSet, srcGroupID, TRUE, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Check split end key failed, rc: %d", rc ) ;
         }

         // Create task
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

         // check task conflict
         match = BSON( CAT_COLLECTION_NAME << splitTask.collectionName() <<
                       CAT_STATUS_NAME <<
                       BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;
         rc = _catSplitCheckConflict( match, splitTask, conflict, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Check task conflict failed, rc: %d",
                      rc ) ;
         PD_CHECK( FALSE == conflict,
                   SDB_CLS_MUTEX_TASK_EXIST, error, PDERROR,
                   "Exist task not compatible with the new task" ) ;

         // add to task collection
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

   static INT32 _chgMetaInTask( const BSONObj& taskObj,
                                BOOLEAN removeSrcGroup, BOOLEAN addDstGroup,
                                const CHAR* srcGroupName,
                                const CHAR* dstGroupName,
                                pmdEDUCB* cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj updator, matcher, selector, dummyObj, groupObj, mainObj ;
      clsTask* pTask = NULL ;
      clsTask* pMainTask = NULL ;
      clsIdxTask* pIdxTask = NULL ;
      clsIdxTask* pMainIdxTask = NULL ;
      catCMDBase* pCommand = NULL ;
      catCMDBase* pMainCommand = NULL ;
      ossPoolVector<BSONObj> subTaskInfoList ;
      INT64 contextID = -1 ;
      pmdKRCB* krcb = pmdGetKRCB() ;
      SDB_DMSCB* dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB* rtnCB = krcb->getRTNCB() ;

      // 1. new task
      rc = clsNewTask( taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to initial task, rc: %d",
                   rc ) ;
      PD_CHECK( CLS_TASK_CREATE_IDX == pTask->taskType() ||
                CLS_TASK_DROP_IDX   == pTask->taskType(),
                SDB_SYS, error, PDERROR,
                "Invalid task type: %d", pTask->taskType() ) ;

      pIdxTask = (clsIdxTask*)pTask ;

      // 2. migrate task for split
      rc = pIdxTask->buildMigrateGroup( srcGroupName, dstGroupName,
                                        updator, matcher ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to migrate group from[%s] to [%s] in task[%d], "
                   "rc: %d", srcGroupName, dstGroupName, pTask->taskID(), rc ) ;

      rc = catUpdateTask( matcher, updator, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task, rc: %d",
                   rc ) ;
      PD_LOG( PDDEBUG,
              "migrate group, update task, matcher: %s, updator: %s",
              matcher.toString().c_str(), updator.toString().c_str() ) ;

      // 3. remove/add group in this task
      //    Add group before remove group, because remove group may cause group
      //    count to be 0, so task status will change to Finished.
      if ( addDstGroup )
      {
         matcher = BSONObj() ;
         updator = BSONObj() ;

         rc = pIdxTask->buildAddGroup( dstGroupName, updator, matcher ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add group[%s] in task[%d], rc: %d",
                      dstGroupName, pTask->taskID(), rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task, rc: %d",
                      rc ) ;
         PD_LOG( PDDEBUG,
                 "add group, update task, matcher: %s, updator: %s",
                 matcher.toString().c_str(), updator.toString().c_str() ) ;
      }
      if ( removeSrcGroup )
      {
         matcher = BSONObj() ;
         updator = BSONObj() ;

         rc = pIdxTask->buildRemoveGroup( srcGroupName, updator, matcher ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove group[%s] in task[%d], rc: %d",
                      srcGroupName, pTask->taskID(), rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task, rc: %d",
                      rc ) ;
         PD_LOG( PDDEBUG,
                 "remove group, update task, matcher: %s, updator: %s",
                 matcher.toString().c_str(), updator.toString().c_str() ) ;
      }

      // 4. if task finish, then we need to update metadata
      if ( CLS_TASK_STATUS_FINISH == pTask->status() )
      {
         if ( pTask->commandName() )
         {
            rc = getCatCmdBuilder()->create( pTask->commandName(),
                                             pCommand ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Failed to create command[%s], rc: %d",
                          pTask->commandName(), rc ) ;

            rc = pCommand->postDoit( pTask, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to post doit for command[%s], rc: %d",
                         pTask->commandName(), rc ) ;
         }
      }

      // 5. remove/add group in this task's main-task
      if ( pTask->hasMainTask() )
      {
         // 5.1 new main-task
         rc = catGetTask( pTask->mainTaskID(), mainObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task[%llu] object, rc: %d",
                      pTask->mainTaskID(), rc ) ;

         rc = clsNewTask( mainObj, pMainTask ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initial task[%llu], rc: %d",
                      pTask->mainTaskID(), rc ) ;
         PD_CHECK( CLS_TASK_CREATE_IDX == pTask->taskType() ||
                   CLS_TASK_DROP_IDX   == pTask->taskType() ||
                   CLS_TASK_COPY_IDX   == pTask->taskType(),
                   SDB_SYS, error, PDERROR,
                   "Invalid task type: %d", pTask->taskType() ) ;

         pMainIdxTask = (clsIdxTask*)pMainTask ;

         // 5.2 migrate task for split
         matcher = BSONObj() ;
         updator = BSONObj() ;

         rc = pMainIdxTask->buildMigrateGroup( srcGroupName, dstGroupName,
                                               updator, matcher ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to migrate group from[%s] to [%s] in main task[%d], "
                      "rc: %d", srcGroupName, dstGroupName, pMainIdxTask->taskID(), rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task, rc: %d",
                      rc ) ;
         PD_LOG( PDDEBUG,
                 "migrate group, update task, matcher: %s, updator: %s",
                 matcher.toString().c_str(), updator.toString().c_str() ) ;

         // 5.3 add group
         if ( addDstGroup )
         {
            matcher = BSONObj() ;
            updator = BSONObj() ;

            rc = pMainIdxTask->buildAddGroupBy( pTask, dstGroupName,
                                                updator, matcher ) ;
            PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove group[%s] in task[%d], rc: %d",
                      dstGroupName, pMainTask->taskID(), rc ) ;

            rc = catUpdateTask( matcher, updator, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task, rc: %d",
                         rc ) ;
            PD_LOG( PDDEBUG,
                    "add group, update task, matcher: %s, updator: %s",
                    matcher.toString().c_str(), updator.toString().c_str() ) ;

         }

         // 5.4 remove group
         if ( removeSrcGroup )
         {
            matcher = BSONObj() ;

            try
            {
               groupObj = BSON( FIELD_NAME_GROUPNAME << srcGroupName ) ;
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
            }

            rc = pMainTask->buildQuerySubTasks( groupObj, matcher, selector ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get subtask's matcher and selector, rc: %d",
                         rc ) ;

            // query sub-task info
            if ( !matcher.isEmpty() )
            {
               rc = rtnQuery( CAT_TASK_INFO_COLLECTION, selector, matcher,
                              dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                              contextID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to query collection[%s]: "
                            "matcher: %s, rc: %d", CAT_TASK_INFO_COLLECTION,
                            matcher.toString().c_str(), rc ) ;
            }

            // get more
            while ( TRUE )
            {
               rtnContextBuf contextBuf ;
               rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

               try
               {
                  BSONObj obj( contextBuf.data() ) ;
                  subTaskInfoList.push_back( obj.getOwned() ) ;
               }
               catch( std::exception &e )
               {
                  rc = ossException2RC( &e ) ;
                  PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
               }
            }

            matcher = BSONObj() ;
            updator = BSONObj() ;

            rc = pMainIdxTask->buildRemoveGroupBy( pTask, subTaskInfoList,
                                                   srcGroupName,
                                                   updator, matcher ) ;
            PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove group[%s] in task[%d], rc: %d",
                      srcGroupName, pMainTask->taskID(), rc ) ;

            rc = catUpdateTask( matcher, updator, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task, rc: %d",
                         rc ) ;
            PD_LOG( PDDEBUG,
                    "remove group, update task, matcher: %s, updator: %s",
                    matcher.toString().c_str(), updator.toString().c_str() ) ;
         }

         // 5.5 if task finish, then we need to update metadata
         if ( CLS_TASK_STATUS_FINISH == pMainTask->status() )
         {
            if ( pMainTask->commandName() )
            {
               rc = getCatCmdBuilder()->create( pMainTask->commandName(),
                                                pMainCommand ) ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Failed to create command[%s], rc: %d",
                             pMainTask->commandName(), rc ) ;

               rc = pMainCommand->postDoit( pMainTask, cb ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to post doit for command[%s], rc: %d",
                            pMainTask->commandName(), rc ) ;
            }
         }
      }

   done :
      if ( pTask )
      {
         clsReleaseTask( pTask ) ;
      }
      if ( pMainTask )
      {
         clsReleaseTask( pMainTask ) ;
      }
      if ( pCommand )
      {
         getCatCmdBuilder()->release( pCommand ) ;
      }
      if ( pMainCommand )
      {
         getCatCmdBuilder()->release( pMainCommand ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   static INT32 _chgMetaInOtherTasks( const clsCatalogSet& cataSet,
                                      BOOLEAN orgInSrcGroup,
                                      BOOLEAN orgInDstGroup,
                                      UINT32 srcGroupID, const CHAR* srcName,
                                      UINT32 dstGroupID, const CHAR* dstName,
                                      pmdEDUCB* cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher, dummyObj, taskObj ;
      INT64 contextID = -1 ;
      BOOLEAN removeSrcGroup = FALSE ;
      BOOLEAN addDstGroup = FALSE ;
      pmdKRCB* krcb = pmdGetKRCB() ;
      SDB_DMSCB* dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB* rtnCB = krcb->getRTNCB() ;

      if ( orgInSrcGroup && !cataSet.isInGroup( srcGroupID ) )
      {
         removeSrcGroup = TRUE ;
      }
      if ( !orgInDstGroup && cataSet.isInGroup( dstGroupID ) )
      {
         addDstGroup = TRUE ;
      }
      if ( !removeSrcGroup && !addDstGroup )
      {
         // do nothing
         goto done ;
      }

      // query all relative tasks
      try
      {
         matcher = BSON( FIELD_NAME_NAME << cataSet.name() <<
                         FIELD_NAME_STATUS <<
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) <<
                         FIELD_NAME_TASKTYPE <<
                         BSON( "$in" << BSON_ARRAY( CLS_TASK_CREATE_IDX <<
                                                    CLS_TASK_DROP_IDX ) ) ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, matcher,
                     dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to query collection[%s], matcher: %s, rc: %d",
                   CAT_TASK_INFO_COLLECTION, matcher.toString().c_str(), rc ) ;

      // loop every task
      while ( TRUE )
      {
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            taskObj = BSONObj( contextBuf.data() ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }

         rc = _chgMetaInTask( taskObj, removeSrcGroup, addDstGroup,
                              srcName, dstName, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to change collection metadata for task, rc: %d",
                      rc ) ;
      }

   done :
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
         BOOLEAN clInSrcGroup = FALSE ;
         BOOLEAN clInDstGroup = FALSE ;
         BSONObj taskObj ;
         BSONObj cataInfo ;
         clsSplitTask splitTask( taskID ) ;

         // NOTE: Check the task status before locking Catalog objects

         // Get task info
         rc = _catGetSplitTask( taskID, splitTask, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get task [%llu] failed, rc: %d",
                      taskID, rc ) ;
         taskObj = splitTask.toBson() ;

         // already finished
         if ( CLS_TASK_STATUS_META    == splitTask.status() ||
              CLS_TASK_STATUS_CLEANUP == splitTask.status() ||
              CLS_TASK_STATUS_FINISH  == splitTask.status() )
         {
            goto done ;
         }
         else if ( CLS_TASK_STATUS_CANCELED == splitTask.status() )
         {
            rc = SDB_TASK_HAS_CANCELED ;
            goto error ;
         }

         PD_CHECK( CLS_TASK_STATUS_RUN == splitTask.status(),
                   SDB_CAT_TASK_STATUS_ERROR, error, PDERROR,
                   "Split task status error, task: %s",
                   taskObj.toString().c_str() ) ;

         // Extract collection name from query
         rc = rtnGetSTDStringElement( splitInfo, CAT_COLLECTION_NAME, clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute splitCL [%d]: "
                      "failed to get the field [%s] from query [%s]",
                      MSG_CAT_SPLIT_PREPARE_REQ, CAT_COLLECTION_NAME,
                      splitInfo.toString().c_str() ) ;

         PD_CHECK( 0 == clName.compare( splitTask.collectionName() ),
                   SDB_SYS, error, PDERROR,
                   "Task [%llu] split on different collection [%s] from "
                   "previous phase [%s]",
                   taskID, splitTask.collectionName(), clName.c_str() ) ;

         // Initialize catalog set
         clsCatalogSet cataSet( clName.c_str() ) ;

         // Check and lock collections and groups for split task
         rc = _catCheckAndLockForSplitTask ( MSG_CAT_SPLIT_CHGMETA_REQ, clName,
                                             cataSet, splitInfo, cb,
                                             srcGroupID, srcName,
                                             dstGroupID, dstName, FALSE ) ;

         clInSrcGroup = cataSet.isInGroup( splitTask.sourceID() ) ;
         clInDstGroup = cataSet.isInGroup( splitTask.dstID() ) ;

         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check and lock collections and groups, rc: %d",
                      rc ) ;

         if ( !cataSet.isKeyInGroup( splitTask.splitKeyObj(),
                                     splitTask.dstID() ) )
         {
            // again check bKey and eKey :
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

            // split
            rc = cataSet.split( splitTask.splitKeyObj(),
                                splitTask.splitEndKeyObj(),
                                splitTask.dstID(), splitTask.dstName() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Catalog split failed, rc: %d, catalog: %s, task obj: %s",
                         rc ,
                         cataSet.toCataInfoBson().toString().c_str(),
                         taskObj.toString().c_str() ) ;

            // save new catalog info
            cataInfo = cataSet.toCataInfoBson() ;
            rc = catUpdateCatalog( clName.c_str(), cataInfo, BSONObj(), cb, w ) ;
            PD_RC_CHECK( rc, PDSEVERE,
                         "Failed to update collection catalog, rc: %d",
                         rc ) ;

            PD_LOG( PDDEBUG,
                    "Split task [%llu] chgmeta step: updated collection [%s]",
                    taskID, clName.c_str() ) ;

            mainCLName = cataSet.getMainCLName();
            if ( !mainCLName.empty() )
            {
               // Increase main-collection's version
               BSONObj emptyObj;
               INT32 tmpRC = SDB_OK ;
               tmpRC = catUpdateCatalog( mainCLName.c_str(), emptyObj, emptyObj, cb, w ) ;
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

         // update task status
         rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_META, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task status, rc: %d",
                      rc ) ;

         // change collection metadata on relative tasks
         rc = _chgMetaInOtherTasks( cataSet,
                                    clInSrcGroup, clInDstGroup,
                                    splitTask.sourceID(), splitTask.sourceName(),
                                    splitTask.dstID(), splitTask.dstName(),
                                    cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to change collection metadata for relative tasks, "
                      "rc: %d", rc ) ;

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

      /// Check the status of task
      rc = catGetTaskStatus( taskID, status, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] status failed, rc: %d",
                   taskID, rc ) ;

      if ( CLS_TASK_STATUS_META == status )
      {
         rc = catUpdateTaskStatus( taskID, CLS_TASK_STATUS_CLEANUP, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] status [%d] -> [%d], rc: %d",
                      taskID, status, CLS_TASK_STATUS_CLEANUP, rc ) ;
      }
      else if ( CLS_TASK_STATUS_CLEANUP == status )
      {
         // do nothing
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
         rc = SDB_CAT_TASK_STATUS_ERROR ;
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
      INT32 status = CLS_TASK_STATUS_READY ;
      PD_TRACE_ENTRY ( SDB_CATSPLITFINISH ) ;

      PD_CHECK( taskID != CLS_INVALID_TASKID,
                SDB_INVALIDARG, error, PDERROR,
                "Invalid task ID for split task" ) ;

      /// Check the status of task
      rc = catGetTaskStatus( taskID, status, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] status failed, rc: %d",
                   taskID, rc ) ;

      if ( CLS_TASK_STATUS_CLEANUP == status )
      {
         rc = catUpdateTask2Finish( taskID, SDB_OK, cb ,w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] status [%d] -> [%d], rc: %d",
                      taskID, status, CLS_TASK_STATUS_FINISH, rc ) ;
      }
      else if ( CLS_TASK_STATUS_FINISH == status )
      {
         // do nothing
      }
      else if ( CLS_TASK_STATUS_CANCELED == status )
      {
         rc = catUpdateTask2Finish( taskID, SDB_TASK_HAS_CANCELED, cb ,w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] status [%d] -> [%d], rc: %d",
                      taskID, status, CLS_TASK_STATUS_FINISH, rc ) ;
      }
      else
      {
         PD_LOG( PDERROR,
                 "Task[%llu] status error in clean up step [%d]",
                 taskID, status ) ;
         rc = SDB_CAT_TASK_STATUS_ERROR ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Finished split finish step on task [%llu]",
              taskID ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATSPLITFINISH, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static INT32 _getAndNewTask( UINT64 taskID, pmdEDUCB* cb, clsTask*& pTask )
   {
      INT32 rc = SDB_OK ;
      BSONObj taskObj ;

      rc = catGetTask( taskID, taskObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get task[%llu] object, rc: %d",
                   taskID, rc ) ;

      rc = clsNewTask( taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to initial task[%llu], rc: %d",
                   taskID, rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATTASKSTART, "catTaskStart" )
   INT32 catTaskStart ( const BSONObj &boQuery, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATTASKSTART ) ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      clsTask* pTask = NULL ;
      clsTask* pMainTask = NULL ;
      BSONObj matcher, updator ;

      // 1. get taskID from message
      rc = rtnGetNumberLongElement( boQuery, FIELD_NAME_TASKID,
                                    (INT64 &)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s] from query[%s]",
                   FIELD_NAME_TASKID, boQuery.toString().c_str() ) ;

      // 2. get and init task
      rc = _getAndNewTask( taskID, cb, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get and initial task[%llu], rc: %d",
                   taskID, rc ) ;

      // 3. start task
      rc = pTask->buildStartTask( boQuery, updator, matcher ) ;
      PD_CHECK( SDB_OK == rc, rc, error_on_start, PDERROR,
                "Failed to start task[%llu], rc: %d",
                pTask->taskID(), rc ) ;

      if ( updator.isEmpty() )
      {
         // nothing changed, just goto done
         goto done ;
      }

      rc = catUpdateTask( matcher, updator, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task[%llu], rc: %d",
                   taskID, rc ) ;

      // 4. process it's main-task
      if ( pTask->hasMainTask() )
      {
         matcher = BSONObj() ;
         updator = BSONObj() ;

         rc = _getAndNewTask( pTask->mainTaskID(), cb, pMainTask ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task[%llu], rc: %d",
                      pTask->mainTaskID(), rc ) ;

         rc = pMainTask->buildStartTaskBy( pTask, boQuery, updator, matcher ) ;
         PD_CHECK( SDB_OK == rc, rc, error_on_start, PDERROR,
                   "Failed to start task[%llu], rc: %d",
                   pMainTask->taskID(), rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu], rc: %d",
                      pMainTask->taskID(), rc ) ;
      }

   done :
      if ( pTask )
      {
         clsReleaseTask( pTask ) ;
      }
      if ( pMainTask )
      {
         clsReleaseTask( pMainTask ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATTASKSTART, rc ) ;
      return rc ;
   error_on_start :
      if ( !updator.isEmpty() )
      {
         // rollback transaction before set task to finish + -243 error.
         rc = catTransEnd( rc, cb, sdbGetDPSCB() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to end transaction, rc: %d",
                      rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu], rc: %d",
                      taskID, rc ) ;
      }
      goto done ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATTASKCANCEL, "catTaskCancel" )
   INT32 catTaskCancel ( const BSONObj &boQuery, pmdEDUCB *cb,
                         INT16 w, UINT32 &returnGroupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATTASKCANCEL ) ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      clsTask* pTask = NULL ;
      clsTask* pMainTask = NULL ;
      clsTask* pSubTask = NULL ;
      BSONObj matcher, updator ;

      // 1. get task ID from message
      rc = rtnGetNumberLongElement( boQuery, CAT_TASKID_NAME,
                                    (INT64 &)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s] from query",
                   CAT_TASKID_NAME ) ;

      // 2. get and init task
      rc = _getAndNewTask( taskID, cb, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get and initial task[%llu], rc: %d",
                   taskID, rc ) ;

      // 3. cancel task
      rc = pTask->buildCancelTask( boQuery, updator, matcher ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to cancel task[%llu], rc: %d",
                   pTask->taskID(), rc ) ;

      if ( updator.isEmpty() )
      {
         // nothing changed, just goto done
         goto done ;
      }

      rc = catUpdateTask( matcher, updator, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task[%llu], rc: %d",
                   pTask->taskID(), rc ) ;

      // 4. process it's main-task
      if ( pTask->hasMainTask() )
      {
         matcher = BSONObj() ;
         updator = BSONObj() ;

         rc = _getAndNewTask( pTask->mainTaskID(), cb, pMainTask ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task[%llu], rc: %d",
                      pTask->mainTaskID(), rc ) ;

         rc = pMainTask->buildCancelTaskBy( pTask, updator, matcher ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to cancel task[%llu], rc: %d",
                      pMainTask->taskID(), rc ) ;

         rc = catUpdateTask( matcher, updator, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu], rc: %d",
                      pMainTask->taskID(), rc ) ;
      }

      // 5. process it's sub-tasks
      if ( pTask->isMainTask() )
      {
         ossPoolVector<UINT64> subTaskList ;
         rc = pTask->getSubTasks( subTaskList ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task[llu]'s sub-tasks, rc: %d",
                      pTask->taskID(), rc ) ;

         for( ossPoolVector<UINT64>::iterator it = subTaskList.begin() ;
              it != subTaskList.end() ; ++it )
         {
            matcher = BSONObj() ;
            updator = BSONObj() ;

            rc = _getAndNewTask( *it, cb, pSubTask ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get task[%llu], rc: %d",
                         *it, rc ) ;

            rc = pSubTask->buildCancelTask( boQuery, updator, matcher ) ;
            if ( SDB_TASK_ALREADY_FINISHED == rc ||
                 SDB_TASK_HAS_CANCELED == rc ||
                 SDB_TASK_ROLLBACK == rc )
            {
               // ignore error
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to cancel task[%llu], rc: %d",
                         pSubTask->taskID(), rc ) ;

            rc = catUpdateTask( matcher, updator, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task[%llu], rc: %d",
                         pSubTask->taskID(), rc ) ;

            clsReleaseTask( pSubTask ) ;
         }
      }

      // 6. get target id for split task
      if ( CLS_TASK_SPLIT == pTask->taskType() )
      {
         returnGroupID = ((clsSplitTask*)pTask)->dstID() ;
      }

   done :
      if ( pTask )
      {
         clsReleaseTask( pTask ) ;
      }
      if ( pMainTask )
      {
         clsReleaseTask( pMainTask ) ;
      }
      if ( pSubTask )
      {
         clsReleaseTask( pSubTask ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATTASKCANCEL, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}
